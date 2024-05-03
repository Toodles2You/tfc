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
	const float damageMax,
	const float damageMin,
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

	ClearEffects();
}


bool CBasePlayer::Spawn()
{
	SetModel("models/player/scout/scout.mdl");
	SetAction(Action::Idle, true);

	ClearEffects();

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
	gHUD.m_Health.Update_Health(pev->health);
	gHUD.m_Battery.Update_Battery(pev->armorvalue, pev->armortype);

	if (IsAlive() && m_pActiveWeapon != nullptr)
	{
		gHUD.m_Ammo.Update_CurWeapon(
			true, m_pActiveWeapon->GetID(), m_pActiveWeapon->m_iClip);
	}
	else
	{
		gHUD.m_Ammo.Update_CurWeapon(0, -1, -1);
	}

	for (int i = 0; i < AMMO_SECONDARY; i++)
	{
		gHUD.m_Ammo.Update_AmmoX(i, m_rgAmmo[i]);
	}

	for (int i = AMMO_SECONDARY; i < AMMO_TYPES; i++)
	{
		gHUD.m_AmmoSecondary.Update_SecAmmoVal(i - AMMO_SECONDARY, m_rgAmmo[i]);
	}

	if (m_iConcussionTime > 0)
	{
		if (m_iConcussionTime > 7000)
		{
			gHUD.Update_Concuss(100);
		}
		else
		{
			gHUD.Update_Concuss(m_iConcussionTime / 70);
		}
	}
	else
	{
		gHUD.Update_Concuss(0);
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

void CBasePlayer::PrimeGrenade(const int grenadeType)
{
	if (InState(State::Grenade))
	{
		return;
	}

	if (m_rgAmmo[AMMO_GRENADES1 + grenadeType] == 0)
	{
		EmitSoundPredicted("common/wpn_denyselect.wav", CHAN_ITEM, VOL_NORM, ATTN_IDLE);
		return;
	}

	m_rgAmmo[AMMO_GRENADES1 + grenadeType]--;

	if (grenadeType == 0)
	{
		switch (PCNumber())
		{
			case PC_SCOUT:
				break;
			default:
				break;
		}
	}
	else
	{
		switch (PCNumber())
		{
		case PC_SCOUT:
			break;
		case PC_SNIPER:
			return;
		case PC_SOLDIER:
			break;
		case PC_DEMOMAN:
			break;
		case PC_MEDIC:
			break;
		case PC_HVYWEAP:
			break;
		case PC_PYRO:
			return;
		case PC_SPY:
			return;
		case PC_ENGINEER:
			return;
		default:
		case PC_CIVILIAN:
			return;
		}
	}

	if (HUD_FirstTimePredicting())
	{
		const char* icon = GetGrenadeIconName(grenadeType);

		if (icon != nullptr)
		{
			gHUD.m_StatusIcons.EnableIcon(icon);
		}
	}

no_icon:
	EnterState(State::GrenadePrime);
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
	state.animtime = (int)(1000.0 * pev->animtime) / 1000.0;

	state.frame = pev->frame;

	state.sequence = pev->sequence;
	state.framerate = pev->framerate;
}


void CBasePlayer::GetEntityState(entity_state_t& state)
{
	CBaseEntity::GetEntityState(state);

	state.gaitsequence = pev->gaitsequence;
}

