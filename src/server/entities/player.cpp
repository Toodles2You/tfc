/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== player.cpp ========================================================

  functions dealing with the player

*/

#include <limits>
#include <algorithm>

#include "extdll.h"
#include "util.h"

#include "cbase.h"
#include "player.h"
#ifdef HALFLIFE_TRAINCONTROL
#include "trains.h"
#endif
#include "weapons.h"
#include "shake.h"
#include "gamerules.h"
#include "game.h"
#include "pm_shared.h"
#include "hltv.h"
#include "UserMessages.h"
#include "client.h"
#include "animation.h"
#include "weaponbox.h"
#ifdef HALFLIFE_NODEGRAPH
#include "nodes.h"
#endif
#include "tf_goal.h"
#include "items.h"

#ifdef HALFLIFE_TRAINCONTROL
enum {
	TRAIN_ACTIVE  = 0x80,
	TRAIN_NEW     = 0xc0,
	TRAIN_OFF     = 0x00,
	TRAIN_NEUTRAL = 0x01,
	TRAIN_SLOW    = 0x02,
	TRAIN_MEDIUM  = 0x03,
	TRAIN_FAST    = 0x04,
	TRAIN_BACK    = 0x05,
};
#endif

#define FLASH_DRAIN_TIME 1.2  //100 units/3 minutes
#define FLASH_CHARGE_TIME 0.2 // 100 units/20 seconds  (seconds per unit)

// Global Savedata for player
#ifdef HALFLIFE_SAVERESTORE
TYPEDESCRIPTION CBasePlayer::m_playerSaveData[] =
{
	DEFINE_FIELD(CBasePlayer, m_afButtonLast, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_afButtonPressed, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_afButtonReleased, FIELD_INTEGER),

	DEFINE_ARRAY(CBasePlayer, m_rgpPlayerWeapons, FIELD_CLASSPTR, WEAPON_TYPES),
	DEFINE_FIELD(CBasePlayer, m_pActiveWeapon, FIELD_CLASSPTR),
	DEFINE_FIELD(CBasePlayer, m_WeaponBits, FIELD_INT64),

	DEFINE_ARRAY(CBasePlayer, m_rgAmmo, FIELD_INTEGER, AMMO_TYPES),

#ifdef HALFLIFE_TRAINCONTROL
	DEFINE_FIELD(CBasePlayer, m_iTrain, FIELD_INTEGER),
#endif
	DEFINE_FIELD(CBasePlayer, m_bitsHUDDamage, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_flFallVelocity, FIELD_FLOAT),

#ifdef HALFLIFE_TANKCONTROL
	DEFINE_FIELD(CBasePlayer, m_pTank, FIELD_EHANDLE),
#endif
	DEFINE_FIELD(CBasePlayer, m_iFOV, FIELD_INTEGER),

	DEFINE_FIELD(CBasePlayer, m_SndRoomtype, FIELD_INTEGER),
};
#endif

LINK_ENTITY_TO_CLASS(player, CBasePlayer);


void CBasePlayer::Pain(const int damageType)
{
	char* sample = nullptr;

	if ((damageType & DMG_FALL) != 0)
	{
		return;
	}

	if ((damageType & DMG_DROWN) != 0)
	{
		// water pain sounds
		switch (engine::RandomLong(0, 11))
		{
			case 0: sample = "player/drown1.wav"; break;
			case 1: sample = "player/drown2.wav"; break;
			default: break;
		}
	}
	else
	{
		// play random sound
		switch (engine::RandomLong(0, 35))
		{
			case 0: sample = "player/pain1.wav"; break;
			case 1: sample = "player/pain2.wav"; break;
			case 2: sample = "player/pain3.wav"; break;
			case 3: sample = "player/pain4.wav"; break;
			case 4: sample = "player/pain5.wav"; break;
			case 5: sample = "player/pain6.wav"; break;
			default: break;
		}
	}

	if (sample == nullptr)
	{
		return;
	}

	EmitSound(sample, CHAN_VOICE, VOL_NORM, ATTN_NORM, GetVoicePitch());
}

#ifdef HALFLIFE_TRAINCONTROL
static int TrainSpeed(int iSpeed, int iMax)
{
	float fSpeed, fMax;
	int iRet = 0;

	fMax = (float)iMax;
	fSpeed = iSpeed;

	fSpeed = fSpeed / fMax;

	if (iSpeed < 0)
		iRet = TRAIN_BACK;
	else if (iSpeed == 0)
		iRet = TRAIN_NEUTRAL;
	else if (fSpeed < 0.33)
		iRet = TRAIN_SLOW;
	else if (fSpeed < 0.66)
		iRet = TRAIN_MEDIUM;
	else
		iRet = TRAIN_FAST;

	return iRet;
}
#endif

void CBasePlayer::DeathSound(const int damageType)
{
	char* sample = nullptr;

	// water death sounds
	if ((damageType & DMG_DROWN) != 0)
	{
		// water death sounds
		sample = "player/h2odeath.wav";
	}
	else
	{
		// play random sound
		switch (engine::RandomLong(0, 4))
		{
			case 0: sample = "player/death1.wav"; break;
			case 1: sample = "player/death2.wav"; break;
			case 2: sample = "player/death3.wav"; break;
			case 3: sample = "player/death4.wav"; break;
			case 4: sample = "player/death5.wav"; break;
			default: break;
		}
	}

	if (sample == nullptr)
	{
		return;
	}

	EmitSound(sample, CHAN_VOICE, VOL_NORM, ATTN_NORM, GetVoicePitch());
}

// override takehealth
// bitsDamageType indicates type of damage healed.

bool CBasePlayer::GiveHealth(float flHealth, int bitsDamageType, bool bClearEffects)
{
	auto result = CBaseAnimating::GiveHealth(flHealth, bitsDamageType, bClearEffects);

	if (bClearEffects)
	{
		if (InState(State::Infected)
		 || InState(State::Burning)
		 || InState(State::Tranquilized)
		 || m_nLegDamage != 0
		 || m_iConcussionTime != 0)
		{
			result = true;
		}

		LeaveState(State::Infected);
		LeaveState(State::Burning);
		LeaveState(State::Tranquilized);
		m_nLegDamage = 0;
		m_iConcussionTime = 0;

		MessageBegin(MSG_ONE, gmsgStatusIcon, this);
		WriteByte(0);
		WriteString("dmg_caltrop");
		MessageEnd();

		MessageBegin(MSG_ONE, gmsgStatusIcon, this);
		WriteByte(0);
		WriteString("dmg_poison");
		MessageEnd();

		MessageBegin(MSG_ONE, gmsgStatusIcon, this);
		WriteByte(0);
		WriteString("dmg_heat");
		MessageEnd();
	}

	if ((bitsDamageType & DMG_IGNORE_MAXHEALTH) != 0)
	{
		m_flNextRotTime = gpGlobals->time + 5.0F;
	}

	return result;
}

//=========================================================
// TraceAttack
//=========================================================
void CBasePlayer::TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, int hitgroup, int bitsDamageType)
{
	if (v.takedamage == DAMAGE_NO)
	{
		return;
	}

	m_LastHitGroup = hitgroup;

	switch (hitgroup)
	{
	case HITGROUP_GENERIC:
		break;
	case HITGROUP_HEAD:
		if ((bitsDamageType & DMG_AIMED) != 0)
		{
			flDamage *= 2;
#ifndef NDEBUG
			engine::AlertMessage(at_console, "HEAD SHOT\n");
#endif
		}
		break;
	case HITGROUP_CHEST:
		break;
	case HITGROUP_STOMACH:
		break;
	case HITGROUP_LEFTARM:
	case HITGROUP_RIGHTARM:
		break;
	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG:
		if ((bitsDamageType & DMG_CALTROP) != 0)
		{
			flDamage *= 0.5F;
			m_nLegDamage = std::min(m_nLegDamage + 1, 6);
			MessageBegin(MSG_ONE, gmsgStatusIcon, this);
			WriteByte(2);
			WriteString("dmg_caltrop");
			MessageEnd();
#ifndef NDEBUG
			engine::AlertMessage(at_console, "LEG SHOT\n");
#endif
		}
		break;
	default:
		break;
	}

	AddMultiDamage(flDamage, bitsDamageType);
}

float CBasePlayer::DamageForce(CBaseEntity* attacker, float damage)
{
	if (InState(State::CannotMove))
	{
		return 0.0F;
	}

	if (PCNumber() == PC_HVYWEAP)
	{
		if (damage < 50.0F)
		{
			return 0.0F;
		}

		damage /= 4.0F;
	}

	if ((v.flags & FL_DUCKING) != 0)
	{
		damage *= 2.0F;
	}

	return std::min(damage * 5.0F, 1000.0F);
}

float CBasePlayer::ArmorBonus(float damage, const int bitsDamageType)
{
	if (v.armorvalue <= 0 || (bitsDamageType & DMG_ARMOR_PIERCING) != 0)
	{
		return damage;
	}

	/* Toodles: These used to stack but, I doubt that was intentional. */
	if (((m_afArmorClass & AT_SAVESHOT) != 0 && (bitsDamageType & DMG_BULLET) != 0)
	 || ((m_afArmorClass & AT_SAVENAIL) != 0 && (bitsDamageType & DMG_NAIL) != 0)
	 || ((m_afArmorClass & AT_SAVEEXPLOSION) != 0 && (bitsDamageType & DMG_BLAST) != 0)
	 || ((m_afArmorClass & AT_SAVEELECTRICITY) != 0 && (bitsDamageType & DMG_SHOCK) != 0)
	 || ((m_afArmorClass & AT_SAVEFIRE) != 0 && (bitsDamageType & DMG_BURN) != 0))
	{
		damage *= 0.5F;
	}

	/*
		Toodles: Each point of armor is equivalent to one point of health.
		Armor type is the percent of damage that will be taken from armor, rather than health.
	*/
	float ratio = 1.0F - v.armortype;
	float newDamage = damage * ratio;
	float takeArmor = damage - newDamage;

	/* Absorbing this much damage would take more armor than the player has. */
	if (takeArmor >= v.armorvalue)
	{
		/* Add the difference back into the damage. */
		newDamage += takeArmor - v.armorvalue;
		takeArmor = v.armorvalue;

		/* All armor has been used up. Clear any types. */
		v.armortype = 0.0F;
		m_afArmorClass = 0;
	}

	v.dmg_save += takeArmor;
	v.armorvalue -= takeArmor;

	return newDamage;
}

void CBasePlayer::SendHitFeedback(CBaseEntity* victim, const float flDamage, const int bitsDamageType)
{
	int flags = 0;

	if (victim->v.health <= 0
	 || (victim->IsClient()
 	  && static_cast<CBasePlayer*>(victim)->InState(State::FeigningDeath)
	  && static_cast<CBasePlayer*>(victim)->m_iFeignTime == 0))
	{
		flags |= kDamageFlagDead;
	}

	if ((bitsDamageType & DMG_AIMED) != 0
	 && victim->IsClient()
	 && static_cast<CBasePlayer*>(victim)->m_LastHitGroup == HITGROUP_HEAD)
	{
		flags |= kDamageFlagHeadshot;
	}

	if (bitsDamageType == DMG_BURN || bitsDamageType == DMG_IGNOREARMOR)
	{
		flags |= kDamageFlagOverTime;
	}

	if (victim == this)
	{
		flags |= kDamageFlagSelf;
	}

	MessageBegin(MSG_ONE, gmsgHitFeedback, this);
	WriteByte(victim->v.GetIndex());
	WriteByte(flags);
	WriteShort(flDamage);
	MessageEnd();
}

bool CBasePlayer::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	if (v.takedamage == DAMAGE_NO || !IsAlive())
	{
		return false;
	}

	if (attacker->IsClient())
	{
		flDamage *= 0.9F;

		if (static_cast<CBasePlayer*>(attacker)->HasPowerUp(kSuperDamage))
		{
			flDamage *= 4.0F;

			bitsDamageType &= ~DMG_NEVERGIB;
		}
	}

	// Grab the vector of the incoming attack. (Pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit.)
	if (attacker != CWorld::World && attacker->v.solid != SOLID_TRIGGER
	 && (bitsDamageType & DMG_NO_KNOCKBACK) == 0)
	{
		// Move them around!
		g_vecAttackDir = (inflictor->Center() - Vector(0, 0, 10) - Center()).Normalize();

		float force = DamageForce(attacker, flDamage);

		if (force > 0.0F)
		{
			if ((bitsDamageType & DMG_NAIL) != 0)
			{
				v.velocity = v.velocity + inflictor->v.velocity.Normalize() * force;
			}
			else
			{
				v.velocity = v.velocity + g_vecAttackDir * -force;
			}
		}
	}

	if ((bitsDamageType & (DMG_ARMOR_PIERCING | DMG_BURN)) == 0)
	{
		v.dmg_inflictor = &inflictor->v;
	}
	else
	{
		v.dmg_inflictor = nullptr;
	}

	if (!g_pGameRules->FPlayerCanTakeDamage(this, attacker, inflictor))
	{
		return false;
	}

	m_bitsDamageType |= bitsDamageType;
	m_bitsHUDDamage = -1;

	if (HasPowerUp(kProtection))
	{
		EmitSound(kPowerUpEffectSounds[kProtection], CHAN_ITEM);

		return false;
	}

	// Check for godmode or invincibility.
	if ((v.flags & FL_GODMODE) != 0)
	{
		return false;
	}

	if (attacker->IsClient())
	{
		if (attacker != m_hLastAttacker[0])
		{
			m_hLastAttacker[1] = m_hLastAttacker[0];
		}
		m_hLastAttacker[0] = attacker;

		if ((bitsDamageType & DMG_RESIST_SELF) != 0 && attacker == this && (v.flags & FL_ONGROUND) == 0)
		{
			flDamage *= 0.6F;
		}
	}

	flDamage = ArmorBonus(flDamage, bitsDamageType);

	flDamage = ceilf(flDamage);

	// Do the damage!
	v.dmg_take += flDamage;
	v.health -= flDamage;

	if (v.health <= 0)
	{
		Killed(inflictor, attacker, bitsDamageType);

		if (util::DoDamageResponse(this, attacker))
		{
			static_cast<CBasePlayer*>(attacker)->SendHitFeedback(this, flDamage, bitsDamageType);
		}

		return false;
	}

	/* Toodles: Prevent damage tanking during combat. */

	m_flNextRegenerationTime = gpGlobals->time + 3.0F;
	m_flMegaHealthTime = gpGlobals->time + 10.0F;
	m_flNextRotTime = gpGlobals->time;

	MessageBegin(MSG_SPEC, SVC_DIRECTOR);
		WriteByte(9);							  // command length in bytes
		WriteByte(DRC_CMD_EVENT);				  // take damage event
		WriteShort(v.GetIndex());	  // index number of primary entity
		WriteShort(inflictor->v.GetIndex()); // index number of secondary entity
		WriteLong(5);							  // eventflags (priority and flags)
	MessageEnd();

	/* If the player survived the damage & is holding the feign button, attempt to feign death. */
	if (PCNumber() == PC_SPY && (v.button & IN_ATTACK2) != 0)
	{
		if (StartFeigningDeath(false, bitsDamageType))
		{
			/* Toodles: Send a faux death notice. */

			CBaseEntity* accomplice = m_hLastAttacker[1];

			if (accomplice == nullptr && m_hLastAttacker[0] != attacker)
			{
				/* No accomplice, yet, the last attacking player is not the killer. */
				accomplice = m_hLastAttacker[0];
			}

			if (accomplice == this)
			{
				/* Don't account for us assisting in our own death. */
				accomplice = nullptr;
			}

			g_pGameRules->DeathNotice(this, attacker, inflictor, accomplice, bitsDamageType);

			if (attacker->IsNetClient() && !g_pGameRules->CanSeeThroughDisguise(this, attacker))
			{
				static_cast<CBasePlayer*>(attacker)->SendHitFeedback(this, flDamage, bitsDamageType);
			}

			/* Return now to prevent pain sounds & effects applied by the damage type. */
			return false;
		}
	}

	const auto doDamageResponse = util::DoDamageResponse(this, attacker);

	if (flDamage >= 1.0F && doDamageResponse)
	{
		Pain(bitsDamageType);
	}

	if ((bitsDamageType & DMG_IGNITE) != 0)
	{
		Ignite(attacker);
	}

	if ((bitsDamageType & DMG_TRANQ) != 0)
	{
		BecomeTranquilized();
	}

	if (doDamageResponse)
	{
		static_cast<CBasePlayer*>(attacker)->SendHitFeedback(this, flDamage, bitsDamageType);
	}

	return true;
}

void CBasePlayer::RemoveAllWeapons()
{
#ifdef HALFLIFE_TANKCONTROL
	if (m_pTank != nullptr)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = nullptr;
	}
#endif

	for (int i = 0; i < WEAPON_TYPES; i++)
	{
		if (m_rgpPlayerWeapons[i] != nullptr)
		{
			m_rgpPlayerWeapons[i]->Remove();
			m_rgpPlayerWeapons[i] = nullptr;
		}
	}

	memset(m_rgAmmo, 0, sizeof(m_rgAmmo));
}


void CBasePlayer::RemoveAllObjects()
{
	auto e = util::GetEntityList();

	if (e == nullptr)
	{
		return;
	}

	/* Ignore the world. */
	e++;

	/*
		Toodles TODO: This doesn't remove client-side nails, obviously.
		That's not a huge deal but, it'd be a nice feature to have.
	*/
#ifndef NDEBUG
	auto count = 0;
#endif

	for (auto i = 1; i < gpGlobals->maxEntities; i++, e++)
	{
		if (e->free != 0 || (e->flags & FL_KILLME) != 0 || e->owner != &v)
		{
			continue;
		}

		auto entity = e->Get<CBaseEntity>();
		if (entity == nullptr)
		{
			continue;
		}

		if ((entity->ObjectCaps() & FCAP_DONT_SAVE) == 0)
		{
			continue;
		}

		entity->Remove();
#ifndef NDEBUG
		count++;
#endif
	}

#ifndef NDEBUG
	engine::AlertMessage(
		at_aiconsole,
		"Removed %i entities owned by '%s'\n",
		count,
		STRING(v.netname));
#endif
}


void CBasePlayer::Killed(CBaseEntity* inflictor, CBaseEntity* attacker, int bitsDamageType)
{
	CBaseEntity* accomplice = m_hLastAttacker[1];

	if (accomplice == nullptr && m_hLastAttacker[0] != attacker)
	{
		/* No accomplice, yet, the last attacking player is not the killer. */
		accomplice = m_hLastAttacker[0];
	}

	if (accomplice == this)
	{
		/* Don't account for us assisting in our own death. */
		accomplice = nullptr;
	}

	g_pGameRules->PlayerKilled(this, attacker, inflictor, accomplice, bitsDamageType);

	SetUseObject(nullptr);
	DropBackpack();
	RemoveAllWeapons();
	RemoveGoalItems();

	m_iActiveWeapon = m_iLastWeapon = WEAPON_NONE;

	SetAction(Action::Die);

	m_fDeadTime = gpGlobals->time;
	m_fNextSuicideTime = gpGlobals->time + 1.0f;

	v.health = std::min(v.health, 0.0F);
	v.deadflag = DEAD_DYING;

	m_iFOV = 0;

	ClearEffects();

	m_iObserverLastMode = OBS_CHASE_FREE;
	v.iuser1 = OBS_DEATHCAM;
	v.iuser2 = v.GetIndex();
	v.iuser3 = attacker->v.GetIndex();

	v.solid = SOLID_NOT;
	v.movetype = MOVETYPE_NONE;
	v.effects |= EF_NODRAW | EF_NOINTERP;

	auto gibMode = GIB_NORMAL;
	if ((bitsDamageType & DMG_ALWAYSGIB) != 0)
	{
		gibMode = GIB_ALWAYS;
	}
	else if ((bitsDamageType & DMG_NEVERGIB) != 0)
	{
		gibMode = GIB_NEVER;
	}

	if (gibMode == GIB_NEVER || (gibMode != GIB_ALWAYS && v.health >= -40.0f))
	{
		DeathSound(bitsDamageType);
	}

	tent::SpawnCorpse(this, gibMode);
}


void CBasePlayer::PlayerDeathFrame()
{
	const auto bIsMultiplayer = util::IsMultiplayer();

	// If the player has been dead for 5 seconds,
	// send the player off to an intermission
	// camera until they respawn.
	if (bIsMultiplayer
	 && (!IsObserver() || v.iuser1 == OBS_DEATHCAM)
	 && (m_fDeadTime + 5.0f) <= gpGlobals->time)
	{
		StartObserver();
	}

	if (!g_pGameRules->FPlayerCanRespawn(this))
	{
		return;
	}

	if (bIsMultiplayer)
	{
		if ((int)forcerespawn.value != 0
		 || m_afButtonPressed != 0
		 || IsBot())
		{
			m_afButtonPressed = 0;
			v.button = 0;
			m_afButtonReleased = 0;
			Spawn();
		}
	}
	else
	{
		if (m_afButtonPressed != 0)
		{
			engine::ServerCommand("reload\n");
		}
	}
}

void CBasePlayer::StartObserver()
{
	// clear any clientside entities attached to this player
	MessageBegin(MSG_PAS, SVC_TEMPENTITY, v.origin);
	WriteByte(TE_KILLPLAYERATTACHMENTS);
	WriteByte(v.GetIndex());
	MessageEnd();

	// reset FOV
	m_iFOV = 0;

	// Setup flags
	v.effects = EF_NOINTERP | EF_NODRAW;
	v.view_ofs = g_vecZero;
	v.fixangle = 1;
	v.solid = SOLID_NOT;
	v.takedamage = DAMAGE_NO;
	v.movetype = MOVETYPE_NONE;
	v.flags &= ~FL_DUCKING;
	v.deadflag = DEAD_RESPAWNABLE;
	v.health = 1;

	m_ResetHUD = ResetHUD::Reset;

	// Find a player to watch
	m_flNextObserverInput = 0;
	Observer_SetMode(m_iObserverLastMode);
}

//
// PlayerUse - handles USE keypress
//
#define PLAYER_SEARCH_RADIUS (float)64

void CBasePlayer::PlayerUse()
{
	if ((m_afButtonReleased & IN_USE) != 0)
	{
		SetUseObject(nullptr);
		return;
	}

	if (m_hUseObject != nullptr)
	{
		if ((v.button & IN_USE) != 0
	 	 && CanUseObject(m_hUseObject, PLAYER_SEARCH_RADIUS * 1.5F, VIEW_FIELD_NARROW))
		{
			CBaseEntity* object = m_hUseObject;
			object->Use(this, this, USE_CONTINUOUS, 1);
			return;
		}

		SetUseObject(nullptr);
	}

	if (!IsPlayer())
	{
		return;
	}

	// Was use pressed or released?
	if ((m_afButtonPressed & IN_USE) == 0)
	{
		return;
	}

#ifdef HALFLIFE_TANKCONTROL
	if (m_pTank != nullptr)
	{
		// Stop controlling the tank
		// TODO: Send HUD Update
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = nullptr;
		return;
	}
#endif

#ifdef HALFLIFE_TRAINCONTROL
	// Hit Use on a train?
	if ((v.flags & FL_ONTRAIN) != 0)
	{
		v.flags &= ~FL_ONTRAIN;
		m_iTrain = TRAIN_NEW | TRAIN_OFF;
		return;
	}
	
	// Start controlling the train!
	if (v.groundentity != nullptr)
	{
		CBaseEntity* pTrain = v.groundentity->Get<CBaseEntity>();

		if ((v.button & IN_JUMP) == 0
		&& (v.flags & FL_ONGROUND) != 0
		&& (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) != 0
		&& pTrain->OnControls(this))
		{
			v.flags |= FL_ONTRAIN;
			m_iTrain = TrainSpeed(pTrain->v.speed, pTrain->v.impulse);
			m_iTrain |= TRAIN_NEW;
			EmitSound("plats/train_use1.wav", CHAN_VOICE);
			return;
		}
	}
#endif

	CBaseEntity* current = nullptr;
	CBaseEntity* closest = nullptr;
	auto maxDot = VIEW_FIELD_NARROW;

	util::MakeVectors(v.v_angle); // so we know which way we are facing

	while ((current = util::FindEntityInSphere(current, v.origin, PLAYER_SEARCH_RADIUS)) != nullptr)
	{
		if ((current->ObjectCaps() & (FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE)) == 0)
		{
			continue;
		}

		// This essentially moves the origin of the target to the corner nearest the player to test to see
		// if it's "hull" is in the view cone
		const auto eyeDelta = current->Center() - EyePosition();
		const auto los = util::ClampVectorToBox(eyeDelta, current->v.size / 2.0F);

		auto dot = DotProduct(los, gpGlobals->v_forward);

		// only if the item is in front of the user
		if (dot > maxDot)
		{
			closest = current;
			maxDot = dot;
		}
	}

	SetUseObject(closest);

	if (closest != nullptr)
	{
		EmitSoundHUD("common/wpn_select.wav", CHAN_VOICE);
	}
	else
	{
		EmitSoundHUD("common/wpn_denyselect.wav", CHAN_VOICE);
	}
}


bool CBasePlayer::CanUseObject(CBaseEntity* object, const float maxDistance, const float maxDot)
{
	if ((object->ObjectCaps() & (FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE)) == 0)
	{
		return false;
	}

	const auto distance = (object->v.origin - v.origin).Length();

	if (distance > maxDistance)
	{
		return false;
	}

	const auto eyeDelta = object->Center() - EyePosition();
	const auto los = util::ClampVectorToBox(eyeDelta, object->v.size / 2.0F);

	util::MakeVectors(v.v_angle);

	const auto dot = DotProduct(los, gpGlobals->v_forward);

	return dot > maxDot;
}


void CBasePlayer::SetUseObject(CBaseEntity* object)
{
	if (m_hUseObject != nullptr)
	{
		/* End the previous continuous use. */
		CBaseEntity* current = m_hUseObject;
		current->Use(this, this, USE_CONTINUOUS_END, 1);
	}

	if (object == nullptr)
	{
		/* Clear the previous object. */
		m_hUseObject = nullptr;
	}
	else
	{
		if ((object->ObjectCaps() & FCAP_CONTINUOUS_USE) != 0)
		{
			/* Begin continuous use. */
			m_hUseObject = object;
			object->Use(this, this, USE_CONTINUOUS_BEGIN, 1);
		}
		else
		{
			/* One-time impulse use. */
			object->Use(this, this, USE_SET, 1);
		}
	}
}


void CBasePlayer::PowerUpThink()
{
	for (int i = 0; i < kPowerUps; i++)
	{
		if ((m_afPowerUpActive & (1 << i)) == 0)
		{
			continue;
		}

		const auto timeLeft =
			m_flPowerUpFinished[i] - gpGlobals->time;

		/* Notify the player that their power-up is going to expire. */

		if ((m_afPowerUpActive & (kPowerUpExpireShift << i)) == 0
		 && timeLeft <= 3.0F)
		{
			EmitSoundHUD(kPowerUpExpireSounds[i], CHAN_STATIC);

			util::ClientPrint(this, HUD_PRINTCENTER,
				kPowerUpExpireMessages[i]);

			m_afPowerUpActive |= kPowerUpExpireShift << i;
		}

		if (timeLeft <= 0.0F)
		{
			RemovePowerUp(i);
		}
	}
}


bool CBasePlayer::GivePowerUp(const int powerUpID, const float duration,
	const bool playSound = true)
{
	if (HasPowerUp(powerUpID))
	{
		/* Toodles: Make sure it extends the timer. */

		if (m_flPowerUpFinished[powerUpID] >= gpGlobals->time + duration)
		{
			return false;
		}
	}
	else if (playSound)
	{
		EmitSound(kPowerUpSounds[powerUpID], CHAN_STATIC);
	}

	m_flPowerUpFinished[powerUpID] = gpGlobals->time + duration;

	m_afPowerUpActive |= 1 << powerUpID;

	if (duration > 3.0F)
	{
		m_afPowerUpActive &= ~(kPowerUpExpireShift << powerUpID);
	}
	else
	{
		m_afPowerUpActive |= kPowerUpExpireShift << powerUpID;
	}

	return true;
}


void CBasePlayer::RemovePowerUp(const int powerUpID, const bool playSound)
{
	m_afPowerUpActive &= ~(1 << powerUpID);
}


bool CBasePlayer::HasPowerUp(const int powerUpID)
{
	return (m_afPowerUpActive & (1 << powerUpID)) != 0;
}


void CBasePlayer::PreThink()
{
	/* Prevent attacking while feigning death. */
	if (InState(State::FeigningDeath) || m_iFeignTime != 0)
	{
		v.button &= ~IN_ATTACK;
	}

	int buttonsChanged = (m_afButtonLast ^ v.button); // These buttons have changed this frame

	// Debounced button codes for pressed/released
	m_afButtonPressed = buttonsChanged & v.button;	  // The changed ones still down are "pressed"
	m_afButtonReleased = buttonsChanged & (~v.button); // The ones not down are "released"

	g_pGameRules->PlayerThink(this);

	UpdateClientData();

	if (g_pGameRules->GetState() == GR_STATE_GAME_OVER)
		return; // intermission or finale

	if (v.iuser1 == OBS_FIXED)
	{
		return;
	}

	// Observer Button Handling
	if (IsObserver())
	{
		Observer_HandleButtons();
		Observer_CheckTarget();
		v.impulse = 0;

		if (IsSpectator())
		{
			return;
		}
	}

	//Run Powerup think (removes powerups over time, etc)
	PowerUpThink();

	if (!IsAlive())
	{
		PlayerDeathFrame();
		return;
	}

	// Train speed control
#ifdef HALFLIFE_TRAINCONTROL
	if ((v.flags & FL_ONTRAIN) != 0)
	{
		CBaseEntity* pTrain = nullptr;
		float vel;

		if (v.groundentity != nullptr)
		{
			pTrain = v.groundentity->Get<CBaseEntity>();
		}

		if (!pTrain)
		{
			TraceResult trainTrace;
			// Maybe this is on the other side of a level transition
			util::TraceLine(v.origin, v.origin + Vector(0, 0, -38), util::ignore_monsters, this, &trainTrace);

			// HACKHACK - Just look for the func_tracktrain classname
			if (trainTrace.flFraction != 1.0 && trainTrace.pHit)
				pTrain = trainTrace.pHit->Get<CBaseEntity>();


			if (!pTrain || (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) == 0 || !pTrain->OnControls(this))
			{
				//engine::AlertMessage( at_error, "In train mode with no train!\n" );
				v.flags &= ~FL_ONTRAIN;
				m_iTrain = TRAIN_NEW | TRAIN_OFF;
				return;
			}
		}
		else if ((v.flags & FL_ONGROUND) == 0 || (pTrain->v.spawnflags & SF_TRACKTRAIN_NOCONTROL) != 0 || (v.button & (IN_MOVELEFT | IN_MOVERIGHT)) != 0)
		{
			// Turn off the train if you jump, strafe, or the train controls go dead
			v.flags &= ~FL_ONTRAIN;
			m_iTrain = TRAIN_NEW | TRAIN_OFF;
			return;
		}

		v.velocity = g_vecZero;
		vel = 0;
		if ((m_afButtonPressed & IN_FORWARD) != 0)
		{
			vel = 1;
			pTrain->Use(this, this, USE_SET, (float)vel);
		}
		else if ((m_afButtonPressed & IN_BACK) != 0)
		{
			vel = -1;
			pTrain->Use(this, this, USE_SET, (float)vel);
		}

		if (0 != vel)
		{
			m_iTrain = TrainSpeed(pTrain->v.speed, pTrain->v.impulse);
			m_iTrain |= TRAIN_ACTIVE | TRAIN_NEW;
		}
	}
	else if ((m_iTrain & TRAIN_ACTIVE) != 0)
		m_iTrain = TRAIN_NEW; // turn off train
#endif

	if ((v.flags & FL_ONGROUND) == 0)
	{
		m_flFallVelocity = -v.velocity.z;
	}

	if (InState(State::Infected) != 0 && m_flNextInfectionTime <= gpGlobals->time)
	{
		CBaseEntity* infector;
		infector = m_hInfector;
		
		if (infector == nullptr)
		{
			infector = CWorld::World;
		}
		
		if (TakeDamage(infector, infector, 8, DMG_IGNOREARMOR) && infector != CWorld::World)
		{
			CBaseEntity* e = nullptr;
			while ((e = util::FindEntityInSphere(e, v.origin, 80.0F)) != nullptr)
			{
				if (e->IsPlayer()
				 && e->IsAlive()
				 && g_pGameRules->PlayerRelationship(this, e) >= GR_ALLY)
				{
					dynamic_cast<CBasePlayer*>(e)->BecomeInfected(infector);
				}
			}
		}
		else
		{
			LeaveState(State::Infected);

			MessageBegin(MSG_ONE, gmsgStatusIcon, this);
			WriteByte(0);
			WriteString("dmg_poison");
			MessageEnd();
		}

		m_flNextInfectionTime = gpGlobals->time + 3.0F;
	}

	if (InState(State::Burning) && m_flNextBurnTime <= gpGlobals->time)
	{
		CBaseEntity* burner;
		burner = m_hBurner;
		
		if (burner == nullptr)
		{
			burner = CWorld::World;
		}

		auto burnDamage = 0;

		/* Check each burn source. */
		for (auto i = 0; i < kMaxBurnSources; i++)
		{
			if (m_nBurnCount[i] == 0)
			{
				continue;
			}

			/* Decrement timer. */
			m_nBurnCount[i]--;

			/* Increment damage. */
			burnDamage++;
		}

		if (!CanBurn() || !TakeDamage(burner, burner, burnDamage, DMG_BURN | DMG_NO_KNOCKBACK)
		 || burnDamage == 0)
		{
			/* Cannot burn, blocked damage, or all sources have expired. */
			Extinguish();
		}

		m_flNextBurnTime = gpGlobals->time + 0.3F;
	}

	if (PCNumber() == PC_MEDIC && m_flNextRegenerationTime <= gpGlobals->time)
	{
		GiveHealth(2, DMG_GENERIC, false);
		m_flNextRegenerationTime = gpGlobals->time + 3.0F;
	}

	if (v.health > v.max_health && m_flNextRotTime <= gpGlobals->time)
	{
		v.health = std::max(v.health - 1.0F, v.max_health);

		m_flNextRotTime = gpGlobals->time + 1.0F;
	}

	if (InState(State::Tranquilized) && m_flTranquilizationTime <= gpGlobals->time)
	{
		LeaveState(State::Tranquilized);
	}
}

bool CBasePlayer::Spawn()
{
	m_bIsSpawning = true;

	//Make sure this gets reset even if somebody adds an early return or throws an exception.
	const CallOnDestroy resetIsSpawning{[this]()
		{
			//Done spawning; reset.
			m_bIsSpawning = false;
			util::FireTargets("game_playerspawn", this, this, USE_TOGGLE, 0);
		}};

	Precache();

	v.classname = MAKE_STRING("player");
	v.health = 100;
	v.armorvalue = 0;
	v.armortype = 0.0F;
	m_flArmorMax = 100;
	m_flArmorTypeMax = 0.8F;
	m_afArmorClass = 0;
	v.takedamage = DAMAGE_AIM;
	v.solid = SOLID_SLIDEBOX;
	v.movetype = MOVETYPE_WALK;
	v.max_health = v.health;
	v.flags &= FL_PROXY | FL_FAKECLIENT; // keep proxy and fakeclient flags set by engine
	v.flags |= FL_CLIENT;
	v.air_finished = gpGlobals->time + 12;
	v.dmg = 2; // initial water damage
	v.effects = EF_NOINTERP;
	v.deadflag = DEAD_NO;
	v.dmg_take = 0;
	v.dmg_save = 0;
	v.friction = 1.0;
	v.gravity = 1.0;
	m_bitsHUDDamage = -1;
	m_bitsDamageType = 0;
	v.iuser1 = OBS_NONE;
	v.iuser2 = v.iuser3 = 0;
	m_hLastAttacker[0] = m_hLastAttacker[1] = nullptr;
	m_hGrenade = nullptr;
	m_iGrenadeExplodeTime = 0;
	m_TFItems = 0;

	m_iFOV = 0;
	m_ClientSndRoomtype = -1;

	m_flNextRegenerationTime = gpGlobals->time;
	m_flMegaHealthTime = gpGlobals->time;
	m_flNextRotTime = gpGlobals->time;

	m_flNextDecalTime = -decalfrequency.value; // let this player decal as soon as they spawn

	// dont let uninitialized value here hurt the player
	m_flFallVelocity = 0;

	auto spawn = g_pGameRules->GetPlayerSpawnSpot(this);

	v.origin = spawn->m_origin;
	v.v_angle = g_vecZero;
	v.velocity = g_vecZero;
	v.angles = spawn->m_angles;
	v.punchangle = g_vecZero;
	v.fixangle = 1;

	SetModel("models/player/scout/scout.mdl");
	SetAction(Action::Idle, true);

	Vector mins, maxs;
	if (PM_GetHullBounds(
		(v.flags & FL_DUCKING) != 0 ? kHullPlayer : kHullDuck, mins, maxs))
	{
		SetSize(mins, maxs);
	}

	v.view_ofs = VEC_VIEW;

	if (m_ResetHUD == ResetHUD::No)
	{
		m_ResetHUD = ResetHUD::Reset;
	}
	m_ClientWeaponBits = 0;

	// reset all ammo values to 0
	memset(m_rgAmmo, 0, sizeof(m_rgAmmo));

	MessageBegin(MSG_ONE, gmsgAmmo, this);
	for (int i = 0; i < AMMO_TYPES; i++)
	{
		WriteByte(g_pGameRules->GetMaxAmmo(this, i));
	}
	MessageEnd();

	ClearEffects();

	m_flSpeedReduction = 0.0F;

	m_iActiveWeapon = m_iLastWeapon = WEAPON_NONE;

	char* infobuffer = engine::GetInfoKeyBuffer(&v);
	char* value = engine::InfoKeyValue(infobuffer, "cl_righthand");

	if ('\0' != *value)
	{
		m_bLeftHanded = atoi(value) == 0;
	}
	else
	{
		m_bLeftHanded = false;
	}

	g_pGameRules->PlayerSpawn(this);

	if (IsNetClient())
	{
		const auto &info = sTFClassInfo[PCNumber()];

		for (auto i = 0; i < 2; i++)
		{
			const auto grenadeType = info.grenades[i];

			const char *icon = GetGrenadeIconName(grenadeType);

			if (icon == nullptr)
			{
				icon = "grenade";
			}

			MessageBegin(MSG_ONE, gmsgSecAmmoIcon, this);
			WriteByte(AMMO_SECONDARY + i);
			WriteString(icon);
			MessageEnd();
		}
	}

    const auto goal = spawn->GetTFSpawn();
    if (goal != nullptr)
    {
        goal->AttemptToActivate(this, &goal->tfv);
    }

	return true;
}


void CBasePlayer::Precache()
{
	m_bitsDamageType = 0;
	m_bitsHUDDamage = -1;

#ifdef HALFLIFE_TRAINCONTROL
	m_iTrain |= TRAIN_NEW;
#endif

	// Make sure any necessary user messages have been registered
	LinkUserMessages();
}


#ifdef HALFLIFE_SAVERESTORE
bool CBasePlayer::Save(CSave& save)
{
	if (!CBaseAnimating::Save(save))
		return false;

	return save.WriteFields("PLAYER", this, m_playerSaveData, ARRAYSIZE(m_playerSaveData));
}


bool CBasePlayer::Restore(CRestore& restore)
{
	if (!CBaseAnimating::Restore(restore))
		return false;

	bool status = restore.ReadFields("PLAYER", this, m_playerSaveData, ARRAYSIZE(m_playerSaveData));

	SAVERESTOREDATA* pSaveData = (SAVERESTOREDATA*)gpGlobals->pSaveData;
	// landmark isn't present.
	if (0 == pSaveData->fUseLandmark)
	{
		engine::AlertMessage(at_console, "No Landmark:%s\n", pSaveData->szLandmarkName);

		// default to normal spawn
		auto spawn = g_pGameRules->GetPlayerSpawnSpot(this);
		v.origin = spawn->m_origin;
		v.angles = spawn->m_angles;
	}
	v.v_angle.z = 0; // Clear out roll
	v.angles = v.v_angle;

	v.fixangle = 1; // turn this way immediately

	m_ClientSndRoomtype = -1;

	// Reset room type on level change.
	if (!streq(restore.GetData().szCurrentMapName, STRING(gpGlobals->mapname)))
	{
		m_SndRoomtype = 0;
	}

	Vector mins, maxs;
	if (PM_GetHullBounds(
		(v.flags & FL_DUCKING) != 0 ? kHullPlayer : kHullDuck, mins, maxs))
	{
		SetSize(mins, maxs);
	}

	return status;
}
#endif


const char* CBasePlayer::TeamID()
{
	if (!m_team)
	{
		return "";
	}

	// return their team name
	return m_team->m_name.c_str();
}


//==============================================
// SprayCan entity to apply
// decal frame at a time. For PreAlpha CD
//==============================================
class CSprayCan : public CBaseEntity
{
public:
	CSprayCan(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	bool Spawn(CBaseEntity* owner);
	void Think() override;

#ifdef HALFLIFE_SAVERESTORE
	int ObjectCaps() override { return FCAP_DONT_SAVE; }
#endif
};

bool CSprayCan::Spawn(CBaseEntity* owner)
{
	v.origin = owner->v.origin + Vector(0, 0, 32);
	v.angles = owner->v.v_angle;
	v.owner = &owner->v;
	v.frame = 0;

	v.nextthink = gpGlobals->time + 0.1;
	EmitSound("player/sprayer.wav", CHAN_VOICE);

	return true;
}

void CSprayCan::Think()
{
	TraceResult tr;
	int playernum;
	int nFrames;
	CBasePlayer* pPlayer;

	pPlayer = v.owner->Get<CBasePlayer>();

	if (pPlayer)
		nFrames = pPlayer->GetCustomDecalFrames();
	else
		nFrames = -1;

	playernum = v.owner->GetIndex();

	// engine::AlertMessage(at_console, "Spray by player %i, %i of %i\n", playernum, (int)(v.frame + 1), nFrames);

	util::MakeVectors(v.angles);
	util::TraceLine(v.origin, v.origin + gpGlobals->v_forward * 128, util::ignore_monsters, pPlayer, &tr);

	tent::PlayerDecalTrace(&tr, playernum, v.frame);
	// Just painted last custom frame.
	if (v.frame++ >= (nFrames - 1))
		Remove();

	v.nextthink = gpGlobals->time + 0.1;
}


//==============================================

static Entity* GiveNamedItem_Common(CBaseEntity* toWhom, const char* pszName)
{
	int istr = MAKE_STRING(pszName);

	Entity* pent = engine::CreateNamedEntity(istr);
	if (pent == nullptr)
	{
		engine::AlertMessage(at_console, "NULL Ent in GiveNamedItem!\n");
		return nullptr;
	}
	pent->origin = toWhom->v.origin;
	pent->spawnflags |= SF_NORESPAWN;

	DispatchSpawn(pent);

	return pent;
}

void CBasePlayer::GiveNamedItem(const char* szName)
{
	auto pent = GiveNamedItem_Common(this, szName);

	if (!pent)
	{
		return;
	}

	DispatchTouch(pent, &v);
}

/*
===============
ForceClientDllUpdate

When recording a demo, we need to have the server tell us the entire client state
so that the client side .dll can behave correctly.
Reset stuff so that the state is transmitted.
===============
*/
void CBasePlayer::ForceClientDllUpdate()
{
	m_ClientWeaponBits = 0;
	m_ClientSndRoomtype = -1;

#ifdef HALFLIFE_TRAINCONTROL
	m_iTrain |= TRAIN_NEW; // Force new train message.
#endif

	m_ResetHUD = ResetHUD::Reset;
}

/*
============
ImpulseCommands
============
*/
void CBasePlayer::ImpulseCommands()
{
	TraceResult tr;

	// Handle use events
	PlayerUse();

	int iImpulse = (int)v.impulse;
	switch (iImpulse)
	{
	case 201: // paint decal

		if (gpGlobals->time - m_flNextDecalTime < decalfrequency.value
		 || GetCustomDecalFrames() == -1)
		{
			// too early!
			break;
		}

		util::MakeVectors(v.v_angle);
		util::TraceLine(v.origin + v.view_ofs, v.origin + v.view_ofs + gpGlobals->v_forward * 128, util::ignore_monsters, this, &tr);

		if (tr.flFraction != 1.0)
		{ // line hit something, so paint a decal
			m_flNextDecalTime = gpGlobals->time;
			CSprayCan* pCan = Entity::Create<CSprayCan>();
			pCan->Spawn(this);
		}

		break;

	default:
		if (g_pGameRules->IsPlayerPrivileged(this))
		{
			// check all of the cheat impulse commands now
			CheatImpulseCommands(iImpulse);
		}
		break;
	}

	v.impulse = 0;
}

//=========================================================
//=========================================================
void CBasePlayer::CheatImpulseCommands(int iImpulse)
{
	CBaseEntity* pEntity;
	TraceResult tr;

	switch (iImpulse)
	{
	case 101:
		gEvilImpulse101 = true;
		{
			const auto& classInfo = sTFClassInfo[PCNumber()];

			GiveHealth(classInfo.maxHealth, DMG_GENERIC);
			GiveArmor(classInfo.maxArmorType, classInfo.maxArmor);

			for (int i = 0; i < AMMO_TYPES; i++)
				GiveAmmo(classInfo.maxAmmo[i], i);
		}
		gEvilImpulse101 = false;
		break;

	case 104:
		// Dump all of the global state varaibles (and global entity names)
		gGlobalState.DumpGlobals();
		break;

	case 106:
		// Give me the classname and targetname of this entity.
		pEntity = util::FindEntityForward(this);
		if (pEntity)
		{
			engine::AlertMessage(at_console, "Classname: %s", STRING(pEntity->v.classname));

			if (!FStringNull(pEntity->v.targetname))
			{
				engine::AlertMessage(at_console, " - Targetname: %s\n", STRING(pEntity->v.targetname));
			}
			else
			{
				engine::AlertMessage(at_console, " - TargetName: No Targetname\n");
			}

			engine::AlertMessage(at_console, "Model: %s\n", STRING(pEntity->v.model));
			if (!FStringNull(pEntity->v.globalname))
				engine::AlertMessage(at_console, "Globalname: %s\n", STRING(pEntity->v.globalname));
		}
		break;

	case 107:
	{
		TraceResult tr;

		Entity* pWorld = &CWorld::World->v;

		Vector start = v.origin + v.view_ofs;
		Vector end = start + gpGlobals->v_forward * 1024;
		util::TraceLine(start, end, util::ignore_monsters, this, &tr);
		if (tr.pHit)
			pWorld = tr.pHit;
		const char* pTextureName = engine::TraceTexture(pWorld, start, end);
		if (pTextureName)
			engine::AlertMessage(at_console, "Texture: %s (%c)\n", pTextureName, PM_FindTextureType(pTextureName));
	}
	break;

#ifdef HALFLIFE_NODEGRAPH
	case 195: // show shortest paths for entire level to nearest node
	{
		if (WorldGraph.IsAvailable())
		{
			Create("node_viewer_fly", v.origin, v.angles);
		}
	}
	break;
	case 196: // show shortest paths for entire level to nearest node
	{
		if (WorldGraph.IsAvailable())
		{
			Create("node_viewer_large", v.origin, v.angles);
		}
	}
	break;
	case 197: // show shortest paths for entire level to nearest node
	{
		if (WorldGraph.IsAvailable())
		{
			Create("node_viewer_human", v.origin, v.angles);
		}
	}
	break;
	case 199: // show nearest node and all connections
	{
		if (WorldGraph.IsAvailable())
		{
			engine::AlertMessage(at_console, "%d\n", WorldGraph.FindNearestNode(v.origin, bits_NODE_GROUP_REALM));
			WorldGraph.ShowNodeConnections(WorldGraph.FindNearestNode(v.origin, bits_NODE_GROUP_REALM));
		}
	}
	break;
#endif

	case 203: // remove creature.
		pEntity = util::FindEntityForward(this);
		if (pEntity)
		{
			if (0 != pEntity->v.takedamage)
			{
				pEntity->Remove();
			}
		}
		break;
	}
}

//
// Add a weapon to the player
//
void CBasePlayer::AddPlayerWeapon(CBasePlayerWeapon* weapon)
{
	m_rgpPlayerWeapons[weapon->GetID()] = weapon;
	m_lpPlayerWeapons.push_front(weapon);
	SetWeaponBit(weapon->GetID());

	g_pGameRules->PlayerGotWeapon(this, weapon);

	if (!m_bIsSpawning && (weapon->iFlags() & WEAPON_FLAG_EXHAUSTIBLE) == 0)
	{
		MessageBegin(MSG_ONE, gmsgWeapPickup, this);
		WriteByte(weapon->GetID());
		MessageEnd();
	}

	if (g_pGameRules->FShouldSwitchWeapon(this, weapon))
	{
		weapon->m_ForceSendAnimations = true;
		SelectWeapon(weapon->GetID());
		weapon->m_ForceSendAnimations = false;
	}
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

	m_rgpPlayerWeapons[weapon->GetID()] = nullptr;
	m_lpPlayerWeapons.remove(weapon);
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


int CBasePlayer::GiveAmmo(int iCount, int iType)
{
	if (!g_pGameRules->CanHaveAmmo(this, iType))
	{
		// game rules say I can't have any more of this ammo type.
		return 0;
	}

	int iAdd = std::min(iCount, g_pGameRules->GetMaxAmmo(this, iType) - m_rgAmmo[iType]);

	if (iAdd < 1)
	{
		return 0;
	}

	m_rgAmmo[iType] += iAdd;

	if (0 != gmsgAmmoPickup) // make sure the ammo messages have been linked first
	{
		// Send the message that ammo has been picked up
		MessageBegin(MSG_ONE, gmsgAmmoPickup, this);
		WriteByte(iType); // ammo ID
		WriteByte(iAdd); // amount
		MessageEnd();
	}

	return iAdd;
}


void CBasePlayer::DropBackpack()
{
	if (g_pGameRules->DeadPlayerAmmo(this) != GR_PLR_DROP_AMMO_NO)
	{
		if (!IsAlive())
		{
			CItem::DropBackpack(
				this,
				m_rgAmmo[AMMO_SHELLS],
				m_rgAmmo[AMMO_NAILS],
				m_rgAmmo[AMMO_ROCKETS],
				m_rgAmmo[AMMO_CELLS],
				true);

			m_rgAmmo[AMMO_SHELLS] = 0;
			m_rgAmmo[AMMO_NAILS] = 0;
			m_rgAmmo[AMMO_ROCKETS] = 0;
			m_rgAmmo[AMMO_CELLS] = 0;
		}
		else
		{
			CItem::DropBackpack(this, 0, 0, 0, 0, true);
		}
	}
}

bool CBasePlayer::ConvertCells(byte discardAmmo[])
{
	auto discarded = false;

	auto totalCells = 0;

	/* Tally up all of the cells we need. */

	for (int i = 0; i < AMMO_SECONDARY; i++)
	{
		if (discardAmmo[i] >= kAmmoCellMax[i])
		{
			continue;
		}

		totalCells += (kAmmoCellMax[i] - discardAmmo[i])
			* kAmmoCellCost[i];
	}

	/* Scale the total by how many cells we actually have. */

	const auto discardScale = std::min(
		m_rgAmmo[AMMO_CELLS] / (float)totalCells, 1.0F);

	for (int j = 0; j < AMMO_SECONDARY; j++)
	{
		/* Sort by priority. */

		const auto i = kAmmoCellPriority[j];

		if (discardAmmo[i] >= kAmmoCellMax[i])
		{
			continue;
		}

		/* How many cells to spend. */

		const int cells = (kAmmoCellMax[i] - discardAmmo[i])
			* kAmmoCellCost[i] * discardScale;

		/* How many rounds it will make. */

		const auto convertAmmo = cells / kAmmoCellCost[i];

		if (convertAmmo <= 0)
		{
			continue;
		}

		/* Spend the cells & convert the rounds! */

		m_rgAmmo[AMMO_CELLS] -= cells;

		discardAmmo[i] += convertAmmo;

		discarded = true;
	}

	return discarded;
}

void CBasePlayer::DiscardAmmo()
{
	if (!IsPlayer() || !IsAlive())
	{
		return;
	}

	bool shouldDiscard[AMMO_SECONDARY] =
	{
		true,
		true,
		true,
		true,
	};

	byte discardAmmo[AMMO_SECONDARY];

	memcpy(discardAmmo, m_rgAmmo, AMMO_SECONDARY);

	for (auto weapon : m_lpPlayerWeapons)
	{
		const auto info = weapon->GetInfo();

		if (info.iAmmo1 != -1 && info.iAmmo1 < AMMO_SECONDARY && info.iShots != 0)
		{
			shouldDiscard[info.iAmmo1] = false;
			discardAmmo[info.iAmmo1] = 0;
		}
	}

	auto discarded = false;

	for (int i = 0; i < AMMO_SECONDARY; i++)
	{
		if (!shouldDiscard[i])
		{
			continue;
		}

		if (discardAmmo[i] == 0)
		{
			shouldDiscard[i] = false;
			continue;
		}

		m_rgAmmo[i] = 0;

		discarded = true;
	}

	if (PCNumber() == PC_ENGINEER)
	{
		/* Engineers convert cells into other ammo types. */

		if (ConvertCells(discardAmmo))
		{
			discarded = true;
		}
	}

	if (!discarded)
	{
		return;
	}

	CItem::DropBackpack(
		this,
		discardAmmo[AMMO_SHELLS],
		discardAmmo[AMMO_NAILS],
		discardAmmo[AMMO_ROCKETS],
		discardAmmo[AMMO_CELLS],
		false);
}


/*
=========================================================
	UpdateClientData

resends any changed player HUD info to the client.
Called every frame by PlayerPreThink
Also called at start of demo recording and playback by
ForceClientDllUpdate to ensure the demo gets messages
reflecting all of the HUD state info.
=========================================================
*/
void CBasePlayer::UpdateClientData()
{
	if (m_ResetHUD != CBasePlayer::ResetHUD::No)
	{
		if (m_ResetHUD == CBasePlayer::ResetHUD::Initialize)
		{
			MessageBegin(MSG_ONE, gmsgInitHUD, this);
			MessageEnd();

			g_pGameRules->InitHUD(this);
		}

		MessageBegin(MSG_ONE, gmsgResetHUD, this);
		MessageEnd();

		m_ResetHUD = CBasePlayer::ResetHUD::No;
	}

	if (m_WeaponBits != m_ClientWeaponBits)
	{
		m_ClientWeaponBits = m_WeaponBits;

		const int lowerBits = m_WeaponBits & 0xFFFFFFFF;
		const int upperBits = (m_WeaponBits >> 32) & 0xFFFFFFFF;

		MessageBegin(MSG_ONE, gmsgWeapons, this);
		WriteLong(lowerBits);
		WriteLong(upperBits);
		MessageEnd();
	}

	if (0 != v.dmg_take || 0 != v.dmg_save || m_bitsHUDDamage != m_bitsDamageType)
	{
		// Comes from inside me if not set
		Vector damageOrigin = v.origin;
		// send "damage" message
		// causes screen to flash, and pain compass to show direction of damage
		Entity* other = v.dmg_inflictor;
		if (other != nullptr)
		{
			CBaseEntity* pEntity = other->Get<CBaseEntity>();
			if (pEntity != nullptr)
			{
				damageOrigin = pEntity->Center();
			}

			v.dmg_inflictor = nullptr;
		}

		MessageBegin(MSG_ONE, gmsgDamage, this);
		WriteByte(std::clamp(static_cast<int>(v.dmg_save), 0, 255));
		WriteByte(std::clamp(static_cast<int>(v.dmg_take), 0, 255));
		WriteLong(m_bitsDamageType);
		WriteCoord(damageOrigin);
		MessageEnd();

		v.dmg_take = 0;
		v.dmg_save = 0;
		m_bitsHUDDamage = m_bitsDamageType;
	}

#ifdef HALFLIFE_TRAINCONTROL
	if ((m_iTrain & TRAIN_NEW) != 0)
	{
		// send "health" update message
		MessageBegin(MSG_ONE, gmsgTrain, this);
		WriteByte(m_iTrain & 0xF);
		MessageEnd();

		m_iTrain &= ~TRAIN_NEW;
	}
#endif

	// Send new room type to client.
	if (m_ClientSndRoomtype != m_SndRoomtype)
	{
		m_ClientSndRoomtype = m_SndRoomtype;

		MessageBegin(MSG_ONE, SVC_ROOMTYPE, this);
		WriteShort((short)m_SndRoomtype); // sequence number
		MessageEnd();
	}
}


void CBasePlayer::EnableControl(bool fControl)
{
	if (!fControl)
		v.flags |= FL_FROZEN;
	else
		v.flags &= ~FL_FROZEN;
}


//=========================================================
// Autoaim
// set crosshair position to point to enemey
//=========================================================
Vector CBasePlayer::GetAimVector()
{
	util::MakeVectors(v.v_angle + v.punchangle);
	return gpGlobals->v_forward;
}


/*
=============
SetCustomDecalFrames

  Note:  -1 means no custom frames present.
=============
*/
void CBasePlayer::SetCustomDecalFrames(int nFrames)
{
	if (nFrames > 0 &&
		nFrames < 8)
		m_nCustomSprayFrames = nFrames;
	else
		m_nCustomSprayFrames = -1;
}

/*
=============
GetCustomDecalFrames

  Returns the # of custom frames this player's custom clan logo contains.
=============
*/
int CBasePlayer::GetCustomDecalFrames()
{
	return m_nCustomSprayFrames;
}


//=========================================================
// DropPlayerWeapon - drop the named weapon, or if no name,
// the active weapon.
//=========================================================
void CBasePlayer::DropPlayerWeapon(char* pszWeaponName)
{
	if (!util::IsMultiplayer() || (weaponstay.value > 0))
	{
		// no dropping in single player.
		return;
	}

	if (g_pGameRules->DeadPlayerWeapons(this) == GR_PLR_DROP_GUN_NO)
	{
		return;
	}

	CBasePlayerWeapon *pWeapon = nullptr;

	if (pszWeaponName != nullptr && pszWeaponName[0] != '\0')
	{
		for (auto it : m_lpPlayerWeapons)
		{
			if (streq(it->v.classname, pszWeaponName))
			{
				pWeapon = it;
				break;
			}
		}
	}
	else
	{
		pWeapon = m_pActiveWeapon;
	}

	if (pWeapon == nullptr || !pWeapon->CanHolster())
	{
		return;
	}

	util::MakeVectors(v.angles);

	CWeaponBox* pWeaponBox = (CWeaponBox*)CBaseEntity::Create("weaponbox", v.origin + gpGlobals->v_forward * 10, v.angles, v);
	pWeaponBox->SetModel(pWeapon->v.model);
	pWeaponBox->v.angles.x = 0;
	pWeaponBox->v.angles.z = 0;

	CTFWeapon* pSibling = dynamic_cast<CTFWeapon*>(pWeapon)->GetSibling();
	if (pSibling != nullptr)
	{
		pWeaponBox->PackWeapon(pSibling);
	}

	pWeaponBox->PackWeapon(pWeapon);
	pWeaponBox->v.velocity = gpGlobals->v_forward * 300 + gpGlobals->v_forward * 100;

	// drop half of the ammo for this weapon.
	int iAmmoIndex = pWeapon->iAmmo1();

	if (iAmmoIndex != -1)
	{
		// this weapon weapon uses ammo, so pack an appropriate amount.
		if ((pWeapon->iFlags() & WEAPON_FLAG_EXHAUSTIBLE) != 0)
		{
			// pack up all the ammo, this weapon is its own ammo type
			pWeaponBox->PackAmmo(pWeapon->iAmmo1(), m_rgAmmo[iAmmoIndex]);
			m_rgAmmo[iAmmoIndex] = 0;
		}
		else
		{
			// pack half of the ammo
			pWeaponBox->PackAmmo(pWeapon->iAmmo1(), m_rgAmmo[iAmmoIndex] / 2);
			m_rgAmmo[iAmmoIndex] /= 2;
		}
	}
}


void CBasePlayer::SetPrefsFromUserinfo(char* infobuffer)
{
	const char* value = engine::InfoKeyValue(infobuffer, "cl_autowepswitch");

	if ('\0' != *value)
	{
		m_iAutoWepSwitch = atoi(value);
	}
	else
	{
		m_iAutoWepSwitch = 1;
	}

	value = engine::InfoKeyValue(infobuffer, "cl_grenadetoggle");

	if ('\0' != *value)
	{
		m_bGrenadeToggle = atoi(value) != 0;
	}
	else
	{
		m_bGrenadeToggle = false;
	}
}

void CBasePlayer::GetEntityState(entity_state_t& state, CBasePlayer* player)
{
	CBaseEntity::GetEntityState(state, player);

	state.team = v.team;
	state.playerclass = v.playerclass;

	state.basevelocity = v.basevelocity;

	state.weaponmodel = v.weaponmodel;
	state.gaitsequence = v.gaitsequence;
	state.spectator = (v.flags & FL_SPECTATOR) != 0;
	state.friction = v.friction;

	state.gravity = v.gravity;

	state.usehull = (v.flags & FL_DUCKING) != 0 ? 1 : 0;

	if (IsSpectator() || IsAlive())
	{
		state.health = std::max(v.health, 1.0F);
	}
	else
	{
		state.health = 0.0F;
	}

	state.fuser1 = v.armorvalue;

	if (InState(State::Burning))
	{
		state.eflags |= EFLAG_BURNING;
	}

	/* Power-up effect flags. */

	for (int i = 0; i < kBioSuit; i++)
	{
		if ((m_afPowerUpActive & (1 << i)) != 0)
		{
			state.eflags |= EFLAG_SUPER_DAMAGE << i;
		}
	}

	/* Keep allies slightly visible. */

	if (v.rendermode != kRenderNormal && v.renderamt < 31
	 && g_pGameRules->CanSeeThroughDisguise(player, this))
	{
		state.renderamt = std::max (v.rendermode, 31);
	}

	/* Check for spy disguise. */

	state.aiment = 0;

	if (InState(State::Disguised) && !g_pGameRules->CanSeeThroughDisguise(player, this))
	{
		if (m_iDisguiseIndex != 0)
		{
			state.aiment = m_iDisguiseIndex;
		}

		if (m_iDisguiseTeam != TEAM_UNASSIGNED)
		{
			state.team = m_iDisguiseTeam;
		}

		if (m_iDisguisePlayerClass != PC_UNDEFINED)
		{
			state.playerclass = m_iDisguisePlayerClass;
		}

		if (m_flDisguiseHealth > 0.0F)
		{
			state.health = std::max(m_flDisguiseHealth, 1.0F);
		}

		if (m_flDisguiseArmor > 0.0F)
		{
			state.fuser1 = m_flDisguiseArmor;
		}

		if (m_iDisguiseWeaponModel != 0)
		{
			state.weaponmodel = m_iDisguiseWeaponModel;
		}

		if (m_iDisguiseSequence != 0)
		{
			state.sequence = m_iDisguiseSequence;
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
		return;
	}

	if (m_rgAmmo[AMMO_GRENADES1 + grenadeSlot] == 0)
	{
		return;
	}

	m_iGrenadeExplodeTime = 0;

	const auto &info = sTFClassInfo[PCNumber()];
	const auto grenadeType = info.grenades[grenadeSlot];

	auto throwGrenade = false;

	switch (grenadeType)
	{
		case GRENADE_NORMAL:
			m_hGrenade = CPrimeGrenade::PrimeGrenade(this);
			break;
		case GRENADE_CALTROP:
			m_hGrenade = CCaltropCanister::CaltropCanister(this);
			throwGrenade = true;
			goto no_icon;
		case GRENADE_CONCUSSION:
			m_hGrenade = CConcussionGrenade::ConcussionGrenade(this);
			m_iGrenadeExplodeTime = 3800;
			break;
		case GRENADE_NAIL:
			m_hGrenade = CNailGrenade::NailGrenade(this);
			break;
		case GRENADE_MIRV:
			m_hGrenade = CMirv::Mirv(this);
			break;
		case GRENADE_NAPALM:
			m_hGrenade = CNapalmGrenade::NapalmGrenade(this);
			break;
		case GRENADE_EMP:
			m_hGrenade = CEMP::EMP(this);
			break;
		case GRENADE_FLASH:
			m_hGrenade = CFlashGrenade::FlashGrenade(this);
			throwGrenade = true;
			goto no_icon;
		default:
			return;
	}

	{
		auto icon = GetGrenadeIconName(grenadeType);

		if (icon != nullptr)
		{
			MessageBegin(MSG_ONE, gmsgStatusIcon, this);
			WriteByte(2);
			WriteString(icon);
			MessageEnd();
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
	LeaveState(State::GrenadePrime);
	EnterState(State::GrenadeThrowing);
	m_iGrenadeExplodeTime = 0;
	m_hGrenade = nullptr;
}

#endif


void CBasePlayer::ConcussionJump(Vector& velocity)
{
	velocity.x *= 2.36F;
	velocity.y *= 2.36F;
	velocity.z *= 3.53F;

	if ((v.flags & FL_ONGROUND) != 0)
	{
		BecomeConcussed(this);
	}

	CBaseEntity* grenade = m_hGrenade;
	if (grenade != nullptr)
	{
		grenade->Remove();
	}
	m_hGrenade = nullptr;

	MessageBegin(MSG_ONE, gmsgStatusIcon, this);
	WriteByte(0);
	WriteString("d_concussiongrenade");
	MessageEnd();

	LeaveState(CBasePlayer::State::Grenade);
}


void CBasePlayer::BecomeInfected(CBaseEntity* infector)
{
	if (PCNumber() == PC_MEDIC)
	{
		return;
	}

	EnterState(State::Infected);
	m_hInfector = infector;
	m_flNextInfectionTime = gpGlobals->time + 1.0F;

	MessageBegin(MSG_ONE, gmsgStatusIcon, this);
	WriteByte(2);
	WriteString("dmg_poison");
	MessageEnd();
}


bool CBasePlayer::CanBurn()
{
	if ((m_afArmorClass & AT_SAVEFIRE) != 0)
	{
		return false;
	}

	if (PCNumber() == PC_PYRO && v.armorvalue > 0.0F)
	{
		return false;
	}

	if (v.waterlevel >= kWaterLevelWaist)
	{
		return false;
	}

	return true;
}


void CBasePlayer::Ignite(CBaseEntity* burner)
{
	if (!CanBurn())
	{
		return;
	}

	if (!InState(State::Burning))
	{
		/* Reset all of the burn sources. */
		for (auto i = 0; i < kMaxBurnSources; i++)
		{
			m_nBurnCount[i] = 0;
		}
		m_nBurnSource = 0;
	}

	/*
		Toodles: This used to deal 6 damage. The extra damage
		was added into to each respective ignition source.
	*/

	EnterState(State::Burning);

	m_flNextBurnTime = gpGlobals->time + 0.3F;

	/* Credit goes to the new burner. */
	m_hBurner = burner;

	/* Reset the oldest burn source timer. */
	m_nBurnCount[m_nBurnSource] = 14;
	/* Cycle to the next burn source. */
	m_nBurnSource = (m_nBurnSource + 1) % kMaxBurnSources;

	MessageBegin(MSG_ONE, gmsgStatusIcon, this);
	WriteByte(2);
	WriteString("dmg_heat");
	MessageEnd();
}

void CBasePlayer::Extinguish()
{
	LeaveState(State::Burning);

	MessageBegin(MSG_ONE, gmsgStatusIcon, this);
	WriteByte(0);
	WriteString("dmg_heat");
	MessageEnd();
}


void CBasePlayer::SaveMe()
{
	if (!IsAlive() || !IsPlayer())
	{
		return;
	}

	if (m_flNextSpeakTime > gpGlobals->time)
	{
		return;
	}

	char* sample = "speech/saveme1.wav";

	if (engine::RandomLong(0, 4) == 0)
	{
		sample = "speech/saveme2.wav";
	}
	else if (engine::RandomLong(0, 99) == 0)
	{
		sample = "speech/saveme3.wav";
	}

	EmitSound(sample, CHAN_VOICE, VOL_NORM, ATTN_NORM, GetVoicePitch());

	int index = v.GetIndex();
	
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* player = util::PlayerByIndex(i);

		if (player != nullptr
		 && player != this
		 && player->IsAlive()
		 && player->PCNumber() == PC_MEDIC
		 && g_pGameRules->PlayerRelationship(player, this) >= GR_ALLY)
		{
			MessageBegin(MSG_ONE, SVC_TEMPENTITY, player);
			WriteByte(TE_PLAYERATTACHMENT);
			WriteByte(index);
			WriteCoordComponent(50);
			WriteShort(g_sModelIndexSaveMe);
			WriteShort(40);
			MessageEnd();
		}
	}

	m_flNextSpeakTime = gpGlobals->time + 4;
}


void CBasePlayer::SendExtraInfo(CBaseEntity* toWhom)
{
	if (toWhom != nullptr)
	{
		MessageBegin(MSG_ONE, gmsgExtraInfo, toWhom);
	}
	else
	{
		MessageBegin(MSG_ALL, gmsgExtraInfo);
	}

	WriteByte(v.GetIndex());

	byte role = 0;
	role |= PCNumber();
	role |= TeamNumber() << 5;
	WriteByte(role);

	byte flags = 0;
	if (IsPlayer() && !IsAlive())
	{
		flags |= 1;
	}
	if (m_bLeftHanded)
	{
		flags |= 2;
	}
	if (IsBot())
	{
		flags |= 4;
	}
	WriteByte(flags);

	MessageEnd();
}


void CBasePlayer::RemoveGoalItems(bool force)
{
	CTFGoalItem* goal = nullptr;
	while ((goal = (CTFGoalItem*)util::FindEntityByClassname(goal, "item_tfgoal")))
	{
		if (goal->v.owner == &v && (force || goal->IsGoalActivatedBy(TFGI_CANBEDROPPED)))
		{
			goal->RemoveFromPlayer(this, GI_DROP_PLAYERDROP);
		}
	}
}


float CBasePlayer::GiveArmor(float type, float amount)
{
	/* Don't pickup if this armor isn't as good as the stuff we've got. */

	if (type == v.armortype)
	{
		if (v.armorvalue >= m_flArmorMax)
		{
			return 0.0F;
		}

		const auto oldvalue = v.armorvalue;

		v.armorvalue = std::clamp(v.armorvalue + amount, 0.0F, m_flArmorMax);

		return v.armorvalue - oldvalue;
	}
	else
	{
		/* Upgrade our armor if a type is not specified. */
		type = (type > 0.0F) ? std::min(type, m_flArmorTypeMax) : m_flArmorTypeMax;

		const auto newamount = std::clamp(amount, 0.0F, m_flArmorMax);

		if (v.armortype * v.armorvalue >= type * newamount)
		{
			return 0.0F;
		}

		v.armorvalue = newamount;
		v.armortype = type;

		return newamount;
	}
}


void CBasePlayer::EmitSoundHUD(
	const char* sample,
	int channel,
	float volume,
	float attenuation,
	int pitch,
	int flags)
{
	if (!IsNetClient())
	{
		return;
	}

	volume *= 255;
	attenuation *= 127;

	MessageBegin(MSG_ONE, gmsgPredictedSound, this);
	WriteByte(v.GetIndex());
	WriteByte(channel);
	WriteByte(volume);
	WriteByte(attenuation);
	WriteByte(pitch);
	WriteString(sample);
	MessageEnd();
}


void CBasePlayer::FinishDisguising()
{
	/* Toodles TODO: Four team. */
	m_iDisguiseTeam = (TeamNumber () == TEAM_BLUE) != m_bNextDisguiseAlly ? TEAM_RED : TEAM_BLUE;

	m_iDisguisePlayerClass = m_iNextDisguisePlayerClass;

	enum
	{
		kDisguiseAnyPlayer,
		kDisguiseAnyTeam,
		kDisguiseAnyClass,
		kDisguiseAsRequested,
	};

	int state = kDisguiseAnyPlayer;

	byte candidates[MAX_PLAYERS];
	int numCandidates = 0;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		auto other = static_cast<CBasePlayer*>(util::PlayerByIndex(i));

		if (other == nullptr)
		{
			continue;
		}

		if ((g_pGameRules->PlayerRelationship(this, other) >= GR_ALLY)
		 != (m_iDisguiseTeam == TeamNumber())) /* Toodles FIXME: */
		{
			if (state > kDisguiseAnyTeam)
			{
				continue;
			}
		}
		else if (state <= kDisguiseAnyTeam)
		{
			state = kDisguiseAnyTeam + 1;
			numCandidates = 0;
		}

		if (other->PCNumber () != m_iDisguisePlayerClass)
		{
			if (state > kDisguiseAnyClass)
			{
				continue;
			}
		}
		else if (state <= kDisguiseAnyClass)
		{
			state = kDisguiseAnyClass + 1;
			numCandidates = 0;
		}

		candidates[numCandidates] = i;
		numCandidates++;
	}

	/* This shouldn't happen but, I don't trust this game engine. */
	if (numCandidates == 0)
	{
		return;
	}

	m_iDisguiseIndex = candidates[engine::RandomLong (1, numCandidates) - 1];

	auto disguise = static_cast<CBasePlayer*>(util::PlayerByIndex (m_iDisguiseIndex));

	if (disguise != nullptr)
	{
		m_flDisguiseHealth = disguise->v.health;
		m_flDisguiseArmor = disguise->v.armorvalue;

		util::ClientPrint(this, HUD_PRINTCENTER, "#Disguise_player",
			STRING(disguise->v.netname));
	}
	else
	{
		m_flDisguiseHealth = 0.0F;
		m_flDisguiseArmor = 0.0F;
	}

	if (m_flDisguiseHealth <= 0.0F)
	{
		m_flDisguiseHealth = sTFClassInfo[m_iDisguisePlayerClass].maxHealth
			* engine::RandomFloat (0.5F, 1.0F);
	}

	if (m_flDisguiseArmor <= 0.0F)
	{
		m_flDisguiseArmor = sTFClassInfo[m_iDisguisePlayerClass].initArmor
			* engine::RandomFloat (0.5F, 1.0F);
	}

	/*
		Toodles FIXME: Get disguise weapons from player class info.
		Toodles TODO: Allow spies to choose their disguise's held weapon.
	*/

	switch (m_iDisguisePlayerClass)
	{
		case PC_SCOUT:
		{
			m_iDisguiseWeaponModel = engine::ModelIndex ("models/p_nailgun.mdl");
			strcpy (m_szDisguiseAnimExtention, "mp5");
			break;
		}
		case PC_SNIPER:
		{
			m_iDisguiseWeaponModel = engine::ModelIndex ("models/p_sniper.mdl");
			strcpy (m_szDisguiseAnimExtention, "sniper");
			break;
		}
		case PC_SOLDIER:
		{
			m_iDisguiseWeaponModel = engine::ModelIndex ("models/p_rpg.mdl");
			strcpy (m_szDisguiseAnimExtention, "rpg");
			break;
		}
		case PC_DEMOMAN:
		{
			m_iDisguiseWeaponModel = engine::ModelIndex ("models/p_glauncher.mdl");
			strcpy (m_szDisguiseAnimExtention, "shotgun");
			break;
		}
		case PC_MEDIC:
		{
			m_iDisguiseWeaponModel = engine::ModelIndex ("models/p_medkit.mdl");
			strcpy (m_szDisguiseAnimExtention, "medkit");
			break;
		}
		case PC_HVYWEAP:
		{
			m_iDisguiseWeaponModel = engine::ModelIndex ("models/p_mini.mdl");
			strcpy (m_szDisguiseAnimExtention, "AC");
			break;
		}
		case PC_PYRO:
		{
			m_iDisguiseWeaponModel = engine::ModelIndex ("models/p_egon.mdl");
			strcpy (m_szDisguiseAnimExtention, "egon");
			break;
		}
		case PC_SPY:
		{
			m_iDisguiseWeaponModel = engine::ModelIndex ("models/p_shotgun.mdl");
			strcpy (m_szDisguiseAnimExtention, "shotgun");
			break;
		}
		case PC_ENGINEER:
		{
			m_iDisguiseWeaponModel = engine::ModelIndex ("models/p_spanner.mdl");
			strcpy (m_szDisguiseAnimExtention, "crowbar");
			break;
		}
		default:
		{
			m_iDisguiseWeaponModel = 0;
			m_szDisguiseAnimExtention[0] = '\0';
			break;
		}
	}

	EnterState(State::Disguised);

	SetAction(Action::Idle, true);
}


void CBasePlayer::BecomeFlashed(CBaseEntity* attacker, CBaseEntity* inflictor)
{
	const auto delta = inflictor->v.origin - EyePosition();
	const auto distance = delta.Length();

	/* Too far away. */

	if (distance >= kFlashMaxDistance)
	{
		return;
	}

	/* Don't scale at all if we're right on top of it. */

	auto full = distance <= kFlashMinDistance && attacker != this;

	/* Scale with distance. */

	auto time = 1.0F - std::max(distance - kFlashMinDistance, 0.0F)
		/ (kFlashMaxDistance - kFlashMinDistance);

	if (!full)
	{
		util::MakeVectors(v.v_angle);

		/* Scale with view angle difference, as well. */

		const auto dot = DotProduct(delta / distance, gpGlobals->v_forward);

		/* Outside of the normal player view frustum. */

		if (dot <= 0.5F)
		{
			return;
		}

		if (dot < 0.9F)
		{
			time *= 1.0F + (dot - 0.9F) * 2.5F;
		}
	}

	MessageBegin(MSG_ONE, gmsgFlash, this);
	WriteByte(full);
	WriteByte(255 * time);
	MessageEnd();
}


bool CBasePlayer::SpannerHit(CBaseEntity* other)
{
	if (g_pGameRules->PlayerRelationship(this, other) < GR_ALLY)
	{
		return false;
	}

	/* Convert 1 cell into 4 points of armor. */

	const auto player = static_cast<CBasePlayer*>(other);

	auto armor = std::min(
		(int)m_flArmorMax - (int)v.armorvalue,
		player->m_rgAmmo[AMMO_CELLS] * 4);

	if (armor <= 0)
	{
		return false;
	}

	/* Toodles: Prevent damage tanking during combat. */

	const auto megaHealth =
		m_flMegaHealthTime <= gpGlobals->time;

	auto health = 4;

	if (megaHealth)
	{
		health *= 10;
	}

	armor = std::min(armor, health);

	if (GiveArmor(v.armortype, armor) != 0.0F)
	{
#if 0
		EmitSound("items/empathy2.wav", CHAN_ITEM);
#endif

		player->m_rgAmmo[AMMO_CELLS] -= (int)std::ceil(armor / 4.0F);

		return true;
	}

	return false;
}


bool CBasePlayer::ElectromagneticPulse(CBaseEntity* attacker, CBaseEntity* inflictor)
{
	if (!g_pGameRules->FPlayerCanTakeDamage(this, attacker, inflictor))
	{
		return false;
	}

	float detonateAmmo[AMMO_SECONDARY];

	auto damage = 0.0F;

	for (int i = 0; i < AMMO_CELLS; i++)
	{
		detonateAmmo[i] = m_rgAmmo[i] * kAmmoEMPStrength[i];

		damage += detonateAmmo[i];
	}

	/* Engineers are immune to cell destruction, I guess. */

	if (PCNumber() != PC_ENGINEER)
	{
		detonateAmmo[AMMO_CELLS] = m_rgAmmo[AMMO_CELLS]
			* kAmmoEMPStrength[AMMO_CELLS];

		damage += detonateAmmo[AMMO_CELLS];
	}
	else
	{
		detonateAmmo[AMMO_CELLS] = 0.0F;
	}

	for (int i = 0; i < AMMO_SECONDARY; i++)
	{
		if (detonateAmmo[i] < m_rgAmmo[i])
			m_rgAmmo[i] -= static_cast<int>(detonateAmmo[i]);
		else
			m_rgAmmo[i] = 0;
	}

	if (damage < 1.0F)
	{
		return false;
	}

	tent::Explosion(v.origin, Vector(0.0F, 0.0F, -1.0F),
		tent::ExplosionType::Normal, damage);

	/* Toodles TODO: Have us take the damage directly. */

	RadiusDamage(v.origin, inflictor, attacker,
		damage, damage * 0.5F, damage * 2.0F, DMG_BLAST | DMG_NO_KNOCKBACK);

	return true;
}


void CBasePlayer::RemoveAllBuildings(const bool detonate)
{
	/* Toodles TODO: Detonate mode. */

#ifndef NDEBUG
	auto count = 0;
#endif

	for (int i = BUILD_DISPENSER; i <= BUILD_EXIT_TELEPORTER; i++)
	{
		CBaseEntity* building = m_hBuildings[i - 1];

		if (building != nullptr)
		{
			building->Remove();

#ifndef NDEBUG
			count++;
#endif
		}
	}

#ifndef NDEBUG
	if (count != 0)
	{
		engine::AlertMessage(
			at_aiconsole,
			"Removed %i buildings owned by '%s'\n",
			count,
			STRING(v.netname));
	}
#endif
}


class CStripWeapons : public CPointEntity
{
public:
	CStripWeapons(Entity* containingEntity) : CPointEntity(containingEntity) {}

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
};

LINK_ENTITY_TO_CLASS(player_weaponstrip, CStripWeapons);

void CStripWeapons::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	CBasePlayer* pPlayer = nullptr;

	if (pActivator && pActivator->IsPlayer())
	{
		pPlayer = (CBasePlayer*)pActivator;
	}
	else if (!util::IsDeathmatch())
	{
		pPlayer = (CBasePlayer*)util::GetLocalPlayer();
	}

	if (pPlayer)
		pPlayer->RemoveAllWeapons();
}

