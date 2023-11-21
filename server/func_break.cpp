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

===== bmodels.cpp ========================================================

  spawn, think, and use functions for entities that use brush models

*/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "func_break.h"
#include "decals.h"
#include "explode.h"

// func breakable
#define SF_BREAK_TRIGGER_ONLY 1 // may only be broken by trigger
#define SF_BREAK_TOUCH 2		// can be 'crashed through' by running player (plate glass)
#define SF_BREAK_PRESSURE 4		// can be broken by a player standing on it
#define SF_BREAK_CROWBAR 256	// instant break if hit with crowbar

// func_pushable (it's also func_breakable, so don't collide with those flags)
#define SF_PUSH_BREAKABLE 128

// =================== FUNC_Breakable ==============================================

// Just add more items to the bottom of this array and they will automagically be supported
// This is done instead of just a classname in the FGD so we can control which entities can
// be spawned, and still remain fairly flexible
const char* CBreakable::pSpawnObjects[] =
	{
		NULL,				  // 0
		"item_battery",		  // 1
		"item_healthkit",	  // 2
		"weapon_9mmhandgun",  // 3
		"ammo_9mmclip",		  // 4
		"weapon_9mmAR",		  // 5
		"ammo_9mmAR",		  // 6
		"ammo_ARgrenades",	  // 7
		"weapon_shotgun",	  // 8
		"ammo_buckshot",	  // 9
		"weapon_crossbow",	  // 10
		"ammo_crossbow",	  // 11
		"weapon_357",		  // 12
		"ammo_357",			  // 13
		"weapon_rpg",		  // 14
		"ammo_rpgclip",		  // 15
		"ammo_gaussclip",	  // 16
		"weapon_handgrenade", // 17
		"weapon_tripmine",	  // 18
		"weapon_satchel",	  // 19
		"weapon_snark",		  // 20
		"weapon_hornetgun",	  // 21
};

bool CBreakable::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "explosion"))
	{
		if (!stricmp(pkvd->szValue, "directed"))
			m_Explosion = expDirected;
		else if (!stricmp(pkvd->szValue, "random"))
			m_Explosion = expRandom;
		else
			m_Explosion = expRandom;

		if (!stricmp(pkvd->szValue, "1"))
		{
			m_Explosion = expDirected;
		}
		else
		{
			m_Explosion = expRandom;
		}

		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "material"))
	{
		int i = atoi(pkvd->szValue);

		// 0:glass, 1:metal, 2:flesh, 3:wood

		if ((i < 0) || (i >= matLastMaterial))
			m_Material = matWood;
		else
			m_Material = (Materials)i;

		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "deadmodel"))
	{
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "shards"))
	{
		//			m_iShards = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gibmodel"))
	{
		m_iszGibModel = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "spawnobject"))
	{
		int object = atoi(pkvd->szValue);
		if (object > 0 && object < ARRAYSIZE(pSpawnObjects))
			m_iszSpawnObject = MAKE_STRING(pSpawnObjects[object]);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "explodemagnitude"))
	{
		ExplosionSetMagnitude(atoi(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "lip"))
		return true;

	return CBaseDelay::KeyValue(pkvd);
}


//
// func_breakable - bmodel that breaks into pieces after taking damage
//
LINK_ENTITY_TO_CLASS(func_breakable, CBreakable);
TYPEDESCRIPTION CBreakable::m_SaveData[] =
	{
		DEFINE_FIELD(CBreakable, m_Material, FIELD_INTEGER),
		DEFINE_FIELD(CBreakable, m_Explosion, FIELD_INTEGER),

		// Don't need to save/restore these because we precache after restore
		//	DEFINE_FIELD( CBreakable, m_idShard, FIELD_INTEGER ),

		DEFINE_FIELD(CBreakable, m_angle, FIELD_FLOAT),
		DEFINE_FIELD(CBreakable, m_iszGibModel, FIELD_STRING),
		DEFINE_FIELD(CBreakable, m_iszSpawnObject, FIELD_STRING),

		// Explosion magnitude is stored in pev->impulse
};

IMPLEMENT_SAVERESTORE(CBreakable, CBaseEntity);

bool CBreakable::Spawn()
{
	Precache();

	if (FBitSet(pev->spawnflags, SF_BREAK_TRIGGER_ONLY))
		pev->takedamage = DAMAGE_NO;
	else
		pev->takedamage = DAMAGE_YES;

	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;
	m_angle = pev->angles.y;
	pev->angles.y = 0;

	// HACK:  matGlass can receive decals, we need the client to know about this
	//  so use class to store the material flag
	if (m_Material == matGlass)
	{
		pev->playerclass = 1;
	}

	SetModel(STRING(pev->model)); //set size and link into world.

	SetTouch(&CBreakable::BreakTouch);
	if (FBitSet(pev->spawnflags, SF_BREAK_TRIGGER_ONLY)) // Only break on trigger
		SetTouch(NULL);

	// Flag unbreakable glass as "worldbrush" so it will block ALL tracelines
	if (!IsBreakable() && pev->rendermode != kRenderNormal)
		pev->flags |= FL_WORLDBRUSH;

	return true;
}


const char* CBreakable::pSoundsWood[] =
	{
		"debris/wood1.wav",
		"debris/wood2.wav",
		"debris/wood3.wav",
};

const char* CBreakable::pSoundsFlesh[] =
	{
		"debris/flesh1.wav",
		"debris/flesh2.wav",
		"debris/flesh3.wav",
		"debris/flesh5.wav",
		"debris/flesh6.wav",
		"debris/flesh7.wav",
};

const char* CBreakable::pSoundsMetal[] =
	{
		"debris/metal1.wav",
		"debris/metal2.wav",
		"debris/metal3.wav",
};

const char* CBreakable::pSoundsConcrete[] =
	{
		"debris/concrete1.wav",
		"debris/concrete2.wav",
		"debris/concrete3.wav",
};


const char* CBreakable::pSoundsGlass[] =
	{
		"debris/glass1.wav",
		"debris/glass2.wav",
		"debris/glass3.wav",
};

const char** CBreakable::MaterialSoundList(Materials precacheMaterial, int& soundCount)
{
	const char** pSoundList = NULL;

	switch (precacheMaterial)
	{
	case matWood:
		pSoundList = pSoundsWood;
		soundCount = ARRAYSIZE(pSoundsWood);
		break;
	case matFlesh:
		pSoundList = pSoundsFlesh;
		soundCount = ARRAYSIZE(pSoundsFlesh);
		break;
	case matComputer:
	case matUnbreakableGlass:
	case matGlass:
		pSoundList = pSoundsGlass;
		soundCount = ARRAYSIZE(pSoundsGlass);
		break;

	case matMetal:
		pSoundList = pSoundsMetal;
		soundCount = ARRAYSIZE(pSoundsMetal);
		break;

	case matCinderBlock:
	case matRocks:
		pSoundList = pSoundsConcrete;
		soundCount = ARRAYSIZE(pSoundsConcrete);
		break;


	case matCeilingTile:
	case matNone:
	default:
		soundCount = 0;
		break;
	}

	return pSoundList;
}

void CBreakable::MaterialSoundPrecache(Materials precacheMaterial)
{
	const char** pSoundList;
	int i, soundCount = 0;

	pSoundList = MaterialSoundList(precacheMaterial, soundCount);

	for (i = 0; i < soundCount; i++)
	{
		PRECACHE_SOUND((char*)pSoundList[i]);
	}
}


void CBreakable::Precache()
{
	const char* pGibName;

	switch (m_Material)
	{
	default: //Wood is default, needs to match constant used in KeyValue
	case matWood:
		pGibName = "models/woodgibs.mdl";

		PRECACHE_SOUND("debris/bustcrate1.wav");
		PRECACHE_SOUND("debris/bustcrate2.wav");
		break;
	case matFlesh:
		pGibName = "models/fleshgibs.mdl";

		PRECACHE_SOUND("debris/bustflesh1.wav");
		PRECACHE_SOUND("debris/bustflesh2.wav");
		break;
	case matComputer:
		PRECACHE_SOUND("buttons/spark5.wav");
		PRECACHE_SOUND("buttons/spark6.wav");
		pGibName = "models/computergibs.mdl";

		PRECACHE_SOUND("debris/bustmetal1.wav");
		PRECACHE_SOUND("debris/bustmetal2.wav");
		break;

	case matUnbreakableGlass:
	case matGlass:
		pGibName = "models/glassgibs.mdl";

		PRECACHE_SOUND("debris/bustglass1.wav");
		PRECACHE_SOUND("debris/bustglass2.wav");
		break;
	case matMetal:
		pGibName = "models/metalplategibs.mdl";

		PRECACHE_SOUND("debris/bustmetal1.wav");
		PRECACHE_SOUND("debris/bustmetal2.wav");
		break;
	case matCinderBlock:
		pGibName = "models/cindergibs.mdl";

		PRECACHE_SOUND("debris/bustconcrete1.wav");
		PRECACHE_SOUND("debris/bustconcrete2.wav");
		break;
	case matRocks:
		pGibName = "models/rockgibs.mdl";

		PRECACHE_SOUND("debris/bustconcrete1.wav");
		PRECACHE_SOUND("debris/bustconcrete2.wav");
		break;
	case matCeilingTile:
		pGibName = "models/ceilinggibs.mdl";

		PRECACHE_SOUND("debris/bustceiling.wav");
		break;
	}
	MaterialSoundPrecache(m_Material);
	if (!FStringNull(m_iszGibModel))
		pGibName = STRING(m_iszGibModel);

	m_idShard = PRECACHE_MODEL((char*)pGibName);

	// Precache the spawn item's data
	if (!FStringNull(m_iszSpawnObject))
		UTIL_PrecacheOther((char*)STRING(m_iszSpawnObject));
}

// play shard sound when func_breakable takes damage.
// the more damage, the louder the shard sound.


void CBreakable::DamageSound()
{
	int pitch;
	float fvol;
	const char* rgpsz[6];
	int i;
	int material = m_Material;

	//	if (RANDOM_LONG(0,1))
	//		return;

	if (RANDOM_LONG(0, 2))
		pitch = PITCH_NORM;
	else
		pitch = 95 + RANDOM_LONG(0, 34);

	fvol = RANDOM_FLOAT(0.75, 1.0);

	if (material == matComputer && RANDOM_LONG(0, 1))
		material = matMetal;

	switch (material)
	{
	case matComputer:
	case matGlass:
	case matUnbreakableGlass:
		rgpsz[0] = "debris/glass1.wav";
		rgpsz[1] = "debris/glass2.wav";
		rgpsz[2] = "debris/glass3.wav";
		i = 3;
		break;

	default: //Wood is default, needs to match constant used in KeyValue
	case matWood:
		rgpsz[0] = "debris/wood1.wav";
		rgpsz[1] = "debris/wood2.wav";
		rgpsz[2] = "debris/wood3.wav";
		i = 3;
		break;

	case matMetal:
		rgpsz[0] = "debris/metal1.wav";
		rgpsz[1] = "debris/metal3.wav";
		rgpsz[2] = "debris/metal2.wav";
		i = 2;
		break;

	case matFlesh:
		rgpsz[0] = "debris/flesh1.wav";
		rgpsz[1] = "debris/flesh2.wav";
		rgpsz[2] = "debris/flesh3.wav";
		rgpsz[3] = "debris/flesh5.wav";
		rgpsz[4] = "debris/flesh6.wav";
		rgpsz[5] = "debris/flesh7.wav";
		i = 6;
		break;

	case matRocks:
	case matCinderBlock:
		rgpsz[0] = "debris/concrete1.wav";
		rgpsz[1] = "debris/concrete2.wav";
		rgpsz[2] = "debris/concrete3.wav";
		i = 3;
		break;

	case matCeilingTile:
		i = 0;
		break;
	}

	if (0 != i)
		EmitSound(rgpsz[RANDOM_LONG(0, i - 1)], CHAN_VOICE, fvol, ATTN_NORM, pitch);
}

void CBreakable::BreakTouch(CBaseEntity* pOther)
{
	float flDamage;

	// only players can break these right now
	if (!pOther->IsPlayer() || !IsBreakable())
	{
		return;
	}

	if (FBitSet(pev->spawnflags, SF_BREAK_TOUCH))
	{ // can be broken when run into
		flDamage = pOther->pev->velocity.Length() * 0.01;

		if (flDamage >= pev->health)
		{
			SetTouch(NULL);
			TakeDamage(pOther, pOther, flDamage, DMG_CRUSH);

			// do a little damage to player if we broke glass or computer
			pOther->TakeDamage(this, this, flDamage / 4, DMG_SLASH);
		}
	}

	if (FBitSet(pev->spawnflags, SF_BREAK_PRESSURE) && pOther->pev->absmin.z >= pev->maxs.z - 2)
	{ // can be broken when stood upon

		// play creaking sound here.
		DamageSound();

		SetThink(&CBreakable::Die);
		SetTouch(NULL);

		if (m_flDelay == 0)
		{ // !!!BUGBUG - why doesn't zero delay work?
			m_flDelay = 0.1;
		}

		pev->nextthink = pev->ltime + m_flDelay;
	}
}


//
// Smash the our breakable object
//

// Break when triggered
void CBreakable::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (IsBreakable())
	{
		pev->angles.y = m_angle;
		UTIL_MakeVectors(pev->angles);
		g_vecAttackDir = gpGlobals->v_forward;

		Die();
	}
}


void CBreakable::TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	// random spark if this is a 'computer' object
	if (RANDOM_LONG(0, 1))
	{
		switch (m_Material)
		{
		case matComputer:
		{
			UTIL_Sparks(ptr->vecEndPos);

			float flVolume = RANDOM_FLOAT(0.7, 1.0); //random volume range
			switch (RANDOM_LONG(0, 1))
			{
			case 0: EmitSound("buttons/spark5.wav", CHAN_VOICE, flVolume); break;
			case 1: EmitSound("buttons/spark6.wav", CHAN_VOICE, flVolume); break;
			}
		}
		break;

		case matUnbreakableGlass:
			UTIL_Ricochet(ptr->vecEndPos, RANDOM_FLOAT(0.5, 1.5));
			break;
		}
	}

	CBaseDelay::TraceAttack(attacker, flDamage, vecDir, ptr, bitsDamageType);
}

//=========================================================
// Special takedamage for func_breakable. Allows us to make
// exceptions that are breakable-specific
// bitsDamageType indicates the type of damage sustained ie: DMG_CRUSH
//=========================================================
bool CBreakable::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	Vector vecTemp;

	// if Attacker == Inflictor, the attack was a melee or other instant-hit attack.
	// (that is, no actual entity projectile was involved in the attack so use the shooter's origin).
	if (attacker == inflictor)
	{
		vecTemp = inflictor->pev->origin - (pev->absmin + (pev->size * 0.5));

		// if a client hit the breakable with a crowbar, and breakable is crowbar-sensitive, break it now.
		if (FBitSet(inflictor->pev->flags, FL_CLIENT) &&
			FBitSet(pev->spawnflags, SF_BREAK_CROWBAR) && (bitsDamageType & DMG_CLUB) != 0)
			flDamage = pev->health;
	}
	else
	// an actual missile was involved.
	{
		vecTemp = inflictor->pev->origin - (pev->absmin + (pev->size * 0.5));
	}

	if (!IsBreakable())
		return false;

	// Breakables take double damage from the crowbar
	if ((bitsDamageType & DMG_CLUB) != 0)
		flDamage *= 2;

	// Boxes / glass / etc. don't take much poison damage, just the impact of the dart - consider that 10%
	if ((bitsDamageType & DMG_POISON) != 0)
		flDamage *= 0.1;

	// this global is still used for glass and other non-monster killables, along with decals.
	g_vecAttackDir = vecTemp.Normalize();

	// do the damage
	pev->health -= flDamage;
	if (pev->health <= 0)
	{
		Killed(inflictor, attacker, bitsDamageType);
		Die();
		return false;
	}

	// Make a shard noise each time func breakable is hit.
	// Don't play shard noise if cbreakable actually died.

	DamageSound();

	return true;
}


void CBreakable::Die()
{
	// Don't allow explosives to damage this again to prevent spawning multiple copies of items and gibs.
	if (pev->solid == SOLID_NOT)
	{
		return;
	}

	Vector vecSpot;		// shard origin
	Vector vecVelocity; // shard velocity
	CBaseEntity* pEntity = NULL;
	char cFlag = 0;
	int pitch;
	float fvol;

	pitch = 95 + RANDOM_LONG(0, 29);

	if (pitch > 97 && pitch < 103)
		pitch = 100;

	// The more negative pev->health, the louder
	// the sound should be.

	fvol = RANDOM_FLOAT(0.85, 1.0) + (fabs(pev->health) / 100.0);

	if (fvol > 1.0)
		fvol = 1.0;

	const char* sample;

	switch (m_Material)
	{
	default:
	case matGlass:
		switch (RANDOM_LONG(0, 1))
		{
		case 0: sample ="debris/bustglass1.wav"; break;
		case 1: sample ="debris/bustglass2.wav"; break;
		}
		cFlag = BREAK_GLASS;
		break;

	case matWood:
		switch (RANDOM_LONG(0, 1))
		{
		case 0: sample ="debris/bustcrate1.wav"; break;
		case 1: sample ="debris/bustcrate2.wav"; break;
		}
		cFlag = BREAK_WOOD;
		break;

	case matComputer:
	case matMetal:
		switch (RANDOM_LONG(0, 1))
		{
		case 0: sample ="debris/bustmetal1.wav"; break;
		case 1: sample ="debris/bustmetal2.wav"; break;
		}
		cFlag = BREAK_METAL;
		break;

	case matFlesh:
		switch (RANDOM_LONG(0, 1))
		{
		case 0: sample ="debris/bustflesh1.wav"; break;
		case 1: sample ="debris/bustflesh2.wav"; break;
		}
		cFlag = BREAK_FLESH;
		break;

	case matRocks:
	case matCinderBlock:
		switch (RANDOM_LONG(0, 1))
		{
		case 0: sample ="debris/bustconcrete1.wav"; break;
		case 1: sample ="debris/bustconcrete2.wav"; break;
		}
		cFlag = BREAK_CONCRETE;
		break;

	case matCeilingTile:
		sample ="debris/bustceiling.wav";
		break;
	}
	
	EmitSound(sample, CHAN_VOICE, fvol, ATTN_NORM, pitch);


	if (m_Explosion == expDirected)
		vecVelocity = -g_vecAttackDir * 200;
	else
	{
		vecVelocity.x = 0;
		vecVelocity.y = 0;
		vecVelocity.z = 0;
	}

	vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
	MessageBegin(MSG_PVS, SVC_TEMPENTITY, vecSpot);
	WriteByte(TE_BREAKMODEL);

	// position
	WriteCoord(vecSpot.x);
	WriteCoord(vecSpot.y);
	WriteCoord(vecSpot.z);

	// size
	WriteCoord(pev->size.x);
	WriteCoord(pev->size.y);
	WriteCoord(pev->size.z);

	// velocity
	WriteCoord(vecVelocity.x);
	WriteCoord(vecVelocity.y);
	WriteCoord(vecVelocity.z);

	// randomization
	WriteByte(10);

	// Model
	WriteShort(m_idShard); //model id#

	// # of shards
	WriteByte(0); // let client decide

	// duration
	WriteByte(25); // 2.5 seconds

	// flags
	WriteByte(cFlag);
	MessageEnd();

	float size = pev->size.x;
	if (size < pev->size.y)
		size = pev->size.y;
	if (size < pev->size.z)
		size = pev->size.z;

	// !!! HACK  This should work!
	// Build a box above the entity that looks like an 8 pixel high sheet
	Vector mins = pev->absmin;
	Vector maxs = pev->absmax;
	mins.z = pev->absmax.z;
	maxs.z += 8;

	// BUGBUG -- can only find 256 entities on a breakable -- should be enough
	CBaseEntity* pList[256];
	int count = UTIL_EntitiesInBox(pList, 256, mins, maxs, FL_ONGROUND);
	if (0 != count)
	{
		for (int i = 0; i < count; i++)
		{
			ClearBits(pList[i]->pev->flags, FL_ONGROUND);
			pList[i]->pev->groundentity = NULL;
		}
	}

	// Don't fire something that could fire myself
	pev->targetname = 0;

	pev->solid = SOLID_NOT;
	// Fire targets on break
	SUB_UseTargets(NULL, USE_TOGGLE, 0);

	if (!FStringNull(m_iszSpawnObject))
		CBaseEntity::Create((char*)STRING(m_iszSpawnObject), VecBModelOrigin(pev), pev->angles, edict());


	if (Explodable())
	{
		ExplosionCreate(Center(), pev->angles, edict(), ExplosionMagnitude(), true);
	}

	Remove();
}



bool CBreakable::IsBreakable()
{
	return m_Material != matUnbreakableGlass;
}


int CBreakable::DamageDecal(int bitsDamageType)
{
	if (m_Material == matGlass)
		return DECAL_GLASSBREAK1 + RANDOM_LONG(0, 2);

	if (m_Material == matUnbreakableGlass)
		return DECAL_BPROOF1;

	return CBaseEntity::DamageDecal(bitsDamageType);
}


class CPushable : public CBreakable
{
public:
	bool Spawn() override;
	void Precache() override;
	void Touch(CBaseEntity* pOther) override;
	void Move(CBaseEntity* pMover, bool push);
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	int ObjectCaps() override { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_CONTINUOUS_USE; }
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	inline float MaxSpeed() { return m_maxSpeed; }

	// breakables use an overridden takedamage
	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;

	static TYPEDESCRIPTION m_SaveData[];

	static const char* m_soundNames[3];
	int m_lastSound; // no need to save/restore, just keeps the same sound from playing twice in a row
	float m_maxSpeed;
	float m_soundTime;
};

TYPEDESCRIPTION CPushable::m_SaveData[] =
	{
		DEFINE_FIELD(CPushable, m_maxSpeed, FIELD_FLOAT),
		DEFINE_FIELD(CPushable, m_soundTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CPushable, CBreakable);

LINK_ENTITY_TO_CLASS(func_pushable, CPushable);

const char* CPushable::m_soundNames[3] = {"debris/pushbox1.wav", "debris/pushbox2.wav", "debris/pushbox3.wav"};


bool CPushable::Spawn()
{
	if ((pev->spawnflags & SF_PUSH_BREAKABLE) != 0)
	{
		if (!CBreakable::Spawn())
		{
			return false;
		}
	}
	else
	{
		Precache();
	}

	pev->movetype = MOVETYPE_PUSHSTEP;
	pev->solid = SOLID_BBOX;
	SetModel(STRING(pev->model));

	if (pev->friction > 399)
		pev->friction = 399;

	m_maxSpeed = 400 - pev->friction;
	SetBits(pev->flags, FL_FLOAT);
	pev->friction = 0;

	pev->origin.z += 1; // Pick up off of the floor
	SetOrigin(pev->origin);

	// Multiply by area of the box's cross-section (assume 1000 units^3 standard volume)
	pev->skin = (pev->skin * (pev->maxs.x - pev->mins.x) * (pev->maxs.y - pev->mins.y)) * 0.0005;
	m_soundTime = 0;

	return true;
}


void CPushable::Precache()
{
	for (int i = 0; i < 3; i++)
		PRECACHE_SOUND(m_soundNames[i]);

	if ((pev->spawnflags & SF_PUSH_BREAKABLE) != 0)
		CBreakable::Precache();
}


bool CPushable::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "size"))
	{
		int bbox = atoi(pkvd->szValue);

		switch (bbox)
		{
		case 0: // Point
			SetSize(Vector(-8, -8, -8), Vector(8, 8, 8));
			break;

		case 2: // Stupid Hull
			SetSize(VEC_DUCK_HULL_MIN * 2, VEC_DUCK_HULL_MAX * 2);
			break;

		case 3: // Player duck
			SetSize(VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
			break;

		default:
		case 1: // Player
			SetSize(VEC_HULL_MIN, VEC_HULL_MAX);
			break;
		}

		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "buoyancy"))
	{
		pev->skin = atof(pkvd->szValue);
		return true;
	}

	return CBreakable::KeyValue(pkvd);
}


// Pull the func_pushable
void CPushable::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!pActivator || !pActivator->IsPlayer())
	{
		if ((pev->spawnflags & SF_PUSH_BREAKABLE) != 0)
			this->CBreakable::Use(pActivator, pCaller, useType, value);
		return;
	}

	if (pActivator->pev->velocity != g_vecZero)
		Move(pActivator, false);
}


void CPushable::Touch(CBaseEntity* pOther)
{
	if (FClassnameIs(pOther->pev, "worldspawn"))
		return;

	Move(pOther, true);
}


void CPushable::Move(CBaseEntity* pOther, bool push)
{
	entvars_t* pevToucher = pOther->pev;
	bool playerTouch = false;

	// Is entity standing on this pushable ?
	if (FBitSet(pevToucher->flags, FL_ONGROUND) && pevToucher->groundentity && VARS(pevToucher->groundentity) == pev)
	{
		// Only push if floating
		if (pev->waterlevel > 0)
			pev->velocity.z += pevToucher->velocity.z * 0.1;

		return;
	}


	if (pOther->IsPlayer())
	{
		if (push && (pevToucher->button & (IN_FORWARD | IN_USE)) == 0) // Don't push unless the player is pushing forward and NOT use (pull)
			return;
		playerTouch = true;
	}

	float factor;

	if (playerTouch)
	{
		if ((pevToucher->flags & FL_ONGROUND) == 0) // Don't push away from jumping/falling players unless in water
		{
			if (pev->waterlevel < 1)
				return;
			else
				factor = 0.1;
		}
		else
			factor = 1;
	}
	else
		factor = 0.25;

	pev->velocity.x += pevToucher->velocity.x * factor;
	pev->velocity.y += pevToucher->velocity.y * factor;

	float length = sqrt(pev->velocity.x * pev->velocity.x + pev->velocity.y * pev->velocity.y);
	if (push && (length > MaxSpeed()))
	{
		pev->velocity.x = (pev->velocity.x * MaxSpeed() / length);
		pev->velocity.y = (pev->velocity.y * MaxSpeed() / length);
	}
	if (playerTouch)
	{
		pevToucher->velocity.x = pev->velocity.x;
		pevToucher->velocity.y = pev->velocity.y;
		if ((gpGlobals->time - m_soundTime) > 0.7)
		{
			m_soundTime = gpGlobals->time;
			if (length > 0 && FBitSet(pev->flags, FL_ONGROUND))
			{
				m_lastSound = RANDOM_LONG(0, 2);
				EmitSound(m_soundNames[m_lastSound], CHAN_WEAPON, 0.5F);
			}
			else
			{
				StopSound(m_soundNames[m_lastSound], CHAN_WEAPON);
			}
		}
	}
}

bool CPushable::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	if ((pev->spawnflags & SF_PUSH_BREAKABLE) != 0)
		return CBreakable::TakeDamage(inflictor, attacker, flDamage, bitsDamageType);

	return true;
}
