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

===== buttons.cpp ========================================================

  button-related code

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "doors.h"

#define SF_BUTTON_DONTMOVE 1
#define SF_ROTBUTTON_NOTSOLID 1
#define SF_BUTTON_TOGGLE 32		  // button stays pushed until reactivated
#define SF_BUTTON_SPARK_IF_OFF 64 // button sparks in OFF state
#define SF_BUTTON_TOUCH_ONLY 256  // button only fires as a result of USE key.

#define SF_GLOBAL_SET 1 // Set global state to initial state on spawn

class CEnvGlobal : public CPointEntity
{
public:
	CEnvGlobal(Entity* containingEntity) : CPointEntity(containingEntity) {}

	DECLARE_SAVERESTORE()

	bool Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	string_t m_globalstate;
	int m_triggermode;
	int m_initialstate;
};

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CEnvGlobal)
	DEFINE_FIELD(CEnvGlobal, m_globalstate, FIELD_STRING),
	DEFINE_FIELD(CEnvGlobal, m_triggermode, FIELD_INTEGER),
	DEFINE_FIELD(CEnvGlobal, m_initialstate, FIELD_INTEGER),
END_SAVERESTORE(CEnvGlobal, CBaseEntity)
#endif

LINK_ENTITY_TO_CLASS(env_global, CEnvGlobal);

bool CEnvGlobal::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "globalstate")) // State name
	{
		m_globalstate = engine::AllocString(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "triggermode"))
	{
		m_triggermode = atoi(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "initialstate"))
	{
		m_initialstate = atoi(pkvd->szValue);
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

bool CEnvGlobal::Spawn()
{
	if (FStringNull(m_globalstate))
	{
		return false;
	}
	if (FBitSet(v.spawnflags, SF_GLOBAL_SET))
	{
		if (!gGlobalState.EntityInTable(m_globalstate))
			gGlobalState.EntityAdd(m_globalstate, gpGlobals->mapname, (GLOBALESTATE)m_initialstate);
	}

	return true;
}


void CEnvGlobal::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	GLOBALESTATE oldState = gGlobalState.EntityGetState(m_globalstate);
	GLOBALESTATE newState;

	switch (m_triggermode)
	{
	case 0:
		newState = GLOBAL_OFF;
		break;

	case 1:
		newState = GLOBAL_ON;
		break;

	case 2:
		newState = GLOBAL_DEAD;
		break;

	default:
	case 3:
		if (oldState == GLOBAL_ON)
			newState = GLOBAL_OFF;
		else if (oldState == GLOBAL_OFF)
			newState = GLOBAL_ON;
		else
			newState = oldState;
	}

	if (gGlobalState.EntityInTable(m_globalstate))
		gGlobalState.EntitySetState(m_globalstate, newState);
	else
		gGlobalState.EntityAdd(m_globalstate, gpGlobals->mapname, newState);
}



#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CMultiSource)
	DEFINE_ARRAY(CMultiSource, m_rgEntities, FIELD_EHANDLE, MS_MAX_TARGETS),
	DEFINE_ARRAY(CMultiSource, m_rgTriggered, FIELD_INTEGER, MS_MAX_TARGETS),
	DEFINE_FIELD(CMultiSource, m_iTotal, FIELD_INTEGER),
	DEFINE_FIELD(CMultiSource, m_globalstate, FIELD_STRING),
END_SAVERESTORE(CMultiSource, CBaseEntity)
#endif

LINK_ENTITY_TO_CLASS(multisource, CMultiSource);
//
// Cache user-entity-field values until spawn is called.
//

bool CMultiSource::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "style") ||
		streq(pkvd->szKeyName, "height") ||
		streq(pkvd->szKeyName, "killtarget") ||
		streq(pkvd->szKeyName, "value1") ||
		streq(pkvd->szKeyName, "value2") ||
		streq(pkvd->szKeyName, "value3"))
		return true;
	else if (streq(pkvd->szKeyName, "globalstate"))
	{
		m_globalstate = engine::AllocString(pkvd->szValue);
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

#define SF_MULTI_INIT 1

bool CMultiSource::Spawn()
{
	// set up think for later registration

	v.solid = SOLID_NOT;
	v.movetype = MOVETYPE_NONE;
	v.nextthink = gpGlobals->time + 0.1;
	v.spawnflags |= SF_MULTI_INIT; // Until it's initialized
	SetThink(&CMultiSource::Register);

	return true;
}

void CMultiSource::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	int i = 0;

	// Find the entity in our list
	while (i < m_iTotal)
		if (m_rgEntities[i++] == pCaller)
			break;

	// if we didn't find it, report error and leave
	if (i > m_iTotal)
	{
		engine::AlertMessage(at_console, "MultiSrc:Used by non member %s.\n", STRING(pCaller->v.classname));
		return;
	}

	// CONSIDER: a Use input to the multisource always toggles.  Could check useType for ON/OFF/TOGGLE

	m_rgTriggered[i - 1] ^= 1;

	//
	if (IsTriggered(pActivator))
	{
		engine::AlertMessage(at_aiconsole, "Multisource %s enabled (%d inputs)\n", STRING(v.targetname), m_iTotal);
		USE_TYPE useType = USE_TOGGLE;
		if (!FStringNull(m_globalstate))
			useType = USE_ON;
		UseTargets(nullptr, useType, 0);
	}
}


bool CMultiSource::IsTriggered(CBaseEntity*)
{
	// Is everything triggered?
	int i = 0;

	// Still initializing?
	if ((v.spawnflags & SF_MULTI_INIT) != 0)
		return false;

	while (i < m_iTotal)
	{
		if (m_rgTriggered[i] == 0)
			break;
		i++;
	}

	if (i == m_iTotal)
	{
		if (FStringNull(m_globalstate) || gGlobalState.EntityGetState(m_globalstate) == GLOBAL_ON)
			return true;
	}

	return false;
}

void CMultiSource::Register()
{
	CBaseEntity* pentTarget = nullptr;

	m_iTotal = 0;
	memset(m_rgEntities, 0, MS_MAX_TARGETS * sizeof(EHANDLE));

	ClearThink();

	// search for all entities which target this multisource (v.targetname)

	while ((pentTarget = util::FindEntityByString(pentTarget, "targetname", STRING(v.targetname))) != nullptr && m_iTotal < MS_MAX_TARGETS)
	{
		m_rgEntities[m_iTotal++] = pentTarget;
	}

	pentTarget = nullptr;

	while ((pentTarget = util::FindEntityByClassname(pentTarget, "multi_manager")) != nullptr && m_iTotal < MS_MAX_TARGETS)
	{
		if (pentTarget->HasTarget(v.targetname))
		{
			m_rgEntities[m_iTotal++] = pentTarget;
		}
	}

	v.spawnflags &= ~SF_MULTI_INIT;
}

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CBaseButton)
	DEFINE_FIELD(CBaseButton, m_fStayPushed, FIELD_BOOLEAN),
	DEFINE_FIELD(CBaseButton, m_fRotating, FIELD_BOOLEAN),

	DEFINE_FIELD(CBaseButton, m_sounds, FIELD_INTEGER),
	DEFINE_FIELD(CBaseButton, m_bLockedSound, FIELD_CHARACTER),
	DEFINE_FIELD(CBaseButton, m_bLockedSentence, FIELD_CHARACTER),
	DEFINE_FIELD(CBaseButton, m_bUnlockedSound, FIELD_CHARACTER),
	DEFINE_FIELD(CBaseButton, m_bUnlockedSentence, FIELD_CHARACTER),
	DEFINE_FIELD(CBaseButton, m_strChangeTarget, FIELD_STRING),
END_SAVERESTORE(CBaseButton, CBaseToggle)
#endif

void CBaseButton::Precache()
{
	const char* pszSound;

	if (FBitSet(v.spawnflags, SF_BUTTON_SPARK_IF_OFF)) // this button should spark in OFF state
	{
		engine::PrecacheSound("buttons/spark1.wav");
		engine::PrecacheSound("buttons/spark2.wav");
		engine::PrecacheSound("buttons/spark3.wav");
		engine::PrecacheSound("buttons/spark4.wav");
		engine::PrecacheSound("buttons/spark5.wav");
		engine::PrecacheSound("buttons/spark6.wav");
	}

	// get door button sounds, for doors which require buttons to open

	if (0 != m_bLockedSound)
	{
		pszSound = ButtonSound((int)m_bLockedSound);
		engine::PrecacheSound(pszSound);
		m_ls.sLockedSound = engine::AllocString(pszSound);
	}

	if (0 != m_bUnlockedSound)
	{
		pszSound = ButtonSound((int)m_bUnlockedSound);
		engine::PrecacheSound(pszSound);
		m_ls.sUnlockedSound = engine::AllocString(pszSound);
	}

	// get sentence group names, for doors which are directly 'touched' to open

	switch (m_bLockedSentence)
	{
	case 1:
		m_ls.sLockedSentence = MAKE_STRING("NA");
		break; // access denied
	case 2:
		m_ls.sLockedSentence = MAKE_STRING("ND");
		break; // security lockout
	case 3:
		m_ls.sLockedSentence = MAKE_STRING("NF");
		break; // blast door
	case 4:
		m_ls.sLockedSentence = MAKE_STRING("NFIRE");
		break; // fire door
	case 5:
		m_ls.sLockedSentence = MAKE_STRING("NCHEM");
		break; // chemical door
	case 6:
		m_ls.sLockedSentence = MAKE_STRING("NRAD");
		break; // radiation door
	case 7:
		m_ls.sLockedSentence = MAKE_STRING("NCON");
		break; // gen containment
	case 8:
		m_ls.sLockedSentence = MAKE_STRING("NH");
		break; // maintenance door
	case 9:
		m_ls.sLockedSentence = MAKE_STRING("NG");
		break; // broken door

	default:
		m_ls.sLockedSentence = 0;
		break;
	}

	switch (m_bUnlockedSentence)
	{
	case 1:
		m_ls.sUnlockedSentence = MAKE_STRING("EA");
		break; // access granted
	case 2:
		m_ls.sUnlockedSentence = MAKE_STRING("ED");
		break; // security door
	case 3:
		m_ls.sUnlockedSentence = MAKE_STRING("EF");
		break; // blast door
	case 4:
		m_ls.sUnlockedSentence = MAKE_STRING("EFIRE");
		break; // fire door
	case 5:
		m_ls.sUnlockedSentence = MAKE_STRING("ECHEM");
		break; // chemical door
	case 6:
		m_ls.sUnlockedSentence = MAKE_STRING("ERAD");
		break; // radiation door
	case 7:
		m_ls.sUnlockedSentence = MAKE_STRING("ECON");
		break; // gen containment
	case 8:
		m_ls.sUnlockedSentence = MAKE_STRING("EH");
		break; // maintenance door

	default:
		m_ls.sUnlockedSentence = 0;
		break;
	}
}

//
// Cache user-entity-field values until spawn is called.
//

bool CBaseButton::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "changetarget"))
	{
		m_strChangeTarget = engine::AllocString(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "locked_sound"))
	{
		m_bLockedSound = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "locked_sentence"))
	{
		m_bLockedSentence = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "unlocked_sound"))
	{
		m_bUnlockedSound = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "unlocked_sentence"))
	{
		m_bUnlockedSentence = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "sounds"))
	{
		m_sounds = atoi(pkvd->szValue);
		return true;
	}

	return CBaseToggle::KeyValue(pkvd);
}

//
// ButtonShot
//
bool CBaseButton::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	BUTTON_CODE code = ButtonResponseToTouch();

	if (code == BUTTON_NOTHING)
		return false;
	// Temporarily disable the touch function, until movement is finished.
	ClearTouch();

	m_hActivator = attacker;
	if (m_hActivator == nullptr)
		return false;

	if (!AttemptToActivate(attacker))
	{
		return false;
	}

	if (code == BUTTON_RETURN)
	{
		EmitSound(STRING(v.noise), CHAN_VOICE);

		// Toggle buttons fire when they get back to their "home" position
		if ((v.spawnflags & SF_BUTTON_TOGGLE) == 0)
			UseTargets(m_hActivator, USE_TOGGLE, 0);
		ButtonReturn();
	}
	else // code == BUTTON_ACTIVATE
		ButtonActivate();

	return false;
}

LINK_ENTITY_TO_CLASS(func_button, CBaseButton);


bool CBaseButton::Spawn()
{
	const char* pszSound;

	//----------------------------------------------------
	//determine sounds for buttons
	//a sound of 0 should not make a sound
	//----------------------------------------------------
	pszSound = ButtonSound(m_sounds);
	engine::PrecacheSound(pszSound);
	v.noise = engine::AllocString(pszSound);

	Precache();

	if (FBitSet(v.spawnflags, SF_BUTTON_SPARK_IF_OFF)) // this button should spark in OFF state
	{
		SetThink(&CBaseButton::ButtonSpark);
		v.nextthink = gpGlobals->time + 0.5; // no hurry, make sure everything else spawns
	}

	v.movedir = util::SetMovedir(v.angles);

	v.movetype = MOVETYPE_PUSH;
	v.solid = SOLID_BSP;
	SetModel(v.model);

	if (v.speed == 0)
		v.speed = 40;

	if (v.health > 0)
	{
		v.takedamage = DAMAGE_YES;
	}

	if (m_flWait == 0)
		m_flWait = 1;
	if (m_flLip == 0)
		m_flLip = 4;

	m_toggle_state = TS_AT_BOTTOM;
	m_vecPosition1 = v.origin;
	// Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big
	m_vecPosition2 = m_vecPosition1 + (v.movedir * (fabs(v.movedir.x * (v.size.x - 2)) + fabs(v.movedir.y * (v.size.y - 2)) + fabs(v.movedir.z * (v.size.z - 2)) - m_flLip));


	// Is this a non-moving button?
	if (((m_vecPosition2 - m_vecPosition1).Length() < 1) || (v.spawnflags & SF_BUTTON_DONTMOVE) != 0)
		m_vecPosition2 = m_vecPosition1;

	m_fStayPushed = (m_flWait == -1 ? true : false);
	m_fRotating = false;

	// if the button is flagged for USE button activation only, take away it's touch function and add a use function

	if (FBitSet(v.spawnflags, SF_BUTTON_TOUCH_ONLY)) // touchable button
	{
		SetTouch(&CBaseButton::ButtonTouch);
	}
	else
	{
		ClearTouch();
		SetUse(&CBaseButton::ButtonUse);
	}

	return true;
}


// Button sound table.
// Also used by CBaseDoor to get 'touched' door lock/unlock sounds

const char* ButtonSound(int sound)
{
	const char* pszSound;

	switch (sound)
	{
	case 0:
		pszSound = "common/null.wav";
		break;
	case 1:
		pszSound = "buttons/button1.wav";
		break;
	case 2:
		pszSound = "buttons/button2.wav";
		break;
	case 3:
		pszSound = "buttons/button3.wav";
		break;
	case 4:
		pszSound = "buttons/button4.wav";
		break;
	case 5:
		pszSound = "buttons/button5.wav";
		break;
	case 6:
		pszSound = "buttons/button6.wav";
		break;
	case 7:
		pszSound = "buttons/button7.wav";
		break;
	case 8:
		pszSound = "buttons/button8.wav";
		break;
	case 9:
		pszSound = "buttons/button9.wav";
		break;
	case 10:
		pszSound = "buttons/button10.wav";
		break;
	case 11:
		pszSound = "buttons/button11.wav";
		break;
	case 12:
		pszSound = "buttons/latchlocked1.wav";
		break;
	case 13:
		pszSound = "buttons/latchunlocked1.wav";
		break;
	case 14:
		pszSound = "buttons/lightswitch2.wav";
		break;

		// next 6 slots reserved for any additional sliding button sounds we may add

	case 21:
		pszSound = "buttons/lever1.wav";
		break;
	case 22:
		pszSound = "buttons/lever2.wav";
		break;
	case 23:
		pszSound = "buttons/lever3.wav";
		break;
	case 24:
		pszSound = "buttons/lever4.wav";
		break;
	case 25:
		pszSound = "buttons/lever5.wav";
		break;

	default:
		pszSound = "buttons/button9.wav";
		break;
	}

	return pszSound;
}

//
// Makes flagged buttons spark when turned off
//

static void DoSpark(CBaseEntity *entity, const Vector& location)
{
	Vector tmp = location + entity->v.size * 0.5;
	tent::Sparks(tmp);

	float flVolume = engine::RandomFloat(0.25, 0.75) * 0.4; //random volume range
	const char *sample;
	switch ((int)(engine::RandomFloat(0, 1) * 6))
	{
	case 0: sample ="buttons/spark1.wav"; break;
	case 1: sample ="buttons/spark2.wav"; break;
	case 2: sample ="buttons/spark3.wav"; break;
	case 3: sample ="buttons/spark4.wav"; break;
	case 4: sample ="buttons/spark5.wav"; break;
	case 5: sample ="buttons/spark6.wav"; break;
	}

	entity->EmitSound(sample, CHAN_VOICE, flVolume);
}

void CBaseButton::ButtonSpark()
{
	SetThink(&CBaseButton::ButtonSpark);
	v.nextthink = v.ltime + (0.1 + engine::RandomFloat(0, 1.5)); // spark again at random interval

	DoSpark(this, v.absmin);
}


//
// Button's Use function
//
void CBaseButton::ButtonUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	// Ignore touches if button is moving, or pushed-in and waiting to auto-come-out.
	if (m_toggle_state == TS_GOING_UP || m_toggle_state == TS_GOING_DOWN)
		return;

	if (!AttemptToActivate(pActivator))
	{
		PlayLockSounds(this, &m_ls, true, true);
		return;
	}

	m_hActivator = pActivator;
	if (m_toggle_state == TS_AT_TOP)
	{
		if (!m_fStayPushed && FBitSet(v.spawnflags, SF_BUTTON_TOGGLE))
		{
			EmitSound(STRING(v.noise), CHAN_VOICE);

			ButtonReturn();
		}
	}
	else
		ButtonActivate();
}


CBaseButton::BUTTON_CODE CBaseButton::ButtonResponseToTouch()
{
	// Ignore touches if button is moving, or pushed-in and waiting to auto-come-out.
	if (m_toggle_state == TS_GOING_UP ||
		m_toggle_state == TS_GOING_DOWN ||
		(m_toggle_state == TS_AT_TOP && !m_fStayPushed && !FBitSet(v.spawnflags, SF_BUTTON_TOGGLE)))
		return BUTTON_NOTHING;

	if (m_toggle_state == TS_AT_TOP)
	{
		if ((FBitSet(v.spawnflags, SF_BUTTON_TOGGLE)) && !m_fStayPushed)
		{
			return BUTTON_RETURN;
		}
	}
	else
		return BUTTON_ACTIVATE;

	return BUTTON_NOTHING;
}


//
// Touching a button simply "activates" it.
//
void CBaseButton::ButtonTouch(CBaseEntity* pOther)
{
	// Ignore touches by anything but players
	if (!pOther->IsPlayer())
		return;

	m_hActivator = pOther;

	BUTTON_CODE code = ButtonResponseToTouch();

	if (code == BUTTON_NOTHING)
		return;

	if (!util::IsMasterTriggered(m_sMaster, pOther))
	{
		// play button locked sound
		PlayLockSounds(this, &m_ls, true, true);
		return;
	}

	if (!AttemptToActivate(pOther))
	{
		PlayLockSounds(this, &m_ls, true, true);
		return;
	}

	// Temporarily disable the touch function, until movement is finished.
	ClearTouch();

	if (code == BUTTON_RETURN)
	{
		EmitSound(STRING(v.noise), CHAN_VOICE);
		UseTargets(m_hActivator, USE_TOGGLE, 0);
		ButtonReturn();
	}
	else // code == BUTTON_ACTIVATE
		ButtonActivate();
}

//
// Starts the button moving "in/up".
//
void CBaseButton::ButtonActivate()
{
	EmitSound(STRING(v.noise), CHAN_VOICE);

	if (!util::IsMasterTriggered(m_sMaster, m_hActivator))
	{
		// button is locked, play locked sound
		PlayLockSounds(this, &m_ls, true, true);
		return;
	}
	else
	{
		// button is unlocked, play unlocked sound
		PlayLockSounds(this, &m_ls, false, true);
	}

	m_toggle_state = TS_GOING_UP;

	SetMoveDone(&CBaseButton::TriggerAndWait);
	if (!m_fRotating)
		LinearMove(m_vecPosition2, v.speed);
	else
		AngularMove(m_vecAngle2, v.speed);
}

//
// Button has reached the "in/up" position.  Activate its "targets", and pause before "popping out".
//
void CBaseButton::TriggerAndWait()
{
	if (!util::IsMasterTriggered(m_sMaster, m_hActivator))
		return;

	m_toggle_state = TS_AT_TOP;

	// If button automatically comes back out, start it moving out.
	// Else re-instate touch method
	if (m_fStayPushed || FBitSet(v.spawnflags, SF_BUTTON_TOGGLE))
	{
		if (!FBitSet(v.spawnflags, SF_BUTTON_TOUCH_ONLY)) // this button only works if USED, not touched!
		{
			// ALL buttons are now use only
			ClearTouch();
		}
		else
			SetTouch(&CBaseButton::ButtonTouch);
	}
	else
	{
		v.nextthink = v.ltime + m_flWait;
		SetThink(&CBaseButton::ButtonReturn);
	}

	v.frame = 1; // use alternate textures


	UseTargets(m_hActivator, USE_TOGGLE, 0);
}


//
// Starts the button moving "out/down".
//
void CBaseButton::ButtonReturn()
{
	m_toggle_state = TS_GOING_DOWN;

	SetMoveDone(&CBaseButton::ButtonBackHome);
	if (!m_fRotating)
		LinearMove(m_vecPosition1, v.speed);
	else
		AngularMove(m_vecAngle1, v.speed);

	v.frame = 0; // use normal textures
}


//
// Button has returned to start state.  Quiesce it.
//
void CBaseButton::ButtonBackHome()
{
	m_toggle_state = TS_AT_BOTTOM;

	if (FBitSet(v.spawnflags, SF_BUTTON_TOGGLE))
	{
		UseTargets(m_hActivator, USE_TOGGLE, 0);
	}


	if (!FStringNull(v.target))
	{
		CBaseEntity* pentTarget = nullptr;
		for (;;)
		{
			pentTarget = util::FindEntityByTargetname(pentTarget, STRING(v.target));

			if (pentTarget == nullptr)
				break;

			if (!pentTarget->Is(Type::MultiSource))
				continue;

			pentTarget->Use(m_hActivator, this, USE_TOGGLE, 0);
		}
	}

	// Re-instate touch method, movement cycle is complete.
	if (!FBitSet(v.spawnflags, SF_BUTTON_TOUCH_ONLY)) // this button only works if USED, not touched!
	{
		// All buttons are now use only
		ClearTouch();
	}
	else
		SetTouch(&CBaseButton::ButtonTouch);

	// reset think for a sparking button
	//func_rot_button's X Axis spawnflag overlaps with this one so don't use it here.
	if (!Is(Type::RotatingButton) && FBitSet(v.spawnflags, SF_BUTTON_SPARK_IF_OFF))
	{
		SetThink(&CBaseButton::ButtonSpark);
		v.nextthink = v.ltime + 0.5; // no hurry.
	}
}



//
// Rotating button (aka "lever")
//
class CRotButton : public CBaseButton
{
public:
	CRotButton(Entity* containingEntity) : CBaseButton(containingEntity) {}

	bool Is(const Type type) override
	{
		return type == Type::RotatingButton || CBaseButton::Is(type);
	}

	bool Spawn() override;
};

LINK_ENTITY_TO_CLASS(func_rot_button, CRotButton);

bool CRotButton::Spawn()
{
	const char* pszSound;
	//----------------------------------------------------
	//determine sounds for buttons
	//a sound of 0 should not make a sound
	//----------------------------------------------------
	pszSound = ButtonSound(m_sounds);
	engine::PrecacheSound(pszSound);
	v.noise = engine::AllocString(pszSound);

	// set the axis of rotation
	CBaseToggle::AxisDir(this);

	// check for clockwise rotation
	if (FBitSet(v.spawnflags, SF_DOOR_ROTATE_BACKWARDS))
		v.movedir = v.movedir * -1;

	v.movetype = MOVETYPE_PUSH;

	if ((v.spawnflags & SF_ROTBUTTON_NOTSOLID) != 0)
		v.solid = SOLID_NOT;
	else
		v.solid = SOLID_BSP;

	SetModel(v.model);

	if (v.speed == 0)
		v.speed = 40;

	if (m_flWait == 0)
		m_flWait = 1;

	if (v.health > 0)
	{
		v.takedamage = DAMAGE_YES;
	}

	m_toggle_state = TS_AT_BOTTOM;
	m_vecAngle1 = v.angles;
	m_vecAngle2 = v.angles + v.movedir * m_flMoveDistance;

	m_fStayPushed = (m_flWait == -1 ? true : false);
	m_fRotating = true;

	// if the button is flagged for USE button activation only, take away it's touch function and add a use function
	if (!FBitSet(v.spawnflags, SF_BUTTON_TOUCH_ONLY))
	{
		ClearTouch();
		SetUse(&CRotButton::ButtonUse);
	}
	else // touchable button
		SetTouch(&CRotButton::ButtonTouch);

	return true;
}


// Make this button behave like a door (HACKHACK)
// This will disable use and make the button solid
// rotating buttons were made SOLID_NOT by default since their were some
// collision problems with them...
#define SF_MOMENTARY_DOOR 0x0001
#define SF_MOMENTARY_AUTO_RETURN 16

class CMomentaryRotButton : public CBaseToggle
{
public:
	CMomentaryRotButton(Entity* containingEntity) : CBaseToggle(containingEntity) {}

	bool Is(const Type type) override { return type == Type::MomentaryButton; }

	DECLARE_SAVERESTORE()

	bool Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	int ObjectCaps() override
	{
		int flags = CBaseToggle::ObjectCaps() & (~FCAP_ACROSS_TRANSITION);
		if ((v.spawnflags & SF_MOMENTARY_DOOR) != 0)
			return flags;
		return flags | FCAP_CONTINUOUS_USE;
	}
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void EXPORT Off();
	void EXPORT Return();
	void UpdateSelf(float value);
	void UpdateSelfReturn(float value);
	void UpdateAllButtons(float value, bool start);

	void PlaySound();
	void UpdateTarget(float value);

	bool m_lastUsed;
	int m_direction;
	float m_returnSpeed;
	Vector m_start;
	Vector m_end;
	int m_sounds;
};

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CMomentaryRotButton)
	DEFINE_FIELD(CMomentaryRotButton, m_lastUsed, FIELD_BOOLEAN),
	DEFINE_FIELD(CMomentaryRotButton, m_direction, FIELD_INTEGER),
	DEFINE_FIELD(CMomentaryRotButton, m_returnSpeed, FIELD_FLOAT),
	DEFINE_FIELD(CMomentaryRotButton, m_start, FIELD_VECTOR),
	DEFINE_FIELD(CMomentaryRotButton, m_end, FIELD_VECTOR),
	DEFINE_FIELD(CMomentaryRotButton, m_sounds, FIELD_INTEGER),
END_SAVERESTORE(CMomentaryRotButton, CBaseToggle)
#endif

LINK_ENTITY_TO_CLASS(momentary_rot_button, CMomentaryRotButton);

bool CMomentaryRotButton::Spawn()
{
	CBaseToggle::AxisDir(this);

	if (v.speed == 0)
		v.speed = 100;

	if (m_flMoveDistance < 0)
	{
		m_start = v.angles + v.movedir * m_flMoveDistance;
		m_end = v.angles;
		m_direction = 1; // This will toggle to -1 on the first use()
		m_flMoveDistance = -m_flMoveDistance;
	}
	else
	{
		m_start = v.angles;
		m_end = v.angles + v.movedir * m_flMoveDistance;
		m_direction = -1; // This will toggle to +1 on the first use()
	}

	if ((v.spawnflags & SF_MOMENTARY_DOOR) != 0)
		v.solid = SOLID_BSP;
	else
		v.solid = SOLID_NOT;

	v.movetype = MOVETYPE_PUSH;
	SetOrigin(v.origin);
	SetModel(v.model);

	const char* pszSound = ButtonSound(m_sounds);
	engine::PrecacheSound(pszSound);
	v.noise = engine::AllocString(pszSound);
	m_lastUsed = false;

	return true;
}

bool CMomentaryRotButton::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "returnspeed"))
	{
		m_returnSpeed = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "sounds"))
	{
		m_sounds = atoi(pkvd->szValue);
		return true;
	}

	return CBaseToggle::KeyValue(pkvd);
}

void CMomentaryRotButton::PlaySound()
{
	EmitSound(STRING(v.noise), CHAN_VOICE);
}

// BUGBUG: This design causes a latentcy.  When the button is retriggered, the first impulse
// will send the target in the wrong direction because the parameter is calculated based on the
// current, not future position.
void CMomentaryRotButton::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	v.ideal_yaw = CBaseToggle::AxisDelta(v.spawnflags, v.angles, m_start) / m_flMoveDistance;

	UpdateAllButtons(v.ideal_yaw, true);

	// Calculate destination angle and use it to predict value, this prevents sending target in wrong direction on retriggering
	Vector dest = v.angles + v.avelocity * (v.nextthink - v.ltime);
	float value1 = CBaseToggle::AxisDelta(v.spawnflags, dest, m_start) / m_flMoveDistance;
	UpdateTarget(value1);
}

void CMomentaryRotButton::UpdateAllButtons(float value, bool start)
{
	// Update all rot buttons attached to the same target
	CBaseEntity* pentTarget = nullptr;
	for (;;)
	{

		pentTarget = util::FindEntityByString(pentTarget, "target", STRING(v.target));
		if (pentTarget == nullptr)
			break;

		if (pentTarget->Is(Type::MomentaryButton))
		{
			CMomentaryRotButton* pEntity = static_cast<CMomentaryRotButton*>(pentTarget);
			if (pEntity)
			{
				if (start)
					pEntity->UpdateSelf(value);
				else
					pEntity->UpdateSelfReturn(value);
			}
		}
	}
}

void CMomentaryRotButton::UpdateSelf(float value)
{
	bool fplaysound = false;

	if (!m_lastUsed)
	{
		fplaysound = true;
		m_direction = -m_direction;
	}
	m_lastUsed = true;

	v.nextthink = v.ltime + 0.1;
	if (m_direction > 0 && value >= 1.0)
	{
		v.avelocity = g_vecZero;
		v.angles = m_end;
		return;
	}
	else if (m_direction < 0 && value <= 0)
	{
		v.avelocity = g_vecZero;
		v.angles = m_start;
		return;
	}

	if (fplaysound)
		PlaySound();

	// HACKHACK -- If we're going slow, we'll get multiple player packets per frame, bump nexthink on each one to avoid stalling
	if (v.nextthink < v.ltime)
		v.nextthink = v.ltime + 0.1;
	else
		v.nextthink += 0.1;

	v.avelocity = (m_direction * v.speed) * v.movedir;
	SetThink(&CMomentaryRotButton::Off);
}

void CMomentaryRotButton::UpdateTarget(float value)
{
	if (!FStringNull(v.target))
	{
		CBaseEntity* pentTarget = nullptr;
		for (;;)
		{
			pentTarget = util::FindEntityByTargetname(pentTarget, STRING(v.target));
			if (pentTarget == nullptr)
				break;

			pentTarget->Use(this, this, USE_SET, value);
		}
	}
}

void CMomentaryRotButton::Off()
{
	v.avelocity = g_vecZero;
	m_lastUsed = false;
	if (FBitSet(v.spawnflags, SF_MOMENTARY_AUTO_RETURN) && m_returnSpeed > 0)
	{
		SetThink(&CMomentaryRotButton::Return);
		v.nextthink = v.ltime + 0.1;
		m_direction = -1;
	}
	else
		ClearThink();
}

void CMomentaryRotButton::Return()
{
	float value = CBaseToggle::AxisDelta(v.spawnflags, v.angles, m_start) / m_flMoveDistance;

	UpdateAllButtons(value, false); // This will end up calling UpdateSelfReturn() n times, but it still works right
	if (value > 0)
		UpdateTarget(value);
}


void CMomentaryRotButton::UpdateSelfReturn(float value)
{
	if (value <= 0)
	{
		v.avelocity = g_vecZero;
		v.angles = m_start;
		v.nextthink = -1;
		ClearThink();
	}
	else
	{
		v.avelocity = -m_returnSpeed * v.movedir;
		v.nextthink = v.ltime + 0.1;
	}
}


//----------------------------------------------------------------
// Spark
//----------------------------------------------------------------

class CEnvSpark : public CPointEntity
{
public:
	CEnvSpark(Entity* containingEntity) : CPointEntity(containingEntity) {}

	DECLARE_SAVERESTORE()

	bool Spawn() override;
	void Precache() override;
	void EXPORT SparkThink();
	void EXPORT SparkStart(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT SparkStop(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	bool KeyValue(KeyValueData* pkvd) override;

	float m_flDelay;
};


#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CEnvSpark)
	DEFINE_FIELD(CEnvSpark, m_flDelay, FIELD_FLOAT),
END_SAVERESTORE(CEnvSpark, CBaseEntity)
#endif

LINK_ENTITY_TO_CLASS(env_spark, CEnvSpark);

bool CEnvSpark::Spawn()
{
	ClearThink();
	ClearUse();

	if (FBitSet(v.spawnflags, 32)) // Use for on/off
	{
		if (FBitSet(v.spawnflags, 64)) // Start on
		{
			SetThink(&CEnvSpark::SparkThink); // start sparking
			SetUse(&CEnvSpark::SparkStop);	  // set up +USE to stop sparking
		}
		else
			SetUse(&CEnvSpark::SparkStart);
	}
	else
		SetThink(&CEnvSpark::SparkThink);

	v.nextthink = gpGlobals->time + (0.1 + engine::RandomFloat(0, 1.5));

	if (m_flDelay <= 0)
		m_flDelay = 1.5;

	Precache();

	return true;
}


void CEnvSpark::Precache()
{
	engine::PrecacheSound("buttons/spark1.wav");
	engine::PrecacheSound("buttons/spark2.wav");
	engine::PrecacheSound("buttons/spark3.wav");
	engine::PrecacheSound("buttons/spark4.wav");
	engine::PrecacheSound("buttons/spark5.wav");
	engine::PrecacheSound("buttons/spark6.wav");
}

bool CEnvSpark::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "MaxDelay"))
	{
		m_flDelay = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "style") ||
			 streq(pkvd->szKeyName, "height") ||
			 streq(pkvd->szKeyName, "killtarget") ||
			 streq(pkvd->szKeyName, "value1") ||
			 streq(pkvd->szKeyName, "value2") ||
			 streq(pkvd->szKeyName, "value3"))
		return true;

	return CBaseEntity::KeyValue(pkvd);
}

void EXPORT CEnvSpark::SparkThink()
{
	v.nextthink = gpGlobals->time + 0.1 + engine::RandomFloat(0, m_flDelay);
	DoSpark(this, v.origin);
}

void EXPORT CEnvSpark::SparkStart(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	SetUse(&CEnvSpark::SparkStop);
	SetThink(&CEnvSpark::SparkThink);
	v.nextthink = gpGlobals->time + (0.1 + engine::RandomFloat(0, m_flDelay));
}

void EXPORT CEnvSpark::SparkStop(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	SetUse(&CEnvSpark::SparkStart);
	ClearThink();
}

#define SF_BTARGET_USE 0x0001
#define SF_BTARGET_ON 0x0002

class CButtonTarget : public CBaseEntity
{
public:
	CButtonTarget(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	bool Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;
	int ObjectCaps() override;
};

LINK_ENTITY_TO_CLASS(button_target, CButtonTarget);

bool CButtonTarget::Spawn()
{
	v.movetype = MOVETYPE_PUSH;
	v.solid = SOLID_BSP;
	SetModel(v.model);
	v.takedamage = DAMAGE_YES;

	if (FBitSet(v.spawnflags, SF_BTARGET_ON))
		v.frame = 1;

	return true;
}

void CButtonTarget::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!ShouldToggle(useType, 0 != v.frame))
		return;
	v.frame = 0 != v.frame ? 0 : 1;
	if (0 != v.frame)
		UseTargets(pActivator, USE_ON, 0);
	else
		UseTargets(pActivator, USE_OFF, 0);
}


int CButtonTarget::ObjectCaps()
{
	int caps = CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;

	if (FBitSet(v.spawnflags, SF_BTARGET_USE))
		return caps | FCAP_IMPULSE_USE;
	else
		return caps;
}


bool CButtonTarget::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	Use(attacker, this, USE_TOGGLE, 0);

	return true;
}
