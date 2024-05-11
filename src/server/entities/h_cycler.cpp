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
	CCycler(Entity* containingEntity) : CBaseAnimating(containingEntity) {}

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
	CGenericCycler(Entity* containingEntity) : CCycler(containingEntity) {}

	bool Spawn() override { return GenericCyclerSpawn(STRING(v.model), Vector(-16, -16, 0), Vector(16, 16, 72)); }
};
LINK_ENTITY_TO_CLASS(cycler, CGenericCycler);


// Cycler member functions

bool CCycler::GenericCyclerSpawn(const char* szModel, Vector vecMin, Vector vecMax)
{
	if (!szModel || '\0' == *szModel)
	{
		ALERT(at_error, "cycler at %.0f %.0f %0.f missing modelname", v.origin.x, v.origin.y, v.origin.z);
		return false;
	}

	v.classname = MAKE_STRING("cycler");
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
	v.solid = SOLID_SLIDEBOX;
	v.movetype = MOVETYPE_NONE;
	v.takedamage = DAMAGE_YES;
	v.effects = 0;
	v.health = 80000; // no cycler should die
	v.yaw_speed = 5;
	v.ideal_yaw = v.angles.y;
	// ChangeYaw(360);

	m_flFrameRate = 75;
	m_flGroundSpeed = 0;

	v.nextthink += 1.0;

	ResetSequenceInfo();

	if (v.sequence != 0 || v.frame != 0)
	{
		m_animate = false;
		v.framerate = 0;
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
	v.nextthink = gpGlobals->time + 0.1;

	if (m_animate)
	{
		StudioFrameAdvance();
	}
	if (m_fSequenceFinished && !m_fSequenceLoops)
	{
		// ResetSequenceInfo();
		// hack to avoid reloading model every frame
		v.animtime = gpGlobals->time;
		v.framerate = 1.0;
		m_fSequenceFinished = false;
		m_flLastEventCheck = gpGlobals->time;
		v.frame = 0;
		if (!m_animate)
			v.framerate = 0.0; // FIX: don't reset framerate
	}
}

//
// CyclerUse - starts a rotation trend
//
void CCycler::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	m_animate = !m_animate;
	if (m_animate)
		v.framerate = 1.0;
	else
		v.framerate = 0.0;
}

//
// CyclerPain , changes sequences when shot
//
bool CCycler::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	if (m_animate)
	{
		v.sequence++;

		ResetSequenceInfo();

		if (m_flFrameRate == 0.0)
		{
			v.sequence = 0;
			ResetSequenceInfo();
		}
		v.frame = 0;
	}
	else
	{
		v.framerate = 1.0;
		StudioFrameAdvance(0.1);
		v.framerate = 0;
		ALERT(at_console, "sequence: %d, frame %.0f\n", v.sequence, v.frame);
	}

	return false;
}

class CCyclerSprite : public CBaseEntity
{
public:
	CCyclerSprite(Entity* containingEntity) : CBaseEntity(containingEntity) {}

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
	v.solid = SOLID_SLIDEBOX;
	v.movetype = MOVETYPE_NONE;
	v.takedamage = DAMAGE_YES;
	v.effects = 0;

	v.frame = 0;
	v.nextthink = gpGlobals->time + 0.1;
	m_animate = true;
	m_lastTime = gpGlobals->time;

	PRECACHE_MODEL((char*)STRING(v.model));
	SetModel(STRING(v.model));

	m_maxFrame = (float)MODEL_FRAMES(v.modelindex) - 1;

	return true;
}


void CCyclerSprite::Think()
{
	if (ShouldAnimate())
		Animate(v.framerate * (gpGlobals->time - m_lastTime));

	v.nextthink = gpGlobals->time + 0.1;
	m_lastTime = gpGlobals->time;
}


void CCyclerSprite::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	m_animate = !m_animate;
	ALERT(at_console, "Sprite: %s\n", STRING(v.model));
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
	v.frame += frames;
	if (m_maxFrame > 0)
		v.frame = fmod(v.frame, m_maxFrame);
}
