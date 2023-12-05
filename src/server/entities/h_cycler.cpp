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

===== h_cycler.cpp ========================================================

  The Halflife Cycler Monsters

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "animation.h"
#include "weapons.h"
#include "player.h"

class CCycler : public CBaseAnimating
{
public:
	DECLARE_SAVERESTORE()

	bool GenericCyclerSpawn(const char* szModel, Vector vecMin, Vector vecMax);
	int ObjectCaps() override { return (CBaseEntity::ObjectCaps() | FCAP_IMPULSE_USE); }
	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;
	bool Spawn() override;
	void Think() override;
	//void Pain( float flDamage );
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	// Don't treat as a live target
	bool IsAlive() override { return false; }

	bool m_animate;
};

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CCycler)
	DEFINE_FIELD(CCycler, m_animate, FIELD_BOOLEAN),
END_SAVERESTORE(CCycler, CBaseAnimating)
#endif


//
// we should get rid of all the other cyclers and replace them with this.
//
class CGenericCycler : public CCycler
{
public:
	bool Spawn() override { return GenericCyclerSpawn(STRING(pev->model), Vector(-16, -16, 0), Vector(16, 16, 72)); }
};
LINK_ENTITY_TO_CLASS(cycler, CGenericCycler);


// Cycler member functions

bool CCycler::GenericCyclerSpawn(const char* szModel, Vector vecMin, Vector vecMax)
{
	if (!szModel || '\0' == *szModel)
	{
		ALERT(at_error, "cycler at %.0f %.0f %0.f missing modelname", pev->origin.x, pev->origin.y, pev->origin.z);
		return false;
	}

	pev->classname = MAKE_STRING("cycler");
	PRECACHE_MODEL(szModel);
	SetModel(szModel);

	if (!CCycler::Spawn())
	{
		return false;
	}

	SetSize(vecMin, vecMax);

	return true;
}


bool CCycler::Spawn()
{
	InitBoneControllers();
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_YES;
	pev->effects = 0;
	pev->health = 80000; // no cycler should die
	pev->yaw_speed = 5;
	pev->ideal_yaw = pev->angles.y;
	// ChangeYaw(360);

	m_flFrameRate = 75;
	m_flGroundSpeed = 0;

	pev->nextthink += 1.0;

	ResetSequenceInfo();

	if (pev->sequence != 0 || pev->frame != 0)
	{
		m_animate = false;
		pev->framerate = 0;
	}
	else
	{
		m_animate = true;
	}

	return true;
}




//
// cycler think
//
void CCycler::Think()
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (m_animate)
	{
		StudioFrameAdvance();
	}
	if (m_fSequenceFinished && !m_fSequenceLoops)
	{
		// ResetSequenceInfo();
		// hack to avoid reloading model every frame
		pev->animtime = gpGlobals->time;
		pev->framerate = 1.0;
		m_fSequenceFinished = false;
		m_flLastEventCheck = gpGlobals->time;
		pev->frame = 0;
		if (!m_animate)
			pev->framerate = 0.0; // FIX: don't reset framerate
	}
}

//
// CyclerUse - starts a rotation trend
//
void CCycler::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	m_animate = !m_animate;
	if (m_animate)
		pev->framerate = 1.0;
	else
		pev->framerate = 0.0;
}

//
// CyclerPain , changes sequences when shot
//
bool CCycler::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	if (m_animate)
	{
		pev->sequence++;

		ResetSequenceInfo();

		if (m_flFrameRate == 0.0)
		{
			pev->sequence = 0;
			ResetSequenceInfo();
		}
		pev->frame = 0;
	}
	else
	{
		pev->framerate = 1.0;
		StudioFrameAdvance(0.1);
		pev->framerate = 0;
		ALERT(at_console, "sequence: %d, frame %.0f\n", pev->sequence, pev->frame);
	}

	return false;
}

class CCyclerSprite : public CBaseEntity
{
public:
	DECLARE_SAVERESTORE()

	bool Spawn() override;
	void Think() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	int ObjectCaps() override { return (CBaseEntity::ObjectCaps() | FCAP_IMPULSE_USE); }
	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;
	void Animate(float frames);

	inline bool ShouldAnimate() { return m_animate && m_maxFrame > 1.0; }
	bool m_animate;
	float m_lastTime;
	float m_maxFrame;
};

LINK_ENTITY_TO_CLASS(cycler_sprite, CCyclerSprite);

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CCyclerSprite)
	DEFINE_FIELD(CCyclerSprite, m_animate, FIELD_BOOLEAN),
	DEFINE_FIELD(CCyclerSprite, m_lastTime, FIELD_TIME),
	DEFINE_FIELD(CCyclerSprite, m_maxFrame, FIELD_FLOAT),
END_SAVERESTORE(CCyclerSprite, CBaseEntity)
#endif


bool CCyclerSprite::Spawn()
{
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_YES;
	pev->effects = 0;

	pev->frame = 0;
	pev->nextthink = gpGlobals->time + 0.1;
	m_animate = true;
	m_lastTime = gpGlobals->time;

	PRECACHE_MODEL((char*)STRING(pev->model));
	SetModel(STRING(pev->model));

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;

	return true;
}


void CCyclerSprite::Think()
{
	if (ShouldAnimate())
		Animate(pev->framerate * (gpGlobals->time - m_lastTime));

	pev->nextthink = gpGlobals->time + 0.1;
	m_lastTime = gpGlobals->time;
}


void CCyclerSprite::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	m_animate = !m_animate;
	ALERT(at_console, "Sprite: %s\n", STRING(pev->model));
}


bool CCyclerSprite::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	if (m_maxFrame > 1.0)
	{
		Animate(1.0);
	}
	return true;
}

void CCyclerSprite::Animate(float frames)
{
	pev->frame += frames;
	if (m_maxFrame > 0)
		pev->frame = fmod(pev->frame, m_maxFrame);
}
