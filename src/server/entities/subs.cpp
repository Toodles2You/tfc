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

===== subs.cpp ========================================================

  frequently used global functions

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "doors.h"
#include "gamerules.h"
#ifdef HALFLIFE_NODEGRAPH
#include "nodes.h"
#endif

// Landmark class
bool CPointEntity::Spawn()
{
	v.solid = SOLID_NOT;
	return true;
}

LINK_ENTITY_TO_CLASS(info_landmark, CPointEntity);

class CBaseDMStart : public CPointEntity
{
public:
	CBaseDMStart(Entity* containingEntity) : CPointEntity(containingEntity) {}

	bool KeyValue(KeyValueData* pkvd) override;
	bool Spawn() override;
};

// These are the new entry points to entities.
LINK_ENTITY_TO_CLASS(info_player_deathmatch, CBaseDMStart);
LINK_ENTITY_TO_CLASS(info_player_start, CBaseDMStart);

bool CBaseDMStart::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "master"))
	{
		v.netname = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

bool CBaseDMStart::Spawn()
{
	g_pGameRules->AddPlayerSpawnSpot(this);
	return false;
}

//=========================================================
// Multiplayer intermission spots.
//=========================================================
class CInfoIntermission : public CPointEntity
{
public:
	CInfoIntermission(Entity* containingEntity) : CPointEntity(containingEntity) {}

	bool Spawn() override;
	void Think() override;
};

bool CInfoIntermission::Spawn()
{
	SetOrigin(v.origin);
	v.solid = SOLID_NOT;
	v.effects = EF_NODRAW;
	v.v_angle = g_vecZero;

	v.nextthink = gpGlobals->time + 2; // let targets spawn!

	return true;
}

void CInfoIntermission::Think()
{
	CBaseEntity* pTarget;

	// find my target
	pTarget = util::FindEntityByTargetname(nullptr, STRING(v.target));

	if (pTarget != nullptr)
	{
		v.v_angle = util::VecToAngles((pTarget->v.origin - v.origin).Normalize());
		v.v_angle.x = -v.v_angle.x;
	}
}

LINK_ENTITY_TO_CLASS(info_intermission, CInfoIntermission);

// This updates global tables that need to know about entities being removed
void CBaseEntity::UpdateOnRemove()
{
#ifdef HALFLIFE_NODEGRAPH
	int i;

	if (FBitSet(v.flags, FL_GRAPHED) && WorldGraph.IsAvailable())
	{
		// this entity was a LinkEnt in the world node graph, so we must remove it from
		// the graph since we are removing it from the world.
		for (i = 0; i < WorldGraph.m_cLinks; i++)
		{
			if (WorldGraph.m_pLinkPool[i].m_pLinkEnt == &v)
			{
				// if this link has a link ent which is the same ent that is removing itself, remove it!
				WorldGraph.m_pLinkPool[i].m_pLinkEnt = nullptr;
			}
		}
	}
#endif

	if (!FStringNull(v.globalname))
		gGlobalState.EntitySetState(v.globalname, GLOBAL_DEAD);
}

// Convenient way to delay removing oneself
void CBaseEntity::Remove()
{
	if ((v.flags & FL_KILLME) != 0)
	{
		return;
	}

	UpdateOnRemove();

	v.flags |= FL_KILLME;
	v.targetname = iStringNull;
	v.globalname = iStringNull;
	v.netname = iStringNull;
	v.target = iStringNull;
	v.model = iStringNull;
	v.modelindex = 0;
	v.solid = SOLID_NOT;
	v.movetype = MOVETYPE_NONE;
	ClearThink();
	ClearTouch();
	ClearUse();
}

bool CBaseEntity::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "delay"))
	{
		m_flDelay = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "killtarget"))
	{
		m_iszKillTarget = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return false;
}


void util::FireTargets(const char* targetName, CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	CBaseEntity* pTarget = nullptr;
	if (!targetName)
		return;

	ALERT(at_aiconsole, "Firing: (%s)\n", targetName);

	for (;;)
	{
		pTarget = util::FindEntityByTargetname(pTarget, targetName);
		if (pTarget == nullptr)
			break;

		if ((pTarget->v.flags & FL_KILLME) == 0) // Don't use dying ents
		{
			ALERT(at_aiconsole, "Found: %s, firing (%s)\n", STRING(pTarget->v.classname), targetName);
			pTarget->Use(pActivator, pCaller, useType, value);
		}
	}
}

LINK_ENTITY_TO_CLASS(DelayedUse, CBaseEntity);


void CBaseEntity::UseTargets(CBaseEntity* pActivator, USE_TYPE useType, float value)
{
	//
	// exit immediatly if we don't have a target or kill target
	//
	if (FStringNull(v.target) && FStringNull(m_iszKillTarget))
		return;

	//
	// check for a delay
	//
	if (m_flDelay != 0)
	{
		// create a temp object to fire at a later time
		CBaseEntity* pTemp = Entity::Create<CBaseEntity>();
		pTemp->v.classname = MAKE_STRING("DelayedUse");

		pTemp->v.nextthink = gpGlobals->time + m_flDelay;

		pTemp->SetThink(&CBaseEntity::DelayThink);

		// Save the useType
		pTemp->v.button = (int)useType;
		pTemp->m_iszKillTarget = m_iszKillTarget;
		pTemp->m_flDelay = 0; // prevent "recursion"
		pTemp->v.target = v.target;

		// HACKHACK
		// This wasn't in the release build of Half-Life.  We should have moved m_hActivator into this class
		// but changing member variable hierarchy would break save/restore without some ugly code.
		// This code is not as ugly as that code
		if (pActivator && pActivator->IsClient()) // If a player activates, then save it
		{
			pTemp->v.owner = pActivator->edict();
		}
		else
		{
			pTemp->v.owner = nullptr;
		}

		return;
	}

	//
	// kill the killtargets
	//

	if (!FStringNull(m_iszKillTarget))
	{
		CBaseEntity* pentKillTarget = nullptr;

		ALERT(at_aiconsole, "KillTarget: %s\n", STRING(m_iszKillTarget));
		pentKillTarget = util::FindEntityByTargetname(nullptr, STRING(m_iszKillTarget));
		while (pentKillTarget != nullptr)
		{
			pentKillTarget->Remove();

			ALERT(at_aiconsole, "killing %s\n", STRING(pentKillTarget->v.classname));
			pentKillTarget = util::FindEntityByClassname(pentKillTarget, STRING(m_iszKillTarget));
		}
	}

	//
	// fire targets
	//
	if (!FStringNull(v.target))
	{
		util::FireTargets(STRING(v.target), pActivator, this, useType, value);
	}
}

/*
QuakeEd only writes a single float for angles (bad idea), so up and down are
just constant angles.
*/
Vector util::SetMovedir(Vector& angles)
{
	Vector movedir = g_vecZero;

	if (angles == Vector(0, -1, 0))
	{
		movedir = Vector(0, 0, 1);
	}
	else if (angles == Vector(0, -2, 0))
	{
		movedir = Vector(0, 0, -1);
	}
	else
	{
		util::MakeVectors(angles);
		movedir = gpGlobals->v_forward;
	}

	angles = g_vecZero;

	return movedir;
}


void CBaseEntity::DelayThink()
{
	CBaseEntity* pActivator = nullptr;

	if (v.owner != nullptr) // A player activated this on delay
	{
		pActivator = v.owner->Get<CBaseEntity>();
	}
	// The use type is cached (and stashed) in v.button
	UseTargets(pActivator, (USE_TYPE)v.button, 0);
	Remove();
}


// Global Savedata for Toggle
#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CBaseToggle)
	DEFINE_FIELD(CBaseToggle, m_toggle_state, FIELD_INTEGER),
	DEFINE_FIELD(CBaseToggle, m_flActivateFinished, FIELD_TIME),
	DEFINE_FIELD(CBaseToggle, m_flMoveDistance, FIELD_FLOAT),
	DEFINE_FIELD(CBaseToggle, m_flWait, FIELD_FLOAT),
	DEFINE_FIELD(CBaseToggle, m_flLip, FIELD_FLOAT),
	DEFINE_FIELD(CBaseToggle, m_flTWidth, FIELD_FLOAT),
	DEFINE_FIELD(CBaseToggle, m_flTLength, FIELD_FLOAT),
	DEFINE_FIELD(CBaseToggle, m_vecPosition1, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CBaseToggle, m_vecPosition2, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CBaseToggle, m_vecAngle1, FIELD_VECTOR),
	DEFINE_FIELD(CBaseToggle, m_vecAngle2, FIELD_VECTOR),
	DEFINE_FIELD(CBaseToggle, m_cTriggersLeft, FIELD_INTEGER),
	DEFINE_FIELD(CBaseToggle, m_flHeight, FIELD_FLOAT),
	DEFINE_FIELD(CBaseToggle, m_hActivator, FIELD_EHANDLE),
	DEFINE_FIELD(CBaseToggle, m_pfnCallWhenMoveDone, FIELD_FUNCTION),
	DEFINE_FIELD(CBaseToggle, m_vecFinalDest, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CBaseToggle, m_vecFinalAngle, FIELD_VECTOR),
	DEFINE_FIELD(CBaseToggle, m_sMaster, FIELD_STRING),
	DEFINE_FIELD(CBaseToggle, m_bitsDamageInflict, FIELD_INTEGER),
END_SAVERESTORE(CBaseToggle, CBaseDelay)
#endif


bool CBaseToggle::EntvarsKeyvalue(KeyValueData* pkvd)
{
	return CBaseEntity::EntvarsKeyvalue(pkvd);
}


bool CBaseToggle::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "lip"))
	{
		m_flLip = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "master"))
	{
		m_sMaster = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "distance"))
	{
		m_flMoveDistance = atof(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}


void CBaseToggle::UseTargets(CBaseEntity* pActivator, USE_TYPE useType, float value)
{
	CBaseEntity::UseTargets(pActivator, useType, value);
}


/*
=============
LinearMove

calculate v.velocity and v.nextthink to reach vecDest from
v.origin traveling at flSpeed
===============
*/
void CBaseToggle::LinearMove(Vector vecDest, float flSpeed)
{
	m_vecFinalDest = vecDest;

	// Already there?
	if (vecDest == v.origin)
	{
		LinearMoveDone();
		return;
	}

	// set destdelta to the vector needed to move
	Vector vecDestDelta = vecDest - v.origin;

	// divide vector length by speed to get time to reach dest
	float flTravelTime = vecDestDelta.Length() / flSpeed;

	// set nextthink to trigger a call to LinearMoveDone when dest is reached
	v.nextthink = v.ltime + flTravelTime;
	SetThink(&CBaseToggle::LinearMoveDone);

	// scale the destdelta vector by the time spent traveling to get velocity
	v.velocity = vecDestDelta / flTravelTime;
}


/*
============
After moving, set origin to exact final destination, call "move done" function
============
*/
void CBaseToggle::LinearMoveDone()
{
	Vector delta = m_vecFinalDest - v.origin;
	float error = delta.Length();
	if (error > 0.03125)
	{
		LinearMove(m_vecFinalDest, 100);
		return;
	}

	SetOrigin(m_vecFinalDest);
	v.velocity = g_vecZero;
	v.nextthink = -1;
	if (m_pfnCallWhenMoveDone)
		(this->*m_pfnCallWhenMoveDone)();
}

bool CBaseToggle::IsLockedByMaster()
{
	return !FStringNull(m_sMaster) && !util::IsMasterTriggered(m_sMaster, m_hActivator);
}

/*
=============
AngularMove

calculate v.velocity and v.nextthink to reach vecDest from
v.origin traveling at flSpeed
Just like LinearMove, but rotational.
===============
*/
void CBaseToggle::AngularMove(Vector vecDestAngle, float flSpeed)
{
	m_vecFinalAngle = vecDestAngle;

	// Already there?
	if (vecDestAngle == v.angles)
	{
		AngularMoveDone();
		return;
	}

	// set destdelta to the vector needed to move
	Vector vecDestDelta = vecDestAngle - v.angles;

	// divide by speed to get time to reach dest
	float flTravelTime = vecDestDelta.Length() / flSpeed;

	// set nextthink to trigger a call to AngularMoveDone when dest is reached
	v.nextthink = v.ltime + flTravelTime;
	SetThink(&CBaseToggle::AngularMoveDone);

	// scale the destdelta vector by the time spent traveling to get velocity
	v.avelocity = vecDestDelta / flTravelTime;
}


/*
============
After rotating, set angle to exact final angle, call "move done" function
============
*/
void CBaseToggle::AngularMoveDone()
{
	v.angles = m_vecFinalAngle;
	v.avelocity = g_vecZero;
	v.nextthink = -1;
	if (m_pfnCallWhenMoveDone)
		(this->*m_pfnCallWhenMoveDone)();
}


float CBaseToggle::AxisValue(int flags, const Vector& angles)
{
	if (FBitSet(flags, SF_DOOR_ROTATE_Z))
		return angles.z;
	if (FBitSet(flags, SF_DOOR_ROTATE_X))
		return angles.x;

	return angles.y;
}


void CBaseToggle::AxisDir(CBaseEntity* entity)
{
	if (FBitSet(entity->v.spawnflags, SF_DOOR_ROTATE_Z))
		entity->v.movedir = Vector(0, 0, 1); // around z-axis
	else if (FBitSet(entity->v.spawnflags, SF_DOOR_ROTATE_X))
		entity->v.movedir = Vector(1, 0, 0); // around x-axis
	else
		entity->v.movedir = Vector(0, 1, 0); // around y-axis
}


float CBaseToggle::AxisDelta(int flags, const Vector& angle1, const Vector& angle2)
{
	if (FBitSet(flags, SF_DOOR_ROTATE_Z))
		return angle1.z - angle2.z;

	if (FBitSet(flags, SF_DOOR_ROTATE_X))
		return angle1.x - angle2.x;

	return angle1.y - angle2.y;
}
