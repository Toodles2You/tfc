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
// ammo_secondary.cpp
//
// implementation of CHudAmmoSecondary class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "ammohistory.h"

DECLARE_MESSAGE(m_AmmoSecondary, SecAmmoVal);
DECLARE_MESSAGE(m_AmmoSecondary, SecAmmoIcon);

bool CHudAmmoSecondary::Init()
{
	HOOK_MESSAGE(SecAmmoVal);
	HOOK_MESSAGE(SecAmmoIcon);

	m_HUD_ammoicon = 0;

	for (int i = 0; i < MAX_SEC_AMMO_VALUES; i++)
		m_iAmmoAmounts[i] = -1; // -1 means don't draw this value

	return CHudBase::Init();
}

void CHudAmmoSecondary::VidInit()
{
	if (m_HUD_ammoicon == 0)
	{
		m_HUD_ammoicon = gHUD.GetSpriteIndex("grenade");
	}
}

void CHudAmmoSecondary::Draw(const float time)
{
	// draw secondary ammo icons above normal ammo readout
	int a, x, y, AmmoWidth;
	a = GetAlpha();

	AmmoWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).right - gHUD.GetSpriteRect(gHUD.m_HUD_number_0).left;

	y = gHUD.GetHeight() - (gHUD.m_iFontHeight * 4); // this is one font height higher than the weapon ammo values
	x = gHUD.GetWidth() - AmmoWidth;

	if (0 != m_HUD_ammoicon)
	{
		// Draw the ammo icon
		x -= (gHUD.GetSpriteRect(m_HUD_ammoicon).right - gHUD.GetSpriteRect(m_HUD_ammoicon).left);
		y -= (gHUD.GetSpriteRect(m_HUD_ammoicon).top - gHUD.GetSpriteRect(m_HUD_ammoicon).bottom);

		gHUD.DrawHudSpriteIndex(m_HUD_ammoicon, x, y, CHud::COLOR_PRIMARY, a);
	}
	else
	{ // move the cursor by the '0' char instead, since we don't have an icon to work with
		x -= AmmoWidth;
		y -= (gHUD.GetSpriteRect(gHUD.m_HUD_number_0).top - gHUD.GetSpriteRect(gHUD.m_HUD_number_0).bottom);
	}

	// draw the ammo counts, in reverse order, from right to left
	for (int i = MAX_SEC_AMMO_VALUES - 1; i >= 0; i--)
	{
		if (m_iAmmoAmounts[i] < 0)
			continue; // negative ammo amounts imply that they shouldn't be drawn

		// half a char gap between the ammo number and the previous pic
		x -= (AmmoWidth / 2);

		// draw the number, right-aligned
		x -= (gHUD.GetNumWidth(m_iAmmoAmounts[i], DHN_DRAWZERO) * AmmoWidth);
		gHUD.DrawHudNumber(x, y, DHN_DRAWZERO, m_iAmmoAmounts[i], CHud::COLOR_PRIMARY, a);

		if (i != 0)
		{
			// draw the divider bar
			x -= (AmmoWidth / 2);
			gHUD.DrawHudFill(x, y, (AmmoWidth / 10), gHUD.m_iFontHeight, CHud::COLOR_PRIMARY, MIN_ALPHA);
		}
	}
}

void CHudAmmoSecondary::Update_SecAmmoVal(int iIndex, int iCount)
{
	if (iIndex < 0 || iIndex >= MAX_SEC_AMMO_VALUES)
		return;

	if (m_iAmmoAmounts[iIndex] != iCount)
	{
		Flash();
	}

	m_iAmmoAmounts[iIndex] = iCount;

	// check to see if there is anything left to draw
	int count = 0;
	for (int i = 0; i < MAX_SEC_AMMO_VALUES; i++)
	{
		count += std::max(0, m_iAmmoAmounts[i]);
	}

	SetActive(count != 0);
}

// Message handler for Secondary Ammo Value
// accepts one value:
//		string:  sprite name
bool CHudAmmoSecondary::MsgFunc_SecAmmoIcon(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int iType = READ_BYTE();
	const char* pszIcon = READ_STRING();

	gWR.LoadAmmoSprites(iType, pszIcon);

	if (iType == AMMO_GRENADES2)
	{
		m_HUD_ammoicon = gHUD.GetSpriteIndex(pszIcon);
	}

	return true;
}

// Message handler for Secondary Ammo Icon
// Sets an ammo value
// takes two values:
//		byte:  ammo index
//		byte:  ammo value
bool CHudAmmoSecondary::MsgFunc_SecAmmoVal(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	Update_SecAmmoVal(READ_BYTE(), READ_BYTE());
	return true;
}
