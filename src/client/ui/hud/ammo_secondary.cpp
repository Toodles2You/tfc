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
	const auto color = CHud::COLOR_PRIMARY;
	const auto alpha = GetAlpha();

	/* Draw the ammo in the lower right corner of the HUD, above the weapon ammo. */
	const auto x = gHUD.GetWidth() - 10;
	const auto y = gHUD.GetHeight() - 73;

	auto w = 32;

	/* Widen the background for each ammo value. */
	for (auto i = 0; i < MAX_SEC_AMMO_VALUES; i++)
	{
		if (m_iAmmoAmounts[i] >= 0)
		{
			w += 30;
		}
	}

	gHUD.DrawHudBackground(
		x - w,
		y - 16,
		x,
		y + 16);

	if (m_HUD_ammoicon != 0)
	{
		/* Draw the icon sprite on the right side. */
		gHUD.DrawHudSpriteIndex(
			m_HUD_ammoicon,
			x - 16,
			y,
			color,
			alpha,
			CHud::a_center);
	}

	auto drawn = false;

	for (auto i = MAX_SEC_AMMO_VALUES - 1; i >= 0; i--)
	{
		/* Negative implies that they shouldn't be drawn. */
		if (m_iAmmoAmounts[i] < 0)
		{
			continue;
		}

		if (drawn)
		{
			/* Vertical divider bar. */
			gHUD.DrawHudFill(
				x - 32 - (i + 1) * 30 + 4, /* Toodles FIXME: This sucks. */
				y - 12,
				2,
				24,
				color,
				CHudBase::kMinAlpha);
		}

		drawn = true;

		/* Draw the current value to the left of the previous. */
		gHUD.DrawHudNumberReverse(
			x - 32 - i * 30,
			y,
			m_iAmmoAmounts[i],
			DHN_DRAWZERO,
			color,
			alpha,
			CHud::a_west);
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
