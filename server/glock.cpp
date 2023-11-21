/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"

LINK_ENTITY_TO_CLASS(weapon_glock, CGlock);
LINK_ENTITY_TO_CLASS(weapon_9mmhandgun, CGlock);

bool CGlock::Spawn()
{
	pev->classname = MAKE_STRING("weapon_9mmhandgun"); // hack to allow for old names
	Precache();
	m_iId = WEAPON_GLOCK;
	SetModel("models/w_9mmhandgun.mdl");

	m_iDefaultAmmo = GLOCK_DEFAULT_GIVE;

	FallInit(); // get ready to fall down.

	return true;
}


void CGlock::Precache()
{
	PRECACHE_MODEL("models/v_9mmhandgun.mdl");
	PRECACHE_MODEL("models/w_9mmhandgun.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");

	PRECACHE_SOUND("weapons/pl_gun1.wav"); //silenced handgun
	PRECACHE_SOUND("weapons/pl_gun2.wav"); //silenced handgun
	PRECACHE_SOUND("weapons/pl_gun3.wav"); //handgun

	m_usFireGlock = PRECACHE_EVENT(1, "events/glock1.sc");
}

bool CGlock::GetWeaponInfo(WeaponInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->iAmmo1 = AMMO_9MM;
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->iAmmo2 = AMMO_NONE;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GLOCK_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLOCK;
	p->iWeight = GLOCK_WEIGHT;

	return true;
}

bool CGlock::Deploy()
{
	// pev->body = 1;
	return DefaultDeploy("models/v_9mmhandgun.mdl", "models/p_9mmhandgun.mdl", GLOCK_DRAW, "onehanded");
}

bool CGlock::Holster()
{
	return DefaultHolster(GLOCK_HOLSTER);
}

void CGlock::SecondaryAttack()
{
	GlockFire(9, 200, false);
}

void CGlock::PrimaryAttack()
{
	GlockFire(1, 300, true);
}

void CGlock::GlockFire(float flSpread, int fCycleTime, bool fUseAutoAim)
{
	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_iNextPrimaryAttack = m_iNextSecondaryAttack = 200;
		return;
	}

	auto shots = 0;

	while (m_iNextPrimaryAttack <= 0)
	{
		m_iNextPrimaryAttack += fCycleTime;
		shots++;
	}
	m_iNextSecondaryAttack = m_iNextPrimaryAttack;

	if (shots > m_iClip)
	{
		shots = m_iClip;
		PlayEmptySound();
	}

	m_iClip -= shots;

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL
	m_pPlayer->FireBullets(gSkillData.plrDmg9MM, Vector2D(flSpread, flSpread));
#endif

	m_pPlayer->PlaybackEvent(m_usFireGlock, flSpread, flSpread, m_pPlayer->random_seed, shots, m_iClip == 0, fUseAutoAim);

	m_pPlayer->CheckAmmoLevel(this);

	m_iTimeWeaponIdle = util::SharedRandomLong(m_pPlayer->random_seed, 10000, 15000);
}


void CGlock::Reload()
{
	DefaultReload(GLOCK_MAX_CLIP, m_iClip == 0 ? GLOCK_RELOAD : GLOCK_RELOAD_NOT_EMPTY, 1500);
}



void CGlock::WeaponIdle()
{
	if (m_iTimeWeaponIdle > 0)
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = util::SharedRandomFloat(m_pPlayer->random_seed, 0.0, 1.0);

		if (flRand <= 0.3 + 0 * 0.75)
		{
			iAnim = GLOCK_IDLE3;
			m_iTimeWeaponIdle = 3063;
		}
		else if (flRand <= 0.6 + 0 * 0.875)
		{
			iAnim = GLOCK_IDLE1;
			m_iTimeWeaponIdle = 3750;
		}
		else
		{
			iAnim = GLOCK_IDLE2;
			m_iTimeWeaponIdle = 2500;
		}
		SendWeaponAnim(iAnim);
	}
}






IMPLEMENT_AMMO_CLASS(ammo_glockclip, CGlockAmmo, "models/w_9mmclip.mdl", AMMO_GLOCKCLIP_GIVE, AMMO_9MM, _9MM_MAX_CARRY);
LINK_ENTITY_TO_CLASS(ammo_9mmclip, CGlockAmmo);
