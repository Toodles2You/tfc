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
		auto oldWeapon = m_pActiveWeapon;
		m_pActiveWeapon = nullptr;
		m_iActiveWeapon = WEAPON_NONE;
		oldWeapon->Holster();
    }

	m_iActiveWeapon = m_iLastWeapon = WEAPON_NONE;

	ClearEffects();
}


bool CBasePlayer::Spawn()
{
	SetModel("models/player/scout/scout.mdl");
	SetAction(Action::Idle, true);

	ClearEffects();

	m_flSpeedReduction = 0.0F;

	m_bLeftHanded = static_cast<int>(floorf(cl_righthand->value)) == 0;

	m_iActiveWeapon = m_iLastWeapon = WEAPON_NONE;

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
		gHUD.m_Ammo.Update_CurWeapon(0, WEAPON_NONE, -1, true);
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

	gHUD.Update_Tranquilization(InState(State::Tranquilized));
	
	if (HasPlayerWeapon(WEAPON_DETPACK))
	{
		if (InState(State::Holstered))
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

	if (HasPlayerWeapon(WEAPON_BUILDER))
	{
		auto builder = static_cast<CBuilder*>(m_rgpPlayerWeapons[WEAPON_BUILDER]);

		gHUD.m_Ammo.Update_BuildSt(builder->GetBuildState());
	}
	else
	{
		gHUD.m_Ammo.Update_BuildSt(0);
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
		m_pActiveWeapon = nullptr;
		m_iActiveWeapon = WEAPON_NONE;
		weapon->Holster();
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

	/* Prevent grenade priming while feigning death. */
	if (InState(State::FeigningDeath) || m_iFeignTime != 0)
	{
		EmitSoundPredicted("common/wpn_denyselect.wav", CHAN_ITEM, VOL_NORM, ATTN_IDLE);
		return;
	}

	if (m_rgAmmo[AMMO_GRENADES1 + grenadeSlot] == 0)
	{
		EmitSoundPredicted("common/wpn_denyselect.wav", CHAN_ITEM, VOL_NORM, ATTN_IDLE);
		return;
	}

	m_iGrenadeExplodeTime = 0;

	const auto &info = sTFClassInfo[PCNumber()];
	const auto grenadeType = info.grenades[grenadeSlot];

	auto throwGrenade = false;

	switch (grenadeType)
	{
		case GRENADE_NORMAL:
			break;
		case GRENADE_CALTROP:
			throwGrenade = true;
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
		case GRENADE_EMP:
			break;
		case GRENADE_FLASH:
			throwGrenade = true;
			goto no_icon;
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

	EnterState(State::GrenadePrime);
	EmitSoundPredicted("weapons/ax1.wav", CHAN_WEAPON);

	if (throwGrenade)
	{
		ThrowGrenade();
	}
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


void CBaseEntity::GetEntityState(entity_state_t& state, CBasePlayer* player)
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


void CBasePlayer::GetEntityState(entity_state_t& state, CBasePlayer* player)
{
	CBaseEntity::GetEntityState(state, player);

	state.gaitsequence = v.gaitsequence;
}


void CBasePlayer::FinishDisguising()
{
	/* Toodles TODO: Four team. */
	m_iDisguiseTeam = (TeamNumber () == TEAM_BLUE) != m_bNextDisguiseAlly ? TEAM_RED : TEAM_BLUE;

	m_iDisguisePlayerClass = m_iNextDisguisePlayerClass;

	EnterState(State::Disguised);

	SetAction(Action::Idle, true);
}

