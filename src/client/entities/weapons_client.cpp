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

	client::CL_LoadModel(info.pszView, &m_pPlayer->v.viewmodel);
	client::CL_LoadModel(info.pszPlayer, &m_pPlayer->v.weaponmodel);
	strcpy(m_pPlayer->m_szAnimExtention, info.pszAnimExt);

	SendWeaponAnim(info.iAnims[kWeaponAnimDeploy]);

	m_pPlayer->SetAction(CBasePlayer::Action::Arm);

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


void CBasePlayerWeapon::RemoveFromPlayer(bool forceSendAnimations)
{
	if (m_pPlayer == nullptr)
	{
		return;
	}

	m_ForceSendAnimations = forceSendAnimations;
	m_pPlayer->RemovePlayerWeapon(this);
	m_ForceSendAnimations = false;
}

