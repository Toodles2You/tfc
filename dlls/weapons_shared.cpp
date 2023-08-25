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
#include "player.h"
#include "weapons.h"
#include "gamerules.h"

bool CBasePlayerWeapon::CanDeploy()
{
	bool bHasAmmo = false;

	if (iAmmo1() <= AMMO_NONE)
	{
		// this weapon doesn't use ammo, can always deploy.
		return true;
	}

	if ((iFlags() & ITEM_FLAG_SELECTONEMPTY) != 0)
	{
		return true;
	}

	if (iAmmo1() > AMMO_NONE)
	{
		bHasAmmo |= (m_pPlayer->m_rgAmmo[iAmmo1()] != 0);
	}
	if (iAmmo2() > AMMO_NONE)
	{
		bHasAmmo |= (m_pPlayer->m_rgAmmo[iAmmo2()] != 0);
	}
	if (m_iClip > 0)
	{
		bHasAmmo |= true;
	}
	if (!bHasAmmo)
	{
		return false;
	}

	return true;
}

bool CBasePlayerWeapon::DefaultReload(int iClipSize, int iAnim, int fDelay, int body)
{
	if (m_pPlayer->m_rgAmmo[iAmmo1()] <= 0)
		return false;

	int j = V_min(iClipSize - m_iClip, m_pPlayer->m_rgAmmo[iAmmo1()]);

	if (j == 0)
		return false;

	m_pPlayer->m_iNextAttack = fDelay;

	//!!UNDONE -- reload sound goes here !!!
	SendWeaponAnim(iAnim, body);

	m_fInReload = true;

	m_iTimeWeaponIdle = 3000;
	return true;
}

bool CanAttack(int attack_time)
{
	return attack_time <= 0 ? true : false;
}

void CBasePlayerWeapon::ItemPostFrame()
{
	if ((m_fInReload) && (m_pPlayer->m_iNextAttack <= 0))
	{
		// complete the reload.
		int j = V_min(iMaxClip() - m_iClip, m_pPlayer->m_rgAmmo[iAmmo1()]);

		// Add them to the clip
		m_iClip += j;
		m_pPlayer->m_rgAmmo[iAmmo1()] -= j;

		m_fInReload = false;

		m_iNextPrimaryAttack = m_iNextSecondaryAttack = 0;
	}

	const auto bCanPrimaryAttack = CanAttack(m_iNextPrimaryAttack);
	const auto bCanSecondaryAttack = CanAttack(m_iNextSecondaryAttack);

	if ((m_pPlayer->pev->button & IN_ATTACK) == 0)
	{
		if (bCanPrimaryAttack)
			m_iNextPrimaryAttack = 0;

		if (bCanSecondaryAttack)
			m_iNextSecondaryAttack = 0;
	}

	if ((m_pPlayer->pev->button & IN_ATTACK2) != 0 && bCanSecondaryAttack)
	{
		SecondaryAttack();
		m_pPlayer->pev->button &= ~IN_ATTACK2;
	}
	else if ((m_pPlayer->pev->button & IN_ATTACK) != 0 && bCanPrimaryAttack)
	{
		PrimaryAttack();
	}
	else if ((m_pPlayer->pev->button & IN_RELOAD) != 0 && iMaxClip() != WEAPON_NOCLIP && !m_fInReload)
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
	}
	else if ((m_pPlayer->pev->button & (IN_ATTACK | IN_ATTACK2)) == 0)
	{
		// no fire buttons down

		m_bPlayEmptySound = true;

		if (!IsUseable() && bCanPrimaryAttack)
		{
#ifndef CLIENT_DLL
			// weapon isn't useable, switch.
			if ((iFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) == 0 && g_pGameRules->GetNextBestWeapon(m_pPlayer, this))
			{
				m_iNextPrimaryAttack = 300;
				return;
			}
#endif
		}
		else
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if (m_iClip == 0 && (iFlags() & ITEM_FLAG_NOAUTORELOAD) == 0 && bCanPrimaryAttack)
			{
				Reload();
				return;
			}
		}

		WeaponIdle();
		return;
	}

	// catch all
	if (ShouldWeaponIdle())
	{
		WeaponIdle();
	}
}
