/* -*- C++ -*-
 * 
 *  setting_parser.cpp -- parses list of installed games on WII
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

#include "setting_parser.h"

struct xml_settings_s {
	mxml_node_t *root;
	mxml_index_t *index;
};

xml_settings_t *open_settings()
{
	FILE *fd;
	xml_settings_t *s;
	s = (struct xml_settings_s *)malloc(sizeof(struct xml_settings_s));
	if (s == NULL)
		return NULL;

	memset(s, 0, sizeof(struct xml_settings_s));

	fd = fopen("/apps/onscripter/games.xml", "r");
	if (fd == NULL) {
		printf("Error : %d\n", errno);
		return NULL;
	}

	s->root = mxmlLoadFile(NULL, fd, MXML_TEXT_CALLBACK);
	s->index = mxmlIndexNew(s->root, "game", "id");

	fclose(fd);

	return s;
}

int save_settings(xml_settings_t *s)
{
	FILE *fd;
	int ret = -1;

	fd = fopen("/apps/onscripter/games.xml", "w");
	if (fd == NULL) {
		printf("Error : %d\n", errno);
		return ret;
	}

	ret = mxmlSaveFile(s->root, fd, MXML_NO_CALLBACK);
	fclose(fd);
	return ret;
}

void add_game(xml_settings_t *s, char* id, char* title, int mode, char *root, char *save, char *font)
{
	mxml_node_t *pRoot, *pSave, *pFont;
	mxml_node_t *game = mxmlNewElement(NULL, "game");
	mxmlElementSetAttr(game, "id", id);
	mxmlElementSetAttr(game, "title", title);
	if (mode) {
		mxmlNewElement(game, "english");
	}
	if (strcmp(root, "")) {
		pRoot = mxmlNewElement(game, "root");
		mxmlElementSetAttr(pRoot, "path", root);
	}
	if (strcmp(save, "")) {
		pSave = mxmlNewElement(game, "save");
		mxmlElementSetAttr(pSave, "path", save);
	}
	if (strcmp(font, "")) {
		pFont = mxmlNewElement(game, "font");
		mxmlElementSetAttr(pFont, "file", font);
	}
	mxmlAdd(s->root, MXML_ADD_AFTER, NULL, game);
	mxmlIndexDelete(s->index);
	s->index = mxmlIndexNew(s->root, "game", "id");
}

void delete_game(xml_settings_t *s, mxml_node_t *game)
{
	mxmlDelete(game);
	mxmlIndexDelete(s->index);
	s->index = mxmlIndexNew(s->root, "game", "id");
}

mxml_node_t *find_first_game(xml_settings_t *s)
{
	mxmlIndexReset(s->index);
	return mxmlIndexEnum(s->index);
}

mxml_node_t *find_next_game(xml_settings_t *s)
{
	return mxmlIndexEnum(s->index);
}

mxml_node_t *get_game_by_index(xml_settings_t *s, int index)
{
	if (index < 0 || index > (s->index->num_nodes - 1))
		return NULL;
	return s->index->nodes[index];
}

int get_game_count(xml_settings_t *s)
{
	return s->index->num_nodes;
}

const char *get_title(mxml_node_t *game)
{
	return mxmlElementGetAttr(game, "title");
}

const char *get_id(mxml_node_t *game)
{
	return mxmlElementGetAttr(game, "id");
}

const char *get_root_path(mxml_node_t *game)
{
	mxml_node_t *node;
	node = mxmlFindElement(game, game, "root", "path", NULL, MXML_DESCEND_FIRST);
	if (node == NULL)
		return NULL;

	return mxmlElementGetAttr(node, "path");
}

const char *get_save_path(mxml_node_t *game)
{
	mxml_node_t *node;
	node = mxmlFindElement(game, game, "save", "path", NULL, MXML_DESCEND_FIRST);
	if (node == NULL)
		return NULL;

	return mxmlElementGetAttr(node, "path");
}

const char *get_font(mxml_node_t *game)
{
	mxml_node_t *node;
	node = mxmlFindElement(game, game, "font", "file", NULL, MXML_DESCEND_FIRST);
	if (node == NULL)
		return NULL;

	return mxmlElementGetAttr(node, "file");
}

const char *get_registry(mxml_node_t *game)
{
	mxml_node_t *node;
	node = mxmlFindElement(game, game, "registry", "file", NULL, MXML_DESCEND_FIRST);
	if (node == NULL)
		return NULL;

	return mxmlElementGetAttr(node, "file");
}

int use_english_mode(mxml_node_t *game)
{
	if(mxmlFindElement(game, game, "english", NULL, NULL, MXML_DESCEND_FIRST))
		return 1;
	return 0;
}

int use_only_ogg(mxml_node_t *game)
{
	if(mxmlFindElement(game, game, "ogg_only", NULL, NULL, MXML_DESCEND_FIRST))
		return 1;
	return 0;
}

void close_settings(xml_settings_t *s)
{
	mxmlIndexDelete(s->index);
	mxmlDelete(s->root);
	free(s);
}


