/* -*- C++ -*-
 *
 *  ONScripterLabel.cpp - Execution block parser of ONScripter
 *
 *  Copyright (c) 2001-2008 Ogapee. All rights reserved.
 *
 *  ogapee@aqua.dti2.ne.jp
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Modified by Haeleth, autumn 2006, to remove unnecessary diagnostics and support OS X/Linux packaging better.

// Modified by Mion of Sonozaki Futago-tachi, March 2008, to update from
// Ogapee's 20080121 release source code.

#include "ONScripterLabel.h"
#include <cstdio>

#ifdef MACOSX
#include <libgen.h>
namespace Carbon {
#include <sys/stat.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>
}
#endif
#ifdef WIN32
#include <windows.h>
typedef HRESULT (WINAPI *GETFOLDERPATH)(HWND, int, HANDLE, DWORD, LPTSTR);
#endif
#ifdef LINUX
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#endif

extern void initSJIS2UTF16();
extern "C" void waveCallback( int channel );

#define DEFAULT_AUDIOBUF  4096

#define FONT_FILE "default.ttf"
#define REGISTRY_FILE "registry.txt"
#define DLL_FILE "dll.txt"
#ifdef HAELETH
#define DEFAULT_ENV_FONT "MS Gothic"
#else
#define DEFAULT_ENV_FONT "ＭＳ ゴシック"
#endif

typedef int (ONScripterLabel::*FuncList)();
static struct FuncLUT{
    char command[40];
    FuncList method;
} func_lut[] = {
    {"wavestop",   &ONScripterLabel::wavestopCommand},
    {"waveloop",   &ONScripterLabel::waveCommand},
    {"wave",   &ONScripterLabel::waveCommand},
    {"waittimer",   &ONScripterLabel::waittimerCommand},
    {"wait",   &ONScripterLabel::waitCommand},
    {"vsp2",   &ONScripterLabel::vspCommand}, //Mion - ogapee2008
    {"vsp",   &ONScripterLabel::vspCommand},
    {"voicevol",   &ONScripterLabel::voicevolCommand},
    {"trap",   &ONScripterLabel::trapCommand},
    {"textspeed",   &ONScripterLabel::textspeedCommand},
    {"textshow",   &ONScripterLabel::textshowCommand},
    {"texton",   &ONScripterLabel::textonCommand},
    {"textoff",   &ONScripterLabel::textoffCommand},
    {"texthide",   &ONScripterLabel::texthideCommand},
    {"textexbtn",   &ONScripterLabel::textexbtnCommand},
    {"textclear",   &ONScripterLabel::textclearCommand},
    {"textbtnwait",   &ONScripterLabel::btnwaitCommand},
    {"textbtnstart",   &ONScripterLabel::textbtnstartCommand},
    {"textbtnoff",   &ONScripterLabel::textbtnoffCommand},
    {"texec",   &ONScripterLabel::texecCommand},
    {"tateyoko",   &ONScripterLabel::tateyokoCommand},
    {"tal", &ONScripterLabel::talCommand},
    {"tablegoto",   &ONScripterLabel::tablegotoCommand},
    {"systemcall",   &ONScripterLabel::systemcallCommand},
    {"strsp",   &ONScripterLabel::strspCommand},
    {"stop",   &ONScripterLabel::stopCommand},
    {"sp_rgb_gradation",   &ONScripterLabel::sp_rgb_gradationCommand},
    {"spstr",   &ONScripterLabel::spstrCommand},
    {"spreload",   &ONScripterLabel::spreloadCommand},
    {"splitstring",   &ONScripterLabel::splitCommand},
    {"split",   &ONScripterLabel::splitCommand},
    {"spclclk",   &ONScripterLabel::spclclkCommand},
    {"spbtn",   &ONScripterLabel::spbtnCommand},
    {"skipoff",   &ONScripterLabel::skipoffCommand},
    {"shell",   &ONScripterLabel::shellCommand},
    {"sevol",   &ONScripterLabel::sevolCommand},
    {"setwindow3",   &ONScripterLabel::setwindow3Command},
    {"setwindow2",   &ONScripterLabel::setwindow2Command},
    {"setwindow",   &ONScripterLabel::setwindowCommand},
    {"setlayer", &ONScripterLabel::setlayerCommand},
    {"setcursor",   &ONScripterLabel::setcursorCommand},
    {"selnum",   &ONScripterLabel::selectCommand},
    {"selgosub",   &ONScripterLabel::selectCommand},
    {"selectbtnwait", &ONScripterLabel::btnwaitCommand},
    {"select",   &ONScripterLabel::selectCommand},
    {"savetime",   &ONScripterLabel::savetimeCommand},
    {"savescreenshot2",   &ONScripterLabel::savescreenshotCommand},
    {"savescreenshot",   &ONScripterLabel::savescreenshotCommand},
    {"saveon",   &ONScripterLabel::saveonCommand},
    {"saveoff",   &ONScripterLabel::saveoffCommand},
    {"savegame2",   &ONScripterLabel::savegameCommand},
    {"savegame",   &ONScripterLabel::savegameCommand},
    {"savefileexist",   &ONScripterLabel::savefileexistCommand},
    {"rnd",   &ONScripterLabel::rndCommand},
    {"rnd2",   &ONScripterLabel::rndCommand},
    {"rmode",   &ONScripterLabel::rmodeCommand},
    {"resettimer",   &ONScripterLabel::resettimerCommand},
    {"resetmenu", &ONScripterLabel::resetmenuCommand},
    {"reset",   &ONScripterLabel::resetCommand},
    {"repaint",   &ONScripterLabel::repaintCommand},
    {"quakey",   &ONScripterLabel::quakeCommand},
    {"quakex",   &ONScripterLabel::quakeCommand},
    {"quake",   &ONScripterLabel::quakeCommand},
    {"puttext",   &ONScripterLabel::puttextCommand},
    {"prnumclear",   &ONScripterLabel::prnumclearCommand},
    {"prnum",   &ONScripterLabel::prnumCommand},
    {"print",   &ONScripterLabel::printCommand},
    {"language", &ONScripterLabel::languageCommand},
    {"playstop",   &ONScripterLabel::playstopCommand},
    {"playonce",   &ONScripterLabel::playCommand},
    {"play",   &ONScripterLabel::playCommand},
    {"ofscpy", &ONScripterLabel::ofscopyCommand},
    {"ofscopy", &ONScripterLabel::ofscopyCommand},
    {"nega", &ONScripterLabel::negaCommand},
    {"msp2", &ONScripterLabel::mspCommand}, //Mion - ogapee2008
    {"msp", &ONScripterLabel::mspCommand},
    {"mpegplay", &ONScripterLabel::mpegplayCommand},
    {"mp3vol", &ONScripterLabel::mp3volCommand},
    {"mp3stop", &ONScripterLabel::playstopCommand},
    {"mp3save", &ONScripterLabel::mp3Command},
    {"mp3loop", &ONScripterLabel::mp3Command},
#ifdef INSANI
    {"mp3fadeout", &ONScripterLabel::mp3fadeoutCommand},
#endif
    {"mp3", &ONScripterLabel::mp3Command},
    {"movemousecursor", &ONScripterLabel::movemousecursorCommand},
    {"monocro", &ONScripterLabel::monocroCommand},
    {"menu_window", &ONScripterLabel::menu_windowCommand},
    {"menu_full", &ONScripterLabel::menu_fullCommand},
    {"menu_automode", &ONScripterLabel::menu_automodeCommand},
    {"lsph2", &ONScripterLabel::lsp2Command}, //Mion - ogapee2008
    {"lsph", &ONScripterLabel::lspCommand},
    {"lsp2", &ONScripterLabel::lsp2Command}, //Mion - ogapee2008
    {"lsp", &ONScripterLabel::lspCommand},
    {"lr_trap",   &ONScripterLabel::trapCommand},
    {"loopbgmstop", &ONScripterLabel::loopbgmstopCommand},
    {"loopbgm", &ONScripterLabel::loopbgmCommand},
    {"lookbackflush", &ONScripterLabel::lookbackflushCommand},
    {"lookbackbutton",      &ONScripterLabel::lookbackbuttonCommand},
    {"logsp2", &ONScripterLabel::logspCommand},
    {"logsp", &ONScripterLabel::logspCommand},
    {"locate", &ONScripterLabel::locateCommand},
    {"loadgame", &ONScripterLabel::loadgameCommand},
    {"linkcolor", &ONScripterLabel::linkcolorCommand},
    {"ld", &ONScripterLabel::ldCommand},
    {"layermessage", &ONScripterLabel::layermessageCommand},
    {"jumpf", &ONScripterLabel::jumpfCommand},
    {"jumpb", &ONScripterLabel::jumpbCommand},
    {"isfull", &ONScripterLabel::isfullCommand},
    {"isskip", &ONScripterLabel::isskipCommand},
    {"ispage", &ONScripterLabel::ispageCommand},
    {"isdown", &ONScripterLabel::isdownCommand},
    {"insertmenu", &ONScripterLabel::insertmenuCommand},
    {"input", &ONScripterLabel::inputCommand},
    {"indent", &ONScripterLabel::indentCommand},
    {"humanorder", &ONScripterLabel::humanorderCommand},
    {"getzxc", &ONScripterLabel::getzxcCommand},
    {"getvoicevol", &ONScripterLabel::getvoicevolCommand},
    {"getversion", &ONScripterLabel::getversionCommand},
    {"gettimer", &ONScripterLabel::gettimerCommand},
    {"getspsize", &ONScripterLabel::getspsizeCommand},
    {"getspmode", &ONScripterLabel::getspmodeCommand},
    {"getsevol", &ONScripterLabel::getsevolCommand},
    {"getscreenshot", &ONScripterLabel::getscreenshotCommand},
    {"gettextbtnstr", &ONScripterLabel::gettextbtnstrCommand},
    {"gettext", &ONScripterLabel::gettextCommand},
    {"gettaglog", &ONScripterLabel::gettaglogCommand},
    {"gettag", &ONScripterLabel::gettagCommand},
    {"gettab", &ONScripterLabel::gettabCommand},
    {"getret", &ONScripterLabel::getretCommand},
    {"getreg", &ONScripterLabel::getregCommand},
    {"getpageup", &ONScripterLabel::getpageupCommand},
    {"getpage", &ONScripterLabel::getpageCommand},
    {"getmp3vol", &ONScripterLabel::getmp3volCommand},
    {"getmousepos", &ONScripterLabel::getmouseposCommand},
    {"getlog", &ONScripterLabel::getlogCommand},
    {"getinsert", &ONScripterLabel::getinsertCommand},
    {"getfunction", &ONScripterLabel::getfunctionCommand},
    {"getenter", &ONScripterLabel::getenterCommand},
    {"getcursorpos", &ONScripterLabel::getcursorposCommand},
    {"getcursor", &ONScripterLabel::getcursorCommand},
    {"getcselstr", &ONScripterLabel::getcselstrCommand},
    {"getcselnum", &ONScripterLabel::getcselnumCommand},
    {"getbtntimer", &ONScripterLabel::gettimerCommand},
    {"getbgmvol", &ONScripterLabel::getmp3volCommand},
    {"game", &ONScripterLabel::gameCommand},
    {"fileexist", &ONScripterLabel::fileexistCommand},
    {"existspbtn", &ONScripterLabel::spbtnCommand},
    {"exec_dll", &ONScripterLabel::exec_dllCommand},
    {"exbtn_d", &ONScripterLabel::exbtnCommand},
    {"exbtn", &ONScripterLabel::exbtnCommand},
    {"erasetextwindow", &ONScripterLabel::erasetextwindowCommand},
    {"erasetextbtn", &ONScripterLabel::erasetextbtnCommand},
    {"end", &ONScripterLabel::endCommand},
    {"dwavestop", &ONScripterLabel::dwavestopCommand},
    {"dwaveplayloop", &ONScripterLabel::dwaveCommand},
    {"dwaveplay", &ONScripterLabel::dwaveCommand},
    {"dwaveloop", &ONScripterLabel::dwaveCommand},
    {"dwaveload", &ONScripterLabel::dwaveCommand},
    {"dwave", &ONScripterLabel::dwaveCommand},
    {"drawtext", &ONScripterLabel::drawtextCommand},
    {"drawsp3", &ONScripterLabel::drawsp3Command},
    {"drawsp2", &ONScripterLabel::drawsp2Command},
    {"drawsp", &ONScripterLabel::drawspCommand},
    {"drawfill", &ONScripterLabel::drawfillCommand},
    {"drawclear", &ONScripterLabel::drawclearCommand},
    {"drawbg2", &ONScripterLabel::drawbg2Command},
    {"drawbg", &ONScripterLabel::drawbgCommand},
    {"draw", &ONScripterLabel::drawCommand},
    {"deletescreenshot", &ONScripterLabel::deletescreenshotCommand},
    {"delay", &ONScripterLabel::delayCommand},
    {"definereset", &ONScripterLabel::defineresetCommand},
    {"csp2", &ONScripterLabel::cspCommand}, //Mion - ogapee2008
    {"csp", &ONScripterLabel::cspCommand},
    {"cselgoto", &ONScripterLabel::cselgotoCommand},
    {"cselbtn", &ONScripterLabel::cselbtnCommand},
    {"csel", &ONScripterLabel::selectCommand},
    {"click", &ONScripterLabel::clickCommand},
    {"cl", &ONScripterLabel::clCommand},
    {"chvol", &ONScripterLabel::chvolCommand},
    {"checkpage", &ONScripterLabel::checkpageCommand},
    {"cellcheckspbtn", &ONScripterLabel::spbtnCommand},
    {"cellcheckexbtn", &ONScripterLabel::exbtnCommand},
    {"cell", &ONScripterLabel::cellCommand},
    {"caption", &ONScripterLabel::captionCommand},
    {"btnwait2", &ONScripterLabel::btnwaitCommand},
    {"btnwait", &ONScripterLabel::btnwaitCommand},
    {"btntime2", &ONScripterLabel::btntimeCommand},
    {"btntime", &ONScripterLabel::btntimeCommand},
    {"btndown",  &ONScripterLabel::btndownCommand},
    {"btndef",  &ONScripterLabel::btndefCommand},
    {"btn",     &ONScripterLabel::btnCommand},
    {"br",      &ONScripterLabel::brCommand},
    {"blt",      &ONScripterLabel::bltCommand},
    {"bgmvol", &ONScripterLabel::mp3volCommand},
    {"bgmstop", &ONScripterLabel::playstopCommand},
    {"bgmonce", &ONScripterLabel::mp3Command},
    {"bgm", &ONScripterLabel::mp3Command},
    {"bgcpy",      &ONScripterLabel::bgcopyCommand},
    {"bgcopy",      &ONScripterLabel::bgcopyCommand},
    {"bg",      &ONScripterLabel::bgCommand},
    {"barclear",      &ONScripterLabel::barclearCommand},
    {"bar",      &ONScripterLabel::barCommand},
    {"avi",      &ONScripterLabel::aviCommand},
    {"automode_time",      &ONScripterLabel::automode_timeCommand},
    {"autoclick",      &ONScripterLabel::autoclickCommand},
    {"amsp2",      &ONScripterLabel::amspCommand}, //Mion - ogapee2008
    {"amsp",      &ONScripterLabel::amspCommand},
    {"allsp2resume",      &ONScripterLabel::allsp2resumeCommand}, //Mion - ogapee2008
    {"allspresume",      &ONScripterLabel::allspresumeCommand},
    {"allsp2hide",      &ONScripterLabel::allsp2hideCommand}, //Mion - ogapee2008
    {"allsphide",      &ONScripterLabel::allsphideCommand},
    {"abssetcursor", &ONScripterLabel::setcursorCommand},
    {"", NULL}
};

static void SDL_Quit_Wrapper()
{
    SDL_Quit();
}

void ONScripterLabel::initSDL()
{
    /* ---------------------------------------- */
    /* Initialize SDL */

    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO ) < 0 ){
        fprintf( stderr, "Couldn't initialize SDL: %s\n", SDL_GetError() );
        exit(-1);
    }
    atexit(SDL_Quit_Wrapper); // work-around for OS/2

    if( cdaudio_flag && SDL_InitSubSystem( SDL_INIT_CDROM ) < 0 ){
        fprintf( stderr, "Couldn't initialize CD-ROM: %s\n", SDL_GetError() );
        exit(-1);
    }

#ifndef HAELETH
    if(SDL_InitSubSystem( SDL_INIT_JOYSTICK ) == 0 && SDL_JoystickOpen(0) != NULL)
        printf( "Initialize JOYSTICK\n");
#endif

#if defined(PSP) || defined(IPODLINUX)
    SDL_ShowCursor(SDL_DISABLE);
#endif

    /* ---------------------------------------- */
    /* Initialize SDL */
    if ( TTF_Init() < 0 ){
        fprintf( stderr, "can't initialize SDL TTF\n");
        exit(-1);
    }

#ifdef INSANI
	SDL_WM_SetIcon(IMG_Load("icon.png"), NULL);
	//fprintf(stderr, "Autodetect: insanity spirit detected!\n");
#endif

#ifdef BPP16
    screen_bpp = 16;
#else
    screen_bpp = 32;
#endif

#if defined(PDA) && defined(PDA_WIDTH)
    screen_ratio1 *= PDA_WIDTH;
    screen_ratio2 *= 320;
    screen_width   = screen_width  * PDA_WIDTH / 320;
    screen_height  = screen_height * PDA_WIDTH / 320;
#elif defined(PDA) && defined(PDA_AUTOSIZE)
    SDL_Rect **modes;
    modes = SDL_ListModes(NULL, 0);
    if (modes == (SDL_Rect **)0){
        fprintf(stderr, "No Video mode available.\n");
        exit(-1);
    }
    else if (modes == (SDL_Rect **)-1){
        // no restriction
    }
 	else{
        int width;
        if (modes[0]->w * 3 > modes[0]->h * 4)
            width = (modes[0]->h / 3) * 4;
        else
            width = (modes[0]->w / 4) * 4;
        screen_ratio1 *= width;
        screen_ratio2 *= 320;
        screen_width   = screen_width  * width / 320;
        screen_height  = screen_height * width / 320;
    }
#endif

#ifdef RCA_SCALE
    scr_stretch_x = 1.0;
    scr_stretch_y = 1.0;

    if (scaled_flag) {
        const SDL_VideoInfo* info = SDL_GetVideoInfo();
        int native_width = info->current_w;
        int native_height = info->current_h;
        
        // Resize up to fill screen
        scr_stretch_x = (float)native_width / (float)screen_width;
        scr_stretch_y = (float)native_height / (float)screen_height;
        if (!widescreen_flag) {
            // Constrain aspect to same as game
            if (scr_stretch_x > scr_stretch_y) {
                scr_stretch_x = scr_stretch_y;
                screen_height = native_height;
                screen_width *= scr_stretch_x;
            } else { 
                scr_stretch_y = scr_stretch_x;
                screen_width = native_width;
                screen_height *= scr_stretch_y;
            }
        } else {
            screen_width = native_width;
            screen_height = native_height;
        }
    }
    else if (widescreen_flag) {
        const SDL_VideoInfo* info = SDL_GetVideoInfo();
        int native_width = info->current_w;
        int native_height = info->current_h;
        
        // Resize to screen aspect ratio
        const float screen_asp = (float)screen_width / (float)screen_height;
        const float native_asp = (float)native_width / (float)native_height;
        const float aspquot = native_asp / screen_asp;
        if (aspquot >1.01) {
            // Widescreen; make gamearea wider
            scr_stretch_x = (float)screen_height * native_asp / (float)screen_width;
            screen_width = screen_height * native_asp;
        } else if (aspquot < 0.99) {
            scr_stretch_y = (float)screen_width / native_asp / (float)screen_height;
            screen_height = screen_width / native_asp;
        }
    }

#endif
    screen_surface = SDL_SetVideoMode( screen_width, screen_height, screen_bpp, DEFAULT_VIDEO_SURFACE_FLAG|(fullscreen_mode?SDL_FULLSCREEN:0) );

    /* ---------------------------------------- */
    /* Check if VGA screen is available. */
#if defined(PDA) && (PDA_WIDTH==640)
    if ( screen_surface == NULL ){
        screen_ratio1 /= 2;
        screen_width  /= 2;
        screen_height /= 2;
        screen_surface = SDL_SetVideoMode( screen_width, screen_height, screen_bpp, DEFAULT_VIDEO_SURFACE_FLAG|(fullscreen_mode?SDL_FULLSCREEN:0) );
    }
#endif
    underline_value = screen_height - 1;

    if ( screen_surface == NULL ) {
        fprintf( stderr, "Couldn't set %dx%dx%d video mode: %s\n",
                 screen_width, screen_height, screen_bpp, SDL_GetError() );
        exit(-1);
    }
    //printf("Display: %d x %d (%d bpp)\n", screen_width, screen_height, screen_bpp);

    initSJIS2UTF16();

    wm_title_string = new char[ strlen(DEFAULT_WM_TITLE) + 1 ];
    memcpy( wm_title_string, DEFAULT_WM_TITLE, strlen(DEFAULT_WM_TITLE) + 1 );
    wm_icon_string = new char[ strlen(DEFAULT_WM_ICON) + 1 ];
    memcpy( wm_icon_string, DEFAULT_WM_TITLE, strlen(DEFAULT_WM_ICON) + 1 );
    SDL_WM_SetCaption( wm_title_string, wm_icon_string );

    openAudio();
}

void ONScripterLabel::openAudio(int freq, Uint16 format, int channels)
{
    if ( Mix_OpenAudio( freq, format, channels, DEFAULT_AUDIOBUF ) < 0 ){
        fprintf(stderr, "Couldn't open audio device!\n"
                "  reason: [%s].\n", SDL_GetError());
        audio_open_flag = false;
    }
    else{
        int freq;
        Uint16 format;
        int channels;

        Mix_QuerySpec( &freq, &format, &channels);
        //printf("Audio: %d Hz %d bit %s\n", freq,
        //       (format&0xFF),
        //       (channels > 1) ? "stereo" : "mono");
        audio_format.format = format;
        audio_format.freq = freq;
        audio_format.channels = channels;

        audio_open_flag = true;

        Mix_AllocateChannels( ONS_MIX_CHANNELS+ONS_MIX_EXTRA_CHANNELS );
        Mix_ChannelFinished( waveCallback );
    }
}

ONScripterLabel::ONScripterLabel()
{
#ifdef PNG_FORCE_NSCRIPTER_MASKS
    png_mask_type = PNG_MASK_USE_NSCRIPTER;
#elif defined PNG_FORCE_ALPHA_MASKS
    png_mask_type = PNG_MASK_USE_ALPHA;    
#else
    png_mask_type = PNG_MASK_AUTODETECT;
#endif
    cdrom_drive_number = 0;
    cdaudio_flag = false;
    default_font = NULL;
    registry_file = NULL;
    setStr( &registry_file, REGISTRY_FILE );
    dll_file = NULL;
    setStr( &dll_file, DLL_FILE );
    getret_str = NULL;
    enable_wheeldown_advance_flag = false;
    disable_rescale_flag = false;
    edit_flag = false;
    key_exe_file = NULL;
    fullscreen_mode = false;
    window_mode = false;
#ifdef INSANI
	skip_to_wait = 0;
#endif
    sprite_info  = new AnimationInfo[MAX_SPRITE_NUM];
    sprite2_info = new AnimationInfo[MAX_SPRITE2_NUM];

    int i;
    for (i=0 ; i<MAX_SPRITE2_NUM ; i++)
        sprite2_info[i].affine_flag = true;
    for (i=0 ; i<NUM_GLYPH_CACHE ; i++){
        if (i != NUM_GLYPH_CACHE-1) glyph_cache[i].next = &glyph_cache[i+1];
        glyph_cache[i].font = NULL;
        glyph_cache[i].surface = NULL;
    }
    glyph_cache[NUM_GLYPH_CACHE-1].next = NULL;
    root_glyph_cache = &glyph_cache[0];
    string_buffer_breaks = NULL;
    string_buffer_margins = NULL;
    line_has_nonspace = false;

    // External Players
    music_cmd = getenv("PLAYER_CMD");
    midi_cmd  = getenv("MUSIC_CMD");
}

ONScripterLabel::~ONScripterLabel()
{
    reset();

    delete[] sprite_info;
    delete[] sprite2_info;
}

void ONScripterLabel::enableCDAudio(){
    cdaudio_flag = true;
}

void ONScripterLabel::setCDNumber(int cdrom_drive_number)
{
    this->cdrom_drive_number = cdrom_drive_number;
}

void ONScripterLabel::setFontFile(const char *filename)
{
    setStr(&default_font, filename);
}

void ONScripterLabel::setRegistryFile(const char *filename)
{
    setStr(&registry_file, filename);
}

void ONScripterLabel::setDLLFile(const char *filename)
{
    setStr(&dll_file, filename);
}

void ONScripterLabel::setArchivePath(const char *path)
{
    if (archive_path) {
        delete archive_path;
        archive_path = NULL;
    }
    archive_path = new DirPaths(path);
    //printf("archive_path: %s\n", archive_path->get_all_paths());
}

void ONScripterLabel::setSavePath(const char *path)
{
    if (script_h.save_path) delete[] script_h.save_path;
    script_h.save_path = new char[ strlen(path) + 2 ];
    sprintf( script_h.save_path, "%s%c", path, DELIMITER );
}

void ONScripterLabel::setFullscreenMode()
{
    fullscreen_mode = true;
}

void ONScripterLabel::setWindowMode()
{
    window_mode = true;
}

void ONScripterLabel::enableButtonShortCut()
{
    force_button_shortcut_flag = true;
}

void ONScripterLabel::enableWheelDownAdvance()
{
    enable_wheeldown_advance_flag = true;
}

void ONScripterLabel::disableRescale()
{
    disable_rescale_flag = true;
}

void ONScripterLabel::enableEdit()
{
    edit_flag = true;
}

void ONScripterLabel::setKeyEXE(const char *filename)
{
    setStr(&key_exe_file, filename);
}

#ifdef RCA_SCALE
void ONScripterLabel::setWidescreen()
{
    widescreen_flag = true;
}

void ONScripterLabel::setScaled()
{
    scaled_flag = true;
}
#endif

int ONScripterLabel::init()
{
    if (archive_path == NULL) {
#ifdef MACOSX
    // On Mac OS X, store archives etc in the application bundle by default,
    // but fall back to the application root directory if bundle doesn't
    // contain any script files.
    using namespace Carbon;
    const int maxpath=32768;
    UInt8 path[maxpath];
    CFBundleRef bundle = CFBundleGetMainBundle();
    if (bundle) {
	archive_path = new DirPaths();
        CFURLRef resourceurl = CFBundleCopyResourcesDirectoryURL(bundle);
        if (resourceurl) {
            Boolean validpath =
		CFURLGetFileSystemRepresentation(resourceurl,true,path,maxpath);
            CFRelease(resourceurl);
            if (validpath) {
		// Verify the archive path by checking for the script file
		const char* scriptfiles[] =
		    {"0.txt","00.txt","nscr_sec.dat","nscript.___","nscript.dat",0};
		char** p = (char**) &scriptfiles;
		UInt8 test[maxpath];
		for(;*p;p++) {
		    sprintf((char*)test,"%s/%s",(const char*)path,*p);
		    
		    FSRef ref;
		    OSErr err = FSPathMakeRef(test, &ref, NULL);
		    if(err == noErr &&
		       FSGetCatalogInfo(&ref, kFSCatInfoNone, 0, 0, 0, 0) == noErr)
			break;
		}
            
		if (*p != NULL) {
		    archive_path->add((const char*) path);
		}
	    }
	}

	// Add the application path.  If there were script files in
	// the bundle, this becomes our fallback position, otherwise
	// it's our primary search path.
	CFURLRef bundleurl = CFBundleCopyBundleURL(bundle);
	if (bundleurl) {
	    CFURLRef archiveurl =
		CFURLCreateCopyDeletingLastPathComponent(kCFAllocatorDefault,
							 bundleurl);
	    if (archiveurl) {
		Boolean validpath =
		    CFURLGetFileSystemRepresentation(archiveurl, true, path,
						     maxpath);
		CFRelease(archiveurl);
		if (validpath) {
		    archive_path->add((const char*) path);

		    // If that was our primary search path, add the
		    // next directory up as our fallback position.
		    if (archive_path->get_num_paths() == 1) {
			strcat((char*) path, "/..");
			archive_path->add((const char*) path);
		    }
		}
	    }
	    CFRelease(bundleurl);
	}
    }
    else {
	// Not in a bundle: just use current dir and parent as normal.
	archive_path = new DirPaths(".");
	archive_path->add("..");
    }
#else
    // On Linux, the path is unpredictable and should be set by
    // using "-r PATH" in a launcher script.  On other platforms
    // they're stored in the same place as the executable.
    archive_path = new DirPaths(".");
    archive_path->add("..");
    //printf("init:archive_paths: \"%s\"\n", archive_path->get_all_paths());
#endif
    }
    
    if (key_exe_file){
        createKeyTable( key_exe_file );
        script_h.setKeyTable( key_table );
    }
    
    if ( open() ) return -1;

    if ( script_h.save_path == NULL ){
        char* gameid = script_h.game_identifier;
        char gamename[20];
        if (!gameid) {
            gameid=(char*)&gamename;
            sprintf(gameid,"ONScripter-%x",script_h.game_hash);
        }
#ifdef WIN32
	// On Windows, store in [Profiles]/All Users/Application Data.
	// TODO: optionally permit saves to be per-user rather than shared?
	HMODULE shdll = LoadLibrary("shfolder");
	if (shdll) {
	    GETFOLDERPATH gfp = GETFOLDERPATH(GetProcAddress(shdll, "SHGetFolderPathA"));
	    if (gfp) {
		char hpath[MAX_PATH];
#define CSIDL_COMMON_APPDATA 0x0023 // for [Profiles]/All Users/Application Data
#define CSIDL_APPDATA 0x001A // for [Profiles]/[User]/Application Data
		HRESULT res = gfp(0, CSIDL_COMMON_APPDATA, 0, 0, hpath);
		if (res != S_FALSE && res != E_FAIL && res != E_INVALIDARG) {
		    script_h.save_path = new char[strlen(hpath) + strlen(gameid) + 3];
		    sprintf(script_h.save_path, "%s%c%s%c",
                            hpath, DELIMITER, gameid, DELIMITER);
		    CreateDirectory(script_h.save_path, 0);
		}
	    }
	    FreeLibrary(shdll);
	}
	if (script_h.save_path == NULL) {
	    // Error; assume ancient Windows. In this case it's safe
	    // to use the archive path!
	    script_h.save_path = archive_path->get_path(0);
	}
#elif defined MACOSX
    // On Mac OS X, place in ~/Library/Application Support/<gameid>/
	using namespace Carbon;
    FSRef appsupport;
    FSFindFolder(kUserDomain, kApplicationSupportFolderType, kDontCreateFolder, &appsupport);
    char path[32768];
    FSRefMakePath(&appsupport, (UInt8*) path, 32768);
    script_h.save_path = new char[strlen(path) + strlen(gameid) + 2];
    sprintf(script_h.save_path, "%s%c%s%c", path, DELIMITER, gameid, DELIMITER);
	mkdir(script_h.save_path, 0755);
#elif defined LINUX
	// On Linux (and similar *nixen), place in ~/.gameid
	passwd* pwd = getpwuid(getuid());
	if (pwd) {
	    script_h.save_path = new char[strlen(pwd->pw_dir) + strlen(gameid) + 4];
	    sprintf(script_h.save_path, "%s%c.%s%c", 
                    pwd->pw_dir, DELIMITER, gameid, DELIMITER);
	    mkdir(script_h.save_path, 0755);
	}
	else script_h.save_path = archive_path->get_path(0);
#else
	// Fall back on default ONScripter behaviour if we don't have
	// any better ideas.
	script_h.save_path = archive_path->get_path(0);
#endif
    }
    if ( script_h.game_identifier ) {
	delete[] script_h.game_identifier; 
	script_h.game_identifier = NULL; 
    }

    if (strcmp(script_h.save_path,archive_path->get_path(0)) != 0) {
        // insert save_path onto the front of archive_path
	// The string returned by get_all_paths is deleted in ~DirPaths(), so the
	// simplest way to do this is to create the new object before deleting the
	// old one.
	DirPaths* new_paths = new DirPaths(script_h.save_path);
	new_paths->add(archive_path->get_all_paths());
	delete archive_path;
	archive_path = new_paths;
        ((DirectReader*) script_h.cBR)->setArchivePath(archive_path);
    }

    initSDL();

    image_surface = SDL_CreateRGBSurface( SDL_SWSURFACE, 1, 1, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 );

    accumulation_surface = AnimationInfo::allocSurface( screen_width, screen_height );
    accumulation_comp_surface = AnimationInfo::allocSurface( screen_width, screen_height );
    effect_src_surface   = AnimationInfo::allocSurface( screen_width, screen_height );
    effect_dst_surface   = AnimationInfo::allocSurface( screen_width, screen_height );
    effect_tmp_surface   = AnimationInfo::allocSurface( screen_width, screen_height );
    SDL_SetAlpha( accumulation_surface, 0, SDL_ALPHA_OPAQUE );
    SDL_SetAlpha( accumulation_comp_surface, 0, SDL_ALPHA_OPAQUE );
    SDL_SetAlpha( effect_src_surface, 0, SDL_ALPHA_OPAQUE );
    SDL_SetAlpha( effect_dst_surface, 0, SDL_ALPHA_OPAQUE );
    SDL_SetAlpha( effect_tmp_surface, 0, SDL_ALPHA_OPAQUE );
    screenshot_surface   = NULL;
    text_info.num_of_cells = 1;
    text_info.allocImage( screen_width, screen_height );
    text_info.fill(0, 0, 0, 0);

    // ----------------------------------------
    // Initialize font
    if ( default_font ){
        font_file = new char[ strlen(default_font) + 1 ];
        sprintf( font_file, "%s", default_font );
    }
    else{
        FILE *fp;
        font_file = new char[ archive_path->max_path_len() + strlen(FONT_FILE) + 1 ];
        for (int i=0; i<(archive_path->get_num_paths()); i++) {
            // look through archive_path(s) for the font file
            sprintf( font_file, "%s%s", archive_path->get_path(i), FONT_FILE );
            //printf("font file: %s\n", font_file);
            fp = std::fopen(font_file, "rb");
            if (fp != NULL) {
                fclose(fp);
                break;
            }
        }
        //sprintf( font_file, "%s%s", archive_path->get_path(0), FONT_FILE );
    }

    // ----------------------------------------
    // Sound related variables
    this->cdaudio_flag = cdaudio_flag;
    cdrom_info = NULL;
    if ( cdaudio_flag ){
        if ( cdrom_drive_number >= 0 && cdrom_drive_number < SDL_CDNumDrives() )
            cdrom_info = SDL_CDOpen( cdrom_drive_number );
        if ( !cdrom_info ){
            fprintf(stderr, "Couldn't open default CD-ROM: %s\n", SDL_GetError());
        }
        else if ( cdrom_info && !CD_INDRIVE( SDL_CDStatus( cdrom_info ) ) ) {
            fprintf( stderr, "no CD-ROM in the drive\n" );
            SDL_CDClose( cdrom_info );
            cdrom_info = NULL;
        }
    }

    wave_file_name = NULL;
    midi_file_name = NULL;
    midi_info  = NULL;
    mp3_sample = NULL;
    music_file_name = NULL;
    music_buffer = NULL;
    music_info = NULL;
    music_struct.ovi = NULL;

    loop_bgm_name[0] = NULL;
    loop_bgm_name[1] = NULL;

    int i;
    for (i=0 ; i<ONS_MIX_CHANNELS+ONS_MIX_EXTRA_CHANNELS ; i++) wave_sample[i] = NULL;

    // ----------------------------------------
    // Initialize misc variables

    internal_timer = SDL_GetTicks();

    trap_dist = NULL;
    resize_buffer = new unsigned char[16];
    resize_buffer_size = 16;

    for (i=0 ; i<MAX_PARAM_NUM ; i++) bar_info[i] = prnum_info[i] = NULL;

    defineresetCommand();
    readToken();

    if ( sentence_font.openFont( font_file, screen_ratio1, screen_ratio2 ) == NULL ){
#ifdef MACOSX
	unsigned char errmsg[2048];
	char* errstr = (char*) errmsg + 1;
	sprintf( errstr, "Could not find the font file '%s'. "
		 "Please ensure it is present with the game data.",
		 basename(font_file) );
	errmsg[0] = strlen( errstr );
	using namespace Carbon;
        StandardAlert( kAlertStopAlert, "\pMissing font file",
		       errmsg, NULL, NULL );
#else
        fprintf( stderr, "can't open font file: %s\n", font_file );

#endif
        return -1;
    }

    loadEnvData();

    return 0;
}

void ONScripterLabel::reset()
{
    automode_flag = false;
    automode_time = 3000;
    autoclick_time = 0;
    remaining_time = -1;
    btntime2_flag = false;
    btntime_value = 0;
    btnwait_time = 0;

    disableGetButtonFlag();

    system_menu_enter_flag = false;
    system_menu_mode = SYSTEM_NULL;
    key_pressed_flag = false;
    shift_pressed_status = 0;
    ctrl_pressed_status = 0;
    display_mode = NORMAL_DISPLAY_MODE;
    event_mode = IDLE_EVENT_MODE;
    all_sprite_hide_flag = false;
    all_sprite2_hide_flag = false;

    if (resize_buffer_size != 16){
        delete[] resize_buffer;
        resize_buffer = new unsigned char[16];
        resize_buffer_size = 16;
    }

    current_over_button = 0;
    variable_edit_mode = NOT_EDIT_MODE;

    new_line_skip_flag = false;
    text_on_flag = true;
    draw_cursor_flag = false;
    if (string_buffer_breaks) delete[] string_buffer_breaks;
    string_buffer_breaks = NULL;

    resetSentenceFont();

    setStr(&getret_str, NULL);
    getret_int = 0;

    // ----------------------------------------
    // Sound related variables

    wave_play_loop_flag = false;
    midi_play_loop_flag = false;
    music_play_loop_flag = false;
    cd_play_loop_flag = false;
    mp3save_flag = false;
    current_cd_track = -1;

    resetSub();

    /* ---------------------------------------- */
    /* Load global variables if available */
    if ( loadFileIOBuf( "gloval.sav" ) == 0 ||
         loadFileIOBuf( "global.sav" ) == 0 )
        readVariables( script_h.global_variable_border, VARIABLE_RANGE );
}

void ONScripterLabel::resetSub()
{
    int i;

    for ( i=0 ; i<script_h.global_variable_border ; i++ )
        script_h.variable_data[i].reset(false);

    for ( i=0 ; i<3 ; i++ ) human_order[i] = 2-i; // "rcl"

    refresh_shadow_text_mode = REFRESH_NORMAL_MODE | REFRESH_SHADOW_MODE | REFRESH_TEXT_MODE;
    erase_text_window_mode = 1;
    skip_flag = false;
    monocro_flag = false;
    nega_mode = 0;
    clickstr_state = CLICK_NONE;
    trap_mode = TRAP_NONE;
    setStr(&trap_dist, NULL);

    saveon_flag = true;
    internal_saveon_flag = true;

    textgosub_clickstr_state = CLICK_NONE;
    indent_offset = 0;
    line_enter_status = 0;
    page_enter_status = 0;

    resetSentenceFont();

    deleteNestInfo();
    deleteButtonLink();
    deleteSelectLink();

    stopCommand();
    loopbgmstopCommand();
    stopAllDWAVE(); //Mion - ogapee2008
    setStr(&loop_bgm_name[1], NULL);

    // ----------------------------------------
    // reset AnimationInfo
    btndef_info.reset();
    bg_info.reset();
    setStr( &bg_info.file_name, "black" );
    createBackground();
    for (i=0 ; i<3 ; i++) tachi_info[i].reset();
    for (i=0 ; i<MAX_SPRITE_NUM ; i++) sprite_info[i].reset();
    for (i=0 ; i<MAX_SPRITE2_NUM ; i++) sprite2_info[i].reset();
    barclearCommand();
    prnumclearCommand();
    for (i=0 ; i<2 ; i++) cursor_info[i].reset();
    for (i=0 ; i<4 ; i++) lookback_info[i].reset();
    sentence_font_info.reset();

    //Mion: reset textbtn
    deleteTextButtonInfo();
    readColor( &linkcolor[0], "#FFFF22" ); // yellow - link color
    readColor( &linkcolor[1], "#88FF88" ); // cyan - mouseover link color
    txtbtn_start_num = next_txtbtn_num = 1;
    in_txtbtn = false;
    txtbtn_show = false;
    txtbtn_visible = false;

    dirty_rect.fill( screen_width, screen_height );
}

void ONScripterLabel::resetSentenceFont()
{
    sentence_font.reset();
    sentence_font.font_size_xy[0] = DEFAULT_FONT_SIZE;
    sentence_font.font_size_xy[1] = DEFAULT_FONT_SIZE;
    sentence_font.top_xy[0] = 21;
    sentence_font.top_xy[1] = 16;// + sentence_font.font_size;
    sentence_font.num_xy[0] = 23;
    sentence_font.num_xy[1] = 16;
    sentence_font.pitch_xy[0] = sentence_font.font_size_xy[0];
    sentence_font.pitch_xy[1] = 2 + sentence_font.font_size_xy[1];
    sentence_font.wait_time = 20;
    sentence_font.window_color[0] = sentence_font.window_color[1] = sentence_font.window_color[2] = 0x99;
    sentence_font.color[0] = sentence_font.color[1] = sentence_font.color[2] = 0xff;
    sentence_font_info.pos.x = 0;
    sentence_font_info.pos.y = 0;
    sentence_font_info.pos.w = screen_width+1;
    sentence_font_info.pos.h = screen_height+1;
    deleteColorChanges();
    setColor(current_page_colors.color, sentence_font.color);
}

void ONScripterLabel::flush( int refresh_mode, SDL_Rect *rect, bool clear_dirty_flag, bool direct_flag )
{

    if ( direct_flag ){
        flushDirect( *rect, refresh_mode );
    }
    else{
        if ( rect ) dirty_rect.add( *rect );

        if ( dirty_rect.area > 0 ){
            if ( dirty_rect.area >= dirty_rect.bounding_box.w * dirty_rect.bounding_box.h ){
                flushDirect( dirty_rect.bounding_box, refresh_mode );
            }
            else{
		for (int i = 0; i < dirty_rect.num_history; ++i)
                    flushDirect( dirty_rect.history[i], refresh_mode, false );
		SDL_UpdateRects( screen_surface, dirty_rect.num_history, dirty_rect.history );
	    }
        }
    }

    if ( clear_dirty_flag ) dirty_rect.clear();
}

void ONScripterLabel::flushDirect( SDL_Rect &rect, int refresh_mode, bool updaterect )
{
    //printf("flush %d: %d %d %d %d\n", refresh_mode, rect.x, rect.y, rect.w, rect.h );

    refreshSurface( accumulation_surface, &rect, refresh_mode );
    if (refresh_mode != REFRESH_NONE_MODE &&
	!(refresh_mode & REFRESH_CURSOR_MODE)){
        if (refresh_mode & REFRESH_SHADOW_MODE)
            refreshSurface(accumulation_comp_surface, &rect,
			   (refresh_mode & ~REFRESH_SHADOW_MODE
			                 & ~REFRESH_TEXT_MODE)
			                 | REFRESH_COMP_MODE);
        else
            refreshSurface(accumulation_comp_surface, &rect,
			   refresh_mode | refresh_shadow_text_mode
			                | REFRESH_COMP_MODE);
    }
 
    SDL_BlitSurface( accumulation_surface, &rect, screen_surface, &rect );
    if (updaterect) SDL_UpdateRect( screen_surface, rect.x, rect.y, rect.w, rect.h );
}

void ONScripterLabel::mouseOverCheck( int x, int y )
{
    int c = 0;

    last_mouse_state.x = x;
    last_mouse_state.y = y;

    /* ---------------------------------------- */
    /* Check button */
    int button = 0;
    ButtonLink *p_button_link = root_button_link.next;
    ButtonLink *cur_button_link;
    while( p_button_link ){
        cur_button_link = p_button_link;
        while (cur_button_link) {
            if ( x >= cur_button_link->select_rect.x &&
                 x < cur_button_link->select_rect.x + cur_button_link->select_rect.w &&
                 y >= cur_button_link->select_rect.y &&
                 y < cur_button_link->select_rect.y + cur_button_link->select_rect.h &&
                 ( cur_button_link->button_type != ButtonLink::TEXT_BUTTON ||
                   ( txtbtn_visible && txtbtn_show ) )){
                button = cur_button_link->no;
                break;
            }
            cur_button_link = cur_button_link->same;
        }
        if (button != 0) break;
        p_button_link = p_button_link->next;
        c++;
    }

    if ( (current_over_button != button) || (current_button_link != p_button_link)){
        DirtyRect dirty = dirty_rect;
        dirty_rect.clear();

        SDL_Rect check_src_rect = {0, 0, 0, 0};
        SDL_Rect check_dst_rect = {0, 0, 0, 0};
        if ( current_over_button != 0 ){
            cur_button_link = current_button_link;
            while (cur_button_link) {
                cur_button_link->show_flag = 0;
                check_src_rect = cur_button_link->image_rect;
                if ( cur_button_link->button_type == ButtonLink::SPRITE_BUTTON ||
                     cur_button_link->button_type == ButtonLink::EX_SPRITE_BUTTON ){
                    sprite_info[ cur_button_link->sprite_no ].visible = true;
                    sprite_info[ cur_button_link->sprite_no ].setCell(0);
                }
                else if ( cur_button_link->button_type == ButtonLink::TMP_SPRITE_BUTTON ){
                    cur_button_link->show_flag = 1;
                    cur_button_link->anim[0]->visible = true;
                    cur_button_link->anim[0]->setCell(0);
                }
                else if ( cur_button_link->button_type == ButtonLink::TEXT_BUTTON ){
                    if (txtbtn_visible) {
                        cur_button_link->show_flag = 1;
                        cur_button_link->anim[0]->visible = true;
                        cur_button_link->anim[0]->setCell(0);
                    }
                }
                else if ( cur_button_link->anim[1] != NULL ){
                    cur_button_link->show_flag = 2;
                }
                dirty_rect.add( cur_button_link->image_rect );
                if ( exbtn_d_button_link.exbtn_ctl ){
                    decodeExbtnControl( exbtn_d_button_link.exbtn_ctl, &check_src_rect, &check_dst_rect );
                }

                cur_button_link = cur_button_link->same;
            }
        } else {
            if ( exbtn_d_button_link.exbtn_ctl ){
                decodeExbtnControl( exbtn_d_button_link.exbtn_ctl, &check_src_rect, &check_dst_rect );
            }
        }

        if ( p_button_link ){
            if ( system_menu_mode != SYSTEM_NULL ){
                if ( menuselectvoice_file_name[MENUSELECTVOICE_OVER] )
                    playSound(menuselectvoice_file_name[MENUSELECTVOICE_OVER],
                              SOUND_WAVE|SOUND_OGG, false, MIX_WAVE_CHANNEL);
            }
            else{
                if ( selectvoice_file_name[SELECTVOICE_OVER] )
                    playSound(selectvoice_file_name[SELECTVOICE_OVER],
                              SOUND_WAVE|SOUND_OGG, false, MIX_WAVE_CHANNEL);
            }
            cur_button_link = p_button_link;
            while (cur_button_link) {
                check_dst_rect = cur_button_link->image_rect;
                if ( cur_button_link->button_type == ButtonLink::SPRITE_BUTTON ||
                     cur_button_link->button_type == ButtonLink::EX_SPRITE_BUTTON ){
                    sprite_info[ cur_button_link->sprite_no ].setCell(1);
                    sprite_info[ cur_button_link->sprite_no ].visible = true;
                    if ( cur_button_link->button_type == ButtonLink::EX_SPRITE_BUTTON ){
                        decodeExbtnControl( cur_button_link->exbtn_ctl, &check_src_rect, &check_dst_rect );
                    }
                }
                else if ( cur_button_link->button_type == ButtonLink::TMP_SPRITE_BUTTON){
                    cur_button_link->show_flag = 1;
                    cur_button_link->anim[0]->visible = true;
                    cur_button_link->anim[0]->setCell(1);
                }
                else if ( cur_button_link->button_type == ButtonLink::TEXT_BUTTON &&
                          txtbtn_show && txtbtn_visible ){
                    cur_button_link->show_flag = 1;
                    cur_button_link->anim[0]->visible = true;
                    cur_button_link->anim[0]->setCell(1);
                    if ( cur_button_link->exbtn_ctl ){
                        decodeExbtnControl( cur_button_link->exbtn_ctl, &check_src_rect, &check_dst_rect );
                    }
                }
                else if ( cur_button_link->button_type == ButtonLink::NORMAL_BUTTON ||
                          cur_button_link->button_type == ButtonLink::LOOKBACK_BUTTON ){
                    cur_button_link->show_flag = 1;
                }
                dirty_rect.add( cur_button_link->image_rect );
                cur_button_link = cur_button_link->same;
            }
            current_button_link = p_button_link;
            shortcut_mouse_line = c;
        }

        flush( refreshMode() );
        dirty_rect = dirty;
    }
    current_over_button = button;
}

void ONScripterLabel::executeLabel()
{
  executeLabelTop:

    while ( current_line<current_label_info.num_of_lines ){
        if ( debug_level > 0 )
            printf("*****  executeLabel %s:%d/%d:%d:%d *****\n",
                   current_label_info.name,
                   current_line,
                   current_label_info.num_of_lines,
                   string_buffer_offset, display_mode );

        if ( script_h.getStringBuffer()[0] == '~' ){
            last_tilde.next_script = script_h.getNext();
            readToken();
            continue;
        }
        if ( break_flag && !script_h.isName("next") ){
            if ( script_h.getStringBuffer()[string_buffer_offset] == 0x0a )
                current_line++;

            if ( script_h.getStringBuffer()[string_buffer_offset] != ':' &&
                 script_h.getStringBuffer()[string_buffer_offset] != ';' &&
                 script_h.getStringBuffer()[string_buffer_offset] != 0x0a )
                script_h.skipToken();

            readToken();
            continue;
        }

        if ( kidokuskip_flag && skip_flag && kidokumode_flag && !script_h.isKidoku() ) skip_flag = false;

        char *current = script_h.getCurrent();
        int ret = ScriptParser::parseLine();
        if ( ret == RET_NOMATCH ) ret = this->parseLine();

        if ( ret & RET_SKIP_LINE ){
            script_h.skipLine();
            if (++current_line >= current_label_info.num_of_lines) break;
        }

        if ( ret & RET_REREAD ) script_h.setCurrent( current );

        if (!(ret & RET_NOREAD)){
            if (script_h.getStringBuffer()[string_buffer_offset] == 0x0a){
                string_buffer_offset = 0;
                if (++current_line >= current_label_info.num_of_lines) break;
            }
            readToken();
        }

        if ( ret & RET_WAIT ) return;
    }

    current_label_info = script_h.lookupLabelNext( current_label_info.name );
    current_line = 0;

    if ( current_label_info.start_address != NULL ){
        script_h.setCurrent( current_label_info.label_header );
        readToken();
        goto executeLabelTop;
    }

    fprintf( stderr, " ***** End *****\n");
    endCommand();
}

int ONScripterLabel::parseLine( )
{
    int ret, lut_counter = 0;
    const char *s_buf = script_h.getStringBuffer();
    const char *cmd = script_h.getStringBuffer();
    if (cmd[0] == '_') cmd++;

    if ( !script_h.isText() ){
        while( func_lut[ lut_counter ].method ){
            if ( !strcmp( func_lut[ lut_counter ].command, cmd ) ){
                return (this->*func_lut[ lut_counter ].method)();
            }
            lut_counter++;
        }

        if ( s_buf[0] == 0x0a )
            return RET_CONTINUE;
        else if ( s_buf[0] == 'v' && s_buf[1] >= '0' && s_buf[1] <= '9' )
            return vCommand();
        else if ( s_buf[0] == 'd' && s_buf[1] == 'v' && s_buf[2] >= '0' && s_buf[2] <= '9' )
            return dvCommand();

        fprintf( stderr, " command [%s] is not supported yet!!\n", s_buf );

        script_h.skipToken();

        return RET_CONTINUE;
    }

    /* Text */
    if ( current_mode == DEFINE_MODE ) errorAndExit( "text cannot be displayed in define section." );
    ret = textCommand();
    //Mion: moved all text processing into textCommand & its subfunctions

    return ret;
}

SDL_Surface *ONScripterLabel::loadImage( char *file_name, bool *has_alpha )
{
    char* alt_buffer = 0;
    if ( !file_name ) return NULL;
    unsigned long length = script_h.cBR->getFileLength( file_name );

    if (length == 0) {
	alt_buffer = new char[strlen(file_name) + strlen(script_h.save_path) + 1];
	sprintf(alt_buffer, "%s%s", script_h.save_path, file_name);
	char* si = alt_buffer;
	do { if (*si == '\\') *si = DELIMITER; } while (*(++si));
	FILE* fp = std::fopen(alt_buffer, "rb");
	if (fp) {
	    fseek(fp, 0, SEEK_END);
	    length = ftell(fp);
	    fclose(fp);
	}
	else delete[] alt_buffer;
    }

    if ( length == 0 ){
        if (strcmp(file_name, "uoncur.bmp" ) &&
	    strcmp(file_name, "uoffcur.bmp") &&
	    strcmp(file_name, "doncur.bmp" ) &&
	    strcmp(file_name, "doffcur.bmp") &&
	    strcmp(file_name, "cursor0.bmp") &&
	    strcmp(file_name, "cursor1.bmp"))
            fprintf( stderr, " *** can't find file [%s] ***\n", file_name );
        return NULL;
    }
    if ( filelog_flag )
        script_h.findAndAddLog( script_h.log_info[ScriptHandler::FILE_LOG], file_name, true );
    //printf(" ... loading %s length %ld\n", file_name, length );
    unsigned char *buffer = new unsigned char[length];
    int location;
    if (!alt_buffer) {
	script_h.cBR->getFile( file_name, buffer, &location );
    }
    else {
	FILE* fp = std::fopen(alt_buffer, "rb");
	fread(buffer, 1, length, fp);
	fclose(fp);
	delete[] alt_buffer;
    }
    SDL_Surface *tmp = IMG_Load_RW(SDL_RWFromMem( buffer, length ), 1);

    char *ext = strrchr(file_name, '.');
    if ( !tmp && ext && (!strcmp( ext+1, "JPG" ) || !strcmp( ext+1, "jpg" ) ) ){
        fprintf( stderr, " *** force-loading a JPG image [%s]\n", file_name );
        SDL_RWops *src = SDL_RWFromMem( buffer, length );
        tmp = IMG_LoadJPG_RW(src);
        SDL_RWclose(src);
    }
    if ( tmp && has_alpha ) *has_alpha = tmp->format->Amask;

    delete[] buffer;
    if ( !tmp ){
        fprintf( stderr, " *** can't load file [%s] ***\n", file_name );
        return NULL;
    }

    SDL_Surface *ret = SDL_ConvertSurface( tmp, image_surface->format, SDL_SWSURFACE );
    if ( ret &&
         screen_ratio2 != screen_ratio1 &&
         (!disable_rescale_flag || location == BaseReader::ARCHIVE_TYPE_NONE) )
    {
        SDL_Surface *src_s = ret;

        int w, h;
        if ( (w = src_s->w * screen_ratio1 / screen_ratio2) == 0 ) w = 1;
        if ( (h = src_s->h * screen_ratio1 / screen_ratio2) == 0 ) h = 1;
        SDL_PixelFormat *fmt = image_surface->format;
        ret = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h,
                                    fmt->BitsPerPixel, fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask );

        resizeSurface( src_s, ret );
        SDL_FreeSurface( src_s );
    }
    SDL_FreeSurface( tmp );

#ifndef BPP16
    // Hack to detect when a PNG image is likely to have an old-style
    // mask.  We assume that an old-style mask is intended if the
    // image either has no alpha channel, or the alpha channel it has
    // is completely opaque.  This behaviour can be overridden with
    // the --force-png-alpha and --force-png-nscmask command-line
    // options.
    if (has_alpha && *has_alpha) {
	if (png_mask_type == PNG_MASK_USE_NSCRIPTER)
	    *has_alpha = false;
	else if (png_mask_type == PNG_MASK_AUTODETECT) {	
	    SDL_LockSurface(ret);
	    const Uint32 aval = *(Uint32*)ret->pixels & ret->format->Amask;
	    if (aval != 0xffUL << ret->format->Ashift) goto breakme;
	    *has_alpha = false;
	    for (int y=0; y<ret->h; ++y) {
		Uint32* pixbuf = (Uint32*)((char*)ret->pixels + y * ret->pitch);
		for (int x=0; x<ret->w; ++x, ++pixbuf) {
		    if (*pixbuf & ret->format->Amask != aval) {
			*has_alpha = true;
			goto breakme;
		    }
		}
	    }
	breakme:
	    SDL_UnlockSurface(ret);
	}
    }
#else
#warning "BPP16 defined: PNGs with NScripter-style masks will not work as expected"
#endif
    
    return ret;
}

/* ---------------------------------------- */
void ONScripterLabel::deleteColorChanges()
{
    while (current_page_colors.next) {
        ColorChange *tmp = current_page_colors.next;
        current_page_colors.next = tmp->next;
        delete tmp;
    }
}

void ONScripterLabel::processTextButtonInfo()
{
    TextButtonInfoLink *info = text_button_info.next;

    if (info) txtbtn_show = true;
    while (info) {
        ButtonLink *firstbtn = NULL;
        char *text = info->prtext;
        char *text2;
        Fontinfo f_info = sentence_font;
        //f_info.clear();
        f_info.xy[0] = info->xy[0];
        f_info.xy[1] = info->xy[1];
        setColor(f_info.off_color, linkcolor[0]);
        setColor(f_info.on_color, linkcolor[1]);
        do {
            text2 = strchr(text, 0x0a);
            if (text2) {
                *text2 = '\0';
            }
            ButtonLink *txtbtn = getSelectableSentence(text, &f_info, true, false, false);
            //printf("made txtbtn: %d '%s'\n", info->no, text);
            txtbtn->button_type = ButtonLink::TEXT_BUTTON;
            txtbtn->no = info->no;
            if (!txtbtn_visible)
                txtbtn->show_flag = 0;
            if (firstbtn)
                firstbtn->connect(txtbtn);
            else
                firstbtn = txtbtn;
            f_info.xy[0] = info->xy[0];
            f_info.xy[1] = info->xy[1];
            f_info.newLine();
            if (text2) {
                *text2 = 0x0a;
                text2++;
            }
            text = text2;
        } while (text2);
        root_button_link.insert(firstbtn);
        info->button = firstbtn;
        info = info->next;
    }
}

void ONScripterLabel::deleteTextButtonInfo()
{
    TextButtonInfoLink *i1 = text_button_info.next;

    while( i1 ){
        TextButtonInfoLink *i2 = i1;
        // need to hide textbtn links
        ButtonLink *cur_button_link = i2->button;
        while (cur_button_link) {
            cur_button_link->show_flag = 0;
            cur_button_link = cur_button_link->same;
        }
        i1 = i1->next;
        delete i2;
    }
    text_button_info.next = NULL;
    txtbtn_visible = false;
    next_txtbtn_num = txtbtn_start_num;
}

void ONScripterLabel::deleteButtonLink()
{
    ButtonLink *b1 = root_button_link.next;

    while( b1 ){
        ButtonLink *b2 = b1->same;
        while ( b2 ) {
            ButtonLink *b3 = b2;
            b2 = b2->same;
            delete b3;
        }
        b2 = b1;
        b1 = b1->next;
        if ( b2->button_type == ButtonLink::TEXT_BUTTON ) {
            // Need to delete ref to button from text_button_info
            TextButtonInfoLink *i1 = text_button_info.next;
            while (i1) {
                if (i1->button == b2)
                    i1->button = NULL;
                i1 = i1->next;
            }
        }
        delete b2;
    }
    root_button_link.next = NULL;

    if ( exbtn_d_button_link.exbtn_ctl ) delete[] exbtn_d_button_link.exbtn_ctl;
    exbtn_d_button_link.exbtn_ctl = NULL;
}

void ONScripterLabel::refreshMouseOverButton()
{
    int mx, my;
    current_over_button = 0;
    current_button_link = root_button_link.next;
    SDL_GetMouseState( &mx, &my );
    mouseOverCheck( mx, my );
}

/* ---------------------------------------- */
/* Delete select link */
void ONScripterLabel::deleteSelectLink()
{
    SelectLink *link, *last_select_link = root_select_link.next;

    while ( last_select_link ){
        link = last_select_link;
        last_select_link = last_select_link->next;
        delete link;
    }
    root_select_link.next = NULL;
}

void ONScripterLabel::clearCurrentPage()
{
    sentence_font.clear();

    int num = (sentence_font.num_xy[0]*2+1)*sentence_font.num_xy[1];
    if (sentence_font.getTateyokoMode() == Fontinfo::TATE_MODE)
        num = (sentence_font.num_xy[1]*2+1)*sentence_font.num_xy[1];

// TEST for ados backlog cutoff problem
	num *= 2;

    if ( current_page->text &&
         current_page->max_text != num ){
        delete[] current_page->text;
        current_page->text = NULL;
    }
    if ( !current_page->text ){
        current_page->text = new char[num];
        current_page->max_text = num;
    }
    current_page->text_count = 0;

    if (current_page->tag){
        delete[] current_page->tag;
        current_page->tag = NULL;
    }

    num_chars_in_sentence = 0;
    internal_saveon_flag = true;

    text_info.fill( 0, 0, 0, 0 );
    cached_page = current_page;

    deleteColorChanges();
    setColor(current_page_colors.color, sentence_font.color);
    deleteTextButtonInfo();
}

void ONScripterLabel::shadowTextDisplay( SDL_Surface *surface, SDL_Rect &clip )
{
    if ( current_font->is_transparent ){

        SDL_Rect rect = {0, 0, screen_width, screen_height};
        if ( current_font == &sentence_font )
            rect = sentence_font_info.pos;

        if ( AnimationInfo::doClipping( &rect, &clip ) ) return;

        if ( rect.x + rect.w > surface->w ) rect.w = surface->w - rect.x;
        if ( rect.y + rect.h > surface->h ) rect.h = surface->h - rect.y;

        SDL_LockSurface( surface );
        ONSBuf *buf = (ONSBuf *)surface->pixels + rect.y * surface->w + rect.x;

        SDL_PixelFormat *fmt = surface->format;
        uchar3 color;
        color[0] = current_font->window_color[0] >> fmt->Rloss;
        color[1] = current_font->window_color[1] >> fmt->Gloss;
        color[2] = current_font->window_color[2] >> fmt->Bloss;

        for ( int i=rect.y ; i<rect.y + rect.h ; i++ ){
            for ( int j=rect.x ; j<rect.x + rect.w ; j++, buf++ ){
                *buf = (((*buf & fmt->Rmask) >> fmt->Rshift) * color[0] >> (8-fmt->Rloss)) << fmt->Rshift |
                    (((*buf & fmt->Gmask) >> fmt->Gshift) * color[1] >> (8-fmt->Gloss)) << fmt->Gshift |
                    (((*buf & fmt->Bmask) >> fmt->Bshift) * color[2] >> (8-fmt->Bloss)) << fmt->Bshift;
            }
            buf += surface->w - rect.w;
        }

        SDL_UnlockSurface( surface );
    }
    else if ( sentence_font_info.image_surface ){
        drawTaggedSurface( surface, &sentence_font_info, clip );
    }
}

void ONScripterLabel::newPage( bool next_flag )
{
    /* ---------------------------------------- */
    /* Set forward the text buffer */
    if ( current_page->text_count != 0 ){
        current_page = current_page->next;
        if ( start_page == current_page )
            start_page = start_page->next;
    }

    if ( next_flag ){
        indent_offset = 0;
        //line_enter_status = 0;
        page_enter_status = 0;
    }
    
    clearCurrentPage();
    txtbtn_visible = false;
    txtbtn_show = false;

    flush( refreshMode(), &sentence_font_info.pos );
}

struct ONScripterLabel::ButtonLink *ONScripterLabel::getSelectableSentence( char *buffer, Fontinfo *info, bool flush_flag, bool nofile_flag, bool skip_whitespace )
{
    int current_text_xy[2];
    current_text_xy[0] = info->xy[0];
    current_text_xy[1] = info->xy[1];

    ButtonLink *button_link = new ButtonLink();
    button_link->button_type = ButtonLink::TMP_SPRITE_BUTTON;
    button_link->show_flag = 1;

    AnimationInfo *anim = new AnimationInfo();
    button_link->anim[0] = anim;

    anim->trans_mode = AnimationInfo::TRANS_STRING;
    anim->is_single_line = false;
    anim->num_of_cells = 2;
    anim->color_list = new uchar3[ anim->num_of_cells ];
    for (int i=0 ; i<3 ; i++){
        if (nofile_flag)
            anim->color_list[0][i] = info->nofile_color[i];
        else
            anim->color_list[0][i] = info->off_color[i];
        anim->color_list[1][i] = info->on_color[i];
    }
    anim->skip_whitespace = skip_whitespace;
    setStr( &anim->file_name, buffer );
    anim->pos.x = info->x() * screen_ratio1 / screen_ratio2;
    anim->pos.y = info->y() * screen_ratio1 / screen_ratio2;
    anim->visible = true;

    setupAnimationInfo( anim, info );
    button_link->select_rect = button_link->image_rect = anim->pos;

    info->newLine();
    if (info->getTateyokoMode() == Fontinfo::YOKO_MODE)
        info->xy[0] = current_text_xy[0];
    else
        info->xy[1] = current_text_xy[1];

    dirty_rect.add( button_link->image_rect );

    return button_link;
}

void ONScripterLabel::decodeExbtnControl( const char *ctl_str, SDL_Rect *check_src_rect, SDL_Rect *check_dst_rect )
{
    char sound_name[256];
    int i, sprite_no, sprite_no2, cell_no;

    while( char com = *ctl_str++ ){
        if (com == 'C' || com == 'c'){
            sprite_no = getNumberFromBuffer( &ctl_str );
            sprite_no2 = sprite_no;
            cell_no = -1;
            if ( *ctl_str == '-' ){
                ctl_str++;
                sprite_no2 = getNumberFromBuffer( &ctl_str );
            }
            for (i=sprite_no ; i<=sprite_no2 ; i++)
                refreshSprite( i, false, cell_no, NULL, NULL );
        }
        else if (com == 'P' || com == 'p'){
            sprite_no = getNumberFromBuffer( &ctl_str );
            if ( *ctl_str == ',' ){
                ctl_str++;
                cell_no = getNumberFromBuffer( &ctl_str );
            }
            else
                cell_no = 0;
            refreshSprite( sprite_no, true, cell_no, check_src_rect, check_dst_rect );
        }
        else if (com == 'S' || com == 's'){
            sprite_no = getNumberFromBuffer( &ctl_str );
            if      (sprite_no < 0) sprite_no = 0;
            else if (sprite_no >= ONS_MIX_CHANNELS) sprite_no = ONS_MIX_CHANNELS-1;
            if ( *ctl_str != ',' ) continue;
            ctl_str++;
            if ( *ctl_str != '(' ) continue;
            ctl_str++;
            char *buf = sound_name;
            while (*ctl_str != ')' && *ctl_str != '\0' ) *buf++ = *ctl_str++;
            *buf++ = '\0';
            playSound(sound_name, SOUND_WAVE|SOUND_OGG, false, sprite_no);
            if ( *ctl_str == ')' ) ctl_str++;
        }
        else if (com == 'M' || com == 'm'){
            sprite_no = getNumberFromBuffer( &ctl_str );
            SDL_Rect rect = sprite_info[ sprite_no ].pos;
            if ( *ctl_str != ',' ) continue;
            ctl_str++; // skip ','
            sprite_info[ sprite_no ].pos.x = getNumberFromBuffer( &ctl_str ) * screen_ratio1 / screen_ratio2;
            if ( *ctl_str != ',' ) continue;
            ctl_str++; // skip ','
            sprite_info[ sprite_no ].pos.y = getNumberFromBuffer( &ctl_str ) * screen_ratio1 / screen_ratio2;
            dirty_rect.add( rect );
            sprite_info[ sprite_no ].visible = true;
            dirty_rect.add( sprite_info[ sprite_no ].pos );
        }
    }
}

void ONScripterLabel::loadCursor( int no, const char *str, int x, int y, bool abs_flag )
{
    cursor_info[ no ].setImageName( str );
    cursor_info[ no ].pos.x = x;
    cursor_info[ no ].pos.y = y;

    parseTaggedString( &cursor_info[ no ] );
    setupAnimationInfo( &cursor_info[ no ] );
    if ( filelog_flag )
        script_h.findAndAddLog( script_h.log_info[ScriptHandler::FILE_LOG], cursor_info[ no ].file_name, true ); // a trick for save file
    cursor_info[ no ].abs_flag = abs_flag;
    if ( cursor_info[ no ].image_surface )
        cursor_info[ no ].visible = true;
    else
        cursor_info[ no ].remove();
}

void ONScripterLabel::saveAll()
{
    saveEnvData();
    saveGlovalData();
    if ( filelog_flag )  writeLog( script_h.log_info[ScriptHandler::FILE_LOG] );
    if ( labellog_flag ) writeLog( script_h.log_info[ScriptHandler::LABEL_LOG] );
    if ( kidokuskip_flag ) script_h.saveKidokuData();
}

void ONScripterLabel::loadEnvData()
{
    volume_on_flag = true;
    text_speed_no = 1;
    draw_one_page_flag = false;
    default_env_font = NULL;
    cdaudio_on_flag = true;
    default_cdrom_drive = NULL;
    kidokumode_flag = true;

    if (loadFileIOBuf( "envdata" ) == 0){
        if (readInt() == 1 && window_mode == false) menu_fullCommand();
        if (readInt() == 0) volume_on_flag = false;
        text_speed_no = readInt();
        if (readInt() == 1) draw_one_page_flag = true;
        readStr( &default_env_font );
        if (default_env_font == NULL)
            setStr(&default_env_font, DEFAULT_ENV_FONT);
        if (readInt() == 0) cdaudio_on_flag = false;
        readStr( &default_cdrom_drive );
        voice_volume = DEFAULT_VOLUME - readInt();
        se_volume = DEFAULT_VOLUME - readInt();
        music_struct.volume = DEFAULT_VOLUME - readInt();
        if (readInt() == 0) kidokumode_flag = false;
    }
    else{
        setStr( &default_env_font, DEFAULT_ENV_FONT );
        voice_volume = se_volume = music_struct.volume = DEFAULT_VOLUME;
    }
}

void ONScripterLabel::saveEnvData()
{
    file_io_buf_ptr = 0;
    bool output_flag = false;
    for (int i=0 ; i<2 ; i++){
        writeInt( fullscreen_mode?1:0, output_flag );
        writeInt( volume_on_flag?1:0, output_flag );
        writeInt( text_speed_no, output_flag );
        writeInt( draw_one_page_flag?1:0, output_flag );
        writeStr( default_env_font, output_flag );
        writeInt( cdaudio_on_flag?1:0, output_flag );
        writeStr( default_cdrom_drive, output_flag );
        writeInt( DEFAULT_VOLUME - voice_volume, output_flag );
        writeInt( DEFAULT_VOLUME - se_volume, output_flag );
        writeInt( DEFAULT_VOLUME - music_struct.volume, output_flag );
        writeInt( kidokumode_flag?1:0, output_flag );
        writeInt( 0, output_flag ); // ?
        writeChar( 0, output_flag ); // ?
	writeInt( 1000, output_flag );

        if (i==1) break;
        allocFileIOBuf();
        output_flag = true;
    }

    saveFileIOBuf( "envdata" );
}

int ONScripterLabel::refreshMode()
{
    return display_mode == TEXT_DISPLAY_MODE
	 ? refresh_shadow_text_mode
	 : REFRESH_NORMAL_MODE;
}

void ONScripterLabel::quit()
{
    saveAll();

    if ( cdrom_info ){
        SDL_CDStop( cdrom_info );
        SDL_CDClose( cdrom_info );
    }
    if ( midi_info ){
        Mix_HaltMusic();
        Mix_FreeMusic( midi_info );
    }
    if ( music_info ){
        Mix_HaltMusic();
        Mix_FreeMusic( music_info );
    }
}

void ONScripterLabel::disableGetButtonFlag()
{
    btndown_flag = false;

    getzxc_flag = false;
    gettab_flag = false;
    getpageup_flag = false;
    getpagedown_flag = false;
    getinsert_flag = false;
    getfunction_flag = false;
    getenter_flag = false;
    getcursor_flag = false;
    spclclk_flag = false;
}

int ONScripterLabel::getNumberFromBuffer( const char **buf )
{
    int ret = 0;
    while ( **buf >= '0' && **buf <= '9' )
        ret = ret*10 + *(*buf)++ - '0';

    return ret;
}
