/* -*- C++ -*-
 * 
 *  text_menu.cpp -- display list of games to run on WII
 *
 *  Copyright (c) 2009 Brian. All rights reserved.
 *
 *  brijohn@gmail.com
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

#include <gccore.h>
#include <wiiuse/wpad.h>

#include <SDL_console.h>

#include "setting_parser.h"
#include "ONScripterLabel.h"

void clr_screen()
{
	SDL_ClearConsole();
	fprintf(stderr, "\x1b[u");
}

void display_game_list(xml_settings_t *games, int current)
{
	mxml_node_t *game;
	int index = 0;
	fprintf(stderr, "\x1b[u\n\n");
	for(game = find_first_game(games); game != NULL; game = find_next_game(games)) {
		fprintf(stderr, "    %d) %s - %s %c\n", index + 1, get_id(game), get_title(game), (index == current) ? '*' : ' ');
		index++;
	}
	fprintf(stderr, "   Select game or press HOME to quite\n");
}


int handle_game_selection(ONScripterLabel *ons)
{
	int index = 0;
	xml_settings_t *settings = open_settings();
	mxml_node_t *game;
	const char *root, *save, *font, *registry;
	if (settings == NULL) {
		ons->setArchivePath("/apps/onscripter");
		ons->setSavePath("/apps/onscripter");
		SDL_ClearConsole();
		return 0;
	}
	clr_screen();
	while(1) {
		display_game_list(settings, index);
		WPAD_ScanPads();
		short pressed = WPAD_ButtonsDown(0);
		if (pressed & WPAD_BUTTON_HOME) {
			close_settings(settings);
			return -1;
		} else if (pressed & WPAD_BUTTON_DOWN) {
			if(index < get_game_count(settings) - 1) index++;
		} else if (pressed & WPAD_BUTTON_UP) {
			if(index) index--;
		} else if (pressed & WPAD_BUTTON_A) {
			break;
		}
	}
	game = get_game_by_index(settings, index);
	root = get_root_path(game);
	save = get_save_path(game);
	font = get_font(game);
	registry = get_registry(game);
	
	if (root)
		ons->setArchivePath(root);
	else
		ons->setArchivePath("/apps/onscripter");

	if (save)
		ons->setArchivePath(save);
	else
		ons->setArchivePath("/apps/onscripter");

	if (font)
		ons->setFontFile(font);

	if (registry)
		ons->setRegistryFile(registry);

	if (use_english_mode(game))
		ons->setEnglishMode();

	close_settings(settings);
	SDL_ClearConsole();

	return 0;
}
