//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Whatever
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"


LINK_ENTITY_TO_CLASS(weapon_9mmAR, CMP5);


bool CMP5::Spawn()
{
	Precache();
	SetModel("models/w_9mmAR.mdl");
	FallInit();
	m_iId = WEAPON_MP5;
	m_iDefaultAmmo = 50;
	return true;
}


bool CMP5::AddDuplicate(CBasePlayerWeapon* original)
{
	if (m_iDefaultAmmo == 50)
	{
		m_iDefaultAmmo = 25;
	}
	return CBasePlayerWeapon::AddDuplicate(original);
}


void CMP5::Precache()
{
	PRECACHE_MODEL("models/v_9mmAR.mdl");
	PRECACHE_MODEL("models/w_9mmAR.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");

	PRECACHE_MODEL("models/w_9mmARclip.mdl");

	PRECACHE_SOUND("weapons/hks1.wav");
	PRECACHE_SOUND("weapons/hks2.wav");
	PRECACHE_SOUND("weapons/hks3.wav");

	PRECACHE_SOUND("weapons/glauncher.wav");
	PRECACHE_SOUND("weapons/glauncher2.wav");

	m_usMP5 = PRECACHE_EVENT(1, "events/mp5.sc");
	m_usMP52 = PRECACHE_EVENT(1, "events/mp52.sc");
}


bool CMP5::GetWeaponInfo(WeaponInfo* i)
{
	i->pszName = STRING(pev->classname);
	i->iAmmo1 = AMMO_9MM;
	i->iMaxAmmo1 = 250;
	i->iAmmo2 = AMMO_ARGRENADES;
	i->iMaxAmmo2 = 10;
	i->iMaxClip = 50;
	i->iSlot = 2;
	i->iPosition = 0;
	i->iFlags = 0;
	i->iId = m_iId = WEAPON_MP5;
	i->iWeight = 15;

	return true;
}


bool CMP5::Deploy()
{
	return DefaultDeploy("models/v_9mmAR.mdl", "models/p_9mmAR.mdl", kAnimDeploy, "mp5");
}


bool CMP5::Holster()
{
	return DefaultHolster(kAnimHolster);
}


void CMP5::PrimaryAttack()
{
	if (m_pPlayer->pev->waterlevel == 3 || m_iClip == 0)
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

#ifdef GAME_DLL
	m_pPlayer->FireBullets(12, Vector2D(6, 6), shots);
#endif

	m_pPlayer->PlaybackEvent(m_usMP5, 0.0F, 0.0F, m_pPlayer->m_randomSeed, shots);
}



void CMP5::SecondaryAttack()
{
	if (m_pPlayer->pev->waterlevel == 3 || m_pPlayer->m_rgAmmo[iAmmo2()] == 0)
	{
		PlayEmptySound();
		m_iNextPrimaryAttack = 150;
		return;
	}

	m_pPlayer->m_rgAmmo[iAmmo2()]--;

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL
	util::MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	CGrenade::ShootContact(m_pPlayer,
		m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16,
		gpGlobals->v_forward * 800);
#endif

	m_pPlayer->PlaybackEvent(m_usMP52);

	m_iNextPrimaryAttack = m_iNextSecondaryAttack = 1000;
}


void CMP5::Reload()
{
	DefaultReload(iMaxClip(), kAnimReload, 1500);
}

