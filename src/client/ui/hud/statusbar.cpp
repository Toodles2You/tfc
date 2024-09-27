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
	return CHudBase::Init();
}

void CHudStatusBar::Reset()
{
	m_targetIndex = 0;
	m_szStatusBar[0] = '\0';
}

void CHudStatusBar::Draw(const float time)
{
	auto playerIndex = m_targetIndex;
	auto entity = client::GetEntityByIndex(playerIndex);

	if (entity == nullptr)
	{
		return;
	}

	/* Check if the entity is invisible. */

	if (entity->curstate.rendermode != kRenderNormal
	 && entity->curstate.renderamt <= 5)
	{
		return;
	}

	if (playerIndex < 1 || playerIndex > client::GetMaxClients())
	{
		return;
	}

	char* fmt = "%s";

	hud_player_info_t info;
	client::GetPlayerInfo(playerIndex, &info);

	if (info.name[0] == '\0')
	{
		info.name = "Player";
	}

	strncpy (m_szStatusFormat, fmt, MAX_STATUSTEXT_LENGTH);

	/* Show extra information for allies. */

	if (gHUD.m_gameMode != kGamemodeDeathmatch && g_iTeamNumber == entity->curstate.team)
	{
		strncat (m_szStatusFormat, "  +%3i", MAX_STATUSTEXT_LENGTH);
	}

	/* Print the formatted text. */

	snprintf (m_szStatusBar, MAX_STATUSTEXT_LENGTH,
		m_szStatusFormat, info.name,
		entity->curstate.health);

	m_szStatusBar[MAX_STATUSTEXT_LENGTH - 1] = '\0';

	/* Get the text size. */

	int textWidth, textHeight;
	gHUD.GetHudStringSize(m_szStatusBar, textWidth, textHeight);

	/* Get the text color. */

	const auto color = gHUD.GetTeamColor(entity->curstate.team);

	client::DrawSetTextColor(color[0], color[1], color[2]);

	/* Center the text. */

	const auto x = std::max(0, std::max(2, ((int)gHUD.GetWidth() - textWidth)) >> 1);
	const auto y = (gHUD.GetHeight() >> 1) + textHeight * 4;

	/* Draw the status bar! */

	int w, h;
	gHUD.GetHudStringSize(m_szStatusBar, w, h);
	gHUD.DrawHudBackground(x, y, x + w, y + h);

	gHUD.DrawHudString(m_szStatusBar, x, y);
}

void CHudStatusBar::UpdateStatusBar(cl_entity_t* entity)
{
	if (entity == nullptr)
	{
		SetActive(false);

		return;
	}

	m_targetIndex = entity->index;

	SetActive(true);
}
