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

LINK_ENTITY_TO_CLASS(weapon_mp5, CMP5);
LINK_ENTITY_TO_CLASS(weapon_9mmAR, CMP5);


//=========================================================
//=========================================================
bool CMP5::Spawn()
{
	pev->classname = MAKE_STRING("weapon_9mmAR"); // hack to allow for old names
	Precache();
	SetModel("models/w_9mmAR.mdl");
	m_iId = WEAPON_MP5;

	m_iDefaultAmmo = MP5_DEFAULT_GIVE;

	FallInit(); // get ready to fall down.

	return true;
}

bool CMP5::AddDuplicate(CBasePlayerWeapon* pOriginal)
{
	if (m_iDefaultAmmo == MP5_DEFAULT_GIVE)
	{
		m_iDefaultAmmo = (MP5_DEFAULT_GIVE >> 1);
	}
	if (CBasePlayerWeapon::AddDuplicate(pOriginal))
	{
		return true;
	}
	return false;
}

void CMP5::Precache()
{
	PRECACHE_MODEL("models/v_9mmAR.mdl");
	PRECACHE_MODEL("models/w_9mmAR.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");

	PRECACHE_MODEL("models/w_9mmARclip.mdl");

	PRECACHE_SOUND("weapons/hks1.wav"); // H to the K
	PRECACHE_SOUND("weapons/hks2.wav"); // H to the K
	PRECACHE_SOUND("weapons/hks3.wav"); // H to the K

	PRECACHE_SOUND("weapons/glauncher.wav");
	PRECACHE_SOUND("weapons/glauncher2.wav");

	m_usMP5 = PRECACHE_EVENT(1, "events/mp5.sc");
	m_usMP52 = PRECACHE_EVENT(1, "events/mp52.sc");
}

bool CMP5::GetWeaponInfo(WeaponInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->iAmmo1 = AMMO_9MM;
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->iAmmo2 = AMMO_ARGRENADES;
	p->iMaxAmmo2 = M203_GRENADE_MAX_CARRY;
	p->iMaxClip = MP5_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MP5;
	p->iWeight = MP5_WEIGHT;

	return true;
}

bool CMP5::Deploy()
{
	return DefaultDeploy("models/v_9mmAR.mdl", "models/p_9mmAR.mdl", MP5_DEPLOY, "mp5");
}

bool CMP5::Holster()
{
	return DefaultHolster(MP5_HOLSTER);
}

void CMP5::PrimaryAttack()
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

	auto shots = 0;

	while (m_iNextPrimaryAttack <= 0)
	{
		m_iNextPrimaryAttack += 100;
		shots++;
	}

	if (shots > m_iClip)
	{
		shots = m_iClip;
		PlayEmptySound();
	}

	m_iClip -= shots;

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL
	if (util::IsDeathmatch())
	{
		m_pPlayer->FireBullets(gSkillData.plrDmgMP5, Vector2D(6, 6), shots);
	}
	else
	{
		m_pPlayer->FireBullets(gSkillData.plrDmgMP5, Vector2D(3, 3), shots);
	}
#endif

	m_pPlayer->PlaybackEvent(m_usMP5, 0.0, 0.0, m_pPlayer->random_seed, shots);

	m_pPlayer->CheckAmmoLevel(this);

	m_iTimeWeaponIdle = util::SharedRandomLong(m_pPlayer->random_seed, 10000, 15000);
}



void CMP5::SecondaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_iNextPrimaryAttack = 150;
		return;
	}

	if (m_pPlayer->m_rgAmmo[iAmmo2()] == 0)
	{
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_rgAmmo[iAmmo2()]--;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	util::MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	CGrenade::ShootContact(m_pPlayer,
		m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16,
		gpGlobals->v_forward * 800);

	m_pPlayer->PlaybackEvent(m_usMP52);

	m_iNextPrimaryAttack = m_iNextSecondaryAttack = 1000;
	m_iTimeWeaponIdle = 5000; // idle pretty soon after shooting.

	m_pPlayer->CheckAmmoLevel(this, false);
}

void CMP5::Reload()
{
	DefaultReload(MP5_MAX_CLIP, MP5_RELOAD, 1500);
}


void CMP5::WeaponIdle()
{
	if (m_iTimeWeaponIdle > 0)
		return;

	int iAnim;
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		iAnim = MP5_LONGIDLE;
		break;

	default:
	case 1:
		iAnim = MP5_IDLE1;
		break;
	}

	SendWeaponAnim(iAnim);

	m_iTimeWeaponIdle = util::SharedRandomLong(m_pPlayer->random_seed, 10000, 15000); // how long till we do this again.
}


IMPLEMENT_AMMO_CLASS(ammo_mp5clip, CMP5AmmoClip, "models/w_9mmARclip.mdl", AMMO_MP5CLIP_GIVE, AMMO_9MM, _9MM_MAX_CARRY);
LINK_ENTITY_TO_CLASS(ammo_9mmAR, CMP5AmmoClip);

IMPLEMENT_AMMO_CLASS(ammo_9mmbox, CMP5Chainammo, "models/w_chainammo.mdl", AMMO_CHAINBOX_GIVE, AMMO_9MM, _9MM_MAX_CARRY);

IMPLEMENT_AMMO_CLASS(ammo_mp5grenades, CMP5AmmoGrenade, "models/w_ARgrenade.mdl", AMMO_M203BOX_GIVE, AMMO_ARGRENADES, M203_GRENADE_MAX_CARRY);
LINK_ENTITY_TO_CLASS(ammo_ARgrenades, CMP5AmmoGrenade);
