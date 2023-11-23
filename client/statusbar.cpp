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
// statusbar.cpp
//
// generic text status bar, set by game dll
// runs across bottom of screen
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

bool CHudStatusBar::Init()
{
	gHUD.AddHudElem(this);

	Reset();

	hud_centerid = CVAR_CREATE("hud_centerid", "0", FCVAR_ARCHIVE);
	hud_expireid = CVAR_CREATE("hud_expireid", "0.2", FCVAR_ARCHIVE);

	return true;
}

bool CHudStatusBar::VidInit()
{
	// Load sprites here

	return true;
}

void CHudStatusBar::Reset()
{
	m_targetIndex = 0;
	m_targetExpireTime = -1000.0F;
	m_szStatusBar[0] = '\0';
	m_iFlags &= ~HUD_ACTIVE; // start out inactive
}

bool CHudStatusBar::Draw(float fTime)
{
	if (hud_expireid->value > 0.0F
	 && fTime - m_targetExpireTime >= hud_expireid->value)
	{
		Reset();
		return true;
	}

	auto info = &g_PlayerInfoList[m_targetIndex];

	if (info->name[0] == '\0')
	{
		Reset();
		return true;
	}

#if 0
	snprintf(m_szStatusBar, MAX_STATUSTEXT_LENGTH, "%s | Health: %i%%", info->name, extra->health);
#else
	snprintf(m_szStatusBar, MAX_STATUSTEXT_LENGTH, "%s", info->name);
#endif
	m_szStatusBar[MAX_STATUSTEXT_LENGTH - 1] = '\0';

	int textWidth, textHeight;
	gHUD.GetHudStringSize(m_szStatusBar, textWidth, textHeight);

	auto color = gHUD.GetClientColor(m_targetIndex);
	gEngfuncs.pfnDrawSetTextColor(color[0], color[1], color[2]);

	int x = std::max(0, std::max(2, ((int)gHUD.GetWidth() - textWidth)) >> 1);
	int y = (gHUD.GetHeight() >> 1) + textHeight * hud_centerid->value;

	gHUD.DrawHudString(m_szStatusBar, x, y);

	return true;
}

void CHudStatusBar::UpdateStatusBar(cl_entity_t* entity)
{
	if (entity == nullptr)
	{
		if (hud_expireid->value <= 0.0F)
		{
			Reset();
		}
		return;
	}

	m_targetIndex = entity->index;
	m_targetExpireTime = gEngfuncs.GetClientTime();
	m_iFlags |= HUD_ACTIVE;
}
