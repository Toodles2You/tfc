/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// menu.cpp
//
// generic menu handler
//
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

#include "vgui_TeamFortressViewport.h"

#define MAX_MENU_STRING 512
char g_szMenuString[MAX_MENU_STRING];
char g_szPrelocalisedMenuString[MAX_MENU_STRING];

bool KB_ConvertString(char* in, char** ppout);

DECLARE_MESSAGE(m_Menu, ShowMenu);
DECLARE_MESSAGE(m_Menu, VoteMenu);

bool CHudMenu::Init()
{
	HOOK_MESSAGE(ShowMenu);
	HOOK_MESSAGE(VoteMenu);

	return CHudBase::Init();
}

void CHudMenu::Reset()
{
	m_fMenuDisplayed = kNone;
	m_bitsValidSlots = 0;
	g_szPrelocalisedMenuString[0] = 0;
	m_fWaitingForMore = false;
}

void CHudMenu::VidInit()
{
	Reset();
}


/*=================================
  ParseEscapeToken

  Interprets the given escape token (backslash followed by a letter). The
  first character of the token must be a backslash.  The second character
  specifies the operation to perform:

   \w : White text (this is the default)
   \d : Dim (gray) text
   \y : Yellow text
   \r : Red text
   \R : Right-align (just for the remainder of the current line)
=================================*/

static int menu_r, menu_g, menu_b, menu_x;
static bool menu_ralign;

static inline const char* ParseEscapeToken(const char* token)
{
	if (*token != '\\')
		return token;

	token++;

	switch (*token)
	{
	case '\0':
		return token;

	case 'w':
		gHUD.GetColor(menu_r, menu_g, menu_b, CHud::COLOR_PRIMARY);
		break;

	case 'd':
		menu_r = 100;
		menu_g = 100;
		menu_b = 100;
		break;

	case 'y':
		menu_r = 255;
		menu_g = 210;
		menu_b = 64;
		break;

	case 'r':
		menu_r = 210;
		menu_g = 24;
		menu_b = 0;
		break;

	case 'R':
		menu_x = gHUD.GetWidth() / 2;
		menu_ralign = true;
		break;
	}

	return ++token;
}


void CHudMenu::Draw(const float time)
{
	// check for if menu is set to disappear
	if (m_flShutoffTime > 0)
	{
		if (m_flShutoffTime <= gHUD.m_flTime)
		{ // times up, shutoff
			m_fMenuDisplayed = kNone;
			SetActive(false);
			return;
		}
	}

	// don't draw the menu if the scoreboard is being shown
	if (gViewPort != nullptr
	 && (gViewPort->IsScoreBoardVisible()
	  || gViewPort->m_pCurrentMenu != nullptr
	  || gViewPort->m_pCurrentCommandMenu != nullptr))
	{
		return;
	}

	// draw the menu, along the left-hand side of the screen

	// count the number of newlines
	int nlc = 0;
	int i;
	for (i = 0; i < MAX_MENU_STRING && g_szMenuString[i] != '\0'; i++)
	{
		if (g_szMenuString[i] == '\n')
			nlc++;
	}

	// center it
	int y = (gHUD.GetHeight() / 2) - ((nlc / 2) * 12) - 40; // make sure it is above the say text

	gHUD.GetColor(menu_r, menu_g, menu_b, CHud::COLOR_PRIMARY);
	menu_x = 20;
	menu_ralign = false;

	const char* sptr = g_szMenuString;

	while (*sptr != '\0')
	{
		if (*sptr == '\\')
		{
			sptr = ParseEscapeToken(sptr);
		}
		else if (*sptr == '\n')
		{
			menu_ralign = false;
			menu_x = 20;
			y += (12);

			sptr++;
		}
		else
		{
			char menubuf[80];
			const char* ptr = sptr;
			while (*sptr != '\0' && *sptr != '\n' && *sptr != '\\')
			{
				sptr++;
			}
			strncpy(menubuf, ptr, std::min((sptr - ptr), (int)sizeof(menubuf)));
			menubuf[std::min((sptr - ptr), (int)(sizeof(menubuf) - 1))] = '\0';

			if (menu_ralign)
			{
				// IMPORTANT: Right-to-left rendered text does not parse escape tokens!
				menu_x = gHUD.DrawHudStringReverse(menu_x, y, 0, menubuf, menu_r, menu_g, menu_b);
			}
			else
			{
				menu_x = gHUD.DrawHudString(menu_x, y, 320, menubuf, menu_r, menu_g, menu_b);
			}
		}
	}

	return true;
}

// selects an item from the menu
void CHudMenu::SelectMenuItem(int menu_item)
{
	// if menu_item is in a valid slot,  send a menuselect command to the server
	if ((menu_item > 0) && (m_bitsValidSlots & (1 << (menu_item - 1))) != 0)
	{
		char szbuf[32];
		sprintf(szbuf, "menuselect %d\n", menu_item);
		client::ClientCmd(szbuf);

		// remove the menu
		m_fMenuDisplayed = kNone;
		SetActive(false);
	}
}


// Message handler for ShowMenu message
// takes four values:
//		short: a bitfield of keys that are valid input
//		char : the duration, in seconds, the menu should stay up. -1 means is stays until something is chosen.
//		byte : a boolean, true if there is more string yet to be received before displaying the menu, false if it's the last string
//		string: menu string to display
// if this message is never received, then scores will simply be the combined totals of the players.
bool CHudMenu::MsgFunc_ShowMenu(const char* pszName, int iSize, void* pbuf)
{
	char* temp = nullptr;

	BEGIN_READ(pbuf, iSize);

	m_bitsValidSlots = READ_SHORT();
	int DisplayTime = READ_CHAR();
	bool NeedMore = READ_BYTE() != 0;

	if (DisplayTime > 0)
		m_flShutoffTime = DisplayTime + gHUD.m_flTime;
	else
		m_flShutoffTime = -1;

	if (0 != m_bitsValidSlots)
	{
		if (!m_fWaitingForMore) // this is the start of a new menu
		{
			strncpy(g_szPrelocalisedMenuString, READ_STRING(), MAX_MENU_STRING);
		}
		else
		{ // append to the current menu string
			strncat(g_szPrelocalisedMenuString, READ_STRING(), MAX_MENU_STRING - strlen(g_szPrelocalisedMenuString));
		}
		g_szPrelocalisedMenuString[MAX_MENU_STRING - 1] = 0; // ensure null termination (strncat/strncpy does not)

		if (!NeedMore)
		{ // we have the whole string, so we can localise it now
			strcpy(g_szMenuString, gHUD.m_TextMessage.BufferedLocaliseTextString(g_szPrelocalisedMenuString));

			// Swap in characters
			if (KB_ConvertString(g_szMenuString, &temp))
			{
				strcpy(g_szMenuString, temp);
				free(temp);
			}
		}

		m_fMenuDisplayed = kMenu;
		SetActive(true);
	}
	else
	{
		m_fMenuDisplayed = kNone; // no valid slots means that the menu should be turned off
		SetActive(false);
	}

	m_fWaitingForMore = NeedMore;

	return true;
}

bool CHudMenu::MsgFunc_VoteMenu(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	m_fWaitingForMore = false;

	int numOptions = READ_BYTE();
	if (numOptions == 0)
	{
		m_fMenuDisplayed = kNone;
		SetActive(false);
		return true;
	}

	int displayTime = READ_BYTE();
	if (displayTime > 0)
	{
		m_flShutoffTime = gHUD.m_flTime + displayTime;
	}
	else
	{
		m_flShutoffTime = -1;
	}

	char* str = READ_STRING();
	if (str[0] == '#')
	{
		str = gHUD.m_TextMessage.BufferedLocaliseTextString(str);
	}

	strncpy(g_szMenuString, str, MAX_MENU_STRING);

	char keyName[8];
	m_bitsValidSlots = 0;
	for (int i = 0; i < numOptions; i++)
	{
		m_bitsValidSlots |= 1 << i;

		str = READ_STRING();
		if (str[0] == '#')
		{
			str = gHUD.m_TextMessage.BufferedLocaliseTextString(str);
		}

		sprintf(keyName, "\nF%i. ", i + 1);
		strncat(g_szMenuString, keyName, MAX_MENU_STRING - strlen(g_szMenuString));
		strncat(g_szMenuString, str, MAX_MENU_STRING - strlen(g_szMenuString));
	}

	g_szMenuString[MAX_MENU_STRING - 1] = '\0';

	m_fMenuDisplayed = kVote;
	SetActive(true);
	return true;
}
