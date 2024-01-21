//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
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


void CTFWeapon::Precache()
{
	WeaponInfo info;

	GetWeaponInfo(info);

	CBasePlayerWeapon::Precache();

#ifdef GAME_DLL
	g_engfuncs.pfnPrecacheSound(info.pszAttackSound);
#endif

	m_usPrimaryAttack = g_engfuncs.pfnPrecacheEvent(1, info.pszEvent);
}


void CTFWeapon::PrimaryAttack()
{
	const auto info = GetInfo();
	int rounds = 0;

	while (m_iNextPrimaryAttack <= 0)
	{
		m_iNextPrimaryAttack += info.iAttackTime;
		rounds++;
	}

	if (info.iShots * rounds > m_iClip)
	{
		rounds = m_iClip / info.iShots;
	}

	m_iClip -= info.iShots * rounds;

	m_pPlayer->SetAction(CBasePlayer::Action::Attack);

#ifdef GAME_DLL
	m_pPlayer->FireBullets(info.iProjectileDamage, info.vecProjectileSpread, info.iProjectileCount * rounds, 2048);
#endif

	m_pPlayer->PlaybackEvent(m_usPrimaryAttack, (float)GetID(), 0.0F, m_pPlayer->m_randomSeed, rounds);
}


void CTFWeapon::WeaponPostFrame()
{
	const auto info = GetInfo();

	if (m_fInReload)
	{
		if ((m_pPlayer->pev->button & IN_ATTACK) != 0 && m_iClip >= info.iShots)
		{
			m_fInReload = false;
			m_iNextPrimaryAttack = 0;
			PrimaryAttack();
		}
		else if (m_iNextPrimaryAttack <= 0)
		{
			if ((m_iWeaponState & kWpnStateReloading) == 0)
			{
				m_iWeaponState |= kWpnStateReloading;
			}
			else
			{
				m_iClip = std::min(m_iClip + info.iShots, info.iMaxClip);
			}

			if (m_iClip < info.iMaxClip)
			{
				m_pPlayer->SetAction(CBasePlayer::Action::Reload);

				SendWeaponAnim(info.iAnims[kWeaponAnimReload]);

				m_iNextPrimaryAttack = info.iReloadTime / (info.iMaxClip / info.iShots);
			}
			else
			{
				SendWeaponAnim(info.iAnims[kWeaponAnimEndReload]);

				m_fInReload = false;
			}
		}
	}
	else if (m_iNextPrimaryAttack <= 0)
	{
		if ((m_pPlayer->pev->button & IN_ATTACK) != 0 && m_iClip >= info.iShots)
		{
			PrimaryAttack();
		}
		else if (m_iClip < info.iMaxClip)
		{
			SendWeaponAnim(info.iAnims[kWeaponAnimStartReload]);

			m_iWeaponState &= ~kWpnStateReloading;
			m_fInReload = true;
			m_iNextPrimaryAttack = 100;
		}
		else
		{
			m_iNextPrimaryAttack = std::max(m_iNextPrimaryAttack, 0);
		}
	}
}


LINK_ENTITY_TO_CLASS(tf_weapon_shotgun, CShotgun);

void CShotgun::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_shotgun";
	i.iAmmo1 = AMMO_NONE;
	i.iMaxAmmo1 = -1;
	i.iAmmo2 = AMMO_NONE;
	i.iMaxAmmo2 = -1;
	i.iMaxClip = 8;
	i.iSlot = 2;
	i.iPosition = 0;
	i.iFlags = 0;
	i.iWeight = 15;

	i.pszWorld = "models/w_9mmAR.mdl";
	i.pszView = "models/v_tfc_12gauge.mdl";
	i.pszPlayer = "models/p_smallshotgun.mdl";
	i.pszAnimExt = "shotgun";

	i.iAnims[kWeaponAnimIdle] = 0;
	i.iAnims[kWeaponAnimDeploy] = 6;
	i.iAnims[kWeaponAnimHolster] = 7;
	i.iAnims[kWeaponAnimAttack] = 1;
	i.iAnims[kWeaponAnimReload] = 3;
	i.iAnims[kWeaponAnimStartReload] = 5;
	i.iAnims[kWeaponAnimEndReload] = 4;

	i.iShots = 1;

	i.iAttackTime = 500;
	i.iReloadTime = 2000;

	i.iProjectileDamage = 4;
	i.vecProjectileSpread = Vector2D(2.3F, 2.3F);
	i.iProjectileCount = 6;

	i.pszEvent = "events/wpn/tf_sg.sc";
	i.pszAttackSound = "weapons/sbarrel1.wav";
	i.flPunchAngle = -2.0F;
}


LINK_ENTITY_TO_CLASS(tf_weapon_supershotgun, CSuperShotgun);

void CSuperShotgun::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_supershotgun";
	i.iAmmo1 = AMMO_NONE;
	i.iMaxAmmo1 = -1;
	i.iAmmo2 = AMMO_NONE;
	i.iMaxAmmo2 = -1;
	i.iMaxClip = 16;
	i.iSlot = 2;
	i.iPosition = 1;
	i.iFlags = 0;
	i.iWeight = 15;

	i.pszWorld = "models/w_9mmAR.mdl";
	i.pszView = "models/v_tfc_shotgun.mdl";
	i.pszPlayer = "models/p_shotgun.mdl";
	i.pszAnimExt = "shotgun";

	i.iAnims[kWeaponAnimIdle] = 0;
	i.iAnims[kWeaponAnimDeploy] = 6;
	i.iAnims[kWeaponAnimHolster] = 7;
	i.iAnims[kWeaponAnimAttack] = 1;
	i.iAnims[kWeaponAnimReload] = 3;
	i.iAnims[kWeaponAnimStartReload] = 5;
	i.iAnims[kWeaponAnimEndReload] = 4;

	i.iShots = 2;

	i.iAttackTime = 700;
	i.iReloadTime = 3000;

	i.iProjectileDamage = 4;
	i.vecProjectileSpread = Vector2D(8.0F, 4.6F);
	i.iProjectileCount = 14;

	i.pszEvent = "events/wpn/tf_ssg.sc";
	i.pszAttackSound = "weapons/shotgn2.wav";
	i.flPunchAngle = -4.0F;
}

