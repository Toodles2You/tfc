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

LINK_ENTITY_TO_CLASS(weapon_shotgun, CShotgun);

void CShotgun::Spawn()
{
	Precache();
	m_iId = WEAPON_SHOTGUN;
	SET_MODEL(ENT(pev), "models/w_shotgun.mdl");

	m_iDefaultAmmo = SHOTGUN_DEFAULT_GIVE;

	FallInit(); // get ready to fall
}


void CShotgun::Precache()
{
	PRECACHE_MODEL("models/v_shotgun.mdl");
	PRECACHE_MODEL("models/w_shotgun.mdl");
	PRECACHE_MODEL("models/p_shotgun.mdl");

	PRECACHE_SOUND("weapons/dbarrel1.wav"); //shotgun
	PRECACHE_SOUND("weapons/sbarrel1.wav"); //shotgun

	PRECACHE_SOUND("weapons/reload1.wav"); // shotgun reload
	PRECACHE_SOUND("weapons/reload3.wav"); // shotgun reload

	PRECACHE_SOUND("weapons/scock1.wav");	 // cock gun

	m_usSingleFire = PRECACHE_EVENT(1, "events/shotgun1.sc");
	m_usDoubleFire = PRECACHE_EVENT(1, "events/shotgun2.sc");
}

bool CShotgun::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->iAmmo1 = AMMO_BUCKSHOT;
	p->iMaxAmmo1 = BUCKSHOT_MAX_CARRY;
	p->iAmmo2 = AMMO_NONE;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SHOTGUN_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SHOTGUN;
	p->iWeight = SHOTGUN_WEIGHT;

	return true;
}



bool CShotgun::Deploy()
{
	if (DefaultDeploy("models/v_shotgun.mdl", "models/p_shotgun.mdl", SHOTGUN_DRAW, "shotgun"))
	{
		m_flPumpTime = -1000;
		return true;
	}
	return false;
}

bool CShotgun::Holster()
{
	return DefaultHolster(SHOTGUN_HOLSTER);
}

void CShotgun::PrimaryAttack()
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
		m_iNextPrimaryAttack = m_iNextSecondaryAttack = 150;
		return;
	}

	m_iClip--;

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL
	if (UTIL_IsDeathmatch())
	{
		m_pPlayer->FireBullets(gSkillData.plrDmgBuckshot, Vector2D(10, 5), 4, 2048);
	}
	else
	{
		m_pPlayer->FireBullets(gSkillData.plrDmgBuckshot, Vector2D(10, 10), 6, 2048);
	}
#endif

	m_pPlayer->PlaybackEvent(m_usSingleFire, 0.0, 0.0, m_pPlayer->random_seed);

	m_pPlayer->CheckAmmoLevel(this);

	m_flPumpTime = 500;

	m_iNextPrimaryAttack = m_iNextSecondaryAttack = 750;
	if (m_iClip != 0)
		m_iTimeWeaponIdle = 5000;
	else
		m_iTimeWeaponIdle = 1000;
	m_fInSpecialReload = 0;
}


void CShotgun::SecondaryAttack()
{
	if (m_iClip < 2)
	{
		PrimaryAttack();
		return;
	}

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_iNextPrimaryAttack = m_iNextSecondaryAttack = 150;
		return;
	}

	m_iClip -= 2;

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL
	if (UTIL_IsDeathmatch())
	{
		m_pPlayer->FireBullets(gSkillData.plrDmgBuckshot, Vector2D(20, 5), 8, 2048);
	}
	else
	{
		m_pPlayer->FireBullets(gSkillData.plrDmgBuckshot, Vector2D(10, 10), 12, 2048);
	}
#endif

	m_pPlayer->PlaybackEvent(m_usDoubleFire, 0.0, 0.0, m_pPlayer->random_seed);

	m_pPlayer->CheckAmmoLevel(this);

	m_flPumpTime = 950;

	m_iNextPrimaryAttack = m_iNextSecondaryAttack = 1500;
	if (m_iClip != 0)
		m_iTimeWeaponIdle = 6000;
	else
		m_iTimeWeaponIdle = 1500;

	m_fInSpecialReload = 0;
}


void CShotgun::Reload()
{
	if (m_pPlayer->m_rgAmmo[iAmmo1()] <= 0 || m_iClip == SHOTGUN_MAX_CLIP)
		return;

	// don't reload until recoil is done
	if (m_iNextPrimaryAttack > 0)
		return;

	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		SendWeaponAnim(SHOTGUN_START_RELOAD);
		m_fInSpecialReload = 1;
		m_pPlayer->m_iNextAttack = 600;
		m_iTimeWeaponIdle = 600;
		m_iNextPrimaryAttack = m_iNextSecondaryAttack = 1000;
		return;
	}
	else if (m_fInSpecialReload == 1)
	{
		if (m_iTimeWeaponIdle > 0)
			return;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		if (UTIL_SharedRandomLong(m_pPlayer->random_seed, 0, 1) == 1)
			PlayWeaponSound(CHAN_ITEM, "weapons/reload1.wav");
		else
			PlayWeaponSound(CHAN_ITEM, "weapons/reload3.wav");

		SendWeaponAnim(SHOTGUN_RELOAD);
		m_iTimeWeaponIdle = 500;
	}
	else
	{
		if (m_iTimeWeaponIdle > 0)
			return;
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[iAmmo1()] -= 1;
		m_fInSpecialReload = 1;
	}
}


void CShotgun::WeaponIdle()
{
	if (m_fInSpecialReload != 0)
		return;

	if (m_iTimeWeaponIdle > 0)
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.8)
	{
		iAnim = SHOTGUN_IDLE_DEEP;
		m_iTimeWeaponIdle = 5000;
	}
	else if (flRand <= 0.95)
	{
		iAnim = SHOTGUN_IDLE;
		m_iTimeWeaponIdle = 2222;
	}
	else
	{
		iAnim = SHOTGUN_IDLE4;
		m_iTimeWeaponIdle = 2222;
	}
	SendWeaponAnim(iAnim);
}

void CShotgun::ItemPostFrame()
{
	if (m_flPumpTime != -1000 && m_flPumpTime <= 0)
	{
		// play cocking sound
		PlayWeaponSound(CHAN_ITEM, "weapons/scock1.wav");
		m_flPumpTime = -1000;
	}

	if (m_fInSpecialReload != 0 && m_iTimeWeaponIdle <= 0)
	{
		if (m_iClip < SHOTGUN_MAX_CLIP && m_pPlayer->m_rgAmmo[iAmmo1()] > 0)
		{
			Reload();
		}
		else
		{
			// reload debounce has timed out
			SendWeaponAnim(SHOTGUN_PUMP);

			// play cocking sound
			PlayWeaponSound(CHAN_ITEM, "weapons/scock1.wav");
			m_fInSpecialReload = 0;
			m_iTimeWeaponIdle = 1500;
		}
	}

	CBasePlayerWeapon::ItemPostFrame();
}


void CShotgun::GetWeaponData(weapon_data_t& data)
{
	data.m_flPumpTime = m_flPumpTime;
}

void CShotgun::SetWeaponData(const weapon_data_t& data)
{
	m_flPumpTime = data.m_flPumpTime;
}

void CShotgun::DecrementTimers(const int msec)
{
	if (m_flPumpTime > 0)
	{
		m_flPumpTime = std::max(m_flPumpTime - msec, -1.0F);
	}
}

IMPLEMENT_AMMO_CLASS(ammo_buckshot, CShotgunAmmo, "models/w_shotbox.mdl", AMMO_BUCKSHOTBOX_GIVE, AMMO_BUCKSHOT, BUCKSHOT_MAX_CARRY);
