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
	CEnvExplosion(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	DECLARE_SAVERESTORE()

	bool Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	int m_iMagnitude;  // how large is the fireball? how much damage?
};

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CEnvExplosion)
	DEFINE_FIELD(CEnvExplosion, m_iMagnitude, FIELD_INTEGER),
END_SAVERESTORE(CEnvExplosion, CBaseEntity)
#endif

LINK_ENTITY_TO_CLASS(env_explosion, CEnvExplosion);

bool CEnvExplosion::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "iMagnitude"))
	{
		m_iMagnitude = atoi(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

bool CEnvExplosion::Spawn()
{
	v.solid = SOLID_NOT;
	v.effects = EF_NODRAW;

	v.movetype = MOVETYPE_NONE;

	return true;
}

void CEnvExplosion::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	TraceResult tr;

	v.model = iStringNull; //invisible
	v.solid = SOLID_NOT;	  // intangible

	Vector vecSpot; // trace starts here!

	vecSpot = v.origin + Vector(0, 0, 8);

	util::TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), util::ignore_monsters, this, &tr);

	// Pull out of the wall a bit
	if (tr.flFraction != 1.0)
	{
		v.origin = tr.vecEndPos + (tr.vecPlaneNormal * (m_iMagnitude - 24) * 0.6);
	}
	else
	{
		v.origin = v.origin;
	}

	// draw fireball
	tent::Explosion(
		v.origin,
		g_vecZero,
		tent::ExplosionType::Normal,
		(v.spawnflags & SF_ENVEXPLOSION_NOFIREBALL) == 0 ? m_iMagnitude : 0,
		(v.spawnflags & SF_ENVEXPLOSION_NOSMOKE) == 0,
		(v.spawnflags & SF_ENVEXPLOSION_NOSPARKS) == 0);

	// do damage
	if ((v.spawnflags & SF_ENVEXPLOSION_NODAMAGE) == 0)
	{
		RadiusDamage(v.origin, this, this, m_iMagnitude, 0, m_iMagnitude * 2.5, DMG_BLAST);
	}

	if ((v.spawnflags & SF_ENVEXPLOSION_REPEATABLE) == 0)
	{
		Remove();
	}
}


// HACKHACK -- create one of these and fake a keyvalue to get the right explosion setup
void ExplosionCreate(const Vector& center, const Vector& angles, Entity* pOwner, int magnitude, bool doDamage)
{
	KeyValueData kvd;
	char buf[128];

	CBaseEntity* pExplosion = CBaseEntity::Create("env_explosion", center, angles, *pOwner);
	sprintf(buf, "%3d", magnitude);
	kvd.szKeyName = "iMagnitude";
	kvd.szValue = buf;
	pExplosion->KeyValue(&kvd);
	if (!doDamage)
		pExplosion->v.spawnflags |= SF_ENVEXPLOSION_NODAMAGE;

	pExplosion->Spawn();
	pExplosion->Use(nullptr, nullptr, USE_TOGGLE, 0);
}
