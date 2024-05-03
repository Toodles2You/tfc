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
// flashlight.cpp
//
// implementation of CHudFlashlight class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>


bool CHudFlashlight::Init()
{
	m_fOn = false;
#ifdef HALFLIFE_HUD_FLASHLIGHT
	m_fFade = 0;
	m_iBat = 100;
	m_flBat = 1.0;

	m_iFlags |= HUD_ACTIVE;
#endif

	gHUD.AddHudElem(this);

	return true;
}


void CHudFlashlight::Reset()
{
	m_fOn = false;
#ifdef HALFLIFE_HUD_FLASHLIGHT
	m_fFade = 0;
	m_iBat = 100;
	m_flBat = 1.0;
#endif
}


bool CHudFlashlight::VidInit()
{
#ifdef HALFLIFE_HUD_FLASHLIGHT
	int HUD_flash_empty = gHUD.GetSpriteIndex("flash_empty");
	int HUD_flash_full = gHUD.GetSpriteIndex("flash_full");
	int HUD_flash_beam = gHUD.GetSpriteIndex("flash_beam");

	m_hSprite1 = gHUD.GetSprite(HUD_flash_empty);
	m_hSprite2 = gHUD.GetSprite(HUD_flash_full);
	m_hBeam = gHUD.GetSprite(HUD_flash_beam);
	m_prc1 = &gHUD.GetSpriteRect(HUD_flash_empty);
	m_prc2 = &gHUD.GetSpriteRect(HUD_flash_full);
	m_prcBeam = &gHUD.GetSpriteRect(HUD_flash_beam);
	m_iWidth = m_prc2->right - m_prc2->left;
#endif
	return true;
}


void CHudFlashlight::Update_Flashlight(const bool on)
{
	if (m_fOn == on)
	{
		return;
	}

	m_fOn = on;

	if (m_fOn)
	{
		PlaySound("items/flashlight1.wav", VOL_NORM);
	}
	else
	{
		PlaySound("items/flashlight2.wav", VOL_NORM);
	}
}


bool CHudFlashlight::Draw(float flTime)
{
#ifdef HALFLIFE_HUD_FLASHLIGHT
	if ((gHUD.m_iHideHUDDisplay & HIDEHUD_FLASHLIGHT) != 0)
		return true;

	int x, y, a;
	Rect rc;

	if (m_fOn)
		a = 225;
	else
		a = MIN_ALPHA;

	auto color = (m_flBat < 0.20) ? CHud::COLOR_WARNING : CHud::COLOR_PRIMARY;

	y = (m_prc1->bottom - m_prc2->top) / 2;
	x = gHUD.GetWidth() - m_iWidth - m_iWidth / 2;

	// Draw the flashlight casing
	gHUD.DrawHudSprite(m_hSprite1, 0, m_prc1, x, y, color, a);

	if (m_fOn)
	{ // draw the flashlight beam
		x = gHUD.GetWidth() - m_iWidth / 2;

		gHUD.DrawHudSprite(m_hBeam, 0, m_prcBeam, x, y, color, a);
	}

	// draw the flashlight energy level
	x = gHUD.GetWidth() - m_iWidth - m_iWidth / 2;
	int iOffset = m_iWidth * (1.0 - m_flBat);
	if (iOffset < m_iWidth)
	{
		rc = *m_prc2;
		rc.left += iOffset;

		gHUD.DrawHudSprite(m_hSprite2, 0, &rc, x + iOffset, y, color, a);
	}
#endif
	return true;
}
