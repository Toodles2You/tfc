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

extern cvar_t* cl_autowepswitch;
extern cvar_t* cl_grenadetoggle;


void CBasePlayer::FireBullets(
	const float damage,
	const Vector2D& spread,
	const unsigned int count,
	const float distance)
{
}


void CBasePlayer::Killed(CBaseEntity* inflictor, CBaseEntity* attacker, int bitsDamageType)
{
	if (m_pActiveWeapon != nullptr)
    {
		m_pActiveWeapon->Holster();
		m_pActiveWeapon = nullptr;
    }

	m_TFState &= ~(kTFStateGrenadePrime | kTFStateGrenadeThrowing);
}


bool CBasePlayer::Spawn()
{
	m_TFState &= ~(kTFStateGrenadePrime | kTFStateGrenadeThrowing);
	return true;
}


void CBasePlayer::UpdateHudData()
{
	if (!HUD_FirstTimePredicting())
	{
		return;
	}

	gHUD.Update_SetFOV(m_iFOV);
	gHUD.m_Health.Update_Health(pev->health);
	gHUD.m_Battery.Update_Battery(pev->armorvalue);

	if (IsAlive() && m_pActiveWeapon != nullptr)
	{
		gHUD.m_Ammo.Update_CurWeapon(
			true, m_pActiveWeapon->GetID(), m_pActiveWeapon->m_iClip);
	}
	else
	{
		gHUD.m_Ammo.Update_CurWeapon(0, -1, -1);
	}

	for (int i = AMMO_NONE + 1; i < AMMO_LAST; i++)
	{
		gHUD.m_Ammo.Update_AmmoX(i, m_rgAmmo[i]);
	}
}


void CBaseEntity::EmitSoundPredicted(const char* sample, int channel, float volume, float attenuation, int pitch, int flags)
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

	gEngfuncs.pEventAPI->EV_PlaySound(player->index, player->origin, channel, sample, volume, attenuation, flags, pitch);
}


void CBasePlayer::SetPrefsFromUserinfo(char* infobuffer)
{
	m_iAutoWepSwitch = (int)cl_autowepswitch->value;
	m_bGrenadeToggle = (int)cl_grenadetoggle->value != 0;
}


void CBasePlayer::PrimeGrenade()
{
	if ((m_TFState & (kTFStateGrenadePrime | kTFStateGrenadeThrowing)) != 0)
	{
		return;
	}

	m_TFState |= kTFStateGrenadePrime;

	if (HUD_FirstTimePredicting())
	{
		gHUD.m_StatusIcons.EnableIcon("grenade");
	}

	EmitSoundPredicted("weapons/ax1.wav", CHAN_WEAPON);
}


void CBasePlayer::ThrowGrenade()
{
	if (!HUD_FirstTimePredicting())
	{
		return;
	}

	m_TFState &= ~kTFStateGrenadePrime;
	m_TFState |= kTFStateGrenadeThrowing;
}

