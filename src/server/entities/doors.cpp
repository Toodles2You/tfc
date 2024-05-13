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

===== doors.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "doors.h"

#define noiseMoving noise1
#define noiseArrived noise2

#define SF_ITEM_USE_ONLY 256

class CBaseDoor : public CBaseToggle
{
public:
	CBaseDoor(Entity* containingEntity) : CBaseToggle(containingEntity) {}

	bool Is(const Type type) override { return type == Type::Door; }

	DECLARE_SAVERESTORE()

	bool Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void Blocked(CBaseEntity* pOther) override;


	int ObjectCaps() override
	{
		if ((v.spawnflags & SF_ITEM_USE_ONLY) != 0)
			return (CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_IMPULSE_USE;
		else
			return (CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION);
	};

	void SetToggleState(int state) override;

	// used to selectivly override defaults
	void EXPORT DoorTouch(CBaseEntity* pOther);

	// local functions
	bool DoorActivate();
	void EXPORT DoorGoUp();
	void EXPORT DoorGoDown();
	void EXPORT DoorHitTop();
	void EXPORT DoorHitBottom();

	byte m_bHealthValue; // some doors are medi-kit doors, they give players health

	byte m_bMoveSnd; // sound a door makes while moving
	byte m_bStopSnd; // sound a door makes when it stops

	locksound_t m_ls; // door lock sounds

	byte m_bLockedSound; // ordinals from entity selection
	byte m_bLockedSentence;
	byte m_bUnlockedSound;
	byte m_bUnlockedSentence;
};


#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CBaseDoor)
	DEFINE_FIELD(CBaseDoor, m_bHealthValue, FIELD_CHARACTER),
	DEFINE_FIELD(CBaseDoor, m_bMoveSnd, FIELD_CHARACTER),
	DEFINE_FIELD(CBaseDoor, m_bStopSnd, FIELD_CHARACTER),

	DEFINE_FIELD(CBaseDoor, m_bLockedSound, FIELD_CHARACTER),
	DEFINE_FIELD(CBaseDoor, m_bLockedSentence, FIELD_CHARACTER),
	DEFINE_FIELD(CBaseDoor, m_bUnlockedSound, FIELD_CHARACTER),
	DEFINE_FIELD(CBaseDoor, m_bUnlockedSentence, FIELD_CHARACTER),
END_SAVERESTORE(CBaseDoor, CBaseToggle)
#endif


#define DOOR_SENTENCEWAIT 6
#define DOOR_SOUNDWAIT 3
#define BUTTON_SOUNDWAIT 0.5

// play door or button locked or unlocked sounds.
// pass in pointer to valid locksound struct.
// if flocked is true, play 'door is locked' sound,
// otherwise play 'door is unlocked' sound
// NOTE: this routine is shared by doors and buttons

void PlayLockSounds(CBaseEntity* entity, locksound_t* pls, bool flocked, bool fbutton)
{
	// LOCKED SOUND

	// CONSIDER: consolidate the locksound_t struct (all entries are duplicates for lock/unlock)
	// CONSIDER: and condense this code.
	float flsoundwait;

	if (fbutton)
		flsoundwait = BUTTON_SOUNDWAIT;
	else
		flsoundwait = DOOR_SOUNDWAIT;

	if (flocked)
	{
		bool fplaysound = (!FStringNull(pls->sLockedSound) && gpGlobals->time > pls->flwaitSound);
		bool fplaysentence = (!FStringNull(pls->sLockedSentence) && 0 == pls->bEOFLocked && gpGlobals->time > pls->flwaitSentence);
		float fvol;

		if (fplaysound && fplaysentence)
			fvol = 0.25;
		else
			fvol = 1.0;

		// if there is a locked sound, and we've debounced, play sound
		if (fplaysound)
		{
			// play 'door locked' sound
			entity->EmitSound(STRING(pls->sLockedSound), CHAN_ITEM, fvol);
			pls->flwaitSound = gpGlobals->time + flsoundwait;
		}

		// if there is a sentence, we've not played all in list, and we've debounced, play sound
		if (fplaysentence)
		{
			// play next 'door locked' sentence in group
			int iprev = pls->iLockedSentence;

			pls->iLockedSentence = SENTENCEG_PlaySequentialSz(entity, STRING(pls->sLockedSentence),
				0.85, ATTN_NORM, 0, 100, pls->iLockedSentence, false);
			pls->iUnlockedSentence = 0;

			// make sure we don't keep calling last sentence in list
			pls->bEOFLocked = static_cast<int>(iprev == pls->iLockedSentence);

			pls->flwaitSentence = gpGlobals->time + DOOR_SENTENCEWAIT;
		}
	}
	else
	{
		// UNLOCKED SOUND

		bool fplaysound = (!FStringNull(pls->sUnlockedSound) && gpGlobals->time > pls->flwaitSound);
		bool fplaysentence = (!FStringNull(pls->sUnlockedSentence) && 0 == pls->bEOFUnlocked && gpGlobals->time > pls->flwaitSentence);
		float fvol;

		// if playing both sentence and sound, lower sound volume so we hear sentence
		if (fplaysound && fplaysentence)
			fvol = 0.25;
		else
			fvol = 1.0;

		// play 'door unlocked' sound if set
		if (fplaysound)
		{
			entity->EmitSound(STRING(pls->sUnlockedSound), CHAN_ITEM, fvol);
			pls->flwaitSound = gpGlobals->time + flsoundwait;
		}

		// play next 'door unlocked' sentence in group
		if (fplaysentence)
		{
			int iprev = pls->iUnlockedSentence;

			pls->iUnlockedSentence = SENTENCEG_PlaySequentialSz(entity, STRING(pls->sUnlockedSentence),
				0.85, ATTN_NORM, 0, 100, pls->iUnlockedSentence, false);
			pls->iLockedSentence = 0;

			// make sure we don't keep calling last sentence in list
			pls->bEOFUnlocked = static_cast<int>(iprev == pls->iUnlockedSentence);
			pls->flwaitSentence = gpGlobals->time + DOOR_SENTENCEWAIT;
		}
	}
}

//
// Cache user-entity-field values until spawn is called.
//

bool CBaseDoor::KeyValue(KeyValueData* pkvd)
{

	if (streq(pkvd->szKeyName, "skin")) //skin is used for content type
	{
		v.skin = atof(pkvd->szValue);
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
	else if (streq(pkvd->szKeyName, "healthvalue"))
	{
		m_bHealthValue = atof(pkvd->szValue);
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
	else if (streq(pkvd->szKeyName, "WaveHeight"))
	{
		v.scale = atof(pkvd->szValue) * (1.0 / 8.0);
		return true;
	}

	return CBaseToggle::KeyValue(pkvd);
}


LINK_ENTITY_TO_CLASS(func_door, CBaseDoor);
//
// func_water - same as a door.
//
LINK_ENTITY_TO_CLASS(func_water, CBaseDoor);


bool CBaseDoor::Spawn()
{
	Precache();
	v.movedir = util::SetMovedir(v.angles);

	if (v.skin == 0)
	{ //normal door
		if (FBitSet(v.spawnflags, SF_DOOR_PASSABLE))
			v.solid = SOLID_NOT;
		else
			v.solid = SOLID_BSP;
	}
	else
	{ // special contents
		v.solid = SOLID_NOT;
		SetBits(v.spawnflags, SF_DOOR_SILENT); // water is silent for now
	}

	v.movetype = MOVETYPE_PUSH;
	SetOrigin(v.origin);
	SetModel(v.model);

	if (v.speed == 0)
		v.speed = 100;

	m_vecPosition1 = v.origin;
	// Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big
	m_vecPosition2 = m_vecPosition1 + (v.movedir * (fabs(v.movedir.x * (v.size.x - 2)) + fabs(v.movedir.y * (v.size.y - 2)) + fabs(v.movedir.z * (v.size.z - 2)) - m_flLip));

	if (FBitSet(v.spawnflags, SF_DOOR_START_OPEN))
	{ // swap pos1 and pos2, put door at pos2
		SetOrigin(m_vecPosition2);
		m_vecPosition2 = m_vecPosition1;
		m_vecPosition1 = v.origin;
	}

	m_toggle_state = TS_AT_BOTTOM;

	// if the door is flagged for USE button activation only, use nullptr touch function
	if (FBitSet(v.spawnflags, SF_DOOR_USE_ONLY))
	{
		ClearTouch();
	}
	else // touchable button
		SetTouch(&CBaseDoor::DoorTouch);

	return true;
}


void CBaseDoor::SetToggleState(int state)
{
	if (state == TS_AT_TOP)
		SetOrigin(m_vecPosition2);
	else
		SetOrigin(m_vecPosition1);
}


void CBaseDoor::Precache()
{
	const char* pszSound;

	// set the door's "in-motion" sound
	switch (m_bMoveSnd)
	{
	case 0:
		v.noiseMoving = ALLOC_STRING("common/null.wav");
		break;
	case 1:
		PRECACHE_SOUND("doors/doormove1.wav");
		v.noiseMoving = ALLOC_STRING("doors/doormove1.wav");
		break;
	case 2:
		PRECACHE_SOUND("doors/doormove2.wav");
		v.noiseMoving = ALLOC_STRING("doors/doormove2.wav");
		break;
	case 3:
		PRECACHE_SOUND("doors/doormove3.wav");
		v.noiseMoving = ALLOC_STRING("doors/doormove3.wav");
		break;
	case 4:
		PRECACHE_SOUND("doors/doormove4.wav");
		v.noiseMoving = ALLOC_STRING("doors/doormove4.wav");
		break;
	case 5:
		PRECACHE_SOUND("doors/doormove5.wav");
		v.noiseMoving = ALLOC_STRING("doors/doormove5.wav");
		break;
	case 6:
		PRECACHE_SOUND("doors/doormove6.wav");
		v.noiseMoving = ALLOC_STRING("doors/doormove6.wav");
		break;
	case 7:
		PRECACHE_SOUND("doors/doormove7.wav");
		v.noiseMoving = ALLOC_STRING("doors/doormove7.wav");
		break;
	case 8:
		PRECACHE_SOUND("doors/doormove8.wav");
		v.noiseMoving = ALLOC_STRING("doors/doormove8.wav");
		break;
	case 9:
		PRECACHE_SOUND("doors/doormove9.wav");
		v.noiseMoving = ALLOC_STRING("doors/doormove9.wav");
		break;
	case 10:
		PRECACHE_SOUND("doors/doormove10.wav");
		v.noiseMoving = ALLOC_STRING("doors/doormove10.wav");
		break;
	default:
		v.noiseMoving = ALLOC_STRING("common/null.wav");
		break;
	}

	// set the door's 'reached destination' stop sound
	switch (m_bStopSnd)
	{
	case 0:
		v.noiseArrived = ALLOC_STRING("common/null.wav");
		break;
	case 1:
		PRECACHE_SOUND("doors/doorstop1.wav");
		v.noiseArrived = ALLOC_STRING("doors/doorstop1.wav");
		break;
	case 2:
		PRECACHE_SOUND("doors/doorstop2.wav");
		v.noiseArrived = ALLOC_STRING("doors/doorstop2.wav");
		break;
	case 3:
		PRECACHE_SOUND("doors/doorstop3.wav");
		v.noiseArrived = ALLOC_STRING("doors/doorstop3.wav");
		break;
	case 4:
		PRECACHE_SOUND("doors/doorstop4.wav");
		v.noiseArrived = ALLOC_STRING("doors/doorstop4.wav");
		break;
	case 5:
		PRECACHE_SOUND("doors/doorstop5.wav");
		v.noiseArrived = ALLOC_STRING("doors/doorstop5.wav");
		break;
	case 6:
		PRECACHE_SOUND("doors/doorstop6.wav");
		v.noiseArrived = ALLOC_STRING("doors/doorstop6.wav");
		break;
	case 7:
		PRECACHE_SOUND("doors/doorstop7.wav");
		v.noiseArrived = ALLOC_STRING("doors/doorstop7.wav");
		break;
	case 8:
		PRECACHE_SOUND("doors/doorstop8.wav");
		v.noiseArrived = ALLOC_STRING("doors/doorstop8.wav");
		break;
	default:
		v.noiseArrived = ALLOC_STRING("common/null.wav");
		break;
	}

	// get door button sounds, for doors which are directly 'touched' to open

	if (0 != m_bLockedSound)
	{
		pszSound = ButtonSound((int)m_bLockedSound);
		PRECACHE_SOUND(pszSound);
		m_ls.sLockedSound = ALLOC_STRING(pszSound);
	}

	if (0 != m_bUnlockedSound)
	{
		pszSound = ButtonSound((int)m_bUnlockedSound);
		PRECACHE_SOUND(pszSound);
		m_ls.sUnlockedSound = ALLOC_STRING(pszSound);
	}

	// get sentence group names, for doors which are directly 'touched' to open

	switch (m_bLockedSentence)
	{
	case 1:
		m_ls.sLockedSentence = ALLOC_STRING("NA");
		break; // access denied
	case 2:
		m_ls.sLockedSentence = ALLOC_STRING("ND");
		break; // security lockout
	case 3:
		m_ls.sLockedSentence = ALLOC_STRING("NF");
		break; // blast door
	case 4:
		m_ls.sLockedSentence = ALLOC_STRING("NFIRE");
		break; // fire door
	case 5:
		m_ls.sLockedSentence = ALLOC_STRING("NCHEM");
		break; // chemical door
	case 6:
		m_ls.sLockedSentence = ALLOC_STRING("NRAD");
		break; // radiation door
	case 7:
		m_ls.sLockedSentence = ALLOC_STRING("NCON");
		break; // gen containment
	case 8:
		m_ls.sLockedSentence = ALLOC_STRING("NH");
		break; // maintenance door
	case 9:
		m_ls.sLockedSentence = ALLOC_STRING("NG");
		break; // broken door

	default:
		m_ls.sLockedSentence = 0;
		break;
	}

	switch (m_bUnlockedSentence)
	{
	case 1:
		m_ls.sUnlockedSentence = ALLOC_STRING("EA");
		break; // access granted
	case 2:
		m_ls.sUnlockedSentence = ALLOC_STRING("ED");
		break; // security door
	case 3:
		m_ls.sUnlockedSentence = ALLOC_STRING("EF");
		break; // blast door
	case 4:
		m_ls.sUnlockedSentence = ALLOC_STRING("EFIRE");
		break; // fire door
	case 5:
		m_ls.sUnlockedSentence = ALLOC_STRING("ECHEM");
		break; // chemical door
	case 6:
		m_ls.sUnlockedSentence = ALLOC_STRING("ERAD");
		break; // radiation door
	case 7:
		m_ls.sUnlockedSentence = ALLOC_STRING("ECON");
		break; // gen containment
	case 8:
		m_ls.sUnlockedSentence = ALLOC_STRING("EH");
		break; // maintenance door

	default:
		m_ls.sUnlockedSentence = 0;
		break;
	}
}

//
// Doors not tied to anything (e.g. button, another door) can be touched, to make them activate.
//
void CBaseDoor::DoorTouch(CBaseEntity* pOther)
{
	// Ignore touches by anything but players
	if (!pOther->IsPlayer())
		return;

	// If door has master, and it's not ready to trigger,
	// play 'locked' sound

	if (!FStringNull(m_sMaster) && !util::IsMasterTriggered(m_sMaster, pOther))
		PlayLockSounds(this, &m_ls, true, false);

	// If door is somebody's target, then touching does nothing.
	// You have to activate the owner (e.g. button).

	if (!FStringNull(v.targetname))
	{
		// play locked sound
		PlayLockSounds(this, &m_ls, true, false);
		return;
	}

	m_hActivator = pOther; // remember who activated the door

	if (DoorActivate())
		ClearTouch(); // Temporarily disable the touch function, until movement is finished.
}


//
// Used by UseTargets, when a door is the target of a button.
//
void CBaseDoor::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	m_hActivator = pActivator;
	// if not ready to be used, ignore "use" command.
	if (m_toggle_state == TS_AT_BOTTOM || FBitSet(v.spawnflags, SF_DOOR_NO_AUTO_RETURN) && m_toggle_state == TS_AT_TOP)
		DoorActivate();
}

//
// Causes the door to "do its thing", i.e. start moving, and cascade activation.
//
bool CBaseDoor::DoorActivate()
{
	if (!util::IsMasterTriggered(m_sMaster, m_hActivator))
		return false;

	if (FBitSet(v.spawnflags, SF_DOOR_NO_AUTO_RETURN) && m_toggle_state == TS_AT_TOP)
	{ // door should close
		DoorGoDown();
	}
	else
	{ // door should open

		if (m_bHealthValue > 0 && m_hActivator != nullptr && m_hActivator->IsPlayer())
		{ // give health if player opened the door (medikit)
			m_hActivator->GiveHealth(m_bHealthValue, DMG_GENERIC);
		}

		// play door unlock sounds
		PlayLockSounds(this, &m_ls, false, false);

		DoorGoUp();
	}

	return true;
}

//
// Starts the door going to its "up" position (simply ToggleData->vecPosition2).
//
void CBaseDoor::DoorGoUp()
{
	CBaseEntity* pevActivator;

	// emit door moving and stop sounds on CHAN_STATIC so that the multicast doesn't
	// filter them out and leave a client stuck with looping door sounds!
	if (!FBitSet(v.spawnflags, SF_DOOR_SILENT))
	{
		if (m_toggle_state != TS_GOING_UP && m_toggle_state != TS_GOING_DOWN)
			EmitSound(STRING(v.noiseMoving), CHAN_STATIC);
	}

	m_toggle_state = TS_GOING_UP;

	SetMoveDone(&CBaseDoor::DoorHitTop);
	if (Is(Type::RotatingDoor)) // !!! BUGBUG Triggered doors don't work with this yet
	{
		float sign = 1.0;

		if (m_hActivator != nullptr)
		{
			pevActivator = m_hActivator;

			if (!FBitSet(v.spawnflags, SF_DOOR_ONEWAY) && 0 != v.movedir.y) // Y axis rotation, move away from the player
			{
				Vector vec = pevActivator->v.origin - v.origin;
				Vector angles = pevActivator->v.angles;
				angles.x = 0;
				angles.z = 0;
				util::MakeVectors(angles);
				//			Vector vnext = (pevToucher->origin + (pevToucher->velocity * 10)) - v.origin;
				util::MakeVectors(pevActivator->v.angles);
				Vector vnext = (pevActivator->v.origin + (gpGlobals->v_forward * 10)) - v.origin;
				if ((vec.x * vnext.y - vec.y * vnext.x) < 0)
					sign = -1.0;
			}
		}
		AngularMove(m_vecAngle2 * sign, v.speed);
	}
	else
		LinearMove(m_vecPosition2, v.speed);
}


//
// The door has reached the "up" position.  Either go back down, or wait for another activation.
//
void CBaseDoor::DoorHitTop()
{
	if (!FBitSet(v.spawnflags, SF_DOOR_SILENT))
	{
		StopSound(STRING(v.noiseMoving), CHAN_STATIC);
		EmitSound(STRING(v.noiseArrived), CHAN_STATIC);
	}

	m_toggle_state = TS_AT_TOP;

	// toggle-doors don't come down automatically, they wait for refire.
	if (FBitSet(v.spawnflags, SF_DOOR_NO_AUTO_RETURN))
	{
		// Re-instate touch method, movement is complete
		if (!FBitSet(v.spawnflags, SF_DOOR_USE_ONLY))
			SetTouch(&CBaseDoor::DoorTouch);
	}
	else
	{
		// In flWait seconds, DoorGoDown will fire, unless wait is -1, then door stays open
		v.nextthink = v.ltime + m_flWait;
		SetThink(&CBaseDoor::DoorGoDown);

		if (m_flWait == -1)
		{
			v.nextthink = -1;
		}
	}

	// Fire the close target (if startopen is set, then "top" is closed) - netname is the close target
	if (!FStringNull(v.netname) && (v.spawnflags & SF_DOOR_START_OPEN) != 0)
		util::FireTargets(STRING(v.netname), m_hActivator, this, USE_TOGGLE, 0);

	UseTargets(m_hActivator, USE_TOGGLE, 0); // this isn't finished
}


//
// Starts the door going to its "down" position (simply ToggleData->vecPosition1).
//
void CBaseDoor::DoorGoDown()
{
	if (!FBitSet(v.spawnflags, SF_DOOR_SILENT))
	{
		if (m_toggle_state != TS_GOING_UP && m_toggle_state != TS_GOING_DOWN)
			EmitSound(STRING(v.noiseMoving), CHAN_STATIC);
	}

	m_toggle_state = TS_GOING_DOWN;

	SetMoveDone(&CBaseDoor::DoorHitBottom);
	if (Is(Type::RotatingDoor)) //rotating door
		AngularMove(m_vecAngle1, v.speed);
	else
		LinearMove(m_vecPosition1, v.speed);
}

//
// The door has reached the "down" position.  Back to quiescence.
//
void CBaseDoor::DoorHitBottom()
{
	if (!FBitSet(v.spawnflags, SF_DOOR_SILENT))
	{
		StopSound(STRING(v.noiseMoving), CHAN_STATIC);
		EmitSound(STRING(v.noiseArrived), CHAN_STATIC);
	}

	m_toggle_state = TS_AT_BOTTOM;

	// Re-instate touch method, cycle is complete
	if (FBitSet(v.spawnflags, SF_DOOR_USE_ONLY))
	{ // use only door
		ClearTouch();
	}
	else // touchable door
		SetTouch(&CBaseDoor::DoorTouch);

	UseTargets(m_hActivator, USE_TOGGLE, 0); // this isn't finished

	// Fire the close target (if startopen is set, then "top" is closed) - netname is the close target
	if (!FStringNull(v.netname) && (v.spawnflags & SF_DOOR_START_OPEN) == 0)
		util::FireTargets(STRING(v.netname), m_hActivator, this, USE_TOGGLE, 0);
}

void CBaseDoor::Blocked(CBaseEntity* pOther)
{
	CBaseEntity* pentTarget = nullptr;
	CBaseDoor* pDoor = nullptr;


	// Hurt the blocker a little.
	if (0 != v.dmg)
		pOther->TakeDamage(this, this, v.dmg, DMG_CRUSH);

	// if a door has a negative wait, it would never come back if blocked,
	// so let it just squash the object to death real fast

	if (m_flWait >= 0)
	{
		if (m_toggle_state == TS_GOING_DOWN)
		{
			DoorGoUp();
		}
		else
		{
			DoorGoDown();
		}
	}

	// Block all door pieces with the same targetname here.
	if (!FStringNull(v.targetname))
	{
		for (;;)
		{
			pentTarget = util::FindEntityByTargetname(pentTarget, STRING(v.targetname));

			if (pentTarget != this)
			{
				if (pentTarget == nullptr)
					break;

				if (pentTarget->Is(Type::Door))
				{

					pDoor = static_cast<CBaseDoor*>(pentTarget);

					if (pDoor->m_flWait >= 0)
					{
						if (pDoor->v.velocity == v.velocity && pDoor->v.avelocity == v.velocity)
						{
							// this is the most hacked, evil, bastardized thing I've ever seen. kjb
							if (Is(Type::RotatingDoor))
							{ // set origin to realign normal doors
								pDoor->v.origin = v.origin;
								pDoor->v.velocity = g_vecZero; // stop!
							}
							else
							{ // set angles to realign rotating doors
								pDoor->v.angles = v.angles;
								pDoor->v.avelocity = g_vecZero;
							}
						}

						if (!FBitSet(v.spawnflags, SF_DOOR_SILENT))
							StopSound(STRING(v.noiseMoving), CHAN_STATIC);

						if (pDoor->m_toggle_state == TS_GOING_DOWN)
							pDoor->DoorGoUp();
						else
							pDoor->DoorGoDown();
					}
				}
			}
		}
	}
}


class CRotDoor : public CBaseDoor
{
public:
	CRotDoor(Entity* containingEntity) : CBaseDoor(containingEntity) {}

	bool Is(const Type type) override
	{
		return type == Type::RotatingDoor || CBaseDoor::Is(type);
	}

	bool Spawn() override;
	void SetToggleState(int state) override;
};

LINK_ENTITY_TO_CLASS(func_door_rotating, CRotDoor);


bool CRotDoor::Spawn()
{
	Precache();
	// set the axis of rotation
	CBaseToggle::AxisDir(this);

	// check for clockwise rotation
	if (FBitSet(v.spawnflags, SF_DOOR_ROTATE_BACKWARDS))
		v.movedir = v.movedir * -1;

	//m_flWait			= 2; who the hell did this? (sjb)
	m_vecAngle1 = v.angles;
	m_vecAngle2 = v.angles + v.movedir * m_flMoveDistance;

	if (FBitSet(v.spawnflags, SF_DOOR_PASSABLE))
		v.solid = SOLID_NOT;
	else
		v.solid = SOLID_BSP;

	v.movetype = MOVETYPE_PUSH;
	SetOrigin(v.origin);
	SetModel(v.model);

	if (v.speed == 0)
		v.speed = 100;

	// DOOR_START_OPEN is to allow an entity to be lighted in the closed position
	// but spawn in the open position
	if (FBitSet(v.spawnflags, SF_DOOR_START_OPEN))
	{ // swap pos1 and pos2, put door at pos2, invert movement direction
		v.angles = m_vecAngle2;
		Vector vecSav = m_vecAngle1;
		m_vecAngle2 = m_vecAngle1;
		m_vecAngle1 = vecSav;
		v.movedir = v.movedir * -1;
	}

	m_toggle_state = TS_AT_BOTTOM;

	if (FBitSet(v.spawnflags, SF_DOOR_USE_ONLY))
	{
		ClearTouch();
	}
	else // touchable button
		SetTouch(&CRotDoor::DoorTouch);

	return true;
}


void CRotDoor::SetToggleState(int state)
{
	if (state == TS_AT_TOP)
		v.angles = m_vecAngle2;
	else
		v.angles = m_vecAngle1;

	SetOrigin(v.origin);
}


class CMomentaryDoor : public CBaseToggle
{
public:
	CMomentaryDoor(Entity* containingEntity) : CBaseToggle(containingEntity) {}

	DECLARE_SAVERESTORE()

	bool Spawn() override;
	void Precache() override;

	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	int ObjectCaps() override { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	void EXPORT DoorMoveDone();
	void EXPORT StopMoveSound();

	byte m_bMoveSnd; // sound a door makes while moving
};

LINK_ENTITY_TO_CLASS(momentary_door, CMomentaryDoor);

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CMomentaryDoor)
	DEFINE_FIELD(CMomentaryDoor, m_bMoveSnd, FIELD_CHARACTER),
END_SAVERESTORE(CMomentaryDoor, CBaseToggle)
#endif

bool CMomentaryDoor::Spawn()
{
	v.movedir = util::SetMovedir(v.angles);

	v.solid = SOLID_BSP;
	v.movetype = MOVETYPE_PUSH;

	SetOrigin(v.origin);
	SetModel(v.model);

	if (v.speed == 0)
		v.speed = 100;
	if (v.dmg == 0)
		v.dmg = 2;

	m_vecPosition1 = v.origin;
	// Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big
	m_vecPosition2 = m_vecPosition1 + (v.movedir * (fabs(v.movedir.x * (v.size.x - 2)) + fabs(v.movedir.y * (v.size.y - 2)) + fabs(v.movedir.z * (v.size.z - 2)) - m_flLip));

	if (FBitSet(v.spawnflags, SF_DOOR_START_OPEN))
	{ // swap pos1 and pos2, put door at pos2
		SetOrigin(m_vecPosition2);
		m_vecPosition2 = m_vecPosition1;
		m_vecPosition1 = v.origin;
	}
	ClearTouch();

	Precache();

	return true;
}

void CMomentaryDoor::Precache()
{

	// set the door's "in-motion" sound
	switch (m_bMoveSnd)
	{
	case 0:
		v.noiseMoving = ALLOC_STRING("common/null.wav");
		break;
	case 1:
		PRECACHE_SOUND("doors/doormove1.wav");
		v.noiseMoving = ALLOC_STRING("doors/doormove1.wav");
		break;
	case 2:
		PRECACHE_SOUND("doors/doormove2.wav");
		v.noiseMoving = ALLOC_STRING("doors/doormove2.wav");
		break;
	case 3:
		PRECACHE_SOUND("doors/doormove3.wav");
		v.noiseMoving = ALLOC_STRING("doors/doormove3.wav");
		break;
	case 4:
		PRECACHE_SOUND("doors/doormove4.wav");
		v.noiseMoving = ALLOC_STRING("doors/doormove4.wav");
		break;
	case 5:
		PRECACHE_SOUND("doors/doormove5.wav");
		v.noiseMoving = ALLOC_STRING("doors/doormove5.wav");
		break;
	case 6:
		PRECACHE_SOUND("doors/doormove6.wav");
		v.noiseMoving = ALLOC_STRING("doors/doormove6.wav");
		break;
	case 7:
		PRECACHE_SOUND("doors/doormove7.wav");
		v.noiseMoving = ALLOC_STRING("doors/doormove7.wav");
		break;
	case 8:
		PRECACHE_SOUND("doors/doormove8.wav");
		v.noiseMoving = ALLOC_STRING("doors/doormove8.wav");
		break;
	default:
		v.noiseMoving = ALLOC_STRING("common/null.wav");
		break;
	}
}

bool CMomentaryDoor::KeyValue(KeyValueData* pkvd)
{

	if (streq(pkvd->szKeyName, "movesnd"))
	{
		m_bMoveSnd = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "stopsnd"))
	{
		//		m_bStopSnd = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "healthvalue"))
	{
		//		m_bHealthValue = atof(pkvd->szValue);
		return true;
	}

	return CBaseToggle::KeyValue(pkvd);
}

void CMomentaryDoor::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (useType != USE_SET) // Momentary buttons will pass down a float in here
		return;

	if (value > 1.0)
		value = 1.0;
	if (value < 0.0)
		value = 0.0;

	Vector move = m_vecPosition1 + (value * (m_vecPosition2 - m_vecPosition1));

	Vector delta = move - v.origin;
	//float speed = delta.Length() * 10;
	float speed = delta.Length() / 0.1; // move there in 0.1 sec
	if (speed == 0)
		return;

	// This entity only thinks when it moves, so if it's thinking, it's in the process of moving
	// play the sound when it starts moving (not yet thinking)
	if (v.nextthink < v.ltime || v.nextthink == 0)
		EmitSound(STRING(v.noiseMoving), CHAN_STATIC);
	// If we already moving to designated point, return
	else if (move == m_vecFinalDest)
		return;

	SetMoveDone(&CMomentaryDoor::DoorMoveDone);
	LinearMove(move, speed);
}

//
// The door has reached needed position.
//
void CMomentaryDoor::DoorMoveDone()
{
	// Stop sounds at the next think, rather than here as another
	// Use call might immediately follow the end of this move
	//This think function will be replaced by LinearMove if that happens.
	SetThink(&CMomentaryDoor::StopMoveSound);
	v.nextthink = v.ltime + 0.1f;
}

void CMomentaryDoor::StopMoveSound()
{
	StopSound(STRING(v.noiseMoving), CHAN_STATIC);
	EmitSound(STRING(v.noiseArrived), CHAN_STATIC);
	ClearThink();
}
