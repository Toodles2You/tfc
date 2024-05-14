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
	const Vector* KillerColor;
	const Vector* AccompliceColor;
	const Vector* VictimColor;
};

#define MAX_DEATHNOTICES 4

#ifdef HALFLIFE_HUD_FLASHLIGHT
#define DEATHNOTICE_TOP 64
#else
#define DEATHNOTICE_TOP 4
#endif

DeathNoticeItem rgDeathNoticeList[MAX_DEATHNOTICES + 1];

bool CHudDeathNotice::Init()
{
	HOOK_MESSAGE(DeathMsg);

	hud_deathnotice_time = client::RegisterVariable("hud_deathnotice_time", "6", 0);

	return CHudBase::Init();
}


void CHudDeathNotice::Reset()
{
	memset(rgDeathNoticeList, 0, sizeof(rgDeathNoticeList));
}


void CHudDeathNotice::VidInit()
{
	m_HUD_d_skull = gHUD.GetSpriteIndex("d_skull");
	m_HUD_d_headshot = gHUD.GetSpriteIndex("d_headshot");

	int string_width;
	gHUD.GetHudStringSize("Player", string_width, m_string_height);

	m_string_height = ceilf(m_string_height / 2.0F);

	auto rect = gHUD.GetSpriteRect(m_HUD_d_headshot);
	m_headshot_width = rect.right - rect.left;
	m_headshot_height = rect.bottom - rect.top;

	m_headshot_height = ceilf(m_headshot_height / 2.0F);
}

void CHudDeathNotice::Draw(const float time)
{
	int r, g, b;

	if (hud_deathnotice_time->value <= 0.0F)
	{
		return;
	}

	auto y = 10;

	for (int i = 0; i < MAX_DEATHNOTICES; i++)
	{
		auto item = rgDeathNoticeList + i;

		if (item->iId == 0)
		{
			break; // we've gone through them all
		}
		
		auto deltaTime = time - item->flDisplayTime;
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

		int id = (item->iId == -1) ? m_HUD_d_skull : item->iId;

		auto rect = gHUD.GetSpriteRect(id);
		int weaponWidth = rect.right - rect.left;
		int weaponHeight = rect.bottom - rect.top;

		auto centerY = y + (weaponHeight >> 1);

		gHUD.GetColor(r, g, b, CHud::COLOR_PRIMARY);
		
		int w = gHUD.HudStringLen(item->szVictim) + 2 + weaponWidth;

		if (item->bHeadshot)
		{
			w += m_headshot_width;
		}

		if (!item->bSuicide)
		{
			w += 2 + gHUD.HudStringLen(item->szKiller);
			
			if (item->szAccomplice[0] != '\0')
			{
				w +=
					gHUD.HudStringLen(" + ")
					+ gHUD.HudStringLen(item->szAccomplice);
			}
		}
		else if (item->szAccomplice[0] != '\0')
		{
			w +=
				gHUD.HudStringLen(item->szAccomplice)
				+ gHUD.HudStringLen(" finished off ");
		}

		auto x = gHUD.GetWidth() - 10;

		gHUD.DrawHudBackground(
			x - w,
			y,
			x,
			y + weaponHeight,
			item->bLocalPlayerInvolved);

		x -= w;

		if (!item->bSuicide)
		{
			// Draw killers name
			if (item->KillerColor)
			{
				client::DrawSetTextColor(
					item->KillerColor->x,
					item->KillerColor->y,
					item->KillerColor->z);
			}
			x = gHUD.DrawHudString(item->szKiller, x, centerY - m_string_height);

			// Draw accomplices name, if any
			if (item->szAccomplice[0] != '\0')
			{
				client::DrawSetTextColor(r / 255.0F, g / 255.0F, b / 255.0F);
				x = gHUD.DrawHudString(" + ", x, centerY - m_string_height);

				if (item->AccompliceColor)
				{
					client::DrawSetTextColor(
						item->AccompliceColor->x,
						item->AccompliceColor->y,
						item->AccompliceColor->z);
				}
				x = gHUD.DrawHudString(item->szAccomplice, x, centerY - m_string_height);
			}

			x += 2;
		}

		// Draw death weapon
		gHUD.DrawHudSpriteIndex(id, x, y, r, g, b);
		x += weaponWidth;

		if (item->bHeadshot)
		{
			gHUD.DrawHudSpriteIndex(m_HUD_d_headshot, x, centerY - m_headshot_height, r, g, b);
			x += m_headshot_width;
		}

		x += 2;

		// Someone attacked the player before they fell or something
		if (item->bSuicide
		 && item->szAccomplice[0] != '\0')
		{
			if (item->AccompliceColor)
			{
				client::DrawSetTextColor(
					item->AccompliceColor->x,
					item->AccompliceColor->y,
					item->AccompliceColor->z);
			}
			x = gHUD.DrawHudString(item->szAccomplice, x, centerY - m_string_height);

			client::DrawSetTextColor(r / 255.0F, g / 255.0F, b / 255.0F);
			x = gHUD.DrawHudString(" finished off ", x, centerY - m_string_height);
		}

		// Draw victims name
		if (item->VictimColor)
		{
			client::DrawSetTextColor(
				item->VictimColor->x,
				item->VictimColor->y,
				item->VictimColor->z);
		}
		x = gHUD.DrawHudString(item->szVictim, x, centerY - m_string_height);

		y += weaponHeight + 10;
	}
}

// This message handler may be better off elsewhere
bool CHudDeathNotice::MsgFunc_DeathMsg(const char* pszName, int iSize, void* pbuf)
{
	SetActive(true);

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
	strncat(killedwith, READ_STRING(), 29);
	killedwith[31] = '\0';

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
		item->KillerColor = &gHUD.GetClientColor(killer);
		strncpy(item->szKiller, killerName, MAX_PLAYER_NAME_LENGTH - 1);
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
			item->AccompliceColor = &gHUD.GetClientColor(accomplice);
			strncpy(item->szAccomplice, accompliceName, MAX_PLAYER_NAME_LENGTH - 1);
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
		item->VictimColor = &gHUD.GetClientColor(victim);
		strncpy(item->szVictim, victimName, MAX_PLAYER_NAME_LENGTH - 1);
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

	return true;
}
