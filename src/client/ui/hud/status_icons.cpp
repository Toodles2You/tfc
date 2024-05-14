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
// status_icons.cpp
//
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "event_api.h"

DECLARE_MESSAGE(m_StatusIcons, StatusIcon);

bool CHudStatusIcons::Init()
{
	HOOK_MESSAGE(StatusIcon);

	return CHudBase::Init();
}

void CHudStatusIcons::VidInit()
{
#ifdef HALFLIFE_GRENADES
	m_hTimer = client::SPR_Load("sprites/timer.spr");

	int i = gHUD.GetSpriteIndex("grenade");
	m_hGrenade = gHUD.GetSprite(i);
	m_rcGrenade = gHUD.GetSpriteRect(i);
#endif
}

void CHudStatusIcons::Reset()
{
	memset(m_IconList, 0, sizeof m_IconList);

#ifdef HALFLIFE_GRENADES
	m_bTimerActive = false;
	m_flTimerStart = -1000;
#endif
}

// Draw status icons along the left-hand side of the screen
void CHudStatusIcons::Draw(const float time)
{
	if (0 != client::IsSpectateOnly())
	{
		return;
	}

	// find starting position to draw from, along right-hand side of screen
	int x = 5;
	int y = gHUD.GetHeight() / 2;

	// loop through icon list, and draw any valid icons drawing up from the middle of screen
	for (int i = 0; i < MAX_ICONSPRITES; i++)
	{
		if (0 != m_IconList[i].spr)
		{
			y -= (m_IconList[i].rc.bottom - m_IconList[i].rc.top) + 5;

			gHUD.DrawHudSprite(m_IconList[i].spr, 0, &m_IconList[i].rc, x, y, m_IconList[i].r, m_IconList[i].g, m_IconList[i].b, 255);
		}
	}

#ifdef HALFLIFE_GRENADES
	if (m_bTimerActive)
	{
		DrawTimer(time);
	}
#endif
}


#ifdef HALFLIFE_GRENADES

bool CHudStatusIcons::DrawTimer(float flTime)
{	
	int timerFrames = client::SPR_Frames(m_hTimer);

	float timerDelta = (flTime - (m_flTimerStart + 0.8)) / 3.1;
	int timerFrame = std::floor(timerFrames * std::max(timerDelta, 0.0F));

	if (timerFrame >= timerFrames)
	{
		m_bTimerActive = false;
		return true;
	}

	const auto color =
		timerFrame >= timerFrames - 1 ? CHud::COLOR_WARNING : CHud::COLOR_PRIMARY;

	int y = gHUD.GetHeight() - (gHUD.m_iFontHeight >> 1);

	int timerWidth = client::SPR_Width(m_hTimer, timerFrame);
	int timerHeight = client::SPR_Height(m_hTimer, timerFrame);
	float timerAlpha = std::min((flTime - m_flTimerStart) / 0.8F, 1.0F);

	const auto timerX = (gHUD.GetWidth() - timerWidth) / 2;
	const auto timerY = y - timerHeight;

	gHUD.DrawHudBackground(
		timerX,
		timerY,
		timerX + timerWidth,
		timerY + timerHeight);

	gHUD.DrawHudSprite(
		m_hTimer,
		timerFrame,
		nullptr,
		timerX,
		timerY,
		color,
		CHudBase::kMaxAlpha * timerAlpha);

	int grenadeWidth = m_rcGrenade.right - m_rcGrenade.left;
	int grenadeHeight = m_rcGrenade.bottom - m_rcGrenade.top;
	
	gHUD.DrawHudSprite(
		m_hGrenade,
		0,
		&m_rcGrenade,
		(gHUD.GetWidth() - grenadeWidth) >> 1,
		y - ((timerHeight + grenadeHeight) >> 1),
		color,
		CHudBase::kMaxAlpha * timerAlpha);

	return true;
}

#endif


// Message handler for StatusIcon message
// accepts five values:
//		byte   : true = ENABLE icon, false = DISABLE icon
//		string : the sprite name to display
//		byte   : red
//		byte   : green
//		byte   : blue
bool CHudStatusIcons::MsgFunc_StatusIcon(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int mode = READ_BYTE();
	char* pszIconName = READ_STRING();
	if (mode != 0)
	{
		if (mode == 1)
		{
			int r = READ_BYTE();
			int g = READ_BYTE();
			int b = READ_BYTE();
			EnableIcon(pszIconName, r, g, b);
		}
		else
		{
			EnableIcon(pszIconName);
		}
	}
	else
	{
		DisableIcon(pszIconName);
	}

	return true;
}

// add the icon to the icon list, and set it's drawing color
void CHudStatusIcons::EnableIcon(const char* pszIconName, int red, int green, int blue)
{
	if (red == 0 && green == 0 && blue == 0)
	{
		gHUD.GetColor(red, green, blue, CHud::COLOR_PRIMARY);
	}

	bool added = false;

	int i;
	// check to see if the sprite is in the current list
	for (i = 0; i < MAX_ICONSPRITES; i++)
	{
		if (!stricmp(m_IconList[i].szSpriteName, pszIconName))
			break;
	}

	if (i == MAX_ICONSPRITES)
	{
		// icon not in list, so find an empty slot to add to
		for (i = 0; i < MAX_ICONSPRITES; i++)
		{
			if (0 == m_IconList[i].spr)
				break;
		}
		added = true;
	}

	// if we've run out of space in the list, overwrite the first icon
	if (i == MAX_ICONSPRITES)
	{
		i = 0;
	}

	// Load the sprite and add it to the list
	// the sprite must be listed in hud.txt
	int spr_index = gHUD.GetSpriteIndex(pszIconName);
	m_IconList[i].spr = gHUD.GetSprite(spr_index);
	m_IconList[i].rc = gHUD.GetSpriteRect(spr_index);
	m_IconList[i].r = red;
	m_IconList[i].g = green;
	m_IconList[i].b = blue;
	strcpy(m_IconList[i].szSpriteName, pszIconName);

#ifdef HALFLIFE_GRENADES
	// Hack: Play Timer sound when a grenade icon is played (in 0.8 seconds)
	if (added && strstr(m_IconList[i].szSpriteName, "grenade"))
	{
		cl_entity_t* pthisplayer = client::GetLocalPlayer();

		client::event::StopSound(
			pthisplayer->index,
			CHAN_STATIC,
			"weapons/timer.wav");

		client::event::PlaySound(
			pthisplayer->index,
			pthisplayer->origin,
			CHAN_STATIC,
			"weapons/timer.wav",
			VOL_NORM,
			ATTN_NORM,
			0,
			PITCH_NORM);

		m_bTimerActive = true;
		m_flTimerStart = gHUD.m_flTime;

		m_hGrenade = m_IconList[i].spr;
		m_rcGrenade = m_IconList[i].rc;
	}
#endif

	SetActive(true);
}

void CHudStatusIcons::DisableIcon(const char* pszIconName)
{
	// find the sprite is in the current list
	for (int i = 0; i < MAX_ICONSPRITES; i++)
	{
		if (!stricmp(m_IconList[i].szSpriteName, pszIconName))
		{
			// clear the item from the list
			memset(&m_IconList[i], 0, sizeof(icon_sprite_t));
			return;
		}
	}
}
