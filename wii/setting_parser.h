/* -*- C++ -*-
 * 
 *  setting_parser.h -- parses list of installed games on WII
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

#ifndef _SETTING_PARSER_H
#define _SETTING_PARSER_H

#include <mxml.h>

typedef struct xml_settings_s xml_settings_t;

xml_settings_t *open_settings();
void close_settings(xml_settings_t *s);
int save_settings(xml_settings_t *s);

void add_game(xml_settings_t *s, char *id, char *title, int mode, char *root, char *save, char *font);
void delete_game(xml_settings_t *s, mxml_node_t *game);

mxml_node_t *find_first_game(xml_settings_t *s);
mxml_node_t *find_next_game(xml_settings_t *s);
mxml_node_t *get_game_by_index(xml_settings_t *s, int index);
int get_game_count(xml_settings_t *s);
const char *get_title(mxml_node_t *game);
const char *get_id(mxml_node_t *game);
const char *get_root_path(mxml_node_t *game);
const char *get_save_path(mxml_node_t *game);
const char *get_font(mxml_node_t *game);
const char *get_registry(mxml_node_t *game);
int use_english_mode(mxml_node_t *game);

#endif
