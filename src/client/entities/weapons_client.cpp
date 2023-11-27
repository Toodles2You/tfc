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


bool CBasePlayerWeapon::DefaultDeploy(const char* szViewModel, const char* szWeaponModel, int iAnim, const char* szAnimExt, int body)
{
	if (!CanDeploy())
    {
		return false;
    }

	gEngfuncs.CL_LoadModel(szViewModel, &m_pPlayer->pev->viewmodel);

	SendWeaponAnim(iAnim, body);

	m_pPlayer->m_iNextAttack = 500;
	m_iNextPrimaryAttack = std::max(m_iNextPrimaryAttack, 500);
	m_iNextSecondaryAttack = std::max(m_iNextSecondaryAttack, 500);
	m_iTimeWeaponIdle = 1000;
	m_bPlayEmptySound = true;
	return true;
}


bool CBasePlayerWeapon::DefaultHolster(int iAnim, int body)
{
	if (!CanHolster())
    {
		return false;
    }

	if (iAnim >= 0)
    {
		SendWeaponAnim(iAnim, body);
    }

	m_pPlayer->m_iNextAttack = 500;
	m_iNextPrimaryAttack = std::max(m_iNextPrimaryAttack, 500);
	m_iNextSecondaryAttack = std::max(m_iNextSecondaryAttack, 500);
	m_iTimeWeaponIdle = 1000;
	m_fInReload = false;
	m_fInSpecialReload = 0;
	m_pPlayer->m_iFOV = 0;
	return true;
}


void CBasePlayerWeapon::PlayEmptySound()
{
	if (!HUD_FirstTimePredicting())
    {
		return;
    }

	if (m_bPlayEmptySound)
	{
		PlayWeaponSound(CHAN_ITEM, "weapons/357_cock1.wav", 0.8);
		m_bPlayEmptySound = false;
	}
}


void CBasePlayerWeapon::SendWeaponAnim(int iAnim, int body)
{
	m_pPlayer->pev->weaponanim = iAnim;
	HUD_SendWeaponAnim(iAnim, body, false);
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

