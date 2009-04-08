/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * menu.h
 * Menu flow routines - handles all menu logic
 ***************************************************************************/

#ifndef _MENU_H_
#define _MENU_H_

#include <ogcsys.h>
#include "ONScripterLabel.h"

int MainMenu(ONScripterLabel *ons);

enum
{
	JAPANESE,
	ENGLISH,
};

enum
{
	MENU_EXIT = -1,
	MENU_NONE,
	MENU_GAME_SEL,
	MENU_GAME_ADD,
	MENU_GAME_LAUNCH,
};

#endif
