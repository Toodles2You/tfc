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
	engine::PrecacheModel("models/w_9mmARclip.mdl");

	engine::PrecacheSound("weapons/hks1.wav");
	engine::PrecacheSound("weapons/hks2.wav");
	engine::PrecacheSound("weapons/hks3.wav");

	engine::PrecacheSound("weapons/glauncher.wav");
	engine::PrecacheSound("weapons/glauncher2.wav");
#endif

	m_usPrimaryAttack = engine::PrecacheEvent(1, "events/mp5.sc");
	m_usSecondaryAttack = engine::PrecacheEvent(1, "events/mp52.sc");
}


void CMP5::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "weapon_9mmAR";
	i.iAmmo1 = AMMO_9MM;
	i.iAmmo2 = AMMO_ARGRENADES;
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
	m_pPlayer->m_rgAmmo[iAmmo2()]--;

	m_pPlayer->SetAction(CBasePlayer::Action::Attack);

	m_pPlayer->PlaybackEvent(m_usSecondaryAttack);

	m_iNextPrimaryAttack = 1000;

#ifndef CLIENT_DLL
	util::MakeVectors(m_pPlayer->v.v_angle + m_pPlayer->v.punchangle);

	int rightOffset = 4;

	if (m_pPlayer->m_bLeftHanded)
	{
		rightOffset = -4;
	}

	auto grenade =
		CGrenade::ShootContact(m_pPlayer,
			m_pPlayer->v.origin
				+ m_pPlayer->v.view_ofs
				+ gpGlobals->v_right * rightOffset
				+ gpGlobals->v_forward * 16,
			gpGlobals->v_forward * 800);

	util::LagCompensation(grenade, m_pPlayer->m_netPing);
#endif
}


void CMP5::WeaponPostFrame()
{
	if ((m_pPlayer->v.button & IN_ATTACK2) != 0 && m_pPlayer->m_rgAmmo[iAmmo2()] != 0)
	{
		if (m_iNextPrimaryAttack <= 0 || m_fInReload)
		{
			m_fInReload = false;
			SecondaryAttack();
			return;
		}
	}

	if (m_iNextPrimaryAttack <= 0)
	{
		if (m_fInReload)
		{
			int shots = std::min(iMaxClip() - m_iClip, static_cast<int>(m_pPlayer->m_rgAmmo[iAmmo1()]));
			m_pPlayer->m_rgAmmo[iAmmo1()] -= shots;
			m_iClip += shots;
			m_fInReload = false;
		}
		else if ((m_iClip == 0
		 || ((m_pPlayer->v.button & IN_RELOAD) != 0 && m_iClip < iMaxClip()))
		 && m_pPlayer->m_rgAmmo[iAmmo1()] != 0)
		{
			SendWeaponAnim(kAnimReload);
			m_pPlayer->SetAction(CBasePlayer::Action::Reload);
			m_fInReload = true;
			m_iNextPrimaryAttack = 1500;
			return;
		}

		if ((m_pPlayer->v.button & IN_ATTACK) != 0 && m_iClip != 0)
		{
			PrimaryAttack();
		}
		else
		{
			m_iNextPrimaryAttack = std::max(m_iNextPrimaryAttack, 0);
		}
	}
}

