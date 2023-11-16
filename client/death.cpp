/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
// death notice
//
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

#include "vgui_TeamFortressViewport.h"

DECLARE_MESSAGE(m_DeathNotice, DeathMsg);

struct DeathNoticeItem
{
	char szKiller[MAX_PLAYER_NAME_LENGTH * 2];
	char szVictim[MAX_PLAYER_NAME_LENGTH * 2];
	int iId; // the index number of the associated sprite
	bool bSuicide;
	bool bTeamKill;
	bool bNonPlayerKill;
	bool bHeadshot;
	bool bLocalPlayerInvolved;
	float flDisplayTime;
	float* KillerColor;
	float* VictimColor;
};

#define MAX_DEATHNOTICES 4

#define DEATHNOTICE_TOP 64

DeathNoticeItem rgDeathNoticeList[MAX_DEATHNOTICES + 1];

float g_ColorBlue[3] = {0.6, 0.8, 1.0};
float g_ColorRed[3] = {1.0, 0.25, 0.25};
float g_ColorGreen[3] = {0.6, 1.0, 0.6};
float g_ColorYellow[3] = {1.0, 0.7, 0.0};
float g_ColorGrey[3] = {0.8, 0.8, 0.8};

float* GetClientColor(int clientIndex)
{
	switch (g_PlayerExtraInfo[clientIndex].teamnumber)
	{
	case 1:
		return g_ColorBlue;
	case 2:
		return g_ColorRed;
	case 3:
		return g_ColorYellow;
	case 4:
		return g_ColorGreen;
	case 0:
		return g_ColorYellow;

	default:
		return g_ColorGrey;
	}

	return NULL;
}

bool CHudDeathNotice::Init()
{
	gHUD.AddHudElem(this);

	HOOK_MESSAGE(DeathMsg);

	hud_deathnotice_time = CVAR_CREATE("hud_deathnotice_time", "6", 0);

	return true;
}


void CHudDeathNotice::InitHUDData()
{
	memset(rgDeathNoticeList, 0, sizeof(rgDeathNoticeList));
}


bool CHudDeathNotice::VidInit()
{
	m_HUD_d_skull = gHUD.GetSpriteIndex("d_skull");
	m_HUD_d_headshot = gHUD.GetSpriteIndex("d_headshot");

	auto rect = gHUD.GetSpriteRect(m_HUD_d_headshot);
	m_headshot_width = rect.right - rect.left;

	return true;
}

bool CHudDeathNotice::Draw(float flTime)
{
	int x, y, r, g, b;

	if (hud_deathnotice_time->value <= 0.0F)
	{
		return true;
	}

	for (int i = 0; i < MAX_DEATHNOTICES; i++)
	{
		if (rgDeathNoticeList[i].iId == 0)
			break; // we've gone through them all
		
		auto deltaTime = flTime - rgDeathNoticeList[i].flDisplayTime;
		auto displayTime = hud_deathnotice_time->value;

		if (rgDeathNoticeList[i].bLocalPlayerInvolved)
		{
			displayTime *= 2;
		}

		if (deltaTime >= displayTime)
		{ // display time has expired
			// remove the current item from the list
			memmove(&rgDeathNoticeList[i], &rgDeathNoticeList[i + 1], sizeof(DeathNoticeItem) * (MAX_DEATHNOTICES - i));
			i--; // continue on the next item;  stop the counter getting incremented
			continue;
		}

		// Draw the death notice
		y = DEATHNOTICE_TOP + 2 + (20 * i); //!!!

		int id = (rgDeathNoticeList[i].iId == -1) ? m_HUD_d_skull : rgDeathNoticeList[i].iId;

		auto rect = gHUD.GetSpriteRect(id);
		int weaponWidth = rect.right - rect.left;
		int weaponHeight = rect.bottom - rect.top;
		
		int w = gHUD.HudStringLen(rgDeathNoticeList[i].szVictim) + weaponWidth;

		if (rgDeathNoticeList[i].bHeadshot)
		{
			w += m_headshot_width;
		}

		if (!rgDeathNoticeList[i].bSuicide)
		{
			w += 5 + gHUD.HudStringLen(rgDeathNoticeList[i].szKiller);
		}

		x = gHUD.GetWidth() - 5 - w;

		if (rgDeathNoticeList[i].bLocalPlayerInvolved)
		{
			gHUD.DrawHudFill(
				x - 2,
				y - 2,
				w + 4,
				weaponHeight + 4,
				CHud::COLOR_WARNING,
				MIN_ALPHA * (1.0F - (deltaTime / displayTime)));
		}

		if (!rgDeathNoticeList[i].bSuicide)
		{
			// Draw killers name
			if (rgDeathNoticeList[i].KillerColor)
				gEngfuncs.pfnDrawSetTextColor(rgDeathNoticeList[i].KillerColor[0], rgDeathNoticeList[i].KillerColor[1], rgDeathNoticeList[i].KillerColor[2]);
			x = 5 + gHUD.DrawHudString(rgDeathNoticeList[i].szKiller, x, y);
		}

		if (rgDeathNoticeList[i].bTeamKill)
		{
			r = 10;
			g = 240;
			b = 10; // display it in sickly green
		}
#if 0
		else if (rgDeathNoticeList[i].KillerColor)
		{
			r = 255 * rgDeathNoticeList[i].KillerColor[0];
			g = 255 * rgDeathNoticeList[i].KillerColor[1];
			b = 255 * rgDeathNoticeList[i].KillerColor[2];
		}
#endif
		else
		{
			r = 255;
			g = 80;
			b = 0;
		}

		// Draw death weapon
		gHUD.DrawHudSpriteIndex(id, x, y, r, g, b);
		x += weaponWidth;

		if (rgDeathNoticeList[i].bHeadshot)
		{
			gHUD.DrawHudSpriteIndex(m_HUD_d_headshot, x, y, r, g, b);
			x += m_headshot_width;
		}

		// Draw victims name (if it was a player that was killed)
		if (!rgDeathNoticeList[i].bNonPlayerKill)
		{
			if (rgDeathNoticeList[i].VictimColor)
				gEngfuncs.pfnDrawSetTextColor(rgDeathNoticeList[i].VictimColor[0], rgDeathNoticeList[i].VictimColor[1], rgDeathNoticeList[i].VictimColor[2]);
			x = gHUD.DrawHudString(rgDeathNoticeList[i].szVictim, x, y);
		}
	}

	return true;
}

// This message handler may be better off elsewhere
bool CHudDeathNotice::MsgFunc_DeathMsg(const char* pszName, int iSize, void* pbuf)
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ(pbuf, iSize);

	int killer = READ_BYTE();
	int victim = READ_BYTE();
	int flags = READ_BYTE();
	
	auto localPlayerInvolved =
		g_PlayerInfoList[killer].thisplayer || g_PlayerInfoList[victim].thisplayer;

	char killedwith[32];
	strcpy(killedwith, "d_");
	strncat(killedwith, READ_STRING(), 32);

	if (gViewPort)
		gViewPort->DeathMsg(killer, victim);

	gHUD.m_Spectator.DeathMessage(victim);
	int i, j;
	for (i = 0; i < MAX_DEATHNOTICES; i++)
	{
		if (rgDeathNoticeList[i].iId == 0)
			break;
	}
	if (i == MAX_DEATHNOTICES)
	{
		// move the rest of the list forward to make room for this item
		memmove(rgDeathNoticeList, rgDeathNoticeList + 1, sizeof(DeathNoticeItem) * MAX_DEATHNOTICES);
		i = MAX_DEATHNOTICES - 1;
	}

	if (gViewPort)
		gViewPort->GetAllPlayersInfo();

	// Get the Killer's name
	const char* killer_name = g_PlayerInfoList[killer].name;
	if (!killer_name)
	{
		killer_name = "";
		rgDeathNoticeList[i].szKiller[0] = 0;
	}
	else
	{
		rgDeathNoticeList[i].KillerColor = GetClientColor(killer);
		strncpy(rgDeathNoticeList[i].szKiller, killer_name, MAX_PLAYER_NAME_LENGTH);
		rgDeathNoticeList[i].szKiller[MAX_PLAYER_NAME_LENGTH - 1] = 0;
	}

	// Get the Victim's name
	const char* victim_name = NULL;
	// If victim is -1, the killer killed a specific, non-player object (like a sentrygun)
	if (((char)victim) != -1)
		victim_name = g_PlayerInfoList[victim].name;
	if (!victim_name)
	{
		victim_name = "";
		rgDeathNoticeList[i].szVictim[0] = 0;
	}
	else
	{
		rgDeathNoticeList[i].VictimColor = GetClientColor(victim);
		strncpy(rgDeathNoticeList[i].szVictim, victim_name, MAX_PLAYER_NAME_LENGTH);
		rgDeathNoticeList[i].szVictim[MAX_PLAYER_NAME_LENGTH - 1] = 0;
	}

	// Is it a non-player object kill?
	if (((char)victim) == -1)
	{
		rgDeathNoticeList[i].bNonPlayerKill = true;

		// Store the object's name in the Victim slot (skip the d_ bit)
		strcpy(rgDeathNoticeList[i].szVictim, killedwith + 2);
	}
	else
	{
		if (killer == victim || killer == 0)
		{
			rgDeathNoticeList[i].bSuicide = true;
		}
		else if ((flags & kDamageFlagFriendlyFire) != 0)
		{
			rgDeathNoticeList[i].bTeamKill = true;
		}
	}

	if ((flags & kDamageFlagHeadshot) != 0)
	{
		rgDeathNoticeList[i].bHeadshot = true;
	}

	// Find the sprite in the list
	int spr = gHUD.GetSpriteIndex(killedwith);

	rgDeathNoticeList[i].iId = spr;
	rgDeathNoticeList[i].flDisplayTime = gHUD.m_flTime;
	rgDeathNoticeList[i].bLocalPlayerInvolved = localPlayerInvolved;

	if (rgDeathNoticeList[i].bNonPlayerKill)
	{
		ConsolePrint(rgDeathNoticeList[i].szKiller);
		ConsolePrint(" killed a ");
		ConsolePrint(rgDeathNoticeList[i].szVictim);
		ConsolePrint("\n");
	}
	else
	{
		// record the death notice in the console
		if (rgDeathNoticeList[i].bSuicide)
		{
			ConsolePrint(rgDeathNoticeList[i].szVictim);

			if (0 == strcmp(killedwith, "d_world"))
			{
				ConsolePrint(" died");
			}
			else
			{
				ConsolePrint(" killed self");
			}
		}
		else if (rgDeathNoticeList[i].bTeamKill)
		{
			ConsolePrint(rgDeathNoticeList[i].szKiller);
			ConsolePrint(" killed their teammate ");
			ConsolePrint(rgDeathNoticeList[i].szVictim);
		}
		else
		{
			ConsolePrint(rgDeathNoticeList[i].szKiller);
			ConsolePrint(" killed ");
			ConsolePrint(rgDeathNoticeList[i].szVictim);
		}

		if (killedwith && '\0' != *killedwith && (*killedwith > 13) && 0 != strcmp(killedwith, "d_world") && !rgDeathNoticeList[i].bTeamKill)
		{
			ConsolePrint(" with ");

			// replace the code names with the 'real' names
			if (0 == strcmp(killedwith + 2, "egon"))
				strcpy(killedwith, "d_gluon gun");
			if (0 == strcmp(killedwith + 2, "gauss"))
				strcpy(killedwith, "d_tau cannon");

			ConsolePrint(killedwith + 2); // skip over the "d_" part
		}

		ConsolePrint("\n");
	}

	return true;
}
