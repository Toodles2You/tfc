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


#define HANDGRENADE_PRIMARY_VOLUME 450

LINK_ENTITY_TO_CLASS(weapon_handgrenade, CHandGrenade);


bool CHandGrenade::Spawn()
{
	Precache();
	m_iId = WEAPON_HANDGRENADE;
	SetModel("models/w_grenade.mdl");

#ifndef CLIENT_DLL
	pev->dmg = gSkillData.plrDmgHandGrenade;
#endif

	m_iDefaultAmmo = HANDGRENADE_DEFAULT_GIVE;

	FallInit(); // get ready to fall down.

	return true;
}


void CHandGrenade::Precache()
{
	PRECACHE_MODEL("models/w_grenade.mdl");
	PRECACHE_MODEL("models/v_grenade.mdl");
	PRECACHE_MODEL("models/p_grenade.mdl");
}

bool CHandGrenade::GetWeaponInfo(WeaponInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->iAmmo1 = AMMO_HANDGRENADES;
	p->iMaxAmmo1 = HANDGRENADE_MAX_CARRY;
	p->iAmmo2 = AMMO_NONE;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_HANDGRENADE;
	p->iWeight = HANDGRENADE_WEIGHT;
	p->iFlags = WEAPON_FLAG_LIMITINWORLD | WEAPON_FLAG_EXHAUSTIBLE;

	return true;
}


bool CHandGrenade::Deploy()
{
	m_flReleaseThrow = -1;
	return DefaultDeploy("models/v_grenade.mdl", "models/p_grenade.mdl", HANDGRENADE_DRAW, "crowbar");
}

bool CHandGrenade::CanHolster()
{
	// can only holster hand grenades when not primed!
	return (m_flStartThrow == 0);
}

bool CHandGrenade::Holster()
{
	const auto bHasAmmo = m_pPlayer->m_rgAmmo[iAmmo1()] != 0;
	
	if (DefaultHolster(bHasAmmo ? HANDGRENADE_HOLSTER : -1))
	{
		//Stop any throw that was in process so players don't blow themselves or somebody else up when the weapon is deployed again.
		m_flStartThrow = 0;
		if (!bHasAmmo)
		{
			// no more grenades!
			m_pPlayer->ClearWeaponBit(m_iId);
			SetThink(&CHandGrenade::DestroyWeapon);
			pev->nextthink = gpGlobals->time + 0.1;
		}
		return true;
	}

	return false;
}

void CHandGrenade::PrimaryAttack()
{
	if (0 == m_flStartThrow && m_pPlayer->m_rgAmmo[iAmmo1()] > 0)
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0;

		SendWeaponAnim(HANDGRENADE_PINPULL);
		m_iTimeWeaponIdle = 500;
	}
}


void CHandGrenade::WeaponIdle()
{
	if (m_flReleaseThrow == 0 && 0 != m_flStartThrow)
		m_flReleaseThrow = gpGlobals->time;

	if (m_iTimeWeaponIdle > 0)
		return;

	if (0 != m_flStartThrow)
	{
		Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		if (angThrow.x < 0)
			angThrow.x = -10 + angThrow.x * ((90 - 10) / 90.0);
		else
			angThrow.x = -10 + angThrow.x * ((90 + 10) / 90.0);

		float flVel = (90 - angThrow.x) * 4;
		if (flVel > 500)
			flVel = 500;

		util::MakeVectors(angThrow);

		Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16;

		Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;

		// alway explode 3 seconds after the pin was pulled
		float time = m_flStartThrow - gpGlobals->time + 3.0;
		if (time < 0)
			time = 0;

		CGrenade::ShootTimed(m_pPlayer, vecSrc, vecThrow, time);

		if (flVel < 500)
		{
			SendWeaponAnim(HANDGRENADE_THROW1);
		}
		else if (flVel < 1000)
		{
			SendWeaponAnim(HANDGRENADE_THROW2);
		}
		else
		{
			SendWeaponAnim(HANDGRENADE_THROW3);
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		//m_flReleaseThrow = 0;
		m_flStartThrow = 0;
		m_iNextPrimaryAttack = 500;
		m_iTimeWeaponIdle = 500;

		m_pPlayer->m_rgAmmo[iAmmo1()]--;

		if (0 == m_pPlayer->m_rgAmmo[iAmmo1()])
		{
			// just threw last grenade
			// set attack times in the future, and weapon idle in the future so we can see the whole throw
			// animation, weapon idle will automatically retire the weapon for us.
			m_iNextSecondaryAttack = m_iNextPrimaryAttack = 500;
			m_iTimeWeaponIdle = 500; // ensure that the animation can finish playing
		}
		return;
	}
	else if (m_flReleaseThrow > 0)
	{
		// we've finished the throw, restart.
		m_flStartThrow = 0;

		if (0 != m_pPlayer->m_rgAmmo[iAmmo1()])
		{
			SendWeaponAnim(HANDGRENADE_DRAW);
		}
		else
		{
			RetireWeapon();
			return;
		}

		m_iTimeWeaponIdle = util::SharedRandomLong(m_pPlayer->random_seed, 10000, 15000);
		m_flReleaseThrow = -1;
		return;
	}

	if (0 != m_pPlayer->m_rgAmmo[iAmmo1()])
	{
		int iAnim;
		float flRand = util::SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
		if (flRand <= 0.75)
		{
			iAnim = HANDGRENADE_IDLE;
			m_iTimeWeaponIdle = util::SharedRandomLong(m_pPlayer->random_seed, 10000, 15000); // how long till we do this again.
		}
		else
		{
			iAnim = HANDGRENADE_FIDGET;
			m_iTimeWeaponIdle = 2500;
		}

		SendWeaponAnim(iAnim);
	}
}
