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


CBaseEntity::CBaseEntity(Entity* containingEntity) : v {*containingEntity}
{
	containingEntity->effects |= EF_NOINTERP;
}


CBaseEntity::~CBaseEntity()
{
	/* Since the edict doesn't get deleted, fix it so it doesn't interfere. */
	v.model = 0;
	v.modelindex = 0;
	v.effects = EF_NOINTERP | EF_NODRAW;
	v.solid = SOLID_NOT;
	v.movetype = MOVETYPE_NONE;
	v.takedamage = DAMAGE_NO;
	v.origin = g_vecZero;
}


CBasePlayer::CBasePlayer(Entity* containingEntity) : CBaseAnimating(containingEntity)
{
	containingEntity->team = TEAM_UNASSIGNED;
	containingEntity->playerclass = PC_UNDEFINED;
	containingEntity->iuser1 = OBS_FIXED;

	m_fDeadTime = gpGlobals->time - 60.0F;
	SetCustomDecalFrames(-1);
	m_flNextChatTime = gpGlobals->time + CHAT_INTERVAL;

#ifdef GAME_DLL
	m_ResetHUD = ResetHUD::Initialize;
	m_team = nullptr;
	m_gameMovement = nullptr;
#endif
}


CBasePlayer::~CBasePlayer()
{
	v.team = TEAM_UNASSIGNED;
	v.playerclass = PC_UNDEFINED;
	v.netname = MAKE_STRING("unconnected");

	InstallGameMovement(nullptr);

#ifdef GAME_DLL
#ifdef HALFLIFE_TANKCONTROL
		if (m_pTank != nullptr)
		{
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = nullptr;
		}
#endif

	SetUseObject(nullptr);

	if (m_team != nullptr)
	{
		m_team->RemovePlayer(this);
	}
#endif

	RemoveAllWeapons();
}


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
	engine::PlaybackEvent(
		flags,
		&v,
		event,
		0.0f,
		v.origin,
		v.angles + v.punchangle,
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
	if (InState(State::GrenadePrime))
	{
		if (m_bGrenadeToggle)
		{
			if ((m_afButtonPressed & IN_GRENADE) != 0)
			{
				ThrowGrenade();
			}
		}
		else if ((v.button & IN_GRENADE) == 0)
		{
			ThrowGrenade();
		}
	}
	else if ((m_afButtonPressed & IN_GRENADE) != 0)
	{
		PrimeGrenade();
	}
#endif

	if (m_pActiveWeapon == nullptr || InState(State::Holstered))
	{
		return;
	}

	m_pActiveWeapon->WeaponPostFrame();
}


#ifdef CLIENT_DLL
void CBasePlayer::PreThink()
{
	const auto buttonsChanged = m_afButtonLast ^ v.button;

	m_afButtonPressed = buttonsChanged & v.button;
	m_afButtonReleased = buttonsChanged & ~v.button;
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
	if (m_pTank != nullptr)
	{
		if (m_pTank->OnControls(this) && 0 == v.weaponmodel)
		{
			m_pTank->Use(this, this, USE_SET, 2); // try fire the gun
		}
		else
		{ // they've moved off the platform
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = nullptr;
		}
	}
#endif
#endif

	WeaponPostFrame();

#ifdef GAME_DLL
	if ((FBitSet(v.flags, FL_ONGROUND)) && (v.health > 0) && m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD)
	{
		if (v.watertype == CONTENT_WATER)
		{
			// Did he hit the world or a non-moving entity?
		}
		else if (m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED)
		{ // after this point, we start doing damage

			float flFallDamage = g_pGameRules->FlPlayerFallDamage(this);

			if (flFallDamage > 0)
			{
				TakeDamage(CWorld::World, CWorld::World, flFallDamage, DMG_FALL);

				if (v.health <= 0)
				{
					EmitSound("common/bodysplat.wav", CHAN_VOICE);
				}
			}
		}
	}

	if (FBitSet(v.flags, FL_ONGROUND))
	{
		m_flFallVelocity = 0;
	}
#endif

pt_end:
    UpdateMovementAction();
	StudioFrameAdvance();

	m_afButtonLast = v.button;
}


void CBasePlayer::GetClientData(clientdata_t& data, bool sendWeapons)
{
#ifdef GAME_DLL
	data.flags = v.flags;
    data.deadflag = v.deadflag;

	if (IsSpectator() || IsAlive())
	{
		data.health = std::max(v.health, 1.0F);
	}
	else
	{
		data.health = 0.0F;
	}
	
	data.vuser4.z = v.armorvalue;

	data.waterlevel = v.waterlevel;
	data.watertype = v.watertype;

	data.origin = v.origin;
	data.velocity = v.velocity;
	data.view_ofs = v.view_ofs;

	data.bInDuck = v.bInDuck;
	data.flTimeStepSound = v.flTimeStepSound;
	data.flDuckTime = v.flDuckTime;
	data.flSwimTime = v.flSwimTime;
	data.waterjumptime = v.teleport_time;

	data.iuser1 = v.iuser1;
	data.iuser2 = v.iuser2;
	data.iuser3 = v.iuser3;
#endif

	data.tfstate = m_StateBits;

	data.m_iId = m_iActiveWeapon | (m_iLastWeapon << 6);

	byte* ammo = reinterpret_cast<byte*>(&data.ammo_shells);
	for (int i = 0; i < AMMO_TYPES; i++)
	{
		ammo[i] = m_rgAmmo[i];
	}

	data.viewmodel = v.viewmodel;
    data.maxspeed = v.maxspeed;
	data.weaponanim = v.weaponanim;
	data.punchangle = v.punchangle;

	data.fov = m_iFOV;
	
	data.iuser4 = m_iWeaponAnimTime;
}


void CBasePlayer::SetClientData(const clientdata_t& data)
{
	v.health = data.health;
    v.deadflag = data.deadflag;

	v.armorvalue = data.vuser4.z;

	v.waterlevel = data.waterlevel;
	v.watertype = data.watertype;

	v.origin = data.origin;
	v.velocity = data.velocity;
	v.view_ofs = data.view_ofs;

	v.bInDuck = data.bInDuck;
	v.flTimeStepSound = data.flTimeStepSound;
	v.flDuckTime = data.flDuckTime;
	v.flSwimTime = data.flSwimTime;
	v.teleport_time = data.waterjumptime;

	v.iuser1 = data.iuser1;
	v.iuser2 = data.iuser2;
	v.iuser3 = data.iuser3;

	m_StateBits = data.tfstate;

	const auto oldActiveWeapon = m_iActiveWeapon;

	m_iActiveWeapon = data.m_iId & 63;
	m_iLastWeapon = data.m_iId >> 6;

	if (m_iActiveWeapon != oldActiveWeapon)
	{
		if (m_iActiveWeapon == WEAPON_NONE)
		{
			m_pActiveWeapon = nullptr;
		}
		else
		{
			m_pActiveWeapon = m_rgpPlayerWeapons[m_iActiveWeapon];
		}
	}

	const byte* ammo = reinterpret_cast<const byte*>(&data.ammo_shells);
	for (int i = 0; i < AMMO_TYPES; i++)
	{
		m_rgAmmo[i] = ammo[i];
	}

	v.viewmodel = data.viewmodel;
	v.maxspeed = data.maxspeed;
	v.weaponanim = data.weaponanim;
	v.punchangle = data.punchangle;

	m_iFOV = data.fov;

	m_iWeaponAnimTime = data.iuser4;
}


void CBasePlayer::DecrementTimers(const int msec)
{
	float len = v.punchangle.NormalizeInPlace();
	if (len != 0.0F)
	{
		len -= (10.0F + len * 0.5F) * (msec / 1000.0F);
		len = std::max(len, 0.0F);
		v.punchangle = v.punchangle * len;
	}

	m_iWeaponAnimTime = std::min(m_iWeaponAnimTime + msec, 16000);
}


void CBasePlayer::SelectWeapon(int id)
{
	if (id < 0 || id > WEAPON_NONE)
	{
		return;
	}

	if (m_iActiveWeapon == id)
	{
		return;
	}

	if (!HasPlayerWeapon(id))
	{
		return;
	}

	CBasePlayerWeapon *weapon;
	if (id == WEAPON_NONE)
	{
		weapon = nullptr;
	}
	else
	{
		weapon = m_rgpPlayerWeapons[id];
	}

	m_iLastWeapon = m_iActiveWeapon;
	m_iActiveWeapon = id;

	/* Switch weapons without holstering & deploying. */
	if (!InState(State::Holstered))
	{
		if (m_pActiveWeapon != nullptr)
		{
			m_pActiveWeapon->Holster();
		}
		if (weapon != nullptr)
		{
			weapon->Deploy();
		}
	}

	m_pActiveWeapon = weapon;
}


void CBasePlayer::SelectLastWeapon()
{
	SelectWeapon(m_iLastWeapon);
}


bool CBasePlayer::SetWeaponHolstered(const bool holstered, const bool forceSendAnimations)
{
	const auto alreadyHolstered = InState(State::Holstered);

	/* The weapon is already where we want it. */
	if (alreadyHolstered == holstered)
	{
		return true;
	}

	const auto hasActiveWeapon = m_pActiveWeapon != nullptr;

	if (hasActiveWeapon)
	{
		m_pActiveWeapon->m_ForceSendAnimations = forceSendAnimations;
	}

	if (holstered)
	{
		/* Holster! */
		if (hasActiveWeapon)
		{
			/* Ensure the weapon can be holstered. */
			if (!m_pActiveWeapon->CanHolster())
			{
				return false;
			}
			m_pActiveWeapon->Holster();
		}
		EnterState(State::Holstered);
	}
	else
	{
		/* Deploy! */
		if (hasActiveWeapon)
		{
			/* Ensure the weapon can be deployed. */
			if (!m_pActiveWeapon->CanDeploy())
			{
				return false;
			}
			m_pActiveWeapon->Deploy();
		}
		LeaveState(State::Holstered);
	}

	if (hasActiveWeapon)
	{
		m_pActiveWeapon->m_ForceSendAnimations = false;
	}

	return true;
}


void CBasePlayer::CmdStart(const usercmd_t& cmd, unsigned int randomSeed)
{
	SelectWeapon(cmd.weaponselect);

	m_randomSeed = randomSeed;
}


CBasePlayerWeapon* CBasePlayer::GetNextBestWeapon(CBasePlayerWeapon* current)
{
	const int currentID = (current != nullptr) ? current->GetID() : -1;
	const int currentWeight = (current != nullptr) ? current->iWeight() : -1;

	CBasePlayerWeapon* best = nullptr;
	int bestWeight = -1;

	for (auto i = 0; i < WEAPON_TYPES; i++)
	{
		if (i == currentID)
		{
			continue;
		}

		if (!HasPlayerWeapon(i))
		{
			continue;
		}

		auto weapon = m_rgpPlayerWeapons[i];

		if (weapon == nullptr)
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

    const auto oldSequence = v.sequence;
    bool restart = false;
    v.sequence = GetActionSequence(action, restart);

    m_Action = action;

    if (restart || oldSequence != v.sequence)
    {
        v.frame = 0;
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

        AngleVectors(v.angles, &forward, nullptr, nullptr);

        dot = DotProduct(gpGlobals->v_forward, g_vecAttackDir * -1);

#ifdef GAME_DLL
		int ignore = v.GetIndex();
#else
		int ignore = -1;
#endif

		if (dot > 0.3)
		{
            Trace trace{center, center + forward * 64, ignore, Trace::kBox};

            if (trace.fraction == 1)
            {
                return LookupActivity(ACT_DIEFORWARD);
            }
		}
		else if (dot <= -0.3)
		{
            Trace trace{center, center - forward * 64, ignore, Trace::kBox};

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
    const auto ducking = (v.flags & FL_DUCKING) != 0;

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
    const auto ducking = (v.flags & FL_DUCKING) != 0;
    const auto speed = v.velocity.Length2D();

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
		if ((v.flags & FL_ONGROUND) == 0)
		{
			v.gaitsequence = 0;
			return;
		}
		SetAction(Action::Idle);
	}

    v.gaitsequence = GetGaitSequence();
}


void CBaseEntity::SetEntityState(const entity_state_t& state)
{
	v.animtime = state.animtime;

	v.origin = state.origin;
	v.angles = state.angles;
	v.mins = state.mins;
	v.maxs = state.maxs;

	v.startpos = state.startpos;
	v.endpos = state.endpos;

	v.impacttime = state.impacttime;
	v.starttime = state.starttime;

	v.modelindex = state.modelindex;

	v.frame = state.frame;

	v.skin = state.skin;
	v.effects = state.effects;

	m_EFlags |= state.eflags;

	v.scale = state.scale;
	v.solid = state.solid;
	v.colormap = state.colormap;

	v.movetype = state.movetype;
	v.sequence = state.sequence;
	v.framerate = state.framerate;
	v.body = state.body;

	for (int i = 0; i < 4; i++)
	{
		v.controller[i] = state.controller[i];
	}

	for (int i = 0; i < 2; i++)
	{
		v.blending[i] = state.blending[i];
	}

	v.rendermode = state.rendermode;
	v.renderamt = state.renderamt;
	v.renderfx = state.renderfx;
	v.rendercolor.x = state.rendercolor.r;
	v.rendercolor.y = state.rendercolor.g;
	v.rendercolor.z = state.rendercolor.b;

	// v.aiment = state.aiment;

	// v.owner = state.owner;

	v.playerclass = state.playerclass;
}


void CBasePlayer::SetEntityState(const entity_state_t& state)
{
	CBaseEntity::SetEntityState(state);

	v.team = state.team;
	v.playerclass = state.playerclass;

	v.basevelocity = state.basevelocity;

	v.weaponmodel = state.weaponmodel;
	v.gaitsequence = state.gaitsequence;
	if (state.spectator != 0)
	{
		v.flags |= FL_SPECTATOR;
	}
	v.friction = state.friction;

	v.gravity = state.gravity;

	if (state.usehull == 1)
	{
		v.flags |= FL_DUCKING;
	}

	v.health = state.health;
}

