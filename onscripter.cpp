/* -*- C++ -*-
 * 
 *  onscripter.cpp -- main function of ONScripter
 *
 *  Copyright (c) 2001-2007 Ogapee. All rights reserved.
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

#include "ONScripterLabel.h"
#include "version.h"

static void optionHelp()
{
    printf( "Usage: onscripter [option ...]\n" );
    printf( "      --cdaudio\t\tuse CD audio if available\n");
    printf( "      --cdnumber no\tchoose the CD-ROM drive number\n");
    printf( "  -f, --font file\tset a TTF font file\n");
    printf( "      --registry file\tset a registry file\n");
    printf( "      --dll file\tset a dll file\n");
#ifdef ENABLE_1BYTE_CHAR
    printf( "      --english\tforce English text mode\n");
#endif
#if   defined WIN32
    printf( "  -r, --root path\tset the root path to the archives\n");
    printf( "  -s, --save path\tset the path to use for saved games (default: folder in All Users profile)\n");
#elif defined MACOSX
    printf( "  -r, --root path\tset the root path to the archives (default: Resources in ONScripter bundle)\n");
    printf( "  -s, --save path\tset the path to use for saved games (default: folder in ~/Library/Preferences)\n");
#elif defined LINUX
    printf( "  -r, --root path\tset the root path to the archives\n");
    printf( "  -s, --save path\tset the path to use for saved games (default: hidden subdirectory in ~)\n");
#else
    printf( "  -r, --root path\tset the root path to the archives\n");
    printf( "  -s, --save path\tset the path to use for saved games (default: same as root path)\n");
#endif
    printf( "      --fullscreen\tstart in fullscreen mode\n");
    printf( "      --window\t\tstart in window mode\n");
#ifdef RCA_SCALE
    printf( "      --widescreen\ttransform game to match widescreen monitors\n");
    printf( "      --scale\t\tscale game to native display size. Yields small sharp text.\n");
#endif
    printf( "      --force-png-alpha\t\talways use PNG alpha channels\n");
    printf( "      --force-png-nscmask\talways use NScripter-style masks\n");
    printf( "      --force-button-shortcut\tignore useescspc and getenter command\n");
    printf( "      --enable-wheeldown-advance\tadvance the text on mouse wheeldown event\n");
    printf( "      --disable-rescale\tdo not rescale the images in the archives when compiled with -DPDA\n");
    printf( "      --edit\t\tenable editing the volumes and the variables when 'z' is pressed\n");
    printf( "      --key-exe file\tset a file (*.EXE) that includes a key table\n");
    printf( "  -h, --help\t\tshow this help and exit\n");
    printf( "  -v, --version\t\tshow the version information and exit\n");
    exit(0);
}

static void optionVersion()
{
    printf("ONScripter version %s (%d.%02d)\n", ONS_VERSION, NSC_VERSION/100, NSC_VERSION%100 );
    printf("Written by Ogapee <ogapee@aqua.dti2.ne.jp>\n\n");
    printf("Copyright (c) 2001-2006 Ogapee.\n");
    printf("This is free software; see the source for copying conditions.\n");
    exit(0);
}
#ifdef QWS
int SDL_main( int argc, char **argv )
#elif defined(PSP)
extern "C" int main( int argc, char **argv )
#else
int main( int argc, char **argv )
#endif
{
    ONScripterLabel ons;

#ifdef PSP
    ons.disableRescale();
    ons.enableButtonShortCut();
#endif

#ifdef ENABLE_1BYTE_CHAR
    // Check filename: we want binaries named "onscripter-en" to
    // default to English mode.  We actually match the regex
    // /(^|[/\\])onscripter[^a-z]en/i, which should allow all likely
    // variants without being terribly likely to produce unexpected
    // results.
    //
    // The code is ugly because Windows, unlike other mainstream
    // platforms fails to provide basic regex support in its libc; I'm
    // loathe to add an entire third-party library just for a single
    // test, so we do this the hard way.  Thanks, Mr Gates.
    {
	char fname[256];
	strncpy(fname, argv[0], 256);
	char* it = fname + strlen(fname);
	if (it > fname + 255) it = fname + 255;
	while (it >= fname && *it != '/' && *it != '\\') --it;
	++it;
	int len = strlen(it);
	if (len >= 13) {
	    it[13] = 0;
	    for (int i = 0; i < 13; ++i) it[i] = tolower(it[i]);
	    if (it[10] < 'a' || it[10] > 'z') {
		it[10] = '-';
		if (strcmp(it, "onscripter-en") == 0) {
		    printf("Setting English mode based on filename\n");
		    ons.setEnglishMode();
		}
	    }
	}
    }
#endif

    // ----------------------------------------
    // Parse options
    bool hasArchivePath = false;
    argv++;
    while( argc > 1 ){
        if ( argv[0][0] == '-' ){
            if ( !strcmp( argv[0]+1, "h" ) || !strcmp( argv[0]+1, "-help" ) ){
                optionHelp();
            }
            else if ( !strcmp( argv[0]+1, "v" ) || !strcmp( argv[0]+1, "-version" ) ){
                optionVersion();
            }
            else if ( !strcmp( argv[0]+1, "-cdaudio" ) ){
                ons.enableCDAudio();
            }
            else if ( !strcmp( argv[0]+1, "-cdnumber" ) ){
                argc--;
                argv++;
                ons.setCDNumber(atoi(argv[0]));
            }
            else if ( !strcmp( argv[0]+1, "f" ) || !strcmp( argv[0]+1, "-font" ) ){
                argc--;
                argv++;
                ons.setFontFile(argv[0]);
            }
            else if ( !strcmp( argv[0]+1, "-registry" ) ){
                argc--;
                argv++;
                ons.setRegistryFile(argv[0]);
            }
            else if ( !strcmp( argv[0]+1, "-dll" ) ){
                argc--;
                argv++;
                ons.setDLLFile(argv[0]);
            }
#ifdef ENABLE_1BYTE_CHAR
            else if ( !strcmp( argv[0]+1, "-english" ) ){
                ons.setEnglishMode();
            }
#endif
            else if ( !strcmp( argv[0]+1, "r" ) || !strcmp( argv[0]+1, "-root" ) ){
		hasArchivePath = true;
		argc--;
                argv++;
                ons.setArchivePath(argv[0]);
            }
            else if ( !strcmp( argv[0]+1, "s" ) || !strcmp( argv[0]+1, "-save" ) ){
                argc--;
                argv++;
                ons.setSavePath(argv[0]);
            }
            else if ( !strcmp( argv[0]+1, "-fullscreen" ) ){
                ons.setFullscreenMode();
            }
            else if ( !strcmp( argv[0]+1, "-window" ) ){
                ons.setWindowMode();
            }
            else if ( !strcmp( argv[0]+1, "-force-button-shortcut" ) ){
                ons.enableButtonShortCut();
            }
            else if ( !strcmp( argv[0]+1, "-enable-wheeldown-advance" ) ){
                ons.enableWheelDownAdvance();
            }
            else if ( !strcmp( argv[0]+1, "-disable-rescale" ) ){
                ons.disableRescale();
            }
            else if ( !strcmp( argv[0]+1, "-edit" ) ){
                ons.enableEdit();
            }
            else if ( !strcmp( argv[0]+1, "-key-exe" ) ){
                argc--;
                argv++;
                ons.setKeyEXE(argv[0]);
            }
#ifdef RCA_SCALE
            else if ( !strcmp( argv[0]+1, "-widescreen" ) ){
                ons.setWidescreen();
            }
            else if ( !strcmp( argv[0]+1, "-scale" ) ){
                ons.setScaled();
            }
#endif
	    else if ( !strcmp( argv[0]+1, "-force-png-alpha" ) ){
		ons.setMaskType( 1 );
	    }
	    else if ( !strcmp( argv[0]+1, "-force-png-nscmask" ) ){
		ons.setMaskType( 2 );
	    }
            else{
                printf(" unknown option %s\n", argv[0] );
            }
        }
        else if (!hasArchivePath) {
	    hasArchivePath = true;
            ons.setArchivePath(argv[0]);
            argc--;
            argv++;
        }
        else{
            optionHelp();
        }
        argc--;
        argv++;
    }
    
    // ----------------------------------------
    // Run ONScripter

    if (ons.init()) exit(-1);
    ons.eventLoop();
    
    exit(0);
}
