/****************************************************************************
 * libwiigui
 * Tantric 2009
 * Brian Johnson 2009
 *
 * gui_listbox.cpp
 * GUI class definitions
 ***************************************************************************/

#include "gui.h"

/**
 * Constructor for the GuiListBox class.
 */
GuiListBox::GuiListBox(int w, int h)
{
	width = w;
	height = h;
	selectedItem = 0;
	pageItem = 0;
	pageIndex = 0;
	selectable = true;
	listChanged = true; // trigger an initial list update
	focus = 0; // allow focus

	trigA = new GuiTrigger;
	trigA->SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);

	btnSoundOver = new GuiSound(button_over_pcm, button_over_pcm_size, SOUND_PCM);

	bgListSelection = new GuiImageData(bg_list_selection_png);
	bgListSelectionImg = new GuiImage(bgListSelection);
	bgListSelectionImg->SetParent(this);
	bgListSelectionImg->SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);

	bgListSelectionEntry = new GuiImageData(bg_list_selection_entry_png);

	scrollbar = new GuiImageData(scrollbar_png);
	scrollbarImg = new GuiImage(scrollbar);
	scrollbarImg->SetParent(this);
	scrollbarImg->SetAlignment(ALIGN_RIGHT, ALIGN_TOP);
	scrollbarImg->SetPosition(0, 30);

	arrowDown = new GuiImageData(scrollbar_arrowdown_png);
	arrowDownImg = new GuiImage(arrowDown);
	arrowDownOver = new GuiImageData(scrollbar_arrowdown_over_png);
	arrowDownOverImg = new GuiImage(arrowDownOver);
	arrowUp = new GuiImageData(scrollbar_arrowup_png);
	arrowUpImg = new GuiImage(arrowUp);
	arrowUpOver = new GuiImageData(scrollbar_arrowup_over_png);
	arrowUpOverImg = new GuiImage(arrowUpOver);
	scrollbarBox = new GuiImageData(scrollbar_box_png);
	scrollbarBoxImg = new GuiImage(scrollbarBox);
	scrollbarBoxOver = new GuiImageData(scrollbar_box_over_png);
	scrollbarBoxOverImg = new GuiImage(scrollbarBoxOver);

	arrowUpBtn = new GuiButton(arrowUpImg->GetWidth(), arrowUpImg->GetHeight());
	arrowUpBtn->SetParent(this);
	arrowUpBtn->SetImage(arrowUpImg);
	arrowUpBtn->SetImageOver(arrowUpOverImg);
	arrowUpBtn->SetAlignment(ALIGN_RIGHT, ALIGN_TOP);
	arrowUpBtn->SetSelectable(false);
	arrowUpBtn->SetTrigger(trigA);
	arrowUpBtn->SetSoundOver(btnSoundOver);

	arrowDownBtn = new GuiButton(arrowDownImg->GetWidth(), arrowDownImg->GetHeight());
	arrowDownBtn->SetParent(this);
	arrowDownBtn->SetImage(arrowDownImg);
	arrowDownBtn->SetImageOver(arrowDownOverImg);
	arrowDownBtn->SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
	arrowDownBtn->SetSelectable(false);
	arrowDownBtn->SetTrigger(trigA);
	arrowDownBtn->SetSoundOver(btnSoundOver);

	scrollbarBoxBtn = new GuiButton(scrollbarBoxImg->GetWidth(), scrollbarBoxImg->GetHeight());
	scrollbarBoxBtn->SetParent(this);
	scrollbarBoxBtn->SetImage(scrollbarBoxImg);
	scrollbarBoxBtn->SetImageOver(scrollbarBoxOverImg);
	scrollbarBoxBtn->SetAlignment(ALIGN_RIGHT, ALIGN_TOP);
	scrollbarBoxBtn->SetSelectable(false);

	for(int i = 0; i < PAGESIZE; i++) {
		list[i] = new GuiButton(380, 30);
		listBg[i] = new GuiImage(bgListSelectionEntry);
		list[i]->SetParent(this);
		list[i]->SetImageOver(listBg[i]);
		list[i]->SetPosition(2, 30 * i + 3);
		list[i]->SetTrigger(trigA);
	}

	SetClickable(true);
	Clear();
}

/**
 * Destructor for the GuiListBox class.
 */
GuiListBox::~GuiListBox()
{
	Clear();

	delete arrowUpBtn;
	delete arrowDownBtn;
	delete scrollbarBoxBtn;

	delete bgListSelectionImg;
	delete scrollbarImg;
	delete arrowDownImg;
	delete arrowDownOverImg;
	delete arrowUpImg;
	delete arrowUpOverImg;
	delete scrollbarBoxImg;
	delete scrollbarBoxOverImg;

	delete bgListSelection;
	delete bgListSelectionEntry;
	delete scrollbar;
	delete arrowDown;
	delete arrowDownOver;
	delete arrowUp;
	delete arrowUpOver;
	delete scrollbarBox;
	delete scrollbarBoxOver;

	delete btnSoundOver;
	delete trigA;

	for(int i = 0; i < PAGESIZE; i++) {
		delete list[i];
		delete listBg[i];
	}
}

void GuiListBox::SetClickable(bool c)
{
	clickable = c;
	for (int i = 0; i < PAGESIZE; i++) {
		list[i]->SetClickable(c);
	}
}

int GuiListBox::GetSelectedItem()
{
	if(list[selectedItem - pageIndex]->GetState() == STATE_SELECTED) {
		return selectedItem;
	}
	return -1;
}

void GuiListBox::Clear()
{
	while (_items.size() > 0) {
		ListBoxItem * item = _items[0];
		_items.erase(_items.begin());
		delete item;
	}
}

void GuiListBox::AddItem(const char * text)
{
	_items.push_back(new ListBoxItem(text));
	TriggerUpdate();
}

void GuiListBox::AddItem(const char * text, const u8* icon)
{
	_items.push_back(new ListBoxItem(text, icon));
	TriggerUpdate();
}

void GuiListBox::RemoveItem(unsigned int item)
{
	if (item < _items.size()) {
		ListBoxItem * listItem = _items[item];
		_items.erase(_items.begin() + item);
		delete listItem;
	}
	for(int i=0; i<PAGESIZE; i++)
	{
		list[i]->ResetState();
		list[i]->SetVisible(false);
	}
	ResetState();
	list[0]->SetState(STATE_SELECTED);
	TriggerUpdate();
}

void GuiListBox::SetFocus(int f)
{
	focus = f;

	for(int i=0; i<PAGESIZE; i++)
		list[i]->ResetState();

	if(f == 1)
		list[pageItem]->SetState(STATE_SELECTED);
}

void GuiListBox::ResetState()
{
	state = STATE_DEFAULT;
	pageItem = 0;
	selectedItem = pageIndex = 0;

	for(int i=0; i<PAGESIZE; i++)
	{
		list[i]->ResetState();
	}
}

void GuiListBox::TriggerUpdate()
{
	listChanged = true;
}

/**
 * Draw the button on screen
 */
void GuiListBox::Draw()
{
	if(!this->IsVisible())
		return;

	bgListSelectionImg->Draw();

	for(int i=0; i<PAGESIZE; i++)
	{
		list[i]->Draw();
	}

	scrollbarImg->Draw();
	arrowUpBtn->Draw();
	arrowDownBtn->Draw();
	scrollbarBoxBtn->Draw();

	this->UpdateEffects();
}

void GuiListBox::Update(GuiTrigger * t)
{
	if(state == STATE_DISABLED || !t)
		return;

	// update the location of the scroll box based on the position in the file list
	int position = 136 * (pageIndex + pageItem) / _items.size();
	scrollbarBoxBtn->SetPosition(0, position + 36);

	arrowUpBtn->Update(t);
	arrowDownBtn->Update(t);
	scrollbarBoxBtn->Update(t);

	// pad/joystick navigation
	if(!focus)
	{
		goto endNavigation; // skip navigation
		listChanged = false;
	}

	if(t->Right() || arrowDownBtn->GetState() == STATE_CLICKED)
	{
		if(pageIndex < _items.size() && _items.size() > PAGESIZE)
		{
			pageIndex += PAGESIZE;
			if(pageIndex + PAGESIZE >= _items.size())
				pageIndex = _items.size() - PAGESIZE;
			listChanged = true;
		}
		arrowDownBtn->ResetState();
	}
	else if(t->Left() || arrowUpBtn->GetState() == STATE_CLICKED)
	{
		if(pageIndex > 0)
		{
			pageIndex -= PAGESIZE;
			if(pageIndex > _items.size())
				pageIndex = 0;
			listChanged = true;
		}
		arrowUpBtn->ResetState();
	}
	else if(t->Down())
	{
		if(pageIndex + pageItem + 1 < _items.size())
		{
			if(pageItem == PAGESIZE - 1)
			{
				// move list down by 1
				pageIndex++;
				listChanged = true;
			}
			else if(list[pageItem + 1]->IsVisible())
			{
				list[pageItem]->ResetState();
				list[++pageItem]->SetState(STATE_SELECTED);
			}
		}
	}
	else if(t->Up())
	{
		if(pageItem == 0 && pageIndex + pageItem > 0)
		{
			// move list up by 1
			pageIndex--;
			listChanged = true;
		}
		else if(pageItem > 0)
		{
			list[pageItem]->ResetState();
			list[--pageItem]->SetState(STATE_SELECTED);
		}
	}

	endNavigation:

	for(unsigned int i = 0; i < PAGESIZE; i++)
	{
		if(listChanged)
		{
			if(pageIndex + i < _items.size())
			{
				if(list[i]->GetState() == STATE_DISABLED)
					list[i]->SetState(STATE_DEFAULT);

				list[i]->SetVisible(true);

				list[i]->SetLabel(_items[pageIndex + i]->GetText());
				if (_items[pageIndex + i]->GetIcon()) {
					list[i]->SetIcon(_items[pageIndex + i]->GetIcon());
					_items[pageIndex + i]->GetText()->SetPosition(30,0);
				} else {
					list[i]->SetIcon(NULL);
					_items[pageIndex + i]->GetText()->SetPosition(10,0);
				}
			}
			else
			{
				list[i]->SetVisible(false);
				list[i]->SetState(STATE_DISABLED);
			}
		}

		if(focus)
		{
			if(i != pageItem && list[i]->GetState() == STATE_SELECTED)
				list[i]->ResetState();
			else if(i == pageItem && list[i]->GetState() == STATE_DEFAULT)
				list[pageItem]->SetState(STATE_SELECTED);
		}

		list[i]->Update(t);

		if (list[i]->GetState() == STATE_CLICKED) {
			state = STATE_CLICKED;
			list[i]->SetState(STATE_SELECTED);
		}

		if (list[i]->GetState() == STATE_SELECTED)
		{
			pageItem = i;
			selectedItem = pageIndex + i;
		}
	}

	listChanged = false;

	if(updateCB)
		updateCB(this);

}
