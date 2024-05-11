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
//
// ========================== PATH_CORNER ===========================
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "trains.h"
#include "saverestore.h"

class CPathCorner : public CPointEntity
{
public:
	CPathCorner(Entity* containingEntity) : CPointEntity(containingEntity) {}

	DECLARE_SAVERESTORE()

	bool Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	float GetDelay() override { return m_flWait; }

private:
	float m_flWait;
};

LINK_ENTITY_TO_CLASS(path_corner, CPathCorner);

// Global Savedata for Delay
#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CPathCorner)
	DEFINE_FIELD(CPathCorner, m_flWait, FIELD_FLOAT),
END_SAVERESTORE(CPathCorner, CPointEntity)
#endif

//
// Cache user-entity-field values until spawn is called.
//
bool CPathCorner::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}


bool CPathCorner::Spawn()
{
	if (FStringNull(v.targetname))
	{
		ALERT(at_console, "path_corner without a targetname\n");
		return false;
	}
	return true;
}


#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CPathTrack)
	DEFINE_FIELD(CPathTrack, m_length, FIELD_FLOAT),
	DEFINE_FIELD(CPathTrack, m_pnext, FIELD_CLASSPTR),
	DEFINE_FIELD(CPathTrack, m_paltpath, FIELD_CLASSPTR),
	DEFINE_FIELD(CPathTrack, m_pprevious, FIELD_CLASSPTR),
	DEFINE_FIELD(CPathTrack, m_altName, FIELD_STRING),
END_SAVERESTORE(CPathTrack, CBaseEntity)
#endif

LINK_ENTITY_TO_CLASS(path_track, CPathTrack);

//
// Cache user-entity-field values until spawn is called.
//
bool CPathTrack::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "altpath"))
	{
		m_altName = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

void CPathTrack::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	bool on;

	// Use toggles between two paths
	if (m_paltpath)
	{
		on = !FBitSet(v.spawnflags, SF_PATH_ALTERNATE);
		if (ShouldToggle(useType, on))
		{
			if (on)
				SetBits(v.spawnflags, SF_PATH_ALTERNATE);
			else
				ClearBits(v.spawnflags, SF_PATH_ALTERNATE);
		}
	}
	else // Use toggles between enabled/disabled
	{
		on = !FBitSet(v.spawnflags, SF_PATH_DISABLED);

		if (ShouldToggle(useType, on))
		{
			if (on)
				SetBits(v.spawnflags, SF_PATH_DISABLED);
			else
				ClearBits(v.spawnflags, SF_PATH_DISABLED);
		}
	}
}


void CPathTrack::Link()
{
	Entity* pentTarget;

	if (!FStringNull(v.target))
	{
		pentTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(v.target));
		if (pentTarget != nullptr)
		{
			m_pnext = pentTarget->Get<CPathTrack>();

			if (m_pnext) // If no next pointer, this is the end of a path
			{
				m_pnext->SetPrevious(this);
			}
		}
		else
			ALERT(at_console, "Dead end link %s\n", STRING(v.target));
	}

	// Find "alternate" path
	if (!FStringNull(m_altName))
	{
		pentTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(m_altName));
		if (pentTarget != nullptr)
		{
			m_paltpath = pentTarget->Get<CPathTrack>();

			if (m_paltpath) // If no next pointer, this is the end of a path
			{
				m_paltpath->SetPrevious(this);
			}
		}
	}
}


bool CPathTrack::Spawn()
{
	v.solid = SOLID_TRIGGER;
	SetSize(Vector(-8, -8, -8), Vector(8, 8, 8));

	m_pnext = NULL;
	m_pprevious = NULL;
// DEBUGGING CODE
#if PATH_SPARKLE_DEBUG
	SetThink(Sparkle);
	v.nextthink = gpGlobals->time + 0.5;
#endif

	return true;
}


void CPathTrack::Activate()
{
	if (!FStringNull(v.targetname)) // Link to next, and back-link
		Link();
}

CPathTrack* CPathTrack::ValidPath(CPathTrack* ppath, bool testFlag)
{
	if (!ppath)
		return NULL;

	if (testFlag && FBitSet(ppath->v.spawnflags, SF_PATH_DISABLED))
		return NULL;

	return ppath;
}


void CPathTrack::Project(CPathTrack* pstart, CPathTrack* pend, Vector* origin, float dist)
{
	if (pstart && pend)
	{
		Vector dir = (pend->v.origin - pstart->v.origin);
		dir = dir.Normalize();
		*origin = pend->v.origin + dir * dist;
	}
}

CPathTrack* CPathTrack::GetNext()
{
	if (m_paltpath && FBitSet(v.spawnflags, SF_PATH_ALTERNATE) && !FBitSet(v.spawnflags, SF_PATH_ALTREVERSE))
		return m_paltpath;

	return m_pnext;
}



CPathTrack* CPathTrack::GetPrevious()
{
	if (m_paltpath && FBitSet(v.spawnflags, SF_PATH_ALTERNATE) && FBitSet(v.spawnflags, SF_PATH_ALTREVERSE))
		return m_paltpath;

	return m_pprevious;
}



void CPathTrack::SetPrevious(CPathTrack* pprev)
{
	// Only set previous if this isn't my alternate path
	if (pprev && !FStrEq(STRING(pprev->v.targetname), STRING(m_altName)))
		m_pprevious = pprev;
}


// Assumes this is ALWAYS enabled
CPathTrack* CPathTrack::LookAhead(Vector* origin, float dist, bool move)
{
	CPathTrack* pcurrent;
	float originalDist = dist;

	pcurrent = this;
	Vector currentPos = *origin;

	if (dist < 0) // Travelling backwards through path
	{
		dist = -dist;
		while (dist > 0)
		{
			Vector dir = pcurrent->v.origin - currentPos;
			float length = dir.Length();
			if (0 == length)
			{
				if (!ValidPath(pcurrent->GetPrevious(), move)) // If there is no previous node, or it's disabled, return now.
				{
					if (!move)
						Project(pcurrent->GetNext(), pcurrent, origin, dist);
					return NULL;
				}
				pcurrent = pcurrent->GetPrevious();
			}
			else if (length > dist) // enough left in this path to move
			{
				*origin = currentPos + (dir * (dist / length));
				return pcurrent;
			}
			else
			{
				dist -= length;
				currentPos = pcurrent->v.origin;
				*origin = currentPos;
				if (!ValidPath(pcurrent->GetPrevious(), move)) // If there is no previous node, or it's disabled, return now.
					return NULL;

				pcurrent = pcurrent->GetPrevious();
			}
		}
		*origin = currentPos;
		return pcurrent;
	}
	else
	{
		while (dist > 0)
		{
			if (!ValidPath(pcurrent->GetNext(), move)) // If there is no next node, or it's disabled, return now.
			{
				if (!move)
					Project(pcurrent->GetPrevious(), pcurrent, origin, dist);
				return NULL;
			}
			Vector dir = pcurrent->GetNext()->v.origin - currentPos;
			float length = dir.Length();
			if (0 == length && !ValidPath(pcurrent->GetNext()->GetNext(), move))
			{
				if (dist == originalDist) // HACK -- up against a dead end
					return NULL;
				return pcurrent;
			}
			if (length > dist) // enough left in this path to move
			{
				*origin = currentPos + (dir * (dist / length));
				return pcurrent;
			}
			else
			{
				dist -= length;
				currentPos = pcurrent->GetNext()->v.origin;
				pcurrent = pcurrent->GetNext();
				*origin = currentPos;
			}
		}
		*origin = currentPos;
	}

	return pcurrent;
}


// Assumes this is ALWAYS enabled
CPathTrack* CPathTrack::Nearest(Vector origin)
{
	int deadCount;
	float minDist, dist;
	Vector delta;
	CPathTrack *ppath, *pnearest;


	delta = origin - v.origin;
	delta.z = 0;
	minDist = delta.Length();
	pnearest = this;
	ppath = GetNext();

	// Hey, I could use the old 2 racing pointers solution to this, but I'm lazy :)
	deadCount = 0;
	while (ppath && ppath != this)
	{
		deadCount++;
		if (deadCount > 9999)
		{
			ALERT(at_error, "Bad sequence of path_tracks from %s", STRING(v.targetname));
			return NULL;
		}
		delta = origin - ppath->v.origin;
		delta.z = 0;
		dist = delta.Length();
		if (dist < minDist)
		{
			minDist = dist;
			pnearest = ppath;
		}
		ppath = ppath->GetNext();
	}
	return pnearest;
}


// DEBUGGING CODE
#if PATH_SPARKLE_DEBUG
void CPathTrack::Sparkle()
{

	v.nextthink = gpGlobals->time + 0.2;
	if (FBitSet(v.spawnflags, SF_PATH_DISABLED))
		util::ParticleEffect(v.origin, Vector(0, 0, 100), 210, 10);
	else
		util::ParticleEffect(v.origin, Vector(0, 0, 100), 84, 10);
}
#endif
