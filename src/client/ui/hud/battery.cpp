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
	m_iBatMax = 0;
	m_flType = 0.0F;
	m_szString[0] = '\0';

	HOOK_MESSAGE(Battery);

	return CHudBase::Init();
}


void CHudBattery::VidInit()
{
	int HUD_suit_empty = gHUD.GetSpriteIndex("suit_empty");
	int HUD_suit_full = gHUD.GetSpriteIndex("suit_full");

	m_hSprite1 = gHUD.GetSprite(HUD_suit_empty);
	m_hSprite2 = gHUD.GetSprite(HUD_suit_full);
	m_prc1 = &gHUD.GetSpriteRect(HUD_suit_empty);
	m_prc2 = &gHUD.GetSpriteRect(HUD_suit_full);
	m_iHeight = m_prc2->bottom - m_prc1->top;
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
	const auto iBatMax = sTFClassInfo[g_iPlayerClass].maxArmor;
	int x, y, a;
	Rect rc;

	rc = *m_prc2;

	rc.top += m_iHeight * ((float)(iBatMax - (std::min(iBatMax, m_iBat))) * (1.0F / iBatMax)); // battery can go from 0 to 100 so * 0.01 goes from 0 to 1

	a = GetAlpha();

	int r, g, b;

	int iOffset = (m_prc1->bottom - m_prc1->top) / 6;

	y = m_iAnchorY - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	x = m_iAnchorX;

	gHUD.DrawHudSprite(m_hSprite1, 0, m_prc1, x, y - iOffset, CHud::COLOR_PRIMARY, a);

	if (rc.bottom > rc.top)
	{
		gHUD.DrawHudSprite(m_hSprite2, 0, &rc, x, y - iOffset + (rc.top - m_prc2->top), CHud::COLOR_PRIMARY, a);
	}

	x += (m_prc1->right - m_prc1->left);
	gHUD.DrawHudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, m_iBat, CHud::COLOR_PRIMARY, a);

	/* Toodles TODO: This looks like poop. */
	if (m_szString[0] != '\0')
	{
		gHUD.GetColor(r, g, b, CHud::COLOR_PRIMARY);
		ScaleColors(r, g, b, a);
		gHUD.DrawHudString(x + 2, m_iAnchorY - (m_prc1->bottom - m_prc1->top) - 8, gHUD.GetWidth(), m_szString, r, g, b);
	}
}
