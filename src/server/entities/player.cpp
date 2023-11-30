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
#include "trains.h"
#include "nodes.h"
#include "weapons.h"
#include "shake.h"
#include "decals.h"
#include "gamerules.h"
#include "game.h"
#include "pm_shared.h"
#include "hltv.h"
#include "UserMessages.h"
#include "client.h"
#include "animation.h"

unsigned short g_usGibbed;

#define TRAIN_ACTIVE 0x80
#define TRAIN_NEW 0xc0
#define TRAIN_OFF 0x00
#define TRAIN_NEUTRAL 0x01
#define TRAIN_SLOW 0x02
#define TRAIN_MEDIUM 0x03
#define TRAIN_FAST 0x04
#define TRAIN_BACK 0x05

#define FLASH_DRAIN_TIME 1.2  //100 units/3 minutes
#define FLASH_CHARGE_TIME 0.2 // 100 units/20 seconds  (seconds per unit)

// Global Savedata for player
TYPEDESCRIPTION CBasePlayer::m_playerSaveData[] =
{
		DEFINE_FIELD(CBasePlayer, m_afButtonLast, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_afButtonPressed, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_afButtonReleased, FIELD_INTEGER),

		DEFINE_FIELD(CBasePlayer, m_afPhysicsFlags, FIELD_INTEGER),

		DEFINE_ARRAY(CBasePlayer, m_rgpPlayerWeapons, FIELD_CLASSPTR, WEAPON_LAST),
		DEFINE_FIELD(CBasePlayer, m_pActiveWeapon, FIELD_CLASSPTR),
		DEFINE_FIELD(CBasePlayer, m_WeaponBits, FIELD_INT64),

		DEFINE_ARRAY(CBasePlayer, m_rgAmmo, FIELD_INTEGER, AMMO_LAST),
		DEFINE_FIELD(CBasePlayer, m_idrowndmg, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_idrownrestored, FIELD_INTEGER),

		DEFINE_FIELD(CBasePlayer, m_iTrain, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_bitsHUDDamage, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_flFallVelocity, FIELD_FLOAT),
		DEFINE_FIELD(CBasePlayer, m_fInitHUD, FIELD_BOOLEAN),
		DEFINE_FIELD(CBasePlayer, m_tbdPrev, FIELD_TIME),

		DEFINE_FIELD(CBasePlayer, m_pTank, FIELD_EHANDLE),
		DEFINE_FIELD(CBasePlayer, m_iHideHUD, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_iFOV, FIELD_INTEGER),

		DEFINE_FIELD(CBasePlayer, m_SndRoomtype, FIELD_INTEGER),
};

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


int TrainSpeed(int iSpeed, int iMax)
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

void CBasePlayer::DeathSound()
{
	// water death sounds
	/*
	if (pev->waterlevel == 3)
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

bool CBasePlayer::TakeHealth(float flHealth, int bitsDamageType)
{
	if (pev->takedamage == DAMAGE_NO)
	{
		return false;
	}
	
	m_bitsDamageType &= ~(bitsDamageType & ~DMG_TIMEBASED);

	return CBaseAnimating::TakeHealth(flHealth, bitsDamageType);
}

//=========================================================
// TraceAttack
//=========================================================
void CBasePlayer::TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	if (pev->takedamage == DAMAGE_NO)
	{
		return;
	}

	m_LastHitGroup = ptr->iHitgroup;

	if ((bitsDamageType & DMG_AIMED) != 0)
	{
		switch (ptr->iHitgroup)
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

	AddMultiDamage(attacker, attacker, this, flDamage, bitsDamageType);
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
		take = armour;
		take *= 1 / bonus;
		newDamage = damage - take;
		return armour;
	}
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
	if (attacker != CWorld::World && attacker->pev->solid != SOLID_TRIGGER)
	{
		// Move them around!
		g_vecAttackDir = (inflictor->Center() - Vector(0, 0, 10) - Center()).Normalize();

		pev->velocity = pev->velocity + g_vecAttackDir * -DamageForce(pev, flDamage);
	}
	pev->dmg_inflictor = inflictor->edict();

	// Check for godmode or invincibility.
	if ((pev->flags & FL_GODMODE) != 0)
	{
		return false;
	}

	if (attacker->IsPlayer())
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

	// Reset damage time countdown for each type of time based damage player just sustained.
	for (int i = 0; i < CDMG_TIMEBASED; i++)
	{
		if ((bitsDamageType & (DMG_PARALYZE << i)) != 0)
		{
			m_rgbTimeBasedDamage[i] = 0;
		}
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
	CBasePlayerWeapon* rgpPackWeapons[WEAPON_LAST];
	int iPackAmmo[AMMO_LAST + 1];
	int iPW = 0; // index into packweapons array
	int iPA = 0; // index into packammo array

	memset(rgpPackWeapons, 0, sizeof(rgpPackWeapons));
	memset(iPackAmmo, -1, sizeof(iPackAmmo));

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
		for (i = 0; i < AMMO_LAST; i++)
		{
			if (m_rgAmmo[i] > 0)
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

	pWeaponBox->SetThink(&CWeaponBox::Kill);
	pWeaponBox->pev->nextthink = gpGlobals->time + 120;

	// back these two lists up to their first elements
	iPA = 0;
	iPW = 0;

	// pack the ammo
	while (iPackAmmo[iPA] != -1)
	{
		pWeaponBox->PackAmmo(iPackAmmo[iPA], m_rgAmmo[iPackAmmo[iPA]]);
		iPA++;
	}

	// now pack all of the weapons in the lists
	while (rgpPackWeapons[iPW])
	{
		// weapon unhooked from the player. Pack it into der box.
		pWeaponBox->PackWeapon(rgpPackWeapons[iPW]);

		iPW++;
	}

	pWeaponBox->pev->velocity = pev->velocity * 1.2; // weaponbox has player's velocity, then some.

	RemoveAllWeapons(); // now strip off everything that wasn't handled by the code above.
}

void CBasePlayer::RemoveAllWeapons()
{
	if (m_pActiveWeapon)
	{
		m_pActiveWeapon->Holster();
		m_pActiveWeapon = NULL;
	}

	if (m_pTank != NULL)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	for (auto gun : m_lpPlayerWeapons)
	{
		gun->Drop();
	}
	m_pActiveWeapon = nullptr;
	memset(m_rgpPlayerWeapons, 0, sizeof(m_rgpPlayerWeapons));
	m_lpPlayerWeapons.clear();

	pev->viewmodel = 0;
	pev->weaponmodel = 0;

	m_WeaponBits = 0ULL;

	for (int i = 0; i < AMMO_LAST; i++)
	{
		m_rgAmmo[i] = 0;
	}

	UpdateClientData();
}


void CBasePlayer::Killed(CBaseEntity* inflictor, CBaseEntity* attacker, int bitsDamageType)
{
	// Holster weapon immediately, to allow it to cleanup
	if (m_pActiveWeapon)
	{
		m_pActiveWeapon->Holster();
	}

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

	if (m_pTank != NULL)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	SetAnimation(PLAYER_DIE);

	m_fDeadTime = gpGlobals->time;
	m_fNextSuicideTime = gpGlobals->time + 1.0f;

	pev->health = std::min(pev->health, 0.0F);
	pev->deadflag = DEAD_DYING;

	m_iFOV = 0;

	m_iObserverLastMode = OBS_CHASE_FREE;
	pev->iuser1 = OBS_DEATHCAM;
	pev->iuser2 = entindex();
	pev->iuser3 = attacker->entindex();

	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;

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

	PLAYBACK_EVENT_FULL(
		FEV_GLOBAL | FEV_RELIABLE,
		edict(),
		g_usGibbed,
		0.0f,
		pev->origin,
		util::VecToAngles(g_vecAttackDir),
		0.0,
		pev->health,
		pev->sequence,
		gibMode,
		false,
		false
	);
}


// Set the activity based on an event or current state
void CBasePlayer::SetAnimation(PLAYER_ANIM playerAnim)
{
	int animDesired;
	float speed;
	char szAnim[64];

	speed = pev->velocity.Length2D();

	switch (playerAnim)
	{
	case PLAYER_JUMP:
		m_IdealActivity = ACT_HOP;
		break;

	case PLAYER_SUPERJUMP:
		m_IdealActivity = ACT_LEAP;
		break;

	case PLAYER_DIE:
		m_IdealActivity = ACT_DIESIMPLE;
		m_IdealActivity = GetDeathActivity();
		break;

	case PLAYER_ATTACK1:
		switch (m_Activity)
		{
		case ACT_HOVER:
		case ACT_SWIM:
		case ACT_HOP:
		case ACT_LEAP:
		case ACT_DIESIMPLE:
			m_IdealActivity = m_Activity;
			break;
		default:
			m_IdealActivity = ACT_RANGE_ATTACK1;
			break;
		}
		break;
	case PLAYER_IDLE:
	case PLAYER_WALK:
		if ((pev->flags & FL_ONGROUND) == 0 && (m_Activity == ACT_HOP || m_Activity == ACT_LEAP)) // Still jumping
		{
			m_IdealActivity = m_Activity;
		}
		else if (pev->waterlevel > 1)
		{
			if (speed == 0)
				m_IdealActivity = ACT_HOVER;
			else
				m_IdealActivity = ACT_SWIM;
		}
		else
		{
			m_IdealActivity = ACT_WALK;
		}
		break;
	}

	switch (m_IdealActivity)
	{
	case ACT_HOVER:
	case ACT_LEAP:
	case ACT_SWIM:
	case ACT_HOP:
	case ACT_DIESIMPLE:
	default:
		if (m_Activity == m_IdealActivity)
			return;
		m_Activity = m_IdealActivity;

		animDesired = LookupActivity(m_Activity);
		// Already using the desired animation?
		if (pev->sequence == animDesired)
			return;

		pev->gaitsequence = 0;
		pev->sequence = animDesired;
		pev->frame = 0;
		ResetSequenceInfo();
		return;

	case ACT_RANGE_ATTACK1:
		if ((pev->flags & FL_DUCKING) != 0) // crouching
			strcpy(szAnim, "crouch_shoot_");
		else
			strcpy(szAnim, "ref_shoot_");
		strcat(szAnim, m_szAnimExtention);
		animDesired = LookupSequence(szAnim);
		if (animDesired == -1)
			animDesired = 0;

		if (pev->sequence != animDesired || !m_fSequenceLoops)
		{
			pev->frame = 0;
		}

		if (!m_fSequenceLoops)
		{
			pev->effects |= EF_NOINTERP;
		}

		m_Activity = m_IdealActivity;

		pev->sequence = animDesired;
		ResetSequenceInfo();
		break;

	case ACT_WALK:
		if (m_Activity != ACT_RANGE_ATTACK1 || m_fSequenceFinished)
		{
			if ((pev->flags & FL_DUCKING) != 0) // crouching
				strcpy(szAnim, "crouch_aim_");
			else
				strcpy(szAnim, "ref_aim_");
			strcat(szAnim, m_szAnimExtention);
			animDesired = LookupSequence(szAnim);
			if (animDesired == -1)
				animDesired = 0;
			m_Activity = ACT_WALK;
		}
		else
		{
			animDesired = pev->sequence;
		}
	}

	if ((pev->flags & FL_DUCKING) != 0)
	{
		if (speed == 0)
		{
			pev->gaitsequence = LookupActivity(ACT_CROUCHIDLE);
		}
		else
		{
			pev->gaitsequence = LookupActivity(ACT_CROUCH);
		}
	}
	else if (speed > 220)
	{
		pev->gaitsequence = LookupActivity(ACT_RUN);
	}
	else if (speed > 0)
	{
		pev->gaitsequence = LookupActivity(ACT_WALK);
	}
	else
	{
		pev->gaitsequence = LookupSequence("deep_idle");
	}


	// Already using the desired animation?
	if (pev->sequence == animDesired)
		return;

	//ALERT( at_console, "Set animation to %d\n", animDesired );
	// Reset to first frame of desired animation
	pev->sequence = animDesired;
	pev->frame = 0;
	ResetSequenceInfo();
}

//=========================================================
// GetDeathActivity - determines the best type of death
// anim to play.
//=========================================================
Activity CBasePlayer::GetDeathActivity()
{
	Activity deathActivity;
	bool fTriedDirection;
	float flDot;
	TraceResult tr;
	Vector vecSrc;

	if (pev->deadflag != DEAD_NO)
	{
		// don't run this while dying.
		return m_IdealActivity;
	}

	vecSrc = Center();

	fTriedDirection = false;
	deathActivity = ACT_DIESIMPLE; // in case we can't find any special deaths to do.

	util::MakeVectors(pev->angles);
	flDot = DotProduct(gpGlobals->v_forward, g_vecAttackDir * -1);

	switch (m_LastHitGroup)
	{
		// try to pick a region-specific death.
	case HITGROUP_HEAD:
		deathActivity = ACT_DIE_HEADSHOT;
		break;

	case HITGROUP_STOMACH:
		deathActivity = ACT_DIE_GUTSHOT;
		break;

	case HITGROUP_GENERIC:
		// try to pick a death based on attack direction
		fTriedDirection = true;

		if (flDot > 0.3)
		{
			deathActivity = ACT_DIEFORWARD;
		}
		else if (flDot <= -0.3)
		{
			deathActivity = ACT_DIEBACKWARD;
		}
		break;

	default:
		// try to pick a death based on attack direction
		fTriedDirection = true;

		if (flDot > 0.3)
		{
			deathActivity = ACT_DIEFORWARD;
		}
		else if (flDot <= -0.3)
		{
			deathActivity = ACT_DIEBACKWARD;
		}
		break;
	}


	// can we perform the prescribed death?
	if (LookupActivity(deathActivity) == ACTIVITY_NOT_AVAILABLE)
	{
		// no! did we fail to perform a directional death?
		if (fTriedDirection)
		{
			// if yes, we're out of options. Go simple.
			deathActivity = ACT_DIESIMPLE;
		}
		else
		{
			// cannot perform the ideal region-specific death, so try a direction.
			if (flDot > 0.3)
			{
				deathActivity = ACT_DIEFORWARD;
			}
			else if (flDot <= -0.3)
			{
				deathActivity = ACT_DIEBACKWARD;
			}
		}
	}

	if (LookupActivity(deathActivity) == ACTIVITY_NOT_AVAILABLE)
	{
		// if we're still invalid, simple is our only option.
		deathActivity = ACT_DIESIMPLE;
	}

	if (deathActivity == ACT_DIEFORWARD)
	{
		// make sure there's room to fall forward
		util::TraceHull(vecSrc, vecSrc + gpGlobals->v_forward * 64, util::dont_ignore_monsters, util::head_hull, edict(), &tr);

		if (tr.flFraction != 1.0)
		{
			deathActivity = ACT_DIESIMPLE;
		}
	}

	if (deathActivity == ACT_DIEBACKWARD)
	{
		// make sure there's room to fall backward
		util::TraceHull(vecSrc, vecSrc - gpGlobals->v_forward * 64, util::dont_ignore_monsters, util::head_hull, edict(), &tr);

		if (tr.flFraction != 1.0)
		{
			deathActivity = ACT_DIESIMPLE;
		}
	}

	return deathActivity;
}

//=========================================================
// GetSmallFlinchActivity - determines the best type of flinch
// anim to play.
//=========================================================
Activity CBasePlayer::GetSmallFlinchActivity()
{
	Activity flinchActivity;
	bool fTriedDirection;
	float flDot;

	fTriedDirection = false;
	util::MakeVectors(pev->angles);
	flDot = DotProduct(gpGlobals->v_forward, g_vecAttackDir * -1);

	switch (m_LastHitGroup)
	{
		// pick a region-specific flinch
	case HITGROUP_HEAD:
		flinchActivity = ACT_FLINCH_HEAD;
		break;
	case HITGROUP_STOMACH:
		flinchActivity = ACT_FLINCH_STOMACH;
		break;
	case HITGROUP_LEFTARM:
		flinchActivity = ACT_FLINCH_LEFTARM;
		break;
	case HITGROUP_RIGHTARM:
		flinchActivity = ACT_FLINCH_RIGHTARM;
		break;
	case HITGROUP_LEFTLEG:
		flinchActivity = ACT_FLINCH_LEFTLEG;
		break;
	case HITGROUP_RIGHTLEG:
		flinchActivity = ACT_FLINCH_RIGHTLEG;
		break;
	case HITGROUP_GENERIC:
	default:
		// just get a generic flinch.
		flinchActivity = ACT_SMALL_FLINCH;
		break;
	}


	// do we have a sequence for the ideal activity?
	if (LookupActivity(flinchActivity) == ACTIVITY_NOT_AVAILABLE)
	{
		flinchActivity = ACT_SMALL_FLINCH;
	}

	return flinchActivity;
}


/*
===========
WaterMove
============
*/
#define AIRTIME 12 // lung full of air lasts this many seconds

void CBasePlayer::WaterMove()
{
	int air;

	if (pev->movetype == MOVETYPE_NOCLIP)
		return;

	if (pev->health < 0)
		return;

	// waterlevel 0 - not in water
	// waterlevel 1 - feet in water
	// waterlevel 2 - waist in water
	// waterlevel 3 - head in water

	if (pev->waterlevel != 3)
	{
		// not underwater

		// play 'up for air' sound
		if (pev->air_finished < gpGlobals->time)
			EmitSound("player/pl_wade1.wav", CHAN_VOICE);
		else if (pev->air_finished < gpGlobals->time + 9)
			EmitSound("player/pl_wade2.wav", CHAN_VOICE);

		pev->air_finished = gpGlobals->time + AIRTIME;
		pev->dmg = 2;

		// if we took drowning damage, give it back slowly
		if (m_idrowndmg > m_idrownrestored)
		{
			// set drowning damage bit.  hack - dmg_drownrecover actually
			// makes the time based damage code 'give back' health over time.
			// make sure counter is cleared so we start count correctly.

			// NOTE: this actually causes the count to continue restarting
			// until all drowning damage is healed.

			m_bitsDamageType |= DMG_DROWNRECOVER;
			m_bitsDamageType &= ~DMG_DROWN;
			m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
		}
	}
	else
	{ // fully under water
		// stop restoring damage while underwater
		m_bitsDamageType &= ~DMG_DROWNRECOVER;
		m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;

		if (pev->air_finished < gpGlobals->time) // drown!
		{
			if (pev->pain_finished < gpGlobals->time)
			{
				// take drowning damage
				pev->dmg += 1;
				if (pev->dmg > 5)
					pev->dmg = 5;

				const float oldHealth = pev->health;

				TakeDamage(CWorld::World, CWorld::World, pev->dmg, DMG_DROWN);
				pev->pain_finished = gpGlobals->time + 1;

				// track drowning damage, give it back when
				// player finally takes a breath

				// Account for god mode, the unkillable flag, potential damage mitigation
				// and gaining health from drowning to avoid counting damage not actually taken.
				const float drownDamageTaken = std::max(0.f, std::floor(oldHealth - pev->health));

				m_idrowndmg += drownDamageTaken;
			}
		}
		else
		{
			m_bitsDamageType &= ~DMG_DROWN;
		}
	}

	if (0 == pev->waterlevel)
	{
		pev->dmgtime = 0;
		return;
	}

	// make bubbles

	if (pev->waterlevel == 3)
	{
		air = (int)(pev->air_finished - gpGlobals->time);
		if (!RANDOM_LONG(0, 0x1f) && RANDOM_LONG(0, AIRTIME - 1) >= air)
		{
			switch (RANDOM_LONG(0, 3))
			{
			case 0:
				EmitSound("player/pl_swim1.wav", CHAN_VOICE, 0.8F);
				break;
			case 1:
				EmitSound("player/pl_swim2.wav", CHAN_VOICE, 0.8F);
				break;
			case 2:
				EmitSound("player/pl_swim3.wav", CHAN_VOICE, 0.8F);
				break;
			case 3:
				EmitSound("player/pl_swim4.wav", CHAN_VOICE, 0.8F);
				break;
			}
		}
	}

	if (pev->watertype == CONTENT_LAVA) // do damage
	{
		if (pev->dmgtime < gpGlobals->time)
			TakeDamage(CWorld::World, CWorld::World, 10 * pev->waterlevel, DMG_BURN);
	}
	else if (pev->watertype == CONTENT_SLIME) // do damage
	{
		pev->dmgtime = gpGlobals->time + 1;
		TakeDamage(CWorld::World, CWorld::World, 4 * pev->waterlevel, DMG_ACID);
	}
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
		if (forcerespawn.value > 0.0f || m_afButtonPressed != 0)
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

	if (m_pTank != NULL)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	// Remove all the player's stuff
	if (HasWeapons())
	{
		PackDeadPlayerWeapons();
	}

	// reset FOV
	m_iFOV = 0;

	// Setup flags
	m_iHideHUD = (HIDEHUD_HEALTH | HIDEHUD_WEAPONS);
	pev->effects = EF_NODRAW;
	pev->view_ofs = g_vecZero;
	pev->fixangle = 1;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	pev->flags &= ~FL_DUCKING;
	pev->deadflag = DEAD_RESPAWNABLE;
	pev->health = 1;

	// Clear out the status bar
	m_fInitHUD = true;

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
	if (IsObserver())
		return;

	// Was use pressed or released?
	if (((pev->button | m_afButtonPressed | m_afButtonReleased) & IN_USE) == 0)
		return;

	// Hit Use on a train?
	if ((m_afButtonPressed & IN_USE) != 0)
	{
		if (m_pTank != NULL)
		{
			// Stop controlling the tank
			// TODO: Send HUD Update
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = NULL;
			return;
		}
		else
		{
			if ((m_afPhysicsFlags & PFLAG_ONTRAIN) != 0)
			{
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW | TRAIN_OFF;
				return;
			}
			else
			{ // Start controlling the train!
				CBaseEntity* pTrain = CBaseEntity::Instance(pev->groundentity);

				if (pTrain && (pev->button & IN_JUMP) == 0 && (pev->flags & FL_ONGROUND) != 0 && (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) != 0 && pTrain->OnControls(pev))
				{
					m_afPhysicsFlags |= PFLAG_ONTRAIN;
					m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
					m_iTrain |= TRAIN_NEW;
					EmitSound("plats/train_use1.wav", CHAN_VOICE);
					return;
				}
			}
		}
	}

	CBaseEntity* pObject = NULL;
	CBaseEntity* pClosest = NULL;
	Vector vecLOS;
	float flMaxDot = VIEW_FIELD_NARROW;
	float flDot;

	util::MakeVectors(pev->v_angle); // so we know which way we are facing

	while ((pObject = util::FindEntityInSphere(pObject, pev->origin, PLAYER_SEARCH_RADIUS)) != NULL)
	{

		if ((pObject->ObjectCaps() & (FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE)) != 0)
		{
			// !!!PERFORMANCE- should this check be done on a per case basis AFTER we've determined that
			// this object is actually usable? This dot is being done for every object within PLAYER_SEARCH_RADIUS
			// when player hits the use key. How many objects can be in that area, anyway? (sjb)
			vecLOS = (pObject->Center() - (pev->origin + pev->view_ofs));

			// This essentially moves the origin of the target to the corner nearest the player to test to see
			// if it's "hull" is in the view cone
			vecLOS = util::ClampVectorToBox(vecLOS, pObject->pev->size * 0.5);

			flDot = DotProduct(vecLOS, gpGlobals->v_forward);
			if (flDot > flMaxDot)
			{ // only if the item is in front of the user
				pClosest = pObject;
				flMaxDot = flDot;
				//				ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
			}
			//			ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
		}
	}
	pObject = pClosest;

	// Found an object
	if (pObject)
	{
		//!!!UNDONE: traceline here to prevent USEing buttons through walls
		int caps = pObject->ObjectCaps();

		if ((m_afButtonPressed & IN_USE) != 0)
			EmitSound("common/wpn_select.wav", CHAN_VOICE);

		if (((pev->button & IN_USE) != 0 && (caps & FCAP_CONTINUOUS_USE) != 0) ||
			((m_afButtonPressed & IN_USE) != 0 && (caps & (FCAP_IMPULSE_USE | FCAP_ONOFF_USE)) != 0))
		{
			if ((caps & FCAP_CONTINUOUS_USE) != 0)
				m_afPhysicsFlags |= PFLAG_USING;

			pObject->Use(this, this, USE_SET, 1);
		}
		// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
		else if ((m_afButtonReleased & IN_USE) != 0 && (pObject->ObjectCaps() & FCAP_ONOFF_USE) != 0) // BUGBUG This is an "off" use
		{
			pObject->Use(this, this, USE_SET, 0);
		}
	}
	else
	{
		if ((m_afButtonPressed & IN_USE) != 0)
			EmitSound("common/wpn_denyselect.wav", CHAN_VOICE);
	}
}


void CBasePlayer::AddPoints(float score, bool bAllowNegativeScore)
{
	// Positive score always adds
	if (score < 0)
	{
		if (!bAllowNegativeScore)
		{
			if (pev->frags < 0) // Can't go more negative
				return;

			if (-score > pev->frags) // Will this go negative?
			{
				score = -pev->frags; // Sum will be 0
			}
		}
	}

	pev->frags += score;

	if (g_pGameRules->IsTeamplay() && m_team != nullptr)
	{
		m_team->AddPoints(score);
	}

	MessageBegin(MSG_ALL, gmsgScoreInfo);
	WriteByte(ENTINDEX(edict()));
	WriteShort(pev->frags);
	WriteShort(m_iDeaths);
	WriteShort(PCNumber());
	WriteShort(TeamNumber());
	MessageEnd();
}


void CBasePlayer::AddPointsToTeam(float score, bool bAllowNegativeScore)
{
	int index = entindex();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pPlayer = util::PlayerByIndex(i);

		if (pPlayer && i != index)
		{
			if (g_pGameRules->PlayerRelationship(this, pPlayer) == GR_TEAMMATE)
			{
				pPlayer->AddPoints(score, bAllowNegativeScore);
			}
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

	if (g_fGameOver)
		return; // intermission or finale

	WaterMove();

	// JOHN: checks if new client data (for HUD and view control) needs to be sent to the client
	UpdateClientData();

	CheckTimeBasedDamage();

	// Observer Button Handling
	if (IsObserver())
	{
		Observer_HandleButtons();
		Observer_CheckTarget();
		Observer_CheckProperties();
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

	// So the correct flags get sent to client asap.
	//
	if ((m_afPhysicsFlags & PFLAG_ONTRAIN) != 0)
		pev->flags |= FL_ONTRAIN;
	else
		pev->flags &= ~FL_ONTRAIN;

	// Train speed control
	if ((m_afPhysicsFlags & PFLAG_ONTRAIN) != 0)
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
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW | TRAIN_OFF;
				return;
			}
		}
		else if ((pev->flags & FL_ONGROUND) == 0 || (pTrain->pev->spawnflags & SF_TRACKTRAIN_NOCONTROL) != 0 || (pev->button & (IN_MOVELEFT | IN_MOVERIGHT)) != 0)
		{
			// Turn off the train if you jump, strafe, or the train controls go dead
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
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

	if ((pev->flags & FL_ONGROUND) == 0)
	{
		m_flFallVelocity = -pev->velocity.z;
	}
}


void CBasePlayer::CheckTimeBasedDamage()
{
	int i;
	byte bDuration = 0;

	static float gtbdPrev = 0.0;

	if ((m_bitsDamageType & DMG_TIMEBASED) == 0)
		return;

	// only check for time based damage approx. every 2 seconds
	if (fabs(gpGlobals->time - m_tbdPrev) < 2.0)
		return;

	m_tbdPrev = gpGlobals->time;

	for (i = 0; i < CDMG_TIMEBASED; i++)
	{
		// make sure bit is set for damage type
		if ((m_bitsDamageType & (DMG_PARALYZE << i)) != 0)
		{
			switch (i)
			{
			case itbd_Paralyze:
				bDuration = PARALYZE_DURATION;
				break;
			case itbd_NerveGas:
				bDuration = NERVEGAS_DURATION;
				break;
			case itbd_Poison:
				TakeDamage(CWorld::World, CWorld::World, POISON_DAMAGE, DMG_GENERIC);
				bDuration = POISON_DURATION;
				break;
			case itbd_Radiation:
				bDuration = RADIATION_DURATION;
				break;
			case itbd_DrownRecover:
				// NOTE: this hack is actually used to RESTORE health
				// after the player has been drowning and finally takes a breath
				if (m_idrowndmg > m_idrownrestored)
				{
					int idif = std::min(m_idrowndmg - m_idrownrestored, 10);

					TakeHealth(idif, DMG_GENERIC);
					m_idrownrestored += idif;
				}
				bDuration = 4; // get up to 5*10 = 50 points back
				break;
			case itbd_Acid:
				bDuration = ACID_DURATION;
				break;
			case itbd_SlowBurn:
				bDuration = SLOWBURN_DURATION;
				break;
			case itbd_SlowFreeze:
				bDuration = SLOWFREEZE_DURATION;
				break;
			default:
				bDuration = 0;
			}

			if (0 != m_rgbTimeBasedDamage[i])
			{
				// decrement damage duration, detect when done.
				if (0 == m_rgbTimeBasedDamage[i] || --m_rgbTimeBasedDamage[i] == 0)
				{
					m_rgbTimeBasedDamage[i] = 0;
					// if we're done, clear damage bits
					m_bitsDamageType &= ~(DMG_PARALYZE << i);
				}
			}
			else
				// first time taking this damage type - init damage duration
				m_rgbTimeBasedDamage[i] = bDuration;
		}
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
		}};

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
	pev->effects = 0;
	pev->deadflag = DEAD_NO;
	pev->dmg_take = 0;
	pev->dmg_save = 0;
	pev->friction = 1.0;
	pev->gravity = 1.0;
	m_bitsHUDDamage = -1;
	m_bitsDamageType = 0;
	m_afPhysicsFlags = 0;
	pev->iuser1 = OBS_NONE;
	pev->iuser2 = pev->iuser3 = 0;
	m_hLastAttacker[0] = m_hLastAttacker[1] = nullptr;

	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "hl", "1");
	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "bj", util::dtos1(sv_allowbunnyhopping.value != 0 ? 1 : 0));

	m_iFOV = 0;		   // init field of view.
	m_ClientSndRoomtype = -1;

	m_flNextDecalTime = -decalfrequency.value; // let this player decal as soon as they spawn

	m_iNextAttack = 0;

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
	pev->sequence = LookupActivity(ACT_IDLE);

	Vector mins, maxs;
	if (PM_GetHullBounds(
		(pev->flags & FL_DUCKING) != 0 ? kHullPlayer : kHullDuck, mins, maxs))
	{
		SetSize(mins, maxs);
	}

	pev->view_ofs = VEC_VIEW;
	Precache();

	m_fInitHUD = true;
	m_iClientHideHUD = -1; // force this to be recalculated
	m_ClientWeaponBits = 0;

	// reset all ammo values to 0
	for (int i = 0; i < AMMO_LAST; i++)
	{
		m_rgAmmo[i] = 0;
		m_rgAmmoLast[i] = 0; // client ammo values also have to be reset  (the death hud clear messages does on the client side)
	}

	m_flNextChatTime = gpGlobals->time;

	g_pGameRules->PlayerSpawn(this);

	return true;
}


void CBasePlayer::Precache()
{
	m_bitsDamageType = 0;
	m_bitsHUDDamage = -1;

	m_iTrain |= TRAIN_NEW;

	// Make sure any necessary user messages have been registered
	LinkUserMessages();

	if (gInitHUD)
		m_fInitHUD = true;
}


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

	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "hl", "1");

	m_iNextAttack = 0;

	m_bRestored = true;

	return status;
}


void CBasePlayer::SelectWeapon(const char* pstr)
{
	if (pstr == nullptr || pstr[0] == '\0')
	{
		return;
	}

	int iId = WEAPON_NONE;

	for (auto it : m_lpPlayerWeapons)
	{
		if (FClassnameIs(it->pev, pstr))
		{
			iId = it->m_iId;
			break;
		}
	}

	if (iId == WEAPON_NONE)
	{
		return;
	}

	SelectWeapon(iId);
}


//==============================================
// HasWeapons - do I have any weapons at all?
//==============================================
bool CBasePlayer::HasWeapons()
{
	return m_WeaponBits != 0;
}


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

	int ObjectCaps() override { return FCAP_DONT_SAVE; }
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

	// No customization present.
	if (nFrames == -1)
	{
		tent::DecalTrace(&tr, DECAL_LAMBDA6);
		Remove();
	}
	else
	{
		tent::PlayerDecalTrace(&tr, playernum, pev->frame, true);
		// Just painted last custom frame.
		if (pev->frame++ >= (nFrames - 1))
			Remove();
	}

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

void CBasePlayer::GiveNamedItem(const char* szName, int defaultAmmo)
{
	auto pent = GiveNamedItem_Common(pev, szName);

	if (!pent)
	{
		return;
	}

	auto entity = CBaseEntity::Instance(pent);

	if (!entity)
	{
		return;
	}

	if (auto weapon = dynamic_cast<CBasePlayerWeapon*>(entity); weapon)
	{
		weapon->m_iDefaultAmmo = defaultAmmo;
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
	m_iClientHideHUD = -1;
	m_ClientWeaponBits = 0;
	m_ClientSndRoomtype = -1;

	for (int i = 0; i < AMMO_LAST; ++i)
	{
		m_rgAmmoLast[i] = 0;
	}

	m_iTrain |= TRAIN_NEW; // Force new train message.
	m_fKnownItem = false;  // Force weaponinit messages.
	m_fInitHUD = true;	   // Force HUD gmsgResetHUD message

	// Now force all the necessary messages
	//  to be sent.
	UpdateClientData();
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

		if (gpGlobals->time - m_flNextDecalTime < decalfrequency.value)
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
		GiveAmmo(250, AMMO_9MM, 250);
		GiveAmmo(10, AMMO_ARGRENADES, 10);
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
bool CBasePlayer::AddPlayerWeapon(CBasePlayerWeapon* pWeapon)
{
	if (pWeapon->m_iId <= WEAPON_NONE || pWeapon->m_iId >= WEAPON_LAST)
	{
		return false;
	}

	if (HasPlayerWeapon(pWeapon))
	{
		if (pWeapon->AddDuplicate(m_rgpPlayerWeapons[pWeapon->m_iId]))
		{
			g_pGameRules->PlayerGotWeapon(this, pWeapon);
			pWeapon->CheckRespawn();

			pWeapon->Kill();
		}
		else if (gEvilImpulse101)
		{
			pWeapon->Kill();
		}
		return false;
	}

	if (pWeapon->CanAddToPlayer(this))
	{
		pWeapon->AddToPlayer(this);

		g_pGameRules->PlayerGotWeapon(this, pWeapon);
		pWeapon->CheckRespawn();

		// Add to the list before extracting ammo so weapon ownership checks work properly.
		m_rgpPlayerWeapons[pWeapon->m_iId] = pWeapon;
		m_lpPlayerWeapons.push_front(pWeapon);

		pWeapon->ExtractAmmo(pWeapon);

		// Don't show weapon pickup if we're spawning or if it's an exhaustible weapon (will show ammo pickup instead).
		if (!m_bIsSpawning && (pWeapon->iFlags() & WEAPON_FLAG_EXHAUSTIBLE) == 0)
		{
			MessageBegin(MSG_ONE, gmsgWeapPickup, this);
			WriteByte(pWeapon->m_iId);
			MessageEnd();
		}

		// Should we switch to this weapon?
		if (g_pGameRules->FShouldSwitchWeapon(this, pWeapon))
		{
			SwitchWeapon(pWeapon);
		}

		return true;
	}
	else if (gEvilImpulse101)
	{
		// FIXME: Remove anyway for deathmatch testing.
		pWeapon->Kill();
	}
	return false;
}


bool CBasePlayer::RemovePlayerWeapon(CBasePlayerWeapon* pWeapon)
{
	if (pWeapon->m_iId <= WEAPON_NONE || pWeapon->m_iId >= WEAPON_LAST)
	{
		return false;
	}
	if (m_pActiveWeapon == pWeapon)
	{
		pWeapon->Holster();
		pWeapon->pev->nextthink = 0;
		pWeapon->SetThink(nullptr);
		m_pActiveWeapon = nullptr;
		pev->viewmodel = 0;
		pev->weaponmodel = 0;
	}
	m_rgpPlayerWeapons[pWeapon->m_iId] = nullptr;
	m_lpPlayerWeapons.remove(pWeapon);
	return true;
}


//
// Returns the unique ID for the ammo, or -1 if error
//
int CBasePlayer::GiveAmmo(int iCount, int iType, int iMax)
{
	if (iType <= AMMO_NONE)
	{
		// no ammo.
		return -1;
	}

	if (!g_pGameRules->CanHaveAmmo(this, iType, iMax))
	{
		// game rules say I can't have any more of this ammo type.
		return -1;
	}

	if (iType <= AMMO_NONE || iType >= AMMO_LAST)
		return -1;

	int iAdd = std::min(iCount, iMax - m_rgAmmo[iType]);
	if (iAdd < 1)
		return iType;

	// Toodles FIXME:
	// If this is an exhaustible weapon make sure the player has it.
	/*
	if (const auto& ammoType = CBasePlayerWeapon::AmmoInfoArray[i]; ammoType.WeaponName != nullptr)
	{
		if (!HasNamedPlayerWeapon(ammoType.WeaponName))
		{
			GiveNamedItem(ammoType.WeaponName, 0);
		}
	}
	*/

	m_rgAmmo[iType] += iAdd;


	if (0 != gmsgAmmoPickup) // make sure the ammo messages have been linked first
	{
		// Send the message that ammo has been picked up
		MessageBegin(MSG_ONE, gmsgAmmoPickup, this);
		WriteByte(iType); // ammo ID
		WriteByte(iAdd);				  // amount
		MessageEnd();
	}

	return iType;
}

int CBasePlayer::AmmoInventory(int iAmmoIndex)
{
	if (iAmmoIndex <= AMMO_NONE)
	{
		return -1;
	}

	return m_rgAmmo[iAmmoIndex];
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
	const bool fullHUDInitRequired = m_fInitHUD != false;

	if (m_fInitHUD)
	{
		m_fInitHUD = false;
		gInitHUD = false;

		MessageBegin(MSG_ONE, gmsgResetHUD, this);
		WriteByte(0);
		MessageEnd();

		if (!m_fGameHUDInitialized)
		{
			MessageBegin(MSG_ONE, gmsgInitHUD, this);
			MessageEnd();

			g_pGameRules->InitHUD(this);
			m_fGameHUDInitialized = true;

			m_iObserverLastMode = OBS_ROAMING;

			if (util::IsMultiplayer())
			{
				util::FireTargets("game_playerjoin", this, this, USE_TOGGLE, 0);
			}
		}

		util::FireTargets("game_playerspawn", this, this, USE_TOGGLE, 0);
	}

	if (m_iHideHUD != m_iClientHideHUD)
	{
		MessageBegin(MSG_ONE, gmsgHideWeapon, this);
		WriteByte(m_iHideHUD);
		MessageEnd();

		m_iClientHideHUD = m_iHideHUD;
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
		if (other)
		{
			CBaseEntity* pEntity = CBaseEntity::Instance(other);
			if (pEntity)
				damageOrigin = pEntity->Center();
		}

		// only send down damage type that have hud art
		int visibleDamageBits = m_bitsDamageType & DMG_SHOWNHUD;

		MessageBegin(MSG_ONE, gmsgDamage, this);
		WriteByte(pev->dmg_save);
		WriteByte(pev->dmg_take);
		WriteLong(visibleDamageBits);
		WriteCoord(damageOrigin.x);
		WriteCoord(damageOrigin.y);
		WriteCoord(damageOrigin.z);
		MessageEnd();

		pev->dmg_take = 0;
		pev->dmg_save = 0;
		m_bitsHUDDamage = m_bitsDamageType;

		// Clear off non-time-based damage indicators
		m_bitsDamageType &= DMG_TIMEBASED;
	}


	if ((m_iTrain & TRAIN_NEW) != 0)
	{
		// send "health" update message
		MessageBegin(MSG_ONE, gmsgTrain, this);
		WriteByte(m_iTrain & 0xF);
		MessageEnd();

		m_iTrain &= ~TRAIN_NEW;
	}

	//
	// New Weapon?
	//
	if (!m_fKnownItem)
	{
		m_fKnownItem = true;

		// WeaponInit Message
		// byte  = # of weapons
		//
		// for each weapon:
		// byte		name str length (not including null)
		// bytes... name
		// byte		Ammo Type
		// byte		Ammo2 Type
		// byte		bucket
		// byte		bucket pos
		// byte		flags
		// ????		Icons

		// Send ALL the weapon info now
		int i;

		for (i = 0; i < WEAPON_LAST; i++)
		{
			WeaponInfo& II = CBasePlayerWeapon::WeaponInfoArray[i];

			if (WEAPON_NONE == II.iId)
				continue;

			const char* pszName;
			if (!II.pszName)
				pszName = "Empty";
			else
				pszName = II.pszName;

			MessageBegin(MSG_ONE, gmsgWeaponList, this);
			WriteString(pszName);				   // string	weapon name
			WriteByte(II.iAmmo1); // byte		Ammo Type
			WriteByte(II.iMaxAmmo1);			   // byte     Max Ammo 1
			WriteByte(II.iAmmo2); // byte		Ammo2 Type
			WriteByte(II.iMaxAmmo2);			   // byte     Max Ammo 2
			WriteByte(II.iSlot);				   // byte		bucket
			WriteByte(II.iPosition);			   // byte		bucket pos
			WriteByte(II.iId);					   // byte		id (bit index into m_WeaponBits)
			WriteByte(II.iFlags);				   // byte		Flags
			MessageEnd();
		}
	}

	// Send new room type to client.
	if (m_ClientSndRoomtype != m_SndRoomtype)
	{
		m_ClientSndRoomtype = m_SndRoomtype;

		MessageBegin(MSG_ONE, SVC_ROOMTYPE, this);
		WriteShort((short)m_SndRoomtype); // sequence number
		MessageEnd();
	}

	//Handled anything that needs resetting
	m_bRestored = false;
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

	if (pWeapon == nullptr)
	{
		return;
	}

	// if we land here with a valid pWeapon pointer, that's because we found the
	// weapon we want to drop and hit a BREAK;  pWeapon is the weapon.

	if (!g_pGameRules->GetNextBestWeapon(this, pWeapon))
	{
		return; // can't drop the weapon they asked for, may be our last weapon or something we can't holster
	}

	ClearWeaponBit(pWeapon->m_iId); // take weapon off hud

	util::MakeVectors(pev->angles);

	CWeaponBox* pWeaponBox = (CWeaponBox*)CBaseEntity::Create("weaponbox", pev->origin + gpGlobals->v_forward * 10, pev->angles, edict());
	pWeaponBox->SetModel(STRING(pWeapon->pev->model));
	pWeaponBox->pev->angles.x = 0;
	pWeaponBox->pev->angles.z = 0;
	pWeaponBox->PackWeapon(pWeapon);
	pWeaponBox->pev->velocity = gpGlobals->v_forward * 300 + gpGlobals->v_forward * 100;

	// drop half of the ammo for this weapon.
	int iAmmoIndex = pWeapon->iAmmo1();

	if (iAmmoIndex > AMMO_NONE)
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

//=========================================================
// HasPlayerWeapon Does the player already have this weapon?
//=========================================================
bool CBasePlayer::HasPlayerWeapon(CBasePlayerWeapon* pCheckWeapon)
{
	if (pCheckWeapon->m_iId <= WEAPON_NONE || pCheckWeapon->m_iId >= WEAPON_LAST)
	{
		return false;
	}
	return (m_WeaponBits & (1ULL << pCheckWeapon->m_iId)) != 0;
}

//=========================================================
// HasNamedPlayerWeapon Does the player already have this weapon?
//=========================================================
bool CBasePlayer::HasNamedPlayerWeapon(const char* pszWeaponName)
{
	for (auto pWeapon : m_lpPlayerWeapons)
	{
		if (FClassnameIs(pWeapon->pev, pszWeaponName))
		{
			return true;
		}
	}
	return false;
}

//=========================================================
//
//=========================================================
bool CBasePlayer::SwitchWeapon(CBasePlayerWeapon* pWeapon)
{
	if (pWeapon && !pWeapon->CanDeploy())
	{
		return false;
	}

	if (m_pActiveWeapon)
	{
		m_pActiveWeapon->Holster();
	}

	m_pActiveWeapon = pWeapon;

	if (pWeapon)
	{
		pWeapon->m_ForceSendAnimations = true;
		pWeapon->Deploy();
		pWeapon->m_ForceSendAnimations = false;
	}

	return true;
}

void CBasePlayer::EquipWeapon()
{
	if (m_pActiveWeapon)
	{
		if ((!FStringNull(pev->viewmodel) || !FStringNull(pev->weaponmodel)))
		{
			//Already have a weapon equipped and deployed.
			return;
		}

		//Have a weapon equipped, but not deployed.
		if (m_pActiveWeapon->CanDeploy() && m_pActiveWeapon->Deploy())
		{
			return;
		}
	}

	//No weapon equipped or couldn't deploy it, find a suitable alternative.
	g_pGameRules->GetNextBestWeapon(this, m_pActiveWeapon, true);
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
}

void CBasePlayer::SetEntityState(entity_state_t& state)
{
	CBaseEntity::SetEntityState(state);

	state.team = pev->team;
	state.playerclass = pev->playerclass;

	state.basevelocity = pev->basevelocity;

	state.weaponmodel = pev->weaponmodel;
	state.gaitsequence = pev->gaitsequence;
	state.spectator = (pev->flags & FL_SPECTATOR) != 0;
	state.friction = pev->friction;

	state.gravity = pev->gravity;

	state.usehull = (pev->flags & FL_DUCKING) != 0 ? 1 : 0;

	if (pev->health > 0.0F)
	{
		state.health = std::max(pev->health, 1.0F);
	}
	else
	{
		state.health = 0.0F;
	}
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


class CRevertSaved : public CPointEntity
{
public:
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void EXPORT MessageThink();
	void EXPORT LoadThink();
	bool KeyValue(KeyValueData* pkvd) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	inline float Duration() { return pev->dmg_take; }
	inline float HoldTime() { return pev->dmg_save; }
	inline float MessageTime() { return m_messageTime; }
	inline float LoadTime() { return m_loadTime; }

	inline void SetDuration(float duration) { pev->dmg_take = duration; }
	inline void SetHoldTime(float hold) { pev->dmg_save = hold; }
	inline void SetMessageTime(float time) { m_messageTime = time; }
	inline void SetLoadTime(float time) { m_loadTime = time; }

private:
	float m_messageTime;
	float m_loadTime;
};

LINK_ENTITY_TO_CLASS(player_loadsaved, CRevertSaved);

TYPEDESCRIPTION CRevertSaved::m_SaveData[] =
	{
		DEFINE_FIELD(CRevertSaved, m_messageTime, FIELD_FLOAT), // These are not actual times, but durations, so save as floats
		DEFINE_FIELD(CRevertSaved, m_loadTime, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CRevertSaved, CPointEntity);

bool CRevertSaved::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "duration"))
	{
		SetDuration(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "holdtime"))
	{
		SetHoldTime(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "messagetime"))
	{
		SetMessageTime(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "loadtime"))
	{
		SetLoadTime(atof(pkvd->szValue));
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

void CRevertSaved::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	util::ScreenFadeAll(pev->rendercolor, Duration(), HoldTime(), pev->renderamt, FFADE_OUT);
	pev->nextthink = gpGlobals->time + MessageTime();
	SetThink(&CRevertSaved::MessageThink);
}


void CRevertSaved::MessageThink()
{
	util::ShowMessageAll(STRING(pev->message));
	float nextThink = LoadTime() - MessageTime();
	if (nextThink > 0)
	{
		pev->nextthink = gpGlobals->time + nextThink;
		SetThink(&CRevertSaved::LoadThink);
	}
	else
		LoadThink();
}


void CRevertSaved::LoadThink()
{
	if (!util::IsMultiplayer())
	{
		SERVER_COMMAND("reload\n");
	}
}


//=========================================================
// Multiplayer intermission spots.
//=========================================================
class CInfoIntermission : public CPointEntity
{
	bool Spawn() override;
	void Think() override;
};

bool CInfoIntermission::Spawn()
{
	SetOrigin(pev->origin);
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->v_angle = g_vecZero;

	pev->nextthink = gpGlobals->time + 2; // let targets spawn!

	return true;
}

void CInfoIntermission::Think()
{
	edict_t* pTarget;

	// find my target
	pTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target));

	if (!FNullEnt(pTarget))
	{
		pev->v_angle = util::VecToAngles((pTarget->v.origin - pev->origin).Normalize());
		pev->v_angle.x = -pev->v_angle.x;
	}
}

LINK_ENTITY_TO_CLASS(info_intermission, CInfoIntermission);
