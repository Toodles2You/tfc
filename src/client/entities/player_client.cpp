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
extern cvar_t* cl_righthand;


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

	m_StateBits = 0;
}


bool CBasePlayer::Spawn()
{
	SetModel("models/player.mdl");
	SetAction(Action::Idle, true);

	m_StateBits = 0;

	m_bLeftHanded = static_cast<int>(floorf(cl_righthand->value)) == 0;

	return true;
}


void CBasePlayer::UpdateHudData()
{
	if (!HUD_FirstTimePredicting())
	{
		return;
	}

	gHUD.Update_SetFOV(m_iFOV);
	gHUD.m_Health.Update_Health(v.health);
	gHUD.m_Battery.Update_Battery(v.armorvalue);

	if (IsAlive() && m_pActiveWeapon != nullptr)
	{
		gHUD.m_Ammo.Update_CurWeapon(
			true, m_pActiveWeapon->GetID(), m_pActiveWeapon->m_iClip);
	}
	else
	{
		gHUD.m_Ammo.Update_CurWeapon(0, -1, -1);
	}

	for (int i = 0; i < AMMO_TYPES; i++)
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

	auto player = client::GetLocalPlayer();

	if (!player)
    {
		return;
    }

	client::event::PlaySound(player->index, player->origin, channel, sample, volume, attenuation, flags, pitch);
}


void CBasePlayer::SetPrefsFromUserinfo(char* infobuffer)
{
	m_iAutoWepSwitch = (int)cl_autowepswitch->value;
	m_bGrenadeToggle = (int)cl_grenadetoggle->value != 0;
}


void CBasePlayer::RemovePlayerWeapon(CBasePlayerWeapon* weapon)
{
	const auto isActive = m_pActiveWeapon == weapon;

	if (isActive)
	{
		weapon->Holster();
		m_pActiveWeapon = nullptr;
	}

	ClearWeaponBit(weapon->GetID());

	if (isActive)
	{
		auto best = GetNextBestWeapon(weapon);
		if (best != nullptr)
		{
			SelectWeapon(best->GetID());
		}
	}
}


#ifdef HALFLIFE_GRENADES

void CBasePlayer::PrimeGrenade()
{
	if (InState(State::Grenade))
	{
		return;
	}

	EnterState(State::GrenadePrime);

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

	LeaveState(State::GrenadePrime);
	EnterState(State::GrenadeThrowing);
}

#endif


void CBaseEntity::GetEntityState(entity_state_t& state)
{
	state.animtime = (int)(1000.0 * v.animtime) / 1000.0;

	state.frame = v.frame;

	state.sequence = v.sequence;
	state.framerate = v.framerate;

	state.rendermode = v.rendermode;
	state.renderamt = v.renderamt;
	state.renderfx = v.renderfx;
	state.rendercolor.r = v.rendercolor.x;
	state.rendercolor.g = v.rendercolor.y;
	state.rendercolor.b = v.rendercolor.z;
}


void CBasePlayer::GetEntityState(entity_state_t& state)
{
	CBaseEntity::GetEntityState(state);

	state.gaitsequence = v.gaitsequence;
}

