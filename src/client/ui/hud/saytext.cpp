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
// saytext.cpp
//
// implementation of CHudSayText class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

#include "vgui_TeamFortressViewport.h"

#define MAX_LINES 5
#define MAX_CHARS_PER_LINE 256 /* it can be less than this, depending on char size */

// allow 20 pixels on either side of the text
#define MAX_LINE_WIDTH (gHUD.GetWidth() - 40)
#define LINE_START 10
static float SCROLL_SPEED = 5;

static char g_szLineBuffer[MAX_LINES + 1][MAX_CHARS_PER_LINE];
static Vector* g_pflNameColors[MAX_LINES + 1];
static int g_iNameLengths[MAX_LINES + 1];
static float flScrollTime = 0; // the time at which the lines next scroll up

DECLARE_MESSAGE(m_SayText, SayText);

bool CHudSayText::Init()
{
	HOOK_MESSAGE(SayText);

	m_HUD_saytext = gEngfuncs.pfnRegisterVariable("hud_saytext", "1", 0);
	m_HUD_saytext_time = gEngfuncs.pfnRegisterVariable("hud_saytext_time", "5", 0);
	m_con_color = gEngfuncs.pfnGetCvarPointer("con_color");

	int iLineWidth;
	gHUD.GetHudStringSize("0", iLineWidth, m_iLineHeight);

	m_iBaseX = LINE_START;
	m_iBaseY = gHUD.GetHeight() - 60 - m_iLineHeight;

	return CHudBase::Init();
}


void CHudSayText::Reset()
{
	memset(g_szLineBuffer, 0, sizeof g_szLineBuffer);
	memset(g_pflNameColors, 0, sizeof g_pflNameColors);
	memset(g_iNameLengths, 0, sizeof g_iNameLengths);
}

void CHudSayText::VidInit()
{
	int iLineWidth;
	gHUD.GetHudStringSize("0", iLineWidth, m_iLineHeight);

	m_iBaseX = LINE_START;
	m_iBaseY = gHUD.GetHeight() - 60 - m_iLineHeight;
}


int ScrollTextUp()
{
	g_szLineBuffer[MAX_LINES][0] = 0;
	memmove(g_szLineBuffer[0], g_szLineBuffer[1], sizeof(g_szLineBuffer) - sizeof(g_szLineBuffer[0])); // overwrite the first line
	memmove(&g_pflNameColors[0], &g_pflNameColors[1], sizeof(g_pflNameColors) - sizeof(g_pflNameColors[0]));
	memmove(&g_iNameLengths[0], &g_iNameLengths[1], sizeof(g_iNameLengths) - sizeof(g_iNameLengths[0]));
	g_szLineBuffer[MAX_LINES - 1][0] = 0;

	if (g_szLineBuffer[0][0] == ' ') // also scroll up following lines
	{
		g_szLineBuffer[0][0] = 2;
		return 1 + ScrollTextUp();
	}

	return 1;
}

void CHudSayText::Draw(const float time)
{
	int y = m_iBaseY;

	if (0 == m_HUD_saytext->value)
		return;

	// make sure the scrolltime is within reasonable bounds,  to guard against the clock being reset
	flScrollTime = std::min(flScrollTime, time + m_HUD_saytext_time->value);

	if (flScrollTime <= time)
	{
		if ('\0' != *g_szLineBuffer[0])
		{
			flScrollTime = time + m_HUD_saytext_time->value;
			// push the console up
			ScrollTextUp();
		}
		else
		{ // buffer is empty,  just disable drawing of this section
			SetActive(false);
			return;
		}
	}

	//Set text color to con_color cvar value before drawing to ensure consistent color.
	//The engine resets this color to that value after drawing a single string.
	if (int r, g, b; sscanf(m_con_color->string, "%i %i %i", &r, &g, &b) == 3)
	{
		gEngfuncs.pfnDrawSetTextColor(r / 255.0f, g / 255.0f, b / 255.0f);
	}

	char line[MAX_CHARS_PER_LINE]{};

	for (int i = MAX_LINES - 1; i >= 0; i--)
	{
		if ('\0' == *g_szLineBuffer[i])
			continue;

		if (*g_szLineBuffer[i] == 2 && g_pflNameColors[i])
		{
			// it's a saytext string

			//Make a copy we can freely modify
			strncpy(line, g_szLineBuffer[i], sizeof(line) - 1);
			line[sizeof(line) - 1] = '\0';

			int x = m_iBaseX;

			// draw the first x characters in the player color
			const std::size_t playerNameEndIndex = std::min(g_iNameLengths[i], MAX_PLAYER_NAME_LENGTH + 31);

			//Cut off the actual text so we can print player name
			line[playerNameEndIndex] = '\0';

			gEngfuncs.pfnDrawSetTextColor(g_pflNameColors[i]->x, g_pflNameColors[i]->y, g_pflNameColors[i]->z);
			x = gHUD.DrawHudString(line + 1, x, y); // don't draw the control code at the start

			//Reset last character
			line[playerNameEndIndex] = g_szLineBuffer[i][playerNameEndIndex];

			// color is reset after each string draw
			//Print the text without player name
			gHUD.DrawHudString(line + g_iNameLengths[i], x, y);
		}
		else
		{
			// normal draw
			gHUD.DrawHudString(g_szLineBuffer[i], m_iBaseX, y);
		}

		y -= m_iLineHeight;
	}
}

bool CHudSayText::MsgFunc_SayText(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int client_index = READ_BYTE(); // the client who spoke the message

	if (client_index > 0 && GetClientVoiceMgr()->IsPlayerBlocked(client_index))
	{
		return true;
	}
	
	bool teamonly = READ_BYTE() != 0;
	char buffer[1024];
	const char* msg;

	if (g_PlayerExtraInfo[client_index].teamnumber == TEAM_SPECTATORS && teamonly)
	{
		msg = "#Chat_spec";
	}
	else if (gHUD.m_gameMode >= kGamemodeTeamplay && teamonly)
	{
		if (!g_PlayerExtraInfo[client_index].dead)
		{
			msg = "#Chat_team";
		}
		else
		{
			msg = "#Chat_team_dead";
		}
	}
	else
	{
		if (!g_PlayerExtraInfo[client_index].dead)
		{
			msg = "#Chat_all";
		}
		else
		{
			msg = "#Chat_all_dead";
		}
	}

	snprintf(
		buffer,
		sizeof(buffer) - 1,
		CHudTextMessage::BufferedLocaliseTextString(msg),
		g_PlayerInfoList[client_index].name,
		READ_STRING());

	buffer[sizeof(buffer) - 1] = '\0';

	SayTextPrint(buffer, client_index);

	return true;
}

void CHudSayText::SayTextPrint(const char* pszBuf, int clientIndex)
{
	const auto hasNewLine =
		pszBuf[std::max((int)strlen(pszBuf) - 1, 0)] == '\n';

	// Print it straight to the console
	ConsolePrint(pszBuf);

	if (!hasNewLine)
	{
		ConsolePrint("\n");
	}

	int i;
	// find an empty string slot
	for (i = 0; i < MAX_LINES; i++)
	{
		if ('\0' == *g_szLineBuffer[i])
			break;
	}
	if (i == MAX_LINES)
	{
		// force scroll buffer up
		ScrollTextUp();
		i = MAX_LINES - 1;
	}

	g_iNameLengths[i] = 0;
	g_pflNameColors[i] = NULL;

	// if it's a say message, search for the players name in the string
	if (*pszBuf == 2 && clientIndex > 0)
	{
		gEngfuncs.pfnGetPlayerInfo(clientIndex, &g_PlayerInfoList[clientIndex]);
		const char* pName = g_PlayerInfoList[clientIndex].name;

		if (pName)
		{
			const char* nameInString = strstr(pszBuf, pName);

			if (nameInString)
			{
				g_iNameLengths[i] = strlen(pName) + (nameInString - pszBuf);
				g_pflNameColors[i] = &const_cast<Vector&>(gHUD.GetClientColor(clientIndex));
			}
		}
	}

	strncpy(g_szLineBuffer[i], pszBuf, MAX_CHARS_PER_LINE);

	if (!hasNewLine)
	{
		strcat(g_szLineBuffer[i], "\n");
	}

	// make sure the text fits in one line
	EnsureTextFitsInOneLineAndWrapIfHaveTo(i);

	// Set scroll time
	if (i == 0)
	{
		flScrollTime = gHUD.m_flTime + m_HUD_saytext_time->value;
	}

	SetActive(true);
	PlaySound("misc/talk.wav", 1);
}

void CHudSayText::EnsureTextFitsInOneLineAndWrapIfHaveTo(int line)
{
	int line_width = 0;
	gHUD.GetHudStringSize(g_szLineBuffer[line], line_width, m_iLineHeight);

	if ((line_width + m_iBaseX) > MAX_LINE_WIDTH)
	{ // string is too long to fit on line
		// scan the string until we find what word is too long,  and wrap the end of the sentence after the word
		int length = m_iBaseX;
		int tmp_len = 0;
		char* last_break = NULL;
		for (char* x = g_szLineBuffer[line]; *x != 0; x++)
		{
			// check for a color change, if so skip past it
			if (x[0] == '/' && x[1] == '(')
			{
				x += 2;
				// skip forward until past mode specifier
				while (*x != 0 && *x != ')')
					x++;

				if (*x != 0)
					x++;

				if (*x == 0)
					break;
			}

			char buf[2];
			buf[1] = 0;

			if (*x == ' ' && x != g_szLineBuffer[line]) // store each line break,  except for the very first character
				last_break = x;

			buf[0] = *x; // get the length of the current character
			gHUD.GetHudStringSize(buf, tmp_len, m_iLineHeight);
			length += tmp_len;

			if (length > MAX_LINE_WIDTH)
			{ // needs to be broken up
				if (!last_break)
					last_break = x - 1;

				x = last_break;

				// find an empty string slot
				int j;
				do
				{
					for (j = 0; j < MAX_LINES; j++)
					{
						if ('\0' == *g_szLineBuffer[j])
							break;
					}
					if (j == MAX_LINES)
					{
						// need to make more room to display text, scroll stuff up then fix the pointers
						int linesmoved = ScrollTextUp();
						line -= linesmoved;
						last_break = last_break - (sizeof(g_szLineBuffer[0]) * linesmoved);
					}
				} while (j == MAX_LINES);

				// copy remaining string into next buffer,  making sure it starts with a space character
				if ((char)*last_break == (char)' ')
				{
					int linelen = strlen(g_szLineBuffer[j]);
					int remaininglen = strlen(last_break);

					if ((linelen - remaininglen) <= MAX_CHARS_PER_LINE)
						strcat(g_szLineBuffer[j], last_break);
				}
				else
				{
					if ((strlen(g_szLineBuffer[j]) - strlen(last_break) - 2) < MAX_CHARS_PER_LINE)
					{
						strcat(g_szLineBuffer[j], " ");
						strcat(g_szLineBuffer[j], last_break);
					}
				}

				*last_break = 0; // cut off the last string

				EnsureTextFitsInOneLineAndWrapIfHaveTo(j);
				break;
			}
		}
	}
}
