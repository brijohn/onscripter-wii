/*!\mainpage libwiigui Documentation
 *
 * \section Introduction
 * libwiigui is a GUI library for the Wii, created to help structure the
 * design of a complicated GUI interface, and to enable an author to create
 * a sophisticated, feature-rich GUI. It was originally conceived and written
 * after I started to design a GUI for Snes9x GX, and found libwiisprite and
 * GRRLIB inadequate for the purpose. It uses GX for drawing, and makes use
 * of PNGU for displaying images and FreeTypeGX for text. It was designed to
 * be flexible and is easy to modify - don't be afraid to change the way it
 * works or expand it to suit your GUI's purposes! If you do, and you think
 * your changes might benefit others, please share them so they might be
 * added to the project!
 *
 * \section Quickstart
 * Start from the supplied template example. For more advanced uses, and more
 * extensions, see the source code for Snes9x GX, FCE Ultra GX, and
 * Visual Boy Advance GX.

 * \section Contact
 * If you have any suggestions for the library or documentation, or want to
 * contribute, please visit the libwiigui website:
 * http://code.google.com/p/libwiigui/

 * \section Credits
 * This library was wholly designed and written by Tantric. Thanks to the
 * authors of PNGU and FreeTypeGX, of which this library makes use. Thanks
 * also to the authors of GRRLIB and libwiisprite for laying the foundations.
 *
*/

#ifndef LIBWIIGUI_H
#define LIBWIIGUI_H

#include <gccore.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <exception>
#include <vector>
#include <math.h>
#include <asndlib.h>
#include "pngu/pngu.h"
#include "FreeTypeGX.h"
#include "video.h"
#include "input.h"
#include "filelist.h"
#include "menu.h"
#include "oggplayer.h"

#define SCROLL_INITIAL_DELAY 	20
#define SCROLL_LOOP_DELAY 		3
#define PAGESIZE	 			8
#define MAX_OPTIONS 			30

typedef void (*UpdateCallback)(void * e);

enum
{
	ALIGN_LEFT,
	ALIGN_RIGHT,
	ALIGN_CENTRE,
	ALIGN_TOP,
	ALIGN_BOTTOM,
	ALIGN_MIDDLE
};

enum
{
	STATE_DEFAULT,
	STATE_SELECTED,
	STATE_CLICKED,
	STATE_DISABLED
};

enum
{
	SOUND_PCM,
	SOUND_OGG
};

enum
{
	IMAGE_TEXTURE,
	IMAGE_COLOR,
	IMAGE_DATA
};

#define EFFECT_SLIDE_TOP			1
#define EFFECT_SLIDE_BOTTOM			2
#define EFFECT_SLIDE_RIGHT			4
#define EFFECT_SLIDE_LEFT			8
#define EFFECT_SLIDE_IN				16
#define EFFECT_SLIDE_OUT			32
#define EFFECT_FADE					64
#define EFFECT_SCALE				128
#define EFFECT_COLOR_TRANSITION		256

extern FreeTypeGX *fontSystem;

//!Sound conversion and playback. A wrapper for other sound libraries - ASND, libmad, ltremor, etc
class GuiSound
{
	public:
		//!Constructor
		//!\param s Pointer to the sound data
		//!\param l Length of sound data
		//!\param t Sound format type (SOUND_PCM or SOUND_OGG)
		GuiSound(const u8 * s, int l, int t);
		//!Destructor
		~GuiSound();
		//!Start sound playback
		void Play();
		//!Stop sound playback
		void Stop();
		//!Pause sound playback
		void Pause();
		//!Resume sound playback
		void Resume();
		//!Checks if the sound is currently playing
		//!\return true if sound is playing, false otherwise
		bool IsPlaying();
		//!Set sound volume
		//!\param v Sound volume (0-100)
		void SetVolume(int v);
		//!Set the sound to loop playback (only applies to OGG)
		//!\param l Loop (true to loop)
		void SetLoop(bool l);
	protected:
		const u8 * sound; //!< Pointer to the sound data
		int type; //!< Sound format type (SOUND_PCM or SOUND_OGG)
		s32 length; //!< Length of sound data
		s32 voice; //!< Currently assigned ASND voice channel
		s32 volume; //!< Sound volume (0-100)
		bool loop; //!< Loop sound playback
};

//!Primary Gui class
class GuiElement
{
	public:
		//!Constructor
		GuiElement();
		//!Destructor
		~GuiElement();
		//!Set the element's parent
		//!\param e Pointer to parent element
		void SetParent(GuiElement * e);
		//!Gets the current leftmost coordinate of the element
		//!Considers horizontal alignment, x offset, width, and parent element's GetLeft() / GetWidth() values
		//!\return left coordinate
		int GetLeft();
		//!Gets the current topmost coordinate of the element
		//!Considers vertical alignment, y offset, height, and parent element's GetTop() / GetHeight() values
		//!\return top coordinate
		int GetTop();
		//!Gets the current width of the element. Does not currently consider the scale
		//!\return width
		int GetWidth();
		//!Gets the height of the element. Does not currently consider the scale
		//!\return height
		int GetHeight();
		//!Sets the size (width/height) of the element
		//!\param w Width of element
		//!\param h Height of element
		void SetSize(int w, int h);
		//!Checks whether or not the element is visible
		//!\return true if visible, false otherwise
		bool IsVisible();
		//!Checks whether or not the element is selectable
		//!\return true if selectable, false otherwise
		bool IsSelectable();
		//!Checks whether or not the element is clickable
		//!\return true if clickable, false otherwise
		bool IsClickable();
		//!Sets whether or not the element is selectable
		//!\param s Selectable
		void SetSelectable(bool s);
		//!Sets whether or not the element is clickable
		//!\param c Clickable
		void SetClickable(bool c);
		//!Gets the element's current state
		//!\return state
		int GetState();
		//!Sets the element's alpha value
		//!\param a alpha value
		void SetAlpha(int a);
		//!Gets the element's alpha value
		//!Considers alpha, alphaDyn, and the parent element's GetAlpha() value
		//!\return alpha
		int GetAlpha();
		//!Sets the element's scale
		//!\param s scale (1 is 100%)
		void SetScale(float s);
		//!Gets the element's current scale
		//!Considers scale, scaleDyn, and the parent element's GetScale() value
		float GetScale();
		//!Set a new GuiTrigger for the element
		//!\param t Pointer to GuiTrigger
		void SetTrigger(GuiTrigger * t);
		//!\overload
		//!\param i Index of trigger array to set
		//!\param t Pointer to GuiTrigger
		void SetTrigger(u8 i, GuiTrigger * t);
		//!Checks whether rumble was requested by the element
		//!\return true is rumble was requested, false otherwise
		bool Rumble();
		//!Sets whether or not the element is requesting a rumble event
		//!\param r true if requesting rumble, false if not
		void SetRumble(bool r);
		//!Set an effect for the element
		//!\param e Effect to enable
		//!\param a Amount of the effect (usage varies on effect)
		//!\param t Target amount of the effect (usage varies on effect)
		void SetEffect(int e, int a, int t=0);
		//!Sets an effect to be enabled on wiimote cursor over
		//!\param e Effect to enable
		//!\param a Amount of the effect (usage varies on effect)
		//!\param t Target amount of the effect (usage varies on effect)
		void SetEffectOnOver(int e, int a, int t=0);
		//!Shortcut to SetEffectOnOver(EFFECT_SCALE, 4, 110)
		void SetEffectGrow();
		//!Gets the current element effects
		//!\return element effects
		int GetEffect();
		//!Checks whether the specified coordinates are within the element's boundaries
		//!\param x X coordinate
		//!\param y Y coordinate
		//!\return true if contained within, false otherwise
		bool IsInside(int x, int y);
		//!Sets the element's position
		//!\param x X coordinate
		//!\param y Y coordinate
		void SetPosition(int x, int y);
		//!Updates the element's effects (dynamic values)
		//!Called by Draw(), used for animation purposes
		void UpdateEffects();
		//!Sets a function to called after after Update()
		//!Callback function can be used to response to changes in the state of the element, and/or update the element's attributes
		void SetUpdateCallback(UpdateCallback u);
		//!Checks whether the element is in focus
		//!\return true if element is in focus, false otherwise
		int IsFocused();
		//!Sets the element's visibility
		//!\param v Visibility (true = visible)
		virtual void SetVisible(bool v);
		//!Sets the element's focus
		//!\param v Focus (true = in focus)
		virtual void SetFocus(int f);
		//!Sets the element's state
		//!\param v State (STATE_DEFAULT, STATE_SELECTED, STATE_CLICKED, STATE_DISABLED)
		virtual void SetState(int s);
		//!Resets the element's state to STATE_DEFAULT
		virtual void ResetState();
		//!Gets whether or not the element is in STATE_SELECTED
		//!\return true if selected, false otherwise
		virtual int GetSelected();
		//!Sets the element's alignment respective to its parent element
		//!\param hor Horizontal alignment (ALIGN_LEFT, ALIGN_RIGHT, ALIGN_CENTRE)
		//!\param vert Vertical alignment (ALIGN_TOP, ALIGN_BOTTOM, ALIGN_MIDDLE)
		virtual void SetAlignment(int hor, int vert);
		//!Called constantly to allow the element to respond to the current input data
		//!\param t Pointer to a GuiTrigger, containing the current input data from PAD/WPAD
		virtual void Update(GuiTrigger * t);
		//!Called constantly to redraw the element
		virtual void Draw();
	protected:
		bool visible; //!< Visibility of the element. If false, Draw() is skipped
		int focus; //!< Element focus (-1 = focus disabled, 0 = not focused, 1 = focused)
		int width; //!< Element width
		int height; //!< Element height
		int xoffset; //!< Element X offset
		int yoffset; //!< Element Y offset
		int xoffsetDyn; //!< Element X offset, dynamic (added to xoffset value for animation effects)
		int yoffsetDyn; //!< Element Y offset, dynamic (added to yoffset value for animation effects)
		int alpha; //!< Element alpha value (0-255)
		f32 scale; //!< Element scale (1 = 100%)
		int alphaDyn; //!< Element alpha, dynamic (multiplied by alpha value for blending/fading effects)
		f32 scaleDyn; //!< Element scale, dynamic (multiplied by alpha value for blending/fading effects)
		bool rumble; //!< Wiimote rumble (on/off) - set to on when this element requests a rumble event
		int effects; //!< Currently enabled effect(s). 0 when no effects are enabled
		int effectAmount; //!< Effect amount. Used by different effects for different purposes
		int effectTarget; //!< Effect target amount. Used by different effects for different purposes
		int effectsOver; //!< Effects to enable when wiimote cursor is over this element. Copied to effects variable on over event
		int effectAmountOver; //!< EffectAmount to set when wiimote cursor is over this element
		int effectTargetOver; //!< EffectTarget to set when wiimote cursor is over this element
		int alignmentHor; //!< Horizontal element alignment, respective to parent element (LEFT, RIGHT, CENTRE)
		int alignmentVert; //!< Horizontal element alignment, respective to parent element (TOP, BOTTOM, MIDDLE)
		int state; //!< Element state (DEFAULT, SELECTED, CLICKED, DISABLED)
		bool selectable; //!< Whether or not this element selectable (can change to SELECTED state)
		bool clickable; //!< Whether or not this element is clickable (can change to CLICKED state)
		GuiTrigger * trigger[2]; //!< GuiTriggers (input actions) that this element responds to
		GuiElement * parentElement; //!< Parent element
		UpdateCallback updateCB; //!< Callback function to call when this element is updated
};

//!Allows GuiElements to be grouped together into a "window"
class GuiWindow : public GuiElement
{
	public:
		//!Constructor
		GuiWindow();
		//!\overload
		//!\param w Width of window
		//!\param h Height of window
		GuiWindow(int w, int h);
		//!Destructor
		~GuiWindow();
		//!Appends a GuiElement to the GuiWindow
		//!\param e The GuiElement to append. If it is already in the GuiWindow, it is removed first
		void Append(GuiElement* e);
		//!Inserts a GuiElement into the GuiWindow at the specified index
		//!\param e The GuiElement to insert. If it is already in the GuiWindow, it is removed first
		//!\param i Index in which to insert the element
		void Insert(GuiElement* e, u32 i);
		//!Removes the specified GuiElement from the GuiWindow
		//!\param e GuiElement to be removed
		void Remove(GuiElement* e);
		//!Removes all GuiElements
		void RemoveAll();
		//!Returns the GuiElement at the specified index
		//!\param index The index of the element
		//!\return A pointer to the element at the index, NULL on error (eg: out of bounds)
		GuiElement* GetGuiElementAt(u32 index) const;
		//!Returns the size of the list of elements
		//!\return The size of the current element list
		u32 GetSize();
		//!Sets the visibility of the window
		//!\param v visibility (true = visible)
		void SetVisible(bool v);
		//!Resets the window's state to STATE_DEFAULT
		void ResetState();
		//!Sets the window's state
		//!\param s State
		void SetState(int s);
		//!Gets the index of the GuiElement inside the window that is currently selected
		//!\return index of selected GuiElement
		int GetSelected();
		//!Sets the window focus
		//!\param f Focus
		void SetFocus(int f);
		//!Change the focus to the specified element
		//!This is intended for the primary GuiWindow only
		//!\param e GuiElement that should have focus
		void ChangeFocus(GuiElement * e);
		//!Changes window focus to the next focusable window or element
		//!If no element is in focus, changes focus to the first available element
		//!If B or 1 button is pressed, changes focus to the next available element
		//!This is intended for the primary GuiWindow only
		//!\param t Pointer to a GuiTrigger, containing the current input data from PAD/WPAD
		void ToggleFocus(GuiTrigger * t);
		//!Moves the selected element to the element to the left or right
		//!\param d Direction to move (-1 = left, 1 = right)
		void MoveSelectionHor(int d);
		//!Moves the selected element to the element above or below
		//!\param d Direction to move (-1 = up, 1 = down)
		void MoveSelectionVert(int d);
		//!Draws all the elements in this GuiWindow
		void Draw();
		//!Updates the window and all elements contains within
		//!Allows the GuiWindow and all elements to respond to the input data specified
		//!\param t Pointer to a GuiTrigger, containing the current input data from PAD/WPAD
		void Update(GuiTrigger * t);
	protected:
		std::vector<GuiElement*> _elements; //!< Contains all elements within the GuiWindow
};

//!Converts image data into GX-useable RGBA8
//!Currently designed for use only with PNG files
class GuiImageData
{
	public:
		//!Constructor
		//!Converts the image data to RGBA8 - expects PNG format
		//!\param i Image data
		GuiImageData(const u8 * i);
		//!Destructor
		~GuiImageData();
		//!Gets a pointer to the image data
		//!\return pointer to image data
		u8 * GetImage();
		//!Gets the image width
		//!\return image width
		int GetWidth();
		//!Gets the image height
		//!\return image height
		int GetHeight();
	protected:
		u8 * data; //!< Image data
		int height; //!< Height of image
		int width; //!< Width of image
};

//!Display, manage, and manipulate images in the Gui
class GuiImage : public GuiElement
{
	public:
		//!Constructor
		//!\param img Pointer to GuiImageData element
		GuiImage(GuiImageData * img);
		//!\overload
		//!Sets up a new image from the image data specified
		//!\param img
		//!\param w Image width
		//!\param h Image height
		GuiImage(u8 * img, int w, int h);
		//!\overload
		//!Creates an image filled with the specified color
		//!\param w Image width
		//!\param h Image height
		//!\param c Image color
		GuiImage(int w, int h, GXColor c);
		//!Destructor
		~GuiImage();
		//!Sets the image rotation angle for drawing
		//!\param a Angle (in degrees)
		void SetAngle(float a);
		//!Sets the number of times to draw the image horizontally
		//!\param t Number of times to draw the image
		void SetTile(int t);
		//!Constantly called to draw the image
		void Draw();
		//!Gets the image data
		//!\return pointer to image data
		u8 * GetImage();
		//!Sets up a new image using the GuiImageData object specified
		//!\param img Pointer to GuiImageData object
		void SetImage(GuiImageData * img);
		//!\overload
		//!\param img Pointer to image data
		//!\param w Width
		//!\param h Height
		void SetImage(u8 * img, int w, int h);
		//!Gets the pixel color at the specified coordinates of the image
		//!\param x X coordinate
		//!\param y Y coordinate
		GXColor GetPixel(int x, int y);
		//!Sets the pixel color at the specified coordinates of the image
		//!\param x X coordinate
		//!\param y Y coordinate
		//!\param color Pixel color
		void SetPixel(int x, int y, GXColor color);
		//!Directly modifies the image data to create a color-striped effect
		//!Alters the RGB values by the specified amount
		//!\param s Amount to increment/decrement the RGB values in the image
		void ColorStripe(int s);
		//!Sets a stripe effect on the image, overlaying alpha blended rectangles
		//!Does not alter the image data
		//!\param s Alpha amount to draw over the image
		void SetStripe(int s);
	protected:
		int imgType; //!< Type of image data (IMAGE_TEXTURE, IMAGE_COLOR, IMAGE_DATA)
		u8 * image; //!< Poiner to image data. May be shared with GuiImageData data
		f32 imageangle; //!< Angle to draw the image
		int tile; //!< Number of times to draw (tile) the image horizontally
		int stripe; //!< Alpha value (0-255) to apply a stripe effect to the texture
};

//!Display, manage, and manipulate text in the Gui
class GuiText : public GuiElement
{
	public:
		//!Constructor
		//!\param t Text
		//!\param s Font size
		//!\param c Font color
		GuiText(const char * t, int s, GXColor c);
		//!\overload
		//!\Assumes SetPresets() has been called to setup preferred text attributes
		//!\param t Text
		GuiText(const char * t);
		//!Destructor
		~GuiText();
		//!Sets the text of the GuiText element
		//!\param t Text
		void SetText(const char * t);
		//!Sets up preset values to be used by GuiText(t)
		//!Useful when printing multiple text elements, all with the same attributes set
		//!\param sz Font size
		//!\param c Font color
		//!\param w Maximum width of texture image (for text wrapping)
		//!\param s Font size
		//!\param h Text alignment (horizontal)
		//!\param v Text alignment (vertical)
		void SetPresets(int sz, GXColor c, int w, u16 s, int h, int v);
		//!Sets the font size
		//!\param s Font size
		void SetFontSize(int s);
		//!Sets the maximum width of the drawn texture image
		//!If the text exceeds this, it is wrapped to the next line
		//!\param w Maximum width
		void SetMaxWidth(int w);
		//!Sets the font color
		//!\param c Font color
		void SetColor(GXColor c);
		//!Sets the FreeTypeGX style attributes
		//!\param s Style attributes
		void SetStyle(u16 s);
		//!Sets the text alignment
		//!\param hor Horizontal alignment (ALIGN_LEFT, ALIGN_RIGHT, ALIGN_CENTRE)
		//!\param vert Vertical alignment (ALIGN_TOP, ALIGN_BOTTOM, ALIGN_MIDDLE)
		void SetAlignment(int hor, int vert);
		//!Constantly called to draw the text
		void Draw();
	protected:
		wchar_t* text; //!< Unicode text value
		int size; //!< Font size
		int maxWidth; //!< Maximum width of the generated text object (for text wrapping)
		u16 style; //!< FreeTypeGX style attributes
		GXColor color; //!< Font color
};

//!Display, manage, and manipulate buttons in the Gui
//!Buttons can have images, icons, text, and sound set (all of which are optional)
class GuiButton : public GuiElement
{
	public:
		//!Constructor
		//!\param w Width
		//!\param h Height
		GuiButton(int w, int h);
		//!Destructor
		~GuiButton();
		//!Sets the button's image
		//!\param i Pointer to GuiImage object
		void SetImage(GuiImage* i);
		//!Sets the button's image on over
		//!\param i Pointer to GuiImage object
		void SetImageOver(GuiImage* i);
		//!Sets the button's icon
		//!\param i Pointer to GuiImage object
		void SetIcon(GuiImage* i);
		//!Sets the button's icon on over
		//!\param i Pointer to GuiImage object
		void SetIconOver(GuiImage* i);
		//!Sets the button's label
		//!\param t Pointer to GuiText object
		void SetLabel(GuiText* t);
		//!\overload
		//!\param t Pointer to GuiText object
		//!\param n Index of label to set
		void SetLabel(GuiText* t, int n);
		//!Sets the button's label on over (eg: different colored text)
		//!\param t Pointer to GuiText object
		void SetLabelOver(GuiText* t);
		//!\overload
		//!\param t Pointer to GuiText object
		//!\param n Index of label to set
		void SetLabelOver(GuiText* t, int n);
		//!Sets the sound to play on over
		//!\param s Pointer to GuiSound object
		void SetSoundOver(GuiSound * s);
		//!Sets the sound to play on click
		//!\param s Pointer to GuiSound object
		void SetSoundClick(GuiSound * s);
		//!Constantly called to draw the GuiButton
		void Draw();
		//!Constantly called to allow the GuiButton to respond to updated input data
		//!\param t Pointer to a GuiTrigger, containing the current input data from PAD/WPAD
		void Update(GuiTrigger * t);
	protected:
		GuiImage * image; //!< Button image
		GuiImage * imageOver; //!< Button image on wiimote cursor over
		GuiImage * icon; //!< Button icon (drawn after button image)
		GuiImage * iconOver; //!< Button icon on wiimote cursor over
		GuiText * label[3]; //!< Label(s) to display
		GuiText * labelOver[3]; //!< Label(s) to display on wiimote cursor over
		GuiSound * soundOver; //!< Sound to play on wiimote cursor over
		GuiSound * soundClick; //!< Sound to play on click
};


typedef struct _keytype {
	char ch, chShift;
} Key;

//!On-screen keyboard
class GuiKeyboard : public GuiWindow
{
	public:
		GuiKeyboard(char * t, u16 m);
		~GuiKeyboard();
		void Update(GuiTrigger * t);
		char kbtextstr[100];
	protected:
		u16 kbtextmaxlen;
		Key keys[4][10];
		int shift;
		int caps;
		GuiText * kbText;
		GuiImage * keyTextboxImg;
		GuiText * keyCapsText;
		GuiImage * keyCapsImg;
		GuiImage * keyCapsOverImg;
		GuiButton * keyCaps;
		GuiText * keyShiftText;
		GuiImage * keyShiftImg;
		GuiImage * keyShiftOverImg;
		GuiButton * keyShift;
		GuiText * keyBackText;
		GuiImage * keyBackImg;
		GuiImage * keyBackOverImg;
		GuiButton * keyBack;
		GuiImage * keySpaceImg;
		GuiImage * keySpaceOverImg;
		GuiButton * keySpace;
		GuiButton * keyBtn[4][10];
		GuiImage * keyImg[4][10];
		GuiImage * keyImgOver[4][10];
		GuiText * keyTxt[4][10];
		GuiImageData * keyTextbox;
		GuiImageData * key;
		GuiImageData * keyOver;
		GuiImageData * keyMedium;
		GuiImageData * keyMediumOver;
		GuiImageData * keyLarge;
		GuiImageData * keyLargeOver;
		GuiSound * keySoundOver;
		GuiTrigger * trigA;
};

typedef struct _optionlist {
	int length;
	char name[MAX_OPTIONS][150];
	char value[MAX_OPTIONS][150];
} OptionList;

//!Display a list of menu options
class GuiOptionBrowser : public GuiElement
{
	public:
		GuiOptionBrowser(int w, int h, OptionList * l);
		~GuiOptionBrowser();
		void SetCol2Position(int x);
		int FindMenuItem(int c, int d);
		int GetClickedOption();
		void ResetState();
		void SetFocus(int f);
		void Draw();
		void Update(GuiTrigger * t);
		GuiText * optionVal[PAGESIZE];
	protected:
		int selectedItem;
		int listOffset;

		OptionList * options;
		int optionIndex[PAGESIZE];
		GuiButton * optionBtn[PAGESIZE];
		GuiText * optionTxt[PAGESIZE];
		GuiImage * optionBg[PAGESIZE];

		GuiButton * arrowUpBtn;
		GuiButton * arrowDownBtn;
		GuiButton * scrollbarBoxBtn;

		GuiImage * bgOptionsImg;
		GuiImage * scrollbarImg;
		GuiImage * arrowDownImg;
		GuiImage * arrowDownOverImg;
		GuiImage * arrowUpImg;
		GuiImage * arrowUpOverImg;
		GuiImage * scrollbarBoxImg;
		GuiImage * scrollbarBoxOverImg;

		GuiImageData * bgOptions;
		GuiImageData * bgOptionsEntry;
		GuiImageData * scrollbar;
		GuiImageData * arrowDown;
		GuiImageData * arrowDownOver;
		GuiImageData * arrowUp;
		GuiImageData * arrowUpOver;
		GuiImageData * scrollbarBox;
		GuiImageData * scrollbarBoxOver;

		GuiTrigger * trigA;
};

class ListBoxItem {
private:
	GuiText *text;
	GuiImageData *iconData;
	GuiImage *icon;
public:
	ListBoxItem(const char* t, const u8 * img = NULL)
	{
		iconData = NULL;
		icon = NULL;
		text = new GuiText(t, 22, (GXColor){0, 0, 0, 0xff});
		text->SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);
		text->SetPosition(5,0);
		if (img) {
			iconData = new GuiImageData(img);
			icon = new GuiImage(iconData);
		}
	}
	~ListBoxItem()
	{
		delete text;
		delete iconData;
		delete icon;
	}
	GuiText* GetText()
	{
		return text;
	}
	GuiImage* GetIcon()
	{
		return icon;
	}
};

//!Display a listbox
class GuiListBox : public GuiElement
{
	public:
		GuiListBox(int w, int h);
		~GuiListBox();
		void ResetState();
		void SetFocus(int f);
		void Draw();
		void TriggerUpdate();
		void Update(GuiTrigger * t);
		void SetClickable(bool c);
		int GetSelectedItem();
		void Clear();
		void AddItem(const char * text);
		void AddItem(const char * text, const u8* icon);
		void RemoveItem(unsigned int item);
		GuiButton * list[PAGESIZE];
	protected:
		std::vector<ListBoxItem*> _items;
		unsigned int selectedItem;
		unsigned int pageIndex;
		unsigned int pageItem;
		bool listChanged;

		GuiButton * arrowUpBtn;
		GuiButton * arrowDownBtn;
		GuiButton * scrollbarBoxBtn;

		GuiImage * listBg[PAGESIZE];
		GuiImage * bgListSelectionImg;
		GuiImage * scrollbarImg;
		GuiImage * arrowDownImg;
		GuiImage * arrowDownOverImg;
		GuiImage * arrowUpImg;
		GuiImage * arrowUpOverImg;
		GuiImage * scrollbarBoxImg;
		GuiImage * scrollbarBoxOverImg;

		GuiImageData * bgListSelection;
		GuiImageData * bgListSelectionEntry;
		GuiImageData * scrollbar;
		GuiImageData * arrowDown;
		GuiImageData * arrowDownOver;
		GuiImageData * arrowUp;
		GuiImageData * arrowUpOver;
		GuiImageData * scrollbarBox;
		GuiImageData * scrollbarBoxOver;

		GuiSound * btnSoundOver;
		GuiTrigger * trigA;
};
#endif
