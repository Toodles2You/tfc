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
// Health.cpp
//
// implementation of CHudHealth class
//

#include "stdio.h"
#include "stdlib.h"
#include "tf_defs.h"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "view.h"
#include <string.h>


DECLARE_MESSAGE(m_Health, Health)
DECLARE_MESSAGE(m_Health, Damage)

#define PAIN_NAME "sprites/%d_pain.spr"
#define DAMAGE_NAME "sprites/%d_dmg.spr"

int giDmgHeight, giDmgWidth;

int giDmgFlags[NUM_DMG_TYPES] =
	{
		DMG_POISON,
		DMG_ACID,
		DMG_FREEZE | DMG_SLOWFREEZE,
		DMG_DROWN,
		DMG_BURN | DMG_SLOWBURN,
		DMG_NERVEGAS,
		DMG_RADIATION,
		DMG_SHOCK,
		DMG_CALTROP,
		DMG_TRANQ,
		DMG_CONCUSS,
		DMG_HALLUC};

bool CHudHealth::Init()
{
	if (!CHudBase::Init())
	{
		return false;
	}

	HOOK_MESSAGE(Health);
	HOOK_MESSAGE(Damage);
	m_iHealth = 100;
	m_bitsDamage = 0;
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
	giDmgHeight = 0;
	giDmgWidth = 0;

	memset(m_dmg, 0, sizeof(DAMAGE_IMAGE) * NUM_DMG_TYPES);

	return true;
}

void CHudHealth::Reset()
{
	// make sure the pain compass is cleared when the player respawns
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;


	// force all the flashing damage icons to expire
	m_bitsDamage = 0;
	for (int i = 0; i < NUM_DMG_TYPES; i++)
	{
		m_dmg[i].fExpire = 0;
	}
}

void CHudHealth::VidInit()
{
	m_hSprite = LoadSprite(PAIN_NAME);

	m_HUD_dmg_bio = gHUD.GetSpriteIndex("dmg_bio") + 1;

	int crossIndex = gHUD.GetSpriteIndex("cross");

	m_hCross = gHUD.GetSprite(crossIndex);
	m_rcCross = gHUD.GetSpriteRect(crossIndex);

	giDmgHeight = gHUD.GetSpriteRect(m_HUD_dmg_bio).right - gHUD.GetSpriteRect(m_HUD_dmg_bio).left;
	giDmgWidth = gHUD.GetSpriteRect(m_HUD_dmg_bio).bottom - gHUD.GetSpriteRect(m_HUD_dmg_bio).top;
}

void CHudHealth::Update_Health(int iHealth)
{
	// Only update the fade if we've changed health
	if (iHealth != m_iHealth)
	{
		m_iHealth = iHealth;
		Flash();
	}
}

bool CHudHealth::MsgFunc_Health(const char* pszName, int iSize, void* pbuf)
{
	// TODO: update local health data
	BEGIN_READ(pbuf, iSize);
	int x = READ_SHORT();

	Update_Health(x);

	return true;
}


bool CHudHealth::MsgFunc_Damage(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int armor = READ_BYTE();	   // armor
	int damageTaken = READ_BYTE(); // health
	long bitsDamage = READ_LONG(); // damage bits

	Vector vecFrom;

	for (int i = 0; i < 3; i++)
		vecFrom[i] = READ_COORD();

	// only send down damage type that have hud art
	UpdateTiles(gHUD.m_flTime, bitsDamage & DMG_SHOWNHUD);

	// Actually took damage?
	if (damageTaken > 0 || armor > 0)
		CalcDamageDirection(vecFrom);

	if (damageTaken > 0 && (bitsDamage & DMG_ARMOR_PIERCING) == 0)
	{
		V_PunchAxis(0, -2.0F);
	}

	return true;
}

void CHudHealth::Draw(const float time)
{
	const auto color = (m_iHealth > 25) ? CHud::COLOR_PRIMARY : CHud::COLOR_WARNING;
	const auto alpha = GetAlpha();

	const auto x = 10;
	const auto y = gHUD.GetHeight() - 26;

	gHUD.DrawHudBackground(
		x,
		y - 16,
		x + 92,
		y + 16);

	gHUD.DrawHudSprite(
		m_hCross,
		0,
		&m_rcCross,
		x + 16,
		y,
		color,
		alpha,
		CHud::a_center);

	gHUD.DrawHudNumber(
		x + 32,
		y,
		DHN_DRAWZERO | DHN_3DIGITS,
		m_iHealth,
		color,
		alpha,
		CHud::a_west);

	DrawDamage(time);
	DrawPain(time);
}

void CHudHealth::CalcDamageDirection(Vector vecFrom)
{
	Vector forward, right, up;
	float side, front;
	Vector vecOrigin, vecAngles;

	if (vecFrom == g_vecZero)
	{
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
		return;
	}

	vecOrigin = gHUD.m_vecOrigin;
	vecAngles = gHUD.m_vecAngles;

	vecFrom = vecFrom - vecOrigin;

	float flDistToTarget = vecFrom.Length();

	vecFrom = vecFrom.Normalize();
	AngleVectors(vecAngles, &forward, &right, &up);

	front = DotProduct(vecFrom, right);
	side = DotProduct(vecFrom, forward);

	if (flDistToTarget <= 50)
	{
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 1;
	}
	else
	{
		if (side > 0)
		{
			if (side > 0.3)
				m_fAttackFront = std::max(m_fAttackFront, side);
		}
		else
		{
			float f = fabs(side);
			if (f > 0.3)
				m_fAttackRear = std::max(m_fAttackRear, f);
		}

		if (front > 0)
		{
			if (front > 0.3)
				m_fAttackRight = std::max(m_fAttackRight, front);
		}
		else
		{
			float f = fabs(front);
			if (f > 0.3)
				m_fAttackLeft = std::max(m_fAttackLeft, f);
		}
	}
}

bool CHudHealth::DrawPain(float time)
{
	if (!(0 != m_fAttackFront || 0 != m_fAttackRear || 0 != m_fAttackLeft || 0 != m_fAttackRight))
		return true;

	int x, y, shade;

	float fFade = gHUD.m_flTimeDelta * 2;

	// SPR_Draw top
	if (m_fAttackFront > 0.4)
	{
		shade = 255 * std::max(m_fAttackFront, 0.5F);

		x = gHUD.GetWidth() / 2 - client::SPR_Width(m_hSprite, 0) / 2;
		y = gHUD.GetHeight() / 2 - client::SPR_Height(m_hSprite, 0) * 3;
		gHUD.DrawHudSprite(m_hSprite, 0, nullptr, x, y, CHud::COLOR_DEFAULT, shade);
		m_fAttackFront = std::max(m_fAttackFront - fFade, 0.0F);
	}
	else
		m_fAttackFront = 0;

	if (m_fAttackRight > 0.4)
	{
		shade = 255 * std::max(m_fAttackRight, 0.5F);

		x = gHUD.GetWidth() / 2 + client::SPR_Width(m_hSprite, 1) * 2;
		y = gHUD.GetHeight() / 2 - client::SPR_Height(m_hSprite, 1) / 2;
		gHUD.DrawHudSprite(m_hSprite, 1, nullptr, x, y, CHud::COLOR_DEFAULT, shade);
		m_fAttackRight = std::max(m_fAttackRight - fFade, 0.0F);
	}
	else
		m_fAttackRight = 0;

	if (m_fAttackRear > 0.4)
	{
		shade = 255 * std::max(m_fAttackRear, 0.5F);

		x = gHUD.GetWidth() / 2 - client::SPR_Width(m_hSprite, 2) / 2;
		y = gHUD.GetHeight() / 2 + client::SPR_Height(m_hSprite, 2) * 2;
		gHUD.DrawHudSprite(m_hSprite, 2, nullptr, x, y, CHud::COLOR_DEFAULT, shade);
		m_fAttackRear = std::max(m_fAttackRear - fFade, 0.0F);
	}
	else
		m_fAttackRear = 0;

	if (m_fAttackLeft > 0.4)
	{
		shade = 255 * std::max(m_fAttackLeft, 0.5F);

		x = gHUD.GetWidth() / 2 - client::SPR_Width(m_hSprite, 3) * 3;
		y = gHUD.GetHeight() / 2 - client::SPR_Height(m_hSprite, 3) / 2;
		gHUD.DrawHudSprite(m_hSprite, 3, nullptr, x, y, CHud::COLOR_DEFAULT, shade);

		m_fAttackLeft = std::max(m_fAttackLeft - fFade, 0.0F);
	}
	else
		m_fAttackLeft = 0;

	return true;
}

bool CHudHealth::DrawDamage(float time)
{
	int r, g, b, a;
	DAMAGE_IMAGE* pdmg;

	if (0 == m_bitsDamage)
		return true;

	a = (int)(fabs(sin(time * 2)) * 256.0);

	// Draw all the items
	int i;
	for (i = 0; i < NUM_DMG_TYPES; i++)
	{
		if ((m_bitsDamage & giDmgFlags[i]) != 0)
		{
			pdmg = &m_dmg[i];
			gHUD.DrawHudSpriteIndex(m_HUD_dmg_bio + i, pdmg->x, pdmg->y, CHud::COLOR_PRIMARY, a);
		}
	}


	// check for bits that should be expired
	for (i = 0; i < NUM_DMG_TYPES; i++)
	{
		DAMAGE_IMAGE* pdmg = &m_dmg[i];

		if ((m_bitsDamage & giDmgFlags[i]) != 0)
		{
			pdmg->fExpire = std::min(time + DMG_IMAGE_LIFE, pdmg->fExpire);

			if (pdmg->fExpire <= time // when the time has expired
				&& a < 40)				// and the flash is at the low point of the cycle
			{
				pdmg->fExpire = 0;

				int y = pdmg->y;
				pdmg->x = pdmg->y = 0;

				// move everyone above down
				for (int j = 0; j < NUM_DMG_TYPES; j++)
				{
					pdmg = &m_dmg[j];
					if (0 != pdmg->y && (pdmg->y < y))
						pdmg->y += giDmgHeight;
				}

				m_bitsDamage &= ~giDmgFlags[i]; // clear the bits
			}
		}
	}

	return true;
}


void CHudHealth::UpdateTiles(float time, long bitsDamage)
{
	DAMAGE_IMAGE* pdmg;

	// Which types are new?
	long bitsOn = ~m_bitsDamage & bitsDamage;

	for (int i = 0; i < NUM_DMG_TYPES; i++)
	{
		pdmg = &m_dmg[i];

		// Is this one already on?
		if ((m_bitsDamage & giDmgFlags[i]) != 0)
		{
			pdmg->fExpire = time + DMG_IMAGE_LIFE; // extend the duration
			if (0 == pdmg->fBaseline)
				pdmg->fBaseline = time;
		}

		// Are we just turning it on?
		if ((bitsOn & giDmgFlags[i]) != 0)
		{
			// put this one at the bottom
			pdmg->x = giDmgWidth / 8;
			pdmg->y = gHUD.GetHeight() - giDmgHeight * 2;
			pdmg->fExpire = time + DMG_IMAGE_LIFE;

			// move everyone else up
			for (int j = 0; j < NUM_DMG_TYPES; j++)
			{
				if (j == i)
					continue;

				pdmg = &m_dmg[j];
				if (0 != pdmg->y)
					pdmg->y -= giDmgHeight;
			}
			pdmg = &m_dmg[i];
		}
	}

	// damage bits are only turned on here;  they are turned off when the draw time has expired (in DrawDamage())
	m_bitsDamage |= bitsDamage;
}
