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
	HOOK_MESSAGE(Health);
	HOOK_MESSAGE(Damage);
	m_iHealth = 100;
	m_fFade = 0;
	m_iFlags = 0;
	m_bitsDamage = 0;
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
	giDmgHeight = 0;
	giDmgWidth = 0;

	memset(m_dmg, 0, sizeof(DAMAGE_IMAGE) * NUM_DMG_TYPES);


	gHUD.AddHudElem(this);
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

bool CHudHealth::VidInit()
{
	m_hSprite = 0;

	m_HUD_dmg_bio = gHUD.GetSpriteIndex("dmg_bio") + 1;
	m_HUD_cross = gHUD.GetSpriteIndex("cross");

	giDmgHeight = gHUD.GetSpriteRect(m_HUD_dmg_bio).right - gHUD.GetSpriteRect(m_HUD_dmg_bio).left;
	giDmgWidth = gHUD.GetSpriteRect(m_HUD_dmg_bio).bottom - gHUD.GetSpriteRect(m_HUD_dmg_bio).top;
	return true;
}

void CHudHealth::Update_Health(int iHealth)
{
	m_iFlags |= HUD_ACTIVE;

	// Only update the fade if we've changed health
	if (iHealth != m_iHealth)
	{
		m_fFade = FADE_TIME;
		m_iHealth = iHealth;
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

	UpdateTiles(gHUD.m_flTime, bitsDamage);

	// Actually took damage?
	if (damageTaken > 0 || armor > 0)
		CalcDamageDirection(vecFrom);

	if (damageTaken > 0)
	{
		V_PunchAxis(0, -2.0F);
	}

	return true;
}


// Returns back a color from the
// Green <-> Yellow <-> Red ramp
void CHudHealth::GetPainColor(int& r, int& g, int& b)
{
	int iHealth = m_iHealth;

	if (iHealth > 25)
		iHealth -= 25;
	else if (iHealth < 0)
		iHealth = 0;
#if 0
	g = iHealth * 255 / 100;
	r = 255 - g;
	b = 0;
#else
	if (m_iHealth > 25)
	{
		gHUD.GetColor(r, g, b, CHud::COLOR_PRIMARY);
	}
	else
	{
		gHUD.GetColor(r, g, b, CHud::COLOR_WARNING);
	}
#endif
}

bool CHudHealth::Draw(float flTime)
{
	int a, x, y;
	int HealthWidth;

	if ((gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH) != 0 || 0 != gEngfuncs.IsSpectateOnly())
		return true;

	if (0 == m_hSprite)
		m_hSprite = LoadSprite(PAIN_NAME);

	// Has health changed? Flash the health #
	if (0 != m_fFade)
	{
		m_fFade -= (gHUD.m_flTimeDelta * 20);
		if (m_fFade <= 0)
		{
			a = MIN_ALPHA;
			m_fFade = 0;
		}

		// Fade the health number back to dim

		a = MIN_ALPHA + (m_fFade / FADE_TIME) * 128;
	}
	else
		a = MIN_ALPHA;

	// If health is getting low, make it bright red
	if (m_iHealth <= 15)
		a = 255;
	
	auto color = (m_iHealth <= 25) ? CHud::COLOR_WARNING : CHud::COLOR_PRIMARY;

	HealthWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).right - gHUD.GetSpriteRect(gHUD.m_HUD_number_0).left;
	int CrossWidth = gHUD.GetSpriteRect(m_HUD_cross).right - gHUD.GetSpriteRect(m_HUD_cross).left;

	y = gHUD.GetHeight() - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	x = CrossWidth / 2;

	gHUD.DrawHudSpriteIndex(m_HUD_cross, x, y, color, a);

	x = CrossWidth + HealthWidth / 2;

	//Reserve space for 3 digits by default, but allow it to expand
	x += gHUD.GetHudNumberWidth(m_iHealth, 3, DHN_DRAWZERO);

	gHUD.DrawHudNumberReverse(x, y, m_iHealth, DHN_DRAWZERO, color, a);

	x += HealthWidth / 2;

	int iHeight = gHUD.m_iFontHeight;
	int iWidth = HealthWidth / 10;

	gHUD.DrawHudFill(x, y, iWidth, iHeight, CHud::COLOR_PRIMARY, MIN_ALPHA);

	gHUD.m_Battery.m_iAnchorX = x + HealthWidth / 2;
	gHUD.m_Battery.m_iAnchorY = gHUD.GetHeight();

	DrawDamage(flTime);
	return DrawPain(flTime);
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


	memcpy(vecOrigin, gHUD.m_vecOrigin, sizeof(Vector));
	memcpy(vecAngles, gHUD.m_vecAngles, sizeof(Vector));


	VectorSubtract(vecFrom, vecOrigin, vecFrom);

	float flDistToTarget = vecFrom.Length();

	vecFrom = vecFrom.Normalize();
	AngleVectors(vecAngles, forward, right, up);

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

bool CHudHealth::DrawPain(float flTime)
{
	if (!(0 != m_fAttackFront || 0 != m_fAttackRear || 0 != m_fAttackLeft || 0 != m_fAttackRight))
		return true;

	int x, y, shade;

	float fFade = gHUD.m_flTimeDelta * 2;

	// SPR_Draw top
	if (m_fAttackFront > 0.4)
	{
		shade = 255 * std::max(m_fAttackFront, 0.5F);

		x = gHUD.GetWidth() / 2 - SPR_Width(m_hSprite, 0) / 2;
		y = gHUD.GetHeight() / 2 - SPR_Height(m_hSprite, 0) * 3;
		gHUD.DrawHudSprite(m_hSprite, 0, NULL, x, y, CHud::COLOR_DEFAULT, shade);
		m_fAttackFront = std::max(m_fAttackFront - fFade, 0.0F);
	}
	else
		m_fAttackFront = 0;

	if (m_fAttackRight > 0.4)
	{
		shade = 255 * std::max(m_fAttackRight, 0.5F);

		x = gHUD.GetWidth() / 2 + SPR_Width(m_hSprite, 1) * 2;
		y = gHUD.GetHeight() / 2 - SPR_Height(m_hSprite, 1) / 2;
		gHUD.DrawHudSprite(m_hSprite, 1, NULL, x, y, CHud::COLOR_DEFAULT, shade);
		m_fAttackRight = std::max(m_fAttackRight - fFade, 0.0F);
	}
	else
		m_fAttackRight = 0;

	if (m_fAttackRear > 0.4)
	{
		shade = 255 * std::max(m_fAttackRear, 0.5F);

		x = gHUD.GetWidth() / 2 - SPR_Width(m_hSprite, 2) / 2;
		y = gHUD.GetHeight() / 2 + SPR_Height(m_hSprite, 2) * 2;
		gHUD.DrawHudSprite(m_hSprite, 2, NULL, x, y, CHud::COLOR_DEFAULT, shade);
		m_fAttackRear = std::max(m_fAttackRear - fFade, 0.0F);
	}
	else
		m_fAttackRear = 0;

	if (m_fAttackLeft > 0.4)
	{
		shade = 255 * std::max(m_fAttackLeft, 0.5F);

		x = gHUD.GetWidth() / 2 - SPR_Width(m_hSprite, 3) * 3;
		y = gHUD.GetHeight() / 2 - SPR_Height(m_hSprite, 3) / 2;
		gHUD.DrawHudSprite(m_hSprite, 3, NULL, x, y, CHud::COLOR_DEFAULT, shade);

		m_fAttackLeft = std::max(m_fAttackLeft - fFade, 0.0F);
	}
	else
		m_fAttackLeft = 0;

	return true;
}

bool CHudHealth::DrawDamage(float flTime)
{
	int r, g, b, a;
	DAMAGE_IMAGE* pdmg;

	if (0 == m_bitsDamage)
		return true;

	a = (int)(fabs(sin(flTime * 2)) * 256.0);

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
			pdmg->fExpire = std::min(flTime + DMG_IMAGE_LIFE, pdmg->fExpire);

			if (pdmg->fExpire <= flTime // when the time has expired
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


void CHudHealth::UpdateTiles(float flTime, long bitsDamage)
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
			pdmg->fExpire = flTime + DMG_IMAGE_LIFE; // extend the duration
			if (0 == pdmg->fBaseline)
				pdmg->fBaseline = flTime;
		}

		// Are we just turning it on?
		if ((bitsOn & giDmgFlags[i]) != 0)
		{
			// put this one at the bottom
			pdmg->x = giDmgWidth / 8;
			pdmg->y = gHUD.GetHeight() - giDmgHeight * 2;
			pdmg->fExpire = flTime + DMG_IMAGE_LIFE;

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
