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

===== plats.cpp ========================================================

  spawn, think, and touch functions for trains, etc

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "trains.h"
#include "saverestore.h"

static void PlatSpawnInsideTrigger(entvars_t* pevPlatform);

#define SF_PLAT_TOGGLE 0x0001

// Trains
#define SF_TRAIN_WAIT_RETRIGGER 1
#define SF_TRAIN_START_ON 4 // Train is initially moving
#define SF_TRAIN_PASSABLE 8 // Train is not solid -- used to make water trains

class CBasePlatTrain : public CBaseToggle
{
public:
	DECLARE_SAVERESTORE()

	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	bool KeyValue(KeyValueData* pkvd) override;
	void Precache() override;

	// This is done to fix spawn flag collisions between this class and a derived class
	virtual bool IsTogglePlat() { return (pev->spawnflags & SF_PLAT_TOGGLE) != 0; }

	byte m_bMoveSnd; // sound a plat makes while moving
	byte m_bStopSnd; // sound a plat makes when it stops
	float m_volume;	 // Sound volume
};

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CBasePlatTrain)
	DEFINE_FIELD(CBasePlatTrain, m_bMoveSnd, FIELD_CHARACTER),
	DEFINE_FIELD(CBasePlatTrain, m_bStopSnd, FIELD_CHARACTER),
	DEFINE_FIELD(CBasePlatTrain, m_volume, FIELD_FLOAT),
END_SAVERESTORE(CBasePlatTrain, CBaseToggle)
#endif

bool CBasePlatTrain::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "lip"))
	{
		m_flLip = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "height"))
	{
		m_flHeight = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "rotation"))
	{
		m_vecFinalAngle.x = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "movesnd"))
	{
		m_bMoveSnd = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "stopsnd"))
	{
		m_bStopSnd = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "volume"))
	{
		m_volume = atof(pkvd->szValue);
		return true;
	}

	return CBaseToggle::KeyValue(pkvd);
}

#define noiseMoving noise
#define noiseArrived noise1

void CBasePlatTrain::Precache()
{
	// set the plat's "in-motion" sound
	switch (m_bMoveSnd)
	{
	case 0:
		pev->noiseMoving = MAKE_STRING("common/null.wav");
		break;
	case 1:
		PRECACHE_SOUND("plats/bigmove1.wav");
		pev->noiseMoving = MAKE_STRING("plats/bigmove1.wav");
		break;
	case 2:
		PRECACHE_SOUND("plats/bigmove2.wav");
		pev->noiseMoving = MAKE_STRING("plats/bigmove2.wav");
		break;
	case 3:
		PRECACHE_SOUND("plats/elevmove1.wav");
		pev->noiseMoving = MAKE_STRING("plats/elevmove1.wav");
		break;
	case 4:
		PRECACHE_SOUND("plats/elevmove2.wav");
		pev->noiseMoving = MAKE_STRING("plats/elevmove2.wav");
		break;
	case 5:
		PRECACHE_SOUND("plats/elevmove3.wav");
		pev->noiseMoving = MAKE_STRING("plats/elevmove3.wav");
		break;
	case 6:
		PRECACHE_SOUND("plats/freightmove1.wav");
		pev->noiseMoving = MAKE_STRING("plats/freightmove1.wav");
		break;
	case 7:
		PRECACHE_SOUND("plats/freightmove2.wav");
		pev->noiseMoving = MAKE_STRING("plats/freightmove2.wav");
		break;
	case 8:
		PRECACHE_SOUND("plats/heavymove1.wav");
		pev->noiseMoving = MAKE_STRING("plats/heavymove1.wav");
		break;
	case 9:
		PRECACHE_SOUND("plats/rackmove1.wav");
		pev->noiseMoving = MAKE_STRING("plats/rackmove1.wav");
		break;
	case 10:
		PRECACHE_SOUND("plats/railmove1.wav");
		pev->noiseMoving = MAKE_STRING("plats/railmove1.wav");
		break;
	case 11:
		PRECACHE_SOUND("plats/squeekmove1.wav");
		pev->noiseMoving = MAKE_STRING("plats/squeekmove1.wav");
		break;
	case 12:
		PRECACHE_SOUND("plats/talkmove1.wav");
		pev->noiseMoving = MAKE_STRING("plats/talkmove1.wav");
		break;
	case 13:
		PRECACHE_SOUND("plats/talkmove2.wav");
		pev->noiseMoving = MAKE_STRING("plats/talkmove2.wav");
		break;
	default:
		pev->noiseMoving = MAKE_STRING("common/null.wav");
		break;
	}

	// set the plat's 'reached destination' stop sound
	switch (m_bStopSnd)
	{
	case 0:
		pev->noiseArrived = MAKE_STRING("common/null.wav");
		break;
	case 1:
		PRECACHE_SOUND("plats/bigstop1.wav");
		pev->noiseArrived = MAKE_STRING("plats/bigstop1.wav");
		break;
	case 2:
		PRECACHE_SOUND("plats/bigstop2.wav");
		pev->noiseArrived = MAKE_STRING("plats/bigstop2.wav");
		break;
	case 3:
		PRECACHE_SOUND("plats/freightstop1.wav");
		pev->noiseArrived = MAKE_STRING("plats/freightstop1.wav");
		break;
	case 4:
		PRECACHE_SOUND("plats/heavystop2.wav");
		pev->noiseArrived = MAKE_STRING("plats/heavystop2.wav");
		break;
	case 5:
		PRECACHE_SOUND("plats/rackstop1.wav");
		pev->noiseArrived = MAKE_STRING("plats/rackstop1.wav");
		break;
	case 6:
		PRECACHE_SOUND("plats/railstop1.wav");
		pev->noiseArrived = MAKE_STRING("plats/railstop1.wav");
		break;
	case 7:
		PRECACHE_SOUND("plats/squeekstop1.wav");
		pev->noiseArrived = MAKE_STRING("plats/squeekstop1.wav");
		break;
	case 8:
		PRECACHE_SOUND("plats/talkstop1.wav");
		pev->noiseArrived = MAKE_STRING("plats/talkstop1.wav");
		break;

	default:
		pev->noiseArrived = MAKE_STRING("common/null.wav");
		break;
	}
}

//
//====================== PLAT code ====================================================
//


#define noiseMovement noise
#define noiseStopMoving noise1

class CFuncPlat : public CBasePlatTrain
{
public:
	bool Spawn() override;
	void Precache() override;
	void Setup();

	void Blocked(CBaseEntity* pOther) override;


	void EXPORT PlatUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	void EXPORT CallGoDown() { GoDown(); }
	void EXPORT CallHitTop() { HitTop(); }
	void EXPORT CallHitBottom() { HitBottom(); }

	virtual void GoUp();
	virtual void GoDown();
	virtual void HitTop();
	virtual void HitBottom();
};
LINK_ENTITY_TO_CLASS(func_plat, CFuncPlat);


class CPlatTrigger : public CBaseToggle
{
public:
#ifdef HALFLIFE_SAVERESTORE
	int ObjectCaps() override { return (CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE; }
#else
	int ObjectCaps() override { return (CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }
#endif
	void SpawnInsideTrigger(CFuncPlat* pPlatform);
	void Touch(CBaseEntity* pOther) override;
	EHANDLE m_hPlatform;
};


void CFuncPlat::Setup()
{
	//pev->noiseMovement = MAKE_STRING("plats/platmove1.wav");
	//pev->noiseStopMoving = MAKE_STRING("plats/platstop1.wav");

	if (m_flTLength == 0)
		m_flTLength = 80;
	if (m_flTWidth == 0)
		m_flTWidth = 10;

	pev->angles = g_vecZero;

	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	SetOrigin(pev->origin); // set size and link into world
	SetSize(pev->mins, pev->maxs);
	SetModel(STRING(pev->model));

	// vecPosition1 is the top position, vecPosition2 is the bottom
	m_vecPosition1 = pev->origin;
	m_vecPosition2 = pev->origin;
	if (m_flHeight != 0)
		m_vecPosition2.z = pev->origin.z - m_flHeight;
	else
		m_vecPosition2.z = pev->origin.z - pev->size.z + 8;
	if (pev->speed == 0)
		pev->speed = 150;

	if (m_volume == 0)
		m_volume = 0.85;
}


void CFuncPlat::Precache()
{
	CBasePlatTrain::Precache();
	//PRECACHE_SOUND("plats/platmove1.wav");
	//PRECACHE_SOUND("plats/platstop1.wav");
	if (!IsTogglePlat())
		PlatSpawnInsideTrigger(pev); // the "start moving" trigger
}


bool CFuncPlat::Spawn()
{
	Setup();

	Precache();

	// If this platform is the target of some button, it starts at the TOP position,
	// and is brought down by that button.  Otherwise, it starts at BOTTOM.
	if (!FStringNull(pev->targetname))
	{
		SetOrigin(m_vecPosition1);
		m_toggle_state = TS_AT_TOP;
		SetUse(&CFuncPlat::PlatUse);
	}
	else
	{
		SetOrigin(m_vecPosition2);
		m_toggle_state = TS_AT_BOTTOM;
	}

	return true;
}



static void PlatSpawnInsideTrigger(entvars_t* pevPlatform)
{
	GetClassPtr((CPlatTrigger*)NULL)->SpawnInsideTrigger(GetClassPtr((CFuncPlat*)pevPlatform));
}


//
// Create a trigger entity for a platform.
//
void CPlatTrigger::SpawnInsideTrigger(CFuncPlat* pPlatform)
{
	m_hPlatform = pPlatform;
	// Create trigger entity, "point" it at the owning platform, give it a touch method
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->origin = pPlatform->pev->origin;

	// Establish the trigger field's size
	Vector vecTMin = pPlatform->pev->mins + Vector(25, 25, 0);
	Vector vecTMax = pPlatform->pev->maxs + Vector(25, 25, 8);
	vecTMin.z = vecTMax.z - (pPlatform->m_vecPosition1.z - pPlatform->m_vecPosition2.z + 8);
	if (pPlatform->pev->size.x <= 50)
	{
		vecTMin.x = (pPlatform->pev->mins.x + pPlatform->pev->maxs.x) / 2;
		vecTMax.x = vecTMin.x + 1;
	}
	if (pPlatform->pev->size.y <= 50)
	{
		vecTMin.y = (pPlatform->pev->mins.y + pPlatform->pev->maxs.y) / 2;
		vecTMax.y = vecTMin.y + 1;
	}
	SetSize(vecTMin, vecTMax);
}


//
// When the platform's trigger field is touched, the platform ???
//
void CPlatTrigger::Touch(CBaseEntity* pOther)
{
	//Platform was removed, remove trigger
	if (!m_hPlatform || !m_hPlatform->pev)
	{
		Remove();
		return;
	}

	// Ignore touches by non-players
	entvars_t* pevToucher = pOther->pev;
	if (!FClassnameIs(pevToucher, "player"))
		return;

	// Ignore touches by corpses
	if (!pOther->IsAlive())
		return;

	CFuncPlat* platform = static_cast<CFuncPlat*>(static_cast<CBaseEntity*>(m_hPlatform));

	// Make linked platform go up/down.
	if (platform->m_toggle_state == TS_AT_BOTTOM)
		platform->GoUp();
	else if (platform->m_toggle_state == TS_AT_TOP)
		platform->pev->nextthink = platform->pev->ltime + 1; // delay going down
}


//
// Used by UseTargets, when a platform is the target of a button.
// Start bringing platform down.
//
void CFuncPlat::PlatUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (IsTogglePlat())
	{
		// Top is off, bottom is on
		bool on = (m_toggle_state == TS_AT_BOTTOM) ? true : false;

		if (!ShouldToggle(useType, on))
			return;

		if (m_toggle_state == TS_AT_TOP)
			GoDown();
		else if (m_toggle_state == TS_AT_BOTTOM)
			GoUp();
	}
	else
	{
		SetUse(NULL);

		if (m_toggle_state == TS_AT_TOP)
			GoDown();
	}
}


//
// Platform is at top, now starts moving down.
//
void CFuncPlat::GoDown()
{
	if (!FStringNull(pev->noiseMovement))
		EmitSound(STRING(pev->noiseMovement), CHAN_STATIC, m_volume);

	m_toggle_state = TS_GOING_DOWN;
	SetMoveDone(&CFuncPlat::CallHitBottom);
	LinearMove(m_vecPosition2, pev->speed);
}


//
// Platform has hit bottom.  Stops and waits forever.
//
void CFuncPlat::HitBottom()
{
	if (!FStringNull(pev->noiseMovement))
		StopSound(STRING(pev->noiseMovement),  CHAN_STATIC);

	if (!FStringNull(pev->noiseStopMoving))
		EmitSound(STRING(pev->noiseStopMoving), CHAN_WEAPON, m_volume);

	m_toggle_state = TS_AT_BOTTOM;
}


//
// Platform is at bottom, now starts moving up
//
void CFuncPlat::GoUp()
{
	if (!FStringNull(pev->noiseMovement))
		EmitSound(STRING(pev->noiseMovement), CHAN_STATIC, m_volume);

	m_toggle_state = TS_GOING_UP;
	SetMoveDone(&CFuncPlat::CallHitTop);
	LinearMove(m_vecPosition1, pev->speed);
}


//
// Platform has hit top.  Pauses, then starts back down again.
//
void CFuncPlat::HitTop()
{
	if (!FStringNull(pev->noiseMovement))
		StopSound(STRING(pev->noiseMovement), CHAN_STATIC);

	if (!FStringNull(pev->noiseStopMoving))
		EmitSound(STRING(pev->noiseStopMoving), CHAN_WEAPON, m_volume);

	m_toggle_state = TS_AT_TOP;

	if (!IsTogglePlat())
	{
		// After a delay, the platform will automatically start going down again.
		SetThink(&CFuncPlat::CallGoDown);
		pev->nextthink = pev->ltime + 3;
	}
}


void CFuncPlat::Blocked(CBaseEntity* pOther)
{
	ALERT(at_aiconsole, "%s Blocked by %s\n", STRING(pev->classname), STRING(pOther->pev->classname));
	// Hurt the blocker a little
	pOther->TakeDamage(this, this, 1, DMG_CRUSH);

	if (!FStringNull(pev->noiseMovement))
		StopSound(STRING(pev->noiseMovement), CHAN_STATIC);

	// Send the platform back where it came from
	if (m_toggle_state == TS_GOING_UP)
		GoDown();
	else if (m_toggle_state == TS_GOING_DOWN)
		GoUp();
}


class CFuncPlatRot : public CFuncPlat
{
public:
	DECLARE_SAVERESTORE()

	bool Spawn() override;
	void SetupRotation();

	void GoUp() override;
	void GoDown() override;
	void HitTop() override;
	void HitBottom() override;

	void RotMove(Vector& destAngle, float time);

	Vector m_end, m_start;
};
LINK_ENTITY_TO_CLASS(func_platrot, CFuncPlatRot);

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CFuncPlatRot)
	DEFINE_FIELD(CFuncPlatRot, m_end, FIELD_VECTOR),
	DEFINE_FIELD(CFuncPlatRot, m_start, FIELD_VECTOR),
END_SAVERESTORE(CFuncPlatRot, CFuncPlat)
#endif


void CFuncPlatRot::SetupRotation()
{
	if (m_vecFinalAngle.x != 0) // This plat rotates too!
	{
		CBaseToggle::AxisDir(pev);
		m_start = pev->angles;
		m_end = pev->angles + pev->movedir * m_vecFinalAngle.x;
	}
	else
	{
		m_start = g_vecZero;
		m_end = g_vecZero;
	}
	if (!FStringNull(pev->targetname)) // Start at top
	{
		pev->angles = m_end;
	}
}


bool CFuncPlatRot::Spawn()
{
	if (!CFuncPlat::Spawn())
	{
		return false;
	}
	SetupRotation();
	return true;
}

void CFuncPlatRot::GoDown()
{
	CFuncPlat::GoDown();
	RotMove(m_start, pev->nextthink - pev->ltime);
}


//
// Platform has hit bottom.  Stops and waits forever.
//
void CFuncPlatRot::HitBottom()
{
	CFuncPlat::HitBottom();
	pev->avelocity = g_vecZero;
	pev->angles = m_start;
}


//
// Platform is at bottom, now starts moving up
//
void CFuncPlatRot::GoUp()
{
	CFuncPlat::GoUp();
	RotMove(m_end, pev->nextthink - pev->ltime);
}


//
// Platform has hit top.  Pauses, then starts back down again.
//
void CFuncPlatRot::HitTop()
{
	CFuncPlat::HitTop();
	pev->avelocity = g_vecZero;
	pev->angles = m_end;
}


void CFuncPlatRot::RotMove(Vector& destAngle, float time)
{
	// set destdelta to the vector needed to move
	Vector vecDestDelta = destAngle - pev->angles;

	// Travel time is so short, we're practically there already;  so make it so.
	if (time >= 0.1)
		pev->avelocity = vecDestDelta / time;
	else
	{
		pev->avelocity = vecDestDelta;
		pev->nextthink = pev->ltime + 1;
	}
}


//
//====================== TRAIN code ==================================================
//

class CFuncTrain : public CBasePlatTrain
{
public:
	DECLARE_SAVERESTORE()

	bool Spawn() override;
	void Precache() override;
	void Activate() override;
	void OverrideReset() override;

	void Blocked(CBaseEntity* pOther) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	bool KeyValue(KeyValueData* pkvd) override;


	void EXPORT Wait();
	void EXPORT Next();

	entvars_t* m_pevCurrentTarget;
	int m_sounds;
	bool m_activated;
};

LINK_ENTITY_TO_CLASS(func_train, CFuncTrain);

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CFuncTrain)
	DEFINE_FIELD(CFuncTrain, m_sounds, FIELD_INTEGER),
	DEFINE_FIELD(CFuncTrain, m_pevCurrentTarget, FIELD_EVARS),
	DEFINE_FIELD(CFuncTrain, m_activated, FIELD_BOOLEAN),
END_SAVERESTORE(CFuncTrain, CBasePlatTrain)
#endif


bool CFuncTrain::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "sounds"))
	{
		m_sounds = atoi(pkvd->szValue);
		return true;
	}

	return CBasePlatTrain::KeyValue(pkvd);
}


void CFuncTrain::Blocked(CBaseEntity* pOther)

{
	if (gpGlobals->time < m_flActivateFinished)
		return;

	m_flActivateFinished = gpGlobals->time + 0.5;

	pOther->TakeDamage(this, this, pev->dmg, DMG_CRUSH);
}


void CFuncTrain::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if ((pev->spawnflags & SF_TRAIN_WAIT_RETRIGGER) != 0)
	{
		// Move toward my target
		pev->spawnflags &= ~SF_TRAIN_WAIT_RETRIGGER;
		Next();
	}
	else
	{
		pev->spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
		// Pop back to last target if it's available
		if (pev->enemy)
			pev->target = pev->enemy->v.targetname;
		pev->nextthink = 0;
		pev->velocity = g_vecZero;
		if (!FStringNull(pev->noiseStopMoving))
			EmitSound(STRING(pev->noiseStopMoving), CHAN_VOICE, m_volume);
	}
}


void CFuncTrain::Wait()
{
	// Fire the pass target if there is one
	if (!FStringNull(m_pevCurrentTarget->message))
	{
		util::FireTargets(STRING(m_pevCurrentTarget->message), this, this, USE_TOGGLE, 0);
		if (FBitSet(m_pevCurrentTarget->spawnflags, SF_CORNER_FIREONCE))
			m_pevCurrentTarget->message = 0;
	}

	// need pointer to LAST target.
	if (FBitSet(m_pevCurrentTarget->spawnflags, SF_TRAIN_WAIT_RETRIGGER) || (pev->spawnflags & SF_TRAIN_WAIT_RETRIGGER) != 0)
	{
		pev->spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
		// clear the sound channel.
		if (!FStringNull(pev->noiseMovement))
			StopSound(STRING(pev->noiseMovement), CHAN_STATIC);
		if (!FStringNull(pev->noiseStopMoving))
			EmitSound(STRING(pev->noiseStopMoving), CHAN_VOICE, m_volume);
		pev->nextthink = 0;
		return;
	}

	// ALERT ( at_console, "%f\n", m_flWait );

	if (m_flWait != 0)
	{ // -1 wait will wait forever!
		pev->nextthink = pev->ltime + m_flWait;
		if (!FStringNull(pev->noiseMovement))
			StopSound(STRING(pev->noiseMovement), CHAN_STATIC);
		if (!FStringNull(pev->noiseStopMoving))
			EmitSound(STRING(pev->noiseStopMoving), CHAN_VOICE, m_volume);
		SetThink(&CFuncTrain::Next);
	}
	else
	{
		Next(); // do it RIGHT now!
	}
}


//
// Train next - path corner needs to change to next target
//
void CFuncTrain::Next()
{
	CBaseEntity* pTarg;


	// now find our next target
	pTarg = GetNextTarget();

	if (!pTarg)
	{
		if (!FStringNull(pev->noiseMovement))
			StopSound(STRING(pev->noiseMovement), CHAN_STATIC);
		// Play stop sound
		if (!FStringNull(pev->noiseStopMoving))
			EmitSound(STRING(pev->noiseStopMoving), CHAN_VOICE, m_volume);
		return;
	}

	// Save last target in case we need to find it again
	pev->message = pev->target;

	pev->target = pTarg->pev->target;
	m_flWait = pTarg->GetDelay();

	if (m_pevCurrentTarget && m_pevCurrentTarget->speed != 0)
	{ // don't copy speed from target if it is 0 (uninitialized)
		pev->speed = m_pevCurrentTarget->speed;
		ALERT(at_aiconsole, "Train %s speed to %4.2f\n", STRING(pev->targetname), pev->speed);
	}
	m_pevCurrentTarget = pTarg->pev; // keep track of this since path corners change our target for us.

	pev->enemy = pTarg->edict(); //hack

	if (FBitSet(m_pevCurrentTarget->spawnflags, SF_CORNER_TELEPORT))
	{
		// Path corner has indicated a teleport to the next corner.
		SetBits(pev->effects, EF_NOINTERP);
		SetOrigin(pTarg->pev->origin - (pev->mins + pev->maxs) * 0.5);
		Wait(); // Get on with doing the next path corner.
	}
	else
	{
		// Normal linear move.

		// CHANGED this from CHAN_VOICE to CHAN_STATIC around OEM beta time because trains should
		// use CHAN_STATIC for their movement sounds to prevent sound field problems.
		// this is not a hack or temporary fix, this is how things should be. (sjb).
		if (!FStringNull(pev->noiseMovement))
			StopSound(STRING(pev->noiseMovement), CHAN_STATIC);
		if (!FStringNull(pev->noiseMovement))
			EmitSound(STRING(pev->noiseMovement), CHAN_STATIC, m_volume);
		ClearBits(pev->effects, EF_NOINTERP);
		SetMoveDone(&CFuncTrain::Wait);
		LinearMove(pTarg->pev->origin - (pev->mins + pev->maxs) * 0.5, pev->speed);
	}
}


void CFuncTrain::Activate()
{
	// Not yet active, so teleport to first target
	if (!m_activated)
	{
		m_activated = true;
		entvars_t* pevTarg = VARS(FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target)));

		pev->target = pevTarg->target;
		m_pevCurrentTarget = pevTarg; // keep track of this since path corners change our target for us.

		SetOrigin(pevTarg->origin - (pev->mins + pev->maxs) * 0.5);

		if (FStringNull(pev->targetname))
		{ // not triggered, so start immediately
			pev->nextthink = pev->ltime + 0.1;
			SetThink(&CFuncTrain::Next);
		}
		else
			pev->spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
	}
}


bool CFuncTrain::Spawn()
{
	Precache();
	if (pev->speed == 0)
		pev->speed = 100;

	if (FStringNull(pev->target))
		ALERT(at_console, "FuncTrain with no target");

	if (pev->dmg == 0)
		pev->dmg = 2;

	pev->movetype = MOVETYPE_PUSH;

	if (FBitSet(pev->spawnflags, SF_TRACKTRAIN_PASSABLE))
		pev->solid = SOLID_NOT;
	else
		pev->solid = SOLID_BSP;

	SetModel(STRING(pev->model));
	SetSize(pev->mins, pev->maxs);
	SetOrigin(pev->origin);

	m_activated = false;

	if (m_volume == 0)
		m_volume = 0.85;

	return true;
}


void CFuncTrain::Precache()
{
	CBasePlatTrain::Precache();

#if 0 // obsolete
	// otherwise use preset sound
	switch (m_sounds)
	{
	case 0:
		pev->noise = 0;
		pev->noise1 = 0;
		break;

	case 1:
		PRECACHE_SOUND ("plats/train2.wav");
		PRECACHE_SOUND ("plats/train1.wav");
		pev->noise = MAKE_STRING("plats/train2.wav");
		pev->noise1 = MAKE_STRING("plats/train1.wav");
		break;

	case 2:
		PRECACHE_SOUND ("plats/platmove1.wav");
		PRECACHE_SOUND ("plats/platstop1.wav");
		pev->noise = MAKE_STRING("plats/platstop1.wav");
		pev->noise1 = MAKE_STRING("plats/platmove1.wav");
		break;
	}
#endif
}


void CFuncTrain::OverrideReset()
{
	CBaseEntity* pTarg;

	// Are we moving?
	if (pev->velocity != g_vecZero && pev->nextthink != 0)
	{
		pev->target = pev->message;
		// now find our next target
		pTarg = GetNextTarget();
		if (!pTarg)
		{
			pev->nextthink = 0;
			pev->velocity = g_vecZero;
		}
		else // Keep moving for 0.1 secs, then find path_corner again and restart
		{
			SetThink(&CFuncTrain::Next);
			pev->nextthink = pev->ltime + 0.1;
		}
	}
}




// ---------------------------------------------------------------------
//
// Track Train
//
// ---------------------------------------------------------------------

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CFuncTrackTrain)
	DEFINE_FIELD(CFuncTrackTrain, m_ppath, FIELD_CLASSPTR),
	DEFINE_FIELD(CFuncTrackTrain, m_length, FIELD_FLOAT),
	DEFINE_FIELD(CFuncTrackTrain, m_height, FIELD_FLOAT),
	DEFINE_FIELD(CFuncTrackTrain, m_speed, FIELD_FLOAT),
	DEFINE_FIELD(CFuncTrackTrain, m_dir, FIELD_FLOAT),
	DEFINE_FIELD(CFuncTrackTrain, m_startSpeed, FIELD_FLOAT),
	DEFINE_FIELD(CFuncTrackTrain, m_controlMins, FIELD_VECTOR),
	DEFINE_FIELD(CFuncTrackTrain, m_controlMaxs, FIELD_VECTOR),
	DEFINE_FIELD(CFuncTrackTrain, m_sounds, FIELD_INTEGER),
	DEFINE_FIELD(CFuncTrackTrain, m_flVolume, FIELD_FLOAT),
	DEFINE_FIELD(CFuncTrackTrain, m_flBank, FIELD_FLOAT),
	DEFINE_FIELD(CFuncTrackTrain, m_oldSpeed, FIELD_FLOAT),
END_SAVERESTORE(CFuncTrackTrain, CBaseEntity)
#endif

LINK_ENTITY_TO_CLASS(func_tracktrain, CFuncTrackTrain);

bool CFuncTrackTrain::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "wheels"))
	{
		m_length = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "height"))
	{
		m_height = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "startspeed"))
	{
		m_startSpeed = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "sounds"))
	{
		m_sounds = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "volume"))
	{
		m_flVolume = (float)(atoi(pkvd->szValue));
		m_flVolume *= 0.1;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "bank"))
	{
		m_flBank = atof(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}


void CFuncTrackTrain::NextThink(float thinkTime, bool alwaysThink)
{
	if (alwaysThink)
		pev->flags |= FL_ALWAYSTHINK;
	else
		pev->flags &= ~FL_ALWAYSTHINK;

	pev->nextthink = thinkTime;
}


void CFuncTrackTrain::Blocked(CBaseEntity* pOther)
{
	entvars_t* pevOther = pOther->pev;

	// Blocker is on-ground on the train
	if (FBitSet(pevOther->flags, FL_ONGROUND) && VARS(pevOther->groundentity) == pev)
	{
		float deltaSpeed = fabs(pev->speed);
		if (deltaSpeed > 50)
			deltaSpeed = 50;
		if (0 == pevOther->velocity.z)
			pevOther->velocity.z += deltaSpeed;
		return;
	}
	else
		pevOther->velocity = (pevOther->origin - pev->origin).Normalize() * pev->dmg;

	ALERT(at_aiconsole, "TRAIN(%s): Blocked by %s (dmg:%.2f)\n", STRING(pev->targetname), STRING(pOther->pev->classname), pev->dmg);
	if (pev->dmg <= 0)
		return;
	// we can't hurt this thing, so we're not concerned with it
	pOther->TakeDamage(this, this, pev->dmg, DMG_CRUSH);
}


void CFuncTrackTrain::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (useType != USE_SET)
	{
		if (!ShouldToggle(useType, (pev->speed != 0)))
			return;

		if (pev->speed == 0)
		{
			pev->speed = m_speed * m_dir;

			Next();
		}
		else
		{
			pev->speed = 0;
			pev->velocity = g_vecZero;
			pev->avelocity = g_vecZero;
			StopSound();
			SetThink(NULL);
		}
	}
	else
	{
		float delta = value;

		delta = ((int)(pev->speed * 4) / (int)m_speed) * 0.25 + 0.25 * delta;
		if (delta > 1)
			delta = 1;
		else if (delta < -1)
			delta = -1;
		if ((pev->spawnflags & SF_TRACKTRAIN_FORWARDONLY) != 0)
		{
			if (delta < 0)
				delta = 0;
		}
		pev->speed = m_speed * delta;
		Next();
		ALERT(at_aiconsole, "TRAIN(%s), speed to %.2f\n", STRING(pev->targetname), pev->speed);
	}
}


static float Fix(float angle)
{
	while (angle < 0)
		angle += 360;
	while (angle > 360)
		angle -= 360;

	return angle;
}


static void FixupAngles(Vector& v)
{
	v.x = Fix(v.x);
	v.y = Fix(v.y);
	v.z = Fix(v.z);
}

#define TRAIN_STARTPITCH 60
#define TRAIN_MAXPITCH 200
#define TRAIN_MAXSPEED 1000 // approx max speed for sound pitch calculation

void CFuncTrackTrain::StopSound()
{
	// if sound playing, stop it
	if (m_soundPlaying && !FStringNull(pev->noise))
	{
		unsigned short us_encode;
		unsigned short us_sound = ((unsigned short)(m_sounds)&0x0007) << 12;

		us_encode = us_sound;

		PLAYBACK_EVENT_FULL(FEV_RELIABLE | FEV_UPDATE, edict(), m_usAdjustPitch, 0.0,
			g_vecZero, g_vecZero, 0.0, 0.0, us_encode, 0, 1, 0);

		EmitSound("plats/ttrain_brake1.wav", CHAN_ITEM, m_flVolume);
	}

	m_soundPlaying = false;
}

// update pitch based on speed, start sound if not playing
// NOTE: when train goes through transition, m_soundPlaying should go to false,
// which will cause the looped sound to restart.

void CFuncTrackTrain::UpdateSound()
{
	float flpitch;

	if (FStringNull(pev->noise))
		return;

	flpitch = TRAIN_STARTPITCH + (fabs(pev->speed) * (TRAIN_MAXPITCH - TRAIN_STARTPITCH) / TRAIN_MAXSPEED);

	if (!m_soundPlaying)
	{
		// play startup sound for train
		EmitSound("plats/ttrain_start1.wav", CHAN_ITEM, m_flVolume);
		EmitSound(STRING(pev->noise), CHAN_STATIC, m_flVolume, ATTN_NORM, flpitch);
		m_soundPlaying = true;
	}
	else
	{
		// volume 0.0 - 1.0 - 6 bits
		// m_sounds 3 bits
		// flpitch = 6 bits
		// 15 bits total

		unsigned short us_encode;
		unsigned short us_sound = ((unsigned short)(m_sounds)&0x0007) << 12;
		unsigned short us_pitch = ((unsigned short)(flpitch / 10.0) & 0x003f) << 6;
		unsigned short us_volume = ((unsigned short)(m_flVolume * 40.0) & 0x003f);

		us_encode = us_sound | us_pitch | us_volume;

		PLAYBACK_EVENT_FULL(FEV_RELIABLE | FEV_UPDATE, edict(), m_usAdjustPitch, 0.0,
			g_vecZero, g_vecZero, 0.0, 0.0, us_encode, 0, 0, 0);
	}
}


void CFuncTrackTrain::Next()
{
	float time = 0.5;

	if (0 == pev->speed)
	{
		ALERT(at_aiconsole, "TRAIN(%s): Speed is 0\n", STRING(pev->targetname));
		StopSound();
		return;
	}

	//	if ( !m_ppath )
	//		m_ppath = CPathTrack::Instance(FIND_ENTITY_BY_TARGETNAME( NULL, STRING(pev->target) ));
	if (!m_ppath)
	{
		ALERT(at_aiconsole, "TRAIN(%s): Lost path\n", STRING(pev->targetname));
		StopSound();
		return;
	}

	UpdateSound();

	Vector nextPos = pev->origin;

	nextPos.z -= m_height;
	CPathTrack* pnext = m_ppath->LookAhead(&nextPos, pev->speed * 0.1, true);
	nextPos.z += m_height;

	pev->velocity = (nextPos - pev->origin) * 10;
	Vector nextFront = pev->origin;

	nextFront.z -= m_height;
	if (m_length > 0)
		m_ppath->LookAhead(&nextFront, m_length, false);
	else
		m_ppath->LookAhead(&nextFront, 100, false);
	nextFront.z += m_height;

	Vector delta = nextFront - pev->origin;
	Vector angles = util::VecToAngles(delta);
	// The train actually points west
	angles.y += 180;

	// !!!  All of this crap has to be done to make the angles not wrap around, revisit this.
	FixupAngles(angles);
	FixupAngles(pev->angles);

	if (!pnext || (delta.x == 0 && delta.y == 0))
		angles = pev->angles;

	float vy, vx;
	if ((pev->spawnflags & SF_TRACKTRAIN_NOPITCH) == 0)
		vx = util::AngleDistance(angles.x, pev->angles.x);
	else
		vx = 0;
	vy = util::AngleDistance(angles.y, pev->angles.y);

	pev->avelocity.y = vy * 10;
	pev->avelocity.x = vx * 10;

	if (m_flBank != 0)
	{
		if (pev->avelocity.y < -5)
			pev->avelocity.z = util::AngleDistance(util::ApproachAngle(-m_flBank, pev->angles.z, m_flBank * 2), pev->angles.z);
		else if (pev->avelocity.y > 5)
			pev->avelocity.z = util::AngleDistance(util::ApproachAngle(m_flBank, pev->angles.z, m_flBank * 2), pev->angles.z);
		else
			pev->avelocity.z = util::AngleDistance(util::ApproachAngle(0, pev->angles.z, m_flBank * 4), pev->angles.z) * 4;
	}

	if (pnext)
	{
		if (pnext != m_ppath)
		{
			CPathTrack* pFire;
			if (pev->speed >= 0)
				pFire = pnext;
			else
				pFire = m_ppath;

			m_ppath = pnext;
			// Fire the pass target if there is one
			if (!FStringNull(pFire->pev->message))
			{
				util::FireTargets(STRING(pFire->pev->message), this, this, USE_TOGGLE, 0);
				if (FBitSet(pFire->pev->spawnflags, SF_PATH_FIREONCE))
					pFire->pev->message = 0;
			}

			if ((pFire->pev->spawnflags & SF_PATH_DISABLE_TRAIN) != 0)
				pev->spawnflags |= SF_TRACKTRAIN_NOCONTROL;

			// Don't override speed if under user control
			if ((pev->spawnflags & SF_TRACKTRAIN_NOCONTROL) != 0)
			{
				if (pFire->pev->speed != 0)
				{ // don't copy speed from target if it is 0 (uninitialized)
					pev->speed = pFire->pev->speed;
					ALERT(at_aiconsole, "TrackTrain %s speed to %4.2f\n", STRING(pev->targetname), pev->speed);
				}
			}
		}
		SetThink(&CFuncTrackTrain::Next);
		NextThink(pev->ltime + time, true);
	}
	else // end of path, stop
	{
		StopSound();
		pev->velocity = (nextPos - pev->origin);
		pev->avelocity = g_vecZero;
		float distance = pev->velocity.Length();
		m_oldSpeed = pev->speed;


		pev->speed = 0;

		// Move to the dead end

		// Are we there yet?
		if (distance > 0)
		{
			// no, how long to get there?
			time = distance / m_oldSpeed;
			pev->velocity = pev->velocity * (m_oldSpeed / distance);
			SetThink(&CFuncTrackTrain::DeadEnd);
			NextThink(pev->ltime + time, false);
		}
		else
		{
			DeadEnd();
		}
	}
}


void CFuncTrackTrain::DeadEnd()
{
	// Fire the dead-end target if there is one
	CPathTrack *pTrack, *pNext;

	pTrack = m_ppath;

	ALERT(at_aiconsole, "TRAIN(%s): Dead end ", STRING(pev->targetname));
	// Find the dead end path node
	// HACKHACK -- This is bugly, but the train can actually stop moving at a different node depending on it's speed
	// so we have to traverse the list to it's end.
	if (pTrack)
	{
		if (m_oldSpeed < 0)
		{
			do
			{
				pNext = pTrack->ValidPath(pTrack->GetPrevious(), true);
				if (pNext)
					pTrack = pNext;
			} while (nullptr != pNext);
		}
		else
		{
			do
			{
				pNext = pTrack->ValidPath(pTrack->GetNext(), true);
				if (pNext)
					pTrack = pNext;
			} while (nullptr != pNext);
		}
	}

	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	if (pTrack)
	{
		ALERT(at_aiconsole, "at %s\n", STRING(pTrack->pev->targetname));
		if (!FStringNull(pTrack->pev->netname))
			util::FireTargets(STRING(pTrack->pev->netname), this, this, USE_TOGGLE, 0);
	}
	else
		ALERT(at_aiconsole, "\n");
}


void CFuncTrackTrain::SetControls(entvars_t* pevControls)
{
	Vector offset = pevControls->origin - pev->oldorigin;

	m_controlMins = pevControls->mins + offset;
	m_controlMaxs = pevControls->maxs + offset;
}


bool CFuncTrackTrain::OnControls(entvars_t* pevTest)
{
	Vector offset = pevTest->origin - pev->origin;

	if ((pev->spawnflags & SF_TRACKTRAIN_NOCONTROL) != 0)
		return false;

	// Transform offset into local coordinates
	util::MakeVectors(pev->angles);
	Vector local;
	local.x = DotProduct(offset, gpGlobals->v_forward);
	local.y = -DotProduct(offset, gpGlobals->v_right);
	local.z = DotProduct(offset, gpGlobals->v_up);

	if (local.x >= m_controlMins.x && local.y >= m_controlMins.y && local.z >= m_controlMins.z &&
		local.x <= m_controlMaxs.x && local.y <= m_controlMaxs.y && local.z <= m_controlMaxs.z)
		return true;

	return false;
}


void CFuncTrackTrain::Find()
{
	m_ppath = CPathTrack::Instance(FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target)));
	if (!m_ppath)
		return;

	entvars_t* pevTarget = m_ppath->pev;
	if (!FClassnameIs(pevTarget, "path_track"))
	{
		ALERT(at_error, "func_track_train must be on a path of path_track\n");
		m_ppath = NULL;
		return;
	}

	Vector nextPos = pevTarget->origin;
	nextPos.z += m_height;

	Vector look = nextPos;
	look.z -= m_height;
	m_ppath->LookAhead(&look, m_length, false);
	look.z += m_height;

	pev->angles = util::VecToAngles(look - nextPos);
	// The train actually points west
	pev->angles.y += 180;

	if ((pev->spawnflags & SF_TRACKTRAIN_NOPITCH) != 0)
		pev->angles.x = 0;
	SetOrigin(nextPos);
	NextThink(pev->ltime + 0.1, false);
	SetThink(&CFuncTrackTrain::Next);
	pev->speed = m_startSpeed;

	UpdateSound();
}


void CFuncTrackTrain::NearestPath()
{
	CBaseEntity* pTrack = NULL;
	CBaseEntity* pNearest = NULL;
	float dist, closest;

	closest = 1024;

	while ((pTrack = util::FindEntityInSphere(pTrack, pev->origin, 1024)) != NULL)
	{
		// filter out non-tracks
		if ((pTrack->pev->flags & (FL_CLIENT | FL_MONSTER)) == 0 && FClassnameIs(pTrack->pev, "path_track"))
		{
			dist = (pev->origin - pTrack->pev->origin).Length();
			if (dist < closest)
			{
				closest = dist;
				pNearest = pTrack;
			}
		}
	}

	if (!pNearest)
	{
		ALERT(at_console, "Can't find a nearby track !!!\n");
		SetThink(NULL);
		return;
	}

	ALERT(at_aiconsole, "TRAIN: %s, Nearest track is %s\n", STRING(pev->targetname), STRING(pNearest->pev->targetname));
	// If I'm closer to the next path_track on this path, then it's my real path
	pTrack = ((CPathTrack*)pNearest)->GetNext();
	if (pTrack)
	{
		if ((pev->origin - pTrack->pev->origin).Length() < (pev->origin - pNearest->pev->origin).Length())
			pNearest = pTrack;
	}

	m_ppath = (CPathTrack*)pNearest;

	if (pev->speed != 0)
	{
		NextThink(pev->ltime + 0.1, false);
		SetThink(&CFuncTrackTrain::Next);
	}
}


void CFuncTrackTrain::OverrideReset()
{
	NextThink(pev->ltime + 0.1, false);
	SetThink(&CFuncTrackTrain::NearestPath);
}


CFuncTrackTrain* CFuncTrackTrain::Instance(edict_t* pent)
{
	if (FClassnameIs(pent, "func_tracktrain"))
		return (CFuncTrackTrain*)GET_PRIVATE(pent);
	return NULL;
}


bool CFuncTrackTrain::Spawn()
{
	if (pev->speed == 0)
		m_speed = 100;
	else
		m_speed = pev->speed;

	pev->speed = 0;
	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	pev->impulse = m_speed;

	m_dir = 1;

	if (FStringNull(pev->target))
		ALERT(at_console, "FuncTrain with no target");

	if ((pev->spawnflags & SF_TRACKTRAIN_PASSABLE) != 0)
		pev->solid = SOLID_NOT;
	else
		pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	SetModel(STRING(pev->model));

	SetSize(pev->mins, pev->maxs);
	SetOrigin(pev->origin);

	// Cache off placed origin for train controls
	pev->oldorigin = pev->origin;

	m_controlMins = pev->mins;
	m_controlMaxs = pev->maxs;
	m_controlMaxs.z += 72;
	// start trains on the next frame, to make sure their targets have had
	// a chance to spawn/activate
	NextThink(pev->ltime + 0.1, false);
	SetThink(&CFuncTrackTrain::Find);
	Precache();

	return true;
}

void CFuncTrackTrain::Precache()
{
	if (m_flVolume == 0.0)
		m_flVolume = 1.0;

	switch (m_sounds)
	{
	default:
		// no sound
		pev->noise = 0;
		break;
	case 1:
		PRECACHE_SOUND("plats/ttrain1.wav");
		pev->noise = MAKE_STRING("plats/ttrain1.wav");
		break;
	case 2:
		PRECACHE_SOUND("plats/ttrain2.wav");
		pev->noise = MAKE_STRING("plats/ttrain2.wav");
		break;
	case 3:
		PRECACHE_SOUND("plats/ttrain3.wav");
		pev->noise = MAKE_STRING("plats/ttrain3.wav");
		break;
	case 4:
		PRECACHE_SOUND("plats/ttrain4.wav");
		pev->noise = MAKE_STRING("plats/ttrain4.wav");
		break;
	case 5:
		PRECACHE_SOUND("plats/ttrain6.wav");
		pev->noise = MAKE_STRING("plats/ttrain6.wav");
		break;
	case 6:
		PRECACHE_SOUND("plats/ttrain7.wav");
		pev->noise = MAKE_STRING("plats/ttrain7.wav");
		break;
	}

	PRECACHE_SOUND("plats/ttrain_brake1.wav");
	PRECACHE_SOUND("plats/ttrain_start1.wav");

	m_usAdjustPitch = PRECACHE_EVENT(1, "events/train.sc");
}

// This class defines the volume of space that the player must stand in to control the train
class CFuncTrainControls : public CBaseEntity
{
public:
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	bool Spawn() override;
	void EXPORT Find();
};
LINK_ENTITY_TO_CLASS(func_traincontrols, CFuncTrainControls);


void CFuncTrainControls::Find()
{
	edict_t* pTarget = NULL;

	do
	{
		pTarget = FIND_ENTITY_BY_TARGETNAME(pTarget, STRING(pev->target));
	} while (!FNullEnt(pTarget) && !FClassnameIs(pTarget, "func_tracktrain"));

	if (FNullEnt(pTarget))
	{
		ALERT(at_console, "No train %s\n", STRING(pev->target));
		return;
	}

	CFuncTrackTrain* ptrain = CFuncTrackTrain::Instance(pTarget);
	ptrain->SetControls(pev);
	Remove();
}


bool CFuncTrainControls::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SetModel(STRING(pev->model));

	SetSize(pev->mins, pev->maxs);
	SetOrigin(pev->origin);

	SetThink(&CFuncTrainControls::Find);
	pev->nextthink = gpGlobals->time;

	return true;
}



// ----------------------------------------------------------------------------
//
// Track changer / Train elevator
//
// ----------------------------------------------------------------------------

#define SF_TRACK_ACTIVATETRAIN 0x00000001
#define SF_TRACK_RELINK 0x00000002
#define SF_TRACK_ROTMOVE 0x00000004
#define SF_TRACK_STARTBOTTOM 0x00000008
#define SF_TRACK_DONT_MOVE 0x00000010

//
// This entity is a rotating/moving platform that will carry a train to a new track.
// It must be larger in X-Y planar area than the train, since it must contain the
// train within these dimensions in order to operate when the train is near it.
//

typedef enum
{
	TRAIN_SAFE,
	TRAIN_BLOCKING,
	TRAIN_FOLLOWING
} TRAIN_CODE;

class CFuncTrackChange : public CFuncPlatRot
{
public:
	DECLARE_SAVERESTORE()

	bool Spawn() override;
	void Precache() override;

	//	void	Blocked() override;
	void EXPORT GoUp() override;
	void EXPORT GoDown() override;

	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void EXPORT Find();
	TRAIN_CODE EvaluateTrain(CPathTrack* pcurrent);
	void UpdateTrain(Vector& dest);
	void HitBottom() override;
	void HitTop() override;
	void Touch(CBaseEntity* pOther) override;
	virtual void UpdateAutoTargets(int toggleState);
	bool IsTogglePlat() override { return true; }

	void DisableUse() { m_use = false; }
	void EnableUse() { m_use = true; }
	bool UseEnabled() { return m_use; }

	void OverrideReset() override;


	CPathTrack* m_trackTop;
	CPathTrack* m_trackBottom;

	CFuncTrackTrain* m_train;

	int m_trackTopName;
	int m_trackBottomName;
	int m_trainName;
	TRAIN_CODE m_code;
	int m_targetState;
	bool m_use;
};
LINK_ENTITY_TO_CLASS(func_trackchange, CFuncTrackChange);

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CFuncTrackChange)
	DEFINE_GLOBAL_FIELD(CFuncTrackChange, m_trackTop, FIELD_CLASSPTR),
	DEFINE_GLOBAL_FIELD(CFuncTrackChange, m_trackBottom, FIELD_CLASSPTR),
	DEFINE_GLOBAL_FIELD(CFuncTrackChange, m_train, FIELD_CLASSPTR),
	DEFINE_GLOBAL_FIELD(CFuncTrackChange, m_trackTopName, FIELD_STRING),
	DEFINE_GLOBAL_FIELD(CFuncTrackChange, m_trackBottomName, FIELD_STRING),
	DEFINE_GLOBAL_FIELD(CFuncTrackChange, m_trainName, FIELD_STRING),
	DEFINE_FIELD(CFuncTrackChange, m_code, FIELD_INTEGER),
	DEFINE_FIELD(CFuncTrackChange, m_targetState, FIELD_INTEGER),
	DEFINE_FIELD(CFuncTrackChange, m_use, FIELD_BOOLEAN),
END_SAVERESTORE(CFuncTrackChange, CFuncPlatRot)
#endif

bool CFuncTrackChange::Spawn()
{
	Setup();
	if (FBitSet(pev->spawnflags, SF_TRACK_DONT_MOVE))
		m_vecPosition2.z = pev->origin.z;

	SetupRotation();

	if (FBitSet(pev->spawnflags, SF_TRACK_STARTBOTTOM))
	{
		SetOrigin(m_vecPosition2);
		m_toggle_state = TS_AT_BOTTOM;
		pev->angles = m_start;
		m_targetState = TS_AT_TOP;
	}
	else
	{
		SetOrigin(m_vecPosition1);
		m_toggle_state = TS_AT_TOP;
		pev->angles = m_end;
		m_targetState = TS_AT_BOTTOM;
	}

	EnableUse();
	pev->nextthink = pev->ltime + 2.0;
	SetThink(&CFuncTrackChange::Find);
	Precache();

	return true;
}

void CFuncTrackChange::Precache()
{
	// Can't trigger sound
	PRECACHE_SOUND("buttons/button11.wav");

	CFuncPlatRot::Precache();
}


void CFuncTrackChange::Touch(CBaseEntity* pOther)
{
#if 0
	TRAIN_CODE code;
	entvars_t *pevToucher = pOther->pev;
#endif
}



bool CFuncTrackChange::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "train"))
	{
		m_trainName = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "toptrack"))
	{
		m_trackTopName = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "bottomtrack"))
	{
		m_trackBottomName = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CFuncPlatRot::KeyValue(pkvd); // Pass up to base class
}


void CFuncTrackChange::OverrideReset()
{
	pev->nextthink = pev->ltime + 1.0;
	SetThink(&CFuncTrackChange::Find);
}

void CFuncTrackChange::Find()
{
	// Find track entities
	edict_t* target;

	target = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(m_trackTopName));
	if (!FNullEnt(target))
	{
		m_trackTop = CPathTrack::Instance(target);
		target = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(m_trackBottomName));
		if (!FNullEnt(target))
		{
			m_trackBottom = CPathTrack::Instance(target);
			target = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(m_trainName));
			if (!FNullEnt(target))
			{
				m_train = CFuncTrackTrain::Instance(FIND_ENTITY_BY_TARGETNAME(NULL, STRING(m_trainName)));
				if (!m_train)
				{
					ALERT(at_error, "Can't find train for track change! %s\n", STRING(m_trainName));
					return;
				}
				Vector center = (pev->absmin + pev->absmax) * 0.5;
				m_trackBottom = m_trackBottom->Nearest(center);
				m_trackTop = m_trackTop->Nearest(center);
				UpdateAutoTargets(m_toggle_state);
				SetThink(NULL);
				return;
			}
			else
			{
				ALERT(at_error, "Can't find train for track change! %s\n", STRING(m_trainName));
				target = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(m_trainName));
			}
		}
		else
			ALERT(at_error, "Can't find bottom track for track change! %s\n", STRING(m_trackBottomName));
	}
	else
		ALERT(at_error, "Can't find top track for track change! %s\n", STRING(m_trackTopName));
}



TRAIN_CODE CFuncTrackChange::EvaluateTrain(CPathTrack* pcurrent)
{
	// Go ahead and work, we don't have anything to switch, so just be an elevator
	if (!pcurrent || !m_train)
		return TRAIN_SAFE;

	if (m_train->m_ppath == pcurrent || (pcurrent->m_pprevious && m_train->m_ppath == pcurrent->m_pprevious) ||
		(pcurrent->m_pnext && m_train->m_ppath == pcurrent->m_pnext))
	{
		if (m_train->pev->speed != 0)
			return TRAIN_BLOCKING;

		Vector dist = pev->origin - m_train->pev->origin;
		float length = dist.Length2D();
		if (length < m_train->m_length) // Empirically determined close distance
			return TRAIN_FOLLOWING;
		else if (length > (150 + m_train->m_length))
			return TRAIN_SAFE;

		return TRAIN_BLOCKING;
	}

	return TRAIN_SAFE;
}


void CFuncTrackChange::UpdateTrain(Vector& dest)
{
	float time = (pev->nextthink - pev->ltime);

	m_train->pev->velocity = pev->velocity;
	m_train->pev->avelocity = pev->avelocity;
	m_train->NextThink(m_train->pev->ltime + time, false);

	// Attempt at getting the train to rotate properly around the origin of the trackchange
	if (time <= 0)
		return;

	Vector offset = m_train->pev->origin - pev->origin;
	Vector delta = dest - pev->angles;
	// Transform offset into local coordinates
	util::MakeInvVectors(delta, gpGlobals);
	Vector local;
	local.x = DotProduct(offset, gpGlobals->v_forward);
	local.y = DotProduct(offset, gpGlobals->v_right);
	local.z = DotProduct(offset, gpGlobals->v_up);

	local = local - offset;
	m_train->pev->velocity = pev->velocity + (local * (1.0 / time));
}

void CFuncTrackChange::GoDown()
{
	if (m_code == TRAIN_BLOCKING)
		return;

	// HitBottom may get called during CFuncPlat::GoDown(), so set up for that
	// before you call GoDown()

	UpdateAutoTargets(TS_GOING_DOWN);
	// If ROTMOVE, move & rotate
	if (FBitSet(pev->spawnflags, SF_TRACK_DONT_MOVE))
	{
		SetMoveDone(&CFuncTrackChange::CallHitBottom);
		m_toggle_state = TS_GOING_DOWN;
		AngularMove(m_start, pev->speed);
	}
	else
	{
		CFuncPlat::GoDown();
		SetMoveDone(&CFuncTrackChange::CallHitBottom);
		RotMove(m_start, pev->nextthink - pev->ltime);
	}
	// Otherwise, rotate first, move second

	// If the train is moving with the platform, update it
	if (m_code == TRAIN_FOLLOWING)
	{
		UpdateTrain(m_start);
		m_train->m_ppath = NULL;
	}
}


//
// Platform is at bottom, now starts moving up
//
void CFuncTrackChange::GoUp()
{
	if (m_code == TRAIN_BLOCKING)
		return;

	// HitTop may get called during CFuncPlat::GoUp(), so set up for that
	// before you call GoUp();

	UpdateAutoTargets(TS_GOING_UP);
	if (FBitSet(pev->spawnflags, SF_TRACK_DONT_MOVE))
	{
		m_toggle_state = TS_GOING_UP;
		SetMoveDone(&CFuncTrackChange::CallHitTop);
		AngularMove(m_end, pev->speed);
	}
	else
	{
		// If ROTMOVE, move & rotate
		CFuncPlat::GoUp();
		SetMoveDone(&CFuncTrackChange::CallHitTop);
		RotMove(m_end, pev->nextthink - pev->ltime);
	}

	// Otherwise, move first, rotate second

	// If the train is moving with the platform, update it
	if (m_code == TRAIN_FOLLOWING)
	{
		UpdateTrain(m_end);
		m_train->m_ppath = NULL;
	}
}


// Normal track change
void CFuncTrackChange::UpdateAutoTargets(int toggleState)
{
	if (!m_trackTop || !m_trackBottom)
		return;

	if (toggleState == TS_AT_TOP)
		ClearBits(m_trackTop->pev->spawnflags, SF_PATH_DISABLED);
	else
		SetBits(m_trackTop->pev->spawnflags, SF_PATH_DISABLED);

	if (toggleState == TS_AT_BOTTOM)
		ClearBits(m_trackBottom->pev->spawnflags, SF_PATH_DISABLED);
	else
		SetBits(m_trackBottom->pev->spawnflags, SF_PATH_DISABLED);
}


void CFuncTrackChange::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (m_toggle_state != TS_AT_TOP && m_toggle_state != TS_AT_BOTTOM)
		return;

	// If train is in "safe" area, but not on the elevator, play alarm sound
	if (m_toggle_state == TS_AT_TOP)
		m_code = EvaluateTrain(m_trackTop);
	else if (m_toggle_state == TS_AT_BOTTOM)
		m_code = EvaluateTrain(m_trackBottom);
	else
		m_code = TRAIN_BLOCKING;
	if (m_code == TRAIN_BLOCKING)
	{
		// Play alarm and return
		EmitSound("buttons/button11.wav", CHAN_VOICE);
		return;
	}

	// Otherwise, it's safe to move
	// If at top, go down
	// at bottom, go up

	DisableUse();
	if (m_toggle_state == TS_AT_TOP)
		GoDown();
	else
		GoUp();
}


//
// Platform has hit bottom.  Stops and waits forever.
//
void CFuncTrackChange::HitBottom()
{
	CFuncPlatRot::HitBottom();
	if (m_code == TRAIN_FOLLOWING)
	{
		//		UpdateTrain();
		m_train->SetTrack(m_trackBottom);
	}
	SetThink(NULL);
	pev->nextthink = -1;

	UpdateAutoTargets(m_toggle_state);

	EnableUse();
}


//
// Platform has hit bottom.  Stops and waits forever.
//
void CFuncTrackChange::HitTop()
{
	CFuncPlatRot::HitTop();
	if (m_code == TRAIN_FOLLOWING)
	{
		//		UpdateTrain();
		m_train->SetTrack(m_trackTop);
	}

	// Don't let the plat go back down
	SetThink(NULL);
	pev->nextthink = -1;
	UpdateAutoTargets(m_toggle_state);
	EnableUse();
}



class CFuncTrackAuto : public CFuncTrackChange
{
public:
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void UpdateAutoTargets(int toggleState) override;
};

LINK_ENTITY_TO_CLASS(func_trackautochange, CFuncTrackAuto);

// Auto track change
void CFuncTrackAuto::UpdateAutoTargets(int toggleState)
{
	CPathTrack *pTarget, *pNextTarget;

	if (!m_trackTop || !m_trackBottom)
		return;

	if (m_targetState == TS_AT_TOP)
	{
		pTarget = m_trackTop->GetNext();
		pNextTarget = m_trackBottom->GetNext();
	}
	else
	{
		pTarget = m_trackBottom->GetNext();
		pNextTarget = m_trackTop->GetNext();
	}
	if (pTarget)
	{
		ClearBits(pTarget->pev->spawnflags, SF_PATH_DISABLED);
		if (m_code == TRAIN_FOLLOWING && m_train && m_train->pev->speed == 0)
			m_train->Use(this, this, USE_ON, 0);
	}

	if (pNextTarget)
		SetBits(pNextTarget->pev->spawnflags, SF_PATH_DISABLED);
}


void CFuncTrackAuto::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	CPathTrack* pTarget;

	if (!UseEnabled())
		return;

	if (m_toggle_state == TS_AT_TOP)
		pTarget = m_trackTop;
	else if (m_toggle_state == TS_AT_BOTTOM)
		pTarget = m_trackBottom;
	else
		pTarget = NULL;

	if (FClassnameIs(pActivator->pev, "func_tracktrain"))
	{
		m_code = EvaluateTrain(pTarget);
		// Safe to fire?
		if (m_code == TRAIN_FOLLOWING && m_toggle_state != m_targetState)
		{
			DisableUse();
			if (m_toggle_state == TS_AT_TOP)
				GoDown();
			else
				GoUp();
		}
	}
	else
	{
		if (pTarget)
			pTarget = pTarget->GetNext();
		if (pTarget && m_train->m_ppath != pTarget && ShouldToggle(useType, TS_AT_TOP != m_targetState))
		{
			if (m_targetState == TS_AT_TOP)
				m_targetState = TS_AT_BOTTOM;
			else
				m_targetState = TS_AT_TOP;
		}

		UpdateAutoTargets(m_targetState);
	}
}

