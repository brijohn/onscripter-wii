/* -*- C++ -*-
 *
 *  ONScripterLabel_rmenu.cpp - Right click menu handler of ONScripter
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

// Modified by Mion of Sonozaki Futago-tachi, March 2008, to update from
// Ogapee's 20080121 release source code.

#include "ONScripterLabel.h"

const char* messages[][8] = {
    { "%s%s@%sŒŽ%s“ú%sŽž%s•ª",
      "%s%s@||||||||||||",
      "%s%s‚ÉƒZ[ƒu‚µ‚Ü‚·B‚æ‚ë‚µ‚¢‚Å‚·‚©H",
      "%s%s‚ðƒ[ƒh‚µ‚Ü‚·B‚æ‚ë‚µ‚¢‚Å‚·‚©H",
      "ƒŠƒZƒbƒg‚µ‚Ü‚·B‚æ‚ë‚µ‚¢‚Å‚·‚©H",
      "I—¹‚µ‚Ü‚·B‚æ‚ë‚µ‚¢‚Å‚·‚©H",
      "‚Í‚¢",
      "‚¢‚¢‚¦" },
    { "`%s%s    Date %s/%s    Time %s:%s",
      "`%s%s    ------------------------",
      "`Save in slot %s%s?",
      "`Load from slot %s%s?",
      "`Return to Title Menu?",
      "`Quit?",
      "Yes",
      "No" }
};

const char* ONScripterLabel::getMessageString( MessageId which )
{
    return messages[script_h.preferred_script][which];
}

void ONScripterLabel::enterSystemCall()
{
    shelter_button_link = root_button_link.next;
    root_button_link.next = NULL;
    shelter_select_link = root_select_link.next;
    root_select_link.next = NULL;
    exbtn_d_shelter_button_link = exbtn_d_button_link;
    exbtn_d_button_link.exbtn_ctl = NULL;
    shelter_event_mode = event_mode;
    shelter_colors = current_page_colors.next;
    current_page_colors.next = NULL;
    shelter_mouse_state.x = last_mouse_state.x;
    shelter_mouse_state.y = last_mouse_state.y;
    event_mode = IDLE_EVENT_MODE;
    system_menu_enter_flag = true;
    yesno_caller = SYSTEM_NULL;
    shelter_display_mode = display_mode;
    display_mode = DISPLAY_MODE_TEXT;
    shelter_draw_cursor_flag = draw_cursor_flag;
    draw_cursor_flag = false;
}

void ONScripterLabel::leaveSystemCall( bool restore_flag )
{
    bool tmp = txtbtn_show;
    txtbtn_show = false;

    current_font = &sentence_font;
    display_mode = shelter_display_mode;
    system_menu_mode = SYSTEM_NULL;
    system_menu_enter_flag = false;
    yesno_caller = SYSTEM_NULL;
    key_pressed_flag = false;
    current_button_state.button = 0;

    if ( restore_flag ){

        current_page = cached_page;
        current_page_colors.next = shelter_colors;
        restoreTextBuffer();
        root_button_link.next = shelter_button_link;
        root_select_link.next = shelter_select_link;
        exbtn_d_button_link = exbtn_d_shelter_button_link;
        erasetextbtnCommand();

        event_mode = shelter_event_mode;
        draw_cursor_flag = shelter_draw_cursor_flag;
        if ( event_mode & WAIT_BUTTON_MODE ){
            SDL_WarpMouse( shelter_mouse_state.x, shelter_mouse_state.y );
        }
    }
    dirty_rect.fill( screen_width, screen_height );
    flush( refreshMode() );

    //printf("leaveSystemCall %d %d\n",event_mode, clickstr_state);

    advancePhase();
    refreshMouseOverButton();
    txtbtn_show = tmp;
}

void ONScripterLabel::executeSystemCall()
{
    //printf("*****  executeSystemCall %d %d %d*****\n", system_menu_enter_flag, volatile_button_state.button, system_menu_mode );
    dirty_rect.fill( screen_width, screen_height );

    if ( !system_menu_enter_flag ){
        enterSystemCall();
    }

    switch( system_menu_mode ){
      case SYSTEM_SKIP:
        executeSystemSkip();
        break;
      case SYSTEM_RESET:
        executeSystemReset();
        break;
      case SYSTEM_SAVE:
        executeSystemSave();
        break;
      case SYSTEM_YESNO:
        executeSystemYesNo();
        break;
      case SYSTEM_LOAD:
        executeSystemLoad();
        break;
      case SYSTEM_LOOKBACK:
        executeSystemLookback();
        break;
      case SYSTEM_WINDOWERASE:
        executeWindowErase();
        break;
      case SYSTEM_MENU:
        executeSystemMenu();
        break;
      case SYSTEM_AUTOMODE:
        executeSystemAutomode();
        break;
      case SYSTEM_END:
        executeSystemEnd();
        break;
      default:
        leaveSystemCall();
    }
}

void ONScripterLabel::executeSystemMenu()
{
    RMenuLink *link;
    int counter = 1;

    current_font = &menu_font;
    if ( event_mode & WAIT_BUTTON_MODE ){

        if ( current_button_state.button == 0 ) return;
        event_mode = IDLE_EVENT_MODE;

        deleteButtonLink();

        if ( current_button_state.button == -1 ){
            if ( menuselectvoice_file_name[MENUSELECTVOICE_CANCEL] )
                playSound(menuselectvoice_file_name[MENUSELECTVOICE_CANCEL],
                          SOUND_WAVE|SOUND_OGG, false, MIX_WAVE_CHANNEL);
            leaveSystemCall();
            return;
        }

        if ( menuselectvoice_file_name[MENUSELECTVOICE_CLICK] )
            playSound(menuselectvoice_file_name[MENUSELECTVOICE_CLICK],
                      SOUND_WAVE|SOUND_OGG, false, MIX_WAVE_CHANNEL);

        link = root_rmenu_link.next;
        while ( link ){
            if ( current_button_state.button == counter++ ){
                system_menu_mode = link->system_call_no;
                break;
            }
            link = link->next;
        }

        advancePhase();
    }
    else{
        if ( menuselectvoice_file_name[MENUSELECTVOICE_OPEN] )
            playSound(menuselectvoice_file_name[MENUSELECTVOICE_OPEN],
                      SOUND_WAVE|SOUND_OGG, false, MIX_WAVE_CHANNEL);

        system_menu_mode = SYSTEM_MENU;
        yesno_caller = SYSTEM_MENU;

        text_info.fill( 0, 0, 0, 0 );
        flush( refreshMode() );

        menu_font.num_xy[0] = rmenu_link_width;
        menu_font.num_xy[1] = rmenu_link_num;
        menu_font.top_xy[0] = (screen_width * screen_ratio2 / screen_ratio1 - menu_font.num_xy[0] * menu_font.pitch_xy[0]) / 2;
        menu_font.top_xy[1] = (screen_height * screen_ratio2 / screen_ratio1  - menu_font.num_xy[1] * menu_font.pitch_xy[1]) / 2;
        menu_font.setXY( (menu_font.num_xy[0] - rmenu_link_width) / 2,
                         (menu_font.num_xy[1] - rmenu_link_num) / 2 );

        link = root_rmenu_link.next;
        while( link ){
            ButtonLink *button = getSelectableSentence( link->label, &menu_font, false );
            root_button_link.insert( button );
            button->no = counter++;

            link = link->next;
            flush( refreshMode() );
        }

        flushEvent();
        event_mode = WAIT_BUTTON_MODE;
        refreshMouseOverButton();
    }
}

void ONScripterLabel::executeSystemSkip()
{
    skip_mode |= SKIP_NORMAL;
    if ( !(shelter_event_mode & WAIT_BUTTON_MODE) )
        shelter_event_mode &= ~WAIT_TIMER_MODE;
    leaveSystemCall();
}

void ONScripterLabel::executeSystemAutomode()
{
    automode_flag = true;
    skip_mode &= ~SKIP_NORMAL;
    printf("systemcall_automode: change to automode\n");
    leaveSystemCall();
}

void ONScripterLabel::executeSystemReset()
{
    if ( yesno_caller == SYSTEM_RESET ){
        ctrl_pressed_status = true;
        leaveSystemCall();
    }
    else{
        yesno_caller = SYSTEM_RESET;
        system_menu_mode = SYSTEM_YESNO;
        advancePhase();
    }
}

void ONScripterLabel::executeSystemEnd()
{
    if ( yesno_caller == SYSTEM_END ){
        leaveSystemCall();
        ctrl_pressed_status = true;
    }
    else{
        yesno_caller = SYSTEM_END;
        system_menu_mode = SYSTEM_YESNO;
        advancePhase();
    }
}

void ONScripterLabel::executeWindowErase()
{
    if ( event_mode & WAIT_BUTTON_MODE ){
        event_mode = IDLE_EVENT_MODE;

        if (windowchip_sprite_no >= 0)
            sprite_info[windowchip_sprite_no].visible = true;

        leaveSystemCall();
    }
    else{
        if (windowchip_sprite_no >= 0)
            sprite_info[windowchip_sprite_no].visible = false;

        display_mode = DISPLAY_MODE_NORMAL;
        flush(mode_saya_flag ? REFRESH_SAYA_MODE : REFRESH_NORMAL_MODE);

        event_mode = WAIT_BUTTON_MODE;
        system_menu_mode = SYSTEM_WINDOWERASE;
        deleteButtonLink();
    }
}

void ONScripterLabel::executeSystemLoad()
{
    SaveFileInfo save_file_info;

    current_font = &menu_font;
    if ( event_mode & WAIT_BUTTON_MODE ){

        if ( current_button_state.button == 0 ) return;
        event_mode = IDLE_EVENT_MODE;

        if ( current_button_state.button > 0 ){
            searchSaveFile( save_file_info, current_button_state.button );
            if ( !save_file_info.valid ){
                event_mode  = WAIT_BUTTON_MODE;
                refreshMouseOverButton();
                return;
            }
            deleteButtonLink();
            yesno_selected_file_no = current_button_state.button;
            yesno_caller = SYSTEM_LOAD;
            system_menu_mode = SYSTEM_YESNO;
            advancePhase();
        }
        else{
            deleteButtonLink();
            leaveSystemCall();
        }
    }
    else{
        system_menu_mode = SYSTEM_LOAD;

        text_info.fill( 0, 0, 0, 0 );

        menu_font.num_xy[0] = (strlen(save_item_name)+1)/2+2+13;
        menu_font.num_xy[1] = num_save_file+2;
        menu_font.top_xy[0] = (screen_width * screen_ratio2 / screen_ratio1 - menu_font.num_xy[0] * menu_font.pitch_xy[0]) / 2;
        menu_font.top_xy[1] = (screen_height * screen_ratio2 / screen_ratio1  - menu_font.num_xy[1] * menu_font.pitch_xy[1]) / 2;
        menu_font.setXY( (menu_font.num_xy[0] - (strlen( load_menu_name )+1) / 2) / 2, 0 );
//        uchar3 color = {0xff, 0xff, 0xff};
        // drawString( load_menu_name, color, &menu_font, true, accumulation_surface, NULL, &text_info );
        /* The following three lines are part of a hack allowing the
           menu name to show up when custom right-menus are in
           existence.  As it stands in ONScripter, these menu names
           get drawn in the accumulation buffer one level below the
           menu itself -- i.e. onto the main playing field itself.  So
           when the user right-clicks out of the right- click menu,
           that string remains there for the rest of play.  I do not
           currently understand why this is, but I do know that using
           ButtonLink and getSelectableSentence to create a
           nonselectable text button instead of using drawString as
           above will not trigger this defect.  I don't know why this
           is right now; more investigation is warranted.  Do not
           recommend for integration. [Seung Park, 20060331] */
        ButtonLink *ooga = getSelectableSentence( load_menu_name, &menu_font, false );
	root_button_link.insert( ooga );
        ooga->no = 0;
	
        menu_font.newLine();
// Mion of Sonozaki Futago-tachi recommends:
//        menu_font.newLine();

        flush( refreshMode() );

        bool nofile_flag;
        int slen = strlen(save_item_name);
        char *buffer = new char[ slen + (slen % 2) + 30 + 1 ];

        for ( unsigned int i=1 ; i<=num_save_file ; i++ ){
            searchSaveFile( save_file_info, i );
            menu_font.setXY( (menu_font.num_xy[0] - ((strlen( save_item_name )+1) / 2 + 15) ) / 2 );

            if ( save_file_info.valid ){
                sprintf( buffer, getMessageString(MESSAGE_SAVE_EXIST),
                         save_item_name,
                         save_file_info.sjis_no,
                         save_file_info.sjis_month,
                         save_file_info.sjis_day,
                         save_file_info.sjis_hour,
                         save_file_info.sjis_minute );
                nofile_flag = false;
            }
            else{
                sprintf( buffer, getMessageString(MESSAGE_SAVE_EMPTY),
                         save_item_name,
                         save_file_info.sjis_no );
                nofile_flag = true;
            }
            ButtonLink *button = getSelectableSentence( buffer, &menu_font, false, nofile_flag );
            root_button_link.insert( button );
            button->no = i;
            flush( refreshMode() );
        }
        delete[] buffer;

        event_mode = WAIT_BUTTON_MODE;
        refreshMouseOverButton();
    }
}

void ONScripterLabel::executeSystemSave()
{
    current_font = &menu_font;
    if ( event_mode & WAIT_BUTTON_MODE ){

        if ( current_button_state.button == 0 ) return;
        event_mode = IDLE_EVENT_MODE;

        deleteButtonLink();

        if ( current_button_state.button > 0 ){
            yesno_selected_file_no = current_button_state.button;
            yesno_caller = SYSTEM_SAVE;
            system_menu_mode = SYSTEM_YESNO;
            advancePhase();
            return;
        }
        leaveSystemCall();
    }
    else{
        system_menu_mode = SYSTEM_SAVE;

        text_info.fill( 0, 0, 0, 0 );

        menu_font.num_xy[0] = (strlen(save_item_name)+1)/2+2+13;
        menu_font.num_xy[1] = num_save_file+2;
        menu_font.top_xy[0] = (screen_width * screen_ratio2 / screen_ratio1 - menu_font.num_xy[0] * menu_font.pitch_xy[0]) / 2;
        menu_font.top_xy[1] = (screen_height * screen_ratio2 / screen_ratio1  - menu_font.num_xy[1] * menu_font.pitch_xy[1]) / 2;
        menu_font.setXY((menu_font.num_xy[0] - (strlen( save_menu_name )+1) / 2 ) / 2, 0);
//        uchar3 color = {0xff, 0xff, 0xff};
        // drawString( save_menu_name, color, &menu_font, true, accumulation_surface, NULL, &text_info );
        /* The following three lines are part of a hack allowing the
           menu name to show up when custom right-menus are in
           existence.  As it stands in ONScripter, these menu names
           get drawn in the accumulation buffer one level below the
           menu itself -- i.e. onto the main playing field itself.  So
           when the user right-clicks out of the right- click menu,
           that string remains there for the rest of play.  I do not
           currently understand why this is, but I do know that using
           ButtonLink and getSelectableSentence to create a
           nonselectable text button instead of using drawString as
           above will not trigger this defect.  I don't know why this
           is right now; more investigation is warranted.  Do not
           recommend for integration. [Seung Park, 20060331] */
        ButtonLink *ooga = getSelectableSentence( save_menu_name, &menu_font, false );
		root_button_link.insert( ooga );
        ooga->no = 0;

        menu_font.newLine();
        //menu_font.newLine(); // another Mion recommendation

        flush( refreshMode() );

        bool nofile_flag;
        int slen = strlen(save_item_name);
        char *buffer = new char[ slen + (slen % 2) + 30 + 1 ];

        for ( unsigned int i=1 ; i<=num_save_file ; i++ ){
            SaveFileInfo save_file_info;
            searchSaveFile( save_file_info, i );
            menu_font.setXY( (menu_font.num_xy[0] - ((strlen( save_item_name )+1) / 2 + 15) ) / 2 );

            if ( save_file_info.valid ){
                sprintf( buffer, getMessageString(MESSAGE_SAVE_EXIST),
                         save_item_name,
                         save_file_info.sjis_no,
                         save_file_info.sjis_month,
                         save_file_info.sjis_day,
                         save_file_info.sjis_hour,
                         save_file_info.sjis_minute );
                nofile_flag = false;
            }
            else{
                sprintf( buffer, getMessageString(MESSAGE_SAVE_EMPTY),
                         save_item_name,
                         save_file_info.sjis_no );
                nofile_flag = true;
            }
            ButtonLink *button = getSelectableSentence( buffer, &menu_font, false, nofile_flag );
            root_button_link.insert( button );
            button->no = i;
            flush( refreshMode() );
        }
        delete[] buffer;

        event_mode = WAIT_BUTTON_MODE;
        refreshMouseOverButton();
    }
}

void ONScripterLabel::executeSystemYesNo()
{
    char name[64] = {'\0'};

    current_font = &menu_font;
    if ( event_mode & WAIT_BUTTON_MODE ){

        if ( current_button_state.button == 0 ) return;
        event_mode = IDLE_EVENT_MODE;

        deleteButtonLink();

        if ( current_button_state.button == 1 ){ // yes is selected
            if ( menuselectvoice_file_name[MENUSELECTVOICE_YES] )
                playSound(menuselectvoice_file_name[MENUSELECTVOICE_YES],
                          SOUND_WAVE|SOUND_OGG, false, MIX_WAVE_CHANNEL);
            if ( yesno_caller == SYSTEM_SAVE ){
                saveSaveFile( yesno_selected_file_no );
                leaveSystemCall();
            }
            else if ( yesno_caller == SYSTEM_LOAD ){

                current_font = &sentence_font;
                if ( loadSaveFile( yesno_selected_file_no ) ){
                    system_menu_mode = yesno_caller;
                    advancePhase();
                    return;
                }
                leaveSystemCall( false );
                saveon_flag = true;
                internal_saveon_flag = true;
                text_on_flag = false;
                indent_offset = 0;
                line_enter_status = 0;
                page_enter_status = 0;
                string_buffer_offset = 0;
		break_flag = false;

                if (loadgosub_label)
                    gosubReal( loadgosub_label, script_h.getCurrent() );
                readToken();
            }
            else if ( yesno_caller ==  SYSTEM_RESET ){
                resetCommand();
                readToken();
                event_mode = IDLE_EVENT_MODE;
                leaveSystemCall( false );
            }
            else if ( yesno_caller ==  SYSTEM_END ){

                endCommand();
            }
        }
        else{
            if ( menuselectvoice_file_name[MENUSELECTVOICE_NO] )
                playSound(menuselectvoice_file_name[MENUSELECTVOICE_NO],
                          SOUND_WAVE|SOUND_OGG, false, MIX_WAVE_CHANNEL);
            system_menu_mode = yesno_caller & 0xf;
            if (yesno_caller == SYSTEM_RESET)
				leaveSystemCall();
            advancePhase();
        }
    }
    else{
        text_info.fill( 0, 0, 0, 0 );

        if ( yesno_caller == SYSTEM_SAVE ){
            SaveFileInfo save_file_info;
            searchSaveFile( save_file_info, yesno_selected_file_no );
            sprintf( name, getMessageString(MESSAGE_SAVE_CONFIRM),
                     save_item_name,
                     save_file_info.sjis_no );
        }
        else if ( yesno_caller == SYSTEM_LOAD ){
            SaveFileInfo save_file_info;
            searchSaveFile( save_file_info, yesno_selected_file_no );
            sprintf( name, getMessageString(MESSAGE_LOAD_CONFIRM),
                     save_item_name,
                     save_file_info.sjis_no );
        }
        else if ( yesno_caller ==  SYSTEM_RESET )
            strcpy( name, getMessageString(MESSAGE_RESET_CONFIRM) );
        else if ( yesno_caller ==  SYSTEM_END )
            strcpy( name, getMessageString(MESSAGE_END_CONFIRM) );


        menu_font.num_xy[0] = strlen(name)/2;
        menu_font.num_xy[1] = 3;
        menu_font.top_xy[0] = (screen_width * screen_ratio2 / screen_ratio1 - menu_font.num_xy[0] * menu_font.pitch_xy[0]) / 2;
        menu_font.top_xy[1] = (screen_height * screen_ratio2 / screen_ratio1  - menu_font.num_xy[1] * menu_font.pitch_xy[1]) / 2;
        menu_font.setXY(0, 0);
//        uchar3 color = {0xff, 0xff, 0xff};

        // drawString( name, color, &menu_font, true, accumulation_surface, NULL, &text_info );
        /* The following three lines are part of a hack allowing the
           menu name to show up when custom right-menus are in
           existence.  As it stands in ONScripter, these menu names
           get drawn in the accumulation buffer one level below the
           menu itself -- i.e. onto the main playing field itself.  So
           when the user right-clicks out of the right- click menu,
           that string remains there for the rest of play.  I do not
           currently understand why this is, but I do know that using
           ButtonLink and getSelectableSentence to create a
           nonselectable text button instead of using drawString as
           above will not trigger this defect.  I don't know why this
           is right now; more investigation is warranted.  Do not
           recommend for integration. [Seung Park, 20060331] */
        ButtonLink *ooga = getSelectableSentence( name, &menu_font, false );
        root_button_link.insert( ooga );
        ooga->no = 0;

        flush( refreshMode() );

        int offset1 = strlen(name)/5;
        int offset2 = strlen(name)/2 - offset1;
        strcpy( name, getMessageString(MESSAGE_YES) );
        menu_font.setXY(offset1-2, 2);
        ButtonLink *button = getSelectableSentence( name, &menu_font, false );
        root_button_link.insert( button );
        button->no = 1;

        strcpy( name, getMessageString(MESSAGE_NO) );
        menu_font.setXY(offset2, 2);
        button = getSelectableSentence( name, &menu_font, false );
        root_button_link.insert( button );
        button->no = 2;

        flush( refreshMode() );

        event_mode = WAIT_BUTTON_MODE;
        refreshMouseOverButton();
    }
}

void ONScripterLabel::setupLookbackButton()
{
    deleteButtonLink();

    /* ---------------------------------------- */
    /* Previous button check */
    if ( (current_page->previous->text_count > 0 ) &&
         current_page != start_page ){
        ButtonLink *button = new ButtonLink();
        root_button_link.insert( button );

        button->no = 1;
        button->select_rect.x = sentence_font_info.pos.x;
        button->select_rect.y = sentence_font_info.pos.y;
        button->select_rect.w = sentence_font_info.pos.w;
        button->select_rect.h = sentence_font_info.pos.h/3;

        if ( lookback_sp[0] >= 0 ){
            button->button_type = ButtonLink::SPRITE_BUTTON;
            button->sprite_no = lookback_sp[0];
            sprite_info[ button->sprite_no ].visible = true;
            button->image_rect = sprite_info[ button->sprite_no ].pos;
        }
        else{
            button->button_type = ButtonLink::LOOKBACK_BUTTON;
            button->show_flag = 2;
            button->anim[0] = &lookback_info[0];
            button->anim[1] = &lookback_info[1];
            button->image_rect.x = sentence_font_info.pos.x + sentence_font_info.pos.w - button->anim[0]->pos.w;
            button->image_rect.y = sentence_font_info.pos.y;
            button->image_rect.w = button->anim[0]->pos.w;
            button->image_rect.h = button->anim[0]->pos.h;
            button->anim[0]->pos.x = button->anim[1]->pos.x = button->image_rect.x;
            button->anim[0]->pos.y = button->anim[1]->pos.y = button->image_rect.y;
        }
    }
    else if (lookback_sp[0] >= 0){
        sprite_info[ lookback_sp[0] ].visible = false;
    }

    /* ---------------------------------------- */
    /* Next button check */
    if ( current_page->next != cached_page ){
        ButtonLink *button = new ButtonLink();
        root_button_link.insert( button );

        button->no = 2;
        button->select_rect.x = sentence_font_info.pos.x;
        button->select_rect.y = sentence_font_info.pos.y + sentence_font_info.pos.h*2/3;
        button->select_rect.w = sentence_font_info.pos.w;
        button->select_rect.h = sentence_font_info.pos.h/3;

        if ( lookback_sp[1] >= 0 ){
            button->button_type = ButtonLink::SPRITE_BUTTON;
            button->sprite_no = lookback_sp[1];
            sprite_info[ button->sprite_no ].visible = true;
            button->image_rect = sprite_info[ button->sprite_no ].pos;
        }
        else{
            button->button_type = ButtonLink::LOOKBACK_BUTTON;
            button->show_flag = 2;
            button->anim[0] = &lookback_info[2];
            button->anim[1] = &lookback_info[3];
            button->image_rect.x = sentence_font_info.pos.x + sentence_font_info.pos.w - button->anim[0]->pos.w;
            button->image_rect.y = sentence_font_info.pos.y + sentence_font_info.pos.h - button->anim[0]->pos.h;
            button->image_rect.w = button->anim[0]->pos.w;
            button->image_rect.h = button->anim[0]->pos.h;
            button->anim[0]->pos.x = button->anim[1]->pos.x = button->image_rect.x;
            button->anim[0]->pos.y = button->anim[1]->pos.y = button->image_rect.y;
        }
    }
    else if (lookback_sp[1] >= 0){
        sprite_info[ lookback_sp[1] ].visible = false;
    }
}

void ONScripterLabel::executeSystemLookback()
{
    uchar3 color;

    current_font = &sentence_font;
    if ( event_mode & WAIT_BUTTON_MODE ){
        if ( current_button_state.button == 0 ||
             ( current_page == start_page &&
               current_button_state.button == -2 ) )
            return;
        if ( current_button_state.button == -1 ||
             ( current_button_state.button == -3 &&
               current_page->next == cached_page ) ||
             current_button_state.button <= -4 )
        {
            event_mode = IDLE_EVENT_MODE;
            deleteButtonLink();
            if ( lookback_sp[0] >= 0 )
                sprite_info[ lookback_sp[0] ].visible = false;
            if ( lookback_sp[1] >= 0 )
                sprite_info[ lookback_sp[1] ].visible = false;
            leaveSystemCall();
            return;
        }

        if ( current_button_state.button == 1 ||
             current_button_state.button == -2 ){
            current_page = current_page->previous;
        }
        else
            current_page = current_page->next;
    }
    else{
        current_page = current_page->previous;
        if ( current_page->text_count == 0 ){
            if ( lookback_sp[0] >= 0 )
                sprite_info[ lookback_sp[0] ].visible = false;
            if ( lookback_sp[1] >= 0 )
                sprite_info[ lookback_sp[1] ].visible = false;
            leaveSystemCall();
            return;
        }

        event_mode = WAIT_BUTTON_MODE;
        system_menu_mode = SYSTEM_LOOKBACK;
    }

    setupLookbackButton();
    refreshMouseOverButton();

    setColor(color, current_page_colors.color);
    setColor(current_page_colors.color, lookback_color);
    restoreTextBuffer();
    setColor(current_page_colors.color, color);

    dirty_rect.fill( screen_width, screen_height );
    flush( refreshMode() );
}
