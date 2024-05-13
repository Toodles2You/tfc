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

#define SF_PLAT_TOGGLE 0x0001

// Trains
#define SF_TRAIN_WAIT_RETRIGGER 1
#define SF_TRAIN_START_ON 4 // Train is initially moving
#define SF_TRAIN_PASSABLE 8 // Train is not solid -- used to make water trains

class CBasePlatTrain : public CBaseToggle
{
public:
	CBasePlatTrain(Entity* containingEntity) : CBaseToggle(containingEntity) {}

	DECLARE_SAVERESTORE()

	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	bool KeyValue(KeyValueData* pkvd) override;
	void Precache() override;

	// This is done to fix spawn flag collisions between this class and a derived class
	virtual bool IsTogglePlat() { return (v.spawnflags & SF_PLAT_TOGGLE) != 0; }

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
	else if (streq(pkvd->szKeyName, "height"))
	{
		m_flHeight = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "rotation"))
	{
		m_vecFinalAngle.x = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "movesnd"))
	{
		m_bMoveSnd = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "stopsnd"))
	{
		m_bStopSnd = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "volume"))
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
		v.noiseMoving = MAKE_STRING("common/null.wav");
		break;
	case 1:
		g_engfuncs.pfnPrecacheSound("plats/bigmove1.wav");
		v.noiseMoving = MAKE_STRING("plats/bigmove1.wav");
		break;
	case 2:
		g_engfuncs.pfnPrecacheSound("plats/bigmove2.wav");
		v.noiseMoving = MAKE_STRING("plats/bigmove2.wav");
		break;
	case 3:
		g_engfuncs.pfnPrecacheSound("plats/elevmove1.wav");
		v.noiseMoving = MAKE_STRING("plats/elevmove1.wav");
		break;
	case 4:
		g_engfuncs.pfnPrecacheSound("plats/elevmove2.wav");
		v.noiseMoving = MAKE_STRING("plats/elevmove2.wav");
		break;
	case 5:
		g_engfuncs.pfnPrecacheSound("plats/elevmove3.wav");
		v.noiseMoving = MAKE_STRING("plats/elevmove3.wav");
		break;
	case 6:
		g_engfuncs.pfnPrecacheSound("plats/freightmove1.wav");
		v.noiseMoving = MAKE_STRING("plats/freightmove1.wav");
		break;
	case 7:
		g_engfuncs.pfnPrecacheSound("plats/freightmove2.wav");
		v.noiseMoving = MAKE_STRING("plats/freightmove2.wav");
		break;
	case 8:
		g_engfuncs.pfnPrecacheSound("plats/heavymove1.wav");
		v.noiseMoving = MAKE_STRING("plats/heavymove1.wav");
		break;
	case 9:
		g_engfuncs.pfnPrecacheSound("plats/rackmove1.wav");
		v.noiseMoving = MAKE_STRING("plats/rackmove1.wav");
		break;
	case 10:
		g_engfuncs.pfnPrecacheSound("plats/railmove1.wav");
		v.noiseMoving = MAKE_STRING("plats/railmove1.wav");
		break;
	case 11:
		g_engfuncs.pfnPrecacheSound("plats/squeekmove1.wav");
		v.noiseMoving = MAKE_STRING("plats/squeekmove1.wav");
		break;
	case 12:
		g_engfuncs.pfnPrecacheSound("plats/talkmove1.wav");
		v.noiseMoving = MAKE_STRING("plats/talkmove1.wav");
		break;
	case 13:
		g_engfuncs.pfnPrecacheSound("plats/talkmove2.wav");
		v.noiseMoving = MAKE_STRING("plats/talkmove2.wav");
		break;
	default:
		v.noiseMoving = MAKE_STRING("common/null.wav");
		break;
	}

	// set the plat's 'reached destination' stop sound
	switch (m_bStopSnd)
	{
	case 0:
		v.noiseArrived = MAKE_STRING("common/null.wav");
		break;
	case 1:
		g_engfuncs.pfnPrecacheSound("plats/bigstop1.wav");
		v.noiseArrived = MAKE_STRING("plats/bigstop1.wav");
		break;
	case 2:
		g_engfuncs.pfnPrecacheSound("plats/bigstop2.wav");
		v.noiseArrived = MAKE_STRING("plats/bigstop2.wav");
		break;
	case 3:
		g_engfuncs.pfnPrecacheSound("plats/freightstop1.wav");
		v.noiseArrived = MAKE_STRING("plats/freightstop1.wav");
		break;
	case 4:
		g_engfuncs.pfnPrecacheSound("plats/heavystop2.wav");
		v.noiseArrived = MAKE_STRING("plats/heavystop2.wav");
		break;
	case 5:
		g_engfuncs.pfnPrecacheSound("plats/rackstop1.wav");
		v.noiseArrived = MAKE_STRING("plats/rackstop1.wav");
		break;
	case 6:
		g_engfuncs.pfnPrecacheSound("plats/railstop1.wav");
		v.noiseArrived = MAKE_STRING("plats/railstop1.wav");
		break;
	case 7:
		g_engfuncs.pfnPrecacheSound("plats/squeekstop1.wav");
		v.noiseArrived = MAKE_STRING("plats/squeekstop1.wav");
		break;
	case 8:
		g_engfuncs.pfnPrecacheSound("plats/talkstop1.wav");
		v.noiseArrived = MAKE_STRING("plats/talkstop1.wav");
		break;

	default:
		v.noiseArrived = MAKE_STRING("common/null.wav");
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
	CFuncPlat(Entity* containingEntity) : CBasePlatTrain(containingEntity) {}

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

static void PlatSpawnInsideTrigger(CFuncPlat* platform);


class CPlatTrigger : public CBaseToggle
{
public:
	CPlatTrigger(Entity* containingEntity) : CBaseToggle(containingEntity) {}

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
	//v.noiseMovement = MAKE_STRING("plats/platmove1.wav");
	//v.noiseStopMoving = MAKE_STRING("plats/platstop1.wav");

	if (m_flTLength == 0)
		m_flTLength = 80;
	if (m_flTWidth == 0)
		m_flTWidth = 10;

	v.angles = g_vecZero;

	v.solid = SOLID_BSP;
	v.movetype = MOVETYPE_PUSH;

	SetOrigin(v.origin); // set size and link into world
	SetSize(v.mins, v.maxs);
	SetModel(v.model);

	// vecPosition1 is the top position, vecPosition2 is the bottom
	m_vecPosition1 = v.origin;
	m_vecPosition2 = v.origin;
	if (m_flHeight != 0)
		m_vecPosition2.z = v.origin.z - m_flHeight;
	else
		m_vecPosition2.z = v.origin.z - v.size.z + 8;
	if (v.speed == 0)
		v.speed = 150;

	if (m_volume == 0)
		m_volume = 0.85;
}


void CFuncPlat::Precache()
{
	CBasePlatTrain::Precache();
	//g_engfuncs.pfnPrecacheSound("plats/platmove1.wav");
	//g_engfuncs.pfnPrecacheSound("plats/platstop1.wav");
	if (!IsTogglePlat())
		PlatSpawnInsideTrigger(this); // the "start moving" trigger
}


bool CFuncPlat::Spawn()
{
	Setup();

	Precache();

	// If this platform is the target of some button, it starts at the TOP position,
	// and is brought down by that button.  Otherwise, it starts at BOTTOM.
	if (!FStringNull(v.targetname))
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



static void PlatSpawnInsideTrigger(CFuncPlat* platform)
{
	auto trigger = Entity::Create<CPlatTrigger>();
	if (trigger != nullptr)
	{
		trigger->SpawnInsideTrigger(platform);
	}
}


//
// Create a trigger entity for a platform.
//
void CPlatTrigger::SpawnInsideTrigger(CFuncPlat* pPlatform)
{
	m_hPlatform = pPlatform;
	// Create trigger entity, "point" it at the owning platform, give it a touch method
	v.solid = SOLID_TRIGGER;
	v.movetype = MOVETYPE_NONE;
	v.origin = pPlatform->v.origin;

	// Establish the trigger field's size
	Vector vecTMin = pPlatform->v.mins + Vector(25, 25, 0);
	Vector vecTMax = pPlatform->v.maxs + Vector(25, 25, 8);
	vecTMin.z = vecTMax.z - (pPlatform->m_vecPosition1.z - pPlatform->m_vecPosition2.z + 8);
	if (pPlatform->v.size.x <= 50)
	{
		vecTMin.x = (pPlatform->v.mins.x + pPlatform->v.maxs.x) / 2;
		vecTMax.x = vecTMin.x + 1;
	}
	if (pPlatform->v.size.y <= 50)
	{
		vecTMin.y = (pPlatform->v.mins.y + pPlatform->v.maxs.y) / 2;
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
	if (!m_hPlatform)
	{
		Remove();
		return;
	}

	// Ignore touches by non-players
	if (!pOther->IsPlayer())
		return;

	// Ignore touches by corpses
	if (!pOther->IsAlive())
		return;

	CFuncPlat* platform = static_cast<CFuncPlat*>(static_cast<CBaseEntity*>(m_hPlatform));

	// Make linked platform go up/down.
	if (platform->m_toggle_state == TS_AT_BOTTOM)
		platform->GoUp();
	else if (platform->m_toggle_state == TS_AT_TOP)
		platform->v.nextthink = platform->v.ltime + 1; // delay going down
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
		ClearUse();

		if (m_toggle_state == TS_AT_TOP)
			GoDown();
	}
}


//
// Platform is at top, now starts moving down.
//
void CFuncPlat::GoDown()
{
	if (!FStringNull(v.noiseMovement))
		EmitSound(STRING(v.noiseMovement), CHAN_STATIC, m_volume);

	m_toggle_state = TS_GOING_DOWN;
	SetMoveDone(&CFuncPlat::CallHitBottom);
	LinearMove(m_vecPosition2, v.speed);
}


//
// Platform has hit bottom.  Stops and waits forever.
//
void CFuncPlat::HitBottom()
{
	if (!FStringNull(v.noiseMovement))
		StopSound(STRING(v.noiseMovement),  CHAN_STATIC);

	if (!FStringNull(v.noiseStopMoving))
		EmitSound(STRING(v.noiseStopMoving), CHAN_WEAPON, m_volume);

	m_toggle_state = TS_AT_BOTTOM;
}


//
// Platform is at bottom, now starts moving up
//
void CFuncPlat::GoUp()
{
	if (!FStringNull(v.noiseMovement))
		EmitSound(STRING(v.noiseMovement), CHAN_STATIC, m_volume);

	m_toggle_state = TS_GOING_UP;
	SetMoveDone(&CFuncPlat::CallHitTop);
	LinearMove(m_vecPosition1, v.speed);
}


//
// Platform has hit top.  Pauses, then starts back down again.
//
void CFuncPlat::HitTop()
{
	if (!FStringNull(v.noiseMovement))
		StopSound(STRING(v.noiseMovement), CHAN_STATIC);

	if (!FStringNull(v.noiseStopMoving))
		EmitSound(STRING(v.noiseStopMoving), CHAN_WEAPON, m_volume);

	m_toggle_state = TS_AT_TOP;

	if (!IsTogglePlat())
	{
		// After a delay, the platform will automatically start going down again.
		SetThink(&CFuncPlat::CallGoDown);
		v.nextthink = v.ltime + 3;
	}
}


void CFuncPlat::Blocked(CBaseEntity* pOther)
{
	g_engfuncs.pfnAlertMessage(at_aiconsole, "%s Blocked by %s\n", STRING(v.classname), STRING(pOther->v.classname));
	// Hurt the blocker a little
	pOther->TakeDamage(this, this, 1, DMG_CRUSH);

	if (!FStringNull(v.noiseMovement))
		StopSound(STRING(v.noiseMovement), CHAN_STATIC);

	// Send the platform back where it came from
	if (m_toggle_state == TS_GOING_UP)
		GoDown();
	else if (m_toggle_state == TS_GOING_DOWN)
		GoUp();
}


class CFuncPlatRot : public CFuncPlat
{
public:
	CFuncPlatRot(Entity* containingEntity) : CFuncPlat(containingEntity) {}

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
		CBaseToggle::AxisDir(this);
		m_start = v.angles;
		m_end = v.angles + v.movedir * m_vecFinalAngle.x;
	}
	else
	{
		m_start = g_vecZero;
		m_end = g_vecZero;
	}
	if (!FStringNull(v.targetname)) // Start at top
	{
		v.angles = m_end;
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
	RotMove(m_start, v.nextthink - v.ltime);
}


//
// Platform has hit bottom.  Stops and waits forever.
//
void CFuncPlatRot::HitBottom()
{
	CFuncPlat::HitBottom();
	v.avelocity = g_vecZero;
	v.angles = m_start;
}


//
// Platform is at bottom, now starts moving up
//
void CFuncPlatRot::GoUp()
{
	CFuncPlat::GoUp();
	RotMove(m_end, v.nextthink - v.ltime);
}


//
// Platform has hit top.  Pauses, then starts back down again.
//
void CFuncPlatRot::HitTop()
{
	CFuncPlat::HitTop();
	v.avelocity = g_vecZero;
	v.angles = m_end;
}


void CFuncPlatRot::RotMove(Vector& destAngle, float time)
{
	// set destdelta to the vector needed to move
	Vector vecDestDelta = destAngle - v.angles;

	// Travel time is so short, we're practically there already;  so make it so.
	if (time >= 0.1)
		v.avelocity = vecDestDelta / time;
	else
	{
		v.avelocity = vecDestDelta;
		v.nextthink = v.ltime + 1;
	}
}


//
//====================== TRAIN code ==================================================
//

class CFuncTrain : public CBasePlatTrain
{
public:
	CFuncTrain(Entity* containingEntity) : CBasePlatTrain(containingEntity) {}

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

	Entity* m_pevCurrentTarget;
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
	if (streq(pkvd->szKeyName, "sounds"))
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

	pOther->TakeDamage(this, this, v.dmg, DMG_CRUSH);
}


void CFuncTrain::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if ((v.spawnflags & SF_TRAIN_WAIT_RETRIGGER) != 0)
	{
		// Move toward my target
		v.spawnflags &= ~SF_TRAIN_WAIT_RETRIGGER;
		Next();
	}
	else
	{
		v.spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
		// Pop back to last target if it's available
		if (v.enemy)
			v.target = v.enemy->targetname;
		v.nextthink = 0;
		v.velocity = g_vecZero;
		if (!FStringNull(v.noiseStopMoving))
			EmitSound(STRING(v.noiseStopMoving), CHAN_VOICE, m_volume);
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
	if (FBitSet(m_pevCurrentTarget->spawnflags, SF_TRAIN_WAIT_RETRIGGER) || (v.spawnflags & SF_TRAIN_WAIT_RETRIGGER) != 0)
	{
		v.spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
		// clear the sound channel.
		if (!FStringNull(v.noiseMovement))
			StopSound(STRING(v.noiseMovement), CHAN_STATIC);
		if (!FStringNull(v.noiseStopMoving))
			EmitSound(STRING(v.noiseStopMoving), CHAN_VOICE, m_volume);
		v.nextthink = 0;
		return;
	}

	// g_engfuncs.pfnAlertMessage ( at_console, "%f\n", m_flWait );

	if (m_flWait != 0)
	{ // -1 wait will wait forever!
		v.nextthink = v.ltime + m_flWait;
		if (!FStringNull(v.noiseMovement))
			StopSound(STRING(v.noiseMovement), CHAN_STATIC);
		if (!FStringNull(v.noiseStopMoving))
			EmitSound(STRING(v.noiseStopMoving), CHAN_VOICE, m_volume);
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
		if (!FStringNull(v.noiseMovement))
			StopSound(STRING(v.noiseMovement), CHAN_STATIC);
		// Play stop sound
		if (!FStringNull(v.noiseStopMoving))
			EmitSound(STRING(v.noiseStopMoving), CHAN_VOICE, m_volume);
		return;
	}

	// Save last target in case we need to find it again
	v.message = v.target;

	v.target = pTarg->v.target;
	m_flWait = pTarg->GetDelay();

	if (m_pevCurrentTarget && m_pevCurrentTarget->speed != 0)
	{ // don't copy speed from target if it is 0 (uninitialized)
		v.speed = m_pevCurrentTarget->speed;
		g_engfuncs.pfnAlertMessage(at_aiconsole, "Train %s speed to %4.2f\n", STRING(v.targetname), v.speed);
	}
	m_pevCurrentTarget = &pTarg->v; // keep track of this since path corners change our target for us.

	v.enemy = &pTarg->v; //hack

	if (FBitSet(m_pevCurrentTarget->spawnflags, SF_CORNER_TELEPORT))
	{
		// Path corner has indicated a teleport to the next corner.
		SetBits(v.effects, EF_NOINTERP);
		SetOrigin(pTarg->v.origin - (v.mins + v.maxs) * 0.5);
		Wait(); // Get on with doing the next path corner.
	}
	else
	{
		// Normal linear move.

		// CHANGED this from CHAN_VOICE to CHAN_STATIC around OEM beta time because trains should
		// use CHAN_STATIC for their movement sounds to prevent sound field problems.
		// this is not a hack or temporary fix, this is how things should be. (sjb).
		if (!FStringNull(v.noiseMovement))
			StopSound(STRING(v.noiseMovement), CHAN_STATIC);
		if (!FStringNull(v.noiseMovement))
			EmitSound(STRING(v.noiseMovement), CHAN_STATIC, m_volume);
		ClearBits(v.effects, EF_NOINTERP);
		SetMoveDone(&CFuncTrain::Wait);
		LinearMove(pTarg->v.origin - (v.mins + v.maxs) * 0.5, v.speed);
	}
}


void CFuncTrain::Activate()
{
	// Not yet active, so teleport to first target
	if (!m_activated)
	{
		m_activated = true;
		CBaseEntity* pevTarg = util::FindEntityByTargetname(nullptr, STRING(v.target));

		v.target = pevTarg->v.target;
		m_pevCurrentTarget = &pevTarg->v; // keep track of this since path corners change our target for us.

		SetOrigin(pevTarg->v.origin - (v.mins + v.maxs) * 0.5);

		if (FStringNull(v.targetname))
		{ // not triggered, so start immediately
			v.nextthink = v.ltime + 0.1;
			SetThink(&CFuncTrain::Next);
		}
		else
			v.spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
	}
}


bool CFuncTrain::Spawn()
{
	Precache();
	if (v.speed == 0)
		v.speed = 100;

	if (FStringNull(v.target))
		g_engfuncs.pfnAlertMessage(at_console, "FuncTrain with no target");

	if (v.dmg == 0)
		v.dmg = 2;

	v.movetype = MOVETYPE_PUSH;

	if (FBitSet(v.spawnflags, SF_TRACKTRAIN_PASSABLE))
		v.solid = SOLID_NOT;
	else
		v.solid = SOLID_BSP;

	SetModel(v.model);
	SetSize(v.mins, v.maxs);
	SetOrigin(v.origin);

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
		v.noise = 0;
		v.noise1 = 0;
		break;

	case 1:
		g_engfuncs.pfnPrecacheSound ("plats/train2.wav");
		g_engfuncs.pfnPrecacheSound ("plats/train1.wav");
		v.noise = MAKE_STRING("plats/train2.wav");
		v.noise1 = MAKE_STRING("plats/train1.wav");
		break;

	case 2:
		g_engfuncs.pfnPrecacheSound ("plats/platmove1.wav");
		g_engfuncs.pfnPrecacheSound ("plats/platstop1.wav");
		v.noise = MAKE_STRING("plats/platstop1.wav");
		v.noise1 = MAKE_STRING("plats/platmove1.wav");
		break;
	}
#endif
}


void CFuncTrain::OverrideReset()
{
	CBaseEntity* pTarg;

	// Are we moving?
	if (v.velocity != g_vecZero && v.nextthink != 0)
	{
		v.target = v.message;
		// now find our next target
		pTarg = GetNextTarget();
		if (!pTarg)
		{
			v.nextthink = 0;
			v.velocity = g_vecZero;
		}
		else // Keep moving for 0.1 secs, then find path_corner again and restart
		{
			SetThink(&CFuncTrain::Next);
			v.nextthink = v.ltime + 0.1;
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
	if (streq(pkvd->szKeyName, "wheels"))
	{
		m_length = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "height"))
	{
		m_height = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "startspeed"))
	{
		m_startSpeed = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "sounds"))
	{
		m_sounds = atoi(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "volume"))
	{
		m_flVolume = (float)(atoi(pkvd->szValue));
		m_flVolume *= 0.1;
		return true;
	}
	else if (streq(pkvd->szKeyName, "bank"))
	{
		m_flBank = atof(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}


void CFuncTrackTrain::NextThink(float thinkTime, bool alwaysThink)
{
	if (alwaysThink)
		v.flags |= FL_ALWAYSTHINK;
	else
		v.flags &= ~FL_ALWAYSTHINK;

	v.nextthink = thinkTime;
}


void CFuncTrackTrain::Blocked(CBaseEntity* pOther)
{
	// Blocker is on-ground on the train
	if (FBitSet(pOther->v.flags, FL_ONGROUND) && pOther->v.groundentity == &v)
	{
		float deltaSpeed = fabs(v.speed);
		if (deltaSpeed > 50)
			deltaSpeed = 50;
		if (0 == pOther->v.velocity.z)
			pOther->v.velocity.z += deltaSpeed;
		return;
	}
	else
		pOther->v.velocity = (pOther->v.origin - v.origin).Normalize() * v.dmg;

	g_engfuncs.pfnAlertMessage(at_aiconsole, "TRAIN(%s): Blocked by %s (dmg:%.2f)\n", STRING(v.targetname), STRING(pOther->v.classname), v.dmg);
	if (v.dmg <= 0)
		return;
	// we can't hurt this thing, so we're not concerned with it
	pOther->TakeDamage(this, this, v.dmg, DMG_CRUSH);
}


void CFuncTrackTrain::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (useType != USE_SET)
	{
		if (!ShouldToggle(useType, (v.speed != 0)))
			return;

		if (v.speed == 0)
		{
			v.speed = m_speed * m_dir;

			Next();
		}
		else
		{
			v.speed = 0;
			v.velocity = g_vecZero;
			v.avelocity = g_vecZero;
			StopSound();
			ClearThink();
		}
	}
	else
	{
		float delta = value;

		delta = ((int)(v.speed * 4) / (int)m_speed) * 0.25 + 0.25 * delta;
		if (delta > 1)
			delta = 1;
		else if (delta < -1)
			delta = -1;
		if ((v.spawnflags & SF_TRACKTRAIN_FORWARDONLY) != 0)
		{
			if (delta < 0)
				delta = 0;
		}
		v.speed = m_speed * delta;
		Next();
		g_engfuncs.pfnAlertMessage(at_aiconsole, "TRAIN(%s), speed to %.2f\n", STRING(v.targetname), v.speed);
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
	if (m_soundPlaying && !FStringNull(v.noise))
	{
		unsigned short us_encode;
		unsigned short us_sound = ((unsigned short)(m_sounds)&0x0007) << 12;

		us_encode = us_sound;

		g_engfuncs.pfnPlaybackEvent(FEV_RELIABLE | FEV_UPDATE, edict(), m_usAdjustPitch, 0.0,
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

	if (FStringNull(v.noise))
		return;

	flpitch = TRAIN_STARTPITCH + (fabs(v.speed) * (TRAIN_MAXPITCH - TRAIN_STARTPITCH) / TRAIN_MAXSPEED);

	if (!m_soundPlaying)
	{
		// play startup sound for train
		EmitSound("plats/ttrain_start1.wav", CHAN_ITEM, m_flVolume);
		EmitSound(STRING(v.noise), CHAN_STATIC, m_flVolume, ATTN_NORM, flpitch);
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

		g_engfuncs.pfnPlaybackEvent(FEV_RELIABLE | FEV_UPDATE, edict(), m_usAdjustPitch, 0.0,
			g_vecZero, g_vecZero, 0.0, 0.0, us_encode, 0, 0, 0);
	}
}


void CFuncTrackTrain::Next()
{
	float time = 0.5;

	if (0 == v.speed)
	{
		g_engfuncs.pfnAlertMessage(at_aiconsole, "TRAIN(%s): Speed is 0\n", STRING(v.targetname));
		StopSound();
		return;
	}

	if (!m_ppath)
	{
		g_engfuncs.pfnAlertMessage(at_aiconsole, "TRAIN(%s): Lost path\n", STRING(v.targetname));
		StopSound();
		return;
	}

	UpdateSound();

	Vector nextPos = v.origin;

	nextPos.z -= m_height;
	CPathTrack* pnext = m_ppath->LookAhead(&nextPos, v.speed * 0.1, true);
	nextPos.z += m_height;

	v.velocity = (nextPos - v.origin) * 10;
	Vector nextFront = v.origin;

	nextFront.z -= m_height;
	if (m_length > 0)
		m_ppath->LookAhead(&nextFront, m_length, false);
	else
		m_ppath->LookAhead(&nextFront, 100, false);
	nextFront.z += m_height;

	Vector delta = nextFront - v.origin;
	Vector angles = util::VecToAngles(delta);
	// The train actually points west
	angles.y += 180;

	// !!!  All of this crap has to be done to make the angles not wrap around, revisit this.
	FixupAngles(angles);
	FixupAngles(v.angles);

	if (!pnext || (delta.x == 0 && delta.y == 0))
		angles = v.angles;

	float vy, vx;
	if ((v.spawnflags & SF_TRACKTRAIN_NOPITCH) == 0)
		vx = util::AngleDistance(angles.x, v.angles.x);
	else
		vx = 0;
	vy = util::AngleDistance(angles.y, v.angles.y);

	v.avelocity.y = vy * 10;
	v.avelocity.x = vx * 10;

	if (m_flBank != 0)
	{
		if (v.avelocity.y < -5)
			v.avelocity.z = util::AngleDistance(util::ApproachAngle(-m_flBank, v.angles.z, m_flBank * 2), v.angles.z);
		else if (v.avelocity.y > 5)
			v.avelocity.z = util::AngleDistance(util::ApproachAngle(m_flBank, v.angles.z, m_flBank * 2), v.angles.z);
		else
			v.avelocity.z = util::AngleDistance(util::ApproachAngle(0, v.angles.z, m_flBank * 4), v.angles.z) * 4;
	}

	if (pnext)
	{
		if (pnext != m_ppath)
		{
			CPathTrack* pFire;
			if (v.speed >= 0)
				pFire = pnext;
			else
				pFire = m_ppath;

			m_ppath = pnext;
			// Fire the pass target if there is one
			if (!FStringNull(pFire->v.message))
			{
				util::FireTargets(STRING(pFire->v.message), this, this, USE_TOGGLE, 0);
				if (FBitSet(pFire->v.spawnflags, SF_PATH_FIREONCE))
					pFire->v.message = 0;
			}

			if ((pFire->v.spawnflags & SF_PATH_DISABLE_TRAIN) != 0)
				v.spawnflags |= SF_TRACKTRAIN_NOCONTROL;

			// Don't override speed if under user control
			if ((v.spawnflags & SF_TRACKTRAIN_NOCONTROL) != 0)
			{
				if (pFire->v.speed != 0)
				{ // don't copy speed from target if it is 0 (uninitialized)
					v.speed = pFire->v.speed;
					g_engfuncs.pfnAlertMessage(at_aiconsole, "TrackTrain %s speed to %4.2f\n", STRING(v.targetname), v.speed);
				}
			}
		}
		SetThink(&CFuncTrackTrain::Next);
		NextThink(v.ltime + time, true);
	}
	else // end of path, stop
	{
		StopSound();
		v.velocity = (nextPos - v.origin);
		v.avelocity = g_vecZero;
		float distance = v.velocity.Length();
		m_oldSpeed = v.speed;


		v.speed = 0;

		// Move to the dead end

		// Are we there yet?
		if (distance > 0)
		{
			// no, how long to get there?
			time = distance / m_oldSpeed;
			v.velocity = v.velocity * (m_oldSpeed / distance);
			SetThink(&CFuncTrackTrain::DeadEnd);
			NextThink(v.ltime + time, false);
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

	g_engfuncs.pfnAlertMessage(at_aiconsole, "TRAIN(%s): Dead end ", STRING(v.targetname));
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

	v.velocity = g_vecZero;
	v.avelocity = g_vecZero;
	if (pTrack)
	{
		g_engfuncs.pfnAlertMessage(at_aiconsole, "at %s\n", STRING(pTrack->v.targetname));
		if (!FStringNull(pTrack->v.netname))
			util::FireTargets(STRING(pTrack->v.netname), this, this, USE_TOGGLE, 0);
	}
	else
		g_engfuncs.pfnAlertMessage(at_aiconsole, "\n");
}


void CFuncTrackTrain::SetControls(CBaseEntity* other)
{
	Vector offset = other->v.origin - v.oldorigin;

	m_controlMins = other->v.mins + offset;
	m_controlMaxs = other->v.maxs + offset;
}


bool CFuncTrackTrain::OnControls(CBaseEntity* other)
{
	Vector offset = other->v.origin - v.origin;

	if ((v.spawnflags & SF_TRACKTRAIN_NOCONTROL) != 0)
		return false;

	// Transform offset into local coordinates
	util::MakeVectors(v.angles);
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
	auto target = util::FindEntityByTargetname(nullptr, STRING(v.target));
	if (!target)
		return;

	m_ppath = static_cast<CPathTrack*>(target);

	if (!m_ppath->Is(Type::Track))
	{
		g_engfuncs.pfnAlertMessage(at_error, "func_track_train must be on a path of path_track\n");
		m_ppath = nullptr;
		return;
	}

	Vector nextPos = m_ppath->v.origin;
	nextPos.z += m_height;

	Vector look = nextPos;
	look.z -= m_height;
	m_ppath->LookAhead(&look, m_length, false);
	look.z += m_height;

	v.angles = util::VecToAngles(look - nextPos);
	// The train actually points west
	v.angles.y += 180;

	if ((v.spawnflags & SF_TRACKTRAIN_NOPITCH) != 0)
		v.angles.x = 0;
	SetOrigin(nextPos);
	NextThink(v.ltime + 0.1, false);
	SetThink(&CFuncTrackTrain::Next);
	v.speed = m_startSpeed;

	UpdateSound();
}


void CFuncTrackTrain::NearestPath()
{
	CBaseEntity* pTrack = nullptr;
	CBaseEntity* pNearest = nullptr;
	float dist, closest;

	closest = 1024;

	while ((pTrack = util::FindEntityInSphere(pTrack, v.origin, 1024)) != nullptr)
	{
		// filter out non-tracks
		if (pTrack->Is(Type::Track))
		{
			dist = (v.origin - pTrack->v.origin).Length();
			if (dist < closest)
			{
				closest = dist;
				pNearest = pTrack;
			}
		}
	}

	if (!pNearest)
	{
		g_engfuncs.pfnAlertMessage(at_console, "Can't find a nearby track !!!\n");
		ClearThink();
		return;
	}

	g_engfuncs.pfnAlertMessage(at_aiconsole, "TRAIN: %s, Nearest track is %s\n", STRING(v.targetname), STRING(pNearest->v.targetname));
	// If I'm closer to the next path_track on this path, then it's my real path
	pTrack = ((CPathTrack*)pNearest)->GetNext();
	if (pTrack)
	{
		if ((v.origin - pTrack->v.origin).Length() < (v.origin - pNearest->v.origin).Length())
			pNearest = pTrack;
	}

	m_ppath = (CPathTrack*)pNearest;

	if (v.speed != 0)
	{
		NextThink(v.ltime + 0.1, false);
		SetThink(&CFuncTrackTrain::Next);
	}
}


void CFuncTrackTrain::OverrideReset()
{
	NextThink(v.ltime + 0.1, false);
	SetThink(&CFuncTrackTrain::NearestPath);
}


bool CFuncTrackTrain::Spawn()
{
	if (v.speed == 0)
		m_speed = 100;
	else
		m_speed = v.speed;

	v.speed = 0;
	v.velocity = g_vecZero;
	v.avelocity = g_vecZero;
	v.impulse = m_speed;

	m_dir = 1;

	if (FStringNull(v.target))
		g_engfuncs.pfnAlertMessage(at_console, "FuncTrain with no target");

	if ((v.spawnflags & SF_TRACKTRAIN_PASSABLE) != 0)
		v.solid = SOLID_NOT;
	else
		v.solid = SOLID_BSP;
	v.movetype = MOVETYPE_PUSH;

	SetModel(v.model);

	SetSize(v.mins, v.maxs);
	SetOrigin(v.origin);

	// Cache off placed origin for train controls
	v.oldorigin = v.origin;

	m_controlMins = v.mins;
	m_controlMaxs = v.maxs;
	m_controlMaxs.z += 72;
	// start trains on the next frame, to make sure their targets have had
	// a chance to spawn/activate
	NextThink(v.ltime + 0.1, false);
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
		v.noise = 0;
		break;
	case 1:
		g_engfuncs.pfnPrecacheSound("plats/ttrain1.wav");
		v.noise = MAKE_STRING("plats/ttrain1.wav");
		break;
	case 2:
		g_engfuncs.pfnPrecacheSound("plats/ttrain2.wav");
		v.noise = MAKE_STRING("plats/ttrain2.wav");
		break;
	case 3:
		g_engfuncs.pfnPrecacheSound("plats/ttrain3.wav");
		v.noise = MAKE_STRING("plats/ttrain3.wav");
		break;
	case 4:
		g_engfuncs.pfnPrecacheSound("plats/ttrain4.wav");
		v.noise = MAKE_STRING("plats/ttrain4.wav");
		break;
	case 5:
		g_engfuncs.pfnPrecacheSound("plats/ttrain6.wav");
		v.noise = MAKE_STRING("plats/ttrain6.wav");
		break;
	case 6:
		g_engfuncs.pfnPrecacheSound("plats/ttrain7.wav");
		v.noise = MAKE_STRING("plats/ttrain7.wav");
		break;
	}

	g_engfuncs.pfnPrecacheSound("plats/ttrain_brake1.wav");
	g_engfuncs.pfnPrecacheSound("plats/ttrain_start1.wav");

	m_usAdjustPitch = g_engfuncs.pfnPrecacheEvent(1, "events/train.sc");
}

// This class defines the volume of space that the player must stand in to control the train
class CFuncTrainControls : public CBaseEntity
{
public:
	CFuncTrainControls(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	bool Spawn() override;
	void EXPORT Find();
};
LINK_ENTITY_TO_CLASS(func_traincontrols, CFuncTrainControls);


void CFuncTrainControls::Find()
{
	CBaseEntity* pTarget = nullptr;

	do
	{
		pTarget = util::FindEntityByTargetname(pTarget, STRING(v.target));
	} while (pTarget != nullptr && !pTarget->Is(Type::ChooChooTrain));

	if (pTarget == nullptr)
	{
		g_engfuncs.pfnAlertMessage(at_console, "No train %s\n", STRING(v.target));
		return;
	}

	CFuncTrackTrain* ptrain = static_cast<CFuncTrackTrain*>(pTarget);
	ptrain->SetControls(this);
	Remove();
}


bool CFuncTrainControls::Spawn()
{
	v.solid = SOLID_NOT;
	v.movetype = MOVETYPE_NONE;
	SetModel(v.model);

	SetSize(v.mins, v.maxs);
	SetOrigin(v.origin);

	SetThink(&CFuncTrainControls::Find);
	v.nextthink = gpGlobals->time;

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
	CFuncTrackChange(Entity* containingEntity) : CFuncPlatRot(containingEntity) {}

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
	if (FBitSet(v.spawnflags, SF_TRACK_DONT_MOVE))
		m_vecPosition2.z = v.origin.z;

	SetupRotation();

	if (FBitSet(v.spawnflags, SF_TRACK_STARTBOTTOM))
	{
		SetOrigin(m_vecPosition2);
		m_toggle_state = TS_AT_BOTTOM;
		v.angles = m_start;
		m_targetState = TS_AT_TOP;
	}
	else
	{
		SetOrigin(m_vecPosition1);
		m_toggle_state = TS_AT_TOP;
		v.angles = m_end;
		m_targetState = TS_AT_BOTTOM;
	}

	EnableUse();
	v.nextthink = v.ltime + 2.0;
	SetThink(&CFuncTrackChange::Find);
	Precache();

	return true;
}

void CFuncTrackChange::Precache()
{
	// Can't trigger sound
	g_engfuncs.pfnPrecacheSound("buttons/button11.wav");

	CFuncPlatRot::Precache();
}


void CFuncTrackChange::Touch(CBaseEntity* pOther)
{
}



bool CFuncTrackChange::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "train"))
	{
		m_trainName = g_engfuncs.pfnAllocString(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "toptrack"))
	{
		m_trackTopName = g_engfuncs.pfnAllocString(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "bottomtrack"))
	{
		m_trackBottomName = g_engfuncs.pfnAllocString(pkvd->szValue);
		return true;
	}

	return CFuncPlatRot::KeyValue(pkvd); // Pass up to base class
}


void CFuncTrackChange::OverrideReset()
{
	v.nextthink = v.ltime + 1.0;
	SetThink(&CFuncTrackChange::Find);
}

void CFuncTrackChange::Find()
{
	// Find track entities
	CBaseEntity* target;

	target = util::FindEntityByTargetname(nullptr, STRING(m_trackTopName));
	if (target != nullptr)
	{
		m_trackTop = static_cast<CPathTrack*>(target);
		target = util::FindEntityByTargetname(nullptr, STRING(m_trackBottomName));
		if (target != nullptr)
		{
			m_trackBottom = static_cast<CPathTrack*>(target);
			target = util::FindEntityByTargetname(nullptr, STRING(m_trainName));
			if (target != nullptr)
			{
				target = util::FindEntityByTargetname(nullptr, STRING(m_trainName));
				if (target == nullptr)
				{
					g_engfuncs.pfnAlertMessage(at_error, "Can't find train for track change! %s\n", STRING(m_trainName));
					return;
				}
				m_train = static_cast<CFuncTrackTrain*>(target);
				Vector center = (v.absmin + v.absmax) * 0.5;
				m_trackBottom = m_trackBottom->Nearest(center);
				m_trackTop = m_trackTop->Nearest(center);
				UpdateAutoTargets(m_toggle_state);
				ClearThink();
				return;
			}
			else
			{
				g_engfuncs.pfnAlertMessage(at_error, "Can't find train for track change! %s\n", STRING(m_trainName));
				target = util::FindEntityByTargetname(nullptr, STRING(m_trainName));
			}
		}
		else
			g_engfuncs.pfnAlertMessage(at_error, "Can't find bottom track for track change! %s\n", STRING(m_trackBottomName));
	}
	else
		g_engfuncs.pfnAlertMessage(at_error, "Can't find top track for track change! %s\n", STRING(m_trackTopName));
}



TRAIN_CODE CFuncTrackChange::EvaluateTrain(CPathTrack* pcurrent)
{
	// Go ahead and work, we don't have anything to switch, so just be an elevator
	if (!pcurrent || !m_train)
		return TRAIN_SAFE;

	if (m_train->m_ppath == pcurrent || (pcurrent->m_pprevious && m_train->m_ppath == pcurrent->m_pprevious) ||
		(pcurrent->m_pnext && m_train->m_ppath == pcurrent->m_pnext))
	{
		if (m_train->v.speed != 0)
			return TRAIN_BLOCKING;

		Vector dist = v.origin - m_train->v.origin;
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
	float time = (v.nextthink - v.ltime);

	m_train->v.velocity = v.velocity;
	m_train->v.avelocity = v.avelocity;
	m_train->NextThink(m_train->v.ltime + time, false);

	// Attempt at getting the train to rotate properly around the origin of the trackchange
	if (time <= 0)
		return;

	Vector offset = m_train->v.origin - v.origin;
	Vector delta = dest - v.angles;
	// Transform offset into local coordinates
	util::MakeInvVectors(delta, gpGlobals);
	Vector local;
	local.x = DotProduct(offset, gpGlobals->v_forward);
	local.y = DotProduct(offset, gpGlobals->v_right);
	local.z = DotProduct(offset, gpGlobals->v_up);

	local = local - offset;
	m_train->v.velocity = v.velocity + (local * (1.0 / time));
}

void CFuncTrackChange::GoDown()
{
	if (m_code == TRAIN_BLOCKING)
		return;

	// HitBottom may get called during CFuncPlat::GoDown(), so set up for that
	// before you call GoDown()

	UpdateAutoTargets(TS_GOING_DOWN);
	// If ROTMOVE, move & rotate
	if (FBitSet(v.spawnflags, SF_TRACK_DONT_MOVE))
	{
		SetMoveDone(&CFuncTrackChange::CallHitBottom);
		m_toggle_state = TS_GOING_DOWN;
		AngularMove(m_start, v.speed);
	}
	else
	{
		CFuncPlat::GoDown();
		SetMoveDone(&CFuncTrackChange::CallHitBottom);
		RotMove(m_start, v.nextthink - v.ltime);
	}
	// Otherwise, rotate first, move second

	// If the train is moving with the platform, update it
	if (m_code == TRAIN_FOLLOWING)
	{
		UpdateTrain(m_start);
		m_train->m_ppath = nullptr;
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
	if (FBitSet(v.spawnflags, SF_TRACK_DONT_MOVE))
	{
		m_toggle_state = TS_GOING_UP;
		SetMoveDone(&CFuncTrackChange::CallHitTop);
		AngularMove(m_end, v.speed);
	}
	else
	{
		// If ROTMOVE, move & rotate
		CFuncPlat::GoUp();
		SetMoveDone(&CFuncTrackChange::CallHitTop);
		RotMove(m_end, v.nextthink - v.ltime);
	}

	// Otherwise, move first, rotate second

	// If the train is moving with the platform, update it
	if (m_code == TRAIN_FOLLOWING)
	{
		UpdateTrain(m_end);
		m_train->m_ppath = nullptr;
	}
}


// Normal track change
void CFuncTrackChange::UpdateAutoTargets(int toggleState)
{
	if (!m_trackTop || !m_trackBottom)
		return;

	if (toggleState == TS_AT_TOP)
		ClearBits(m_trackTop->v.spawnflags, SF_PATH_DISABLED);
	else
		SetBits(m_trackTop->v.spawnflags, SF_PATH_DISABLED);

	if (toggleState == TS_AT_BOTTOM)
		ClearBits(m_trackBottom->v.spawnflags, SF_PATH_DISABLED);
	else
		SetBits(m_trackBottom->v.spawnflags, SF_PATH_DISABLED);
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
	ClearThink();
	v.nextthink = -1;

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
	ClearThink();
	v.nextthink = -1;
	UpdateAutoTargets(m_toggle_state);
	EnableUse();
}



class CFuncTrackAuto : public CFuncTrackChange
{
public:
	CFuncTrackAuto(Entity* containingEntity) : CFuncTrackChange(containingEntity) {}

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
		ClearBits(pTarget->v.spawnflags, SF_PATH_DISABLED);
		if (m_code == TRAIN_FOLLOWING && m_train && m_train->v.speed == 0)
			m_train->Use(this, this, USE_ON, 0);
	}

	if (pNextTarget)
		SetBits(pNextTarget->v.spawnflags, SF_PATH_DISABLED);
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
		pTarget = nullptr;

	if (pActivator->Is(Type::ChooChooTrain))
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

