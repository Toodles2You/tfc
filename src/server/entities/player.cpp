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


void CBasePlayer::Pain()
{
	float flRndSound; //sound randomizer

	flRndSound = RANDOM_FLOAT(0, 1);

	if (flRndSound <= 0.33)
		EmitSound("player/pl_pain5.wav", CHAN_VOICE);
	else if (flRndSound <= 0.66)
		EmitSound("player/pl_pain6.wav", CHAN_VOICE);
	else
		EmitSound("player/pl_pain7.wav", CHAN_VOICE);
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

void CBasePlayer::DeathSound()
{
	// water death sounds
	/*
	if (pev->waterlevel >= kWaterLevelEyes)
	{
		EmitSound("player/h2odeath.wav", CHAN_VOICE);
		return;
	}
	*/

	// temporarily using pain sounds for death sounds
	switch (RANDOM_LONG(1, 5))
	{
	case 1: EmitSound("player/pl_pain5.wav", CHAN_VOICE); break;
	case 2: EmitSound("player/pl_pain6.wav", CHAN_VOICE); break;
	case 3: EmitSound("player/pl_pain7.wav", CHAN_VOICE); break;
	}
}

// override takehealth
// bitsDamageType indicates type of damage healed.

bool CBasePlayer::GiveHealth(float flHealth, int bitsDamageType)
{
	if (pev->takedamage == DAMAGE_NO)
	{
		return false;
	}

	return CBaseAnimating::GiveHealth(flHealth, bitsDamageType);
}

//=========================================================
// TraceAttack
//=========================================================
void CBasePlayer::TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, int hitgroup, int bitsDamageType)
{
	if (pev->takedamage == DAMAGE_NO)
	{
		return;
	}

	m_LastHitGroup = hitgroup;

	if ((bitsDamageType & DMG_AIMED) != 0)
	{
		switch (hitgroup)
		{
		case HITGROUP_GENERIC:
			break;
		case HITGROUP_HEAD:
			flDamage *= 3;
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
			break;
		default:
			break;
		}
	}

	AddMultiDamage(flDamage, bitsDamageType);
}

static inline float DamageForce(entvars_t* pev, int damage)
{
	float size = (32 * 32 * 72) / (pev->size.x * pev->size.y * pev->size.z);
	return std::clamp(damage * size * 5.0F, 0.0F, 1000.0F);
}

static float ArmourBonus(float &damage, float armour, float ratio, float bonus)
{
	if (armour <= 0)
	{
		return 0.0f;
	}
	float newDamage = damage * ratio;
	float take = (damage - newDamage) * bonus;
	if (take > armour)
	{
		damage -= armour * (1.0f / bonus);
		return armour;
	}
	damage = newDamage;
	return take;
}

#define kArmourRatio 0.2f // Armor Takes 80% of the damage
#define kArmourBonus 0.5f // Each Point of Armor is work 1/x points of health

bool CBasePlayer::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	if (pev->takedamage == DAMAGE_NO || !IsAlive())
	{
		return false;
	}

	if (!g_pGameRules->FPlayerCanTakeDamage(this, attacker))
	{
		return false;
	}

	m_bitsDamageType |= bitsDamageType;
	m_bitsHUDDamage = -1;

	float flArmour = 0.0f;
	if ((bitsDamageType & DMG_ARMOR_PIERCING) == 0)
	{
		float armourBonus = kArmourBonus;
		if ((bitsDamageType & DMG_BLAST) != 0 && util::IsDeathmatch())
		{
			armourBonus *= 2;
		}
		flArmour = ArmourBonus(flDamage, pev->armorvalue, kArmourRatio, armourBonus);
	}

	// Grab the vector of the incoming attack. (Pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit.)
	if (attacker != CWorld::World
	 && attacker->pev->solid != SOLID_TRIGGER
	 && (bitsDamageType & DMG_ARMOR_PIERCING) == 0)
	{
		// Move them around!
		g_vecAttackDir = (inflictor->Center() - Vector(0, 0, 10) - Center()).Normalize();

		pev->velocity = pev->velocity + g_vecAttackDir * -DamageForce(pev, flDamage);
	}

	if ((bitsDamageType & DMG_ARMOR_PIERCING) == 0)
	{
		pev->dmg_inflictor = inflictor->edict();
	}
	else
	{
		pev->dmg_inflictor = nullptr;
	}

	// Check for godmode or invincibility.
	if ((pev->flags & FL_GODMODE) != 0)
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
	}

	// Do the damage!
	pev->dmg_take += flDamage;
	pev->health -= flDamage;

	pev->dmg_save += flArmour;
	pev->armorvalue -= flArmour;

	if (attacker->IsNetClient())
	{
		int flags = 0;

		if (pev->health <= 0)
		{
			flags |= kDamageFlagDead;
		}

		if ((bitsDamageType & DMG_AIMED) != 0
		 && m_LastHitGroup == HITGROUP_HEAD)
		{
			flags |= kDamageFlagHeadshot;
		}

		if (attacker == this)
		{
			flags |= kDamageFlagSelf;
		}

		MessageBegin(MSG_ONE, gmsgHitFeedback, attacker);
		WriteByte(entindex());
		WriteByte(flags);
		WriteShort(flDamage);
		MessageEnd();
	}

	if (pev->health <= 0)
	{
		Killed(inflictor, attacker, bitsDamageType);
		return false;
	}

	MessageBegin(MSG_SPEC, SVC_DIRECTOR);
		WriteByte(9);							  // command length in bytes
		WriteByte(DRC_CMD_EVENT);				  // take damage event
		WriteShort(entindex());	  // index number of primary entity
		WriteShort(inflictor->entindex()); // index number of secondary entity
		WriteLong(5);							  // eventflags (priority and flags)
	MessageEnd();

	return true;
}

//=========================================================
// PackDeadPlayerWeapons - call this when a player dies to
// pack up the appropriate weapons and ammo items, and to
// destroy anything that shouldn't be packed.
//
// This is pretty brute force :(
//=========================================================
void CBasePlayer::PackDeadPlayerWeapons()
{
	int iWeaponRules;
	int iAmmoRules;
	int i;
	CBasePlayerWeapon* rgpPackWeapons[WEAPON_TYPES];
	int iPackAmmo[AMMO_TYPES];
	int iPW = 0; // index into packweapons array
	int iPA = 0; // index into packammo array

	memset(rgpPackWeapons, 0, sizeof(rgpPackWeapons));
	for (i = 0; i < AMMO_TYPES; i++)
	{
		iPackAmmo[i] = -1;
	}

	// get the game rules
	iWeaponRules = g_pGameRules->DeadPlayerWeapons(this);
	iAmmoRules = g_pGameRules->DeadPlayerAmmo(this);

	if (iWeaponRules == GR_PLR_DROP_GUN_NO && iAmmoRules == GR_PLR_DROP_AMMO_NO)
	{
		// nothing to pack. Remove the weapons and return. Don't call create on the box!
		RemoveAllWeapons();
		return;
	}

	// go through all of the weapons and make a list of the ones to pack
	for (auto pWeapon : m_lpPlayerWeapons)
	{
		switch (iWeaponRules)
		{
		case GR_PLR_DROP_GUN_ACTIVE:
			if (m_pActiveWeapon && pWeapon == m_pActiveWeapon)
			{
				// this is the active weapon. Pack it.
				rgpPackWeapons[iPW++] = pWeapon;
			}
			break;

		case GR_PLR_DROP_GUN_ALL:
			rgpPackWeapons[iPW++] = pWeapon;
			break;

		default:
			break;
		}
	}

	// now go through ammo and make a list of which types to pack.
	if (iAmmoRules != GR_PLR_DROP_AMMO_NO)
	{
		for (i = 0; i < AMMO_TYPES; i++)
		{
			if (m_rgAmmo[i] != 0)
			{
				// player has some ammo of this type.
				switch (iAmmoRules)
				{
				case GR_PLR_DROP_AMMO_ALL:
					iPackAmmo[iPA++] = i;
					break;

				case GR_PLR_DROP_AMMO_ACTIVE:
					if (m_pActiveWeapon && i == m_pActiveWeapon->iAmmo1())
					{
						// this is the primary ammo type for the active weapon
						iPackAmmo[iPA++] = i;
					}
					else if (m_pActiveWeapon && i == m_pActiveWeapon->iAmmo2())
					{
						// this is the secondary ammo type for the active weapon
						iPackAmmo[iPA++] = i;
					}
					break;

				default:
					break;
				}
			}
		}
	}

	// create a box to pack the stuff into.
	CWeaponBox* pWeaponBox = (CWeaponBox*)CBaseEntity::Create("weaponbox", pev->origin, pev->angles, edict());

	if (iWeaponRules == GR_PLR_DROP_GUN_ACTIVE && m_pActiveWeapon != nullptr)
	{
		pWeaponBox->SetModel(STRING(m_pActiveWeapon->pev->model));
	}

	pWeaponBox->pev->angles.x = 0; // don't let weaponbox tilt.
	pWeaponBox->pev->angles.z = 0;

	pWeaponBox->SetThink(&CWeaponBox::Remove);
	pWeaponBox->pev->nextthink = gpGlobals->time + 120;

	// back these two lists up to their first elements

	// pack the ammo
	for (iPA = 0; iPA < AMMO_TYPES && iPackAmmo[iPA] != -1; iPA++)
	{
		pWeaponBox->PackAmmo(iPackAmmo[iPA], m_rgAmmo[iPackAmmo[iPA]]);
	}

	// now pack all of the weapons in the lists
	for (iPW = 0; iPW < WEAPON_TYPES && rgpPackWeapons[iPW]; iPW++)
	{
		// weapon unhooked from the player. Pack it into der box.
		pWeaponBox->PackWeapon(rgpPackWeapons[iPW]);
	}

	pWeaponBox->pev->velocity = pev->velocity * 1.2; // weaponbox has player's velocity, then some.

	RemoveAllWeapons(); // now strip off everything that wasn't handled by the code above.
}

void CBasePlayer::RemoveAllWeapons()
{
#ifdef HALFLIFE_TANKCONTROL
	if (m_pTank != NULL)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}
#endif

	for (int i = 0; i < WEAPON_TYPES; i++)
	{
		if (m_rgpPlayerWeapons[i] != nullptr)
		{
			m_rgpPlayerWeapons[i]->Remove();
		}
	}

	memset(m_rgAmmo, 0, sizeof(m_rgAmmo));
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

#ifdef HALFLIFE_TANKCONTROL
	if (m_pTank != NULL)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}
#endif

	SetUseObject(nullptr);

	SetAction(Action::Die);

	m_fDeadTime = gpGlobals->time;
	m_fNextSuicideTime = gpGlobals->time + 1.0f;

	pev->health = std::min(pev->health, 0.0F);
	pev->deadflag = DEAD_DYING;

	m_iFOV = 0;

	m_StateBits = 0;

	m_iObserverLastMode = OBS_CHASE_FREE;
	pev->iuser1 = OBS_DEATHCAM;
	pev->iuser2 = entindex();
	pev->iuser3 = attacker->entindex();

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW | EF_NOINTERP;

	DeathSound();

	auto gibMode = GIB_NORMAL;
	if ((bitsDamageType & DMG_ALWAYSGIB) != 0)
	{
		gibMode = GIB_ALWAYS;
	}
	else if ((bitsDamageType & DMG_NEVERGIB) != 0)
	{
		gibMode = GIB_NEVER;
	}

	tent::SpawnCorpse(this, gibMode);
}


void CBasePlayer::PlayerDeathFrame()
{
	const auto bIsMultiplayer = util::IsMultiplayer();

	if (HasWeapons())
	{
		PackDeadPlayerWeapons();
	}

	// If the player has been dead for 5 seconds,
	// send the player off to an intermission
	// camera until they respawn.
	if (bIsMultiplayer
	 && (!IsObserver() || pev->iuser1 == OBS_DEATHCAM)
	 && (m_fDeadTime + 5.0f) <= gpGlobals->time)
	{
		StartObserver();
	}

	if (pev->deadflag != DEAD_RESPAWNABLE)
	{
		if (!g_pGameRules->FPlayerCanRespawn(this))
		{
			return;
		}
		pev->deadflag = DEAD_RESPAWNABLE;
	}

	if (bIsMultiplayer)
	{
		if ((int)forcerespawn.value != 0
		 || m_afButtonPressed != 0
		 || IsBot())
		{
			m_afButtonPressed = 0;
			pev->button = 0;
			m_afButtonReleased = 0;
			Spawn();
		}
	}
	else
	{
		if (m_afButtonPressed != 0)
		{
			SERVER_COMMAND("reload\n");
		}
	}
}

void CBasePlayer::StartObserver()
{
	// clear any clientside entities attached to this player
	MessageBegin(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WriteByte(TE_KILLPLAYERATTACHMENTS);
	WriteByte((byte)entindex());
	MessageEnd();

	// Holster weapon immediately, to allow it to cleanup
	if (m_pActiveWeapon)
		m_pActiveWeapon->Holster();

#ifdef HALFLIFE_TANKCONTROL
	if (m_pTank != NULL)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}
#endif

	SetUseObject(nullptr);

	// Remove all the player's stuff
	if (HasWeapons())
	{
		PackDeadPlayerWeapons();
	}

	// reset FOV
	m_iFOV = 0;

	// Setup flags
	pev->effects = EF_NOINTERP | EF_NODRAW;
	pev->view_ofs = g_vecZero;
	pev->fixangle = 1;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	pev->flags &= ~FL_DUCKING;
	pev->deadflag = DEAD_RESPAWNABLE;
	pev->health = 1;

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
		if ((pev->button & IN_USE) != 0
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
	if ((pev->flags & FL_ONTRAIN) != 0)
	{
		pev->flags &= ~FL_ONTRAIN;
		m_iTrain = TRAIN_NEW | TRAIN_OFF;
		return;
	}
	
	// Start controlling the train!
	CBaseEntity* pTrain = CBaseEntity::Instance(pev->groundentity);

	if (pTrain != nullptr
	 && (pev->button & IN_JUMP) == 0
	 && (pev->flags & FL_ONGROUND) != 0
	 && (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) != 0
	 && pTrain->OnControls(pev))
	{
		pev->flags |= FL_ONTRAIN;
		m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
		m_iTrain |= TRAIN_NEW;
		EmitSound("plats/train_use1.wav", CHAN_VOICE);
		return;
	}
#endif

	CBaseEntity* current = nullptr;
	CBaseEntity* closest = nullptr;
	auto maxDot = VIEW_FIELD_NARROW;

	util::MakeVectors(pev->v_angle); // so we know which way we are facing

	while ((current = util::FindEntityInSphere(current, pev->origin, PLAYER_SEARCH_RADIUS)) != nullptr)
	{
		if ((current->ObjectCaps() & (FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE)) == 0)
		{
			continue;
		}

		// This essentially moves the origin of the target to the corner nearest the player to test to see
		// if it's "hull" is in the view cone
		const auto eyeDelta = current->Center() - EyePosition();
		const auto los = util::ClampVectorToBox(eyeDelta, current->pev->size / 2.0F);

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

	const auto distance = (object->pev->origin - pev->origin).Length();

	if (distance > maxDistance)
	{
		return false;
	}

	const auto eyeDelta = object->Center() - EyePosition();
	const auto los = util::ClampVectorToBox(eyeDelta, object->pev->size / 2.0F);

	util::MakeVectors(pev->v_angle);

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


void CBasePlayer::PreThink()
{
	int buttonsChanged = (m_afButtonLast ^ pev->button); // These buttons have changed this frame

	// Debounced button codes for pressed/released
	m_afButtonPressed = buttonsChanged & pev->button;	  // The changed ones still down are "pressed"
	m_afButtonReleased = buttonsChanged & (~pev->button); // The ones not down are "released"

	g_pGameRules->PlayerThink(this);

	UpdateClientData();

	if (g_pGameRules->GetState() == GR_STATE_GAME_OVER)
		return; // intermission or finale

	// Observer Button Handling
	if (IsObserver())
	{
		Observer_HandleButtons();
		Observer_CheckTarget();
		pev->impulse = 0;

		if (IsSpectator())
		{
			return;
		}
	}

	if (!IsAlive())
	{
		PlayerDeathFrame();
		return;
	}

	// Train speed control
#ifdef HALFLIFE_TRAINCONTROL
	if ((pev->flags & FL_ONTRAIN) != 0)
	{
		CBaseEntity* pTrain = CBaseEntity::Instance(pev->groundentity);
		float vel;

		if (!pTrain)
		{
			TraceResult trainTrace;
			// Maybe this is on the other side of a level transition
			util::TraceLine(pev->origin, pev->origin + Vector(0, 0, -38), util::ignore_monsters, this, &trainTrace);

			// HACKHACK - Just look for the func_tracktrain classname
			if (trainTrace.flFraction != 1.0 && trainTrace.pHit)
				pTrain = CBaseEntity::Instance(trainTrace.pHit);


			if (!pTrain || (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) == 0 || !pTrain->OnControls(pev))
			{
				//ALERT( at_error, "In train mode with no train!\n" );
				pev->flags &= ~FL_ONTRAIN;
				m_iTrain = TRAIN_NEW | TRAIN_OFF;
				return;
			}
		}
		else if ((pev->flags & FL_ONGROUND) == 0 || (pTrain->pev->spawnflags & SF_TRACKTRAIN_NOCONTROL) != 0 || (pev->button & (IN_MOVELEFT | IN_MOVERIGHT)) != 0)
		{
			// Turn off the train if you jump, strafe, or the train controls go dead
			pev->flags &= ~FL_ONTRAIN;
			m_iTrain = TRAIN_NEW | TRAIN_OFF;
			return;
		}

		pev->velocity = g_vecZero;
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
			m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
			m_iTrain |= TRAIN_ACTIVE | TRAIN_NEW;
		}
	}
	else if ((m_iTrain & TRAIN_ACTIVE) != 0)
		m_iTrain = TRAIN_NEW; // turn off train
#endif

	if ((pev->flags & FL_ONGROUND) == 0)
	{
		m_flFallVelocity = -pev->velocity.z;
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

	pev->classname = MAKE_STRING("player");
	pev->health = 100;
	pev->armorvalue = 0;
	pev->takedamage = DAMAGE_AIM;
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_WALK;
	pev->max_health = pev->health;
	pev->flags &= FL_PROXY | FL_FAKECLIENT; // keep proxy and fakeclient flags set by engine
	pev->flags |= FL_CLIENT;
	pev->air_finished = gpGlobals->time + 12;
	pev->dmg = 2; // initial water damage
	pev->effects = EF_NOINTERP;
	pev->deadflag = DEAD_NO;
	pev->dmg_take = 0;
	pev->dmg_save = 0;
	pev->friction = 1.0;
	pev->gravity = 1.0;
	m_bitsHUDDamage = -1;
	m_bitsDamageType = 0;
	pev->iuser1 = OBS_NONE;
	pev->iuser2 = pev->iuser3 = 0;
	m_hLastAttacker[0] = m_hLastAttacker[1] = nullptr;

	m_iFOV = 0;
	m_ClientSndRoomtype = -1;

	m_flNextDecalTime = -decalfrequency.value; // let this player decal as soon as they spawn

	// dont let uninitialized value here hurt the player
	m_flFallVelocity = 0;

	auto spawn = g_pGameRules->GetPlayerSpawnSpot(this);

	pev->origin = spawn->m_origin;
	pev->v_angle = g_vecZero;
	pev->velocity = g_vecZero;
	pev->angles = spawn->m_angles;
	pev->punchangle = g_vecZero;
	pev->fixangle = 1;

	SetModel("models/player.mdl");
	SetAction(Action::Idle, true);

	Vector mins, maxs;
	if (PM_GetHullBounds(
		(pev->flags & FL_DUCKING) != 0 ? kHullPlayer : kHullDuck, mins, maxs))
	{
		SetSize(mins, maxs);
	}

	pev->view_ofs = VEC_VIEW;

	m_ResetHUD = ResetHUD::Reset;
	m_ClientWeaponBits = 0;

	// reset all ammo values to 0
	memset(m_rgAmmo, 0, sizeof(m_rgAmmo));

	MessageBegin(MSG_ONE, gmsgAmmo, this);
	for (int i = 0; i < AMMO_TYPES; i++)
	{
		WriteByte(g_pGameRules->GetMaxAmmo(this, i));
	}
	MessageEnd();

	m_StateBits = 0;

	char* infobuffer = g_engfuncs.pfnGetInfoKeyBuffer(edict());
	char* value = g_engfuncs.pfnInfoKeyValue(infobuffer, "cl_righthand");

	if ('\0' != *value)
	{
		m_bLeftHanded = atoi(value) == 0;
	}
	else
	{
		m_bLeftHanded = false;
	}

	g_pGameRules->PlayerSpawn(this);

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
		ALERT(at_console, "No Landmark:%s\n", pSaveData->szLandmarkName);

		// default to normal spawn
		auto spawn = g_pGameRules->GetPlayerSpawnSpot(this);
		pev->origin = spawn->m_origin;
		pev->angles = spawn->m_angles;
	}
	pev->v_angle.z = 0; // Clear out roll
	pev->angles = pev->v_angle;

	pev->fixangle = 1; // turn this way immediately

	m_ClientSndRoomtype = -1;

	// Reset room type on level change.
	if (!FStrEq(restore.GetData().szCurrentMapName, STRING(gpGlobals->mapname)))
	{
		m_SndRoomtype = 0;
	}

	Vector mins, maxs;
	if (PM_GetHullBounds(
		(pev->flags & FL_DUCKING) != 0 ? kHullPlayer : kHullDuck, mins, maxs))
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
	bool Spawn(CBaseEntity* owner);
	void Think() override;

#ifdef HALFLIFE_SAVERESTORE
	int ObjectCaps() override { return FCAP_DONT_SAVE; }
#endif
};

bool CSprayCan::Spawn(CBaseEntity* owner)
{
	pev->origin = owner->pev->origin + Vector(0, 0, 32);
	pev->angles = owner->pev->v_angle;
	pev->owner = owner->edict();
	pev->frame = 0;

	pev->nextthink = gpGlobals->time + 0.1;
	EmitSound("player/sprayer.wav", CHAN_VOICE);

	return true;
}

void CSprayCan::Think()
{
	TraceResult tr;
	int playernum;
	int nFrames;
	CBasePlayer* pPlayer;

	pPlayer = (CBasePlayer*)GET_PRIVATE(pev->owner);

	if (pPlayer)
		nFrames = pPlayer->GetCustomDecalFrames();
	else
		nFrames = -1;

	playernum = ENTINDEX(pev->owner);

	// ALERT(at_console, "Spray by player %i, %i of %i\n", playernum, (int)(pev->frame + 1), nFrames);

	util::MakeVectors(pev->angles);
	util::TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * 128, util::ignore_monsters, pPlayer, &tr);

	tent::PlayerDecalTrace(&tr, playernum, pev->frame);
	// Just painted last custom frame.
	if (pev->frame++ >= (nFrames - 1))
		Remove();

	pev->nextthink = gpGlobals->time + 0.1;
}


//==============================================

static edict_t* GiveNamedItem_Common(entvars_t* pev, const char* pszName)
{
	int istr = MAKE_STRING(pszName);

	edict_t* pent = CREATE_NAMED_ENTITY(istr);
	if (FNullEnt(pent))
	{
		ALERT(at_console, "NULL Ent in GiveNamedItem!\n");
		return nullptr;
	}
	VARS(pent)->origin = pev->origin;
	pent->v.spawnflags |= SF_NORESPAWN;

	DispatchSpawn(pent);

	return pent;
}

void CBasePlayer::GiveNamedItem(const char* szName)
{
	auto pent = GiveNamedItem_Common(pev, szName);

	if (!pent)
	{
		return;
	}

	DispatchTouch(pent, ENT(pev));
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

	int iImpulse = (int)pev->impulse;
	switch (iImpulse)
	{
	case 201: // paint decal

		if (gpGlobals->time - m_flNextDecalTime < decalfrequency.value
		 || GetCustomDecalFrames() == -1)
		{
			// too early!
			break;
		}

		util::MakeVectors(pev->v_angle);
		util::TraceLine(pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, util::ignore_monsters, this, &tr);

		if (tr.flFraction != 1.0)
		{ // line hit something, so paint a decal
			m_flNextDecalTime = gpGlobals->time;
			CSprayCan* pCan = GetClassPtr((CSprayCan*)nullptr);
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

	pev->impulse = 0;
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
		GiveNamedItem("weapon_crowbar");
		GiveNamedItem("weapon_9mmAR");
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
			ALERT(at_console, "Classname: %s", STRING(pEntity->pev->classname));

			if (!FStringNull(pEntity->pev->targetname))
			{
				ALERT(at_console, " - Targetname: %s\n", STRING(pEntity->pev->targetname));
			}
			else
			{
				ALERT(at_console, " - TargetName: No Targetname\n");
			}

			ALERT(at_console, "Model: %s\n", STRING(pEntity->pev->model));
			if (!FStringNull(pEntity->pev->globalname))
				ALERT(at_console, "Globalname: %s\n", STRING(pEntity->pev->globalname));
		}
		break;

	case 107:
	{
		TraceResult tr;

		edict_t* pWorld = CWorld::World->edict();

		Vector start = pev->origin + pev->view_ofs;
		Vector end = start + gpGlobals->v_forward * 1024;
		util::TraceLine(start, end, util::ignore_monsters, this, &tr);
		if (tr.pHit)
			pWorld = tr.pHit;
		const char* pTextureName = TRACE_TEXTURE(pWorld, start, end);
		if (pTextureName)
			ALERT(at_console, "Texture: %s (%c)\n", pTextureName, PM_FindTextureType(pTextureName));
	}
	break;

#ifdef HALFLIFE_NODEGRAPH
	case 195: // show shortest paths for entire level to nearest node
	{
		if (WorldGraph.IsAvailable())
		{
			Create("node_viewer_fly", pev->origin, pev->angles);
		}
	}
	break;
	case 196: // show shortest paths for entire level to nearest node
	{
		if (WorldGraph.IsAvailable())
		{
			Create("node_viewer_large", pev->origin, pev->angles);
		}
	}
	break;
	case 197: // show shortest paths for entire level to nearest node
	{
		if (WorldGraph.IsAvailable())
		{
			Create("node_viewer_human", pev->origin, pev->angles);
		}
	}
	break;
	case 199: // show nearest node and all connections
	{
		if (WorldGraph.IsAvailable())
		{
			ALERT(at_console, "%d\n", WorldGraph.FindNearestNode(pev->origin, bits_NODE_GROUP_REALM));
			WorldGraph.ShowNodeConnections(WorldGraph.FindNearestNode(pev->origin, bits_NODE_GROUP_REALM));
		}
	}
	break;
#endif

	case 203: // remove creature.
		pEntity = util::FindEntityForward(this);
		if (pEntity)
		{
			if (0 != pEntity->pev->takedamage)
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
		weapon->Holster();
		m_pActiveWeapon = nullptr;
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


bool CBasePlayer::GiveAmmo(int iCount, int iType)
{
	if (!g_pGameRules->CanHaveAmmo(this, iType))
	{
		// game rules say I can't have any more of this ammo type.
		return false;
	}

	int iAdd = std::min(iCount, g_pGameRules->GetMaxAmmo(this, iType) - m_rgAmmo[iType]);

	if (iAdd < 1)
	{
		return true;
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

	return true;
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

	if (0 != pev->dmg_take || 0 != pev->dmg_save || m_bitsHUDDamage != m_bitsDamageType)
	{
		// Comes from inside me if not set
		Vector damageOrigin = pev->origin;
		// send "damage" message
		// causes screen to flash, and pain compass to show direction of damage
		edict_t* other = pev->dmg_inflictor;
		if (other != nullptr)
		{
			CBaseEntity* pEntity = CBaseEntity::Instance(other);
			if (pEntity != nullptr)
			{
				damageOrigin = pEntity->Center();
			}

			pev->dmg_inflictor = nullptr;
		}

		MessageBegin(MSG_ONE, gmsgDamage, this);
		WriteByte(std::clamp(static_cast<int>(pev->dmg_save), 0, 255));
		WriteByte(std::clamp(static_cast<int>(pev->dmg_take), 0, 255));
		WriteLong(m_bitsDamageType);
		WriteCoord(damageOrigin.x);
		WriteCoord(damageOrigin.y);
		WriteCoord(damageOrigin.z);
		MessageEnd();

		pev->dmg_take = 0;
		pev->dmg_save = 0;
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
		pev->flags |= FL_FROZEN;
	else
		pev->flags &= ~FL_FROZEN;
}


//=========================================================
// Autoaim
// set crosshair position to point to enemey
//=========================================================
Vector CBasePlayer::GetAimVector()
{
	util::MakeVectors(pev->v_angle + pev->punchangle);
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
			if (FClassnameIs(it->pev, pszWeaponName))
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

	util::MakeVectors(pev->angles);

	CWeaponBox* pWeaponBox = (CWeaponBox*)CBaseEntity::Create("weaponbox", pev->origin + gpGlobals->v_forward * 10, pev->angles, edict());
	pWeaponBox->SetModel(STRING(pWeapon->pev->model));
	pWeaponBox->pev->angles.x = 0;
	pWeaponBox->pev->angles.z = 0;
	pWeaponBox->PackWeapon(pWeapon);
	pWeaponBox->pev->velocity = gpGlobals->v_forward * 300 + gpGlobals->v_forward * 100;

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
	const char* value = g_engfuncs.pfnInfoKeyValue(infobuffer, "cl_autowepswitch");

	if ('\0' != *value)
	{
		m_iAutoWepSwitch = atoi(value);
	}
	else
	{
		m_iAutoWepSwitch = 1;
	}

	value = g_engfuncs.pfnInfoKeyValue(infobuffer, "cl_grenadetoggle");

	if ('\0' != *value)
	{
		m_bGrenadeToggle = atoi(value) != 0;
	}
	else
	{
		m_bGrenadeToggle = false;
	}
}

void CBasePlayer::GetEntityState(entity_state_t& state)
{
	CBaseEntity::GetEntityState(state);

	state.team = pev->team;
	state.playerclass = pev->playerclass;

	state.basevelocity = pev->basevelocity;

	state.weaponmodel = pev->weaponmodel;
	state.gaitsequence = pev->gaitsequence;
	state.spectator = (pev->flags & FL_SPECTATOR) != 0;
	state.friction = pev->friction;

	state.gravity = pev->gravity;

	state.usehull = (pev->flags & FL_DUCKING) != 0 ? 1 : 0;

	if (IsSpectator() || IsAlive())
	{
		state.health = std::max(pev->health, 1.0F);
	}
	else
	{
		state.health = 0.0F;
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

	CPrimeGrenade::PrimeGrenade(this);

	MessageBegin(MSG_ONE, gmsgStatusIcon, this);
	WriteByte(2);
	WriteString("grenade");
	MessageEnd();

	EmitSoundPredicted("weapons/ax1.wav", CHAN_WEAPON);
}


void CBasePlayer::ThrowGrenade()
{
	LeaveState(State::GrenadePrime);
	EnterState(State::GrenadeThrowing);
}

#endif


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

	WriteByte(entindex());

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
	if (!IsNetClient())
	{
		flags |= 4;
	}
	WriteByte(flags);

	MessageEnd();
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
	WriteByte(entindex());
	WriteByte(channel);
	WriteByte(volume);
	WriteByte(attenuation);
	WriteByte(pitch);
	WriteString(sample);
	MessageEnd();
}


class CStripWeapons : public CPointEntity
{
public:
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

private:
};

LINK_ENTITY_TO_CLASS(player_weaponstrip, CStripWeapons);

void CStripWeapons::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	CBasePlayer* pPlayer = NULL;

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

