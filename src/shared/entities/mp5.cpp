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
#ifdef GAME_DLL
#include "gamerules.h"
#endif


LINK_ENTITY_TO_CLASS(weapon_9mmAR, CMP5);


void CMP5::Precache()
{
	CBasePlayerWeapon::Precache();

#ifdef GAME_DLL
	g_engfuncs.pfnPrecacheModel("models/w_9mmARclip.mdl");

	g_engfuncs.pfnPrecacheSound("weapons/hks1.wav");
	g_engfuncs.pfnPrecacheSound("weapons/hks2.wav");
	g_engfuncs.pfnPrecacheSound("weapons/hks3.wav");

	g_engfuncs.pfnPrecacheSound("weapons/glauncher.wav");
	g_engfuncs.pfnPrecacheSound("weapons/glauncher2.wav");
#endif

	m_usPrimaryAttack = g_engfuncs.pfnPrecacheEvent(1, "events/mp5.sc");
	m_usSecondaryAttack = g_engfuncs.pfnPrecacheEvent(1, "events/mp52.sc");
}


void CMP5::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "weapon_9mmAR";
	i.iAmmo1 = AMMO_NONE;
	i.iMaxAmmo1 = -1;
	i.iAmmo2 = AMMO_NONE;
	i.iMaxAmmo2 = -1;
	i.iMaxClip = 30;
	i.iSlot = 2;
	i.iPosition = 0;
	i.iFlags = 0;
	i.iWeight = 15;

	i.pszWorld = "models/w_9mmAR.mdl";
	i.pszView = "models/v_9mmAR.mdl";
	i.pszPlayer = "models/p_9mmAR.mdl";
	i.pszAnimExt = "mp5";

	i.iAnims[kWeaponAnimIdle] = kAnimIdle1;
	i.iAnims[kWeaponAnimDeploy] = kAnimDeploy;
	i.iAnims[kWeaponAnimHolster] = kAnimHolster;
}


void CMP5::PrimaryAttack()
{
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

	m_iClip -= shots;

	m_pPlayer->SetAction(CBasePlayer::Action::Attack);

#ifdef GAME_DLL
	m_pPlayer->FireBullets(8, Vector2D(3, 3), shots);
#endif

	m_pPlayer->PlaybackEvent(m_usPrimaryAttack, 0.0F, 0.0F, m_pPlayer->m_randomSeed, shots);
}


void CMP5::SecondaryAttack()
{
	m_pPlayer->SetAction(CBasePlayer::Action::Attack);

#ifndef CLIENT_DLL
	util::MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	int rightOffset = 4;

	if (m_pPlayer->m_bLeftHanded)
	{
		rightOffset = -4;
	}

	CGrenade::ShootContact(m_pPlayer,
		m_pPlayer->pev->origin
			+ m_pPlayer->pev->view_ofs
			+ gpGlobals->v_right * rightOffset
			+ gpGlobals->v_forward * 16,
		gpGlobals->v_forward * 800);
#endif

	m_pPlayer->PlaybackEvent(m_usSecondaryAttack);

	m_iNextPrimaryAttack = 1000;
}


void CMP5::WeaponPostFrame()
{
	if ((m_pPlayer->pev->button & IN_ATTACK2) != 0)
	{
		if (m_iNextPrimaryAttack <= 0 || m_fInReload)
		{
			SecondaryAttack();
			m_fInReload = false;
			return;
		}
	}

	if (m_iNextPrimaryAttack <= 0)
	{
		if (m_fInReload)
		{
			m_iClip = iMaxClip();
			m_fInReload = false;
		}
		else if (m_iClip == 0
		 || ((m_pPlayer->pev->button & IN_RELOAD) != 0 && m_iClip < iMaxClip()))
		{
			SendWeaponAnim(kAnimReload);
			m_pPlayer->SetAction(CBasePlayer::Action::Reload);
			m_fInReload = true;
			m_iNextPrimaryAttack = 1500;
			return;
		}

		if ((m_pPlayer->pev->button & IN_ATTACK) != 0 && m_iClip != 0)
		{
			PrimaryAttack();
		}
		else
		{
			m_iNextPrimaryAttack = std::max(m_iNextPrimaryAttack, 0);
		}
	}
}

