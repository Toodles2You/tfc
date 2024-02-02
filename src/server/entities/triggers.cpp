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

===== triggers.cpp ========================================================

  spawn and use functions for editor-placed triggers              

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "saverestore.h"
#include "trains.h" // trigger_camera has train functionality
#include "gamerules.h"

#define SF_TRIGGER_ALLOWMONSTERS 1 // monsters allowed to fire this trigger
#define SF_TRIGGER_NOCLIENTS 2	   // players not allowed to fire this trigger
#define SF_TRIGGER_PUSHABLES 4	   // only pushables can fire this trigger

#define SF_TRIG_PUSH_ONCE 1

#define SF_TRIGGER_PUSH_START_OFF 2		   //spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_TARGETONCE 1	   // Only fire hurt target once
#define SF_TRIGGER_HURT_START_OFF 2		   //spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_NO_CLIENTS 8	   //spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_CLIENTONLYFIRE 16  // trigger hurt will only fire its target if it is hurting a client
#define SF_TRIGGER_HURT_CLIENTONLYTOUCH 32 // only clients may touch this trigger.

class CFrictionModifier : public CBaseEntity
{
public:
	DECLARE_SAVERESTORE()

	bool Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT ChangeFriction(CBaseEntity* pOther);

	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	float m_frictionFraction; // Sorry, couldn't resist this name :)
};

LINK_ENTITY_TO_CLASS(func_friction, CFrictionModifier);

// Global Savedata for changelevel friction modifier
#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CFrictionModifier)
	DEFINE_FIELD(CFrictionModifier, m_frictionFraction, FIELD_FLOAT),
END_SAVERESTORE(CFrictionModifier, CBaseEntity)
#endif


// Modify an entity's friction
bool CFrictionModifier::Spawn()
{
	pev->solid = SOLID_TRIGGER;
	SetModel(STRING(pev->model)); // set size and link into world
	pev->movetype = MOVETYPE_NONE;
	SetTouch(&CFrictionModifier::ChangeFriction);
	return true;
}


// Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
void CFrictionModifier::ChangeFriction(CBaseEntity* pOther)
{
	if (pOther->pev->movetype != MOVETYPE_BOUNCEMISSILE && pOther->pev->movetype != MOVETYPE_BOUNCE)
		pOther->pev->friction = m_frictionFraction;
}



// Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
bool CFrictionModifier::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "modifier"))
	{
		m_frictionFraction = atof(pkvd->szValue) / 100.0;
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}


// This trigger will fire when the level spawns (or respawns if not fire once)
// It will check a global state before firing.  It supports delay and killtargets

#define SF_AUTO_FIREONCE 0x0001

class CAutoTrigger : public CBaseEntity
{
public:
	DECLARE_SAVERESTORE()

	bool KeyValue(KeyValueData* pkvd) override;
	bool Spawn() override;
	void Precache() override;
	void Think() override;

	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

private:
	int m_globalstate;
	USE_TYPE triggerType;
};
LINK_ENTITY_TO_CLASS(trigger_auto, CAutoTrigger);

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CAutoTrigger)
	DEFINE_FIELD(CAutoTrigger, m_globalstate, FIELD_STRING),
	DEFINE_FIELD(CAutoTrigger, triggerType, FIELD_INTEGER),
END_SAVERESTORE(CAutoTrigger, CBaseDelay)
#endif

bool CAutoTrigger::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "globalstate"))
	{
		m_globalstate = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "triggerstate"))
	{
		int type = atoi(pkvd->szValue);
		switch (type)
		{
		case 0:
			triggerType = USE_OFF;
			break;
		case 2:
			triggerType = USE_TOGGLE;
			break;
		default:
			triggerType = USE_ON;
			break;
		}
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}


bool CAutoTrigger::Spawn()
{
	Precache();
	return true;
}


void CAutoTrigger::Precache()
{
	pev->nextthink = gpGlobals->time + 0.1;
}


void CAutoTrigger::Think()
{
	if (FStringNull(m_globalstate) || gGlobalState.EntityGetState(m_globalstate) == GLOBAL_ON)
	{
		UseTargets(this, triggerType, 0);
		if ((pev->spawnflags & SF_AUTO_FIREONCE) != 0)
			Remove();
	}
}



#define SF_RELAY_FIREONCE 0x0001

class CTriggerRelay : public CBaseEntity
{
public:
	DECLARE_SAVERESTORE()

	bool KeyValue(KeyValueData* pkvd) override;
	bool Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

private:
	USE_TYPE triggerType;
};
LINK_ENTITY_TO_CLASS(trigger_relay, CTriggerRelay);

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CTriggerRelay)
	DEFINE_FIELD(CTriggerRelay, triggerType, FIELD_INTEGER),
END_SAVERESTORE(CTriggerRelay, CBaseDelay)
#endif

bool CTriggerRelay::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "triggerstate"))
	{
		int type = atoi(pkvd->szValue);
		switch (type)
		{
		case 0:
			triggerType = USE_OFF;
			break;
		case 2:
			triggerType = USE_TOGGLE;
			break;
		default:
			triggerType = USE_ON;
			break;
		}
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}


bool CTriggerRelay::Spawn()
{
	return true;
}




void CTriggerRelay::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	UseTargets(this, triggerType, 0);
	if ((pev->spawnflags & SF_RELAY_FIREONCE) != 0)
		Remove();
}


//**********************************************************
// The Multimanager Entity - when fired, will fire up to 16 targets
// at specified times.
// FLAG:		THREAD (create clones when triggered)
// FLAG:		CLONE (this is a clone for a threaded execution)

#define SF_MULTIMAN_CLONE 0x80000000
#define SF_MULTIMAN_THREAD 0x00000001

class CMultiManager : public CBaseToggle
{
public:
	DECLARE_SAVERESTORE()

	bool KeyValue(KeyValueData* pkvd) override;
	bool Spawn() override;
	void EXPORT ManagerThink();
	void EXPORT ManagerUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

#ifndef NDEBUG
	void EXPORT ManagerReport();
#endif

	bool HasTarget(string_t targetname) override;

	int ObjectCaps() override { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	int m_cTargets;							  // the total number of targets in this manager's fire list.
	int m_index;							  // Current target
	float m_startTime;						  // Time we started firing
	int m_iTargetName[MAX_MULTI_TARGETS];	  // list if indexes into global string array
	float m_flTargetDelay[MAX_MULTI_TARGETS]; // delay (in seconds) from time of manager fire to target fire
private:
	inline bool IsClone() { return (pev->spawnflags & SF_MULTIMAN_CLONE) != 0; }
	inline bool ShouldClone()
	{
		if (IsClone())
			return false;

		return (pev->spawnflags & SF_MULTIMAN_THREAD) != 0;
	}

	CMultiManager* Clone();
};
LINK_ENTITY_TO_CLASS(multi_manager, CMultiManager);

// Global Savedata for multi_manager
#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CMultiManager)
	DEFINE_FIELD(CMultiManager, m_cTargets, FIELD_INTEGER),
	DEFINE_FIELD(CMultiManager, m_index, FIELD_INTEGER),
	DEFINE_FIELD(CMultiManager, m_startTime, FIELD_TIME),
	DEFINE_ARRAY(CMultiManager, m_iTargetName, FIELD_STRING, MAX_MULTI_TARGETS),
	DEFINE_ARRAY(CMultiManager, m_flTargetDelay, FIELD_FLOAT, MAX_MULTI_TARGETS),
END_SAVERESTORE(CMultiManager, CBaseToggle)
#endif

bool CMultiManager::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		return true;
	}
	else // add this field to the target list
	{
		// this assumes that additional fields are targetnames and their values are delay values.
		if (m_cTargets < MAX_MULTI_TARGETS)
		{
			char tmp[128];

			util::StripToken(pkvd->szKeyName, tmp);
			m_iTargetName[m_cTargets] = ALLOC_STRING(tmp);
			m_flTargetDelay[m_cTargets] = atof(pkvd->szValue);
			m_cTargets++;
			return true;
		}
	}

	return false;
}


bool CMultiManager::Spawn()
{
	pev->solid = SOLID_NOT;
	SetUse(&CMultiManager::ManagerUse);
	SetThink(&CMultiManager::ManagerThink);

	// Sort targets
	// Quick and dirty bubble sort
	bool swapped = true;

	while (swapped)
	{
		swapped = false;
		for (int i = 1; i < m_cTargets; i++)
		{
			if (m_flTargetDelay[i] < m_flTargetDelay[i - 1])
			{
				// Swap out of order elements
				int name = m_iTargetName[i];
				float delay = m_flTargetDelay[i];
				m_iTargetName[i] = m_iTargetName[i - 1];
				m_flTargetDelay[i] = m_flTargetDelay[i - 1];
				m_iTargetName[i - 1] = name;
				m_flTargetDelay[i - 1] = delay;
				swapped = true;
			}
		}
	}

	return true;
}


bool CMultiManager::HasTarget(string_t targetname)
{
	for (int i = 0; i < m_cTargets; i++)
		if (FStrEq(STRING(targetname), STRING(m_iTargetName[i])))
			return true;

	return false;
}


// Designers were using this to fire targets that may or may not exist --
// so I changed it to use the standard target fire code, made it a little simpler.
void CMultiManager::ManagerThink()
{
	float time;

	time = gpGlobals->time - m_startTime;
	while (m_index < m_cTargets && m_flTargetDelay[m_index] <= time)
	{
		util::FireTargets(STRING(m_iTargetName[m_index]), m_hActivator, this, USE_TOGGLE, 0);
		m_index++;
	}

	if (m_index >= m_cTargets) // have we fired all targets?
	{
		SetThink(NULL);
		if (IsClone())
		{
			Remove();
			return;
		}
		SetUse(&CMultiManager::ManagerUse); // allow manager re-use
	}
	else
		pev->nextthink = m_startTime + m_flTargetDelay[m_index];
}

CMultiManager* CMultiManager::Clone()
{
	CMultiManager* pMulti = GetClassPtr((CMultiManager*)NULL);

	edict_t* pEdict = pMulti->pev->pContainingEntity;
	memcpy(pMulti->pev, pev, sizeof(*pev));
	pMulti->pev->pContainingEntity = pEdict;

	pMulti->pev->spawnflags |= SF_MULTIMAN_CLONE;
	pMulti->m_cTargets = m_cTargets;
	memcpy(pMulti->m_iTargetName, m_iTargetName, sizeof(m_iTargetName));
	memcpy(pMulti->m_flTargetDelay, m_flTargetDelay, sizeof(m_flTargetDelay));

	return pMulti;
}


// The USE function builds the time table and starts the entity thinking.
void CMultiManager::ManagerUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	// In multiplayer games, clone the MM and execute in the clone (like a thread)
	// to allow multiple players to trigger the same multimanager
	if (ShouldClone())
	{
		CMultiManager* pClone = Clone();
		pClone->ManagerUse(pActivator, pCaller, useType, value);
		return;
	}

	m_hActivator = pActivator;
	m_index = 0;
	m_startTime = gpGlobals->time;

	SetUse(NULL); // disable use until all targets have fired

	SetThink(&CMultiManager::ManagerThink);
	pev->nextthink = gpGlobals->time;
}

#ifndef NDEBUG
void CMultiManager::ManagerReport()
{
	int cIndex;

	for (cIndex = 0; cIndex < m_cTargets; cIndex++)
	{
		ALERT(at_console, "%s %f\n", STRING(m_iTargetName[cIndex]), m_flTargetDelay[cIndex]);
	}
}
#endif

//***********************************************************


//
// Render parameters trigger
//
// This entity will copy its render parameters (renderfx, rendermode, rendercolor, renderamt)
// to its targets when triggered.
//


// Flags to indicate masking off various render parameters that are normally copied to the targets
#define SF_RENDER_MASKFX (1 << 0)
#define SF_RENDER_MASKAMT (1 << 1)
#define SF_RENDER_MASKMODE (1 << 2)
#define SF_RENDER_MASKCOLOR (1 << 3)

class CRenderFxManager : public CBaseEntity
{
public:
	bool Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
};

LINK_ENTITY_TO_CLASS(env_render, CRenderFxManager);


bool CRenderFxManager::Spawn()
{
	pev->solid = SOLID_NOT;
	return true;
}

void CRenderFxManager::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!FStringNull(pev->target))
	{
		edict_t* pentTarget = NULL;
		while (true)
		{
			pentTarget = FIND_ENTITY_BY_TARGETNAME(pentTarget, STRING(pev->target));
			if (FNullEnt(pentTarget))
				break;

			entvars_t* pevTarget = VARS(pentTarget);
			if (!FBitSet(pev->spawnflags, SF_RENDER_MASKFX))
				pevTarget->renderfx = pev->renderfx;
			if (!FBitSet(pev->spawnflags, SF_RENDER_MASKAMT))
				pevTarget->renderamt = pev->renderamt;
			if (!FBitSet(pev->spawnflags, SF_RENDER_MASKMODE))
				pevTarget->rendermode = pev->rendermode;
			if (!FBitSet(pev->spawnflags, SF_RENDER_MASKCOLOR))
				pevTarget->rendercolor = pev->rendercolor;
		}
	}
}



class CBaseTrigger : public CBaseToggle
{
public:
	void EXPORT TeleportTouch(CBaseEntity* pOther);
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT MultiTouch(CBaseEntity* pOther);
	void EXPORT HurtTouch(CBaseEntity* pOther);
	void EXPORT CDAudioTouch(CBaseEntity* pOther);
	void ActivateMultiTrigger(CBaseEntity* pActivator);
	void EXPORT MultiWaitOver();
	void EXPORT CounterUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void InitTrigger();

	int ObjectCaps() override { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};

LINK_ENTITY_TO_CLASS(trigger, CBaseTrigger);

/*
================
InitTrigger
================
*/
void CBaseTrigger::InitTrigger()
{
	// trigger angles are used for one-way touches.  An angle of 0 is assumed
	// to mean no restrictions, so use a yaw of 360 instead.
	if (pev->angles != g_vecZero)
		pev->movedir = util::SetMovedir(pev->angles);
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	SetModel(STRING(pev->model)); // set size and link into world
	SetBits(pev->effects, EF_NODRAW);
}


//
// Cache user-entity-field values until spawn is called.
//

bool CBaseTrigger::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "damage"))
	{
		pev->dmg = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "count"))
	{
		m_cTriggersLeft = (int)atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "damagetype"))
	{
		m_bitsDamageInflict = atoi(pkvd->szValue);
		return true;
	}

	return CBaseToggle::KeyValue(pkvd);
}

class CTriggerHurt : public CBaseTrigger
{
public:
	bool Spawn() override;
	void EXPORT RadiationThink();
};

LINK_ENTITY_TO_CLASS(trigger_hurt, CTriggerHurt);

//
// trigger_monsterjump
//
class CTriggerMonsterJump : public CBaseTrigger
{
public:
	bool Spawn() override;
	void Touch(CBaseEntity* pOther) override;
	void Think() override;
};

LINK_ENTITY_TO_CLASS(trigger_monsterjump, CTriggerMonsterJump);


bool CTriggerMonsterJump::Spawn()
{
	pev->movedir = util::SetMovedir(pev->angles);

	InitTrigger();

	pev->nextthink = 0;
	pev->speed = 200;
	m_flHeight = 150;

	if (!FStringNull(pev->targetname))
	{ // if targetted, spawn turned off
		pev->solid = SOLID_NOT;
		SetOrigin(pev->origin); // Unlink from trigger list
		SetUse(&CTriggerMonsterJump::ToggleUse);
	}

	return true;
}


void CTriggerMonsterJump::Think()
{
	pev->solid = SOLID_NOT;
	SetOrigin(pev->origin); // Unlink from trigger list
	SetThink(NULL);
}

void CTriggerMonsterJump::Touch(CBaseEntity* pOther)
{
	entvars_t* pevOther = pOther->pev;

	if (!FBitSet(pevOther->flags, FL_MONSTER))
	{ // touched by a non-monster.
		return;
	}

	pevOther->origin.z += 1;

	if (FBitSet(pevOther->flags, FL_ONGROUND))
	{ // clear the onground so physics don't bitch
		pevOther->flags &= ~FL_ONGROUND;
	}

	// toss the monster!
	pevOther->velocity = pev->movedir * pev->speed;
	pevOther->velocity.z += m_flHeight;
	pev->nextthink = gpGlobals->time;
}


//=====================================
//
// trigger_cdaudio - starts/stops cd audio tracks
//
class CTriggerCDAudio : public CBaseTrigger
{
public:
	bool Spawn() override;

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void PlayTrack();
	void Touch(CBaseEntity* pOther) override;
};

LINK_ENTITY_TO_CLASS(trigger_cdaudio, CTriggerCDAudio);

//
// Changes tracks or stops CD when player touches
//
// !!!HACK - overloaded HEALTH to avoid adding new field
void CTriggerCDAudio::Touch(CBaseEntity* pOther)
{
	if (!pOther->IsPlayer())
	{ // only clients may trigger these events
		return;
	}

	PlayTrack();
}

bool CTriggerCDAudio::Spawn()
{
	InitTrigger();
	return true;
}

void CTriggerCDAudio::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	PlayTrack();
}

void PlayCDTrack(int iTrack)
{
	// manually find the single player.
	CBaseEntity* pClient = util::GetLocalPlayer();

	// Can't play if the client is not connected!
	if (!pClient)
		return;

	if (iTrack < -1 || iTrack > 30)
	{
		ALERT(at_console, "TriggerCDAudio - Track %d out of range\n");
		return;
	}

	if (iTrack == -1)
	{
		CLIENT_COMMAND(pClient->edict(), "cd stop\n");
	}
	else
	{
		char string[64];

		sprintf(string, "cd play %3d\n", iTrack);
		CLIENT_COMMAND(pClient->edict(), string);
	}
}


// only plays for ONE client, so only use in single play!
void CTriggerCDAudio::PlayTrack()
{
	PlayCDTrack((int)pev->health);

	SetTouch(NULL);
	Remove();
}


// This plays a CD track when fired or when the player enters it's radius
class CTargetCDAudio : public CPointEntity
{
public:
	bool Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void Think() override;
	void Play();
};

LINK_ENTITY_TO_CLASS(target_cdaudio, CTargetCDAudio);

bool CTargetCDAudio::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "radius"))
	{
		pev->scale = atof(pkvd->szValue);
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

bool CTargetCDAudio::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	if (pev->scale > 0)
		pev->nextthink = gpGlobals->time + 1.0;

	return true;
}

void CTargetCDAudio::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	Play();
}

// only plays for ONE client, so only use in single play!
void CTargetCDAudio::Think()
{
	// manually find the single player.
	CBaseEntity* pClient = util::GetLocalPlayer();

	// Can't play if the client is not connected!
	if (!pClient)
		return;

	pev->nextthink = gpGlobals->time + 0.5;

	if ((pClient->pev->origin - pev->origin).Length() <= pev->scale)
		Play();
}

void CTargetCDAudio::Play()
{
	PlayCDTrack((int)pev->health);
	Remove();
}

//=====================================

//
// trigger_hurt - hurts anything that touches it. if the trigger has a targetname, firing it will toggle state
//
//int gfToggleState = 0; // used to determine when all radiation trigger hurts have called 'RadiationThink'

bool CTriggerHurt::Spawn()
{
	InitTrigger();
	SetTouch(&CTriggerHurt::HurtTouch);

	if (!FStringNull(pev->targetname))
	{
		SetUse(&CTriggerHurt::ToggleUse);
	}
	else
	{
		SetUse(NULL);
	}

	if ((m_bitsDamageInflict & DMG_RADIATION) != 0)
	{
		SetThink(&CTriggerHurt::RadiationThink);
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT(0.0, 0.5);
	}

	if (FBitSet(pev->spawnflags, SF_TRIGGER_HURT_START_OFF)) // if flagged to Start Turned Off, make trigger nonsolid.
		pev->solid = SOLID_NOT;

	SetOrigin(pev->origin); // Link into the list

	return true;
}

// trigger hurt that causes radiation will do a radius
// check and set the player's geiger counter level
// according to distance from center of trigger

void CTriggerHurt::RadiationThink()
{

	edict_t* pentPlayer;
	CBasePlayer* pPlayer = NULL;
	float flRange;
	entvars_t* pevTarget;
	Vector vecSpot1;
	Vector vecSpot2;
	Vector vecRange;
	Vector origin;
	Vector view_ofs;

	// check to see if a player is in pvs
	// if not, continue

	// set origin to center of trigger so that this check works
	origin = pev->origin;
	view_ofs = pev->view_ofs;

	pev->origin = (pev->absmin + pev->absmax) * 0.5;
	pev->view_ofs = pev->view_ofs * 0.0;

	pentPlayer = FIND_CLIENT_IN_PVS(edict());

	pev->origin = origin;
	pev->view_ofs = view_ofs;

	// reset origin

	if (!FNullEnt(pentPlayer))
	{

		pPlayer = GetClassPtr((CBasePlayer*)VARS(pentPlayer));

		pevTarget = VARS(pentPlayer);

		// get range to player;

		vecSpot1 = (pev->absmin + pev->absmax) * 0.5;
		vecSpot2 = (pevTarget->absmin + pevTarget->absmax) * 0.5;

		vecRange = vecSpot1 - vecSpot2;
		flRange = vecRange.Length();
	}

	pev->nextthink = gpGlobals->time + 0.25;
}

//
// ToggleUse - If this is the USE function for a trigger, its state will toggle every time it's fired
//
void CBaseTrigger::ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (pev->solid == SOLID_NOT)
	{ // if the trigger is off, turn it on
		pev->solid = SOLID_TRIGGER;

		// Force retouch
		gpGlobals->force_retouch++;
	}
	else
	{ // turn the trigger off
		pev->solid = SOLID_NOT;
	}
	SetOrigin(pev->origin);
}

// When touched, a hurt trigger does DMG points of damage each half-second
void CBaseTrigger::HurtTouch(CBaseEntity* pOther)
{
	float fldmg;

	if (0 == pOther->pev->takedamage)
		return;

	if ((pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYTOUCH) != 0 && !pOther->IsPlayer())
	{
		// this trigger is only allowed to touch clients, and this ain't a client.
		return;
	}

	if ((pev->spawnflags & SF_TRIGGER_HURT_NO_CLIENTS) != 0 && pOther->IsPlayer())
		return;

	static_assert(MAX_PLAYERS <= 32, "Rework the player mask logic to support more than 32 players");

	// HACKHACK -- In multiplayer, players touch this based on packet receipt.
	// So the players who send packets later aren't always hurt.  Keep track of
	// how much time has passed and whether or not you've touched that player
	if (util::IsMultiplayer())
	{
		if (pev->dmgtime > gpGlobals->time)
		{
			if (gpGlobals->time != pev->pain_finished)
			{ // too early to hurt again, and not same frame with a different entity
				if (pOther->IsPlayer())
				{
					int playerMask = 1 << (pOther->entindex() - 1);

					// If I've already touched this player (this time), then bail out
					if ((pev->impulse & playerMask) != 0)
						return;

					// Mark this player as touched
					// BUGBUG - There can be only 32 players!
					pev->impulse |= playerMask;
				}
				else
				{
					return;
				}
			}
		}
		else
		{
			// New clock, "un-touch" all players
			pev->impulse = 0;
			if (pOther->IsPlayer())
			{
				int playerMask = 1 << (pOther->entindex() - 1);

				// Mark this player as touched
				// BUGBUG - There can be only 32 players!
				pev->impulse |= playerMask;
			}
		}
	}
	else // Original code -- single player
	{
		if (pev->dmgtime > gpGlobals->time && gpGlobals->time != pev->pain_finished)
		{ // too early to hurt again, and not same frame with a different entity
			return;
		}
	}



	// If this is time_based damage (poison, radiation), override the pev->dmg with a
	// default for the given damage type.  Monsters only take time-based damage
	// while touching the trigger.  Player continues taking damage for a while after
	// leaving the trigger

	fldmg = pev->dmg * 0.5; // 0.5 seconds worth of damage, pev->dmg is damage/second

	if (fldmg < 0)
		pOther->GiveHealth(-fldmg, m_bitsDamageInflict);
	else
		pOther->TakeDamage(this, this, fldmg, m_bitsDamageInflict);

	// Store pain time so we can get all of the other entities on this frame
	pev->pain_finished = gpGlobals->time;

	// Apply damage every half second
	pev->dmgtime = gpGlobals->time + 0.5; // half second delay until this trigger can hurt toucher again



	if (!FStringNull(pev->target))
	{
		// trigger has a target it wants to fire.
		if ((pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYFIRE) != 0)
		{
			// if the toucher isn't a client, don't fire the target!
			if (!pOther->IsPlayer())
			{
				return;
			}
		}

		UseTargets(pOther, USE_TOGGLE, 0);
		if ((pev->spawnflags & SF_TRIGGER_HURT_TARGETONCE) != 0)
			pev->target = 0;
	}
}


class CTriggerMultiple : public CBaseTrigger
{
public:
	bool Spawn() override;
};

LINK_ENTITY_TO_CLASS(trigger_multiple, CTriggerMultiple);


bool CTriggerMultiple::Spawn()
{
	if (m_flWait == 0)
		m_flWait = 0.2;

	InitTrigger();

	SetTouch(&CTriggerMultiple::MultiTouch);

	return true;
}


class CTriggerOnce : public CTriggerMultiple
{
public:
	bool Spawn() override;
};

LINK_ENTITY_TO_CLASS(trigger_once, CTriggerOnce);
bool CTriggerOnce::Spawn()
{
	m_flWait = -1;

	return CTriggerMultiple::Spawn();
}



void CBaseTrigger::MultiTouch(CBaseEntity* pOther)
{
	entvars_t* pevToucher;

	pevToucher = pOther->pev;

	// Only touch clients, monsters, or pushables (depending on flags)
	if (((pevToucher->flags & FL_CLIENT) != 0 && (pev->spawnflags & SF_TRIGGER_NOCLIENTS) == 0) ||
		((pevToucher->flags & FL_MONSTER) != 0 && (pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS) != 0) ||
		(pev->spawnflags & SF_TRIGGER_PUSHABLES) != 0 && FClassnameIs(pevToucher, "func_pushable"))
	{

#if 0
		// if the trigger has an angles field, check player's facing direction
		if (pev->movedir != g_vecZero)
		{
			util::MakeVectors( pevToucher->angles );
			if ( DotProduct( gpGlobals->v_forward, pev->movedir ) < 0 )
				return;         // not facing the right way
		}
#endif

		ActivateMultiTrigger(pOther);
	}
}


//
// the trigger was just touched/killed/used
// self.enemy should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
//
void CBaseTrigger::ActivateMultiTrigger(CBaseEntity* pActivator)
{
	if (pev->nextthink > gpGlobals->time)
		return; // still waiting for reset time

	if (!util::IsMasterTriggered(m_sMaster, pActivator))
		return;

	if (FClassnameIs(pev, "trigger_secret"))
	{
		if (pev->enemy == NULL || !FClassnameIs(pev->enemy, "player"))
			return;
		gpGlobals->found_secrets++;
	}

	if (!FStringNull(pev->noise))
		EmitSound(STRING(pev->noise), CHAN_VOICE);

	// don't trigger again until reset

	m_hActivator = pActivator;
	UseTargets(m_hActivator, USE_TOGGLE, 0);

	if (!FStringNull(pev->message) && pActivator->IsPlayer())
	{
		util::ShowMessage(STRING(pev->message), pActivator);
		//		CLIENT_PRINTF( ENT( pActivator->pev ), print_center, STRING(pev->message) );
	}

	if (m_flWait > 0)
	{
		SetThink(&CBaseTrigger::MultiWaitOver);
		pev->nextthink = gpGlobals->time + m_flWait;
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while C code is looping through area links...
		SetTouch(NULL);
		Remove();
	}
}


// the wait time has passed, so set back up for another activation
void CBaseTrigger::MultiWaitOver()
{
	SetThink(NULL);
}


// ========================= COUNTING TRIGGER =====================================

//
// GLOBALS ASSUMED SET:  g_eoActivator
//
void CBaseTrigger::CounterUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	m_cTriggersLeft--;
	m_hActivator = pActivator;

	if (m_cTriggersLeft != 0)
	{
		return;
	}

	ActivateMultiTrigger(m_hActivator);
}


class CTriggerCounter : public CBaseTrigger
{
public:
	bool Spawn() override;
};
LINK_ENTITY_TO_CLASS(trigger_counter, CTriggerCounter);

bool CTriggerCounter::Spawn()
{
	// By making the flWait be -1, this counter-trigger will disappear after it's activated
	// (but of course it needs cTriggersLeft "uses" before that happens).
	m_flWait = -1;

	if (m_cTriggersLeft == 0)
		m_cTriggersLeft = 2;
	SetUse(&CTriggerCounter::CounterUse);

	return true;
}

// ====================== TRIGGER_CHANGELEVEL ================================

class CTriggerVolume : public CPointEntity // Derive from point entity so this doesn't move across levels
{
public:
	bool Spawn() override;
};

LINK_ENTITY_TO_CLASS(trigger_transition, CTriggerVolume);

// Define space that travels across a level transition
bool CTriggerVolume::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SetModel(STRING(pev->model)); // set size and link into world
	pev->model = iStringNull;
	pev->modelindex = 0;
	return true;
}


// Fires a target after level transition and then dies
class CFireAndDie : public CBaseEntity
{
public:
	bool Spawn() override;
	void Precache() override;
	void Think() override;
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() | FCAP_FORCE_TRANSITION; } // Always go across transitions
};
LINK_ENTITY_TO_CLASS(fireanddie, CFireAndDie);

bool CFireAndDie::Spawn()
{
	pev->classname = MAKE_STRING("fireanddie");
	// Don't call Precache() - it should be called on restore
	return true;
}


void CFireAndDie::Precache()
{
	// This gets called on restore
	pev->nextthink = gpGlobals->time + m_flDelay;
}


void CFireAndDie::Think()
{
	UseTargets(this, USE_TOGGLE, 0);
	Remove();
}


#define SF_CHANGELEVEL_USEONLY 0x0002

static constexpr int kMapNameMost = 32;

static char st_szNextMap[kMapNameMost];
static char st_szNextSpot[kMapNameMost];

class CChangeLevel : public CBaseTrigger
{
public:
	DECLARE_SAVERESTORE()

	bool Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT UseChangeLevel(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT TriggerChangeLevel();
	void EXPORT TouchChangeLevel(CBaseEntity* pOther);
	void ChangeLevelNow(CBaseEntity* pActivator);

	static edict_t* FindLandmark(const char* pLandmarkName);
#ifdef HALFLIFE_SAVERESTORE
	static int ChangeList(LEVELLIST* pLevelList, int maxList);
	static bool AddTransitionToList(LEVELLIST* pLevelList, int listCount, const char* pMapName, const char* pLandmarkName, edict_t* pentLandmark);
#endif
	static bool InTransitionVolume(CBaseEntity* pEntity, char* pVolumeName);

	char m_szMapName[kMapNameMost];	   // trigger_changelevel only:  next map
	char m_szLandmarkName[kMapNameMost]; // trigger_changelevel only:  landmark on next map
	int m_changeTarget;
	float m_changeTargetDelay;
};
LINK_ENTITY_TO_CLASS(trigger_changelevel, CChangeLevel);

// Global Savedata for changelevel trigger
#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CChangeLevel)
	DEFINE_ARRAY(CChangeLevel, m_szMapName, FIELD_CHARACTER, kMapNameMost),
	DEFINE_ARRAY(CChangeLevel, m_szLandmarkName, FIELD_CHARACTER, kMapNameMost),
	DEFINE_FIELD(CChangeLevel, m_changeTarget, FIELD_STRING),
	DEFINE_FIELD(CChangeLevel, m_changeTargetDelay, FIELD_FLOAT),
END_SAVERESTORE(CChangeLevel, CBaseTrigger)
#endif

//
// Cache user-entity-field values until spawn is called.
//

bool CChangeLevel::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "map"))
	{
		if (strlen(pkvd->szValue) >= kMapNameMost)
			ALERT(at_error, "Map name '%s' too long (32 chars)\n", pkvd->szValue);
		strcpy(m_szMapName, pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "landmark"))
	{
		if (strlen(pkvd->szValue) >= kMapNameMost)
			ALERT(at_error, "Landmark name '%s' too long (32 chars)\n", pkvd->szValue);
		strcpy(m_szLandmarkName, pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "changetarget"))
	{
		m_changeTarget = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "changedelay"))
	{
		m_changeTargetDelay = atof(pkvd->szValue);
		return true;
	}

	return CBaseTrigger::KeyValue(pkvd);
}


bool CChangeLevel::Spawn()
{
	if (FStrEq(m_szMapName, ""))
	{
		ALERT(at_console, "a trigger_changelevel doesn't have a map");
		return false;
	}

	if (FStrEq(m_szLandmarkName, ""))
	{
		ALERT(at_console, "trigger_changelevel to %s doesn't have a landmark", m_szMapName);
		return false;
	}

	if (0 == stricmp(m_szMapName, STRING(gpGlobals->mapname)))
	{
		ALERT(at_error, "trigger_changelevel points to the current map (%s), which does not work\n", STRING(gpGlobals->mapname));
		return false;
	}

	if (!FStringNull(pev->targetname))
	{
		SetUse(&CChangeLevel::UseChangeLevel);
	}
	InitTrigger();
	if ((pev->spawnflags & SF_CHANGELEVEL_USEONLY) == 0)
		SetTouch(&CChangeLevel::TouchChangeLevel);
	//	ALERT( at_console, "TRANSITION: %s (%s)\n", m_szMapName, m_szLandmarkName );

	return true;
}


edict_t* CChangeLevel::FindLandmark(const char* pLandmarkName)
{
	edict_t* pentLandmark;

	pentLandmark = FIND_ENTITY_BY_STRING(NULL, "targetname", pLandmarkName);
	while (!FNullEnt(pentLandmark))
	{
		// Found the landmark
		if (FClassnameIs(pentLandmark, "info_landmark"))
			return pentLandmark;
		else
			pentLandmark = FIND_ENTITY_BY_STRING(pentLandmark, "targetname", pLandmarkName);
	}
	ALERT(at_error, "Can't find landmark %s\n", pLandmarkName);
	return NULL;
}


//=========================================================
// CChangeLevel:: Use - allows level transitions to be
// triggered by buttons, etc.
//
//=========================================================
void CChangeLevel::UseChangeLevel(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	ChangeLevelNow(pActivator);
}

void CChangeLevel::ChangeLevelNow(CBaseEntity* pActivator)
{
	edict_t* pentLandmark;
	LEVELLIST levels[16];

	// Don't work in deathmatch
	if (util::IsDeathmatch())
		return;

	// Some people are firing these multiple times in a frame, disable
	if (gpGlobals->time == pev->dmgtime)
		return;

	pev->dmgtime = gpGlobals->time;


	CBaseEntity* pPlayer = util::GetLocalPlayer();
	if (!InTransitionVolume(pPlayer, m_szLandmarkName))
	{
		ALERT(at_aiconsole, "Player isn't in the transition volume %s, aborting\n", m_szLandmarkName);
		return;
	}

	// Create an entity to fire the changetarget
	if (!FStringNull(m_changeTarget))
	{
		CFireAndDie* pFireAndDie = GetClassPtr((CFireAndDie*)NULL);
		if (pFireAndDie)
		{
			// Set target and delay
			pFireAndDie->pev->target = m_changeTarget;
			pFireAndDie->m_flDelay = m_changeTargetDelay;
			pFireAndDie->pev->origin = pPlayer->pev->origin;
			// Call spawn
			DispatchSpawn(pFireAndDie->edict());
		}
	}
	// This object will get removed in the call to CHANGE_LEVEL, copy the params into "safe" memory
	strcpy(st_szNextMap, m_szMapName);

	m_hActivator = pActivator;
	UseTargets(pActivator, USE_TOGGLE, 0);
	st_szNextSpot[0] = 0; // Init landmark to NULL

	// look for a landmark entity
	pentLandmark = FindLandmark(m_szLandmarkName);
	if (!FNullEnt(pentLandmark))
	{
		strcpy(st_szNextSpot, m_szLandmarkName);
		gpGlobals->vecLandmarkOffset = VARS(pentLandmark)->origin;
	}
	//	ALERT( at_console, "Level touches %d levels\n", ChangeList( levels, 16 ) );
	ALERT(at_console, "CHANGE LEVEL: %s %s\n", st_szNextMap, st_szNextSpot);
	CHANGE_LEVEL(st_szNextMap, st_szNextSpot);
}

//
// GLOBALS ASSUMED SET:  st_szNextMap
//
void CChangeLevel::TouchChangeLevel(CBaseEntity* pOther)
{
	if (!FClassnameIs(pOther->pev, "player"))
		return;

	ChangeLevelNow(pOther);
}


bool CChangeLevel::InTransitionVolume(CBaseEntity* pEntity, char* pVolumeName)
{
	edict_t* pentVolume;


	if ((pEntity->ObjectCaps() & FCAP_FORCE_TRANSITION) != 0)
		return true;

	// If you're following another entity, follow it through the transition (weapons follow the player)
	if (pEntity->pev->movetype == MOVETYPE_FOLLOW)
	{
		if (pEntity->pev->aiment != NULL)
			pEntity = CBaseEntity::Instance(pEntity->pev->aiment);
	}

	bool inVolume = true; // Unless we find a trigger_transition, everything is in the volume

	pentVolume = FIND_ENTITY_BY_TARGETNAME(NULL, pVolumeName);
	while (!FNullEnt(pentVolume))
	{
		CBaseEntity* pVolume = CBaseEntity::Instance(pentVolume);

		if (pVolume && FClassnameIs(pVolume->pev, "trigger_transition"))
		{
			if (pVolume->Intersects(pEntity)) // It touches one, it's in the volume
				return true;
			else
				inVolume = false; // Found a trigger_transition, but I don't intersect it -- if I don't find another, don't go!
		}
		pentVolume = FIND_ENTITY_BY_TARGETNAME(pentVolume, pVolumeName);
	}

	return inVolume;
}


#ifdef HALFLIFE_SAVERESTORE
// Add a transition to the list, but ignore duplicates
// (a designer may have placed multiple trigger_changelevels with the same landmark)
bool CChangeLevel::AddTransitionToList(LEVELLIST* pLevelList, int listCount, const char* pMapName, const char* pLandmarkName, edict_t* pentLandmark)
{
	int i;

	if (!pLevelList || !pMapName || !pLandmarkName || !pentLandmark)
		return false;

	for (i = 0; i < listCount; i++)
	{
		if (pLevelList[i].pentLandmark == pentLandmark && strcmp(pLevelList[i].mapName, pMapName) == 0)
			return false;
	}
	strcpy(pLevelList[listCount].mapName, pMapName);
	strcpy(pLevelList[listCount].landmarkName, pLandmarkName);
	pLevelList[listCount].pentLandmark = pentLandmark;
	pLevelList[listCount].vecLandmarkOrigin = VARS(pentLandmark)->origin;

	return true;
}

int util::BuildChangeList(LEVELLIST* pLevelList, int maxList)
{
	return CChangeLevel::ChangeList(pLevelList, maxList);
}

// We can only ever move 512 entities across a transition
#define MAX_ENTITY 512

// This has grown into a complicated beast
// Can we make this more elegant?
// This builds the list of all transitions on this level and which entities are in their PVS's and can / should
// be moved across.
int CChangeLevel::ChangeList(LEVELLIST* pLevelList, int maxList)
{
	edict_t *pentChangelevel, *pentLandmark;
	int i, count;

	count = 0;

	// Find all of the possible level changes on this BSP
	pentChangelevel = FIND_ENTITY_BY_STRING(NULL, "classname", "trigger_changelevel");
	if (FNullEnt(pentChangelevel))
		return 0;
	while (!FNullEnt(pentChangelevel))
	{
		CChangeLevel* pTrigger;

		pTrigger = GetClassPtr((CChangeLevel*)VARS(pentChangelevel));
		if (pTrigger)
		{
			// Find the corresponding landmark
			pentLandmark = FindLandmark(pTrigger->m_szLandmarkName);
			if (pentLandmark)
			{
				// Build a list of unique transitions
				if (AddTransitionToList(pLevelList, count, pTrigger->m_szMapName, pTrigger->m_szLandmarkName, pentLandmark))
				{
					count++;
					if (count >= maxList) // FULL!!
						break;
				}
			}
		}
		pentChangelevel = FIND_ENTITY_BY_STRING(pentChangelevel, "classname", "trigger_changelevel");
	}

	//Token table is null at this point, so don't use CSaveRestoreBuffer::IsValidSaveRestoreData here.
	if (auto pSaveData = reinterpret_cast<SAVERESTOREDATA*>(gpGlobals->pSaveData);
		nullptr != pSaveData && pSaveData->pTable)
	{
		CSave saveHelper(*pSaveData);

		for (i = 0; i < count; i++)
		{
			int j, entityCount = 0;
			CBaseEntity* pEntList[MAX_ENTITY];
			int entityFlags[MAX_ENTITY];

			// Follow the linked list of entities in the PVS of the transition landmark
			edict_t* pent = util::EntitiesInPVS(pLevelList[i].pentLandmark);

			// Build a list of valid entities in this linked list (we're going to use pent->v.chain again)
			while (!FNullEnt(pent))
			{
				CBaseEntity* pEntity = CBaseEntity::Instance(pent);
				if (pEntity)
				{
					//					ALERT( at_console, "Trying %s\n", STRING(pEntity->pev->classname) );
					int caps = pEntity->ObjectCaps();
					if ((caps & FCAP_DONT_SAVE) == 0)
					{
						int flags = 0;

						// If this entity can be moved or is global, mark it
						if ((caps & FCAP_ACROSS_TRANSITION) != 0)
							flags |= FENTTABLE_MOVEABLE;
						if (!FStringNull(pEntity->pev->globalname) && !pEntity->IsDormant())
							flags |= FENTTABLE_GLOBAL;
						if (0 != flags)
						{
							pEntList[entityCount] = pEntity;
							entityFlags[entityCount] = flags;
							entityCount++;
							if (entityCount > MAX_ENTITY)
								ALERT(at_error, "Too many entities across a transition!");
						}
						//						else
						//							ALERT( at_console, "Failed %s\n", STRING(pEntity->pev->classname) );
					}
					//					else
					//						ALERT( at_console, "DON'T SAVE %s\n", STRING(pEntity->pev->classname) );
				}
				pent = pent->v.chain;
			}

			for (j = 0; j < entityCount; j++)
			{
				// Check to make sure the entity isn't screened out by a trigger_transition
				if (0 != entityFlags[j] && InTransitionVolume(pEntList[j], pLevelList[i].landmarkName))
				{
					// Mark entity table with 1<<i
					int index = saveHelper.EntityIndex(pEntList[j]);
					// Flag it with the level number
					saveHelper.EntityFlagsSet(index, entityFlags[j] | (1 << i));
				}
				//				else
				//					ALERT( at_console, "Screened out %s\n", STRING(pEntList[j]->pev->classname) );
			}
		}
	}

	return count;
}
#endif


// ========================== A TRIGGER THAT PUSHES YOU ===============================

class CTriggerPush : public CBaseTrigger
{
public:
	bool Spawn() override;
	void Touch(CBaseEntity* pOther) override;
};
LINK_ENTITY_TO_CLASS(trigger_push, CTriggerPush);


bool CTriggerPush::Spawn()
{
	if (pev->angles == g_vecZero)
		pev->angles.y = 360;
	InitTrigger();

	if (pev->speed == 0)
		pev->speed = 100;

	if (FBitSet(pev->spawnflags, SF_TRIGGER_PUSH_START_OFF)) // if flagged to Start Turned Off, make trigger nonsolid.
		pev->solid = SOLID_NOT;

	SetUse(&CTriggerPush::ToggleUse);

	SetOrigin(pev->origin); // Link into the list

	return true;
}


void CTriggerPush::Touch(CBaseEntity* pOther)
{
	entvars_t* pevToucher = pOther->pev;

	switch (pevToucher->movetype)
	{
	case MOVETYPE_NONE:
	case MOVETYPE_PUSH:
	case MOVETYPE_NOCLIP:
	case MOVETYPE_FOLLOW:
		return;
	}

	if (pevToucher->solid != SOLID_NOT && pevToucher->solid != SOLID_BSP)
	{
		// Instant trigger, just transfer velocity and remove
		if (FBitSet(pev->spawnflags, SF_TRIG_PUSH_ONCE))
		{
			pevToucher->velocity = pevToucher->velocity + (pev->speed * pev->movedir);
			if (pevToucher->velocity.z > 0)
				pevToucher->flags &= ~FL_ONGROUND;
			Remove();
		}
		else
		{ // Push field, transfer to base velocity
			Vector vecPush = (pev->speed * pev->movedir);
			if ((pevToucher->flags & FL_BASEVELOCITY) != 0)
				vecPush = vecPush + pevToucher->basevelocity;

			pevToucher->basevelocity = vecPush;

			pevToucher->flags |= FL_BASEVELOCITY;
		}
	}
}


//======================================
// teleport trigger
//
//

void CBaseTrigger::TeleportTouch(CBaseEntity* pOther)
{
	entvars_t* pevToucher = pOther->pev;
	edict_t* pentTarget = NULL;

	// Only teleport monsters or clients
	if (!FBitSet(pevToucher->flags, FL_CLIENT | FL_MONSTER))
		return;

	if (!util::IsMasterTriggered(m_sMaster, pOther))
		return;

	if ((pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS) == 0)
	{ // no monsters allowed!
		if (FBitSet(pevToucher->flags, FL_MONSTER))
		{
			return;
		}
	}

	if ((pev->spawnflags & SF_TRIGGER_NOCLIENTS) != 0)
	{ // no clients allowed
		if (pOther->IsPlayer())
		{
			return;
		}
	}

	pentTarget = FIND_ENTITY_BY_TARGETNAME(pentTarget, STRING(pev->target));
	if (FNullEnt(pentTarget))
		return;

	Vector tmp = VARS(pentTarget)->origin;

	if (pOther->IsPlayer())
	{
		tmp.z -= pOther->pev->mins.z; // make origin adjustments in case the teleportee is a player. (origin in center, not at feet)
	}

	tmp.z++;

	pevToucher->flags &= ~FL_ONGROUND;

	pOther->SetOrigin(tmp);

	pevToucher->angles = pentTarget->v.angles;

	if (pOther->IsPlayer())
	{
		pevToucher->v_angle = pentTarget->v.angles;
	}

	pevToucher->fixangle = 1;
	pevToucher->velocity = pevToucher->basevelocity = g_vecZero;
}


class CTriggerTeleport : public CBaseTrigger
{
public:
	bool Spawn() override;
};
LINK_ENTITY_TO_CLASS(trigger_teleport, CTriggerTeleport);

bool CTriggerTeleport::Spawn()
{
	InitTrigger();

	SetTouch(&CTriggerTeleport::TeleportTouch);

	return true;
}


LINK_ENTITY_TO_CLASS(info_teleport_destination, CPointEntity);


#ifdef HALFLIFE_SAVERESTORE
class CTriggerSave : public CBaseTrigger
{
public:
	bool Spawn() override;
	void EXPORT SaveTouch(CBaseEntity* pOther);
};
LINK_ENTITY_TO_CLASS(trigger_autosave, CTriggerSave);

bool CTriggerSave::Spawn()
{
	if (util::IsDeathmatch())
	{
		return false;
	}

	InitTrigger();
	SetTouch(&CTriggerSave::SaveTouch);

	return true;
}

void CTriggerSave::SaveTouch(CBaseEntity* pOther)
{
	if (!util::IsMasterTriggered(m_sMaster, pOther))
		return;

	// Only save on clients
	if (!pOther->IsPlayer())
		return;

	SetTouch(NULL);
	Remove();
	SERVER_COMMAND("autosave\n");
}


class CRevertSaved : public CPointEntity
{
public:
	DECLARE_SAVERESTORE()

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void EXPORT MessageThink();
	void EXPORT LoadThink();
	bool KeyValue(KeyValueData* pkvd) override;

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

IMPLEMENT_SAVERESTORE(CRevertSaved)
	DEFINE_FIELD(CRevertSaved, m_messageTime, FIELD_FLOAT),
	DEFINE_FIELD(CRevertSaved, m_loadTime, FIELD_FLOAT),
END_SAVERESTORE(CRevertSaved, CPointEntity)

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
#endif


#define SF_ENDSECTION_USEONLY 0x0001

class CTriggerEndSection : public CBaseTrigger
{
public:
	bool Spawn() override;
	void EXPORT EndSectionTouch(CBaseEntity* pOther);
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT EndSectionUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
};
LINK_ENTITY_TO_CLASS(trigger_endsection, CTriggerEndSection);


void CTriggerEndSection::EndSectionUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	// Only save on clients
	if (pActivator && !pActivator->IsNetClient())
		return;

	SetUse(NULL);

	if (!FStringNull(pev->message))
	{
		g_engfuncs.pfnEndSection(STRING(pev->message));
	}
	Remove();
}

bool CTriggerEndSection::Spawn()
{
	if (util::IsDeathmatch())
	{
		return false;
	}

	InitTrigger();

	SetUse(&CTriggerEndSection::EndSectionUse);
	// If it is a "use only" trigger, then don't set the touch function.
	if ((pev->spawnflags & SF_ENDSECTION_USEONLY) == 0)
		SetTouch(&CTriggerEndSection::EndSectionTouch);

	return true;
}

void CTriggerEndSection::EndSectionTouch(CBaseEntity* pOther)
{
	// Only save on clients
	if (!pOther->IsNetClient())
		return;

	SetTouch(NULL);

	if (!FStringNull(pev->message))
	{
		g_engfuncs.pfnEndSection(STRING(pev->message));
	}
	Remove();
}

bool CTriggerEndSection::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "section"))
	{
		//		m_iszSectionName = ALLOC_STRING( pkvd->szValue );
		// Store this in message so we don't have to write save/restore for this ent
		pev->message = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CBaseTrigger::KeyValue(pkvd);
}


class CTriggerGravity : public CBaseTrigger
{
public:
	bool Spawn() override;
	void EXPORT GravityTouch(CBaseEntity* pOther);
};
LINK_ENTITY_TO_CLASS(trigger_gravity, CTriggerGravity);

bool CTriggerGravity::Spawn()
{
	InitTrigger();
	SetTouch(&CTriggerGravity::GravityTouch);
	return true;
}

void CTriggerGravity::GravityTouch(CBaseEntity* pOther)
{
	// Only save on clients
	if (!pOther->IsPlayer())
		return;

	pOther->pev->gravity = pev->gravity;
}







// this is a really bad idea.
class CTriggerChangeTarget : public CBaseEntity
{
public:
	DECLARE_SAVERESTORE()

	bool KeyValue(KeyValueData* pkvd) override;
	bool Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

private:
	int m_iszNewTarget;
};
LINK_ENTITY_TO_CLASS(trigger_changetarget, CTriggerChangeTarget);

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CTriggerChangeTarget)
	DEFINE_FIELD(CTriggerChangeTarget, m_iszNewTarget, FIELD_STRING),
END_SAVERESTORE(CTriggerChangeTarget, CBaseDelay)
#endif

bool CTriggerChangeTarget::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_iszNewTarget"))
	{
		m_iszNewTarget = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

bool CTriggerChangeTarget::Spawn()
{
	return true;
}


void CTriggerChangeTarget::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	CBaseEntity* pTarget = util::FindEntityByString(NULL, "targetname", STRING(pev->target));

	if (pTarget)
	{
		pTarget->pev->target = m_iszNewTarget;
	}
}
