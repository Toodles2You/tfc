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
unsigned short g_usTeleport;

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

		DEFINE_ARRAY(CBasePlayer, m_rgItems, FIELD_INTEGER, MAX_ITEMS),
		DEFINE_FIELD(CBasePlayer, m_afPhysicsFlags, FIELD_INTEGER),

		DEFINE_FIELD(CBasePlayer, m_flTimeWeaponIdle, FIELD_TIME),
		DEFINE_FIELD(CBasePlayer, m_flSwimTime, FIELD_TIME),
		DEFINE_FIELD(CBasePlayer, m_flDuckTime, FIELD_TIME),
		DEFINE_FIELD(CBasePlayer, m_flWallJumpTime, FIELD_TIME),

		DEFINE_FIELD(CBasePlayer, m_flSuitUpdate, FIELD_TIME),
		DEFINE_ARRAY(CBasePlayer, m_rgSuitPlayList, FIELD_INTEGER, CSUITPLAYLIST),
		DEFINE_FIELD(CBasePlayer, m_iSuitPlayNext, FIELD_INTEGER),
		DEFINE_ARRAY(CBasePlayer, m_rgiSuitNoRepeat, FIELD_INTEGER, CSUITNOREPEAT),
		DEFINE_ARRAY(CBasePlayer, m_rgflSuitNoRepeatTime, FIELD_TIME, CSUITNOREPEAT),
		DEFINE_FIELD(CBasePlayer, m_lastDamageAmount, FIELD_INTEGER),

		DEFINE_ARRAY(CBasePlayer, m_rgpPlayerItems, FIELD_CLASSPTR, MAX_WEAPONS),
		DEFINE_FIELD(CBasePlayer, m_pActiveItem, FIELD_CLASSPTR),
		DEFINE_FIELD(CBasePlayer, m_WeaponBits, FIELD_INT64),

		DEFINE_ARRAY(CBasePlayer, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS),
		DEFINE_FIELD(CBasePlayer, m_idrowndmg, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_idrownrestored, FIELD_INTEGER),

		DEFINE_FIELD(CBasePlayer, m_iTrain, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_bitsHUDDamage, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_flFallVelocity, FIELD_FLOAT),
		DEFINE_FIELD(CBasePlayer, m_fLongJump, FIELD_BOOLEAN),
		DEFINE_FIELD(CBasePlayer, m_fInitHUD, FIELD_BOOLEAN),
		DEFINE_FIELD(CBasePlayer, m_tbdPrev, FIELD_TIME),

		DEFINE_FIELD(CBasePlayer, m_pTank, FIELD_EHANDLE),
		DEFINE_FIELD(CBasePlayer, m_hViewEntity, FIELD_EHANDLE),
		DEFINE_FIELD(CBasePlayer, m_iHideHUD, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_iFOV, FIELD_INTEGER),

		DEFINE_FIELD(CBasePlayer, m_SndRoomtype, FIELD_INTEGER),

		DEFINE_FIELD(CBasePlayer, m_flStartCharge, FIELD_TIME),
};

LINK_ENTITY_TO_CLASS(player, CBasePlayer);


void CBasePlayer::Pain()
{
	float flRndSound; //sound randomizer

	flRndSound = RANDOM_FLOAT(0, 1);

	if (flRndSound <= 0.33)
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM);
	else if (flRndSound <= 0.66)
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM);
	else
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM);
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
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/h2odeath.wav", 1, ATTN_NONE);
		return;
	}
	*/

	// temporarily using pain sounds for death sounds
	switch (RANDOM_LONG(1, 5))
	{
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM);
		break;
	case 3:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM);
		break;
	}

	// play one of the suit death alarms
	EMIT_GROUPNAME_SUIT(ENT(pev), "HEV_DEAD");
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
void CBasePlayer::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
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
			flDamage *= gSkillData.plrHead;
			break;
		case HITGROUP_CHEST:
			flDamage *= gSkillData.plrChest;
			break;
		case HITGROUP_STOMACH:
			flDamage *= gSkillData.plrStomach;
			break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			flDamage *= gSkillData.plrArm;
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			flDamage *= gSkillData.plrLeg;
			break;
		default:
			break;
		}
	}

	AddMultiDamage(pevAttacker, this, flDamage, bitsDamageType);
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

bool CBasePlayer::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if (pev->takedamage == DAMAGE_NO || !IsAlive())
	{
		return false;
	}
	CBaseEntity* pAttacker = CBaseEntity::Instance(pevAttacker);
	if (!g_pGameRules->FPlayerCanTakeDamage(this, pAttacker))
	{
		return false;
	}

	m_lastDamageAmount = flDamage;
	m_bitsDamageType |= bitsDamageType;
	m_bitsHUDDamage = -1;

	float flArmour = 0.0f;
	if ((bitsDamageType & DMG_ARMOR_PIERCING) == 0)
	{
		float armourBonus = kArmourBonus;
		if ((bitsDamageType & DMG_BLAST) != 0 && UTIL_IsDeathmatch())
		{
			armourBonus *= 2;
		}
		flArmour = ArmourBonus(flDamage, pev->armorvalue, kArmourRatio, armourBonus);
	}

	// Grab the vector of the incoming attack. (Pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit.)
	if (!FNullEnt(pevInflictor))
	{
		if (FNullEnt(pevAttacker) || pevAttacker->solid != SOLID_TRIGGER)
		{
			CBaseEntity* pInflictor = CBaseEntity::Instance(pevInflictor);
			if (pInflictor)
			{
				// Move them around!
				g_vecAttackDir = (pInflictor->Center() - Vector(0, 0, 10) - Center()).Normalize();

				pev->velocity = pev->velocity + g_vecAttackDir * -DamageForce(pev, flDamage);
			}
		}
		pev->dmg_inflictor = ENT(pevInflictor);
	}

	// Check for godmode or invincibility.
	if ((pev->flags & FL_GODMODE) != 0)
	{
		return false;
	}

	// Do the damage!
	pev->dmg_take += flDamage;
	pev->health -= flDamage;
	pev->armorvalue -= flArmour;

	if (!FNullEnt(pevAttacker) && (pevAttacker->flags & FL_CLIENT) != 0)
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
		if (pevAttacker == pev)
		{
			flags |= kDamageFlagSelf;
		}

		MESSAGE_BEGIN(MSG_ONE, gmsgHitFeedback, nullptr, pevAttacker);
		WRITE_BYTE(entindex());
		WRITE_BYTE(flags);
		WRITE_SHORT(flDamage);
		MESSAGE_END();
	}

	if (pev->health <= 0)
	{
		Killed(pevInflictor, pevAttacker, bitsDamageType);
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

	MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
		WRITE_BYTE(9);							  // command length in bytes
		WRITE_BYTE(DRC_CMD_EVENT);				  // take damage event
		WRITE_SHORT(ENTINDEX(this->edict()));	  // index number of primary entity
		WRITE_SHORT(ENTINDEX(ENT(pevInflictor))); // index number of secondary entity
		WRITE_LONG(5);							  // eventflags (priority and flags)
	MESSAGE_END();

	return true;
}

//=========================================================
// PackDeadPlayerItems - call this when a player dies to
// pack up the appropriate weapons and ammo items, and to
// destroy anything that shouldn't be packed.
//
// This is pretty brute force :(
//=========================================================
void CBasePlayer::PackDeadPlayerItems()
{
	int iWeaponRules;
	int iAmmoRules;
	int i;
	CBasePlayerWeapon* rgpPackWeapons[MAX_WEAPONS];
	int iPackAmmo[MAX_AMMO_SLOTS + 1];
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
		RemoveAllItems(true);
		return;
	}

	// go through all of the weapons and make a list of the ones to pack
	for (auto pPlayerItem : m_lpPlayerItems)
	{
		switch (iWeaponRules)
		{
		case GR_PLR_DROP_GUN_ACTIVE:
			if (m_pActiveItem && pPlayerItem == m_pActiveItem)
			{
				// this is the active item. Pack it.
				rgpPackWeapons[iPW++] = (CBasePlayerWeapon*)pPlayerItem;
			}
			break;

		case GR_PLR_DROP_GUN_ALL:
			rgpPackWeapons[iPW++] = (CBasePlayerWeapon*)pPlayerItem;
			break;

		default:
			break;
		}
	}

	// now go through ammo and make a list of which types to pack.
	if (iAmmoRules != GR_PLR_DROP_AMMO_NO)
	{
		for (i = 0; i < MAX_AMMO_SLOTS; i++)
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
					if (m_pActiveItem && i == m_pActiveItem->iAmmo1())
					{
						// this is the primary ammo type for the active weapon
						iPackAmmo[iPA++] = i;
					}
					else if (m_pActiveItem && i == m_pActiveItem->iAmmo2())
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

	// now pack all of the items in the lists
	while (rgpPackWeapons[iPW])
	{
		// weapon unhooked from the player. Pack it into der box.
		pWeaponBox->PackWeapon(rgpPackWeapons[iPW]);

		iPW++;
	}

	pWeaponBox->pev->velocity = pev->velocity * 1.2; // weaponbox has player's velocity, then some.

	RemoveAllItems(true); // now strip off everything that wasn't handled by the code above.
}

void CBasePlayer::RemoveAllItems(bool removeSuit)
{
	if (m_pActiveItem)
	{
		m_pActiveItem->Holster();
		m_pActiveItem = NULL;
	}

	if (m_pTank != NULL)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	for (auto pPendingItem : m_lpPlayerItems)
	{
		pPendingItem->Drop();
	}
	m_pActiveItem = nullptr;
	memset(m_rgpPlayerItems, 0, sizeof(m_rgpPlayerItems));
	m_lpPlayerItems.clear();

	pev->viewmodel = 0;
	pev->weaponmodel = 0;

	m_WeaponBits = 0ULL;

	//Re-add suit bit if needed.
	SetHasSuit(!removeSuit);

	for (int i = 0; i < MAX_AMMO_SLOTS; i++)
	{
		m_rgAmmo[i] = 0;
	}

	UpdateClientData();
}


void CBasePlayer::Killed(entvars_t* pevInflictor, entvars_t* pevAttacker, int bitsDamageType)
{
	// Holster weapon immediately, to allow it to cleanup
	if (m_pActiveItem)
	{
		m_pActiveItem->Holster();
	}

	g_pGameRules->PlayerKilled(this, pevAttacker, pevInflictor, bitsDamageType);

	if (m_pTank != NULL)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	SetAnimation(PLAYER_DIE);

	m_fDeadTime = gpGlobals->time;
	m_fNextSuicideTime = gpGlobals->time + 1.0f;

	pev->deadflag = DEAD_DYING;

	SetSuitUpdate(NULL, false, 0);

	m_iFOV = 0;

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
		UTIL_VecToAngles(g_vecAttackDir),
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

	if ((pev->flags & FL_FROZEN) != 0)
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

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
		if (!FBitSet(pev->flags, FL_ONGROUND) && (m_Activity == ACT_HOP || m_Activity == ACT_LEAP)) // Still jumping
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
		if (FBitSet(pev->flags, FL_DUCKING)) // crouching
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
			if (FBitSet(pev->flags, FL_DUCKING)) // crouching
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

	if (FBitSet(pev->flags, FL_DUCKING))
	{
		if (speed == 0)
		{
			pev->gaitsequence = LookupActivity(ACT_CROUCHIDLE);
			// pev->gaitsequence	= LookupActivity( ACT_CROUCH );
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
		// pev->gaitsequence	= LookupActivity( ACT_WALK );
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

	UTIL_MakeVectors(pev->angles);
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
		UTIL_TraceHull(vecSrc, vecSrc + gpGlobals->v_forward * 64, dont_ignore_monsters, head_hull, edict(), &tr);

		if (tr.flFraction != 1.0)
		{
			deathActivity = ACT_DIESIMPLE;
		}
	}

	if (deathActivity == ACT_DIEBACKWARD)
	{
		// make sure there's room to fall backward
		UTIL_TraceHull(vecSrc, vecSrc - gpGlobals->v_forward * 64, dont_ignore_monsters, head_hull, edict(), &tr);

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
	UTIL_MakeVectors(pev->angles);
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
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_wade1.wav", 1, ATTN_NORM);
		else if (pev->air_finished < gpGlobals->time + 9)
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_wade2.wav", 1, ATTN_NORM);

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

				TakeDamage(CWorld::World->pev, CWorld::World->pev, pev->dmg, DMG_DROWN);
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
		if (FBitSet(pev->flags, FL_INWATER))
		{
			ClearBits(pev->flags, FL_INWATER);
		}
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
				EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim1.wav", 0.8, ATTN_NORM);
				break;
			case 1:
				EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim2.wav", 0.8, ATTN_NORM);
				break;
			case 2:
				EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim3.wav", 0.8, ATTN_NORM);
				break;
			case 3:
				EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim4.wav", 0.8, ATTN_NORM);
				break;
			}
		}
	}

	if (pev->watertype == CONTENT_LAVA) // do damage
	{
		if (pev->dmgtime < gpGlobals->time)
			TakeDamage(CWorld::World->pev, CWorld::World->pev, 10 * pev->waterlevel, DMG_BURN);
	}
	else if (pev->watertype == CONTENT_SLIME) // do damage
	{
		pev->dmgtime = gpGlobals->time + 1;
		TakeDamage(CWorld::World->pev, CWorld::World->pev, 4 * pev->waterlevel, DMG_ACID);
	}

	if (!FBitSet(pev->flags, FL_INWATER))
	{
		SetBits(pev->flags, FL_INWATER);
		pev->dmgtime = 0;
	}
}


// true if the player is attached to a ladder
bool CBasePlayer::IsOnLadder()
{
	return (pev->movetype == MOVETYPE_FLY);
}

void CBasePlayer::PlayerDeathFrame()
{
	const auto bIsMultiplayer = UTIL_IsMultiplayer();

	if (HasWeapons())
	{
		PackDeadPlayerItems();
	}

	// If the player has been dead for 5 seconds,
	// send the player off to an intermission
	// camera until they respawn.
	if (bIsMultiplayer && (m_afPhysicsFlags & PFLAG_OBSERVER) == 0 && (m_fDeadTime + 5.0f) <= gpGlobals->time)
	{
		StartDeathCam();
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

//=========================================================
// StartDeathCam - find an intermission spot and send the
// player off into observer mode
//=========================================================
void CBasePlayer::StartDeathCam()
{
	edict_t *pSpot, *pNewSpot;
	int iRand;

	if (pev->view_ofs == g_vecZero)
	{
		// don't accept subsequent attempts to StartDeathCam()
		return;
	}

	pSpot = FIND_ENTITY_BY_CLASSNAME(NULL, "info_intermission");

	if (!FNullEnt(pSpot))
	{
		// at least one intermission spot in the world.
		iRand = RANDOM_LONG(0, 3);

		while (iRand > 0)
		{
			pNewSpot = FIND_ENTITY_BY_CLASSNAME(pSpot, "info_intermission");

			if (pNewSpot)
			{
				pSpot = pNewSpot;
			}

			iRand--;
		}

		UTIL_SetOrigin(pev, pSpot->v.origin);
		pev->angles = pev->v_angle = pSpot->v.v_angle;
	}
	else
	{
		// no intermission spot. Push them up in the air, looking down at their corpse
		TraceResult tr;
		UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, 128), ignore_monsters, edict(), &tr);

		UTIL_SetOrigin(pev, tr.vecEndPos);
		pev->angles = pev->v_angle = UTIL_VecToAngles(tr.vecEndPos - pev->origin);
	}

	// start death cam

	m_afPhysicsFlags |= PFLAG_OBSERVER;
	pev->view_ofs = g_vecZero;
	pev->fixangle = 1;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	pev->modelindex = 0;
}

void CBasePlayer::StartObserver(Vector vecPosition, Vector vecViewAngle)
{
	// clear any clientside entities attached to this player
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_KILLPLAYERATTACHMENTS);
	WRITE_BYTE((byte)entindex());
	MESSAGE_END();

	// Holster weapon immediately, to allow it to cleanup
	if (m_pActiveItem)
		m_pActiveItem->Holster();

	if (m_pTank != NULL)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(NULL, false, 0);

	// reset FOV
	m_iFOV = 0;

	// Setup flags
	m_iHideHUD = (HIDEHUD_HEALTH | HIDEHUD_WEAPONS);
	m_afPhysicsFlags |= PFLAG_OBSERVER;
	pev->effects = EF_NODRAW;
	pev->view_ofs = g_vecZero;
	pev->angles = pev->v_angle = vecViewAngle;
	pev->fixangle = 1;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	ClearBits(m_afPhysicsFlags, PFLAG_DUCKING);
	ClearBits(pev->flags, FL_DUCKING);
	pev->deadflag = DEAD_RESPAWNABLE;
	pev->health = 1;

	// Clear out the status bar
	m_fInitHUD = true;

	pev->team = 0;
	MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
	WRITE_BYTE(ENTINDEX(edict()));
	WRITE_STRING("");
	MESSAGE_END();

	// Remove all the player's stuff
	RemoveAllItems(false);

	// Move them to the new position
	UTIL_SetOrigin(pev, vecPosition);

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

				if (pTrain && (pev->button & IN_JUMP) == 0 && FBitSet(pev->flags, FL_ONGROUND) && (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) != 0 && pTrain->OnControls(pev))
				{
					m_afPhysicsFlags |= PFLAG_ONTRAIN;
					m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
					m_iTrain |= TRAIN_NEW;
					EMIT_SOUND(ENT(pev), CHAN_ITEM, "plats/train_use1.wav", 0.8, ATTN_NORM);
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

	UTIL_MakeVectors(pev->v_angle); // so we know which way we are facing

	while ((pObject = UTIL_FindEntityInSphere(pObject, pev->origin, PLAYER_SEARCH_RADIUS)) != NULL)
	{

		if ((pObject->ObjectCaps() & (FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE)) != 0)
		{
			// !!!PERFORMANCE- should this check be done on a per case basis AFTER we've determined that
			// this object is actually usable? This dot is being done for every object within PLAYER_SEARCH_RADIUS
			// when player hits the use key. How many objects can be in that area, anyway? (sjb)
			vecLOS = (VecBModelOrigin(pObject->pev) - (pev->origin + pev->view_ofs));

			// This essentially moves the origin of the target to the corner nearest the player to test to see
			// if it's "hull" is in the view cone
			vecLOS = UTIL_ClampVectorToBox(vecLOS, pObject->pev->size * 0.5);

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
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/wpn_select.wav", 0.4, ATTN_NORM);

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
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/wpn_denyselect.wav", 0.4, ATTN_NORM);
	}
}



void CBasePlayer::Jump()
{
	Vector vecWallCheckDir; // direction we're tracing a line to find a wall when walljumping
	Vector vecAdjustedVelocity;
	Vector vecSpot;
	TraceResult tr;

	if (FBitSet(pev->flags, FL_WATERJUMP))
		return;

	if (pev->waterlevel >= 2)
	{
		return;
	}

	// jump velocity is sqrt( height * gravity * 2)

	// If this isn't the first frame pressing the jump button, break out.
	if (!FBitSet(m_afButtonPressed, IN_JUMP))
		return; // don't pogo stick

	if ((pev->flags & FL_ONGROUND) == 0 || !pev->groundentity)
	{
		return;
	}

	// many features in this function use v_forward, so makevectors now.
	UTIL_MakeVectors(pev->angles);

	// ClearBits(pev->flags, FL_ONGROUND);		// don't stairwalk

	SetAnimation(PLAYER_JUMP);

	if (m_fLongJump &&
		(pev->button & IN_DUCK) != 0 &&
		(pev->flDuckTime > 0) &&
		pev->velocity.Length() > 50)
	{
		SetAnimation(PLAYER_SUPERJUMP);
	}

	// If you're standing on a conveyor, add it's velocity to yours (for momentum)
	entvars_t* pevGround = VARS(pev->groundentity);
	if (pevGround && (pevGround->flags & FL_CONVEYOR) != 0)
	{
		pev->velocity = pev->velocity + pev->basevelocity;
	}
}


void CBasePlayer::Duck()
{
	if ((pev->button & IN_DUCK) != 0)
	{
		if (m_IdealActivity != ACT_LEAP)
		{
			SetAnimation(PLAYER_WALK);
		}
	}
}

//
// ID's player as such.
//
int CBasePlayer::Classify()
{
	return CLASS_PLAYER;
}


void CBasePlayer::AddPoints(int score, bool bAllowNegativeScore)
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

	MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
	WRITE_BYTE(ENTINDEX(edict()));
	WRITE_SHORT(pev->frags);
	WRITE_SHORT(m_iDeaths);
	WRITE_SHORT(0);
	WRITE_SHORT(TeamNumber());
	MESSAGE_END();
}


void CBasePlayer::AddPointsToTeam(int score, bool bAllowNegativeScore)
{
	int index = entindex();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex(i);

		if (pPlayer && i != index)
		{
			if (g_pGameRules->PlayerRelationship(this, pPlayer) == GR_TEAMMATE)
			{
				pPlayer->AddPoints(score, bAllowNegativeScore);
			}
		}
	}
}

//Player ID
void CBasePlayer::InitStatusBar()
{
	m_flStatusBarDisappearDelay = 0;
	m_SbarString1[0] = m_SbarString0[0] = 0;
}

void CBasePlayer::UpdateStatusBar()
{
	int newSBarState[SBAR_END];
	char sbuf0[SBAR_STRING_SIZE];
	char sbuf1[SBAR_STRING_SIZE];

	memset(newSBarState, 0, sizeof(newSBarState));
	strcpy(sbuf0, m_SbarString0);
	strcpy(sbuf1, m_SbarString1);

	// Find an ID Target
	TraceResult tr;
	UTIL_MakeVectors(pev->v_angle + pev->punchangle);
	Vector vecSrc = EyePosition();
	Vector vecEnd = vecSrc + (gpGlobals->v_forward * MAX_ID_RANGE);
	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, edict(), &tr);

	if (tr.flFraction != 1.0)
	{
		if (!FNullEnt(tr.pHit))
		{
			CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

			if (pEntity->Classify() == CLASS_PLAYER)
			{
				newSBarState[SBAR_ID_TARGETNAME] = ENTINDEX(pEntity->edict());
				strcpy(sbuf1, "1 %p1\n2 Health: %i2%%\n3 Armor: %i3%%");

				// allies and medics get to see the targets health
				if (g_pGameRules->PlayerRelationship(this, pEntity) == GR_TEAMMATE)
				{
					newSBarState[SBAR_ID_TARGETHEALTH] = 100 * (pEntity->pev->health / pEntity->pev->max_health);
					newSBarState[SBAR_ID_TARGETARMOR] = pEntity->pev->armorvalue; //No need to get it % based since 100 it's the max.
				}

				m_flStatusBarDisappearDelay = gpGlobals->time + 1.0;
			}
		}
		else if (m_flStatusBarDisappearDelay > gpGlobals->time)
		{
			// hold the values for a short amount of time after viewing the object
			newSBarState[SBAR_ID_TARGETNAME] = m_izSBarState[SBAR_ID_TARGETNAME];
			newSBarState[SBAR_ID_TARGETHEALTH] = m_izSBarState[SBAR_ID_TARGETHEALTH];
			newSBarState[SBAR_ID_TARGETARMOR] = m_izSBarState[SBAR_ID_TARGETARMOR];
		}
	}

	bool bForceResend = false;

	if (0 != strcmp(sbuf0, m_SbarString0))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgStatusText, NULL, pev);
		WRITE_BYTE(0);
		WRITE_STRING(sbuf0);
		MESSAGE_END();

		strcpy(m_SbarString0, sbuf0);

		// make sure everything's resent
		bForceResend = true;
	}

	if (0 != strcmp(sbuf1, m_SbarString1))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgStatusText, NULL, pev);
		WRITE_BYTE(1);
		WRITE_STRING(sbuf1);
		MESSAGE_END();

		strcpy(m_SbarString1, sbuf1);

		// make sure everything's resent
		bForceResend = true;
	}

	// Check values and send if they don't match
	for (int i = 1; i < SBAR_END; i++)
	{
		if (newSBarState[i] != m_izSBarState[i] || bForceResend)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgStatusValue, NULL, pev);
			WRITE_BYTE(i);
			WRITE_SHORT(newSBarState[i]);
			MESSAGE_END();

			m_izSBarState[i] = newSBarState[i];
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

	UTIL_MakeVectors(pev->v_angle); // is this still used?

	ItemPreFrame();
	WaterMove();

	if (m_bResetViewEntity)
	{
		m_bResetViewEntity = false;

		CBaseEntity* viewEntity = m_hViewEntity;

		if (viewEntity)
		{
			SET_VIEW(edict(), viewEntity->edict());
		}
	}

	// JOHN: checks if new client data (for HUD and view control) needs to be sent to the client
	UpdateClientData();

	CheckTimeBasedDamage();

	CheckSuitUpdate();

	// Observer Button Handling
	if (IsObserver())
	{
		Observer_HandleButtons();
		Observer_CheckTarget();
		Observer_CheckProperties();
		pev->impulse = 0;
		return;
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
			UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -38), ignore_monsters, ENT(pev), &trainTrace);

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
		else if (!FBitSet(pev->flags, FL_ONGROUND) || FBitSet(pTrain->pev->spawnflags, SF_TRACKTRAIN_NOCONTROL) || (pev->button & (IN_MOVELEFT | IN_MOVERIGHT)) != 0)
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

	if ((pev->button & IN_JUMP) != 0)
	{
		// If on a ladder, jump off the ladder
		// else Jump
		Jump();
	}


	// If trying to duck, already ducked, or in the process of ducking
	if ((pev->button & IN_DUCK) != 0 || FBitSet(pev->flags, FL_DUCKING) || (m_afPhysicsFlags & PFLAG_DUCKING) != 0)
		Duck();

	if (!FBitSet(pev->flags, FL_ONGROUND))
	{
		m_flFallVelocity = -pev->velocity.z;
	}

	if ((m_afPhysicsFlags & PFLAG_ONBARNACLE) != 0)
	{
		pev->velocity = g_vecZero;
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
				// UNDONE - flag movement as half-speed
				bDuration = PARALYZE_DURATION;
				break;
			case itbd_NerveGas:
				//				TakeDamage(pev, pev, NERVEGAS_DAMAGE, DMG_GENERIC);
				bDuration = NERVEGAS_DURATION;
				break;
			case itbd_Poison:
				TakeDamage(pev, pev, POISON_DAMAGE, DMG_GENERIC);
				bDuration = POISON_DURATION;
				break;
			case itbd_Radiation:
				//				TakeDamage(pev, pev, RADIATION_DAMAGE, DMG_GENERIC);
				bDuration = RADIATION_DURATION;
				break;
			case itbd_DrownRecover:
				// NOTE: this hack is actually used to RESTORE health
				// after the player has been drowning and finally takes a breath
				if (m_idrowndmg > m_idrownrestored)
				{
					int idif = V_min(m_idrowndmg - m_idrownrestored, 10);

					TakeHealth(idif, DMG_GENERIC);
					m_idrownrestored += idif;
				}
				bDuration = 4; // get up to 5*10 = 50 points back
				break;
			case itbd_Acid:
				//				TakeDamage(pev, pev, ACID_DAMAGE, DMG_GENERIC);
				bDuration = ACID_DURATION;
				break;
			case itbd_SlowBurn:
				//				TakeDamage(pev, pev, SLOWBURN_DAMAGE, DMG_GENERIC);
				bDuration = SLOWBURN_DURATION;
				break;
			case itbd_SlowFreeze:
				//				TakeDamage(pev, pev, SLOWFREEZE_DAMAGE, DMG_GENERIC);
				bDuration = SLOWFREEZE_DURATION;
				break;
			default:
				bDuration = 0;
			}

			if (0 != m_rgbTimeBasedDamage[i])
			{
				// use up an antitoxin on poison or nervegas after a few seconds of damage
				if (((i == itbd_NerveGas) && (m_rgbTimeBasedDamage[i] < NERVEGAS_DURATION)) ||
					((i == itbd_Poison) && (m_rgbTimeBasedDamage[i] < POISON_DURATION)))
				{
					if (0 != m_rgItems[ITEM_ANTIDOTE])
					{
						m_rgbTimeBasedDamage[i] = 0;
						m_rgItems[ITEM_ANTIDOTE]--;
						SetSuitUpdate("!HEV_HEAL4", false, SUIT_REPEAT_OK);
					}
				}


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


// if in range of radiation source, ping geiger counter

#define GEIGERDELAY 0.25

void CBasePlayer::UpdateGeigerCounter()
{
	byte range;

	// delay per update ie: don't flood net with these msgs
	if (gpGlobals->time < m_flgeigerDelay)
		return;

	m_flgeigerDelay = gpGlobals->time + GEIGERDELAY;

	// send range to radition source to client

	range = (byte)(m_flgeigerRange / 4);

	if (range != m_igeigerRangePrev)
	{
		m_igeigerRangePrev = range;

		MESSAGE_BEGIN(MSG_ONE, gmsgGeigerRange, NULL, pev);
		WRITE_BYTE(range);
		MESSAGE_END();
	}

	// reset counter and semaphore
	if (!RANDOM_LONG(0, 3))
		m_flgeigerRange = 1000;
}

/*
================
CheckSuitUpdate

Play suit update if it's time
================
*/

#define SUITUPDATETIME 3.5
#define SUITFIRSTUPDATETIME 0.1

void CBasePlayer::CheckSuitUpdate()
{
	int i;
	int isentence = 0;
	int isearch = m_iSuitPlayNext;

	// Ignore suit updates if no suit
	if (!HasSuit())
		return;

	// if in range of radiation source, ping geiger counter
	UpdateGeigerCounter();

	if (UTIL_IsDeathmatch())
	{
		// don't bother updating HEV voice in multiplayer.
		return;
	}

	if (gpGlobals->time >= m_flSuitUpdate && m_flSuitUpdate > 0)
	{
		// play a sentence off of the end of the queue
		for (i = 0; i < CSUITPLAYLIST; i++)
		{
			isentence = m_rgSuitPlayList[isearch];
			if (0 != isentence)
				break;

			if (++isearch == CSUITPLAYLIST)
				isearch = 0;
		}

		if (0 != isentence)
		{
			m_rgSuitPlayList[isearch] = 0;
			if (isentence > 0)
			{
				// play sentence number

				char sentence[CBSENTENCENAME_MAX + 1];
				strcpy(sentence, "!");
				strcat(sentence, gszallsentencenames[isentence]);
				EMIT_SOUND_SUIT(ENT(pev), sentence);
			}
			else
			{
				// play sentence group
				EMIT_GROUPID_SUIT(ENT(pev), -isentence);
			}
			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME;
		}
		else
			// queue is empty, don't check
			m_flSuitUpdate = 0;
	}
}

// add sentence to suit playlist queue. if fgroup is true, then
// name is a sentence group (HEV_AA), otherwise name is a specific
// sentence name ie: !HEV_AA0.  If iNoRepeat is specified in
// seconds, then we won't repeat playback of this word or sentence
// for at least that number of seconds.

void CBasePlayer::SetSuitUpdate(const char* name, bool fgroup, int iNoRepeatTime)
{
	int i;
	int isentence;
	int iempty = -1;


	// Ignore suit updates if no suit
	if (!HasSuit())
		return;

	if (UTIL_IsDeathmatch())
	{
		// due to static channel design, etc. We don't play HEV sounds in multiplayer right now.
		return;
	}

	// if name == NULL, then clear out the queue

	if (!name)
	{
		for (i = 0; i < CSUITPLAYLIST; i++)
			m_rgSuitPlayList[i] = 0;
		return;
	}
	// get sentence or group number
	if (!fgroup)
	{
		isentence = SENTENCEG_Lookup(name, NULL);
		if (isentence < 0)
			return;
	}
	else
		// mark group number as negative
		isentence = -SENTENCEG_GetIndex(name);

	// check norepeat list - this list lets us cancel
	// the playback of words or sentences that have already
	// been played within a certain time.

	for (i = 0; i < CSUITNOREPEAT; i++)
	{
		if (isentence == m_rgiSuitNoRepeat[i])
		{
			// this sentence or group is already in
			// the norepeat list

			if (m_rgflSuitNoRepeatTime[i] < gpGlobals->time)
			{
				// norepeat time has expired, clear it out
				m_rgiSuitNoRepeat[i] = 0;
				m_rgflSuitNoRepeatTime[i] = 0.0;
				iempty = i;
				break;
			}
			else
			{
				// don't play, still marked as norepeat
				return;
			}
		}
		// keep track of empty slot
		if (0 == m_rgiSuitNoRepeat[i])
			iempty = i;
	}

	// sentence is not in norepeat list, save if norepeat time was given

	if (0 != iNoRepeatTime)
	{
		if (iempty < 0)
			iempty = RANDOM_LONG(0, CSUITNOREPEAT - 1); // pick random slot to take over
		m_rgiSuitNoRepeat[iempty] = isentence;
		m_rgflSuitNoRepeatTime[iempty] = iNoRepeatTime + gpGlobals->time;
	}

	// find empty spot in queue, or overwrite last spot

	m_rgSuitPlayList[m_iSuitPlayNext++] = isentence;
	if (m_iSuitPlayNext == CSUITPLAYLIST)
		m_iSuitPlayNext = 0;

	if (m_flSuitUpdate <= gpGlobals->time)
	{
		if (m_flSuitUpdate == 0)
			// play queue is empty, don't delay too long before playback
			m_flSuitUpdate = gpGlobals->time + SUITFIRSTUPDATETIME;
		else
			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME;
	}
}

void CBasePlayer::CheckAmmoLevel(CBasePlayerItem* pItem, bool bPrimary)
{
	if (UTIL_IsDeathmatch())
	{
		return;
	}
	
	if (bPrimary)
	{
		if (pItem->iAmmo1() > WEAPON_NONE && static_cast<CBasePlayerWeapon*>(pItem)->m_iClip == 0 && m_rgAmmo[pItem->iAmmo1()] <= 0)
		{
			SetSuitUpdate("!HEV_AMO0", false, 0);
		}
	}
	else
	{
		if (pItem->iAmmo2() > WEAPON_NONE && m_rgAmmo[pItem->iAmmo2()] == 0)
		{
			SetSuitUpdate("!HEV_AMO0", false, 0);
		}
	}
}


void CBasePlayer::PostThink()
{
	if (g_fGameOver)
		goto pt_end; // intermission or finale

	if (!IsAlive())
		goto pt_end;

	// Handle Tank controlling
	if (m_pTank != NULL)
	{ // if they've moved too far from the gun,  or selected a weapon, unuse the gun
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

	// do weapon stuff
	ItemPostFrame();

	// check to see if player landed hard enough to make a sound
	// falling farther than half of the maximum safe distance, but not as far a max safe distance will
	// play a bootscrape sound, and no damage will be inflicted. Fallling a distance shorter than half
	// of maximum safe distance will make no sound. Falling farther than max safe distance will play a
	// fallpain sound, and damage will be inflicted based on how far the player fell

	if ((FBitSet(pev->flags, FL_ONGROUND)) && (pev->health > 0) && m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD)
	{
		// ALERT ( at_console, "%f\n", m_flFallVelocity );

		if (pev->watertype == CONTENT_WATER)
		{
			// Did he hit the world or a non-moving entity?
			// BUG - this happens all the time in water, especially when
			// BUG - water has current force
			// if ( !pev->groundentity || VARS(pev->groundentity)->velocity.z == 0 )
			// EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade1.wav", 1, ATTN_NORM);
		}
		else if (m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED)
		{ // after this point, we start doing damage

			float flFallDamage = g_pGameRules->FlPlayerFallDamage(this);

			if (flFallDamage > pev->health)
			{ //splat
				// note: play on item channel because we play footstep landing on body channel
				EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/bodysplat.wav", 1, ATTN_NORM);
			}

			if (flFallDamage > 0)
			{
				TakeDamage(CWorld::World->pev, CWorld::World->pev, flFallDamage, DMG_FALL);
			}
		}

		SetAnimation(PLAYER_WALK);
	}

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		m_flFallVelocity = 0;
	}

	// select the proper animation for the player character
	if (0 == pev->velocity.x && 0 == pev->velocity.y)
	{
		SetAnimation(PLAYER_IDLE);
	}
	else if (FBitSet(pev->flags, FL_ONGROUND))
	{
		SetAnimation(PLAYER_WALK);
	}
	else if (pev->waterlevel > 1)
	{
		SetAnimation(PLAYER_WALK);
	}

pt_end:
	StudioFrameAdvance();

	const int msec = static_cast<int>(std::roundf(gpGlobals->frametime * 1000));

	float len = VectorNormalize(pev->punchangle);
	len -= (10.0 + len * 0.5) * gpGlobals->frametime;
	len = V_max(len, 0.0);
	VectorScale(pev->punchangle, len, pev->punchangle);

	CBasePlayerWeapon* gun;
	// Decay timers on weapons
	for (auto pPlayerItem : m_lpPlayerItems)
	{
		gun = pPlayerItem->GetWeaponPtr();

		if (!gun)
		{
			continue;
		}

		gun->m_iNextPrimaryAttack = V_max(gun->m_iNextPrimaryAttack - msec, -1100);
		gun->m_iNextSecondaryAttack = V_max(gun->m_iNextSecondaryAttack - msec, -1);
		gun->m_iTimeWeaponIdle = V_max(gun->m_iTimeWeaponIdle - msec, -1);

		gun->pev->fuser1 = V_max(gun->pev->fuser1 - gpGlobals->frametime, -0.001);

		gun->DecrementTimers(msec);
	}

	m_iNextAttack -= msec;
	if (m_iNextAttack < -1)
		m_iNextAttack = -1;

	if (m_flNextAmmoBurn != 1000)
	{
		m_flNextAmmoBurn -= gpGlobals->frametime;

		if (m_flNextAmmoBurn < -0.001)
			m_flNextAmmoBurn = -0.001;
	}

	if (m_flAmmoStartCharge != 1000)
	{
		m_flAmmoStartCharge -= gpGlobals->frametime;

		if (m_flAmmoStartCharge < -0.001)
			m_flAmmoStartCharge = -0.001;
	}

	// Track button info so we can detect 'pressed' and 'released' buttons next frame
	m_afButtonLast = pev->button;
}


void CBasePlayer::Spawn()
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
	m_fLongJump = false; // no longjump module.

	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "slj", "0");
	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "hl", "1");
	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "bj", UTIL_dtos1(sv_allowbunnyhopping.value != 0 ? 1 : 0));

	m_iFOV = 0;		   // init field of view.
	m_ClientSndRoomtype = -1;

	m_flNextDecalTime = 0; // let this player decal as soon as he spawns.

	m_flgeigerDelay = gpGlobals->time + 2.0; // wait a few seconds until user-defined message registrations
											 // are recieved by all clients

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

	SET_MODEL(ENT(pev), "models/player.mdl");
	pev->sequence = LookupActivity(ACT_IDLE);

	if (FBitSet(pev->flags, FL_DUCKING))
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);

	pev->view_ofs = VEC_VIEW;
	Precache();

	m_fInitHUD = true;
	m_iClientHideHUD = -1; // force this to be recalculated
	m_pClientActiveItem = NULL;

	// reset all ammo values to 0
	for (int i = 0; i < MAX_AMMO_SLOTS; i++)
	{
		m_rgAmmo[i] = 0;
		m_rgAmmoLast[i] = 0; // client ammo values also have to be reset  (the death hud clear messages does on the client side)
	}

	m_flNextChatTime = gpGlobals->time;

	g_pGameRules->PlayerSpawn(this);
}


void CBasePlayer::Precache()
{
	m_flgeigerRange = 1000;
	m_igeigerRangePrev = 1000;

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

	if (FBitSet(pev->flags, FL_DUCKING))
	{
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	}
	else
	{
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
	}

	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "hl", "1");

	if (m_fLongJump)
	{
		g_engfuncs.pfnSetPhysicsKeyValue(edict(), "slj", "1");
	}
	else
	{
		g_engfuncs.pfnSetPhysicsKeyValue(edict(), "slj", "0");
	}

	m_iNextAttack = 0;

	m_bResetViewEntity = true;

	m_bRestored = true;

	return status;
}


void CBasePlayer::SelectItem(const char* pstr)
{
	if (pstr == nullptr || pstr[0] == '\0')
	{
		return;
	}

	int iId = WEAPON_NONE;

	for (auto it : m_lpPlayerItems)
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

	SelectItem(iId);
}

void CBasePlayer::SelectItem(int iId)
{
	if (iId <= WEAPON_NONE || iId >= MAX_WEAPONS)
	{
		return;
	}

	auto pItem = m_rgpPlayerItems[iId];

	if (!pItem)
	{
		return;
	}

	if (pItem == m_pActiveItem)
	{
		return;
	}

	if ((!m_pActiveItem || m_pActiveItem->Holster()) && pItem->Deploy())
	{
		m_pActiveItem = pItem;
		m_pActiveItem->UpdateItemInfo();
	}
}


//==============================================
// HasWeapons - do I have any weapons at all?
//==============================================
bool CBasePlayer::HasWeapons()
{
	return (m_WeaponBits & ~(1ULL << WEAPON_SUIT)) != 0;
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

int CBasePlayer::TeamNumber()
{
	return pev->team;
}


//==============================================
// !!!UNDONE:ultra temporary SprayCan entity to apply
// decal frame at a time. For PreAlpha CD
//==============================================
class CSprayCan : public CBaseEntity
{
public:
	void Spawn(entvars_t* pevOwner);
	void Think() override;

	int ObjectCaps() override { return FCAP_DONT_SAVE; }
};

void CSprayCan::Spawn(entvars_t* pevOwner)
{
	pev->origin = pevOwner->origin + Vector(0, 0, 32);
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT(pevOwner);
	pev->frame = 0;

	pev->nextthink = gpGlobals->time + 0.1;
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/sprayer.wav", 1, ATTN_NORM);
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

	UTIL_MakeVectors(pev->angles);
	UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, &tr);

	// No customization present.
	if (nFrames == -1)
	{
		UTIL_DecalTrace(&tr, DECAL_LAMBDA6);
		UTIL_Remove(this);
	}
	else
	{
		UTIL_PlayerDecalTrace(&tr, playernum, pev->frame, true);
		// Just painted last custom frame.
		if (pev->frame++ >= (nFrames - 1))
			UTIL_Remove(this);
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

	for (int i = 0; i < MAX_AMMO_SLOTS; ++i)
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
	TraceResult tr; // UNDONE: kill me! This is temporary for PreAlpha CDs

	// Handle use events
	PlayerUse();

	int iImpulse = (int)pev->impulse;
	switch (iImpulse)
	{
	case 99:
		break;
	case 100:
		// temporary flashlight for level designers
		break;

	case 201: // paint decal

		if (gpGlobals->time < m_flNextDecalTime)
		{
			// too early!
			break;
		}

		UTIL_MakeVectors(pev->v_angle);
		UTIL_TraceLine(pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, ENT(pev), &tr);

		if (tr.flFraction != 1.0)
		{ // line hit something, so paint a decal
			m_flNextDecalTime = gpGlobals->time + decalfrequency.value;
			CSprayCan* pCan = GetClassPtr((CSprayCan*)NULL);
			pCan->Spawn(pev);
		}

		break;

	default:
		// check all of the cheat impulse commands now
		CheatImpulseCommands(iImpulse);
		break;
	}

	pev->impulse = 0;
}

//=========================================================
//=========================================================
void CBasePlayer::CheatImpulseCommands(int iImpulse)
{
	if (0 == g_psv_cheats->value)
	{
		return;
	}

	CBaseEntity* pEntity;
	TraceResult tr;

	switch (iImpulse)
	{
	case 76:
		break;

	case 101:
		gEvilImpulse101 = true;
		SetHasSuit(true);
		GiveNamedItem("weapon_crowbar");
		GiveNamedItem("weapon_9mmhandgun");
		GiveNamedItem("ammo_9mmclip");
		GiveNamedItem("weapon_shotgun");
		GiveNamedItem("ammo_buckshot");
		GiveNamedItem("weapon_9mmAR");
		GiveNamedItem("ammo_9mmAR");
		GiveNamedItem("ammo_ARgrenades");
		GiveNamedItem("weapon_handgrenade");
		GiveNamedItem("weapon_tripmine");
		GiveNamedItem("weapon_357");
		GiveNamedItem("ammo_357");
		GiveNamedItem("weapon_crossbow");
		GiveNamedItem("ammo_crossbow");
		GiveNamedItem("weapon_egon");
		GiveNamedItem("weapon_gauss");
		GiveNamedItem("ammo_gaussclip");
		GiveNamedItem("weapon_rpg");
		GiveNamedItem("ammo_rpgclip");
		GiveNamedItem("weapon_satchel");
		GiveNamedItem("weapon_snark");
		GiveNamedItem("weapon_hornetgun");
		gEvilImpulse101 = false;
		break;

	case 104:
		// Dump all of the global state varaibles (and global entity names)
		gGlobalState.DumpGlobals();
		break;

	case 106:
		// Give me the classname and targetname of this entity.
		pEntity = UTIL_FindEntityForward(this);
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
		UTIL_TraceLine(start, end, ignore_monsters, edict(), &tr);
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
	case 202: // Random blood splatter
		break;
	case 203: // remove creature.
		pEntity = UTIL_FindEntityForward(this);
		if (pEntity)
		{
			if (0 != pEntity->pev->takedamage)
				pEntity->SetThink(&CBaseEntity::SUB_Remove);
		}
		break;
	}
}

//
// Add a weapon to the player (Item == Weapon == Selectable Object)
//
bool CBasePlayer::AddPlayerItem(CBasePlayerItem* pItem)
{
	if (pItem->m_iId <= WEAPON_NONE || pItem->m_iId >= MAX_WEAPONS)
	{
		return false;
	}

	if (HasPlayerItem(pItem))
	{
		if (pItem->AddDuplicate(m_rgpPlayerItems[pItem->m_iId]))
		{
			g_pGameRules->PlayerGotWeapon(this, pItem);
			pItem->CheckRespawn();

			// Ugly hack to update clip w/o an update clip message.
			m_rgpPlayerItems[pItem->m_iId]->UpdateItemInfo();
			if (m_pActiveItem)
			{
				m_pActiveItem->UpdateItemInfo();
			}

			pItem->Kill();
		}
		else if (gEvilImpulse101)
		{
			// FIXME: Remove anyway for deathmatch testing.
			pItem->Kill();
		}
		return false;
	}

	if (pItem->CanAddToPlayer(this))
	{
		pItem->AddToPlayer(this);

		g_pGameRules->PlayerGotWeapon(this, pItem);
		pItem->CheckRespawn();

		// Add to the list before extracting ammo so weapon ownership checks work properly.
		m_rgpPlayerItems[pItem->m_iId] = pItem;
		m_lpPlayerItems.push_front(pItem);

		if (auto weapon = pItem->GetWeaponPtr(); weapon)
		{
			weapon->ExtractAmmo(weapon);

			// Don't show weapon pickup if we're spawning or if it's an exhaustible weapon (will show ammo pickup instead).
			if (!m_bIsSpawning && (weapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE) == 0)
			{
				MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pev);
				WRITE_BYTE(weapon->m_iId);
				MESSAGE_END();
			}
		}

		// Should we switch to this item?
		if (g_pGameRules->FShouldSwitchWeapon(this, pItem))
		{
			SwitchWeapon(pItem);
		}

		return true;
	}
	else if (gEvilImpulse101)
	{
		// FIXME: Remove anyway for deathmatch testing.
		pItem->Kill();
	}
	return false;
}


bool CBasePlayer::RemovePlayerItem(CBasePlayerItem* pItem)
{
	if (pItem->m_iId <= WEAPON_NONE || pItem->m_iId >= MAX_WEAPONS)
	{
		return false;
	}
	if (m_pActiveItem == pItem)
	{
		pItem->Holster();
		pItem->pev->nextthink = 0;
		pItem->SetThink(nullptr);
		m_pActiveItem = nullptr;
		pev->viewmodel = 0;
		pev->weaponmodel = 0;
	}
	m_rgpPlayerItems[pItem->m_iId] = nullptr;
	m_lpPlayerItems.remove(pItem);
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

	if (iType <= AMMO_NONE || iType >= MAX_AMMO_SLOTS)
		return -1;

	int iAdd = V_min(iCount, iMax - m_rgAmmo[iType]);
	if (iAdd < 1)
		return iType;

	// Toodles FIXME:
	// If this is an exhaustible weapon make sure the player has it.
	/*
	if (const auto& ammoType = CBasePlayerItem::AmmoInfoArray[i]; ammoType.WeaponName != nullptr)
	{
		if (!HasNamedPlayerItem(ammoType.WeaponName))
		{
			GiveNamedItem(ammoType.WeaponName, 0);
		}
	}
	*/

	m_rgAmmo[iType] += iAdd;


	if (0 != gmsgAmmoPickup) // make sure the ammo messages have been linked first
	{
		// Send the message that ammo has been picked up
		MESSAGE_BEGIN(MSG_ONE, gmsgAmmoPickup, NULL, pev);
		WRITE_BYTE(iType); // ammo ID
		WRITE_BYTE(iAdd);				  // amount
		MESSAGE_END();
	}

	return iType;
}


/*
============
ItemPreFrame

Called every frame by the player PreThink
============
*/
void CBasePlayer::ItemPreFrame()
{
	if (m_iNextAttack > 0)
	{
		return;
	}

	if (!m_pActiveItem)
		return;

	m_pActiveItem->ItemPreFrame();
}


/*
============
ItemPostFrame

Called every frame by the player PostThink
============
*/
void CBasePlayer::ItemPostFrame()
{
	ImpulseCommands();

	// check if the player is using a tank
	if (m_pTank != NULL)
		return;

	if (m_iNextAttack > 0)
	{
		return;
	}

	// check again if the player is using a tank if they started using it in PlayerUse
	if (m_pTank != NULL)
		return;

	if (!m_pActiveItem)
		return;

	m_pActiveItem->ItemPostFrame();
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

		MESSAGE_BEGIN(MSG_ONE, gmsgResetHUD, NULL, pev);
		WRITE_BYTE(0);
		MESSAGE_END();

		if (!m_fGameHUDInitialized)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgInitHUD, NULL, pev);
			MESSAGE_END();

			g_pGameRules->InitHUD(this);
			m_fGameHUDInitialized = true;

			m_iObserverLastMode = OBS_ROAMING;

			if (UTIL_IsMultiplayer())
			{
				FireTargets("game_playerjoin", this, this, USE_TOGGLE, 0);
			}
		}

		FireTargets("game_playerspawn", this, this, USE_TOGGLE, 0);

		InitStatusBar();
	}

	if (m_iHideHUD != m_iClientHideHUD)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgHideWeapon, NULL, pev);
		WRITE_BYTE(m_iHideHUD);
		MESSAGE_END();

		m_iClientHideHUD = m_iHideHUD;
	}

	if (m_WeaponBits != m_ClientWeaponBits)
	{
		m_ClientWeaponBits = m_WeaponBits;

		const int lowerBits = m_WeaponBits & 0xFFFFFFFF;
		const int upperBits = (m_WeaponBits >> 32) & 0xFFFFFFFF;

		MESSAGE_BEGIN(MSG_ONE, gmsgWeapons, nullptr, pev);
		WRITE_LONG(lowerBits);
		WRITE_LONG(upperBits);
		MESSAGE_END();
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

		MESSAGE_BEGIN(MSG_ONE, gmsgDamage, NULL, pev);
		WRITE_BYTE(pev->dmg_save);
		WRITE_BYTE(pev->dmg_take);
		WRITE_LONG(visibleDamageBits);
		WRITE_COORD(damageOrigin.x);
		WRITE_COORD(damageOrigin.y);
		WRITE_COORD(damageOrigin.z);
		MESSAGE_END();

		pev->dmg_take = 0;
		pev->dmg_save = 0;
		m_bitsHUDDamage = m_bitsDamageType;

		// Clear off non-time-based damage indicators
		m_bitsDamageType &= DMG_TIMEBASED;
	}


	if ((m_iTrain & TRAIN_NEW) != 0)
	{
		ASSERT(gmsgTrain > 0);
		// send "health" update message
		MESSAGE_BEGIN(MSG_ONE, gmsgTrain, NULL, pev);
		WRITE_BYTE(m_iTrain & 0xF);
		MESSAGE_END();

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

		for (i = 0; i < MAX_WEAPONS; i++)
		{
			ItemInfo& II = CBasePlayerItem::ItemInfoArray[i];

			if (WEAPON_NONE == II.iId)
				continue;

			const char* pszName;
			if (!II.pszName)
				pszName = "Empty";
			else
				pszName = II.pszName;

			MESSAGE_BEGIN(MSG_ONE, gmsgWeaponList, NULL, pev);
			WRITE_STRING(pszName);				   // string	weapon name
			WRITE_BYTE(II.iAmmo1); // byte		Ammo Type
			WRITE_BYTE(II.iMaxAmmo1);			   // byte     Max Ammo 1
			WRITE_BYTE(II.iAmmo2); // byte		Ammo2 Type
			WRITE_BYTE(II.iMaxAmmo2);			   // byte     Max Ammo 2
			WRITE_BYTE(II.iSlot);				   // byte		bucket
			WRITE_BYTE(II.iPosition);			   // byte		bucket pos
			WRITE_BYTE(II.iId);					   // byte		id (bit index into m_WeaponBits)
			WRITE_BYTE(II.iFlags);				   // byte		Flags
			MESSAGE_END();
		}
	}

	//Active item is becoming null, or we're sending all HUD state to client
	//Only if we're not in Observer mode, which uses the target player's weapon
	if (pev->iuser1 == OBS_NONE && !m_pActiveItem && ((m_pClientActiveItem != m_pActiveItem) || fullHUDInitRequired))
	{
		//Tell ammo hud that we have no weapon selected
		MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, NULL, pev);
		WRITE_BYTE(0);
		WRITE_BYTE(0);
		WRITE_BYTE(0);
		MESSAGE_END();
	}

	// Cache and client weapon change
	m_pClientActiveItem = m_pActiveItem;

	// Update Status Bar
	if (m_flNextSBarUpdateTime < gpGlobals->time)
	{
		UpdateStatusBar();
		m_flNextSBarUpdateTime = gpGlobals->time + 0.2;
	}

	// Send new room type to client.
	if (m_ClientSndRoomtype != m_SndRoomtype)
	{
		m_ClientSndRoomtype = m_SndRoomtype;

		MESSAGE_BEGIN(MSG_ONE, SVC_ROOMTYPE, nullptr, edict());
		WRITE_SHORT((short)m_SndRoomtype); // sequence number
		MESSAGE_END();
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


#define DOT_1DEGREE 0.9998476951564
#define DOT_2DEGREE 0.9993908270191
#define DOT_3DEGREE 0.9986295347546
#define DOT_4DEGREE 0.9975640502598
#define DOT_5DEGREE 0.9961946980917
#define DOT_6DEGREE 0.9945218953683
#define DOT_7DEGREE 0.9925461516413
#define DOT_8DEGREE 0.9902680687416
#define DOT_9DEGREE 0.9876883405951
#define DOT_10DEGREE 0.9848077530122
#define DOT_15DEGREE 0.9659258262891
#define DOT_20DEGREE 0.9396926207859
#define DOT_25DEGREE 0.9063077870367

//=========================================================
// Autoaim
// set crosshair position to point to enemey
//=========================================================
Vector CBasePlayer::GetAimVector()
{
	UTIL_MakeVectors(pev->v_angle + pev->punchangle);
	return gpGlobals->v_forward;
}


/*
=============
SetCustomDecalFrames

  UNDONE:  Determine real frame limit, 8 is a placeholder.
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
// DropPlayerItem - drop the named item, or if no name,
// the active item.
//=========================================================
void CBasePlayer::DropPlayerItem(char* pszItemName)
{
	if (!UTIL_IsMultiplayer() || (weaponstay.value > 0))
	{
		// no dropping in single player.
		return;
	}

	CBasePlayerItem *pWeapon = nullptr;

	if (pszItemName != nullptr && pszItemName[0] != '\0')
	{
		for (auto it : m_lpPlayerItems)
		{
			if (FClassnameIs(it->pev, pszItemName))
			{
				pWeapon = it;
				break;
			}
		}
	}
	else
	{
		pWeapon = m_pActiveItem;
	}

	if (pWeapon == nullptr)
	{
		return;
	}

	// if we land here with a valid pWeapon pointer, that's because we found the
	// item we want to drop and hit a BREAK;  pWeapon is the item.

	if (!g_pGameRules->GetNextBestWeapon(this, pWeapon))
	{
		return; // can't drop the item they asked for, may be our last item or something we can't holster
	}

	ClearWeaponBit(pWeapon->m_iId); // take item off hud

	UTIL_MakeVectors(pev->angles);

	CWeaponBox* pWeaponBox = (CWeaponBox*)CBaseEntity::Create("weaponbox", pev->origin + gpGlobals->v_forward * 10, pev->angles, edict());
	pWeaponBox->pev->angles.x = 0;
	pWeaponBox->pev->angles.z = 0;
	pWeaponBox->PackWeapon(pWeapon);
	pWeaponBox->pev->velocity = gpGlobals->v_forward * 300 + gpGlobals->v_forward * 100;

	// drop half of the ammo for this weapon.
	int iAmmoIndex = pWeapon->iAmmo1();

	if (iAmmoIndex > AMMO_NONE)
	{
		// this weapon weapon uses ammo, so pack an appropriate amount.
		if ((pWeapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE) != 0)
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
// HasPlayerItem Does the player already have this item?
//=========================================================
bool CBasePlayer::HasPlayerItem(CBasePlayerItem* pCheckItem)
{
	if (pCheckItem->m_iId <= WEAPON_NONE || pCheckItem->m_iId >= MAX_WEAPONS)
	{
		return false;
	}
	return (m_WeaponBits & (1ULL << pCheckItem->m_iId)) != 0;
}

//=========================================================
// HasNamedPlayerItem Does the player already have this item?
//=========================================================
bool CBasePlayer::HasNamedPlayerItem(const char* pszItemName)
{
	for (auto pItem : m_lpPlayerItems)
	{
		if (FClassnameIs(pItem->pev, pszItemName))
		{
			return true;
		}
	}
	return false;
}

//=========================================================
//
//=========================================================
bool CBasePlayer::SwitchWeapon(CBasePlayerItem* pWeapon)
{
	if (pWeapon && !pWeapon->CanDeploy())
	{
		return false;
	}

	if (m_pActiveItem)
	{
		m_pActiveItem->Holster();
	}

	m_pActiveItem = pWeapon;

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
	if (m_pActiveItem)
	{
		if ((!FStringNull(pev->viewmodel) || !FStringNull(pev->weaponmodel)))
		{
			//Already have a weapon equipped and deployed.
			return;
		}

		//Have a weapon equipped, but not deployed.
		if (m_pActiveItem->CanDeploy() && m_pActiveItem->Deploy())
		{
			return;
		}
	}

	//No weapon equipped or couldn't deploy it, find a suitable alternative.
	g_pGameRules->GetNextBestWeapon(this, m_pActiveItem, true);
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
	else if (!UTIL_IsDeathmatch())
	{
		pPlayer = (CBasePlayer*)UTIL_GetLocalPlayer();
	}

	if (pPlayer)
		pPlayer->RemoveAllItems(false);
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
	UTIL_ScreenFadeAll(pev->rendercolor, Duration(), HoldTime(), pev->renderamt, FFADE_OUT);
	pev->nextthink = gpGlobals->time + MessageTime();
	SetThink(&CRevertSaved::MessageThink);
}


void CRevertSaved::MessageThink()
{
	UTIL_ShowMessageAll(STRING(pev->message));
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
	if (!UTIL_IsMultiplayer())
	{
		SERVER_COMMAND("reload\n");
	}
}


//=========================================================
// Multiplayer intermission spots.
//=========================================================
class CInfoIntermission : public CPointEntity
{
	void Spawn() override;
	void Think() override;
};

void CInfoIntermission::Spawn()
{
	UTIL_SetOrigin(pev, pev->origin);
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->v_angle = g_vecZero;

	pev->nextthink = gpGlobals->time + 2; // let targets spawn!
}

void CInfoIntermission::Think()
{
	edict_t* pTarget;

	// find my target
	pTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target));

	if (!FNullEnt(pTarget))
	{
		pev->v_angle = UTIL_VecToAngles((pTarget->v.origin - pev->origin).Normalize());
		pev->v_angle.x = -pev->v_angle.x;
	}
}

LINK_ENTITY_TO_CLASS(info_intermission, CInfoIntermission);
