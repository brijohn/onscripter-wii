/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 * Brian Johnson 2009
 *
 * menu.cpp
 * Menu flow routines - handles all menu logic
 ***************************************************************************/

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <wiiuse/wpad.h>

#include "libwiigui/gui.h"
#include "audio.h"
#include "video.h"
#include "menu.h"
#include "input.h"
#include "filelist.h"
#include "setting_parser.h"
#include "ONScripterLabel.h"

static GuiImageData * pointer[4];
static GuiImage * bgImg = NULL;
static GuiSound * bgMusic = NULL;
static GuiButton * exitBtn = NULL;
static GuiWindow * mainWindow = NULL;
static lwp_t guithread = LWP_THREAD_NULL;
static bool guiHalt = true;

FreeTypeGX *fontSystem;

/****************************************************************************
 * ResumeGui
 *
 * Signals the GUI thread to start, and resumes the thread. This is called
 * after finishing the removal/insertion of new elements, and after initial
 * GUI setup.
 ***************************************************************************/
static void
ResumeGui()
{
	guiHalt = false;
	LWP_ResumeThread (guithread);
}

/****************************************************************************
 * HaltGui
 *
 * Signals the GUI thread to stop, and waits for GUI thread to stop
 * This is necessary whenever removing/inserting new elements into the GUI.
 * This eliminates the possibility that the GUI is in the middle of accessing
 * an element that is being changed.
 ***************************************************************************/
static void
HaltGui()
{
	guiHalt = true;

	// wait for thread to finish
	while(!LWP_ThreadIsSuspended(guithread))
		usleep(50);
}

/****************************************************************************
 * WindowPrompt
 *
 * Displays a prompt window to user, with information, an error message, or
 * presenting a user with a choice
 ***************************************************************************/
int
WindowPrompt(const char *title, const char *msg, const char *btn1Label, const char *btn2Label)
{
	int choice = -1;

	GuiWindow promptWindow(448,288);
	promptWindow.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	promptWindow.SetPosition(0, -10);
	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);

	GuiImageData dialogBox(dialogue_box_png);
	GuiImage dialogBoxImg(&dialogBox);

	GuiText titleTxt(title, 26, (GXColor){0, 0, 0, 255});
	titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	titleTxt.SetPosition(0,40);
	GuiText msgTxt(msg, 22, (GXColor){0, 0, 0, 255});
	msgTxt.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	msgTxt.SetPosition(0,-20);
	msgTxt.SetMaxWidth(430);

	GuiText btn1Txt(btn1Label, 22, (GXColor){0, 0, 0, 255});
	GuiImage btn1Img(&btnOutline);
	GuiImage btn1ImgOver(&btnOutlineOver);
	GuiButton btn1(btnOutline.GetWidth(), btnOutline.GetHeight());

	if(btn2Label)
	{
		btn1.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
		btn1.SetPosition(20, -25);
	}
	else
	{
		btn1.SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
		btn1.SetPosition(0, -25);
	}

	btn1.SetLabel(&btn1Txt);
	btn1.SetImage(&btn1Img);
	btn1.SetImageOver(&btn1ImgOver);
	btn1.SetSoundOver(&btnSoundOver);
	btn1.SetTrigger(&trigA);
	btn1.SetState(STATE_SELECTED);
	btn1.SetEffectGrow();

	GuiText btn2Txt(btn2Label, 22, (GXColor){0, 0, 0, 255});
	GuiImage btn2Img(&btnOutline);
	GuiImage btn2ImgOver(&btnOutlineOver);
	GuiButton btn2(btnOutline.GetWidth(), btnOutline.GetHeight());
	btn2.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
	btn2.SetPosition(-20, -25);
	btn2.SetLabel(&btn2Txt);
	btn2.SetImage(&btn2Img);
	btn2.SetImageOver(&btn2ImgOver);
	btn2.SetSoundOver(&btnSoundOver);
	btn2.SetTrigger(&trigA);
	btn2.SetEffectGrow();

	promptWindow.Append(&dialogBoxImg);
	promptWindow.Append(&titleTxt);
	promptWindow.Append(&msgTxt);
	promptWindow.Append(&btn1);

	if(btn2Label)
		promptWindow.Append(&btn2);

	promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_IN, 50);
	HaltGui();
	mainWindow->SetState(STATE_DISABLED);
	mainWindow->Append(&promptWindow);
	mainWindow->ChangeFocus(&promptWindow);
	ResumeGui();

	while(choice == -1)
	{
		VIDEO_WaitVSync();

		if(btn1.GetState() == STATE_CLICKED)
			choice = 1;
		else if(btn2.GetState() == STATE_CLICKED)
			choice = 0;
	}

	promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_OUT, 50);
	while(promptWindow.GetEffect() > 0) usleep(50);
	HaltGui();
	mainWindow->Remove(&promptWindow);
	mainWindow->SetState(STATE_DEFAULT);
	ResumeGui();
	return choice;
}

/****************************************************************************
 * UpdateGUI
 *
 * Primary thread to allow GUI to respond to state changes, and draws GUI
 ***************************************************************************/

static void *
UpdateGUI (void *arg)
{
	while(1)
	{
		if(guiHalt)
		{
			LWP_SuspendThread(guithread);
		}
		else
		{
			mainWindow->Draw();

			#ifdef HW_RVL
			for(int i=3; i >= 0; i--) // so that player 1's cursor appears on top!
			{
				if(userInput[i].wpad.ir.valid)
					Menu_DrawImg(userInput[i].wpad.ir.x-48, userInput[i].wpad.ir.y-48,
						96, 96, pointer[i]->GetImage(), userInput[i].wpad.ir.angle, 1, 1, 255);
				DoRumble(i);
			}
			#endif

			Menu_Render();

			for(int i=0; i < 4; i++)
				mainWindow->Update(&userInput[i]);

	/*		if(ExitRequested)
			{
				for(int a = 0; a < 255; a += 15)
				{
					mainWindow->Draw();
					Menu_DrawRectangle(0,0,screenwidth,screenheight,(GXColor){0, 0, 0, a},1);
					Menu_Render();
				}
			}*/
		}
	}
	return NULL;
}

/****************************************************************************
 * InitGUIThread
 *
 * Startup GUI threads
 ***************************************************************************/
void
InitGUIThreads()
{
	LWP_CreateThread (&guithread, UpdateGUI, NULL, NULL, 0, 70);
}

/****************************************************************************
 * OnScreenKeyboard
 *
 * Opens an on-screen keyboard window, with the data entered being stored
 * into the specified variable.
 ***************************************************************************/
static void OnScreenKeyboard(char * var, u16 maxlen)
{
	int save = -1;

	GuiKeyboard keyboard(var, maxlen);

	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);

	GuiText okBtnTxt("OK", 22, (GXColor){0, 0, 0, 255});
	GuiImage okBtnImg(&btnOutline);
	GuiImage okBtnImgOver(&btnOutlineOver);
	GuiButton okBtn(btnOutline.GetWidth(), btnOutline.GetHeight());

	okBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	okBtn.SetPosition(25, -25);

	okBtn.SetLabel(&okBtnTxt);
	okBtn.SetImage(&okBtnImg);
	okBtn.SetImageOver(&okBtnImgOver);
	okBtn.SetSoundOver(&btnSoundOver);
	okBtn.SetTrigger(&trigA);
	okBtn.SetEffectGrow();

	GuiText cancelBtnTxt("Cancel", 22, (GXColor){0, 0, 0, 255});
	GuiImage cancelBtnImg(&btnOutline);
	GuiImage cancelBtnImgOver(&btnOutlineOver);
	GuiButton cancelBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
	cancelBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
	cancelBtn.SetPosition(-25, -25);
	cancelBtn.SetLabel(&cancelBtnTxt);
	cancelBtn.SetImage(&cancelBtnImg);
	cancelBtn.SetImageOver(&cancelBtnImgOver);
	cancelBtn.SetSoundOver(&btnSoundOver);
	cancelBtn.SetTrigger(&trigA);
	cancelBtn.SetEffectGrow();

	keyboard.Append(&okBtn);
	keyboard.Append(&cancelBtn);

	HaltGui();
	mainWindow->SetState(STATE_DISABLED);
	mainWindow->Append(&keyboard);
	mainWindow->ChangeFocus(&keyboard);
	ResumeGui();

	while(save == -1)
	{
		VIDEO_WaitVSync();

		if(okBtn.GetState() == STATE_CLICKED)
			save = 1;
		else if(cancelBtn.GetState() == STATE_CLICKED)
			save = 0;
	}

	if(save)
	{
		snprintf(var, maxlen, "%s", keyboard.kbtextstr);
	}

	HaltGui();
	mainWindow->Remove(&keyboard);
	mainWindow->SetState(STATE_DEFAULT);
	ResumeGui();
}

/****************************************************************************
 * MenuGameSel
 ***************************************************************************/

static int MenuGameSel(ONScripterLabel *ons, xml_settings_t *settings)
{
	int menu = MENU_NONE;
	int choice;
	mxml_node_t *game;
	char msg[100];
	const char *root, *save, *font, *registry;

	GuiText titleTxt("Select game", 28, (GXColor){255, 255, 255, 255});
	titleTxt.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	titleTxt.SetPosition(70,75);

	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);

	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);

	GuiText delBtnTxt("Delete", 22, (GXColor){0, 0, 0, 255});
	GuiImage delBtnImg(&btnOutline);
	GuiImage delBtnImgOver(&btnOutlineOver);
	GuiButton delBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
	delBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	delBtn.SetPosition(100, -35);
	delBtn.SetLabel(&delBtnTxt);
	delBtn.SetImage(&delBtnImg);
	delBtn.SetImageOver(&delBtnImgOver);
	delBtn.SetSoundOver(&btnSoundOver);
	delBtn.SetTrigger(&trigA);
	delBtn.SetEffectGrow();

	GuiText addBtnTxt("Add", 22, (GXColor){0, 0, 0, 255});
	GuiImage addBtnImg(&btnOutline);
	GuiImage addBtnImgOver(&btnOutlineOver);
	GuiButton addBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
	addBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	addBtn.SetPosition(325, -35);
	addBtn.SetLabel(&addBtnTxt);
	addBtn.SetImage(&addBtnImg);
	addBtn.SetImageOver(&addBtnImgOver);
	addBtn.SetSoundOver(&btnSoundOver);
	addBtn.SetTrigger(&trigA);
	addBtn.SetEffectGrow();

	GuiListBox listBox(552, 248);
	for(game = find_first_game(settings); game != NULL; game = find_next_game(settings)) {
		listBox.AddItem(get_title(game));
	}
	listBox.SetPosition(50, 108);

	HaltGui();
	GuiWindow w(screenwidth, screenheight);
	w.Append(&delBtn);
	w.Append(&addBtn);
	mainWindow->Append(&listBox);
	mainWindow->Append(&w);
	mainWindow->Append(&titleTxt);
	ResumeGui();

	while(menu == MENU_NONE)
	{
		VIDEO_WaitVSync ();
		if (listBox.GetState() == STATE_CLICKED) {
			game = get_game_by_index(settings, listBox.GetSelectedItem());
			root = get_root_path(game);
			save = get_save_path(game);
			font = get_font(game);
			registry = get_registry(game);

			if (root)
				ons->setArchivePath(root);
			else
				ons->setArchivePath("/apps/onscripter");

			if (save)
				ons->setSavePath(save);

			if (font)
				ons->setFontFile(font);

			if (registry)
				ons->setRegistryFile(registry);

			if (use_english_mode(game))
				ons->setEnglishMode();

			if (use_only_ogg(game))
				ons->supportOggOnly();

			menu = MENU_GAME_LAUNCH;
		}

		if (addBtn.GetState() == STATE_CLICKED) {
			menu = MENU_GAME_ADD;
		}

		if (delBtn.GetState() == STATE_CLICKED) {
			game = get_game_by_index(settings, listBox.GetSelectedItem());
			snprintf(msg, 100, "Delete '%s'?", get_title(game));
			choice = WindowPrompt("Confirm", msg, "Yes", "No");
			if (choice == 1) {
				delete_game(settings, game);
				save_settings(settings);
				menu = MENU_GAME_SEL;
			} else {
				menu = MENU_NONE;
			}
		}

		if (exitBtn->GetState() == STATE_CLICKED) {
			menu = MENU_EXIT;
		}
	}
	HaltGui();
	mainWindow->Remove(&listBox);
	mainWindow->Remove(&w);
	mainWindow->Remove(&titleTxt);
	return menu;
}

/****************************************************************************
 * MenuGameAdd
 ***************************************************************************/

static int MenuGameAdd(xml_settings_t *settings)
{
	int menu = MENU_NONE;
	int ret;
	int i = 0;
	OptionList options;
	int choice;
	char msg[100];
	int game_mode = ENGLISH;
	char game_id[11] = {0};
	char game_title[51] = {0};
	char game_root[256] = {0};
	char game_save[256] = {0};
	char game_font[256] = {0};
	sprintf(options.name[i++], "Id");
	sprintf(options.name[i++], "Title");
	sprintf(options.name[i++], "Language");
	sprintf(options.name[i++], "Root Path");
	sprintf(options.name[i++], "Save Path");
	sprintf(options.name[i++], "Font");
	options.length = i;

	GuiText titleTxt("Add New Game", 28, (GXColor){255, 255, 255, 255});
	titleTxt.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	titleTxt.SetPosition(70,75);

	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);

	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);

	GuiText backBtnTxt("Cancel", 22, (GXColor){0, 0, 0, 255});
	GuiImage backBtnImg(&btnOutline);
	GuiImage backBtnImgOver(&btnOutlineOver);
	GuiButton backBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
	backBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	backBtn.SetPosition(100, -35);
	backBtn.SetLabel(&backBtnTxt);
	backBtn.SetImage(&backBtnImg);
	backBtn.SetImageOver(&backBtnImgOver);
	backBtn.SetSoundOver(&btnSoundOver);
	backBtn.SetTrigger(&trigA);
	backBtn.SetEffectGrow();

	GuiText addBtnTxt("Add Game", 22, (GXColor){0, 0, 0, 255});
	GuiImage addBtnImg(&btnOutline);
	GuiImage addBtnImgOver(&btnOutlineOver);
	GuiButton addBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
	addBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	addBtn.SetPosition(325, -35);
	addBtn.SetLabel(&addBtnTxt);
	addBtn.SetImage(&addBtnImg);
	addBtn.SetImageOver(&addBtnImgOver);
	addBtn.SetSoundOver(&btnSoundOver);
	addBtn.SetTrigger(&trigA);
	addBtn.SetEffectGrow();

	GuiOptionBrowser optionBrowser(552, 248, &options);
	optionBrowser.SetPosition(0, 108);
	optionBrowser.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	optionBrowser.SetCol2Position(185);

	HaltGui();
	GuiWindow w(screenwidth, screenheight);
	w.Append(&backBtn);
	w.Append(&addBtn);
	mainWindow->Append(&optionBrowser);
	mainWindow->Append(&w);
	mainWindow->Append(&titleTxt);
	ResumeGui();

	while(menu == MENU_NONE)
	{
		VIDEO_WaitVSync ();

		if(game_mode > 1)
			game_mode = 0;

		snprintf (options.value[0], 10, "%s", game_id);
		snprintf (options.value[1], 50, "%s", game_title);

		if (game_mode == ENGLISH) sprintf (options.value[2],"English");
		else sprintf (options.value[2],"Japanese");

		snprintf (options.value[3], 255, "%s", game_root);
		snprintf (options.value[4], 255, "%s", game_save);
		snprintf (options.value[5], 255, "%s", game_font);

		ret = optionBrowser.GetClickedOption();

		switch (ret)
		{
			case 0:
				OnScreenKeyboard(game_id, 10);
				break;

			case 1:
				OnScreenKeyboard(game_title, 50);
				break;

			case 2:
				game_mode++;
				break;

			case 3:
				OnScreenKeyboard(game_root, 255);
				break;

			case 4:
				OnScreenKeyboard(game_save, 255);
				break;

			case 5:
				OnScreenKeyboard(game_font, 255);
				break;
		}

		if(backBtn.GetState() == STATE_CLICKED)
		{
			menu = MENU_GAME_SEL;
		}

		if(addBtn.GetState() == STATE_CLICKED)
		{
			snprintf(msg, 100, "Add '%s'?", game_title);
			choice = WindowPrompt("Confirm", msg, "Yes", "No");
			if (choice == 1) {
				add_game(settings, game_id, game_title, game_mode, game_root, game_save, game_font);
				save_settings(settings);
				menu = MENU_GAME_SEL;
			} else {
				menu = MENU_NONE;
			}
		}

		if (exitBtn->GetState() == STATE_CLICKED) {
			menu = MENU_EXIT;
		}
	}
	HaltGui();
	mainWindow->Remove(&optionBrowser);
	mainWindow->Remove(&w);
	mainWindow->Remove(&titleTxt);
	return menu;
}

/****************************************************************************
 * MainMenu
 ***************************************************************************/
int MainMenu(ONScripterLabel *ons)
{
	int currentMenu = MENU_GAME_SEL;

	xml_settings_t *settings = open_settings();
	if (settings == NULL) {
		ons->setArchivePath("/apps/onscripter");
		return MENU_GAME_LAUNCH;
	}

	InitVideo(); // Initialise video
	InitAudio(); // Initialize audio

	// Initialize font system
	fontSystem = new FreeTypeGX();
	fontSystem->loadFont(font_ttf, font_ttf_size, 0);
	fontSystem->setCompatibilityMode(FTGX_COMPATIBILITY_DEFAULT_TEVOP_GX_PASSCLR | FTGX_COMPATIBILITY_DEFAULT_VTXDESC_GX_NONE);

	InitGUIThreads();

	pointer[0] = new GuiImageData(player1_point_png);
	pointer[1] = new GuiImageData(player2_point_png);
	pointer[2] = new GuiImageData(player3_point_png);
	pointer[3] = new GuiImageData(player4_point_png);

	mainWindow = new GuiWindow(screenwidth, screenheight);

	bgImg = new GuiImage(screenwidth, screenheight, (GXColor){50, 50, 50, 255});
	bgImg->ColorStripe(30);
	mainWindow->Append(bgImg);

	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	GuiText headerTxt("ONScripter", 28, (GXColor){255, 255, 255, 255});
	headerTxt.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	headerTxt.SetPosition(50,35);

	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiText exitBtnTxt("Exit", 22, (GXColor){0, 0, 0, 255});
	GuiImage exitBtnImg(&btnOutline);
	GuiImage exitBtnImgOver(&btnOutlineOver);

	exitBtn = new GuiButton(btnOutline.GetWidth(), btnOutline.GetHeight());
	exitBtn->SetAlignment(ALIGN_RIGHT, ALIGN_TOP);
	exitBtn->SetPosition(-20, 40);
	exitBtn->SetLabel(&exitBtnTxt);
	exitBtn->SetImage(&exitBtnImg);
	exitBtn->SetImageOver(&exitBtnImgOver);
	exitBtn->SetSoundOver(&btnSoundOver);
	exitBtn->SetTrigger(&trigA);

	mainWindow->Append(&headerTxt);
	mainWindow->Append(exitBtn);


	ResumeGui();

	bgMusic = new GuiSound(bg_music_ogg, bg_music_ogg_size, SOUND_OGG);
	bgMusic->SetVolume(50);
	bgMusic->Play(); // startup music

	while(currentMenu != MENU_EXIT && currentMenu != MENU_GAME_LAUNCH)
	{
		switch (currentMenu)
		{
			case MENU_GAME_SEL:
				currentMenu = MenuGameSel(ons, settings);
				break;
			case MENU_GAME_ADD:
				currentMenu = MenuGameAdd(settings);
				break;
			default: // unrecognized menu
				currentMenu = MenuGameSel(ons, settings);
				break;
		}
	}

	HaltGui();

	bgMusic->Stop();
	delete bgMusic;
	delete bgImg;
	delete exitBtn;
	delete mainWindow;

	delete pointer[0];
	delete pointer[1];
	delete pointer[2];
	delete pointer[3];

	mainWindow = NULL;
	close_settings(settings);
	ShutoffRumble();
	StopGX();
	VIDEO_SetPostRetraceCallback(NULL);
	return currentMenu;
}

