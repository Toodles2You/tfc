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
	hud_expireid = client::RegisterVariable("hud_expireid", "0.2", FCVAR_ARCHIVE);

	return CHudBase::Init();
}

void CHudStatusBar::Reset()
{
	m_targetIndex = 0;
	m_targetExpireTime = -1000.0F;
	m_szStatusBar[0] = '\0';
}

void CHudStatusBar::Draw(const float time)
{
	if (hud_expireid->value > 0.0F
	 && time - m_targetExpireTime >= hud_expireid->value)
	{
		SetActive(false);
		return;
	}

	auto info = &g_PlayerInfoList[m_targetIndex];

	auto extra = &g_PlayerExtraInfo[m_targetIndex];

	auto name = info->name;
	auto teamNumber = extra->teamnumber;
	auto health = (int)extra->health;
	auto armorValue = (int)extra->armorvalue;

	const auto entity = client::GetEntityByIndex(m_targetIndex);

	if (entity != nullptr)
	{
		/* Check for spy disguise. */

		if (entity->curstate.aiment != 0)
		{
			/* Toodles FIXME: Don't perform this every frame. */
			client::GetPlayerInfo(
				entity->curstate.aiment,
				g_PlayerInfoList + entity->curstate.aiment);

			name = g_PlayerInfoList[entity->curstate.aiment].name;
		}

		teamNumber = entity->curstate.team;
		health = entity->curstate.health;
		armorValue = entity->curstate.fuser1;
	}

	if (name[0] == '\0')
	{
		SetActive(false);
		return;
	}

	/* Entity is invisible. */
	if (extra->teamnumber != g_iTeamNumber && (entity == nullptr
	 || (entity->curstate.rendermode != kRenderNormal && entity->curstate.renderamt <= 5)))
	{
		SetActive(false);
		return;
	}

	if (gHUD.m_gameMode == kGamemodeCooperative
	 || (gHUD.m_gameMode >= kGamemodeTeamplay && teamNumber == g_iTeamNumber)
	 || gHUD.IsSpectator())
	{
		/* Toodles TODO: Use actual icon sprites for stats. */

		snprintf(m_szStatusBar, MAX_STATUSTEXT_LENGTH, "%s  +%3i  \u03bb%3i",
			name, health, armorValue);
	}
	else
	{
		snprintf(m_szStatusBar, MAX_STATUSTEXT_LENGTH, "%s", name);
	}
	m_szStatusBar[MAX_STATUSTEXT_LENGTH - 1] = '\0';

	int textWidth, textHeight;
	gHUD.GetHudStringSize(m_szStatusBar, textWidth, textHeight);

	auto color = gHUD.GetTeamColor(teamNumber);
	client::DrawSetTextColor(color[0], color[1], color[2]);

	int x = std::max(0, std::max(2, ((int)gHUD.GetWidth() - textWidth)) >> 1);
	int y = (gHUD.GetHeight() >> 1) + textHeight * 4;

	int w, h;
	gHUD.GetHudStringSize(m_szStatusBar, w, h);
	gHUD.DrawHudBackground(x, y, x + w, y + h);

	gHUD.DrawHudString(m_szStatusBar, x, y);
}

void CHudStatusBar::UpdateStatusBar(cl_entity_t* entity)
{
	if (entity == nullptr)
	{
		if (hud_expireid->value <= 0.0F)
		{
			SetActive(false);
		}
		return;
	}

	if (m_targetIndex != entity->index)
	{
		client::GetPlayerInfo(
			entity->index,
			g_PlayerInfoList + entity->index);
	}

	m_targetIndex = entity->index;
	m_targetExpireTime = client::GetClientTime();
	SetActive(true);
}
