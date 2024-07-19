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

	m_flSpeedReduction = 0.0F;

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
	gHUD.m_Battery.Update_Battery(v.armorvalue, v.armortype);

	if (IsAlive() && m_pActiveWeapon != nullptr)
	{
		gHUD.m_Ammo.Update_CurWeapon(
			true, m_pActiveWeapon->GetID(), m_pActiveWeapon->m_iClip,
			m_pActiveWeapon->CanHolster());
	}
	else
	{
		gHUD.m_Ammo.Update_CurWeapon(0, -1, -1, true);
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
	
	if (HasPlayerWeapon(WEAPON_DETPACK))
	{
		if (InState(CBasePlayer::State::Holstered))
		{
			gHUD.m_Ammo.Update_Detpack(1);
		}
		else
		{
			gHUD.m_Ammo.Update_Detpack(2);
		}
	}
	else
	{
		gHUD.m_Ammo.Update_Detpack(0);
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

void CBasePlayer::PrimeGrenade(const int grenadeSlot)
{
	if (InState(State::Grenade))
	{
		return;
	}

	if (m_rgAmmo[AMMO_GRENADES1 + grenadeSlot] == 0)
	{
		EmitSoundPredicted("common/wpn_denyselect.wav", CHAN_ITEM, VOL_NORM, ATTN_IDLE);
		return;
	}

	const auto &info = sTFClassInfo[PCNumber()];
	const auto grenadeType = info.grenades[grenadeSlot];

	switch (grenadeType)
	{
		case GRENADE_NORMAL:
			break;
		case GRENADE_CALTROP:
			goto no_icon;
		case GRENADE_CONCUSSION:
			m_iGrenadeExplodeTime = 3800;
			break;
		case GRENADE_NAIL:
			break;
		case GRENADE_MIRV:
			break;
		case GRENADE_NAPALM:
			break;
		case GRENADE_GAS:
			return;
		case GRENADE_EMP:
			return;
		default:
			return;
	}

	if (HUD_FirstTimePredicting())
	{
		auto icon = GetGrenadeIconName(grenadeType);

		if (icon != nullptr)
		{
			gHUD.m_StatusIcons.EnableIcon(icon);
		}
	}

no_icon:
	m_rgAmmo[AMMO_GRENADES1 + grenadeSlot]--;

	m_iGrenadeExplodeTime = 0;

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
	m_iGrenadeExplodeTime = 0;
}

#endif


void CBasePlayer::ConcussionJump(Vector& velocity)
{
	if (HUD_FirstTimePredicting())
	{
		PlaybackEvent(
			g_usConcBlast,
			0.0F,
			0.0F,
			180.0F,
			static_cast<int>(tent::ExplosionType::Concussion),
			true,
			true);
	}

	velocity.x *= 2.36F;
	velocity.y *= 2.36F;
	velocity.z *= 3.53F;

	if (HUD_FirstTimePredicting())
	{
		gHUD.m_StatusIcons.DisableIcon("d_concussiongrenade");
	}
	
	LeaveState(CBasePlayer::State::Grenade);
}


void CBaseEntity::GetEntityState(entity_state_t& state)
{
	state.animtime = (int)(1000.0 * v.animtime) / 1000.0;

	state.frame = v.frame;

	state.sequence = v.sequence;
	state.framerate = v.framerate;
}


void CBasePlayer::GetEntityState(entity_state_t& state)
{
	CBaseEntity::GetEntityState(state);

	state.gaitsequence = v.gaitsequence;
}

