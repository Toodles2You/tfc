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
#ifdef GAME_DLL
#include "gamerules.h"
#endif


void CBasePlayerWeapon::GetWeaponData(weapon_data_t& data)
{
	data.m_fInReload = m_fInReload;
	data.m_iClip = m_iClip;
	data.m_iWeaponState = m_iWeaponState;

    *reinterpret_cast<int*>(&data.m_flNextPrimaryAttack) = m_iNextPrimaryAttack;
}


void CBasePlayerWeapon::SetWeaponData(const weapon_data_t& data)
{
	m_fInReload = data.m_fInReload;
	m_iClip = data.m_iClip;
	m_iWeaponState = data.m_iWeaponState;

    m_iNextPrimaryAttack = *reinterpret_cast<const int*>(&data.m_flNextPrimaryAttack);
}


void CBasePlayerWeapon::DecrementTimers(const int msec)
{
	m_iNextPrimaryAttack = std::max(m_iNextPrimaryAttack - msec, -1100);
}

