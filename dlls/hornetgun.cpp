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
#include "hornet.h"
#include "gamerules.h"
#include "UserMessages.h"

enum firemode_e
{
	FIREMODE_TRACK = 0,
	FIREMODE_FAST
};


LINK_ENTITY_TO_CLASS(weapon_hornetgun, CHgun);

bool CHgun::IsUseable()
{
	return true;
}

void CHgun::Spawn()
{
	Precache();
	m_iId = WEAPON_HORNETGUN;
	SET_MODEL(ENT(pev), "models/w_hgun.mdl");

	m_iDefaultAmmo = HIVEHAND_DEFAULT_GIVE;
	m_iFirePhase = 0;

	FallInit(); // get ready to fall down.
}


void CHgun::Precache()
{
	PRECACHE_MODEL("models/v_hgun.mdl");
	PRECACHE_MODEL("models/w_hgun.mdl");
	PRECACHE_MODEL("models/p_hgun.mdl");

	m_usHornetFire = PRECACHE_EVENT(1, "events/firehornet.sc");

	UTIL_PrecacheOther("hornet");
}

void CHgun::AddToPlayer(CBasePlayer* pPlayer)
{
	if (UTIL_IsDeathmatch())
	{
		// in multiplayer, all hivehands come full.
		m_iDefaultAmmo = HORNET_MAX_CARRY;
	}

	CBasePlayerWeapon::AddToPlayer(pPlayer);
}

bool CHgun::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->iAmmo1 = AMMO_HORNETS;
	p->iMaxAmmo1 = HORNET_MAX_CARRY;
	p->iAmmo2 = AMMO_NONE;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_HORNETGUN;
	p->iFlags = ITEM_FLAG_SELECTONEMPTY | ITEM_FLAG_NOAUTOSWITCHEMPTY | ITEM_FLAG_NOAUTORELOAD;
	p->iWeight = HORNETGUN_WEIGHT;

	return true;
}


bool CHgun::Deploy()
{
	if (DefaultDeploy("models/v_hgun.mdl", "models/p_hgun.mdl", HGUN_UP, "hive"))
	{
		// Let ItemPostFrame run so the hornets start recharging.
		m_pPlayer->m_iNextAttack = 0;
		return true;
	}
	return false;
}

bool CHgun::Holster()
{
	return DefaultHolster(HGUN_DOWN);
}


void CHgun::PrimaryAttack()
{
	Reload();

	if (m_pPlayer->m_rgAmmo[iAmmo1()] <= 0)
	{
		return;
	}

#ifndef CLIENT_DLL
	UTIL_MakeVectors(m_pPlayer->pev->v_angle);

	CBaseEntity* pHornet = CBaseEntity::Create("hornet", m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -12, m_pPlayer->pev->v_angle, m_pPlayer->edict());
	pHornet->pev->velocity = gpGlobals->v_forward * 300;

	m_flRechargeTime = gpGlobals->time + 0.5;
#endif

	m_pPlayer->m_rgAmmo[iAmmo1()]--;

	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usHornetFire, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 0, 0, 0, 0);



	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_iNextPrimaryAttack = m_iNextSecondaryAttack = 250;

	m_iTimeWeaponIdle = UTIL_SharedRandomLong(m_pPlayer->random_seed, 10000, 15000);
}



void CHgun::SecondaryAttack()
{
	Reload();

	if (m_pPlayer->m_rgAmmo[iAmmo1()] <= 0)
	{
		return;
	}

	//Wouldn't be a bad idea to completely predict these, since they fly so fast...
#ifndef CLIENT_DLL
	CBaseEntity* pHornet;
	Vector vecSrc;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);

	vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -12;

	m_iFirePhase++;
	switch (m_iFirePhase)
	{
	case 1:
		vecSrc = vecSrc + gpGlobals->v_up * 8;
		break;
	case 2:
		vecSrc = vecSrc + gpGlobals->v_up * 8;
		vecSrc = vecSrc + gpGlobals->v_right * 8;
		break;
	case 3:
		vecSrc = vecSrc + gpGlobals->v_right * 8;
		break;
	case 4:
		vecSrc = vecSrc + gpGlobals->v_up * -8;
		vecSrc = vecSrc + gpGlobals->v_right * 8;
		break;
	case 5:
		vecSrc = vecSrc + gpGlobals->v_up * -8;
		break;
	case 6:
		vecSrc = vecSrc + gpGlobals->v_up * -8;
		vecSrc = vecSrc + gpGlobals->v_right * -8;
		break;
	case 7:
		vecSrc = vecSrc + gpGlobals->v_right * -8;
		break;
	case 8:
		vecSrc = vecSrc + gpGlobals->v_up * 8;
		vecSrc = vecSrc + gpGlobals->v_right * -8;
		m_iFirePhase = 0;
		break;
	}

	pHornet = CBaseEntity::Create("hornet", vecSrc, m_pPlayer->pev->v_angle, m_pPlayer->edict());
	pHornet->pev->velocity = gpGlobals->v_forward * 1200;
	pHornet->pev->angles = UTIL_VecToAngles(pHornet->pev->velocity);

	pHornet->SetThink(&CHornet::StartDart);

	m_flRechargeTime = gpGlobals->time + 0.5;
#endif

	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usHornetFire, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 0, 0, 0, 0);


	m_pPlayer->m_rgAmmo[iAmmo1()]--;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_iNextPrimaryAttack = m_iNextSecondaryAttack = 100;
	m_iTimeWeaponIdle = UTIL_SharedRandomLong(m_pPlayer->random_seed, 10000, 15000);
}


void CHgun::Reload()
{
#ifndef CLIENT_DLL
	if (m_pPlayer->m_rgAmmo[iAmmo1()] >= HORNET_MAX_CARRY)
		return;

	while (m_pPlayer->m_rgAmmo[iAmmo1()] < HORNET_MAX_CARRY && m_flRechargeTime < gpGlobals->time)
	{
		m_pPlayer->m_rgAmmo[iAmmo1()]++;
		m_flRechargeTime += 0.5;
	}
#endif
}


void CHgun::WeaponIdle()
{
	Reload();

	if (m_iTimeWeaponIdle > 0)
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.75)
	{
		iAnim = HGUN_IDLE1;
		m_iTimeWeaponIdle = 3750;
	}
	else if (flRand <= 0.875)
	{
		iAnim = HGUN_FIDGETSWAY;
		m_iTimeWeaponIdle = 2500;
	}
	else
	{
		iAnim = HGUN_FIDGETSHAKE;
		m_iTimeWeaponIdle = 2188;
	}
	SendWeaponAnim(iAnim);
}
