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
#include "soundent.h"
#include "gamerules.h"
#include "UserMessages.h"

LINK_ENTITY_TO_CLASS(weapon_mp5, CMP5);
LINK_ENTITY_TO_CLASS(weapon_9mmAR, CMP5);


//=========================================================
//=========================================================
void CMP5::Spawn()
{
	pev->classname = MAKE_STRING("weapon_9mmAR"); // hack to allow for old names
	Precache();
	SET_MODEL(ENT(pev), "models/w_9mmAR.mdl");
	m_iId = WEAPON_MP5;

	m_iDefaultAmmo = MP5_DEFAULT_GIVE;

	FallInit(); // get ready to fall down.
}

bool CMP5::AddDuplicate(CBasePlayerItem* pOriginal)
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

	PRECACHE_SOUND("weapons/357_cock1.wav");

	m_usMP5 = PRECACHE_EVENT(1, "events/mp5.sc");
	m_usMP52 = PRECACHE_EVENT(1, "events/mp52.sc");
}

bool CMP5::GetItemInfo(ItemInfo* p)
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

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

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


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	Vector vecDir;

	if (UTIL_IsDeathmatch())
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		vecDir = m_pPlayer->FireBulletsPlayer(shots, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	}
	else
	{
		// single player spread
		vecDir = m_pPlayer->FireBulletsPlayer(shots, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	}

	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usMP5, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, m_pPlayer->random_seed, shots, 0, 0);

	m_pPlayer->CheckAmmoLevel(this);

	m_iTimeWeaponIdle = UTIL_SharedRandomLong(m_pPlayer->random_seed, 10000, 15000);
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

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
	m_pPlayer->m_flStopExtraSoundTime = UTIL_WeaponTimeBase() + 0.2;

	m_pPlayer->m_rgAmmo[iAmmo2()]--;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	// we don't add in player velocity anymore.
	CGrenade::ShootContact(m_pPlayer->pev,
		m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16,
		gpGlobals->v_forward * 800);

	PLAYBACK_EVENT(FEV_NOTHOST, m_pPlayer->edict(), m_usMP52);

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
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

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

	m_iTimeWeaponIdle = UTIL_SharedRandomLong(m_pPlayer->random_seed, 10000, 15000); // how long till we do this again.
}


IMPLEMENT_AMMO_CLASS(ammo_mp5clip, CMP5AmmoClip, "models/w_9mmARclip.mdl", AMMO_MP5CLIP_GIVE, AMMO_9MM, _9MM_MAX_CARRY);
LINK_ENTITY_TO_CLASS(ammo_9mmAR, CMP5AmmoClip);

IMPLEMENT_AMMO_CLASS(ammo_9mmbox, CMP5Chainammo, "models/w_chainammo.mdl", AMMO_CHAINBOX_GIVE, AMMO_9MM, _9MM_MAX_CARRY);

IMPLEMENT_AMMO_CLASS(ammo_mp5grenades, CMP5AmmoGrenade, "models/w_ARgrenade.mdl", AMMO_M203BOX_GIVE, AMMO_ARGRENADES, M203_GRENADE_MAX_CARRY);
LINK_ENTITY_TO_CLASS(ammo_ARgrenades, CMP5AmmoGrenade);
