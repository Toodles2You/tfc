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
	char szAccomplice[MAX_PLAYER_NAME_LENGTH * 2];
	char szVictim[MAX_PLAYER_NAME_LENGTH * 2];
	int iId; // the index number of the associated sprite
	bool bSuicide;
	bool bTeamKill;
	bool bHeadshot;
	bool bLocalPlayerInvolved;
	float flDisplayTime;
	float* KillerColor;
	float* AccompliceColor;
	float* VictimColor;
};

#define MAX_DEATHNOTICES 4

#define DEATHNOTICE_TOP 64

DeathNoticeItem rgDeathNoticeList[MAX_DEATHNOTICES + 1];

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

	DeathNoticeItem* item;
	for (int i = 0; i < MAX_DEATHNOTICES; i++)
	{
		item = rgDeathNoticeList + i;

		if (item->iId == 0)
			break; // we've gone through them all
		
		auto deltaTime = flTime - item->flDisplayTime;
		auto displayTime = hud_deathnotice_time->value;

		if (item->bLocalPlayerInvolved)
		{
			displayTime *= 2;
		}

		if (deltaTime >= displayTime)
		{ // display time has expired
			// remove the current item from the list
			memmove(rgDeathNoticeList + i, rgDeathNoticeList + i + 1, sizeof(DeathNoticeItem) * (MAX_DEATHNOTICES - i));
			i--; // continue on the next item;  stop the counter getting incremented
			continue;
		}

		// Draw the death notice
		y = DEATHNOTICE_TOP + 2 + (20 * i);

		int id = (item->iId == -1) ? m_HUD_d_skull : item->iId;

		auto rect = gHUD.GetSpriteRect(id);
		int weaponWidth = rect.right - rect.left;
		int weaponHeight = rect.bottom - rect.top;

#if 0
		if (item->KillerColor)
		{
			r = 255 * item->KillerColor[0];
			g = 255 * item->KillerColor[1];
			b = 255 * item->KillerColor[2];
		}
		else
#endif
		{
			r = 255;
			g = 80;
			b = 0;
		}
		
		int w = gHUD.HudStringLen(item->szVictim) + weaponWidth;

		if (item->bHeadshot)
		{
			w += m_headshot_width;
		}

		if (!item->bSuicide)
		{
			w +=
				gHUD.HudStringLen(item->szKiller)
				+ gHUD.HudStringLen(" + ")
				+ gHUD.HudStringLen(item->szAccomplice);
		}
		else if (item->szAccomplice[0] != '\0')
		{
			w +=
				gHUD.HudStringLen(item->szAccomplice)
				+ gHUD.HudStringLen(" finished off ");
		}

		x = gHUD.GetWidth() - 6 - w;

		if (item->bLocalPlayerInvolved)
		{
			gHUD.DrawHudFill(
				x - 2,
				y - 2,
				w + 4,
				weaponHeight + 4,
				CHud::COLOR_WARNING,
				MIN_ALPHA * (1.0F - (deltaTime / displayTime)));
		}

		if (!item->bSuicide)
		{
			// Draw killers name
			if (item->KillerColor)
			{
				gEngfuncs.pfnDrawSetTextColor(
					item->KillerColor[0],
					item->KillerColor[1],
					item->KillerColor[2]);
			}
			x = gHUD.DrawHudString(item->szKiller, x, y);

			// Draw accomplices name, if any
			if (item->szAccomplice[0] != '\0')
			{
				gEngfuncs.pfnDrawSetTextColor(r / 255.0F, g / 255.0F, b / 255.0F);
				x = gHUD.DrawHudString(" + ", x, y);

				if (item->AccompliceColor)
				{
					gEngfuncs.pfnDrawSetTextColor(
						item->AccompliceColor[0],
						item->AccompliceColor[1],
						item->AccompliceColor[2]);
				}
				x = gHUD.DrawHudString(item->szAccomplice, x, y);
			}
		}

		// Draw death weapon
		gHUD.DrawHudSpriteIndex(id, x, y, r, g, b);
		x += weaponWidth;

		if (item->bHeadshot)
		{
			gHUD.DrawHudSpriteIndex(m_HUD_d_headshot, x, y, r, g, b);
			x += m_headshot_width;
		}

		// Someone attacked the player before they fell or something
		if (item->bSuicide
		 && item->szAccomplice[0] != '\0')
		{
			if (item->AccompliceColor)
			{
				gEngfuncs.pfnDrawSetTextColor(
					item->AccompliceColor[0],
					item->AccompliceColor[1],
					item->AccompliceColor[2]);
			}
			x = gHUD.DrawHudString(item->szAccomplice, x, y);

			gEngfuncs.pfnDrawSetTextColor(r / 255.0F, g / 255.0F, b / 255.0F);
			x = gHUD.DrawHudString(" finished off ", x, y);
		}

		// Draw victims name
		if (item->VictimColor)
		{
			gEngfuncs.pfnDrawSetTextColor(
				item->VictimColor[0],
				item->VictimColor[1],
				item->VictimColor[2]);
		}
		x = gHUD.DrawHudString(item->szVictim, x, y);
	}

	return true;
}

// This message handler may be better off elsewhere
bool CHudDeathNotice::MsgFunc_DeathMsg(const char* pszName, int iSize, void* pbuf)
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ(pbuf, iSize);

	int killer = READ_BYTE();
	int accomplice = READ_BYTE();
	int victim = READ_BYTE();
	int flags = READ_BYTE();
	
	auto localPlayerInvolved =
		g_PlayerInfoList[killer].thisplayer || g_PlayerInfoList[victim].thisplayer;

	if (accomplice != 0)
	{
		localPlayerInvolved |= g_PlayerInfoList[accomplice].thisplayer;
	}

	char killedwith[32];
	strcpy(killedwith, "d_");
	strncat(killedwith, READ_STRING(), 32);

	gHUD.m_Spectator.DeathMessage(victim);
	int i, j;
	DeathNoticeItem* item;
	for (i = 0; i < MAX_DEATHNOTICES; i++)
	{
		item = rgDeathNoticeList + i;
		if (item->iId == 0)
			break;
	}
	if (i == MAX_DEATHNOTICES)
	{
		/* Move the rest of the list forward to make room for this item */
		if (localPlayerInvolved)
		{
			/* See if we can find a less important one to discard */
			for (int j = 0; j < MAX_DEATHNOTICES; j++)
			{
				if (rgDeathNoticeList[j].bLocalPlayerInvolved)
				{
					continue;
				}
				memmove(
					rgDeathNoticeList + j,
					rgDeathNoticeList + j + 1,
					sizeof(DeathNoticeItem) * (MAX_DEATHNOTICES - j));
				i = MAX_DEATHNOTICES - 1;
			}
		}

		if (i == MAX_DEATHNOTICES)
		{
			memmove(
				rgDeathNoticeList,
				rgDeathNoticeList + 1,
				sizeof(DeathNoticeItem) * MAX_DEATHNOTICES);
			i = MAX_DEATHNOTICES - 1;
		}
	}

	if (gViewPort)
		gViewPort->GetAllPlayersInfo();

	// Get the Killer's name
	const char* killerName = g_PlayerInfoList[killer].name;
	if (!killerName)
	{
		item->szKiller[0] = '\0';
	}
	else
	{
		item->KillerColor = gHUD.GetClientColor(killer);
		strncpy(item->szKiller, killerName, MAX_PLAYER_NAME_LENGTH);
		item->szKiller[MAX_PLAYER_NAME_LENGTH - 1] = '\0';
	}

	if (accomplice != 0)
	{
		const char* accompliceName = g_PlayerInfoList[accomplice].name;
		if (!accompliceName)
		{
			item->szAccomplice[0] = '\0';
		}
		else
		{
			item->AccompliceColor = gHUD.GetClientColor(accomplice);
			strncpy(item->szAccomplice, accompliceName, MAX_PLAYER_NAME_LENGTH);
			item->szAccomplice[MAX_PLAYER_NAME_LENGTH - 1] = '\0';
		}
	}
	else
	{
		item->szAccomplice[0] = '\0';
	}

	// Get the Victim's name
	const char* victimName = g_PlayerInfoList[victim].name;
	if (!victimName)
	{
		item->szVictim[0] = '\0';
	}
	else
	{
		item->VictimColor = gHUD.GetClientColor(victim);
		strncpy(item->szVictim, victimName, MAX_PLAYER_NAME_LENGTH);
		item->szVictim[MAX_PLAYER_NAME_LENGTH - 1] = '\0';
	}

	if (killer == victim || killer == 0)
	{
		item->bSuicide = true;
	}
	else if ((flags & kDamageFlagFriendlyFire) != 0)
	{
		item->bTeamKill = true;
	}

	if ((flags & kDamageFlagHeadshot) != 0)
	{
		item->bHeadshot = true;
	}

	// Find the sprite in the list
	int spr = gHUD.GetSpriteIndex(killedwith);

	item->iId = spr;
	item->flDisplayTime = gHUD.m_flTime;
	item->bLocalPlayerInvolved = localPlayerInvolved;

#if 0
	PrintDeathMsg(item, killedwith);
#endif

	return true;
}

#if 0
void CHudDeathNotice::PrintDeathMsg(const DeathNoticeItem* item, const char* weapon)
{
	if (item->bSuicide)
	{
		if (item->szAccomplice[0] != '\0')
		{
			ConsolePrint(item->szAccomplice);
			ConsolePrint(" finished off ");
			ConsolePrint(item->szVictim);
		}
		else
		{
			ConsolePrint(item->szVictim);

			if (strcmp(weapon, "d_world") == 0)
			{
				ConsolePrint(" died");
			}
			else
			{
				ConsolePrint(" killed self");
			}
		}
	}
	else
	{
		ConsolePrint(item->szKiller);
		if (item->szAccomplice[0] != '\0')
		{
			ConsolePrint(" + ");
			ConsolePrint(item->szAccomplice);
		}
		if (item->bTeamKill)
		{
			ConsolePrint(" killed their teammate ");
		}
		else
		{
			ConsolePrint(" killed ");
		}
		ConsolePrint(item->szVictim);
	}

	if (weapon[0] != '\0' && strcmp(weapon, "d_world") != 0)
	{
		ConsolePrint(" with ");
		ConsolePrint(weapon + 2);
	}

	ConsolePrint("\n");
}
#endif
