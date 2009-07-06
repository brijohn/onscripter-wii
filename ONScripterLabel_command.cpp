/* -*- C++ -*-
 *
 *  ONScripterLabel_command.cpp - Command executer of ONScripter
 *
 *  Copyright (c) 2001-2009 Ogapee. All rights reserved.
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

// Modified by Haeleth, autumn 2006, to remove unnecessary diagnostics and support ADOS properly.

// Modified by Mion of Sonozaki Futago-tachi, March 2008, to update from
// Ogapee's 20080121 release source code.

// Modified by Mion of Sonozaki Futago-tachi, April 2009, to update from
// Ogapee's 20090331 release source code.

#include "ONScripterLabel.h"
#include "version.h"

#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

#ifdef WIN32
#include <direct.h>
#include <windows.h>
#endif

#ifdef MACOSX
namespace Carbon {
#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
};
#endif

#ifdef LINUX
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#endif

#define DEFAULT_CURSOR_WAIT    ":l/3,160,2;cursor0.bmp"
#define DEFAULT_CURSOR_NEWPAGE ":l/3,160,2;cursor1.bmp"

#define CONTINUOUS_PLAY

extern SDL_TimerID timer_mp3fadeout_id;
extern SDL_TimerID timer_mp3fadein_id;
extern "C" Uint32 SDLCALL mp3fadeoutCallback( Uint32 interval, void *param );
extern "C" Uint32 SDLCALL mp3fadeinCallback( Uint32 interval, void *param );

int ONScripterLabel::waveCommand()
{
    wave_play_loop_flag = false;

    if (script_h.isName( "waveloop" ))
        wave_play_loop_flag = true;

    wavestopCommand();

    setStr(&wave_file_name, script_h.readStr());
    playSound(wave_file_name, SOUND_WAVE|SOUND_OGG, wave_play_loop_flag, MIX_WAVE_CHANNEL);

    return RET_CONTINUE;
}

int ONScripterLabel::wavestopCommand()
{
    if ( wave_sample[MIX_WAVE_CHANNEL] ){
        Mix_Pause( MIX_WAVE_CHANNEL );
        Mix_FreeChunk( wave_sample[MIX_WAVE_CHANNEL] );
        wave_sample[MIX_WAVE_CHANNEL] = NULL;
    }
    setStr( &wave_file_name, NULL );

    return RET_CONTINUE;
}

int ONScripterLabel::waittimerCommand()
{
    int count = script_h.readInt() + internal_timer - SDL_GetTicks();
    startTimer( count );

    return RET_WAIT;
}

int ONScripterLabel::waitCommand()
{
#ifdef INSANI
    int count = script_h.readInt();

    if( (skip_mode & (SKIP_NORMAL | SKIP_TO_EOP | SKIP_TO_WAIT)) || ctrl_pressed_status ) {
        //Mion: instead of skipping entirely, let's do a shortened wait (safer)
        count = count / 10;
        if (count < 10) count = 10;
    }
    startTimer( count );
    return RET_WAIT;
#else
    startTimer( script_h.readInt() );

    return RET_WAIT;
#endif
}

int ONScripterLabel::vspCommand()
{
    int ret = leaveTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    bool vsp2_flag = false;
    if (script_h.isName("vsp2")) vsp2_flag = true;

    int no = script_h.readInt();
    int v  = script_h.readInt();

    if (vsp2_flag){
        sprite2_info[ no ].visible = (v==1)?true:false;
        dirty_rect.add( sprite2_info[no].bounding_rect );
    }
    else {
        sprite_info[ no ].visible = (v==1)?true:false;
        dirty_rect.add( sprite_info[no].pos );
    }

    return RET_CONTINUE;
}

int ONScripterLabel::voicevolCommand()
{
    voice_volume = script_h.readInt();

    if ( wave_sample[0] ) Mix_Volume( 0, voice_volume * 128 / 100 );

    channelvolumes[0] = voice_volume;

    return RET_CONTINUE;
}

int ONScripterLabel::vCommand()
{
    char buf[256];

    sprintf(buf, "wav%c%s.wav", DELIMITER, script_h.getStringBuffer()+1);
    playSound(buf, SOUND_WAVE|SOUND_OGG, false, MIX_WAVE_CHANNEL);

    return RET_CONTINUE;
}

int ONScripterLabel::trapCommand()
{
    if      ( script_h.isName( "lr_trap" ) ){
        trap_mode = TRAP_LEFT_CLICK | TRAP_RIGHT_CLICK;
    }
    else if ( script_h.isName( "trap" ) ){
        trap_mode = TRAP_LEFT_CLICK;
    }

    if ( script_h.compareString("off") ){
        script_h.readName();
        trap_mode = TRAP_NONE;
        return RET_CONTINUE;
    }

    const char *buf = script_h.readLabel();
    if ( buf[0] == '*' ){
        setStr(&trap_dist, buf+1);
    }
    else{
        printf("trapCommand: [%s] is not supported\n", buf );
    }

    return RET_CONTINUE;
}

int ONScripterLabel::textspeeddefaultCommand()
{
    sentence_font.wait_time = -1;

    return RET_CONTINUE;
}

int ONScripterLabel::textspeedCommand()
{
    sentence_font.wait_time = script_h.readInt();

    return RET_CONTINUE;
}

int ONScripterLabel::textshowCommand()
{
    dirty_rect.fill( screen_width, screen_height );
    refresh_shadow_text_mode = REFRESH_NORMAL_MODE | REFRESH_SHADOW_MODE | REFRESH_TEXT_MODE;
    flush(refreshMode());

    return RET_CONTINUE;
}

int ONScripterLabel::textonCommand()
{
    if (windowchip_sprite_no >= 0)
        sprite_info[windowchip_sprite_no].visible = true;

    int ret = enterTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    text_on_flag = true;

    return RET_CONTINUE;
}

int ONScripterLabel::textoffCommand()
{
    if (windowchip_sprite_no >= 0)
        sprite_info[windowchip_sprite_no].visible = false;
    refreshSurface(backup_surface, NULL, REFRESH_NORMAL_MODE);

    int ret = leaveTextDisplayMode(true);
    if (ret != RET_NOMATCH) return ret;

    text_on_flag = false;

    return RET_CONTINUE;
}

int ONScripterLabel::texthideCommand()
{
    dirty_rect.fill( screen_width, screen_height );
    refresh_shadow_text_mode = REFRESH_NORMAL_MODE | REFRESH_SHADOW_MODE;
    flush(refreshMode());

    return RET_CONTINUE;
}

int ONScripterLabel::textexbtnCommand()
{
    int txtbtn_no = script_h.readInt();
    const char *buf = script_h.readStr();

    TextButtonInfoLink *info = text_button_info.next;
    TextButtonInfoLink *found = NULL;
    while (info) {
        if (info->no == txtbtn_no)
            found = info;
        info = info->next;
    }
    
    if (found) {
        ButtonLink *button = found->button;
        while (button) {
            button->exbtn_ctl   = new char[ strlen( buf ) + 1 ];
            strcpy( button->exbtn_ctl, buf );
            button = button->same;
        }
    }

    return RET_CONTINUE;
}

int ONScripterLabel::textclearCommand()
{
    newPage( false );
    return RET_CONTINUE;
}

int ONScripterLabel::textbtnstartCommand()
{
    txtbtn_start_num = script_h.readInt();

    return RET_CONTINUE;
}

int ONScripterLabel::textbtnoffCommand()
{
    txtbtn_show = false;

    return RET_CONTINUE;
}

int ONScripterLabel::texecCommand()
{
    if ( textgosub_clickstr_state == CLICK_NEWPAGE ){
        newPage( true );
        clickstr_state = CLICK_NONE;
    }
    else if ( textgosub_clickstr_state == CLICK_WAITEOL ){
        if ( !sentence_font.isLineEmpty() && !new_line_skip_flag ){
            indent_offset = 0;
            line_enter_status = 0;
            current_page->add( 0x0a );
            sentence_font.newLine();
        }
    }

    return RET_CONTINUE;
}

int ONScripterLabel::tateyokoCommand()
{
    sentence_font.setTateyokoMode( script_h.readInt() );

    return RET_CONTINUE;
}

int ONScripterLabel::talCommand()
{
    int ret = leaveTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    char loc = script_h.readName()[0];
    int no = -1, trans = 0;
    if      ( loc == 'l' ) no = 0;
    else if ( loc == 'c' ) no = 1;
    else if ( loc == 'r' ) no = 2;

    if (no >= 0){
        trans = script_h.readInt();
        if      (trans > 256) trans = 256;
        else if (trans < 0  ) trans = 0;
    }

    if ( event_mode & EFFECT_EVENT_MODE ){
        return doEffect( parseEffect(false) );
    }
    else{
        if (no >= 0){
            tachi_info[ no ].trans = trans;
            dirty_rect.add( tachi_info[ no ].pos );
        }

        return setEffect( parseEffect(true), true, true );
   }
}

int ONScripterLabel::tablegotoCommand()
{
    int count = 0;
    int no = script_h.readInt();

    while( script_h.getEndStatus() & ScriptHandler::END_COMMA ){
        const char *buf = script_h.readLabel();
        if ( count++ == no ){
            setCurrentLabel( buf+1 );
            break;
        }
    }

    return RET_CONTINUE;
}

int ONScripterLabel::systemcallCommand()
{
    system_menu_mode = getSystemCallNo( script_h.readName() );
    enterSystemCall();
    advancePhase();

    return RET_WAIT;
}

int ONScripterLabel::strspCommand()
{
    int ret = leaveTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    bool v=true;

    if ( script_h.isName( "strsph" ) )
        v = false;

    int sprite_no = script_h.readInt();
    AnimationInfo *ai = &sprite_info[sprite_no];
    if ( ai->visible )
        dirty_rect.add( ai->pos );
    ai->removeTag();
    setStr(&ai->file_name, script_h.readStr());

    Fontinfo fi;
    fi.is_newline_accepted = true;
#ifndef RCA_SCALE
    ai->pos.x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    ai->pos.y = script_h.readInt() * screen_ratio1 / screen_ratio2;
#else
    ai->pos.x = script_h.readInt() * scr_stretch_x * screen_ratio1 / screen_ratio2;
    ai->pos.y = script_h.readInt() * scr_stretch_y * screen_ratio1 / screen_ratio2;
#endif
    fi.num_xy[0] = script_h.readInt();
    fi.num_xy[1] = script_h.readInt();
    fi.font_size_xy[0] = script_h.readInt();
    fi.font_size_xy[1] = script_h.readInt();
    fi.pitch_xy[0] = script_h.readInt() + fi.font_size_xy[0];
    fi.pitch_xy[1] = script_h.readInt() + fi.font_size_xy[1];
    fi.is_bold = script_h.readInt()?true:false;
    fi.is_shadow = script_h.readInt()?true:false;

    char *buffer = script_h.getNext();
    while(script_h.getEndStatus() & ScriptHandler::END_COMMA){
        ai->num_of_cells++;
        script_h.readStr();
    }
    if (ai->num_of_cells == 0){
        ai->num_of_cells = 1;
        ai->color_list = new uchar3[ai->num_of_cells];
        ai->color_list[0][0] = ai->color_list[0][1] = ai->color_list[0][2] = 0xff;
    }
    else{
        ai->color_list = new uchar3[ai->num_of_cells];
        script_h.setCurrent(buffer);
        for (int i=0 ; i<ai->num_of_cells ; i++)
            readColor(&ai->color_list[i], script_h.readStr());
    }

    ai->trans_mode = AnimationInfo::TRANS_STRING;
    ai->trans = 256;
    ai->visible = v;
    ai->is_single_line = false;
    ai->is_tight_region = false;
    setupAnimationInfo(ai, &fi);

    return RET_CONTINUE;
}

int ONScripterLabel::stopCommand()
{
    wavestopCommand();
    //loopbgmstopCommand(); NScr doesn't stop loopbgm w/this cmd
    return mp3stopCommand();
}

int ONScripterLabel::sp_rgb_gradationCommand()
{
    int ret = leaveTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    int no = script_h.readInt();
    int upper_r = script_h.readInt();
    int upper_g = script_h.readInt();
    int upper_b = script_h.readInt();
    int lower_r = script_h.readInt();
    int lower_g = script_h.readInt();
    int lower_b = script_h.readInt();
    ONSBuf key_r = script_h.readInt();
    ONSBuf key_g = script_h.readInt();
    ONSBuf key_b = script_h.readInt();
    Uint32 alpha = script_h.readInt();

    AnimationInfo *si;
    if (no == -1) si = &sentence_font_info;
    else          si = &sprite_info[no];
    SDL_Surface *surface = si->image_surface;
    if (surface == NULL) return RET_CONTINUE;

    SDL_PixelFormat *fmt = surface->format;

    ONSBuf key_mask = (key_r >> fmt->Rloss) << fmt->Rshift |
        (key_g >> fmt->Gloss) << fmt->Gshift |
        (key_b >> fmt->Bloss) << fmt->Bshift;
    ONSBuf rgb_mask = fmt->Rmask | fmt->Gmask | fmt->Bmask;

    SDL_LockSurface(surface);
    // check upper and lower bound
    int i, j;
    int upper_bound=0, lower_bound=0;
    bool is_key_found = false;
    for (i=0 ; i<surface->h ; i++){
        ONSBuf *buf = (ONSBuf *)surface->pixels + surface->w * i;
        for (j=0 ; j<surface->w ; j++, buf++){
            if ((*buf & rgb_mask) == key_mask){
                if (is_key_found == false){
                    is_key_found = true;
                    upper_bound = lower_bound = i;
                }
                else{
                    lower_bound = i;
                }
                break;
            }
        }
    }

    // replace pixels of the key-color with the specified color in gradation
    for (i=upper_bound ; i<=lower_bound ; i++){
        ONSBuf *buf = (ONSBuf *)surface->pixels + surface->w * i;
#ifdef BPP16
        unsigned char *alphap = si->alpha_buf + surface->w * i;
#else
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
        unsigned char *alphap = (unsigned char *)buf + 3;
#else
        unsigned char *alphap = (unsigned char *)buf;
#endif
#endif
        Uint32 color = alpha << surface->format->Ashift;
        if (upper_bound != lower_bound){
            color |= (((lower_r - upper_r) * (i-upper_bound) / (lower_bound - upper_bound) + upper_r) >> fmt->Rloss) << fmt->Rshift;
            color |= (((lower_g - upper_g) * (i-upper_bound) / (lower_bound - upper_bound) + upper_g) >> fmt->Gloss) << fmt->Gshift;
            color |= (((lower_b - upper_b) * (i-upper_bound) / (lower_bound - upper_bound) + upper_b) >> fmt->Bloss) << fmt->Bshift;
        }
        else{
            color |= (upper_r >> fmt->Rloss) << fmt->Rshift;
            color |= (upper_g >> fmt->Gloss) << fmt->Gshift;
            color |= (upper_b >> fmt->Bloss) << fmt->Bshift;
        }

        for (j=0 ; j<surface->w ; j++, buf++){
            if ((*buf & rgb_mask) == key_mask){
                *buf = color;
                *alphap = alpha;
            }
#ifdef BPP16
            alphap++;
#else
            alphap += 4;
#endif
        }
    }

    SDL_UnlockSurface(surface);

    if ( si->visible )
        dirty_rect.add( si->pos );

    return RET_CONTINUE;
}

int ONScripterLabel::spstrCommand()
{
    decodeExbtnControl( script_h.readStr() );

    return RET_CONTINUE;
}

int ONScripterLabel::spreloadCommand()
{
    int ret = leaveTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    int no = script_h.readInt();
    AnimationInfo *si;
    if (no == -1) si = &sentence_font_info;
    else          si = &sprite_info[no];

    parseTaggedString( si );
    setupAnimationInfo( si );

    if ( si->visible )
        dirty_rect.add( si->pos );

    return RET_CONTINUE;
}

int ONScripterLabel::splitCommand()
{
    script_h.readStr();
    const char *save_buf = script_h.saveStringBuffer();

    char delimiter = script_h.readStr()[0];

    char token[256];
    while( script_h.getEndStatus() & ScriptHandler::END_COMMA ){

        unsigned int c=0;
        while(save_buf[c] != delimiter && save_buf[c] != '\0'){
            if (IS_TWO_BYTE(save_buf[c]))
                c += 2;
            else
                c++;
        }
        memcpy( token, save_buf, c );
        token[c] = '\0';

        script_h.readVariable();
        if ( script_h.current_variable.type & ScriptHandler::VAR_INT ||
             script_h.current_variable.type & ScriptHandler::VAR_ARRAY ){
            script_h.setInt( &script_h.current_variable, atoi(token) );
        }
        else if ( script_h.current_variable.type & ScriptHandler::VAR_STR ){
            setStr( &script_h.getVariableData(script_h.current_variable.var_no).str, token );
        }

        save_buf += c;
        if (save_buf[0] != '\0') save_buf++;
    }

    return RET_CONTINUE;
}

int ONScripterLabel::spclclkCommand()
{
    if ( !force_button_shortcut_flag )
        spclclk_flag = true;
    return RET_CONTINUE;
}

int ONScripterLabel::spbtnCommand()
{
    bool cellcheck_flag = false;

    if ( script_h.isName( "cellcheckspbtn" ) )
        cellcheck_flag = true;

    int sprite_no = script_h.readInt();
    int no        = script_h.readInt();

    if ( cellcheck_flag ){
        if ( sprite_info[ sprite_no ].num_of_cells < 2 ) return RET_CONTINUE;
    }
    else{
        if ( sprite_info[ sprite_no ].num_of_cells == 0 ) return RET_CONTINUE;
    }

    ButtonLink *button = new ButtonLink();
    root_button_link.insert( button );

    button->button_type = ButtonLink::SPRITE_BUTTON;
    button->sprite_no   = sprite_no;
    button->no          = no;

    if ( sprite_info[ sprite_no ].image_surface ||
         sprite_info[ sprite_no ].trans_mode == AnimationInfo::TRANS_STRING )
    {
        button->image_rect = button->select_rect = sprite_info[ sprite_no ].pos;
        sprite_info[ sprite_no ].visible = true;
    }

    return RET_CONTINUE;
}

int ONScripterLabel::skipoffCommand()
{
    skip_mode &= ~SKIP_NORMAL;

    return RET_CONTINUE;
}

#ifdef LINUX
int tryToLaunch(const char* command, const char* target)
{
    if (!command || !target) return -1;
    pid_t child = vfork();
    if (child == -1) {
	// Parent, failed
	fprintf(stderr, "Could not open `%s': fork error: %s\n",
		target, strerror(errno));
	return 0;
    }
    else if (child) {
	// Parent, success
	int status;
	waitpid(child, &status, 0);
	if (WIFEXITED(status)) return WEXITSTATUS(status);
	return -1;
    }
    else {
	// Child
	execlp(command, command, target, (char*) NULL);
	_exit(255);
    }
}
#endif

int ONScripterLabel::shellCommand()
{
#ifdef WIN32
    const char *url = script_h.readStr();
    HMODULE shdll = LoadLibrary("shell32");
    if (shdll) {
        typedef HINSTANCE (WINAPI *SHELLEXECUTE)(HWND, LPCSTR, LPCSTR, LPCSTR,
						 LPCSTR, int);
        SHELLEXECUTE shexec =
	    SHELLEXECUTE(GetProcAddress(shdll, "ShellExecuteA"));
        if (shexec) {
            shexec(NULL, "open", url, NULL, NULL, SW_SHOW);
        }
        FreeLibrary(shdll);
    }
    
#elif defined MACOSX
    using namespace Carbon;
    CFStringRef url_string =
	CFStringCreateWithCString(NULL, script_h.readStr(),
				  kCFStringEncodingShiftJIS);
    CFURLRef url = CFURLCreateWithString(NULL, url_string, NULL);
    LSOpenCFURLRef(url, NULL);
    CFRelease(url);
    CFRelease(url_string);
    
#elif defined LINUX
    // Linux/BSD/other Unixes don't provide standard APIs for this
    // kind of thing, but there are various things we can try.
    
    const char *url = script_h.readStr();
    int status;

    // First up is xdg-open, the freedesktop solution that's becoming
    // standard in the Linux world, at least.
    //
    status = tryToLaunch("xdg-open", url);
    switch (status) {
    case 0: // Success
	return RET_CONTINUE;

    case 2: // File not found
	fprintf(stderr, "Failed to open %s: xdg-error reports that it doesn't "
		"exist\n", url);
	return RET_CONTINUE;

    case 255: // execlp() failed (e.g. xdg-open not present)
	// Don't say anything.
	break;
	
    case 1:   // Syntax error
    case 3:   // Required tool missing
    case 4:   // Action failed
    case -1:  // child didn't exit normally
    default:  // unknown problem
	fprintf(stderr, "Open URL with xdg-open failed with status %d\n",
		status);
    }

    // Failing that, try $BROWSER, or give up.
    const char* browser = getenv("BROWSER");
    if (browser) {
	char* cmd = new char[strlen(browser) + strlen(url) + 8];
	sprintf(cmd, "\"%s\" '%s' &", browser, url);
	if (system(cmd) != 0)
            fprintf(stderr, "Couldn't launch web browser `%s': check your "
                    "BROWSER setting.\n", browser);
	delete[] cmd;
    }
    else {
	fputs("Could not determine which web browser to use. "
	      "Please set BROWSER appropriately.\n", stderr);
    }
    
#else
    fprintf(stderr, "[shell] command not supported for this OS\n");
#endif

    return RET_CONTINUE;
}

int ONScripterLabel::sevolCommand()
{
    se_volume = script_h.readInt();

    for ( int i=1 ; i<ONS_MIX_CHANNELS ; i++ ) {
        if ( wave_sample[i] ) Mix_Volume( i, se_volume * 128 / 100 );
        channelvolumes[i] = se_volume;
     }

    if ( wave_sample[MIX_LOOPBGM_CHANNEL0] ) Mix_Volume( MIX_LOOPBGM_CHANNEL0, se_volume * 128 / 100 );
    if ( wave_sample[MIX_LOOPBGM_CHANNEL1] ) Mix_Volume( MIX_LOOPBGM_CHANNEL1, se_volume * 128 / 100 );

    return RET_CONTINUE;
}

void ONScripterLabel::setwindowCore()
{
    sentence_font.ttf_font  = NULL;
    sentence_font.top_xy[0] = script_h.readInt();
    sentence_font.top_xy[1] = script_h.readInt();
    sentence_font.num_xy[0] = script_h.readInt();
    sentence_font.num_xy[1] = script_h.readInt();
    sentence_font.font_size_xy[0] = script_h.readInt();
    sentence_font.font_size_xy[1] = script_h.readInt();
    sentence_font.pitch_xy[0] = script_h.readInt() + sentence_font.font_size_xy[0];
    sentence_font.pitch_xy[1] = script_h.readInt() + sentence_font.font_size_xy[1];
    sentence_font.wait_time = script_h.readInt();
    sentence_font.is_bold = script_h.readInt()?true:false;
    sentence_font.is_shadow = script_h.readInt()?true:false;

    const char *buf = script_h.readStr();
    dirty_rect.add( sentence_font_info.pos );
    if ( buf[0] == '#' ){
        sentence_font.is_transparent = true;
        readColor( &sentence_font.window_color, buf );

        sentence_font_info.pos.x = script_h.readInt() * screen_ratio1 / screen_ratio2;
        sentence_font_info.pos.y = script_h.readInt() * screen_ratio1 / screen_ratio2;
        sentence_font_info.pos.w = script_h.readInt() * screen_ratio1 / screen_ratio2 - sentence_font_info.pos.x + 1;
        sentence_font_info.pos.h = script_h.readInt() * screen_ratio1 / screen_ratio2 - sentence_font_info.pos.y + 1;
    }
    else{
        sentence_font.is_transparent = false;
        sentence_font_info.setImageName( buf );
        parseTaggedString( &sentence_font_info );
        setupAnimationInfo( &sentence_font_info );
#ifndef RCA_SCALE
        sentence_font_info.pos.x = script_h.readInt() * screen_ratio1 / screen_ratio2;
        sentence_font_info.pos.y = script_h.readInt() * screen_ratio1 / screen_ratio2;
#   if 0
        if ( sentence_font_info.image_surface ){
            sentence_font_info.pos.w = sentence_font_info.image_surface->w * screen_ratio1 / screen_ratio2;
            sentence_font_info.pos.h = sentence_font_info.image_surface->h * screen_ratio1 / screen_ratio2;
        }
#   endif	    
        sentence_font.window_color[0] = sentence_font.window_color[1] = sentence_font.window_color[2] = 0xff;
    }
#else // if RCA_SCALE
        sentence_font_info.pos.x = script_h.readInt() * screen_ratio1 * scr_stretch_x / screen_ratio2;
        sentence_font_info.pos.y = script_h.readInt() * screen_ratio1 * scr_stretch_y / screen_ratio2;
        if ( sentence_font_info.image_surface && (scr_stretch_y > 1.0 || scr_stretch_x > 1.0)) {
            SDL_Surface* src = sentence_font_info.image_surface;                    
            SDL_PixelFormat *fmt = src->format;                  
            SDL_Surface* dst = SDL_CreateRGBSurface( SDL_SWSURFACE,
                                                     scr_stretch_x*src->w,
                                                     scr_stretch_y*src->h,
                                                     fmt->BitsPerPixel, fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask );
            resizeSurface( src, dst );
            sentence_font_info.image_surface = dst;
            sentence_font_info.pos.w *= scr_stretch_x;
            sentence_font_info.pos.h *= scr_stretch_y;
            SDL_FreeSurface( src );
	}
        sentence_font.window_color[0] = sentence_font.window_color[1] = sentence_font.window_color[2] = 0xff;
    }

    // Scale and reposition window size if screen is bigger than game
    if (scr_stretch_x > 1.0) {
        sentence_font_info.pos.w = sentence_font_info.pos.w * scr_stretch_x;
        sentence_font.top_xy[0] = sentence_font.top_xy[0] * scr_stretch_x;
#   if 1
        // Either increase number of characters on screen
        sentence_font.num_xy[0] = sentence_font.num_xy[0] * scr_stretch_x;
#   else
        // Or increase font size
        sentence_font.pitch_xy[0] = sentence_font.pitch_xy[0] * scr_stretch_x;
        sentence_font.font_size_xy[0] = sentence_font.font_size_xy[0] * scr_stretch_x;        
#   endif
    }
    if (scr_stretch_y > 1.0) {
        sentence_font_info.pos.h = sentence_font_info.pos.h * scr_stretch_y;
        sentence_font.top_xy[1] = sentence_font.top_xy[1] * scr_stretch_y;
#   if 1
        // Either increase number of characters on screen
        sentence_font.num_xy[1] = sentence_font.num_xy[1] * scr_stretch_y;
#   else
        // Or increase font size
        sentence_font.pitch_xy[1] = sentence_font.pitch_xy[1] * scr_stretch_y;
        sentence_font.font_size_xy[1] = sentence_font.font_size_xy[1] * scr_stretch_y;
#   endif

    }
#endif
    deleteColorChanges();
    setColor(current_page_colors.color, sentence_font.color);
}

int ONScripterLabel::setwindow3Command()
{
    setwindowCore();

    display_mode = DISPLAY_MODE_NORMAL;
    flush( refreshMode(), &sentence_font_info.pos );

    return RET_CONTINUE;
}

int ONScripterLabel::setwindow2Command()
{
    const char *buf = script_h.readStr();
    if ( buf[0] == '#' ){
        sentence_font.is_transparent = true;
        readColor( &sentence_font.window_color, buf );
    }
    else{
        sentence_font.is_transparent = false;
        sentence_font_info.setImageName( buf );
        parseTaggedString( &sentence_font_info );
        setupAnimationInfo( &sentence_font_info );
    }
    repaintCommand();

    return RET_CONTINUE;
}

int ONScripterLabel::setwindowCommand()
{
    setwindowCore();

    lookbackflushCommand();
    indent_offset = 0;
    line_enter_status = 0;
    page_enter_status = 0;
    display_mode = DISPLAY_MODE_NORMAL;
    flush( refreshMode(), &sentence_font_info.pos );

    return RET_CONTINUE;
}

int ONScripterLabel::setcursorCommand()
{
    bool abs_flag;

    if ( script_h.isName( "abssetcursor" ) ){
        abs_flag = true;
    }
    else{
        abs_flag = false;
    }

    int no = script_h.readInt();
    script_h.readStr();
    const char* buf = script_h.saveStringBuffer();
    int x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    int y = script_h.readInt() * screen_ratio1 / screen_ratio2;

    loadCursor( no, buf, x, y, abs_flag );

    return RET_CONTINUE;
}

int ONScripterLabel::selectCommand()
{
    int ret = enterTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    int select_mode = SELECT_GOTO_MODE;
    SelectLink *last_select_link;

    if ( script_h.isName( "selnum" ) )
        select_mode = SELECT_NUM_MODE;
    else if ( script_h.isName( "selgosub" ) )
        select_mode = SELECT_GOSUB_MODE;
    else if ( script_h.isName( "select" ) )
        select_mode = SELECT_GOTO_MODE;
    else if ( script_h.isName( "csel" ) )
        select_mode = SELECT_CSEL_MODE;

    if ( select_mode == SELECT_NUM_MODE ){
        script_h.readVariable();
        script_h.pushVariable();
    }

    if ( event_mode & WAIT_BUTTON_MODE ){

        if ( current_button_state.button <= 0 ) return RET_WAIT | RET_REREAD;

        if ( selectvoice_file_name[SELECTVOICE_SELECT] )
            playSound(selectvoice_file_name[SELECTVOICE_SELECT],
                      SOUND_WAVE|SOUND_OGG, false, MIX_WAVE_CHANNEL );

        event_mode = IDLE_EVENT_MODE;

        deleteButtonLink();

        int counter = 1;
        last_select_link = root_select_link.next;
        while ( last_select_link ){
            if ( current_button_state.button == counter++ ) break;
            last_select_link = last_select_link->next;
        }

        if ( select_mode  == SELECT_GOTO_MODE ){
            setCurrentLabel( last_select_link->label );
        }
        else if ( select_mode == SELECT_GOSUB_MODE ){
            gosubReal( last_select_link->label, select_label_info.next_script );
        }
        else{ // selnum
            script_h.setInt( &script_h.pushed_variable, current_button_state.button - 1 );
            current_label_info = script_h.getLabelByAddress( select_label_info.next_script );
            current_line = script_h.getLineByAddress( select_label_info.next_script );
            script_h.setCurrent( select_label_info.next_script );
        }
        deleteSelectLink();

        newPage( true );

        return RET_CONTINUE;
    }
    else{
        bool comma_flag = true;
        if ( select_mode == SELECT_CSEL_MODE ){
            saveoffCommand();
        }
        shortcut_mouse_line = -1;

        int xy[2];
        xy[0] = sentence_font.xy[0];
        xy[1] = sentence_font.xy[1];

        if ( selectvoice_file_name[SELECTVOICE_OPEN] )
            playSound(selectvoice_file_name[SELECTVOICE_OPEN],
                      SOUND_WAVE|SOUND_OGG, false, MIX_WAVE_CHANNEL );

        last_select_link = &root_select_link;

        while(1){
            if ( script_h.getNext()[0] != 0x0a && comma_flag == true ){

                const char *buf = script_h.readStr();
                comma_flag = (script_h.getEndStatus() & ScriptHandler::END_COMMA);
                if ( select_mode != SELECT_NUM_MODE && !comma_flag ) errorAndExit( "select: comma is needed here." );

                // Text part
                SelectLink *slink = new SelectLink();
                setStr( &slink->text, buf );
                //printf("Select text %s\n", slink->text);

                // Label part
                if (select_mode != SELECT_NUM_MODE){
                    script_h.readLabel();
                    setStr( &slink->label, script_h.getStringBuffer()+1 );
                    //printf("Select label %s\n", slink->label );
                }
                last_select_link->next = slink;
                last_select_link = last_select_link->next;

                comma_flag = (script_h.getEndStatus() & ScriptHandler::END_COMMA);
                //printf("2 comma %d %c %x\n", comma_flag, script_h.getCurrent()[0], script_h.getCurrent()[0]);
            }
            else if (script_h.getNext()[0] == 0x0a){
                //printf("comma %d\n", comma_flag);
                char *buf = script_h.getNext() + 1; // consume eol
                while ( *buf == ' ' || *buf == '\t' ) buf++;

                if (comma_flag && *buf == ',')
                    errorAndExit( "select: double comma." );

                bool comma2_flag = false;
                if (*buf == ','){
                    comma2_flag = true;
                    buf++;
                    while ( *buf == ' ' || *buf == '\t' ) buf++;
                }
                script_h.setCurrent(buf);

                if (*buf == 0x0a){
                    comma_flag |= comma2_flag;
                    continue;
                }

                if (!comma_flag && !comma2_flag){
                    select_label_info.next_script = buf;
                    //printf("select: stop at the end of line\n");
                    break;
                }

                //printf("continue\n");
                comma_flag = true;
            }
            else{ // if select ends at the middle of the line
                select_label_info.next_script = script_h.getNext();
                //printf("select: stop at the middle of the line\n");
                break;
            }
        }

        if ( select_mode != SELECT_CSEL_MODE ){
            last_select_link = root_select_link.next;
            int counter = 1;
            while( last_select_link ){
                if ( *last_select_link->text ){
                    ButtonLink *button = getSelectableSentence( last_select_link->text, &sentence_font );
                    root_button_link.insert( button );
                    button->no = counter;
                }
                counter++;
                last_select_link = last_select_link->next;
            }
        }

        if ( select_mode == SELECT_CSEL_MODE ){
            setCurrentLabel( "customsel" );
            return RET_CONTINUE;
        }
        skip_mode &= ~SKIP_NORMAL;
        automode_flag = false;
        sentence_font.xy[0] = xy[0];
        sentence_font.xy[1] = xy[1];

        flush( refreshMode() );

        event_mode = WAIT_TEXT_MODE | WAIT_BUTTON_MODE | WAIT_TIMER_MODE;
        advancePhase();
        refreshMouseOverButton();

        return RET_WAIT | RET_REREAD;
    }
}

int ONScripterLabel::savetimeCommand()
{
    int no = script_h.readInt();

    SaveFileInfo info;
    searchSaveFile( info, no );

    script_h.readVariable();
    if ( !info.valid ){
        script_h.setInt( &script_h.current_variable, 0 );
        for ( int i=0 ; i<3 ; i++ )
            script_h.readVariable();
        return RET_CONTINUE;
    }

    script_h.setInt( &script_h.current_variable, info.month );
    script_h.readInt();
    script_h.setInt( &script_h.current_variable, info.day );
    script_h.readInt();
    script_h.setInt( &script_h.current_variable, info.hour );
    script_h.readInt();
    script_h.setInt( &script_h.current_variable, info.minute );

    return RET_CONTINUE;
}

int ONScripterLabel::savescreenshotCommand()
{
    if      ( script_h.isName( "savescreenshot" ) ){
    }
    else if ( script_h.isName( "savescreenshot2" ) ){
    }

    const char *buf = script_h.readStr();
    char filename[4096];

    char *ext = strrchr( buf, '.' );
    if ( ext && (!strcmp( ext+1, "BMP" ) || !strcmp( ext+1, "bmp" ) ) ){
        sprintf( filename, "%s%s", script_h.save_path, buf );
        int last_delim = 0;
	for ( unsigned int i=0 ; i<strlen( filename ) ; i++ ) {
            if ( filename[i] == '/' || filename[i] == '\\' ) {
                filename[i] = DELIMITER;
		last_delim = i;
	    }
	}
	if (last_delim) {	
	    filename[last_delim] = 0;
	    mkdir(filename
#ifndef WIN32
		  , 0755
#endif
		);
	    filename[last_delim] = DELIMITER;
	}
        SDL_SaveBMP( screenshot_surface, filename );
    }
    else
        printf("savescreenshot: file %s is not supported.\n", buf );

    return RET_CONTINUE;
}

int ONScripterLabel::saveonCommand()
{
    saveon_flag = true;

    return RET_CONTINUE;
}

int ONScripterLabel::saveoffCommand()
{
    saveon_flag = false;

    return RET_CONTINUE;
}

int ONScripterLabel::savegameCommand()
{
    bool savegame2_flag = false;
    if ( script_h.isName( "savegame2" ) )
        savegame2_flag = true;
    
    int no = script_h.readInt();

    const char* savestr = NULL;
    if (savegame2_flag)
        savestr = script_h.readStr();

    if ( no < 0 )
        errorAndExit("savegame: the specified number is less than 0.");
    else{
        shelter_event_mode = event_mode;
        saveSaveFile( no, savestr ); 
    }

    return RET_CONTINUE;
}

int ONScripterLabel::savefileexistCommand()
{
    script_h.readInt();
    script_h.pushVariable();
    int no = script_h.readInt();

    SaveFileInfo info;
    searchSaveFile( info, no );

    script_h.setInt( &script_h.pushed_variable, (info.valid==true)?1:0 );

    return RET_CONTINUE;
}

int ONScripterLabel::rndCommand()
{
    int upper, lower;

    if ( script_h.isName( "rnd2" ) ){
        script_h.readInt();
        script_h.pushVariable();

        lower = script_h.readInt();
        upper = script_h.readInt();
    }
    else{
        script_h.readInt();
        script_h.pushVariable();

        lower = 0;
        upper = script_h.readInt() - 1;
    }

    script_h.setInt( &script_h.pushed_variable, lower + (int)( (double)(upper-lower+1)*rand()/(RAND_MAX+1.0)) );

    return RET_CONTINUE;
}

int ONScripterLabel::rmodeCommand()
{
    if ( script_h.readInt() == 1 ) rmode_flag = true;
    else                           rmode_flag = false;

    return RET_CONTINUE;
}

int ONScripterLabel::resettimerCommand()
{
    internal_timer = SDL_GetTicks();
    return RET_CONTINUE;
}

int ONScripterLabel::resetCommand()
{
    resetSub();
    clearCurrentPage();

    setCurrentLabel( "start" );
    saveSaveFile(-1);

    return RET_CONTINUE;
}

int ONScripterLabel::repaintCommand()
{
    dirty_rect.fill( screen_width, screen_height );
    flush( refreshMode() );

    return RET_CONTINUE;
}

int ONScripterLabel::quakeCommand()
{
    int quake_type;

    if      ( script_h.isName( "quakey" ) ){
        quake_type = 0;
    }
    else if ( script_h.isName( "quakex" ) ){
        quake_type = 1;
    }
    else{
        quake_type = 2;
    }

    tmp_effect.no       = script_h.readInt();
    tmp_effect.duration = script_h.readInt();

#ifdef INSANI
	if ( ctrl_pressed_status || skip_mode & SKIP_TO_WAIT )
	{
		dirty_rect.fill( screen_width, screen_height );
        SDL_BlitSurface( accumulation_surface, NULL, effect_dst_surface, NULL );
        event_mode = IDLE_EVENT_MODE;
        return RET_CONTINUE;
	}
#endif

    if ( tmp_effect.duration < tmp_effect.no * 4 ) tmp_effect.duration = tmp_effect.no * 4;
    tmp_effect.effect   = CUSTOM_EFFECT_NO + quake_type;

    if ( event_mode & EFFECT_EVENT_MODE ){
        return doEffect( &tmp_effect );
    }
    else{
        dirty_rect.fill( screen_width, screen_height );
        SDL_BlitSurface( accumulation_surface, NULL, effect_dst_surface, NULL );

        return setEffect( &tmp_effect, false, true );
    }
}

int ONScripterLabel::puttextCommand()
{
    int ret = enterTextDisplayMode(false);
    if ( ret != RET_NOMATCH ) return ret;

    script_h.readStr();
    script_h.addStringBuffer(0x0a);
    if (script_h.getEndStatus() & ScriptHandler::END_1BYTE_CHAR &&
        string_buffer_offset == 0)
        string_buffer_offset = 1; // skip the heading `

    ret = processText();
    if (script_h.getStringBuffer()[string_buffer_offset] == 0x0a){
        ret = RET_CONTINUE; // suppress RET_CONTINUE | RET_NOREAD
        if (!sentence_font.isLineEmpty() && !new_line_skip_flag){
            doLineBreak();
        }
    }
    if (ret != RET_CONTINUE){
        ret &= ~RET_NOREAD;
        return ret | RET_REREAD;
    }

    string_buffer_offset = 0;

    return RET_CONTINUE;
}

int ONScripterLabel::prnumclearCommand()
{
    int ret = leaveTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    for ( int i=0 ; i<MAX_PARAM_NUM ; i++ ) {
        if ( prnum_info[i] ) {
            dirty_rect.add( prnum_info[i]->pos );
            delete prnum_info[i];
            prnum_info[i] = NULL;
        }
    }
    return RET_CONTINUE;
}

int ONScripterLabel::prnumCommand()
{
    int ret = leaveTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    int no = script_h.readInt();
    if ( prnum_info[no] ){
        dirty_rect.add( prnum_info[no]->pos );
        delete prnum_info[no];
    }
    prnum_info[no] = new AnimationInfo();
    prnum_info[no]->trans_mode = AnimationInfo::TRANS_STRING;
    prnum_info[no]->num_of_cells = 1;
    prnum_info[no]->setCell(0);
    prnum_info[no]->color_list = new uchar3[ prnum_info[no]->num_of_cells ];

    prnum_info[no]->param = script_h.readInt();
    prnum_info[no]->pos.x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    prnum_info[no]->pos.y = script_h.readInt() * screen_ratio1 / screen_ratio2;
    prnum_info[no]->font_size_xy[0] = script_h.readInt();
    prnum_info[no]->font_size_xy[1] = script_h.readInt();

    const char *buf = script_h.readStr();
    readColor( &prnum_info[no]->color_list[0], buf );

    char num_buf[7];
    // Use fullwidth digits
    script_h.getStringFromInteger( num_buf, prnum_info[no]->param, 3, false, true );
    setStr( &prnum_info[no]->file_name, num_buf );

    setupAnimationInfo( prnum_info[no] );
    dirty_rect.add( prnum_info[no]->pos );

    return RET_CONTINUE;
}

int ONScripterLabel::printCommand()
{
    int ret = leaveTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    if ( event_mode & EFFECT_EVENT_MODE ){
        return doEffect( parseEffect(false) );
    }
    else{
        return setEffect( parseEffect(true), true, true );
    }
}

int ONScripterLabel::playstopCommand()
{
    stopBGM( false );
    return RET_CONTINUE;
}

int ONScripterLabel::playCommand()
{
    bool loop_flag = true;
    if ( script_h.isName( "playonce" ) )
        loop_flag = false;

    const char *buf = script_h.readStr();
    if ( buf[0] == '*' ){
        cd_play_loop_flag = loop_flag;
        int new_cd_track = atoi( buf + 1 );
#ifdef CONTINUOUS_PLAY
        if ( current_cd_track != new_cd_track ) {
#endif
            stopBGM( false );
            current_cd_track = new_cd_track;
            playCDAudio();
#ifdef CONTINUOUS_PLAY
        }
#endif
    }
    else{ // play MIDI
        stopBGM( false );

        setStr(&midi_file_name, buf);
        midi_play_loop_flag = loop_flag;
        if (playSound(midi_file_name, SOUND_MIDI, midi_play_loop_flag) != SOUND_MIDI){
            fprintf(stderr, "can't play MIDI file %s\n", midi_file_name);
        }
    }

    return RET_CONTINUE;
}

int ONScripterLabel::ofscopyCommand()
{
    SDL_BlitSurface( screen_surface, NULL, accumulation_surface, NULL );

    return RET_CONTINUE;
}

int ONScripterLabel::negaCommand()
{
    nega_mode = script_h.readInt();

    dirty_rect.fill( screen_width, screen_height );

    return RET_CONTINUE;
}

int ONScripterLabel::mspCommand()
{
    int ret = leaveTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    bool msp2_flag = false;
    if (script_h.isName("msp2")) msp2_flag = true;

    int no = script_h.readInt();

    AnimationInfo *si=NULL;
    if (msp2_flag) {
        si = &sprite2_info[no];
        dirty_rect.add( si->bounding_rect );
    }
    else{
        si = &sprite_info[no];
        dirty_rect.add( si->pos );
    }

    si->pos.x += script_h.readInt() * screen_ratio1 / screen_ratio2;
    si->pos.y += script_h.readInt() * screen_ratio1 / screen_ratio2;

    if (msp2_flag){
        si->scale_x += script_h.readInt();
        si->scale_y += script_h.readInt();
        si->rot += script_h.readInt();
        si->calcAffineMatrix();
        dirty_rect.add( si->bounding_rect );
    }
    else{
        dirty_rect.add( si->pos );
    }
    
    if ( script_h.getEndStatus() & ScriptHandler::END_COMMA )
        si->trans += script_h.readInt();
    if ( si->trans > 256 ) si->trans = 256;
    else if ( si->trans < 0 ) si->trans = 0;

    return RET_CONTINUE;
}

int ONScripterLabel::mp3volCommand()
{
    music_volume = script_h.readInt();

    setCurMusicVolume(music_volume);

    return RET_CONTINUE;
}

int ONScripterLabel::mp3stopCommand()
{
    if ( playingMusic() && (mp3fadeout_duration > 0) ) {
        // do a bgm fadeout
        mp3fade_start = SDL_GetTicks();

        timer_mp3fadeout_id = SDL_AddTimer(20, mp3fadeoutCallback, NULL);

        event_mode |= WAIT_TIMER_MODE;
        return RET_WAIT;
    }

    stopBGM( false );
    return RET_CONTINUE;

}

//Mion: integrating mp3fadeout as it's supposed to work.
int ONScripterLabel::mp3fadeoutCommand()
{
    mp3fadeout_duration = script_h.readInt();

    return RET_CONTINUE;
}

int ONScripterLabel::mp3fadeinCommand()
{
    mp3fadein_duration = script_h.readInt();

    return RET_CONTINUE;
}

int ONScripterLabel::mp3Command()
{
    bool loop_flag = false;
    if      ( script_h.isName( "mp3save" ) ){
        mp3save_flag = true;
    }
    else if ( script_h.isName( "bgmonce" ) ){
        mp3save_flag = false;
    }
    else if ( script_h.isName( "mp3loop" ) ||
              script_h.isName( "bgm" ) ){
        mp3save_flag = true;
        loop_flag = true;
    }
    else{
        mp3save_flag = false;
    }

    if (!(event_mode & WAIT_TIMER_MODE)) {
        mp3stopCommand();
    }

    if (event_mode & WAIT_TIMER_MODE)
        return RET_WAIT | RET_REREAD;

    music_play_loop_flag = loop_flag;

    const char *buf = script_h.readStr();
    if (buf[0] != '\0'){
        int tmp = music_volume;
        setStr(&music_file_name, buf);

        if (mp3fadein_duration > 0) {
            music_volume = 0;
        }

        playSound(music_file_name,
                  SOUND_WAVE | SOUND_OGG_STREAMING | SOUND_MP3 | SOUND_MIDI,
                  music_play_loop_flag, MIX_BGM_CHANNEL);

        if (mp3fadein_duration > 0) {
            // do a bgm fadein
            music_volume = tmp;

            mp3fade_start = SDL_GetTicks();
            timer_mp3fadein_id = SDL_AddTimer(20, mp3fadeinCallback, NULL);

            event_mode |= WAIT_TIMER_MODE;
            return RET_WAIT;
        }
    }

    return RET_CONTINUE;
}

int ONScripterLabel::movieCommand()
{
    bool mpegplay_flag = false;

    if ( script_h.isName( "mpegplay" ) ){
        mpegplay_flag = true;
    } else {
        if ( script_h.compareString( "stop" ) ){
            script_h.readName();
            if (async_movie) stopMovie(async_movie);
            async_movie = NULL;

            return RET_CONTINUE;
        }
    }

    script_h.readStr();
    const char *save_buf = script_h.saveStringBuffer();

    stopBGM( false );

    if (mpegplay_flag) {
        movie_loop_flag = false;
        movie_click_flag = (script_h.readInt()==1)?true:false;
        if (playMPEG( save_buf, false )) endCommand();
        return RET_CONTINUE;
    }

    movie_click_flag = false;
    movie_loop_flag = false;
    bool async_flag = false;
    bool pos_flag = false;
    int x=0,y=0,w=0,h=0;

    while( script_h.getEndStatus() & ScriptHandler::END_COMMA ){
        const char *param = script_h.readName();
        if ( strcmp(param, "click") == 0 )
            movie_click_flag = true;
        else if ( strcmp(param, "loop") == 0 )
            movie_loop_flag = true;
        else if ( strcmp(param, "async") == 0 )
            async_flag = true;
        else if ( strcmp(param, "pos") == 0 ) {
            pos_flag = true;
            x = script_h.readInt();
            y = script_h.readInt();
            w = script_h.readInt();
            h = script_h.readInt();
        }
    }

    if (playMPEG( save_buf, async_flag, pos_flag, x, y, w, h )) endCommand();
    return RET_CONTINUE;
}

int ONScripterLabel::movemousecursorCommand()
{
    int x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    int y = script_h.readInt() * screen_ratio1 / screen_ratio2;

    SDL_WarpMouse( x, y );

    return RET_CONTINUE;
}

int ONScripterLabel::monocroCommand()
{
    if ( script_h.compareString( "off" ) ){
        script_h.readName();
        monocro_flag = false;
    }
    else{
        monocro_flag = true;
        readColor( &monocro_color, script_h.readStr() );

        for (int i=0 ; i<256 ; i++){
            monocro_color_lut[i][0] = (monocro_color[0] * i) >> 8;
            monocro_color_lut[i][1] = (monocro_color[1] * i) >> 8;
            monocro_color_lut[i][2] = (monocro_color[2] * i) >> 8;
        }
    }

    dirty_rect.fill( screen_width, screen_height );

    return RET_CONTINUE;
}

int ONScripterLabel::minimizewindowCommand()
{
#ifndef PSP
    SDL_WM_IconifyWindow();
#endif

    return RET_CONTINUE;
}

int ONScripterLabel::menu_windowCommand()
{
    if ( fullscreen_mode ){
#ifndef PSP
        if ( !SDL_WM_ToggleFullScreen( screen_surface ) ){
            SDL_FreeSurface(screen_surface);
            screen_surface = SDL_SetVideoMode( screen_width, screen_height, screen_bpp, DEFAULT_VIDEO_SURFACE_FLAG );
            SDL_Rect rect = {0, 0, screen_width, screen_height};
            flushDirect( rect, refreshMode() );
        }
#endif
        fullscreen_mode = false;
    }

    return RET_CONTINUE;
}

int ONScripterLabel::menu_fullCommand()
{
    if ( !fullscreen_mode ){
#ifndef PSP
        if ( !SDL_WM_ToggleFullScreen( screen_surface ) ){
            SDL_FreeSurface(screen_surface);
            screen_surface = SDL_SetVideoMode( screen_width, screen_height, screen_bpp, DEFAULT_VIDEO_SURFACE_FLAG|SDL_FULLSCREEN );
            SDL_Rect rect = {0, 0, screen_width, screen_height};
            flushDirect( rect, refreshMode() );
        }
#endif
        fullscreen_mode = true;
    }

    return RET_CONTINUE;
}

int ONScripterLabel::menu_click_pageCommand()
{
    skip_mode |= SKIP_TO_EOP;
    printf("menu_click_page: enabling page-at-once mode\n");

    return RET_CONTINUE;
}

int ONScripterLabel::menu_click_defCommand()
{
    skip_mode &= ~SKIP_TO_EOP;
    printf("menu_click_def: disabling page-at-once mode\n");

    return RET_CONTINUE;
}

int ONScripterLabel::menu_automodeCommand()
{
    automode_flag = true;
    skip_mode &= ~SKIP_NORMAL;
    printf("menu_automode: change to automode\n");

    return RET_CONTINUE;
}

int ONScripterLabel::lsp2Command()
{
    int ret = leaveTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    bool v=true;

    if ( script_h.isName( "lsph2" ) || script_h.isName( "lsph2add" ) ||
         script_h.isName( "lsph2sub" ) )
        v = false;

    int blend_mode = AnimationInfo::BLEND_NORMAL;
    if ( script_h.isName( "lsp2add" ) || script_h.isName( "lsph2add" ) )
        blend_mode = AnimationInfo::BLEND_ADD;
    else if ( script_h.isName( "lsp2sub" ) || script_h.isName( "lsph2sub" ) )
        blend_mode = AnimationInfo::BLEND_SUB;

    int no = script_h.readInt();
    if ( sprite2_info[no].visible )
        dirty_rect.add( sprite2_info[no].bounding_rect );
    sprite2_info[ no ].visible = v;
    sprite2_info[ no ].blending_mode = blend_mode;
    
    const char *buf = script_h.readStr();
    sprite2_info[ no ].setImageName( buf );

#ifdef RCA_SCALE
    sprite2_info[ no ].pos.x = script_h.readInt() * screen_ratio1 * scr_stretch_x / screen_ratio2;
    sprite2_info[ no ].pos.y = script_h.readInt() * screen_ratio1 * scr_stretch_y / screen_ratio2;
#else
    sprite2_info[ no ].pos.x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    sprite2_info[ no ].pos.y = script_h.readInt() * screen_ratio1 / screen_ratio2;
#endif
    sprite2_info[ no ].scale_x = script_h.readInt();
    sprite2_info[ no ].scale_y = script_h.readInt();
    sprite2_info[ no ].rot = script_h.readInt();

    if ( script_h.getEndStatus() & ScriptHandler::END_COMMA )
        sprite2_info[ no ].trans = script_h.readInt();
    else
        sprite2_info[ no ].trans = 256;

    parseTaggedString( &sprite2_info[ no ] );
    setupAnimationInfo( &sprite2_info[ no ] );
    sprite2_info[ no ].calcAffineMatrix();

#ifdef RCA_SCALE
    if (sprite2_info[ no ].image_surface 
        && ( scr_stretch_y > 1.0 || scr_stretch_x > 1.0 )) {
        SDL_Surface* src = sprite2_info[ no ].image_surface;
        SDL_PixelFormat *fmt = src->format;
        SDL_Surface* dst = SDL_CreateRGBSurface( SDL_SWSURFACE, 
                                                 scr_stretch_x*src->w,
                                                 scr_stretch_y*src->h,
                                                 fmt->BitsPerPixel, fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask );
        resizeSurface( src, dst );
        sprite2_info[ no ].image_surface = dst;
        sprite2_info[ no ].pos.w *= scr_stretch_x;
        sprite2_info[ no ].pos.h *= scr_stretch_y;
        SDL_FreeSurface( src );

    }
#endif

    if ( sprite2_info[no].visible )
        dirty_rect.add( sprite2_info[no].bounding_rect );

    return RET_CONTINUE;
}

int ONScripterLabel::lspCommand()
{
    int ret = leaveTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    bool v=true;

    if ( script_h.isName( "lsph" ) )
        v = false;

    int no = script_h.readInt();
    if ( sprite_info[no].visible )
        dirty_rect.add( sprite_info[no].pos );
    sprite_info[ no ].visible = v;

    const char *buf = script_h.readStr();
    sprite_info[ no ].setImageName( buf );

#ifdef RCA_SCALE
    sprite_info[ no ].pos.x = script_h.readInt() * screen_ratio1 * scr_stretch_x / screen_ratio2;
    sprite_info[ no ].pos.y = script_h.readInt() * screen_ratio1 * scr_stretch_y / screen_ratio2;
#else
    sprite_info[ no ].pos.x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    sprite_info[ no ].pos.y = script_h.readInt() * screen_ratio1 / screen_ratio2;
#endif
    
    if ( script_h.getEndStatus() & ScriptHandler::END_COMMA )
        sprite_info[ no ].trans = script_h.readInt();
    else
        sprite_info[ no ].trans = 256;

    parseTaggedString( &sprite_info[ no ] );
    setupAnimationInfo( &sprite_info[ no ] );
#ifdef RCA_SCALE
    if (sprite_info[ no ].image_surface 
        && ( scr_stretch_y > 1.0 || scr_stretch_x > 1.0 )) {
        SDL_Surface* src = sprite_info[ no ].image_surface;
        SDL_PixelFormat *fmt = src->format;
        SDL_Surface* dst = SDL_CreateRGBSurface( SDL_SWSURFACE, 
                                                 scr_stretch_x*src->w,
                                                 scr_stretch_y*src->h,
                                                 fmt->BitsPerPixel, fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask );
        resizeSurface( src, dst );
        sprite_info[ no ].image_surface = dst;
        sprite_info[ no ].pos.w *= scr_stretch_x;
        sprite_info[ no ].pos.h *= scr_stretch_y;
        SDL_FreeSurface( src );

    }
#endif
    if ( sprite_info[no].visible )
        dirty_rect.add( sprite_info[no].pos );

    return RET_CONTINUE;
}

int ONScripterLabel::loopbgmstopCommand()
{
    if ( wave_sample[MIX_LOOPBGM_CHANNEL0] ){
        Mix_Pause(MIX_LOOPBGM_CHANNEL0);
        Mix_FreeChunk( wave_sample[MIX_LOOPBGM_CHANNEL0] );
        wave_sample[MIX_LOOPBGM_CHANNEL0] = NULL;
    }
    if ( wave_sample[MIX_LOOPBGM_CHANNEL1] ){
        Mix_Pause(MIX_LOOPBGM_CHANNEL1);
        Mix_FreeChunk( wave_sample[MIX_LOOPBGM_CHANNEL1] );
        wave_sample[MIX_LOOPBGM_CHANNEL1] = NULL;
    }
    setStr(&loop_bgm_name[0], NULL);

    return RET_CONTINUE;
}

int ONScripterLabel::loopbgmCommand()
{
    const char *buf = script_h.readStr();
    setStr( &loop_bgm_name[0], buf );
    buf = script_h.readStr();
    setStr( &loop_bgm_name[1], buf );

    playSound(loop_bgm_name[1],
              SOUND_PRELOAD|SOUND_WAVE|SOUND_OGG, false, MIX_LOOPBGM_CHANNEL1);
    playSound(loop_bgm_name[0],
              SOUND_WAVE|SOUND_OGG, false, MIX_LOOPBGM_CHANNEL0);

    return RET_CONTINUE;
}

int ONScripterLabel::lookbackflushCommand()
{
    current_page = current_page->next;
    for ( int i=0 ; i<max_page_list-1 ; i++ ){
        current_page->text_count = 0;
        current_page = current_page->next;
    }
    clearCurrentPage();
    start_page = current_page;

    return RET_CONTINUE;
}

int ONScripterLabel::lookbackbuttonCommand()
{
    for ( int i=0 ; i<4 ; i++ ){
        const char *buf = script_h.readStr();
        setStr( &lookback_info[i].image_name, buf );
        parseTaggedString( &lookback_info[i] );
        setupAnimationInfo( &lookback_info[i] );
    }
    return RET_CONTINUE;
}

int ONScripterLabel::logspCommand()
{
    int ret = leaveTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    bool logsp2_flag = false;

    if ( script_h.isName( "logsp2" ) )
        logsp2_flag = true;

    int sprite_no = script_h.readInt();

    AnimationInfo &si = sprite_info[sprite_no];
    if ( si.visible ) dirty_rect.add( si.pos );
    si.remove();
    setStr( &si.file_name, script_h.readStr() );

    si.pos.x = script_h.readInt();
    si.pos.y = script_h.readInt();

    si.trans_mode = AnimationInfo::TRANS_STRING;
    if (logsp2_flag){
        si.font_size_xy[0] = script_h.readInt();
        si.font_size_xy[1] = script_h.readInt();
        si.font_pitch = script_h.readInt() + si.font_size_xy[0];
        script_h.readInt(); // dummy read for y pitch
    }
    else{
        si.font_size_xy[0] = sentence_font.font_size_xy[0];
        si.font_size_xy[1] = sentence_font.font_size_xy[1];
        si.font_pitch = sentence_font.pitch_xy[0];
    }

    char *current = script_h.getNext();
    int num = 0;
    while(script_h.getEndStatus() & ScriptHandler::END_COMMA){
        script_h.readStr();
        num++;
    }

    script_h.setCurrent(current);
    if (num == 0){
        si.num_of_cells = 1;
        si.color_list = new uchar3[ si.num_of_cells ];
        readColor( &si.color_list[0], "#ffffff" );
    }
    else{
        si.num_of_cells = num;
        si.color_list = new uchar3[ si.num_of_cells ];
        for (int i=0 ; i<num ; i++){
            readColor( &si.color_list[i], script_h.readStr() );
        }
    }

    si.is_single_line = false;
    si.is_tight_region = false;
    si.is_ruby_drawable = true;
#ifdef ENABLE_1BYTE_CHAR
    si.skip_whitespace = false;
#endif
    sentence_font.is_newline_accepted = true;
    setupAnimationInfo( &si );
    sentence_font.is_newline_accepted = false;
    si.visible = true;
    dirty_rect.add( si.pos );

    return RET_CONTINUE;
}

int ONScripterLabel::locateCommand()
{
    int x = script_h.readInt();
    int y = script_h.readInt();

    int cur_x = sentence_font.xy[0]/2;
    int cur_y = sentence_font.xy[1]/2;
    //printf("locate %d, %d: cur_x: %d, cur_y: %d\n", x, y, cur_x, cur_y);

    //Mion: should adjust the current page buffer
    if (y < cur_y) {
        for (int i=y; i < cur_y; i++)
            current_page->add(TEXT_UP);
    } else if (cur_y < y) {
        for (int i=cur_y; i < y; i++)
            current_page->add(TEXT_DOWN);
    } else if (sentence_font.xy[1] != cur_y * 2) {
        // "halfway" through a character
        current_page->add(TEXT_DOWN);
        current_page->add(TEXT_UP);
    }
    if (x < cur_x) {
        for (int i=x; i < cur_x; i++)
            current_page->add(TEXT_LEFT);
    } else if (cur_x < x) {
        for (int i=cur_x; i < x; i++)
            current_page->add(TEXT_RIGHT);
    } else if (sentence_font.xy[0] != cur_x * 2) {
        // "halfway" through a character
        current_page->add(TEXT_LEFT);
        current_page->add(TEXT_RIGHT);
    }

    sentence_font.setXY( x, y );

    return RET_CONTINUE;
}

int ONScripterLabel::loadgameCommand()
{
    int no = script_h.readInt();

    if ( no < 0 )
        errorAndExit( "loadgame: no < 0." );

    if ( loadSaveFile( no ) ) return RET_CONTINUE;
    else {
        dirty_rect.fill( screen_width, screen_height );
        flush( refreshMode() );

        saveon_flag = true;
        internal_saveon_flag = true;
        skip_mode &= ~SKIP_NORMAL;
        automode_flag = false;
        deleteButtonLink();
        deleteSelectLink();
        key_pressed_flag = false;
        text_on_flag = false;
        indent_offset = 0;
        line_enter_status = 0;
        page_enter_status = 0;
        string_buffer_offset = 0;
	break_flag = false;

        refreshMouseOverButton();

        if (loadgosub_label)
            gosubReal( loadgosub_label, script_h.getCurrent() );
        readToken();

        if ( event_mode & WAIT_INPUT_MODE ) return RET_WAIT | RET_NOREAD;
        return RET_CONTINUE | RET_NOREAD;
    }
}

int ONScripterLabel::linkcolorCommand()
{
    const char *buf;
    
    buf = script_h.readStr();
    readColor( &linkcolor[0], buf );
    buf = script_h.readStr();
    readColor( &linkcolor[1], buf );

    return RET_CONTINUE;
}

int ONScripterLabel::ldCommand()
{
    //Mion: remove text to animate sprites, if in normal mode
    int ret = leaveTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    char loc = script_h.readName()[0];
    int no = -1;
    if      (loc == 'l') no = 0;
    else if (loc == 'c') no = 1;
    else if (loc == 'r') no = 2;

    const char *buf = NULL;
    if (no >= 0) buf = script_h.readStr();

    if ( event_mode & EFFECT_EVENT_MODE ){
        return doEffect( parseEffect(false) );
    }
    else{
        if (no >= 0){
            dirty_rect.add( tachi_info[ no ].pos );
            tachi_info[ no ].setImageName( buf );
            parseTaggedString( &tachi_info[ no ] );
            setupAnimationInfo( &tachi_info[ no ] );
            if ( tachi_info[ no ].image_surface ){
                tachi_info[ no ].visible = true;
#ifdef RCA_SCALE
                if (scr_stretch_y > 1.0) {
                    // RCA: Stretch characters to screen size.
                    // Note all stretches are with Y-scale, so they don't get distorted (FIXME assumes widescreen)
                    SDL_Surface* src = tachi_info[ no ].image_surface;
                    SDL_PixelFormat *fmt = src->format;
                    SDL_Surface* dst = SDL_CreateRGBSurface( SDL_SWSURFACE, 
                                                             scr_stretch_y*src->w, 
                                                             scr_stretch_y*src->h,
                                                             fmt->BitsPerPixel, fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask );
                    resizeSurface( src, dst );
                    tachi_info[ no ].image_surface = dst;
                    tachi_info[ no ].pos.w *= scr_stretch_y;
                    tachi_info[ no ].pos.h *= scr_stretch_y;
                    SDL_FreeSurface( src );
                    
                }
#endif
                tachi_info[ no ].pos.x = screen_width * (no+1) / 4 - tachi_info[ no ].pos.w / 2;
                tachi_info[ no ].pos.y = underline_value - tachi_info[ no ].image_surface->h + 1;
                dirty_rect.add( tachi_info[ no ].pos );
            }
        }

        return setEffect( parseEffect(true), true, true );
    }
}

int ONScripterLabel::layermessageCommand()
{
    int no = script_h.readInt();
    const char *message = script_h.readStr();

    LayerInfo *tmp = layer_info;
    while (tmp) {
        if ( tmp->num == no ) break;
        tmp = tmp->next;
    }
    if (tmp) {
        getret_str = tmp->handler->message(message, getret_int);
        //printf("layermessage returned: '%s', %d\n", getret_str, getret_int);
    }

    return RET_CONTINUE;
}

int ONScripterLabel::languageCommand()
{
    const char* which = script_h.readName();
    if ( strcmp(which, "japanese") == 0 ){
	script_h.preferred_script = ScriptHandler::JAPANESE_SCRIPT;
    }
    else if ( strcmp(which, "english") == 0 ){
#ifdef ENABLE_1BYTE_CHAR	
	script_h.preferred_script = ScriptHandler::LATIN_SCRIPT;
#else
	errorAndExit("cannot set language to english: this ONScripter was "
		     "not compiled with the ENABLE_1BYTE_CHAR option");
#endif	
    }
    else {
	char errbuf[2048];
	snprintf(errbuf, 2047, "unknown language '%s'", which);
	errorAndExit(errbuf, "valid options are 'japanese' and 'english'");
    }
    return RET_CONTINUE;
}

int ONScripterLabel::jumpfCommand()
{
    char *buf = script_h.getNext();
    while(*buf != '\0' && *buf != '~') buf++;
    if (*buf == '~') buf++;

    script_h.setCurrent(buf);
    current_label_info = script_h.getLabelByAddress(buf);
    current_line = script_h.getLineByAddress(buf);

    return RET_CONTINUE;
}

int ONScripterLabel::jumpbCommand()
{
    script_h.setCurrent( last_tilde.next_script );
    current_label_info = script_h.getLabelByAddress( last_tilde.next_script );
    current_line = script_h.getLineByAddress( last_tilde.next_script );

    return RET_CONTINUE;
}

int ONScripterLabel::ispageCommand()
{
    script_h.readInt();

    if ( textgosub_clickstr_state == CLICK_NEWPAGE )
        script_h.setInt( &script_h.current_variable, 1 );
    else
        script_h.setInt( &script_h.current_variable, 0 );

    return RET_CONTINUE;
}

int ONScripterLabel::isfullCommand()
{
    script_h.readInt();
    script_h.setInt( &script_h.current_variable, fullscreen_mode?1:0 );

    return RET_CONTINUE;
}

int ONScripterLabel::isskipCommand()
{
    script_h.readInt();

    if ( automode_flag )
        script_h.setInt( &script_h.current_variable, 2 );
    else if ( skip_mode & SKIP_NORMAL )
        script_h.setInt( &script_h.current_variable, 1 );
#ifdef INSANI
    else if ( ctrl_pressed_status )
        script_h.setInt( &script_h.current_variable, 3 );
    else if ( skip_mode & SKIP_TO_EOP )
        script_h.setInt( &script_h.current_variable, 4 );
#endif
    else
        script_h.setInt( &script_h.current_variable, 0 );

    return RET_CONTINUE;
}

int ONScripterLabel::isdownCommand()
{
    script_h.readInt();

    if ( current_button_state.down_flag )
        script_h.setInt( &script_h.current_variable, 1 );
    else
        script_h.setInt( &script_h.current_variable, 0 );

    return RET_CONTINUE;
}

int ONScripterLabel::inputCommand()
{
    script_h.readStr();

    if ( script_h.current_variable.type != ScriptHandler::VAR_STR )
        errorAndExit( "input: no string variable." );
    int no = script_h.current_variable.var_no;

    script_h.readStr(); // description
    const char *buf = script_h.readStr(); // default value
    setStr( &script_h.getVariableData(no).str, buf );

    printf( "*** inputCommand(): $%d is set to the default value: %s\n",
            no, buf );
    script_h.readInt(); // maxlen
    script_h.readInt(); // widechar flag
    if ( script_h.getEndStatus() & ScriptHandler::END_COMMA ){
        script_h.readInt(); // window width
        script_h.readInt(); // window height
        script_h.readInt(); // text box width
        script_h.readInt(); // text box height
    }

    return RET_CONTINUE;
}

int ONScripterLabel::indentCommand()
{
    indent_offset = script_h.readInt();

    return RET_CONTINUE;
}

int ONScripterLabel::humanorderCommand()
{
    int ret = leaveTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    const char *buf = script_h.readStr();
    int i;
    for (i=0 ; i<3 ; i++){
        if      (buf[i] == 'l') human_order[i] = 0;
        else if (buf[i] == 'c') human_order[i] = 1;
        else if (buf[i] == 'r') human_order[i] = 2;
        else                    human_order[i] = -1;
    }

    if ( event_mode & EFFECT_EVENT_MODE ){
        return doEffect( parseEffect(false) );
    }
    else{
        for ( i=0 ; i<3 ; i++ )
            dirty_rect.add( tachi_info[i].pos );

        return setEffect( parseEffect(true), true, true );
    }
}

int ONScripterLabel::getzxcCommand()
{
    getzxc_flag = true;

    return RET_CONTINUE;
}

int ONScripterLabel::getvoicevolCommand()
{
    script_h.readInt();
    script_h.setInt( &script_h.current_variable, voice_volume );
    return RET_CONTINUE;
}

int ONScripterLabel::getversionCommand()
{
    script_h.readInt();
    script_h.setInt( &script_h.current_variable, NSC_VERSION );

    return RET_CONTINUE;
}

int ONScripterLabel::gettimerCommand()
{
    bool gettimer_flag=false;

    if      ( script_h.isName( "gettimer" ) ){
        gettimer_flag = true;
    }
    else if ( script_h.isName( "getbtntimer" ) ){
    }

    script_h.readInt();

    if ( gettimer_flag ){
        script_h.setInt( &script_h.current_variable, SDL_GetTicks() - internal_timer );
    }
    else{
        script_h.setInt( &script_h.current_variable, btnwait_time );
    }

    return RET_CONTINUE;
}

int ONScripterLabel::gettextbtnstrCommand()
{
    script_h.readVariable();
    script_h.pushVariable();

    int txtbtn_no = script_h.readInt();

    TextButtonInfoLink *info = text_button_info.next;
    TextButtonInfoLink *found = NULL;
    while (info) {
        if (info->no == txtbtn_no)
            found = info;
        info = info->next;
    }

    if (found)
        setStr(&script_h.getVariableData( script_h.pushed_variable.var_no ).str, found->text);
    else
        setStr(&script_h.getVariableData( script_h.pushed_variable.var_no ).str, NULL);

    return RET_CONTINUE;
}

int ONScripterLabel::gettextCommand()
{
    script_h.readStr();
    int no = script_h.current_variable.var_no;

    char *buf = new char[ current_page->text_count + 1 ];
    int i, j;
    for ( i=0, j=0 ; i<current_page->text_count ; i++ ){
        if ( current_page->text[i] != 0x0a )
            buf[j++] = current_page->text[i];
    }
    buf[j] = '\0';

    setStr( &script_h.getVariableData(no).str, buf );
    delete[] buf;

    return RET_CONTINUE;
}

int ONScripterLabel::gettaglogCommand()
{
    script_h.readVariable();
    script_h.pushVariable();

    int page_no = script_h.readInt();

    Page *page = current_page;
    while(page != start_page && page_no > 0){
        page_no--;
        page = page->previous;
    }

    if (page->tag)
        setStr(&script_h.getVariableData( script_h.pushed_variable.var_no ).str, page->tag);
    else
        setStr(&script_h.getVariableData( script_h.pushed_variable.var_no ).str, NULL);

    return RET_CONTINUE;
}

int ONScripterLabel::gettagCommand()
{
    if ( !last_nest_info->previous || last_nest_info->nest_mode != NestInfo::LABEL )
        errorAndExit( "gettag: not in a subroutine, i.e. pretextgosub" );

    char *buf = current_tag.tag;

    int end_status;
    do{
        script_h.readVariable();
        end_status = script_h.getEndStatus();
        script_h.pushVariable();

        if ( script_h.pushed_variable.type & ScriptHandler::VAR_INT ||
             script_h.pushed_variable.type & ScriptHandler::VAR_ARRAY ){
            if (buf)
                script_h.setInt( &script_h.pushed_variable, script_h.parseInt(&buf));
            else
                script_h.setInt( &script_h.pushed_variable, 0);
        }
        else if ( script_h.pushed_variable.type & ScriptHandler::VAR_STR ){
            if (buf){
                const char *buf_start = buf;
                bool in_1byte_mode = false;
                while((in_1byte_mode || *buf != '/') && *buf != 0){
#ifdef ENABLE_1BYTE_CHAR
                    if (*buf == '`') {
                        in_1byte_mode = !in_1byte_mode;
                        buf++;
                    } else
#endif
                    if (IS_TWO_BYTE(*buf))
                        buf += 2;
                    else
                        buf++;
                }
                setStr( &script_h.getVariableData( script_h.pushed_variable.var_no ).str, buf_start, buf-buf_start );
            }
            else{
                setStr( &script_h.getVariableData( script_h.pushed_variable.var_no ).str, NULL);
            }
        }

        if (buf && *buf == '/')
            buf++;
        else
            buf = NULL;
    }
    while(end_status & ScriptHandler::END_COMMA);

    return RET_CONTINUE;
}

int ONScripterLabel::gettabCommand()
{
    gettab_flag = true;

    return RET_CONTINUE;
}

int ONScripterLabel::getspsizeCommand()
{
    int no = script_h.readInt();

    script_h.readVariable();
#ifndef RCA_SCALE
    script_h.setInt( &script_h.current_variable, sprite_info[no].pos.w * screen_ratio2 / screen_ratio1 );
#else
    script_h.setInt( &script_h.current_variable, sprite_info[no].pos.w * screen_ratio2 / ( screen_ratio1 * scr_stretch_x ) );
#endif
    script_h.readVariable();
#ifndef RCA_SCALE
    script_h.setInt( &script_h.current_variable, sprite_info[no].pos.h * screen_ratio2 / screen_ratio1 );
#else
    script_h.setInt( &script_h.current_variable, sprite_info[no].pos.h * screen_ratio2 / ( screen_ratio1 * scr_stretch_y ) );
#endif
    if ( script_h.getEndStatus() & ScriptHandler::END_COMMA ){
        script_h.readVariable();
        script_h.setInt( &script_h.current_variable, sprite_info[no].num_of_cells );
    }

    return RET_CONTINUE;
}

int ONScripterLabel::getspmodeCommand()
{
    script_h.readVariable();
    script_h.pushVariable();

    int no = script_h.readInt();
    script_h.setInt( &script_h.pushed_variable, sprite_info[no].visible?1:0 );

    return RET_CONTINUE;
}

int ONScripterLabel::getsevolCommand()
{
    script_h.readInt();
    script_h.setInt( &script_h.current_variable, se_volume );
    return RET_CONTINUE;
}

int ONScripterLabel::getscreenshotCommand()
{
    int w = script_h.readInt();
    int h = script_h.readInt();
    if ( w == 0 ) w = 1;
    if ( h == 0 ) h = 1;

    if ( screenshot_surface &&
         (screenshot_surface->w != w ||
         screenshot_surface->h != h )){
        SDL_FreeSurface( screenshot_surface );
        screenshot_surface = NULL;
    }

    if ( screenshot_surface == NULL )
        screenshot_surface = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 );

    SDL_Surface *surface = SDL_ConvertSurface( screen_surface, image_surface->format, SDL_SWSURFACE );
    resizeSurface( surface, screenshot_surface );
    SDL_FreeSurface( surface );

    return RET_CONTINUE;
}

int ONScripterLabel::getpageupCommand()
{
    getpageup_flag = true;

    return RET_CONTINUE;
}

int ONScripterLabel::getpageCommand()
{
    getpageup_flag = true;
    getpagedown_flag = true;

    return RET_CONTINUE;
}

int ONScripterLabel::getretCommand()
{
    script_h.readVariable();

    if ( script_h.current_variable.type == ScriptHandler::VAR_INT ||
         script_h.current_variable.type == ScriptHandler::VAR_ARRAY ){
        script_h.setInt( &script_h.current_variable, getret_int );
    }
    else if ( script_h.current_variable.type == ScriptHandler::VAR_STR ){
        int no = script_h.current_variable.var_no;
        setStr( &script_h.getVariableData(no).str, getret_str );
    }
    else errorAndExit( "getret: no variable." );

    return RET_CONTINUE;
}

int ONScripterLabel::getregCommand()
{
    script_h.readVariable();

    if ( script_h.current_variable.type != ScriptHandler::VAR_STR )
        errorAndExit( "getreg: no string variable." );
    int no = script_h.current_variable.var_no;

    const char *buf = script_h.readStr();
    char path[256], key[256];
    strcpy( path, buf );
    buf = script_h.readStr();
    strcpy( key, buf );

    printf("  reading Registry file for [%s] %s\n", path, key );

    FILE *fp;
    if ( ( fp = fopen( registry_file, "r" ) ) == NULL ){
        fprintf( stderr, "Cannot open file [%s]\n", registry_file );
        return RET_CONTINUE;
    }

    char reg_buf[256], reg_buf2[256];
    bool found_flag = false;
    while( fgets( reg_buf, 256, fp) && !found_flag ){
        if ( reg_buf[0] == '[' ){
            unsigned int c=0;
            while ( reg_buf[c] != ']' && reg_buf[c] != '\0' ) c++;
            if ( !strncmp( reg_buf + 1, path, (c-1>strlen(path))?(c-1):strlen(path) ) ){
                while( fgets( reg_buf2, 256, fp) ){

                    script_h.pushCurrent( reg_buf2 );
                    buf = script_h.readStr();
                    if ( strncmp( buf,
                                  key,
                                  (strlen(buf)>strlen(key))?strlen(buf):strlen(key) ) ){
                        script_h.popCurrent();
                        continue;
                    }

                    if ( !script_h.compareString("=") ){
                        script_h.popCurrent();
                        continue;
                    }
                    script_h.setCurrent(script_h.getNext()+1);

                    buf = script_h.readStr();
                    setStr( &script_h.getVariableData(no).str, buf );
                    script_h.popCurrent();
                    printf("  $%d = %s\n", no, script_h.getVariableData(no).str );
                    found_flag = true;
                    break;
                }
            }
        }
    }

    if ( !found_flag ) fprintf( stderr, "  The key is not found.\n" );
    fclose(fp);

    return RET_CONTINUE;
}

int ONScripterLabel::getmp3volCommand()
{
    script_h.readInt();
    script_h.setInt( &script_h.current_variable, music_volume );
    return RET_CONTINUE;
}

int ONScripterLabel::getmouseposCommand()
{
    script_h.readInt();
    script_h.setInt( &script_h.current_variable, current_button_state.x * screen_ratio2 / screen_ratio1 );

    script_h.readInt();
    script_h.setInt( &script_h.current_variable, current_button_state.y * screen_ratio2 / screen_ratio1 );

    return RET_CONTINUE;
}

int ONScripterLabel::getlogCommand()
{
    script_h.readVariable();
    script_h.pushVariable();

    int page_no = script_h.readInt();

    Page *page = current_page;
    while(page != start_page && page_no > 0){
        page_no--;
        page = page->previous;
    }

    if (page_no > 0)
        setStr( &script_h.getVariableData( script_h.pushed_variable.var_no ).str, NULL );
    else
        setStr( &script_h.getVariableData( script_h.pushed_variable.var_no ).str, page->text, page->text_count );

    return RET_CONTINUE;
}

int ONScripterLabel::getinsertCommand()
{
    getinsert_flag = true;

    return RET_CONTINUE;
}

int ONScripterLabel::getfunctionCommand()
{
    getfunction_flag = true;

    return RET_CONTINUE;
}

int ONScripterLabel::getenterCommand()
{
    if ( !force_button_shortcut_flag )
        getenter_flag = true;

    return RET_CONTINUE;
}

int ONScripterLabel::getcursorposCommand()
{
    script_h.readInt();
    //script_h.setInt( &script_h.current_variable, sentence_font.x() );
#ifndef RCA_SCALE
    script_h.setInt( &script_h.current_variable, sentence_font.x()-sentence_font.ruby_offset_xy[0] ); // workaround for possibly a bug in the original
#else
    script_h.setInt( &script_h.current_variable, sentence_font.x()/scr_stretch_x-sentence_font.ruby_offset_xy[0] ); // workaround for possibly a bug in the original
#endif

    script_h.readInt();
    //script_h.setInt( &script_h.current_variable, sentence_font.y() );
#ifndef RCA_SCALE
    script_h.setInt( &script_h.current_variable, sentence_font.y()-sentence_font.ruby_offset_xy[1] ); // workaround for possibly a bug in the original
#else
    script_h.setInt( &script_h.current_variable, sentence_font.y()/scr_stretch_y-sentence_font.ruby_offset_xy[1] ); // workaround for possibly a bug in the original
#endif

    return RET_CONTINUE;
}

int ONScripterLabel::getcursorCommand()
{
    if ( !force_button_shortcut_flag )
        getcursor_flag = true;

    return RET_CONTINUE;
}

int ONScripterLabel::getcselstrCommand()
{
    script_h.readVariable();
    script_h.pushVariable();

    int csel_no = script_h.readInt();

    int counter = 0;
    SelectLink *link = root_select_link.next;
    while (link){
        if (csel_no == counter++) break;
        link = link->next;
    }
    if (!link) errorAndExit("getcselstr: no select link");
    setStr(&script_h.getVariableData( script_h.pushed_variable.var_no ).str, link->text);

    return RET_CONTINUE;
}

int ONScripterLabel::getcselnumCommand()
{
    int count = 0;

    SelectLink *link = root_select_link.next;
    while ( link ) {
        count++;
        link = link->next;
    }
    script_h.readInt();
    script_h.setInt( &script_h.current_variable, count );

    return RET_CONTINUE;
}

int ONScripterLabel::gameCommand()
{
    int i;
    current_mode = NORMAL_MODE;

    /* ---------------------------------------- */
    /* Load global variables if available */
    if ( loadFileIOBuf( "gloval.sav" ) == 0 ||
         loadFileIOBuf( "global.sav" ) == 0 )
        readVariables( script_h.global_variable_border, VARIABLE_RANGE );

    /* ---------------------------------------- */
    if ( !lookback_info[0].image_surface ){
        setStr( &lookback_info[0].image_name, DEFAULT_LOOKBACK_NAME0 );
        parseTaggedString( &lookback_info[0] );
        setupAnimationInfo( &lookback_info[0] );
    }
    if ( !lookback_info[1].image_surface ){
        setStr( &lookback_info[1].image_name, DEFAULT_LOOKBACK_NAME1 );
        parseTaggedString( &lookback_info[1] );
        setupAnimationInfo( &lookback_info[1] );
    }
    if ( !lookback_info[2].image_surface ){
        setStr( &lookback_info[2].image_name, DEFAULT_LOOKBACK_NAME2 );
        parseTaggedString( &lookback_info[2] );
        setupAnimationInfo( &lookback_info[2] );
    }
    if ( !lookback_info[3].image_surface ){
        setStr( &lookback_info[3].image_name, DEFAULT_LOOKBACK_NAME3 );
        parseTaggedString( &lookback_info[3] );
        setupAnimationInfo( &lookback_info[3] );
    }

    /* ---------------------------------------- */
    /* Load default cursor */
    loadCursor( CURSOR_WAIT_NO, DEFAULT_CURSOR_WAIT, 0, 0 );
    loadCursor( CURSOR_NEWPAGE_NO, DEFAULT_CURSOR_NEWPAGE, 0, 0 );

    /* ---------------------------------------- */
    /* Initialize text buffer */
    page_list = new Page[max_page_list];
    for ( i=0 ; i<max_page_list-1 ; i++ ){
        page_list[i].next = &page_list[i+1];
        page_list[i+1].previous = &page_list[i];
    }
    page_list[0].previous = &page_list[max_page_list-1];
    page_list[max_page_list-1].next = &page_list[0];
    start_page = current_page = &page_list[0];

    clearCurrentPage();

    /* ---------------------------------------- */
    /* Initialize local variables */
    for ( i=0 ; i<script_h.global_variable_border ; i++ )
        script_h.getVariableData(i).reset(false);

    setCurrentLabel( "start" );
    saveSaveFile(-1);

    return RET_CONTINUE;
}

int ONScripterLabel::fileexistCommand()
{
    script_h.readInt();
    script_h.pushVariable();
    const char *buf = script_h.readStr();

    int found = (script_h.cBR->getFileLength(buf)>0)?1:0;
    if (!found) {
	char fn[4096];
	sprintf(fn, "%s%s", script_h.save_path, buf);
	char* si = fn;
	do { if (*si == '\\') *si = DELIMITER; } while (*(++si));
	FILE* fp = std::fopen(fn, "rb"); // FIXME: failing even when file exists?!
//printf("Seek %s, fp = %s, ", fn, fp ? "yes" : "no");
	if (fp) {
	    found = 1;
	    fclose(fp);
//puts("found\n");
	}
//else printf(" fail (%s)\n", strerror(errno));
    }
    script_h.setInt( &script_h.pushed_variable, found );

    return RET_CONTINUE;
}

int ONScripterLabel::exec_dllCommand()
{
    const char *buf = script_h.readStr();
    char dll_name[256];
    unsigned int c=0;
    while(buf[c] != '/'){
        dll_name[c] = buf[c];
        c++;
    }
    dll_name[c] = '\0';

    printf("  reading %s for %s\n", dll_file, dll_name );

    FILE *fp;
    if ( ( fp = fopen( dll_file, "r" ) ) == NULL ){
        fprintf( stderr, "Cannot open file [%s]\n", dll_file );
        return RET_CONTINUE;
    }

    char dll_buf[256], dll_buf2[256];
    bool found_flag = false;
    while( fgets( dll_buf, 256, fp) && !found_flag ){
        if ( dll_buf[0] == '[' ){
            c=0;
            while ( dll_buf[c] != ']' && dll_buf[c] != '\0' ) c++;
            if ( !strncmp( dll_buf + 1, dll_name, (c-1>strlen(dll_name))?(c-1):strlen(dll_name) ) ){
                found_flag = true;
                while( fgets( dll_buf2, 256, fp) ){
                    c=0;
                    while ( dll_buf2[c] == ' ' || dll_buf2[c] == '\t' ) c++;
                    if ( !strncmp( &dll_buf2[c], "str", 3 ) ){
                        c+=3;
                        while ( dll_buf2[c] == ' ' || dll_buf2[c] == '\t' ) c++;
                        if ( dll_buf2[c] != '=' ) continue;
                        c++;
                        while ( dll_buf2[c] != '"' ) c++;
                        unsigned int c2 = ++c;
                        while ( dll_buf2[c2] != '"' && dll_buf2[c2] != '\0' ) c2++;
                        dll_buf2[c2] = '\0';
                        setStr( &getret_str, &dll_buf2[c] );
                        printf("  getret_str = %s\n", getret_str );
                    }
                    else if ( !strncmp( &dll_buf2[c], "ret", 3 ) ){
                        c+=3;
                        while ( dll_buf2[c] == ' ' || dll_buf2[c] == '\t' ) c++;
                        if ( dll_buf2[c] != '=' ) continue;
                        c++;
                        while ( dll_buf2[c] == ' ' || dll_buf2[c] == '\t' ) c++;
                        getret_int = atoi( &dll_buf2[c] );
                        printf("  getret_int = %d\n", getret_int );
                    }
                    else if ( dll_buf2[c] == '[' )
                        break;
                }
            }
        }
    }

    if ( !found_flag ) fprintf( stderr, "  The DLL is not found in %s.\n", dll_file );
    fclose( fp );

    return RET_CONTINUE;
}

int ONScripterLabel::exbtnCommand()
{
    int sprite_no=-1, no=0;
    ButtonLink *button;

    if ( script_h.isName( "exbtn_d" ) ){
        button = &exbtn_d_button_link;
        if ( button->exbtn_ctl ) delete[] button->exbtn_ctl;
    }
    else{
        bool cellcheck_flag = false;

        if ( script_h.isName( "cellcheckexbtn" ) )
            cellcheck_flag = true;

        sprite_no = script_h.readInt();
        no = script_h.readInt();

        if ((cellcheck_flag  && (sprite_info[ sprite_no ].num_of_cells < 2)) ||
            (!cellcheck_flag && (sprite_info[ sprite_no ].num_of_cells == 0))){
            script_h.readStr();
            return RET_CONTINUE;
        }

        button = new ButtonLink();
        root_button_link.insert( button );
        is_exbtn_enabled = true;
    }

    const char *buf = script_h.readStr();

    button->button_type = ButtonLink::EX_SPRITE_BUTTON;
    button->sprite_no   = sprite_no;
    button->no          = no;
    button->exbtn_ctl   = new char[ strlen( buf ) + 1 ];
    strcpy( button->exbtn_ctl, buf );

    if ( sprite_no >= 0 &&
         ( sprite_info[ sprite_no ].image_surface ||
           sprite_info[ sprite_no ].trans_mode == AnimationInfo::TRANS_STRING ) )
    {
        button->image_rect = button->select_rect = sprite_info[ sprite_no ].pos;
        sprite_info[ sprite_no ].visible = true;
    }

    return RET_CONTINUE;
}

int ONScripterLabel::erasetextwindowCommand()
{
    erase_text_window_mode = script_h.readInt();
    dirty_rect.add( sentence_font_info.pos );

    return RET_CONTINUE;
}

int ONScripterLabel::erasetextbtnCommand()
{
    if (!txtbtn_visible) return RET_CONTINUE;

    TextButtonInfoLink *info = text_button_info.next;
    while (info) {
        ButtonLink *cur_button_link = info->button;
        while (cur_button_link) {
            cur_button_link->show_flag = 1;
            cur_button_link->anim[0]->visible = true;
            cur_button_link->anim[0]->setCell(0);
            dirty_rect.add( cur_button_link->image_rect );
            cur_button_link = cur_button_link->same;
        }
        info = info->next;
    }
    flush( refreshMode() );

    return RET_CONTINUE;
}

int ONScripterLabel::endCommand()
{
    if (async_movie) stopMovie(async_movie);
    async_movie = NULL;
    printf("Quitting...\n");
    quit();
    exit(0);
    return RET_CONTINUE; // dummy
}

int ONScripterLabel::dwavestopCommand()
{
    int ch = script_h.readInt();
    //Mion - ogapee2008: avoid dwavestop outside array
    if (ch < 0) ch = 0;
    else if (ch >= ONS_MIX_CHANNELS) ch = ONS_MIX_CHANNELS-1;

    if ( wave_sample[ch] ){
        Mix_Pause( ch );
        Mix_FreeChunk( wave_sample[ch] );
        wave_sample[ch] = NULL;
    }

    return RET_CONTINUE;
}

int ONScripterLabel::dwaveCommand()
{
    int play_mode = WAVE_PLAY;
    bool loop_flag = false;

    if ( script_h.isName( "dwaveloop" ) ){
        loop_flag = true;
    }
    else if ( script_h.isName( "dwaveload" ) ){
        play_mode = WAVE_PRELOAD;
    }
    else if ( script_h.isName( "dwaveplayloop" ) ){
        play_mode = WAVE_PLAY_LOADED;
        loop_flag = true;
    }
    else if ( script_h.isName( "dwaveplay" ) ){
        play_mode = WAVE_PLAY_LOADED;
        loop_flag = false;
    }

    int ch = script_h.readInt();
    if      (ch < 0) ch = 0;
    else if (ch >= ONS_MIX_CHANNELS) ch = ONS_MIX_CHANNELS-1;

    if (play_mode == WAVE_PLAY_LOADED){
        Mix_PlayChannel(ch, wave_sample[ch], loop_flag?-1:0);
    }
    else{
        const char *buf = script_h.readStr();
        int fmt = SOUND_WAVE|SOUND_OGG;
        if (play_mode == WAVE_PRELOAD) fmt |= SOUND_PRELOAD;
        playSound(buf, fmt, loop_flag, ch);
    }

    return RET_CONTINUE;
}

int ONScripterLabel::dvCommand()
{
    char buf[256];

    sprintf(buf, "voice%c%s.wav", DELIMITER, script_h.getStringBuffer()+2);
    playSound(buf, SOUND_WAVE|SOUND_OGG, false, 0);

    return RET_CONTINUE;
}

int ONScripterLabel::drawtextCommand()
{
    SDL_Rect clip = {0, 0, accumulation_surface->w, accumulation_surface->h};
    text_info.blendOnSurface( accumulation_surface, 0, 0, clip );

    return RET_CONTINUE;
}

int ONScripterLabel::drawsp3Command()
{
    int sprite_no = script_h.readInt();
    int cell_no = script_h.readInt();
    int alpha = script_h.readInt();
#ifndef RCA_SCALE
    int x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    int y = script_h.readInt() * screen_ratio1 / screen_ratio2;
#else
    int x = script_h.readInt() * screen_ratio1 * scr_stretch_x / screen_ratio2;
    int y = script_h.readInt() * screen_ratio1 * scr_stretch_y / screen_ratio2;
#endif

    AnimationInfo &si = sprite_info[sprite_no];
    int old_cell_no = si.current_cell;
    si.setCell(cell_no);

    si.mat[0][0] = script_h.readInt();
    si.mat[0][1] = script_h.readInt();
    si.mat[1][0] = script_h.readInt();
    si.mat[1][1] = script_h.readInt();

    int denom = (si.mat[0][0]*si.mat[1][1]-si.mat[0][1]*si.mat[1][0])/1000;
    if (denom != 0){
        si.inv_mat[0][0] =  si.mat[1][1] * 1000 / denom;
        si.inv_mat[0][1] = -si.mat[0][1] * 1000 / denom;
        si.inv_mat[1][0] = -si.mat[1][0] * 1000 / denom;
        si.inv_mat[1][1] =  si.mat[0][0] * 1000 / denom;
    }

    SDL_Rect clip = {0, 0, screen_surface->w, screen_surface->h};
    si.blendOnSurface2( accumulation_surface, x, y, clip, alpha );
    si.setCell(old_cell_no);

    return RET_CONTINUE;
}

int ONScripterLabel::drawsp2Command()
{
    int sprite_no = script_h.readInt();
    int cell_no = script_h.readInt();
    int alpha = script_h.readInt();

    AnimationInfo si = sprite_info[sprite_no];
#ifndef RCA_SCALE
    si.pos.x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    si.pos.y = script_h.readInt() * screen_ratio1 / screen_ratio2;
#else
    si.pos.x = script_h.readInt() * screen_ratio1 * scr_stretch_x / screen_ratio2;
    si.pos.y = script_h.readInt() * screen_ratio1 * scr_stretch_y / screen_ratio2;
#endif
    si.scale_x = script_h.readInt();
    si.scale_y = script_h.readInt();
    si.rot = script_h.readInt();
    si.calcAffineMatrix();
    si.setCell(cell_no);

    SDL_Rect clip = {0, 0, screen_surface->w, screen_surface->h};
    si.blendOnSurface2( accumulation_surface, si.pos.x, si.pos.y, clip, alpha );

    return RET_CONTINUE;
}

int ONScripterLabel::drawspCommand()
{
    int sprite_no = script_h.readInt();
    int cell_no = script_h.readInt();
    int alpha = script_h.readInt();
    int x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    int y = script_h.readInt() * screen_ratio1 / screen_ratio2;

    AnimationInfo &si = sprite_info[sprite_no];
    int old_cell_no = si.current_cell;
    si.setCell(cell_no);
    SDL_Rect clip = {0, 0, accumulation_surface->w, accumulation_surface->h};
    si.blendOnSurface( accumulation_surface, x, y, clip, alpha );
    si.setCell(old_cell_no);

    return RET_CONTINUE;
}

int ONScripterLabel::drawfillCommand()
{
    int r = script_h.readInt();
    int g = script_h.readInt();
    int b = script_h.readInt();

    SDL_FillRect( accumulation_surface, NULL, SDL_MapRGBA( accumulation_surface->format, r, g, b, 0xff) );

    return RET_CONTINUE;
}

int ONScripterLabel::drawclearCommand()
{
    SDL_FillRect( accumulation_surface, NULL, SDL_MapRGBA( accumulation_surface->format, 0, 0, 0, 0xff) );

    return RET_CONTINUE;
}

int ONScripterLabel::drawbgCommand()
{
    SDL_Rect clip = {0, 0, accumulation_surface->w, accumulation_surface->h};
    bg_info.blendOnSurface( accumulation_surface, bg_info.pos.x, bg_info.pos.y, clip );

    return RET_CONTINUE;
}

int ONScripterLabel::drawbg2Command()
{
    //Mion - ogapee2008
    AnimationInfo bi = bg_info;
    bi.pos.x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    bi.pos.y = script_h.readInt() * screen_ratio1 / screen_ratio2;
    bi.scale_x = script_h.readInt();
    bi.scale_y = script_h.readInt();
    bi.rot = script_h.readInt();
    bi.calcAffineMatrix();

    SDL_Rect clip = {0, 0, screen_surface->w, screen_surface->h};
    bi.blendOnSurface2( accumulation_surface, bi.pos.x, bi.pos.y,
                        clip, 256 );

    return RET_CONTINUE;
}

int ONScripterLabel::drawCommand()
{
    SDL_Rect rect = {0, 0, screen_width, screen_height};
    flushDirect( rect, REFRESH_NONE_MODE );
    dirty_rect.clear();

    return RET_CONTINUE;
}

int ONScripterLabel::deletescreenshotCommand()
{
    if ( screenshot_surface ) {
        SDL_FreeSurface( screenshot_surface );
        screenshot_surface = NULL;
    }
    return RET_CONTINUE;
}

int ONScripterLabel::delayCommand()
{
    int t = script_h.readInt();

    if ( event_mode & WAIT_INPUT_MODE ){
        event_mode = IDLE_EVENT_MODE;
        return RET_CONTINUE;
    }
    else{
        event_mode = WAIT_INPUT_MODE;
        key_pressed_flag = false;
        startTimer( t );
        return RET_WAIT | RET_REREAD;
    }
}

int ONScripterLabel::defineresetCommand()
{
    script_h.reset();
    ScriptParser::reset();
    reset();

    setCurrentLabel( "define" );

    return RET_CONTINUE;
}

int ONScripterLabel::cspCommand()
{
    int ret = leaveTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    bool csp2_flag = false;
    if (script_h.isName("csp2")) csp2_flag = true;

    int no = script_h.readInt();
    AnimationInfo *si = NULL;
    int num = 0;
    if (csp2_flag) {
        num = MAX_SPRITE2_NUM;
        si = sprite2_info;
    }
    else{
        num = MAX_SPRITE_NUM;
        si = sprite_info;
    }

    if ( no == -1 )
        for ( int i=0 ; i<num ; i++ ){
            if ( si[i].visible ){
                if (csp2_flag)
                    dirty_rect.add( si[i].bounding_rect );
                else
                    dirty_rect.add( si[i].pos );
            }
            if ( si[i].image_name ){
#ifndef RCA_SCALE
                si[i].pos.x = -1000 * screen_ratio1 / screen_ratio2;
                si[i].pos.y = -1000 * screen_ratio1 / screen_ratio2;
#else
                si[i].pos.x = -10000 * screen_ratio1 / screen_ratio2;
                si[i].pos.y = -10000 * screen_ratio1 / screen_ratio2;
#endif
            }
            if (!csp2_flag) root_button_link.removeSprite(i);
            si[i].remove();
        }
    else if (no >= 0 && no < MAX_SPRITE_NUM){
        if ( si[no].visible ) {
            if (csp2_flag)
                dirty_rect.add( si[no].bounding_rect );
            else
                dirty_rect.add( si[no].pos );
        }
        if (!csp2_flag) root_button_link.removeSprite(no);
        si[no].remove();
    }

    return RET_CONTINUE;
}

int ONScripterLabel::cselgotoCommand()
{
    int csel_no = script_h.readInt();

    int counter = 0;
    SelectLink *link = root_select_link.next;
    while( link ){
        if ( csel_no == counter++ ) break;
        link = link->next;
    }
    if ( !link ) errorAndExit( "cselgoto: no select link" );

    setCurrentLabel( link->label );

    deleteSelectLink();
    newPage( true );

    return RET_CONTINUE;
}

int ONScripterLabel::cselbtnCommand()
{
    int csel_no   = script_h.readInt();
    int button_no = script_h.readInt();

    Fontinfo csel_info = sentence_font;
    csel_info.setRubyOnFlag(false);
    csel_info.top_xy[0] = script_h.readInt();
    csel_info.top_xy[1] = script_h.readInt();

    int counter = 0;
    SelectLink *link = root_select_link.next;
    while ( link ){
        if ( csel_no == counter++ ) break;
        link = link->next;
    }
    if ( link == NULL || link->text == NULL || *link->text == '\0' )
        return RET_CONTINUE;

    csel_info.setLineArea( strlen(link->text)/2+1 );
    csel_info.clear();
    ButtonLink *button = getSelectableSentence( link->text, &csel_info );
    root_button_link.insert( button );
    button->no          = button_no;
    button->sprite_no   = csel_no;

    sentence_font.ttf_font = csel_info.ttf_font;

    return RET_CONTINUE;
}

int ONScripterLabel::clickCommand()
{
    if ( event_mode & WAIT_INPUT_MODE ){
        event_mode = IDLE_EVENT_MODE;
        return RET_CONTINUE;
    }
    else{
        skip_mode &= ~SKIP_NORMAL;
        event_mode = WAIT_INPUT_MODE | WAIT_TIMER_MODE;
        key_pressed_flag = false;
        advancePhase();
        return RET_WAIT | RET_REREAD;
    }
}

int ONScripterLabel::clCommand()
{
    int ret = leaveTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    char loc = script_h.readName()[0];

    if ( event_mode & EFFECT_EVENT_MODE ){
        return doEffect( parseEffect(false) );
    }
    else{
        if ( loc == 'l' || loc == 'a' ){
            dirty_rect.add( tachi_info[0].pos );
            tachi_info[0].remove();
        }
        if ( loc == 'c' || loc == 'a' ){
            dirty_rect.add( tachi_info[1].pos );
            tachi_info[1].remove();
        }
        if ( loc == 'r' || loc == 'a' ){
            dirty_rect.add( tachi_info[2].pos );
            tachi_info[2].remove();
        }

        return setEffect( parseEffect(true), true, true );
    }
}

int ONScripterLabel::chvolCommand()
{
    int ch  = script_h.readInt();
    //Mion - ogapee2008: avoid access outside array bounds
    if (ch < 0) ch = 0;
    else if (ch >= ONS_MIX_CHANNELS) ch = ONS_MIX_CHANNELS-1;
    int vol = script_h.readInt();

    if ( wave_sample[ch] ){
        Mix_Volume( ch, vol * 128 / 100 );
    }

    channelvolumes[ch] = vol;

    return RET_CONTINUE;
}

int ONScripterLabel::checkpageCommand()
{
    script_h.readVariable();
    script_h.pushVariable();

    if ( script_h.pushed_variable.type != ScriptHandler::VAR_INT &&
         script_h.pushed_variable.type != ScriptHandler::VAR_ARRAY )
        errorAndExit( "checkpage: no variable." );

    int page_no = script_h.readInt();

    Page *page = current_page;
    while(page != start_page && page_no > 0){
        page_no--;
        page = page->previous;
    }

    if (page_no > 0)
        script_h.setInt( &script_h.pushed_variable, 0 );
    else
        script_h.setInt( &script_h.pushed_variable, 1 );

    return RET_CONTINUE;

}

int ONScripterLabel::cellCommand()
{
    int sprite_no = script_h.readInt();
    int no        = script_h.readInt();

    sprite_info[sprite_no].setCell(no);
    dirty_rect.add( sprite_info[sprite_no].pos );

    return RET_CONTINUE;
}

int ONScripterLabel::captionCommand()
{
    const char* buf = script_h.readStr();
    size_t len = strlen(buf);

    char *buf2 = new char[len*2+3];
#if defined(MACOSX) && (SDL_COMPILEDVERSION >= 1208) /* convert sjis to utf-8 */
    char *buf1 = new char[len+1];
    strcpy(buf1, buf);
    DirectReader::convertFromSJISToUTF8(buf2, buf1, len);
    delete[] buf1;
#elif defined(LINUX) || (defined(WIN32) && defined(UTF8_CAPTION))
#if defined(UTF8_CAPTION)
    char *buf1 = new char[len+1];
    strcpy(buf1, buf);
    DirectReader::convertFromSJISToUTF8(buf2, buf1, len);
    delete[] buf1;
#else
    strcpy(buf2, buf);
    DirectReader::convertFromSJISToEUC(buf2);
#endif
#else
    strcpy(buf2, buf);
#endif

    setStr( &wm_title_string, buf2 );
    setStr( &wm_icon_string,  buf2 );
    delete[] buf2;

    SDL_WM_SetCaption( wm_title_string, wm_icon_string );

    return RET_CONTINUE;
}

int ONScripterLabel::btnwaitCommand()
{
    bool del_flag=false, textbtn_flag=false;

    if ( script_h.isName( "btnwait2" ) ){
        if (erase_text_window_mode > 0) display_mode = DISPLAY_MODE_NORMAL;
    }
    else if ( script_h.isName( "btnwait" ) ){
        del_flag = true;
        if (erase_text_window_mode > 0) display_mode = DISPLAY_MODE_NORMAL;
    }
    else if ( script_h.isName( "textbtnwait" ) ){
        textbtn_flag = true;
    }

    script_h.readInt();

    bool skip_flag = (skip_mode & SKIP_NORMAL) || ctrl_pressed_status || 
                     ((skip_mode & SKIP_TO_EOP) && 
                      ((clickstr_state == CLICK_WAIT) ||
                       (textgosub_label && ((textgosub_clickstr_state == CLICK_WAIT) ||
                                            (textgosub_clickstr_state == CLICK_WAITEOL)))));

    if ( event_mode & WAIT_BUTTON_MODE || ( skip_flag && textbtn_flag ) ) {

        btnwait_time = SDL_GetTicks() - internal_button_timer;
	// commenting out appears to fix btnwait bug
//        btntime_value = 0;
        num_chars_in_sentence = 0;

        if ( skip_flag && textbtn_flag )
            current_button_state.set(0);
        script_h.setInt( &script_h.current_variable, current_button_state.button );

        if ( current_button_state.button >= 1 && del_flag ){
	    // adding next appears to fix btnwait bug
	    btntime_value = 0;
	    deleteButtonLink();
            if ( exbtn_d_button_link.exbtn_ctl ){
                delete[] exbtn_d_button_link.exbtn_ctl;
                exbtn_d_button_link.exbtn_ctl = NULL;
            }
        }

        event_mode = IDLE_EVENT_MODE;
        disableGetButtonFlag();

        ButtonLink *p_button_link = root_button_link.next;
        while( p_button_link ){
            ButtonLink *cur_button_link = p_button_link;
            while (cur_button_link) {
                cur_button_link->show_flag = 0;
                cur_button_link = cur_button_link->same;
            }
            p_button_link = p_button_link->next;
        }

        return RET_CONTINUE;
    }
    else{
        shortcut_mouse_line = 0;
        skip_mode &= ~SKIP_NORMAL;

        if ( exbtn_d_button_link.exbtn_ctl ){
            SDL_Rect check_src_rect = {0, 0, screen_width, screen_height};
            if (is_exbtn_enabled)
                decodeExbtnControl( exbtn_d_button_link.exbtn_ctl, &check_src_rect );
        }

        if (txtbtn_show) txtbtn_visible = true;

        ButtonLink *p_button_link = root_button_link.next;
        while( p_button_link ){
            ButtonLink *cur_button_link = p_button_link;
            while (cur_button_link) {
                cur_button_link->show_flag = 0;
                if ( cur_button_link->button_type == ButtonLink::SPRITE_BUTTON ||
                     cur_button_link->button_type == ButtonLink::EX_SPRITE_BUTTON ){
                    sprite_info[ cur_button_link->sprite_no ].setCell(0);
                }
                else if ( cur_button_link->button_type == ButtonLink::TMP_SPRITE_BUTTON ){
                    cur_button_link->show_flag = 1;
                    sprite_info[ cur_button_link->sprite_no ].setCell(0);
                }
                else if ( cur_button_link->button_type == ButtonLink::TEXT_BUTTON ){
                    if (txtbtn_visible)
                        cur_button_link->show_flag = 1;
                        sprite_info[ cur_button_link->sprite_no ].setCell(0);
                }
                else if ( cur_button_link->anim[1] != NULL ){
                    cur_button_link->show_flag = 2;
                }
                dirty_rect.add( cur_button_link->image_rect );
                cur_button_link = cur_button_link->same;
            }
            p_button_link = p_button_link->next;
        }

        flush( refreshMode() );

        event_mode = WAIT_BUTTON_MODE;
        //refreshMouseOverButton();

        if ( btntime_value > 0 ){
            if ( btntime2_flag )
                event_mode |= WAIT_VOICE_MODE;
            startTimer( btntime_value );
            //if ( usewheel_flag ) current_button_state.button = -5;
            //else                 current_button_state.button = -2;
        }
        internal_button_timer = SDL_GetTicks();

        if ( textbtn_flag ){
            event_mode |= WAIT_TEXTBTN_MODE;
            if ( btntime_value == 0 ){
                if ( automode_flag ){
                    event_mode |= WAIT_VOICE_MODE;
                    if ( automode_time < 0 )
                        startTimer( -automode_time * num_chars_in_sentence / 2);
                    else
                        startTimer( automode_time );
                    //current_button_state.button = 0;
                }
		else if (autoclick_time > 0) {
		    startTimer(autoclick_time);
		}
            }
        }

        if ((event_mode & WAIT_TIMER_MODE) == 0){
            event_mode |= WAIT_TIMER_MODE;
            advancePhase();
        }
        refreshMouseOverButton();

        return RET_WAIT | RET_REREAD;
    }
}

int ONScripterLabel::btntimeCommand()
{
    if ( script_h.isName( "btntime2" ) )
        btntime2_flag = true;
    else
        btntime2_flag = false;
    btntime_value = script_h.readInt();

    return RET_CONTINUE;
}

int ONScripterLabel::btndownCommand()
{
    btndown_flag = (script_h.readInt()==1)?true:false;

    return RET_CONTINUE;
}

int ONScripterLabel::btndefCommand()
{
    if (script_h.compareString("clear")){
        script_h.readName();
    }
    else{
        const char *buf = script_h.readStr();

        btndef_info.remove();

        if ( buf[0] != '\0' ){
            btndef_info.setImageName( buf );
            parseTaggedString( &btndef_info );
            btndef_info.trans_mode = AnimationInfo::TRANS_COPY;
            setupAnimationInfo( &btndef_info );
#ifdef RCA_SCALE
            if (btndef_info.image_surface
                && (scr_stretch_x > 1.0 || scr_stretch_y > 1.0 )) {
                // Scale and reposition buttons if screen is bigger than game
                SDL_Surface* src = btndef_info.image_surface;
                SDL_PixelFormat *fmt = src->format;
                SDL_Surface* dst = SDL_CreateRGBSurface( SDL_SWSURFACE, 
                                                         scr_stretch_x*src->w, 
                                                         scr_stretch_y*src->h,
                                                         fmt->BitsPerPixel, fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask );
                resizeSurface( src, dst );
                btndef_info.image_surface = dst;
                btndef_info.pos.w *= scr_stretch_x;
                btndef_info.pos.h *= scr_stretch_y;
                SDL_FreeSurface( src );
            }
  
            if (btndef_info.image_surface)
#endif
            SDL_SetAlpha( btndef_info.image_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
        } else btntime_value = 0; //Mion - clear the btn wait time
    }

    deleteButtonLink();
    current_button_state.reset();
    processTextButtonInfo();

    disableGetButtonFlag();

    return RET_CONTINUE;
}

int ONScripterLabel::btnCommand()
{
    SDL_Rect src_rect;

    ButtonLink *button = new ButtonLink();

    button->no           = script_h.readInt();
#ifndef RCA_SCALE
    button->image_rect.x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    button->image_rect.y = script_h.readInt() * screen_ratio1 / screen_ratio2;
    button->image_rect.w = script_h.readInt() * screen_ratio1 / screen_ratio2;
    button->image_rect.h = script_h.readInt() * screen_ratio1 / screen_ratio2;
#else
    button->image_rect.x = script_h.readInt() * screen_ratio1 * scr_stretch_x / screen_ratio2;
    button->image_rect.y = script_h.readInt() * screen_ratio1 * scr_stretch_y / screen_ratio2;
    button->image_rect.w = script_h.readInt() * screen_ratio1 * scr_stretch_x / screen_ratio2;
    button->image_rect.h = script_h.readInt() * screen_ratio1 * scr_stretch_y / screen_ratio2;
#endif
    button->select_rect = button->image_rect;

#ifndef RCA_SCALE
    src_rect.x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    src_rect.y = script_h.readInt() * screen_ratio1 / screen_ratio2;
#else
    src_rect.x = script_h.readInt() * screen_ratio1 * scr_stretch_x / screen_ratio2;
    src_rect.y = script_h.readInt() * screen_ratio1 * scr_stretch_y / screen_ratio2;
#endif
    if (btndef_info.image_surface &&
        src_rect.x + button->image_rect.w > btndef_info.image_surface->w){
        button->image_rect.w = btndef_info.image_surface->w - src_rect.x;
    }
    if (btndef_info.image_surface &&
        src_rect.y + button->image_rect.h > btndef_info.image_surface->h){
        button->image_rect.h = btndef_info.image_surface->h - src_rect.y;
    }
    src_rect.w = button->image_rect.w;
    src_rect.h = button->image_rect.h;

    button->anim[0] = new AnimationInfo();
    button->anim[0]->num_of_cells = 1;
    button->anim[0]->trans_mode = AnimationInfo::TRANS_COPY;
    button->anim[0]->pos.x = button->image_rect.x;
    button->anim[0]->pos.y = button->image_rect.y;
    button->anim[0]->allocImage( button->image_rect.w, button->image_rect.h );
    button->anim[0]->copySurface( btndef_info.image_surface, &src_rect );

    root_button_link.insert( button );

    return RET_CONTINUE;
}

int ONScripterLabel::brCommand()
{
    int ret = enterTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    sentence_font.newLine();
    current_page->add( 0x0a );

    return RET_CONTINUE;
}

int ONScripterLabel::bltCommand()
{
    int dx,dy,dw,dh;
    int sx,sy,sw,sh;

    dx = script_h.readInt() * screen_ratio1 / screen_ratio2;
    dy = script_h.readInt() * screen_ratio1 / screen_ratio2;
    dw = script_h.readInt() * screen_ratio1 / screen_ratio2;
    dh = script_h.readInt() * screen_ratio1 / screen_ratio2;
    sx = script_h.readInt() * screen_ratio1 / screen_ratio2;
    sy = script_h.readInt() * screen_ratio1 / screen_ratio2;
    sw = script_h.readInt() * screen_ratio1 / screen_ratio2;
    sh = script_h.readInt() * screen_ratio1 / screen_ratio2;

    if (btndef_info.image_surface == NULL) return RET_CONTINUE;
    if (dw == 0 || dh == 0 || sw == 0 || sh == 0) return RET_CONTINUE;

    if ( sw == dw && sw > 0 && sh == dh && sh > 0 ){

        SDL_Rect src_rect = {sx,sy,sw,sh};
        SDL_Rect dst_rect = {dx,dy,dw,dh};

        SDL_BlitSurface( btndef_info.image_surface, &src_rect, screen_surface, &dst_rect );
        SDL_UpdateRect( screen_surface, dst_rect.x, dst_rect.y, dst_rect.w, dst_rect.h );
        dirty_rect.clear();
    }
    else{
        SDL_LockSurface(accumulation_surface);
        SDL_LockSurface(btndef_info.image_surface);
        ONSBuf *dst_buf = (ONSBuf*)accumulation_surface->pixels;
        ONSBuf *src_buf = (ONSBuf*)btndef_info.image_surface->pixels;
#ifdef BPP16
        int dst_width = accumulation_surface->pitch / 2;
        int src_width = btndef_info.image_surface->pitch / 2;
#else
        int dst_width = accumulation_surface->pitch / 4;
        int src_width = btndef_info.image_surface->pitch / 4;
#endif

        int start_y = dy, end_y = dy+dh;
        if (dh < 0){
            start_y = dy+dh;
            end_y = dy;
        }
        if (start_y < 0) start_y = 0;
        if (end_y > screen_height) end_y = screen_height;

        int start_x = dx, end_x = dx+dw;
        if (dw < 0){
            start_x = dx+dw;
            end_x = dx;
        }
        if (start_x < 0) start_x = 0;
        if (end_x >= screen_width) end_x = screen_width;

        dst_buf += start_y*dst_width;
        for (int i=start_y ; i<end_y ; i++){
            int y = sy+sh*(i-dy)/dh;
            for (int j=start_x ; j<end_x ; j++){

                int x = sx+sw*(j-dx)/dw;
                if (x<0 || x>=btndef_info.image_surface->w ||
                    y<0 || y>=btndef_info.image_surface->h)
                    *(dst_buf+j) = 0;
                else
                    *(dst_buf+j) = *(src_buf+y*src_width+x);
            }
            dst_buf += dst_width;
        }
        SDL_UnlockSurface(btndef_info.image_surface);
        SDL_UnlockSurface(accumulation_surface);

        SDL_Rect dst_rect = {start_x, start_y, end_x-start_x, end_y-start_y};
        flushDirect( (SDL_Rect&)dst_rect, REFRESH_NONE_MODE );
    }

    return RET_CONTINUE;
}

int ONScripterLabel::bgcopyCommand()
{
    SDL_BlitSurface( screen_surface, NULL, accumulation_surface, NULL );

    bg_info.num_of_cells = 1;
    bg_info.trans_mode = AnimationInfo::TRANS_COPY;
    bg_info.pos.x = 0;
    bg_info.pos.y = 0;
    bg_info.copySurface( accumulation_surface, NULL );

    return RET_CONTINUE;
}

int ONScripterLabel::bgCommand()
{
    int ret = leaveTextDisplayMode(true);
    if ( ret != RET_NOMATCH ) return ret;

    const char *buf;
    if (script_h.compareString("white")){
        buf = "white";
        script_h.readName();
    }
    else if (script_h.compareString("black")){
        buf = "black";
        script_h.readName();
    }
    else{
        buf = script_h.readStr();
        setStr( &bg_info.file_name, buf );
    }

    if ( event_mode & EFFECT_EVENT_MODE ){
        return doEffect( parseEffect(false) );
    }
    else{
        for ( int i=0 ; i<3 ; i++ )
            tachi_info[i].remove();

        bg_info.remove();
        setStr( &bg_info.file_name, buf );

        createBackground();
        dirty_rect.fill( screen_width, screen_height );

        return setEffect( parseEffect(true), true, true );
    }
}

int ONScripterLabel::barclearCommand()
{
    for ( int i=0 ; i<MAX_PARAM_NUM ; i++ ) {
        if ( bar_info[i] ) {
            dirty_rect.add( bar_info[i]->pos );
            delete bar_info[i];
            bar_info[i] = NULL;
        }
    }
    return RET_CONTINUE;
}

int ONScripterLabel::barCommand()
{
    int no = script_h.readInt();
    if ( bar_info[no] ){
        dirty_rect.add( bar_info[no]->pos );
        bar_info[no]->remove();
    }
    else{
        bar_info[no] = new AnimationInfo();
    }
    bar_info[no]->trans_mode = AnimationInfo::TRANS_COPY;
    bar_info[no]->num_of_cells = 1;
    bar_info[no]->setCell(0);

    bar_info[no]->param = script_h.readInt();
    bar_info[no]->pos.x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    bar_info[no]->pos.y = script_h.readInt() * screen_ratio1 / screen_ratio2;

    bar_info[no]->max_width = script_h.readInt() * screen_ratio1 / screen_ratio2;
    bar_info[no]->pos.h = script_h.readInt() * screen_ratio1 / screen_ratio2;
    bar_info[no]->max_param = script_h.readInt();

    const char *buf = script_h.readStr();
    readColor( &bar_info[no]->color, buf );

    int w = bar_info[no]->max_width * bar_info[no]->param / bar_info[no]->max_param;
    if ( bar_info[no]->max_width > 0 && w > 0 ){
        bar_info[no]->pos.w = w;
        bar_info[no]->allocImage( bar_info[no]->pos.w, bar_info[no]->pos.h );
        bar_info[no]->fill( bar_info[no]->color[0], bar_info[no]->color[1], bar_info[no]->color[2], 0xff );
        dirty_rect.add( bar_info[no]->pos );
    }

    return RET_CONTINUE;
}

int ONScripterLabel::aviCommand()
{
    script_h.readStr();
    const char *save_buf = script_h.saveStringBuffer();

    bool click_flag = (script_h.readInt()==1)?true:false;

    stopBGM( false );
    playAVI( save_buf, click_flag );

    return RET_CONTINUE;
}

int ONScripterLabel::automode_timeCommand()
{
    automode_time = script_h.readInt();

    return RET_CONTINUE;
}

int ONScripterLabel::autoclickCommand()
{
    autoclick_time = script_h.readInt();

    return RET_CONTINUE;
}

int ONScripterLabel::amspCommand()
{
    int ret = leaveTextDisplayMode();
    if ( ret != RET_NOMATCH ) return ret;

    bool amsp2_flag = false;
    if (script_h.isName("amsp2")) amsp2_flag = true;

    int no = script_h.readInt();
    AnimationInfo *si=NULL;
    if (amsp2_flag){
        si = &sprite2_info[no];
        dirty_rect.add( si->bounding_rect );
    }
    else{
        si = &sprite_info[no];
        dirty_rect.add( si->pos );
    }

#ifndef RCA_SCALE
    si->pos.x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    si->pos.y = script_h.readInt() * screen_ratio1 / screen_ratio2;
#else
    si->pos.x = script_h.readInt() * screen_ratio1 * scr_stretch_x / screen_ratio2;
    si->pos.y = script_h.readInt() * screen_ratio1 * scr_stretch_y / screen_ratio2;
#endif
    if (amsp2_flag){
        si->scale_x = script_h.readInt();
        si->scale_y = script_h.readInt();
        si->rot = script_h.readInt();
        si->calcAffineMatrix();
        dirty_rect.add( si->bounding_rect );
    }
    else{
        dirty_rect.add( si->pos );
    }

    if ( script_h.getEndStatus() & ScriptHandler::END_COMMA )
        si->trans = script_h.readInt();

    if ( si->trans > 256 ) si->trans = 256;
    else if ( si->trans < 0 ) si->trans = 0;

    return RET_CONTINUE;
}

int ONScripterLabel::allsp2resumeCommand()
{
    all_sprite2_hide_flag = false;
    for ( int i=0 ; i<MAX_SPRITE2_NUM ; i++ ){
        if ( sprite2_info[i].visible )
            dirty_rect.add( sprite2_info[i].bounding_rect );
    }
    return RET_CONTINUE;
}

int ONScripterLabel::allspresumeCommand()
{
    all_sprite_hide_flag = false;
    for ( int i=0 ; i<MAX_SPRITE_NUM ; i++ ){
        if ( sprite_info[i].visible )
            dirty_rect.add( sprite_info[i].pos );
    }
    return RET_CONTINUE;
}

int ONScripterLabel::allsp2hideCommand()
{
    all_sprite2_hide_flag = true;
    for ( int i=0 ; i<MAX_SPRITE2_NUM ; i++ ){
        if ( sprite2_info[i].visible )
            dirty_rect.add( sprite2_info[i].bounding_rect );
    }
    return RET_CONTINUE;
}

int ONScripterLabel::allsphideCommand()
{
    all_sprite_hide_flag = true;
    for ( int i=0 ; i<MAX_SPRITE_NUM ; i++ ){
        if ( sprite_info[i].visible )
            dirty_rect.add( sprite_info[i].pos );
    }
    return RET_CONTINUE;
}

// Haeleth: Stub out some commands to suppress unwanted debug messages

int ONScripterLabel::insertmenuCommand()
{
    script_h.skipToken();
    return RET_CONTINUE;
}
int ONScripterLabel::resetmenuCommand()
{
    script_h.skipToken();
    return RET_CONTINUE;
}
