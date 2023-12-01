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
	m_iDefaultAmmo = 30;
	return true;
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
#ifdef GAME_DLL
	i->pszName = STRING(pev->classname);
#else
	i->pszName = "weapon_9mmAR";
#endif
	i->iAmmo1 = AMMO_9MM;
	i->iMaxAmmo1 = 250;
	i->iAmmo2 = AMMO_ARGRENADES;
	i->iMaxAmmo2 = 10;
	i->iMaxClip = 30;
	i->iSlot = 2;
	i->iPosition = 0;
	i->iFlags = 0;
	i->iId = m_iId = WEAPON_MP5;
	i->iWeight = 15;

	i->pszView = "models/v_9mmAR.mdl";
	i->pszPlayer = "models/p_9mmAR.mdl";
	i->pszAnimExt = "mp5";

	i->iAnims[kWeaponAnimIdle] = kAnimIdle1;
	i->iAnims[kWeaponAnimDeploy] = kAnimDeploy;
	i->iAnims[kWeaponAnimHolster] = kAnimHolster;

	return true;
}


void CMP5::PrimaryAttack()
{
	if (m_pPlayer->pev->waterlevel == 3 || m_iClip == 0)
	{
		m_iNextPrimaryAttack = 150;
		return;
	}

	auto shots = 0;

	while (m_iNextPrimaryAttack <= 0)
	{
		m_iNextPrimaryAttack += 80;
		shots++;
	}

	if (shots > m_iClip)
	{
		shots = m_iClip;
	}

	// m_iClip -= shots;

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifdef GAME_DLL
	m_pPlayer->FireBullets(8, Vector2D(3, 3), shots);
#endif

	m_pPlayer->PlaybackEvent(m_usMP5, 0.0F, 0.0F, m_pPlayer->m_randomSeed, shots);
}



void CMP5::SecondaryAttack()
{
	if (m_pPlayer->pev->waterlevel == 3 || m_pPlayer->m_rgAmmo[iAmmo2()] == 0)
	{
		m_iNextPrimaryAttack = 150;
		return;
	}

	// m_pPlayer->m_rgAmmo[iAmmo2()]--;

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL
	util::MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	CGrenade::ShootContact(m_pPlayer,
		m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16,
		gpGlobals->v_forward * 800);
#endif

	m_pPlayer->PlaybackEvent(m_usMP52);

	m_iNextPrimaryAttack = 1000;
}



void CMP5::WeaponPostFrame()
{
	if (m_iNextPrimaryAttack <= 0)
	{
		if ((m_pPlayer->pev->button & IN_ATTACK2) != 0)
		{
			SecondaryAttack();
		}
		else if ((m_pPlayer->pev->button & IN_ATTACK) != 0)
		{
			PrimaryAttack();
		}
	}
}

