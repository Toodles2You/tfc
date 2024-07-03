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
// battery.cpp
//
// implementation of CHudBattery class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "tf_defs.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_Battery, Battery)

bool CHudBattery::Init()
{
	m_iBat = 0;
	m_flType = 0.0F;
	m_szString[0] = '\0';

	HOOK_MESSAGE(Battery);

	return CHudBase::Init();
}


void CHudBattery::VidInit()
{
	const auto suitIndex = gHUD.GetSpriteIndex("suit");
	m_hSuit = gHUD.GetSprite(suitIndex);
	m_rcSuit = gHUD.GetSpriteRect(suitIndex);
}

void CHudBattery::Update_Battery(int iBat, float flType)
{
	SetActive(true);

	if (iBat != m_iBat || flType != m_flType)
	{
		m_iBat = iBat;
		Flash();
	}
	
	if (flType != m_flType)
	{
		Flash();
		m_flType = flType;
		m_szString[0] = '\0';

		if (m_flType != 0.0F)
		{
			if (m_flType <= 0.3F)
			{
				CHudTextMessage::LocaliseTextString("#Game_light_armor", m_szString, sizeof(m_szString));
			}
			else if (m_flType >= 0.8F)
			{
				CHudTextMessage::LocaliseTextString("#Game_heavy_armor", m_szString, sizeof(m_szString));
			}
			else
			{
				CHudTextMessage::LocaliseTextString("#Game_medium_armor", m_szString, sizeof(m_szString));
			}
		}
	}
}

bool CHudBattery::MsgFunc_Battery(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int x = READ_SHORT();

	Update_Battery(x);

	return true;
}


void CHudBattery::Draw(const float time)
{
	const auto percent = m_iBat / (float)sTFClassInfo[g_iPlayerClass].maxArmor;
	const auto color = CHud::COLOR_PRIMARY;
	const auto alpha = GetAlpha();

	const auto x = 117 + 8;
	const auto y = gHUD.GetHeight() - 26;

	gHUD.DrawHudBackground(
		x,
		y - 16,
		x + 100,
		y + 16);

	gHUD.DrawHudSprite(
		m_hSuit,
		0,
		&m_rcSuit,
		x + 16,
		y,
		color,
		alpha,
		CHud::a_center);

	gHUD.DrawHudNumber(
		x + 40,
		y,
		DHN_DRAWZERO | DHN_3DIGITS,
		m_iBat,
		color,
		alpha,
		CHud::a_west);

	/* Vertical fill bar next to the icon. */
	const auto barColor = CHud::COLOR_SECONDARY;
	const auto barHeight = (int)(percent * gHUD.m_iFontHeight);

	/* Unfilled background. */
	gHUD.DrawHudFill(
		x + 32,
		y - gHUD.m_iFontHeight / 2,
		4,
		gHUD.m_iFontHeight - barHeight,
		barColor,
		alpha / 2);

	/* Filled foreground. */
	gHUD.DrawHudFill(
		x + 32,
		y + gHUD.m_iFontHeight / 2 - barHeight,
		4,
		barHeight,
		barColor,
		alpha);

#if 0
	/* Toodles TODO: This looks like poop. */
	if (m_szString[0] != '\0')
	{
		gHUD.GetColor(r, g, b, CHud::COLOR_PRIMARY);
		ScaleColors(r, g, b, a);
		gHUD.DrawHudString(x + 2, m_iAnchorY - (m_prc1->bottom - m_prc1->top) - 8, gHUD.GetWidth(), m_szString, r, g, b);
	}
#endif
}
