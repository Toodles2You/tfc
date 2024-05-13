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
#include "doors.h"

#define SF_BRUSH_ROTATE_Y_AXIS 0
#define SF_BRUSH_ROTATE_INSTANT 1
#define SF_BRUSH_ROTATE_BACKWARDS 2
#define SF_BRUSH_ROTATE_Z_AXIS 4
#define SF_BRUSH_ROTATE_X_AXIS 8
#define SF_BRUSH_ROTATE_SMALLRADIUS 128
#define SF_BRUSH_ROTATE_MEDIUMRADIUS 256
#define SF_BRUSH_ROTATE_LARGERADIUS 512

#define SF_BRUSH_ACCDCC 16		 // brush should accelerate and decelerate when toggled
#define SF_BRUSH_HURT 32		 // rotating brush that inflicts pain based on rotation speed
#define SF_ROTATING_NOT_SOLID 64 // some special rotating objects are not solid.

// covering cheesy noise1, noise2, & noise3 fields so they make more sense (for rotating fans)
#define noiseStart noise1
#define noiseStop noise2
#define noiseRunning noise3

#define SF_PENDULUM_SWING 2 // spawnflag that makes a pendulum a rope swing.
#define SF_PENDULUM_AUTO_RETURN 16
#define SF_PENDULUM_PASSABLE 32

// =================== FUNC_WALL ==============================================

class CFuncWall : public CBaseEntity
{
public:
	CFuncWall(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	bool Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	// Bmodels don't go across transitions
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};

LINK_ENTITY_TO_CLASS(func_wall, CFuncWall);

bool CFuncWall::Spawn()
{
	v.angles = g_vecZero;
	v.movetype = MOVETYPE_PUSH; // so it doesn't get pushed by anything
	v.solid = SOLID_BSP;
	SetModel(v.model);

	// If it can't move/go away, it's really part of the world
	v.flags |= FL_WORLDBRUSH;

	return true;
}


void CFuncWall::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (ShouldToggle(useType, v.frame != 0))
		v.frame = 1 - v.frame;
}


#define SF_WALL_START_OFF 0x0001

class CFuncWallToggle : public CFuncWall
{
public:
	CFuncWallToggle(Entity* containingEntity) : CFuncWall(containingEntity) {}

	bool Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void TurnOff();
	void TurnOn();
	bool IsOn();
};

LINK_ENTITY_TO_CLASS(func_wall_toggle, CFuncWallToggle);

bool CFuncWallToggle::Spawn()
{
	if (!CFuncWall::Spawn())
	{
		return false;
	}

	if ((v.spawnflags & SF_WALL_START_OFF) != 0)
		TurnOff();

	return true;
}


void CFuncWallToggle::TurnOff()
{
	v.solid = SOLID_NOT;
	v.effects |= EF_NODRAW;
	SetOrigin(v.origin);
}


void CFuncWallToggle::TurnOn()
{
	v.solid = SOLID_BSP;
	v.effects &= ~EF_NODRAW;
	SetOrigin(v.origin);
}


bool CFuncWallToggle::IsOn()
{
	if (v.solid == SOLID_NOT)
		return false;
	return true;
}


void CFuncWallToggle::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	bool status = IsOn();

	if (ShouldToggle(useType, status))
	{
		if (status)
			TurnOff();
		else
			TurnOn();
	}
}


#define SF_CONVEYOR_VISUAL 0x0001
#define SF_CONVEYOR_NOTSOLID 0x0002

class CFuncConveyor : public CFuncWall
{
public:
	CFuncConveyor(Entity* containingEntity) : CFuncWall(containingEntity) {}

	bool Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void UpdateSpeed(float speed);
};

LINK_ENTITY_TO_CLASS(func_conveyor, CFuncConveyor);
bool CFuncConveyor::Spawn()
{
	v.movedir = util::SetMovedir(v.angles);

	if (!CFuncWall::Spawn())
	{
		return false;
	}

	if ((v.spawnflags & SF_CONVEYOR_VISUAL) == 0)
		SetBits(v.flags, FL_CONVEYOR);

	// HACKHACK - This is to allow for some special effects
	if ((v.spawnflags & SF_CONVEYOR_NOTSOLID) != 0)
	{
		v.solid = SOLID_NOT;
		v.skin = 0; // Don't want the engine thinking we've got special contents on this brush
	}

	if (v.speed == 0)
		v.speed = 100;

	UpdateSpeed(v.speed);

	return true;
}


// HACKHACK -- This is ugly, but encode the speed in the rendercolor to avoid adding more data to the network stream
void CFuncConveyor::UpdateSpeed(float speed)
{
	// Encode it as an integer with 4 fractional bits
	int speedCode = (int)(fabs(speed) * 16.0);

	if (speed < 0)
		v.rendercolor.x = 1;
	else
		v.rendercolor.x = 0;

	v.rendercolor.y = (speedCode >> 8);
	v.rendercolor.z = (speedCode & 0xFF);
}


void CFuncConveyor::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	v.speed = -v.speed;
	UpdateSpeed(v.speed);
}



// =================== FUNC_ILLUSIONARY ==============================================


class CFuncIllusionary : public CBaseToggle
{
public:
	CFuncIllusionary(Entity* containingEntity) : CBaseToggle(containingEntity) {}

	bool Spawn() override;
	void EXPORT SloshTouch(CBaseEntity* pOther);
	bool KeyValue(KeyValueData* pkvd) override;
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};

LINK_ENTITY_TO_CLASS(func_illusionary, CFuncIllusionary);

bool CFuncIllusionary::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "skin")) //skin is used for content type
	{
		v.skin = atof(pkvd->szValue);
		return true;
	}

	return CBaseToggle::KeyValue(pkvd);
}

bool CFuncIllusionary::Spawn()
{
	v.angles = g_vecZero;
	v.movetype = MOVETYPE_NONE;
	v.solid = SOLID_NOT; // always solid_not
	SetModel(v.model);

	// I'd rather eat the network bandwidth of this than figure out how to save/restore
	// these entities after they have been moved to the client, or respawn them ala Quake
	// Perhaps we can do this in deathmatch only.
	//	g_engfuncs.pfnMakeStatic(&v);
	return true;
}


// -------------------------------------------------------------------------------
//
// Monster only clip brush
//
// This brush will be solid for any entity who has the FL_MONSTERCLIP flag set
// in v.flags
//
// otherwise it will be invisible and not solid.  This can be used to keep
// specific monsters out of certain areas
//
// -------------------------------------------------------------------------------
class CFuncMonsterClip : public CFuncWall
{
public:
	CFuncMonsterClip(Entity* containingEntity) : CFuncWall(containingEntity) {}

	bool Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override {} // Clear out func_wall's use function
};

LINK_ENTITY_TO_CLASS(func_monsterclip, CFuncMonsterClip);

bool CFuncMonsterClip::Spawn()
{
	if (!CFuncWall::Spawn())
	{
		return false;
	}

	v.effects = EF_NODRAW;
	v.flags |= FL_MONSTERCLIP;

	return true;
}


// =================== FUNC_ROTATING ==============================================
class CFuncRotating : public CBaseEntity
{
public:
	CFuncRotating(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	DECLARE_SAVERESTORE()

	// basic functions
	bool Spawn() override;
	void Precache() override;
	void EXPORT SpinUp();
	void EXPORT SpinDown();
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT HurtTouch(CBaseEntity* pOther);
	void EXPORT RotatingUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT Rotate();
	void RampPitchVol(bool fUp);
	void Blocked(CBaseEntity* pOther) override;
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	float m_flFanFriction;
	float m_flAttenuation;
	float m_flVolume;
	float m_pitch;
	int m_sounds;
};

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CFuncRotating)
	DEFINE_FIELD(CFuncRotating, m_flFanFriction, FIELD_FLOAT),
	DEFINE_FIELD(CFuncRotating, m_flAttenuation, FIELD_FLOAT),
	DEFINE_FIELD(CFuncRotating, m_flVolume, FIELD_FLOAT),
	DEFINE_FIELD(CFuncRotating, m_pitch, FIELD_FLOAT),
	DEFINE_FIELD(CFuncRotating, m_sounds, FIELD_INTEGER),
END_SAVERESTORE(CFuncRotating, CBaseEntity)
#endif


LINK_ENTITY_TO_CLASS(func_rotating, CFuncRotating);

bool CFuncRotating::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "fanfriction"))
	{
		m_flFanFriction = atof(pkvd->szValue) / 100;
		return true;
	}
	else if (streq(pkvd->szKeyName, "Volume"))
	{
		m_flVolume = atof(pkvd->szValue) / 10.0;

		if (m_flVolume > 1.0)
			m_flVolume = 1.0;
		if (m_flVolume < 0.0)
			m_flVolume = 0.0;
		return true;
	}
	else if (streq(pkvd->szKeyName, "spawnorigin"))
	{
		Vector tmp;
		util::StringToVector(tmp, pkvd->szValue);
		if (tmp != g_vecZero)
			v.origin = tmp;
		return true;
	}
	else if (streq(pkvd->szKeyName, "sounds"))
	{
		m_sounds = atoi(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}


bool CFuncRotating::Spawn()
{
	// set final pitch.  Must not be PITCH_NORM, since we
	// plan on pitch shifting later.

	m_pitch = PITCH_NORM - 1;

	// maintain compatibility with previous maps
	if (m_flVolume == 0.0)
		m_flVolume = 1.0;

	// if the designer didn't set a sound attenuation, default to one.
	m_flAttenuation = ATTN_NORM;

	if (FBitSet(v.spawnflags, SF_BRUSH_ROTATE_SMALLRADIUS))
	{
		m_flAttenuation = ATTN_IDLE;
	}
	else if (FBitSet(v.spawnflags, SF_BRUSH_ROTATE_MEDIUMRADIUS))
	{
		m_flAttenuation = ATTN_STATIC;
	}
	else if (FBitSet(v.spawnflags, SF_BRUSH_ROTATE_LARGERADIUS))
	{
		m_flAttenuation = ATTN_NORM;
	}

	// prevent divide by zero if level designer forgets friction!
	if (m_flFanFriction == 0)
	{
		m_flFanFriction = 1;
	}

	if (FBitSet(v.spawnflags, SF_BRUSH_ROTATE_Z_AXIS))
		v.movedir = Vector(0, 0, 1);
	else if (FBitSet(v.spawnflags, SF_BRUSH_ROTATE_X_AXIS))
		v.movedir = Vector(1, 0, 0);
	else
		v.movedir = Vector(0, 1, 0); // y-axis

	// check for reverse rotation
	if (FBitSet(v.spawnflags, SF_BRUSH_ROTATE_BACKWARDS))
		v.movedir = v.movedir * -1;

	// some rotating objects like fake volumetric lights will not be solid.
	if (FBitSet(v.spawnflags, SF_ROTATING_NOT_SOLID))
	{
		v.solid = SOLID_NOT;
		v.skin = CONTENTS_EMPTY;
		v.movetype = MOVETYPE_PUSH;
	}
	else
	{
		v.solid = SOLID_BSP;
		v.movetype = MOVETYPE_PUSH;
	}

	SetOrigin(v.origin);
	SetModel(v.model);

	SetUse(&CFuncRotating::RotatingUse);
	// did level designer forget to assign speed?
	if (v.speed <= 0)
		v.speed = 0;

	// instant-use brush?
	if (FBitSet(v.spawnflags, SF_BRUSH_ROTATE_INSTANT))
	{
		SetThink(&CFuncRotating::SUB_CallUseToggle);
		v.nextthink = v.ltime + 1.5; // leave a magic delay for client to start up
	}
	// can this brush inflict pain?
	if (FBitSet(v.spawnflags, SF_BRUSH_HURT))
	{
		SetTouch(&CFuncRotating::HurtTouch);
	}

	Precache();

	return true;
}


void CFuncRotating::Precache()
{
	char* szSoundFile = (char*)STRING(v.message);

	// set up fan sounds

	if (!FStringNull(v.message) && strlen(szSoundFile) > 0)
	{
		// if a path is set for a wave, use it

		g_engfuncs.pfnPrecacheSound(szSoundFile);

		v.noiseRunning = g_engfuncs.pfnAllocString(szSoundFile);
	}
	else
	{
		// otherwise use preset sound
		switch (m_sounds)
		{
		case 1:
			g_engfuncs.pfnPrecacheSound("fans/fan1.wav");
			v.noiseRunning = g_engfuncs.pfnAllocString("fans/fan1.wav");
			break;
		case 2:
			g_engfuncs.pfnPrecacheSound("fans/fan2.wav");
			v.noiseRunning = g_engfuncs.pfnAllocString("fans/fan2.wav");
			break;
		case 3:
			g_engfuncs.pfnPrecacheSound("fans/fan3.wav");
			v.noiseRunning = g_engfuncs.pfnAllocString("fans/fan3.wav");
			break;
		case 4:
			g_engfuncs.pfnPrecacheSound("fans/fan4.wav");
			v.noiseRunning = g_engfuncs.pfnAllocString("fans/fan4.wav");
			break;
		case 5:
			g_engfuncs.pfnPrecacheSound("fans/fan5.wav");
			v.noiseRunning = g_engfuncs.pfnAllocString("fans/fan5.wav");
			break;

		case 0:
		default:
			if (!FStringNull(v.message) && strlen(szSoundFile) > 0)
			{
				g_engfuncs.pfnPrecacheSound(szSoundFile);

				v.noiseRunning = g_engfuncs.pfnAllocString(szSoundFile);
				break;
			}
			else
			{
				v.noiseRunning = g_engfuncs.pfnAllocString("common/null.wav");
				break;
			}
		}
	}

	if (v.avelocity != g_vecZero)
	{
		// if fan was spinning, and we went through transition or save/restore,
		// make sure we restart the sound.  1.5 sec delay is magic number. KDB

		SetThink(&CFuncRotating::SpinUp);
		v.nextthink = v.ltime + 1.5;
	}
}



//
// Touch - will hurt others based on how fast the brush is spinning
//
void CFuncRotating::HurtTouch(CBaseEntity* pOther)
{
	// we can't hurt this thing, so we're not concerned with it
	if (0 == pOther->v.takedamage)
		return;

	// calculate damage based on rotation speed
	v.dmg = v.avelocity.Length() / 10;

	pOther->TakeDamage(this, this, v.dmg, DMG_CRUSH);

	pOther->v.velocity = (pOther->v.origin - Center()).Normalize() * v.dmg;
}

//
// RampPitchVol - ramp pitch and volume up to final values, based on difference
// between how fast we're going vs how fast we plan to go
//
#define FANPITCHMIN 30
#define FANPITCHMAX 100

void CFuncRotating::RampPitchVol(bool fUp)
{

	Vector vecAVel = v.avelocity;
	vec_t vecCur;
	vec_t vecFinal;
	float fpct;
	float fvol;
	float fpitch;
	int pitch;

	// get current angular velocity

	vecCur = fabs(vecAVel.x != 0 ? vecAVel.x : (vecAVel.y != 0 ? vecAVel.y : vecAVel.z));

	// get target angular velocity

	vecFinal = (v.movedir.x != 0 ? v.movedir.x : (v.movedir.y != 0 ? v.movedir.y : v.movedir.z));
	vecFinal *= v.speed;
	vecFinal = fabs(vecFinal);

	// calc volume and pitch as % of final vol and pitch

	fpct = vecCur / vecFinal;
	//	if (fUp)
	//		fvol = m_flVolume * (0.5 + fpct/2.0); // spinup volume ramps up from 50% max vol
	//	else
	fvol = m_flVolume * fpct; // slowdown volume ramps down to 0

	fpitch = FANPITCHMIN + (FANPITCHMAX - FANPITCHMIN) * fpct;

	pitch = (int)fpitch;
	if (pitch == PITCH_NORM)
		pitch = PITCH_NORM - 1;

	// change the fan's vol and pitch

	EmitSound(
		STRING(v.noiseRunning),
		CHAN_STATIC,
		fvol,
		m_flAttenuation,
		pitch,
		SND_CHANGE_PITCH | SND_CHANGE_VOL);
}

//
// SpinUp - accelerates a non-moving func_rotating up to it's speed
//
void CFuncRotating::SpinUp()
{
	Vector vecAVel; //rotational velocity

	v.nextthink = v.ltime + 0.1;
	v.avelocity = v.avelocity + (v.movedir * (v.speed * m_flFanFriction));

	vecAVel = v.avelocity; // cache entity's rotational velocity

	// if we've met or exceeded target speed, set target speed and stop thinking
	if (fabs(vecAVel.x) >= fabs(v.movedir.x * v.speed) &&
		fabs(vecAVel.y) >= fabs(v.movedir.y * v.speed) &&
		fabs(vecAVel.z) >= fabs(v.movedir.z * v.speed))
	{
		v.avelocity = v.movedir * v.speed; // set speed in case we overshot

		EmitSound(
			STRING(v.noiseRunning),
			CHAN_STATIC,
			m_flVolume,
			m_flAttenuation,
			FANPITCHMAX,
			SND_CHANGE_PITCH | SND_CHANGE_VOL);

		SetThink(&CFuncRotating::Rotate);
		Rotate();
	}
	else
	{
		RampPitchVol(true);
	}
}

//
// SpinDown - decelerates a moving func_rotating to a standstill.
//
void CFuncRotating::SpinDown()
{
	Vector vecAVel; //rotational velocity
	vec_t vecdir;

	v.nextthink = v.ltime + 0.1;

	v.avelocity = v.avelocity - (v.movedir * (v.speed * m_flFanFriction)); //spin down slower than spinup

	vecAVel = v.avelocity; // cache entity's rotational velocity

	if (v.movedir.x != 0)
		vecdir = v.movedir.x;
	else if (v.movedir.y != 0)
		vecdir = v.movedir.y;
	else
		vecdir = v.movedir.z;

	// if we've met or exceeded target speed, set target speed and stop thinking
	// (note: must check for movedir > 0 or < 0)
	if (((vecdir > 0) && (vecAVel.x <= 0 && vecAVel.y <= 0 && vecAVel.z <= 0)) ||
		((vecdir < 0) && (vecAVel.x >= 0 && vecAVel.y >= 0 && vecAVel.z >= 0)))
	{
		v.avelocity = g_vecZero; // set speed in case we overshot

		// stop sound, we're done

		EmitSound(
			STRING(v.noiseRunning),
			CHAN_STATIC,
			VOL_NORM,
			m_flAttenuation,
			m_pitch,
			SND_STOP);

		SetThink(&CFuncRotating::Rotate);
		Rotate();
	}
	else
	{
		RampPitchVol(false);
	}
}

void CFuncRotating::Rotate()
{
	v.nextthink = v.ltime + 10;
}

//=========================================================
// Rotating Use - when a rotating brush is triggered
//=========================================================
void CFuncRotating::RotatingUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	// is this a brush that should accelerate and decelerate when turned on/off (fan)?
	if (FBitSet(v.spawnflags, SF_BRUSH_ACCDCC))
	{
		// fan is spinning, so stop it.
		if (v.avelocity != g_vecZero)
		{
			SetThink(&CFuncRotating::SpinDown);

			v.nextthink = v.ltime + 0.1;
		}
		else // fan is not moving, so start it
		{
			SetThink(&CFuncRotating::SpinUp);

			EmitSound(
				STRING(v.noiseRunning),
				CHAN_STATIC,
				0.01F,
				m_flAttenuation,
				FANPITCHMIN);

			v.nextthink = v.ltime + 0.1;
		}
	}
	else if (!FBitSet(v.spawnflags, SF_BRUSH_ACCDCC)) //this is a normal start/stop brush.
	{
		if (v.avelocity != g_vecZero)
		{
			// play stopping sound here
			SetThink(&CFuncRotating::SpinDown);

			v.nextthink = v.ltime + 0.1;
		}
		else
		{
			EmitSound(
				STRING(v.noiseRunning),
				CHAN_STATIC,
				m_flVolume,
				m_flAttenuation,
				FANPITCHMAX);

			v.avelocity = v.movedir * v.speed;

			SetThink(&CFuncRotating::Rotate);
			Rotate();
		}
	}
}


//
// RotatingBlocked - An entity has blocked the brush
//
void CFuncRotating::Blocked(CBaseEntity* pOther)

{
	pOther->TakeDamage(this, this, v.dmg, DMG_CRUSH);
}






//#endif


class CPendulum : public CBaseEntity
{
public:
	CPendulum(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	DECLARE_SAVERESTORE()

	bool Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT Swing();
	void EXPORT PendulumUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT Stop();
	void Touch(CBaseEntity* pOther) override;
	void EXPORT RopeTouch(CBaseEntity* pOther); // this touch func makes the pendulum a rope
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void Blocked(CBaseEntity* pOther) override;

	float m_accel;	  // Acceleration
	float m_distance; //
	float m_time;
	float m_damp;
	float m_maxSpeed;
	float m_dampSpeed;
	Vector m_center;
	Vector m_start;
};

LINK_ENTITY_TO_CLASS(func_pendulum, CPendulum);

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CPendulum)
	DEFINE_FIELD(CPendulum, m_accel, FIELD_FLOAT),
	DEFINE_FIELD(CPendulum, m_distance, FIELD_FLOAT),
	DEFINE_FIELD(CPendulum, m_time, FIELD_TIME),
	DEFINE_FIELD(CPendulum, m_damp, FIELD_FLOAT),
	DEFINE_FIELD(CPendulum, m_maxSpeed, FIELD_FLOAT),
	DEFINE_FIELD(CPendulum, m_dampSpeed, FIELD_FLOAT),
	DEFINE_FIELD(CPendulum, m_center, FIELD_VECTOR),
	DEFINE_FIELD(CPendulum, m_start, FIELD_VECTOR),
END_SAVERESTORE(CPendulum, CBaseEntity)
#endif



bool CPendulum::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "distance"))
	{
		m_distance = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "damp"))
	{
		m_damp = atof(pkvd->szValue) * 0.001;
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}


bool CPendulum::Spawn()
{
	// set the axis of rotation
	CBaseToggle::AxisDir(this);

	if (FBitSet(v.spawnflags, SF_DOOR_PASSABLE))
		v.solid = SOLID_NOT;
	else
		v.solid = SOLID_BSP;
	v.movetype = MOVETYPE_PUSH;
	SetOrigin(v.origin);
	SetModel(v.model);

	if (m_distance == 0)
		return;

	if (v.speed == 0)
		v.speed = 100;

	m_accel = (v.speed * v.speed) / (2 * fabs(m_distance)); // Calculate constant acceleration from speed and distance
	m_maxSpeed = v.speed;
	m_start = v.angles;
	m_center = v.angles + (m_distance * 0.5) * v.movedir;

	if (FBitSet(v.spawnflags, SF_BRUSH_ROTATE_INSTANT))
	{
		SetThink(&CPendulum::SUB_CallUseToggle);
		v.nextthink = gpGlobals->time + 0.1;
	}
	v.speed = 0;
	SetUse(&CPendulum::PendulumUse);

	if (FBitSet(v.spawnflags, SF_PENDULUM_SWING))
	{
		SetTouch(&CPendulum::RopeTouch);
	}

	return true;
}


void CPendulum::PendulumUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (0 != v.speed) // Pendulum is moving, stop it and auto-return if necessary
	{
		if (FBitSet(v.spawnflags, SF_PENDULUM_AUTO_RETURN))
		{
			float delta;

			delta = CBaseToggle::AxisDelta(v.spawnflags, v.angles, m_start);

			v.avelocity = m_maxSpeed * v.movedir;
			v.nextthink = v.ltime + (delta / m_maxSpeed);
			SetThink(&CPendulum::Stop);
		}
		else
		{
			v.speed = 0; // Dead stop
			ClearThink();
			v.avelocity = g_vecZero;
		}
	}
	else
	{
		v.nextthink = v.ltime + 0.1; // Start the pendulum moving
		m_time = gpGlobals->time;		   // Save time to calculate dt
		SetThink(&CPendulum::Swing);
		m_dampSpeed = m_maxSpeed;
	}
}


void CPendulum::Stop()
{
	v.angles = m_start;
	v.speed = 0;
	ClearThink();
	v.avelocity = g_vecZero;
}


void CPendulum::Blocked(CBaseEntity* pOther)
{
	m_time = gpGlobals->time;
}


void CPendulum::Swing()
{
	float delta, dt;

	delta = CBaseToggle::AxisDelta(v.spawnflags, v.angles, m_center);
	dt = gpGlobals->time - m_time; // How much time has passed?
	m_time = gpGlobals->time;	   // Remember the last time called

	if (delta > 0 && m_accel > 0)
		v.speed -= m_accel * dt; // Integrate velocity
	else
		v.speed += m_accel * dt;

	if (v.speed > m_maxSpeed)
		v.speed = m_maxSpeed;
	else if (v.speed < -m_maxSpeed)
		v.speed = -m_maxSpeed;
	// scale the destdelta vector by the time spent traveling to get velocity
	v.avelocity = v.speed * v.movedir;

	// Call this again
	v.nextthink = v.ltime + 0.1;

	if (0 != m_damp)
	{
		m_dampSpeed -= m_damp * m_dampSpeed * dt;
		if (m_dampSpeed < 30.0)
		{
			v.angles = m_center;
			v.speed = 0;
			ClearThink();
			v.avelocity = g_vecZero;
		}
		else if (v.speed > m_dampSpeed)
			v.speed = m_dampSpeed;
		else if (v.speed < -m_dampSpeed)
			v.speed = -m_dampSpeed;
	}
}


void CPendulum::Touch(CBaseEntity* pOther)
{
	if (v.dmg <= 0)
		return;

	// we can't hurt this thing, so we're not concerned with it
	if (0 == pOther->v.takedamage)
		return;

	// calculate damage based on rotation speed
	float damage = v.dmg * v.speed * 0.01;

	if (damage < 0)
		damage = -damage;

	pOther->TakeDamage(this, this, damage, DMG_CRUSH);

	pOther->v.velocity = (pOther->v.origin - Center()).Normalize() * damage;
}

void CPendulum::RopeTouch(CBaseEntity* pOther)
{
	if (!pOther->IsPlayer())
	{ // not a player!
		g_engfuncs.pfnAlertMessage(at_console, "Not a client\n");
		return;
	}

	if (&pOther->v == v.enemy)
	{ // this player already on the rope.
		return;
	}

	v.enemy = pOther->edict();
	pOther->v.velocity = g_vecZero;
	pOther->v.movetype = MOVETYPE_NONE;
}
