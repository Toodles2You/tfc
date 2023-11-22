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
#include "gamerules.h"
#include "UserMessages.h"

LINK_ENTITY_TO_CLASS(weapon_python, CPython);
LINK_ENTITY_TO_CLASS(weapon_357, CPython);

bool CPython::GetWeaponInfo(WeaponInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->iAmmo1 = AMMO_357;
	p->iMaxAmmo1 = _357_MAX_CARRY;
	p->iAmmo2 = AMMO_NONE;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = PYTHON_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_PYTHON;
	p->iWeight = PYTHON_WEIGHT;

	return true;
}

bool CPython::Spawn()
{
	pev->classname = MAKE_STRING("weapon_357"); // hack to allow for old names
	Precache();
	m_iId = WEAPON_PYTHON;
	SetModel("models/w_357.mdl");

	m_iDefaultAmmo = PYTHON_DEFAULT_GIVE;

	FallInit(); // get ready to fall down.

	return true;
}


void CPython::Precache()
{
	PRECACHE_MODEL("models/v_357.mdl");
	PRECACHE_MODEL("models/w_357.mdl");
	PRECACHE_MODEL("models/p_357.mdl");

	PRECACHE_MODEL("models/w_357ammobox.mdl");

	PRECACHE_SOUND("weapons/357_shot1.wav");
	PRECACHE_SOUND("weapons/357_shot2.wav");

	m_usFirePython = PRECACHE_EVENT(1, "events/python.sc");
}

bool CPython::Deploy()
{
	if (util::IsDeathmatch())
	{
		// enable laser sight geometry.
		pev->body = 1;
	}
	else
	{
		pev->body = 0;
	}

	return DefaultDeploy("models/v_357.mdl", "models/p_357.mdl", PYTHON_DRAW, "python", pev->body);
}


bool CPython::Holster()
{
	return DefaultHolster(PYTHON_HOLSTER);
}

void CPython::SecondaryAttack()
{
	if (!util::IsDeathmatch())
	{
		return;
	}

	if (m_pPlayer->m_iFOV != 0)
	{
		m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
	}
	else if (m_pPlayer->m_iFOV != 40)
	{
		m_pPlayer->m_iFOV = 40;
	}

	m_iNextSecondaryAttack = 200;
}

void CPython::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_iNextPrimaryAttack = 150;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_iNextPrimaryAttack = 150;
		return;
	}

	m_iClip--;

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL
	m_pPlayer->FireBullets(gSkillData.plrDmg357, Vector2D(1, 1));
#endif

	m_pPlayer->PlaybackEvent(m_usFirePython, 0.0, 0.0, m_pPlayer->random_seed);

	m_pPlayer->CheckAmmoLevel(this);

	m_iNextPrimaryAttack = 750;
	m_iTimeWeaponIdle = util::SharedRandomLong(m_pPlayer->random_seed, 10000, 15000);
}


void CPython::Reload()
{
	if (DefaultReload(PYTHON_MAX_CLIP, PYTHON_RELOAD, 2000, util::IsDeathmatch() ? 1 : 0))
	{
		m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
	}
}


void CPython::WeaponIdle()
{
	if (m_iTimeWeaponIdle > 0)
		return;

	int iAnim;
	float flRand = util::SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.5)
	{
		iAnim = PYTHON_IDLE1;
		m_iTimeWeaponIdle = 2333;
	}
	else if (flRand <= 0.7)
	{
		iAnim = PYTHON_IDLE2;
		m_iTimeWeaponIdle = 2000;
	}
	else if (flRand <= 0.9)
	{
		iAnim = PYTHON_IDLE3;
		m_iTimeWeaponIdle = 2933;
	}
	else
	{
		iAnim = PYTHON_FIDGET;
		m_iTimeWeaponIdle = 5667;
	}

	SendWeaponAnim(iAnim, util::IsDeathmatch() ? 1 : 0);
}

IMPLEMENT_AMMO_CLASS(ammo_357, CPythonAmmo, "models/w_357ammobox.mdl", AMMO_357BOX_GIVE, AMMO_357, _357_MAX_CARRY);
