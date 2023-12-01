//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Whatever
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"

#include "usercmd.h"
#include "entity_state.h"
#include "demo_api.h"
#include "pm_defs.h"
#include "event_api.h"
#include "r_efx.h"
#include "pm_shared.h"
#include "gamemovement.h"

#include "cl_dll.h"
#include "com_weapons.h"
#include "demo.h"
#include "hud.h"


void CBasePlayerWeapon::Deploy()
{
	const auto info = GetInfo();

	gEngfuncs.CL_LoadModel(info.pszView, &m_pPlayer->pev->viewmodel);
	gEngfuncs.CL_LoadModel(info.pszPlayer, &m_pPlayer->pev->weaponmodel);
	strcpy(m_pPlayer->m_szAnimExtention, info.pszAnimExt);

	SendWeaponAnim(info.iAnims[kWeaponAnimDeploy]);

	m_iNextPrimaryAttack = std::max(m_iNextPrimaryAttack, 500);
}


void CBasePlayerWeapon::Holster()
{
	const auto info = GetInfo();

	SendWeaponAnim(info.iAnims[kWeaponAnimHolster]);

	m_iNextPrimaryAttack = std::max(m_iNextPrimaryAttack, 500);
	m_fInReload = false;
	m_pPlayer->m_iFOV = 0;
}


void CBasePlayerWeapon::SendWeaponAnim(int iAnim)
{
	m_pPlayer->pev->weaponanim = iAnim;
	HUD_SendWeaponAnim(iAnim, pev->body, false);
}


void CBasePlayerWeapon::PlayWeaponSound(int iChannel, const char* szSound, float flVolume, float flAttn, int iFlags, float flPitch)
{
	if (!HUD_FirstTimePredicting())
    {
		return;
    }

	auto player = gEngfuncs.GetLocalPlayer();

	if (!player)
    {
		return;
    }

	gEngfuncs.pEventAPI->EV_PlaySound(player->index, player->origin, iChannel, szSound, flVolume, flAttn, iFlags, flPitch);
}

