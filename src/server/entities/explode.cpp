/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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

===== explode.cpp ========================================================

  Explosion-related code

*/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "explode.h"
#include "weapons.h"

class CEnvExplosion : public CBaseEntity
{
public:
	bool Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	int m_iMagnitude;  // how large is the fireball? how much damage?
	int m_spriteScale; // what's the exact fireball sprite scale?
};

TYPEDESCRIPTION CEnvExplosion::m_SaveData[] =
	{
		DEFINE_FIELD(CEnvExplosion, m_iMagnitude, FIELD_INTEGER),
		DEFINE_FIELD(CEnvExplosion, m_spriteScale, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CEnvExplosion, CBaseEntity);
LINK_ENTITY_TO_CLASS(env_explosion, CEnvExplosion);

bool CEnvExplosion::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "iMagnitude"))
	{
		m_iMagnitude = atoi(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

bool CEnvExplosion::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	pev->movetype = MOVETYPE_NONE;

	float flSpriteScale;
	flSpriteScale = (m_iMagnitude - 50) * 0.6;

	if (flSpriteScale < 10)
	{
		flSpriteScale = 10;
	}

	m_spriteScale = (int)flSpriteScale;

	return true;
}

void CEnvExplosion::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	TraceResult tr;

	pev->model = iStringNull; //invisible
	pev->solid = SOLID_NOT;	  // intangible

	Vector vecSpot; // trace starts here!

	vecSpot = pev->origin + Vector(0, 0, 8);

	util::TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), util::ignore_monsters, this, &tr);

	// Pull out of the wall a bit
	if (tr.flFraction != 1.0)
	{
		pev->origin = tr.vecEndPos + (tr.vecPlaneNormal * (m_iMagnitude - 24) * 0.6);
	}
	else
	{
		pev->origin = pev->origin;
	}

	// draw fireball
	tent::Explosion(
		pev->origin,
		g_vecZero,
		(pev->spawnflags & SF_ENVEXPLOSION_NOFIREBALL) == 0 ? m_spriteScale : 0,
		(pev->spawnflags & SF_ENVEXPLOSION_NOSMOKE) == 0,
		(pev->spawnflags & SF_ENVEXPLOSION_NOSPARKS) == 0);

	// do damage
	if ((pev->spawnflags & SF_ENVEXPLOSION_NODAMAGE) == 0)
	{
		RadiusDamage(pev->origin, this, this, m_iMagnitude, m_iMagnitude * 2.5, DMG_BLAST);
	}

	if ((pev->spawnflags & SF_ENVEXPLOSION_REPEATABLE) == 0)
	{
		Remove();
	}
}


// HACKHACK -- create one of these and fake a keyvalue to get the right explosion setup
void ExplosionCreate(const Vector& center, const Vector& angles, edict_t* pOwner, int magnitude, bool doDamage)
{
	KeyValueData kvd;
	char buf[128];

	CBaseEntity* pExplosion = CBaseEntity::Create("env_explosion", center, angles, pOwner);
	sprintf(buf, "%3d", magnitude);
	kvd.szKeyName = "iMagnitude";
	kvd.szValue = buf;
	pExplosion->KeyValue(&kvd);
	if (!doDamage)
		pExplosion->pev->spawnflags |= SF_ENVEXPLOSION_NODAMAGE;

	pExplosion->Spawn();
	pExplosion->Use(NULL, NULL, USE_TOGGLE, 0);
}
