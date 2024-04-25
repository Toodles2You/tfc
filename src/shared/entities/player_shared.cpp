//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Whatever
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#ifdef GAME_DLL
#include "gamerules.h"
#include "UserMessages.h"
#endif
#include "animation.h"
#include "trace.h"
#include <string>


void
CBasePlayer::PlaybackEvent(
	unsigned short event,
	float fParam1,
	float fParam2,
	int iParam1,
	int iParam2,
	qboolean bParam1,
	qboolean bParam2,
	unsigned int flags)
{
	g_engfuncs.pfnPlaybackEvent(
		flags,
		edict(),
		event,
		0.0f,
		pev->origin,
		pev->angles + pev->punchangle,
		fParam1,
		fParam2,
		iParam1,
		iParam2,
		bParam1,
		bParam2);
}


void CBasePlayer::WeaponPostFrame()
{
	ImpulseCommands();

	// check if the player is using a tank
#ifdef HALFLIFE_TANKCONTROL
#ifdef GAME_DLL
	if (m_pTank != nullptr)
	{
		return;
	}
#endif
#endif

#ifdef HALFLIFE_GRENADES
	if ((m_TFState & kTFStateGrenadePrime) != 0)
	{
		if (m_bGrenadeToggle)
		{
			if ((m_afButtonPressed & IN_GRENADE) != 0)
			{
				ThrowGrenade();
			}
		}
		else if ((pev->button & IN_GRENADE) == 0)
		{
			ThrowGrenade();
		}
	}
	else if ((m_afButtonPressed & IN_GRENADE) != 0)
	{
		PrimeGrenade();
	}
#endif

	if (m_pActiveWeapon == nullptr)
	{
		return;
	}

	m_pActiveWeapon->WeaponPostFrame();
}


#ifdef CLIENT_DLL
void CBasePlayer::PreThink()
{
	const auto buttonsChanged = m_afButtonLast ^ pev->button;

	m_afButtonPressed = buttonsChanged & pev->button;
	m_afButtonReleased = buttonsChanged & ~pev->button;
}
#endif


void CBasePlayer::PostThink()
{
#ifdef GAME_DLL
	if (g_pGameRules->GetState() == GR_STATE_GAME_OVER)
	{
		goto pt_end;
	}
#endif

	if (!IsAlive())
	{
		goto pt_end;
	}

#ifdef HALFLIFE_TANKCONTROL
#ifdef GAME_DLL
	if (m_pTank != NULL)
	{
		if (m_pTank->OnControls(pev) && 0 == pev->weaponmodel)
		{
			m_pTank->Use(this, this, USE_SET, 2); // try fire the gun
		}
		else
		{ // they've moved off the platform
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = NULL;
		}
	}
#endif
#endif

	WeaponPostFrame();

#ifdef GAME_DLL
	if ((FBitSet(pev->flags, FL_ONGROUND)) && (pev->health > 0) && m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD)
	{
		if (pev->watertype == CONTENT_WATER)
		{
			// Did he hit the world or a non-moving entity?
			// BUG - this happens all the time in water, especially when
			// BUG - water has current force
			// if ( !pev->groundentity || VARS(pev->groundentity)->velocity.z == 0 )
			// EmitSound("player/pl_wade1.wav", CHAN_VOICE);
		}
		else if (m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED)
		{ // after this point, we start doing damage

			float flFallDamage = g_pGameRules->FlPlayerFallDamage(this);

			if (flFallDamage > 0)
			{
				TakeDamage(CWorld::World, CWorld::World, flFallDamage, DMG_FALL);

				if (pev->health <= 0)
				{
					EmitSound("common/bodysplat.wav", CHAN_VOICE);
				}
			}
		}
	}

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		m_flFallVelocity = 0;
	}
#endif

pt_end:
    UpdateMovementAction();
	StudioFrameAdvance();

	m_afButtonLast = pev->button;
}


void CBasePlayer::GetClientData(clientdata_t& data, bool sendWeapons)
{
#ifdef GAME_DLL
	data.flags = pev->flags;
    data.deadflag = pev->deadflag;

	if (IsSpectator() || IsAlive())
	{
		data.health = std::max(pev->health, 1.0F);
	}
	else
	{
		data.health = 0.0F;
	}
	
	data.vuser4.z = pev->armorvalue;

	data.waterlevel = pev->waterlevel;
	data.watertype = pev->watertype;

	data.origin = pev->origin;
	data.velocity = pev->velocity;
	data.view_ofs = pev->view_ofs;

	data.bInDuck = pev->bInDuck;
	data.flTimeStepSound = pev->flTimeStepSound;
	data.flDuckTime = pev->flDuckTime;
	data.flSwimTime = pev->flSwimTime;
	data.waterjumptime = pev->teleport_time;

	data.iuser1 = pev->iuser1;
	data.iuser2 = pev->iuser2;
	data.iuser3 = pev->iuser3;
#endif

	data.tfstate = m_TFState;

	data.m_iId = (m_pActiveWeapon != nullptr) ? m_pActiveWeapon->GetID() + 1 : 0;

	byte* ammo = reinterpret_cast<byte*>(&data.ammo_shells);
	for (int i = 0; i < AMMO_TYPES; i++)
	{
		ammo[i] = m_rgAmmo[i];
	}

	data.viewmodel = pev->viewmodel;
    data.maxspeed = pev->maxspeed;
	data.weaponanim = pev->weaponanim;
	data.punchangle = pev->punchangle;

	data.fov = m_iFOV;
}


void CBasePlayer::SetClientData(const clientdata_t& data)
{
	pev->health = data.health;
    pev->deadflag = data.deadflag;

	pev->armorvalue = data.vuser4.z;

	pev->waterlevel = data.waterlevel;
	pev->watertype = data.watertype;

	pev->origin = data.origin;
	pev->velocity = data.velocity;
	pev->view_ofs = data.view_ofs;

	pev->bInDuck = data.bInDuck;
	pev->flTimeStepSound = data.flTimeStepSound;
	pev->flDuckTime = data.flDuckTime;
	pev->flSwimTime = data.flSwimTime;
	pev->teleport_time = data.waterjumptime;

	pev->iuser1 = data.iuser1;
	pev->iuser2 = data.iuser2;
	pev->iuser3 = data.iuser3;

	m_TFState = data.tfstate;

	if (m_pActiveWeapon == nullptr)
	{
		if (data.m_iId != 0)
		{
			m_pActiveWeapon = m_rgpPlayerWeapons[data.m_iId - 1];
		}
	}
	else if (data.m_iId - 1 != m_pActiveWeapon->GetID())
	{
		m_pActiveWeapon = m_rgpPlayerWeapons[data.m_iId - 1];
	}

	const byte* ammo = reinterpret_cast<const byte*>(&data.ammo_shells);
	for (int i = 0; i < AMMO_TYPES; i++)
	{
		m_rgAmmo[i] = ammo[i];
	}

	pev->viewmodel = data.viewmodel;
	pev->maxspeed = data.maxspeed;
	pev->weaponanim = data.weaponanim;
	pev->punchangle = data.punchangle;

	m_iFOV = data.fov;
}


void CBasePlayer::DecrementTimers(const int msec)
{
	float len = VectorNormalize(pev->punchangle);
	if (len != 0.0F)
	{
		len -= (10.0F + len * 0.5F) * (msec / 1000.0F);
		len = std::max(len, 0.0F);
		pev->punchangle = pev->punchangle * len;
	}
}


void CBasePlayer::SelectWeapon(int id)
{
	if (!HasPlayerWeapon(id))
	{
		return;
	}

	auto weapon = m_rgpPlayerWeapons[id];

	if (weapon == m_pActiveWeapon)
	{
		return;
	}

	if (m_pActiveWeapon != nullptr)
	{
		m_pActiveWeapon->Holster();
	}
	m_pActiveWeapon = weapon;
	m_pActiveWeapon->Deploy();
}


void CBasePlayer::CmdStart(const usercmd_t& cmd, unsigned int randomSeed)
{
	if (cmd.weaponselect != 0)
	{
        SelectWeapon(cmd.weaponselect - 1);
		((usercmd_t*)&cmd)->weaponselect = 0;
	}

	m_randomSeed = randomSeed;
}


CBasePlayerWeapon* CBasePlayer::GetNextBestWeapon(CBasePlayerWeapon* current)
{
	const int currentID = (current != nullptr) ? current->GetID() : -1;
	const int currentWeight = (current != nullptr) ? current->iWeight() : -1;

	CBasePlayerWeapon* best = nullptr;
	int bestWeight = -1;

	for (auto weapon : m_lpPlayerWeapons)
	{
		if (weapon->GetID() == currentID)
		{
			continue;
		}

		if (weapon->iWeight() > -1 && weapon->iWeight() == currentWeight)
		{
			if (weapon->CanDeploy())
			{
				return weapon;
			}
		}
		else if (weapon->iWeight() > bestWeight)
		{
			if (weapon->CanDeploy())
			{
				bestWeight = weapon->iWeight();
				best = weapon;
			}
		}
	}

	return best;
}


bool CBasePlayer::CanBeginAction(const Action action)
{
    switch (action)
    {
    case Action::Idle:
        switch (m_Action)
        {
        case Action::Attack:
        case Action::Reload:
        case Action::Arm:
            return m_fSequenceFinished;
        default:
            break;
        }
        break;

    case Action::Attack:
    case Action::Reload:
    case Action::Arm:
        switch (m_Action)
        {
        case Action::Jump:
        case Action::Die:
            return m_fSequenceFinished;
        default:
            break;
        }
        break;

    case Action::Jump:
    case Action::Die:
        break;
    }

    return true;
}


void CBasePlayer::SetAction(const Action action, const bool force)
{
    if (!force && !CanBeginAction(action))
    {
        return;
    }

    const auto oldSequence = pev->sequence;
    bool restart = false;
    pev->sequence = GetActionSequence(action, restart);

    m_Action = action;

    if (restart || oldSequence != pev->sequence)
    {
        pev->frame = 0;
        ResetSequenceInfo();
    }
}


void CBasePlayer::HandleSequenceFinished()
{
	if (m_Action != Action::Idle && m_Action != Action::Die)
	{
		SetAction(Action::Idle);
	}
}


int CBasePlayer::GetDeathSequence()
{
	Activity activity = ACT_DIESIMPLE;
	Vector center;
    Vector forward;
	float dot;

	switch (m_LastHitGroup)
	{
	case HITGROUP_HEAD:     activity = ACT_DIE_HEADSHOT;    break;
	case HITGROUP_STOMACH:  activity = ACT_DIE_GUTSHOT;     break;

	case HITGROUP_GENERIC:
	default:
        center = Center();

        AngleVectors(pev->angles, &forward, nullptr, nullptr);

        dot = DotProduct(gpGlobals->v_forward, g_vecAttackDir * -1);

		if (dot > 0.3)
		{
            Trace trace{center, center + forward * 64, entindex(), Trace::kBox};

            if (trace.fraction == 1)
            {
                return LookupActivity(ACT_DIEFORWARD);
            }
		}
		else if (dot <= -0.3)
		{
            Trace trace{center, center - forward * 64, entindex(), Trace::kBox};

            if (trace.fraction == 1)
            {
                return LookupActivity(ACT_DIEBACKWARD);
            }
		}
		break;
	}

	return LookupActivity(activity);
}


int CBasePlayer::GetSmallFlinchSequence()
{
	Activity activity;

	switch (m_LastHitGroup)
	{
	case HITGROUP_HEAD:     activity = ACT_FLINCH_HEAD;     break;
	case HITGROUP_STOMACH:  activity = ACT_FLINCH_STOMACH;  break;
	case HITGROUP_LEFTARM:  activity = ACT_FLINCH_LEFTARM;  break;
	case HITGROUP_RIGHTARM: activity = ACT_FLINCH_RIGHTARM; break;
	case HITGROUP_LEFTLEG:  activity = ACT_FLINCH_LEFTLEG;  break;
	case HITGROUP_RIGHTLEG: activity = ACT_FLINCH_RIGHTLEG; break;

	case HITGROUP_GENERIC:
	default:
        return LookupActivity(ACT_SMALL_FLINCH);
	}

	return LookupActivity(activity);
}


int CBasePlayer::GetActionSequence(const Action action, bool& restart)
{
    const auto ducking = (pev->flags & FL_DUCKING) != 0;

    std::string sequenceName;

    restart = false;

    switch (action)
    {
    case Action::Idle:
    case Action::Attack:
    case Action::Reload:
    case Action::Arm:
        if (m_szAnimExtention[0] == '\0')
        {
            break;
        }

		sequenceName = ducking ? "crouch_" : "ref_";

		switch (action)
		{
			case Action::Idle: sequenceName += "aim_"; break;
			case Action::Attack: sequenceName += "shoot_"; restart = true; break;
    		case Action::Reload: sequenceName += "aim_"; restart = true; break;
			case Action::Arm: sequenceName += "aim_"; restart = true; break;
		}

		sequenceName += m_szAnimExtention;
        return LookupSequence(sequenceName.c_str());

    case Action::Jump:
        restart = true;
        return LookupActivity(ACT_HOP);
    case Action::Die:
        restart = true;
        return GetDeathSequence();
    }

    return LookupSequence("deep_idle");
}


int CBasePlayer::GetGaitSequence()
{
    const auto ducking = (pev->flags & FL_DUCKING) != 0;
    const auto speed = pev->velocity.Length2D();

    if (ducking)
    {
        if (speed > 5)
        {
	        return LookupActivity(ACT_CROUCH);
        }

		return LookupActivity(ACT_CROUCHIDLE);
    }

    if (speed > 220)
    {
        return LookupActivity(ACT_RUN);
    }

    if (speed > 5)
    {
        return LookupActivity(ACT_WALK);
    }

    return LookupSequence("deep_idle");
}


void CBasePlayer::UpdateMovementAction()
{
	if (m_Action == Action::Jump)
	{
		if ((pev->flags & FL_ONGROUND) == 0)
		{
			pev->gaitsequence = 0;
			return;
		}
		SetAction(Action::Idle);
	}

    pev->gaitsequence = GetGaitSequence();
}


void CBaseEntity::SetEntityState(const entity_state_t& state)
{
	pev->animtime = state.animtime;

	pev->origin = state.origin;
	pev->angles = state.angles;
	pev->mins = state.mins;
	pev->maxs = state.maxs;

	pev->startpos = state.startpos;
	pev->endpos = state.endpos;

	pev->impacttime = state.impacttime;
	pev->starttime = state.starttime;

	pev->modelindex = state.modelindex;

	pev->frame = state.frame;

	pev->skin = state.skin;
	pev->effects = state.effects;

	m_EFlags |= state.eflags;

	pev->scale = state.scale;
	pev->solid = state.solid;
	pev->colormap = state.colormap;

	pev->movetype = state.movetype;
	pev->sequence = state.sequence;
	pev->framerate = state.framerate;
	pev->body = state.body;

	for (int i = 0; i < 4; i++)
	{
		pev->controller[i] = state.controller[i];
	}

	for (int i = 0; i < 2; i++)
	{
		pev->blending[i] = state.blending[i];
	}

	pev->rendermode = state.rendermode;
	pev->renderamt = state.renderamt;
	pev->renderfx = state.renderfx;
	pev->rendercolor.x = state.rendercolor.r;
	pev->rendercolor.y = state.rendercolor.g;
	pev->rendercolor.z = state.rendercolor.b;

	// pev->aiment = state.aiment;

	// pev->owner = state.owner;

	pev->playerclass = state.playerclass;
}


void CBasePlayer::SetEntityState(const entity_state_t& state)
{
	CBaseEntity::SetEntityState(state);

	pev->team = state.team;
	pev->playerclass = state.playerclass;

	pev->basevelocity = state.basevelocity;

	pev->weaponmodel = state.weaponmodel;
	pev->gaitsequence = state.gaitsequence;
	if (state.spectator != 0)
	{
		pev->flags |= FL_SPECTATOR;
	}
	pev->friction = state.friction;

	pev->gravity = state.gravity;

	if (state.usehull == 1)
	{
		pev->flags |= FL_DUCKING;
	}

	pev->health = state.health;
}

