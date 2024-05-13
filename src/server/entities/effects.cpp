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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "customentity.h"
#include "effects.h"
#include "weapons.h"
#include "func_break.h"
#include "shake.h"

#define SF_GIBSHOOTER_REPEATABLE 1 // allows a gibshooter to be refired

#define SF_FUNNEL_REVERSE 1 // funnel effect repels particles instead of attracting them.


// Lightning target, just alias landmark
LINK_ENTITY_TO_CLASS(info_target, CPointEntity);


class CBubbling : public CBaseEntity
{
public:
	CBubbling(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	DECLARE_SAVERESTORE()

	bool Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;

	void EXPORT FizzThink();
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	int m_density;
	int m_frequency;
	int m_bubbleModel;
	bool m_state;
};

LINK_ENTITY_TO_CLASS(env_bubbles, CBubbling);

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CBubbling)
	DEFINE_FIELD(CBubbling, m_density, FIELD_INTEGER),
	DEFINE_FIELD(CBubbling, m_frequency, FIELD_INTEGER),
	DEFINE_FIELD(CBubbling, m_state, FIELD_BOOLEAN),
END_SAVERESTORE(CBubbling, CBaseEntity)
#endif


#define SF_BUBBLES_STARTOFF 0x0001

bool CBubbling::Spawn()
{
	Precache();
	SetModel(v.model); // Set size

	v.solid = SOLID_NOT; // Remove model & collisions
	v.renderamt = 0;		// The engine won't draw this model if this is set to 0 and blending is on
	v.rendermode = kRenderTransTexture;
	int speed = v.speed > 0 ? v.speed : -v.speed;

	// HACKHACK!!! - Speed in rendercolor
	v.rendercolor.x = speed >> 8;
	v.rendercolor.y = speed & 255;
	v.rendercolor.z = (v.speed < 0) ? 1 : 0;


	if ((v.spawnflags & SF_BUBBLES_STARTOFF) == 0)
	{
		SetThink(&CBubbling::FizzThink);
		v.nextthink = gpGlobals->time + 2.0;
		m_state = true;
	}
	else
		m_state = false;

	return true;
}

void CBubbling::Precache()
{
	m_bubbleModel = PRECACHE_MODEL("sprites/bubble.spr"); // Precache bubble sprite
}


void CBubbling::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (ShouldToggle(useType, m_state))
		m_state = !m_state;

	if (m_state)
	{
		SetThink(&CBubbling::FizzThink);
		v.nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		ClearThink();
		v.nextthink = 0;
	}
}


bool CBubbling::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "density"))
	{
		m_density = atoi(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "frequency"))
	{
		m_frequency = atoi(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "current"))
	{
		v.speed = atoi(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}


void CBubbling::FizzThink()
{
	MessageBegin(MSG_PAS, SVC_TEMPENTITY, Center());
	WriteByte(TE_FIZZ);
	WriteShort((short)ENTINDEX(edict()));
	WriteShort((short)m_bubbleModel);
	WriteByte(m_density);
	MessageEnd();

	if (m_frequency > 19)
		v.nextthink = gpGlobals->time + 0.5;
	else
		v.nextthink = gpGlobals->time + 2.5 - (0.1 * m_frequency);
}

// --------------------------------------------------
//
// Beams
//
// --------------------------------------------------

bool CBeam::Spawn()
{
	v.solid = SOLID_NOT; // Remove model & collisions
	Precache();

	return true;
}

void CBeam::Precache()
{
	if (v.owner)
		SetStartEntity(ENTINDEX(v.owner));
	if (v.aiment)
		SetEndEntity(ENTINDEX(v.aiment));
}

void CBeam::SetStartEntity(int entityIndex)
{
	v.sequence = (entityIndex & 0x0FFF) | (v.sequence & 0xF000);
	v.owner = g_engfuncs.pfnPEntityOfEntIndex(entityIndex);
}

void CBeam::SetEndEntity(int entityIndex)
{
	v.skin = (entityIndex & 0x0FFF) | (v.skin & 0xF000);
	v.aiment = g_engfuncs.pfnPEntityOfEntIndex(entityIndex);
}


// These don't take attachments into account
const Vector& CBeam::GetStartPos()
{
	if (GetType() == BEAM_ENTS)
	{
		Entity* pent = g_engfuncs.pfnPEntityOfEntIndex(GetStartEntity());
		return pent->origin;
	}
	return v.origin;
}


const Vector& CBeam::GetEndPos()
{
	int type = GetType();
	if (type == BEAM_POINTS || type == BEAM_HOSE)
	{
		return v.angles;
	}

	Entity* pent = g_engfuncs.pfnPEntityOfEntIndex(GetEndEntity());
	if (pent)
		return pent->origin;
	return v.angles;
}


CBeam* CBeam::BeamCreate(const char* pSpriteName, int width)
{
	// Create a new entity with CBeam private data
	CBeam* pBeam = Entity::Create<CBeam>();
	pBeam->v.classname = MAKE_STRING("beam");

	pBeam->BeamInit(pSpriteName, width);

	return pBeam;
}


void CBeam::BeamInit(const char* pSpriteName, int width)
{
	v.flags |= FL_CUSTOMENTITY;
	SetColor(255, 255, 255);
	SetBrightness(255);
	SetNoise(0);
	SetFrame(0);
	SetScrollRate(0);
	v.model = MAKE_STRING(pSpriteName);
	SetTexture(PRECACHE_MODEL((char*)pSpriteName));
	SetWidth(width);
	v.skin = 0;
	v.sequence = 0;
	v.rendermode = 0;
}


void CBeam::PointsInit(const Vector& start, const Vector& end)
{
	SetType(BEAM_POINTS);
	SetStartPos(start);
	SetEndPos(end);
	SetStartAttachment(0);
	SetEndAttachment(0);
	RelinkBeam();
}


void CBeam::HoseInit(const Vector& start, const Vector& direction)
{
	SetType(BEAM_HOSE);
	SetStartPos(start);
	SetEndPos(direction);
	SetStartAttachment(0);
	SetEndAttachment(0);
	RelinkBeam();
}


void CBeam::PointEntInit(const Vector& start, int endIndex)
{
	SetType(BEAM_ENTPOINT);
	SetStartPos(start);
	SetEndEntity(endIndex);
	SetStartAttachment(0);
	SetEndAttachment(0);
	RelinkBeam();
}

void CBeam::EntsInit(int startIndex, int endIndex)
{
	SetType(BEAM_ENTS);
	SetStartEntity(startIndex);
	SetEndEntity(endIndex);
	SetStartAttachment(0);
	SetEndAttachment(0);
	RelinkBeam();
}


void CBeam::RelinkBeam()
{
	const Vector &startPos = GetStartPos(), &endPos = GetEndPos();

	v.mins.x = std::min(startPos.x, endPos.x);
	v.mins.y = std::min(startPos.y, endPos.y);
	v.mins.z = std::min(startPos.z, endPos.z);
	v.maxs.x = std::max(startPos.x, endPos.x);
	v.maxs.y = std::max(startPos.y, endPos.y);
	v.maxs.z = std::max(startPos.z, endPos.z);
	v.mins = v.mins - v.origin;
	v.maxs = v.maxs - v.origin;

	SetSize(v.mins, v.maxs);
	SetOrigin(v.origin);
}

#if 0
void CBeam::SetObjectCollisionBox()
{
	const Vector &startPos = GetStartPos(), &endPos = GetEndPos();

	v.absmin.x = std::min( startPos.x, endPos.x );
	v.absmin.y = std::min( startPos.y, endPos.y );
	v.absmin.z = std::min( startPos.z, endPos.z );
	v.absmax.x = std::max( startPos.x, endPos.x );
	v.absmax.y = std::max( startPos.y, endPos.y );
	v.absmax.z = std::max( startPos.z, endPos.z );
}
#endif


void CBeam::TriggerTouch(CBaseEntity* pOther)
{
	if ((pOther->v.flags & (FL_CLIENT | FL_MONSTER)) != 0)
	{
		if (v.owner)
		{
			CBaseEntity* pOwner = v.owner->Get<CBaseEntity>();
			pOwner->Use(pOther, this, USE_TOGGLE, 0);
		}
		ALERT(at_console, "Firing targets!!!\n");
	}
}


CBaseEntity* CBeam::RandomTargetname(const char* szName)
{
	int total = 0;

	CBaseEntity* pEntity = nullptr;
	CBaseEntity* pNewEntity = nullptr;
	while ((pNewEntity = util::FindEntityByTargetname(pNewEntity, szName)) != nullptr)
	{
		total++;
		if (RANDOM_LONG(0, total - 1) < 1)
			pEntity = pNewEntity;
	}
	return pEntity;
}


void CBeam::DoSparks(const Vector& start, const Vector& end)
{
	if ((v.spawnflags & (SF_BEAM_SPARKSTART | SF_BEAM_SPARKEND)) != 0)
	{
		if ((v.spawnflags & SF_BEAM_SPARKSTART) != 0)
		{
			tent::Sparks(start);
		}
		if ((v.spawnflags & SF_BEAM_SPARKEND) != 0)
		{
			tent::Sparks(end);
		}
	}
}


class CLightning : public CBeam
{
public:
	CLightning(Entity* containingEntity) : CBeam(containingEntity) {}

	DECLARE_SAVERESTORE()

	bool Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Activate() override;

	void EXPORT StrikeThink();
	void EXPORT DamageThink();
	void RandomArea();
	void RandomPoint(Vector& vecSrc);
	void Zap(const Vector& vecSrc, const Vector& vecDest);
	void EXPORT StrikeUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	inline bool ServerSide()
	{
		if (m_life == 0 && (v.spawnflags & SF_BEAM_RING) == 0)
			return true;
		return false;
	}

	void BeamUpdateVars();

	bool m_active;
	int m_iszStartEntity;
	int m_iszEndEntity;
	float m_life;
	int m_boltWidth;
	int m_noiseAmplitude;
	int m_brightness;
	int m_speed;
	float m_restrike;
	int m_spriteTexture;
	int m_iszSpriteName;
	int m_frameStart;

	float m_radius;
};

LINK_ENTITY_TO_CLASS(env_beam, CLightning);


#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CLightning)
	DEFINE_FIELD(CLightning, m_active, FIELD_BOOLEAN),
	DEFINE_FIELD(CLightning, m_iszStartEntity, FIELD_STRING),
	DEFINE_FIELD(CLightning, m_iszEndEntity, FIELD_STRING),
	DEFINE_FIELD(CLightning, m_life, FIELD_FLOAT),
	DEFINE_FIELD(CLightning, m_boltWidth, FIELD_INTEGER),
	DEFINE_FIELD(CLightning, m_noiseAmplitude, FIELD_INTEGER),
	DEFINE_FIELD(CLightning, m_brightness, FIELD_INTEGER),
	DEFINE_FIELD(CLightning, m_speed, FIELD_INTEGER),
	DEFINE_FIELD(CLightning, m_restrike, FIELD_FLOAT),
	DEFINE_FIELD(CLightning, m_spriteTexture, FIELD_INTEGER),
	DEFINE_FIELD(CLightning, m_iszSpriteName, FIELD_STRING),
	DEFINE_FIELD(CLightning, m_frameStart, FIELD_INTEGER),
	DEFINE_FIELD(CLightning, m_radius, FIELD_FLOAT),
END_SAVERESTORE(CLightning, CBeam)
#endif


bool CLightning::Spawn()
{
	if (FStringNull(m_iszSpriteName))
	{
		return false;
	}
	v.solid = SOLID_NOT; // Remove model & collisions
	Precache();

	v.dmgtime = gpGlobals->time;

	if (ServerSide())
	{
		ClearThink();
		if (v.dmg > 0)
		{
			SetThink(&CLightning::DamageThink);
			v.nextthink = gpGlobals->time + 0.1;
		}
		if (!FStringNull(v.targetname))
		{
			if ((v.spawnflags & SF_BEAM_STARTON) == 0)
			{
				v.effects = EF_NODRAW;
				m_active = false;
				v.nextthink = 0;
			}
			else
				m_active = true;

			SetUse(&CLightning::ToggleUse);
		}
	}
	else
	{
		m_active = false;
		if (!FStringNull(v.targetname))
		{
			SetUse(&CLightning::StrikeUse);
		}
		if (FStringNull(v.targetname) || FBitSet(v.spawnflags, SF_BEAM_STARTON))
		{
			SetThink(&CLightning::StrikeThink);
			v.nextthink = gpGlobals->time + 1.0;
		}
	}

	return true;
}

void CLightning::Precache()
{
	m_spriteTexture = PRECACHE_MODEL((char*)STRING(m_iszSpriteName));
	CBeam::Precache();
}


void CLightning::Activate()
{
	if (ServerSide())
		BeamUpdateVars();
}


bool CLightning::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "LightningStart"))
	{
		m_iszStartEntity = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "LightningEnd"))
	{
		m_iszEndEntity = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "life"))
	{
		m_life = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "BoltWidth"))
	{
		m_boltWidth = atoi(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "NoiseAmplitude"))
	{
		m_noiseAmplitude = atoi(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "TextureScroll"))
	{
		m_speed = atoi(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "StrikeTime"))
	{
		m_restrike = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "texture"))
	{
		m_iszSpriteName = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "framestart"))
	{
		m_frameStart = atoi(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "Radius"))
	{
		m_radius = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "damage"))
	{
		v.dmg = atof(pkvd->szValue);
		return true;
	}

	return CBeam::KeyValue(pkvd);
}


void CLightning::ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!ShouldToggle(useType, m_active))
		return;
	if (m_active)
	{
		m_active = false;
		v.effects |= EF_NODRAW;
		v.nextthink = 0;
	}
	else
	{
		m_active = true;
		v.effects &= ~EF_NODRAW;
		DoSparks(GetStartPos(), GetEndPos());
		if (v.dmg > 0)
		{
			v.nextthink = gpGlobals->time;
			v.dmgtime = gpGlobals->time;
		}
	}
}


void CLightning::StrikeUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!ShouldToggle(useType, m_active))
		return;

	if (m_active)
	{
		m_active = false;
		ClearThink();
	}
	else
	{
		SetThink(&CLightning::StrikeThink);
		v.nextthink = gpGlobals->time + 0.1;
	}

	if (!FBitSet(v.spawnflags, SF_BEAM_TOGGLE))
		ClearUse();
}


bool IsPointEntity(CBaseEntity* pEnt)
{
	if (0 == pEnt->v.modelindex)
		return true;
	if (pEnt->Is(CBaseEntity::Type::Point))
		return true;

	return false;
}


void CLightning::StrikeThink()
{
	if (m_life != 0)
	{
		if ((v.spawnflags & SF_BEAM_RANDOM) != 0)
			v.nextthink = gpGlobals->time + m_life + RANDOM_FLOAT(0, m_restrike);
		else
			v.nextthink = gpGlobals->time + m_life + m_restrike;
	}
	m_active = true;

	if (FStringNull(m_iszEndEntity))
	{
		if (FStringNull(m_iszStartEntity))
		{
			RandomArea();
		}
		else
		{
			CBaseEntity* pStart = RandomTargetname(STRING(m_iszStartEntity));
			if (pStart != nullptr)
				RandomPoint(pStart->v.origin);
			else
				ALERT(at_console, "env_beam: unknown entity \"%s\"\n", STRING(m_iszStartEntity));
		}
		return;
	}

	CBaseEntity* pStart = RandomTargetname(STRING(m_iszStartEntity));
	CBaseEntity* pEnd = RandomTargetname(STRING(m_iszEndEntity));

	if (pStart != nullptr && pEnd != nullptr)
	{
		if (IsPointEntity(pStart) || IsPointEntity(pEnd))
		{
			if ((v.spawnflags & SF_BEAM_RING) != 0)
			{
				// don't work
				return;
			}
		}

		MessageBegin(MSG_BROADCAST, SVC_TEMPENTITY);
		if (IsPointEntity(pStart) || IsPointEntity(pEnd))
		{
			if (!IsPointEntity(pEnd)) // One point entity must be in pEnd
			{
				CBaseEntity* pTemp;
				pTemp = pStart;
				pStart = pEnd;
				pEnd = pTemp;
			}
			if (!IsPointEntity(pStart)) // One sided
			{
				WriteByte(TE_BEAMENTPOINT);
				WriteShort(pStart->entindex());
				WriteCoord(pEnd->v.origin.x);
				WriteCoord(pEnd->v.origin.y);
				WriteCoord(pEnd->v.origin.z);
			}
			else
			{
				WriteByte(TE_BEAMPOINTS);
				WriteCoord(pStart->v.origin.x);
				WriteCoord(pStart->v.origin.y);
				WriteCoord(pStart->v.origin.z);
				WriteCoord(pEnd->v.origin.x);
				WriteCoord(pEnd->v.origin.y);
				WriteCoord(pEnd->v.origin.z);
			}
		}
		else
		{
			if ((v.spawnflags & SF_BEAM_RING) != 0)
				WriteByte(TE_BEAMRING);
			else
				WriteByte(TE_BEAMENTS);
			WriteShort(pStart->entindex());
			WriteShort(pEnd->entindex());
		}

		WriteShort(m_spriteTexture);
		WriteByte(m_frameStart);			 // framestart
		WriteByte((int)v.framerate);	 // framerate
		WriteByte((int)(m_life * 10.0));	 // life
		WriteByte(m_boltWidth);			 // width
		WriteByte(m_noiseAmplitude);		 // noise
		WriteByte((int)v.rendercolor.x); // r, g, b
		WriteByte((int)v.rendercolor.y); // r, g, b
		WriteByte((int)v.rendercolor.z); // r, g, b
		WriteByte(v.renderamt);			 // brightness
		WriteByte(m_speed);				 // speed
		MessageEnd();
		DoSparks(pStart->v.origin, pEnd->v.origin);
		if (v.dmg > 0)
		{
			TraceResult tr;
			util::TraceLine(pStart->v.origin, pEnd->v.origin, util::dont_ignore_monsters, nullptr, &tr);
			BeamDamageInstant(&tr, v.dmg);
		}
	}
}


void CBeam::BeamDamage(TraceResult* ptr)
{
	RelinkBeam();
	if (ptr->flFraction != 1.0 && ptr->pHit != nullptr)
	{
		CBaseEntity* pHit = ptr->pHit->Get<CBaseEntity>();
		if (pHit)
		{
			pHit->TraceAttack(this, v.dmg * (gpGlobals->time - v.dmgtime), (ptr->vecEndPos - v.origin).Normalize(), ptr->iHitgroup, DMG_ENERGYBEAM);
			pHit->ApplyMultiDamage(this, this);
		}
	}
	v.dmgtime = gpGlobals->time;
}


void CLightning::DamageThink()
{
	v.nextthink = gpGlobals->time + 0.1;
	TraceResult tr;
	util::TraceLine(GetStartPos(), GetEndPos(), util::dont_ignore_monsters, nullptr, &tr);
	BeamDamage(&tr);
}



void CLightning::Zap(const Vector& vecSrc, const Vector& vecDest)
{
#if 1
	MessageBegin(MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte(TE_BEAMPOINTS);
	WriteCoord(vecSrc.x);
	WriteCoord(vecSrc.y);
	WriteCoord(vecSrc.z);
	WriteCoord(vecDest.x);
	WriteCoord(vecDest.y);
	WriteCoord(vecDest.z);
	WriteShort(m_spriteTexture);
	WriteByte(m_frameStart);			 // framestart
	WriteByte((int)v.framerate);	 // framerate
	WriteByte((int)(m_life * 10.0));	 // life
	WriteByte(m_boltWidth);			 // width
	WriteByte(m_noiseAmplitude);		 // noise
	WriteByte((int)v.rendercolor.x); // r, g, b
	WriteByte((int)v.rendercolor.y); // r, g, b
	WriteByte((int)v.rendercolor.z); // r, g, b
	WriteByte(v.renderamt);			 // brightness
	WriteByte(m_speed);				 // speed
	MessageEnd();
#else
	MessageBegin(MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte(TE_LIGHTNING);
	WriteCoord(vecSrc.x);
	WriteCoord(vecSrc.y);
	WriteCoord(vecSrc.z);
	WriteCoord(vecDest.x);
	WriteCoord(vecDest.y);
	WriteCoord(vecDest.z);
	WriteByte(10);
	WriteByte(50);
	WriteByte(40);
	WriteShort(m_spriteTexture);
	MessageEnd();
#endif
	DoSparks(vecSrc, vecDest);
}

void CLightning::RandomArea()
{
	int iLoops = 0;

	for (iLoops = 0; iLoops < 10; iLoops++)
	{
		Vector vecSrc = v.origin;

		Vector vecDir1 = Vector(RANDOM_FLOAT(-1.0, 1.0), RANDOM_FLOAT(-1.0, 1.0), RANDOM_FLOAT(-1.0, 1.0));
		vecDir1 = vecDir1.Normalize();
		TraceResult tr1;
		util::TraceLine(vecSrc, vecSrc + vecDir1 * m_radius, util::ignore_monsters, this, &tr1);

		if (tr1.flFraction == 1.0)
			continue;

		Vector vecDir2;
		do
		{
			vecDir2 = Vector(RANDOM_FLOAT(-1.0, 1.0), RANDOM_FLOAT(-1.0, 1.0), RANDOM_FLOAT(-1.0, 1.0));
		} while (DotProduct(vecDir1, vecDir2) > 0);
		vecDir2 = vecDir2.Normalize();
		TraceResult tr2;
		util::TraceLine(vecSrc, vecSrc + vecDir2 * m_radius, util::ignore_monsters, this, &tr2);

		if (tr2.flFraction == 1.0)
			continue;

		if ((tr1.vecEndPos - tr2.vecEndPos).Length() < m_radius * 0.1)
			continue;

		util::TraceLine(tr1.vecEndPos, tr2.vecEndPos, util::ignore_monsters, this, &tr2);

		if (tr2.flFraction != 1.0)
			continue;

		Zap(tr1.vecEndPos, tr2.vecEndPos);

		break;
	}
}


void CLightning::RandomPoint(Vector& vecSrc)
{
	int iLoops = 0;

	for (iLoops = 0; iLoops < 10; iLoops++)
	{
		Vector vecDir1 = Vector(RANDOM_FLOAT(-1.0, 1.0), RANDOM_FLOAT(-1.0, 1.0), RANDOM_FLOAT(-1.0, 1.0));
		vecDir1 = vecDir1.Normalize();
		TraceResult tr1;
		util::TraceLine(vecSrc, vecSrc + vecDir1 * m_radius, util::ignore_monsters, this, &tr1);

		if ((tr1.vecEndPos - vecSrc).Length() < m_radius * 0.1)
			continue;

		if (tr1.flFraction == 1.0)
			continue;

		Zap(vecSrc, tr1.vecEndPos);
		break;
	}
}



void CLightning::BeamUpdateVars()
{
	int beamType;
	bool pointStart, pointEnd;

	CBaseEntity* pStart = util::FindEntityByTargetname(nullptr, STRING(m_iszStartEntity));
	CBaseEntity* pEnd = util::FindEntityByTargetname(nullptr, STRING(m_iszEndEntity));
	pointStart = IsPointEntity(pStart);
	pointEnd = IsPointEntity(pEnd);

	v.skin = 0;
	v.sequence = 0;
	v.rendermode = 0;
	v.flags |= FL_CUSTOMENTITY;
	v.model = m_iszSpriteName;
	SetTexture(m_spriteTexture);

	beamType = BEAM_ENTS;
	if (pointStart || pointEnd)
	{
		if (!pointStart) // One point entity must be in pStart
		{
			CBaseEntity* pTemp;
			// Swap start & end
			pTemp = pStart;
			pStart = pEnd;
			pEnd = pTemp;
			bool swap = pointStart;
			pointStart = pointEnd;
			pointEnd = swap;
		}
		if (!pointEnd)
			beamType = BEAM_ENTPOINT;
		else
			beamType = BEAM_POINTS;
	}

	SetType(beamType);
	if (beamType == BEAM_POINTS || beamType == BEAM_ENTPOINT || beamType == BEAM_HOSE)
	{
		SetStartPos(pStart->v.origin);
		if (beamType == BEAM_POINTS || beamType == BEAM_HOSE)
			SetEndPos(pEnd->v.origin);
		else
			SetEndEntity(pEnd->entindex());
	}
	else
	{
		SetStartEntity(pStart->entindex());
		SetEndEntity(pEnd->entindex());
	}

	RelinkBeam();

	SetWidth(m_boltWidth);
	SetNoise(m_noiseAmplitude);
	SetFrame(m_frameStart);
	SetScrollRate(m_speed);
	if ((v.spawnflags & SF_BEAM_SHADEIN) != 0)
		SetFlags(BEAM_FSHADEIN);
	else if ((v.spawnflags & SF_BEAM_SHADEOUT) != 0)
		SetFlags(BEAM_FSHADEOUT);
}


LINK_ENTITY_TO_CLASS(env_laser, CLaser);

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CLaser)
	DEFINE_FIELD(CLaser, m_pSprite, FIELD_CLASSPTR),
	DEFINE_FIELD(CLaser, m_iszSpriteName, FIELD_STRING),
	DEFINE_FIELD(CLaser, m_firePosition, FIELD_POSITION_VECTOR),
END_SAVERESTORE(CLaser, CBeam)
#endif

bool CLaser::Spawn()
{
	if (FStringNull(v.model))
	{
		return false;
	}
	v.solid = SOLID_NOT; // Remove model & collisions
	Precache();

	SetThink(&CLaser::StrikeThink);
	v.flags |= FL_CUSTOMENTITY;

	PointsInit(v.origin, v.origin);

	if (!m_pSprite && !FStringNull(m_iszSpriteName))
		m_pSprite = CSprite::SpriteCreate(STRING(m_iszSpriteName), v.origin, true);
	else
		m_pSprite = nullptr;

	if (m_pSprite)
		m_pSprite->SetTransparency(kRenderGlow, v.rendercolor.x, v.rendercolor.y, v.rendercolor.z, v.renderamt, v.renderfx);

	if (!FStringNull(v.targetname) && (v.spawnflags & SF_BEAM_STARTON) == 0)
		TurnOff();
	else
		TurnOn();

	return true;
}

void CLaser::Precache()
{
	v.modelindex = PRECACHE_MODEL((char*)STRING(v.model));
	if (!FStringNull(m_iszSpriteName))
		PRECACHE_MODEL((char*)STRING(m_iszSpriteName));
}


bool CLaser::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "LaserTarget"))
	{
		v.message = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "width"))
	{
		SetWidth((int)atof(pkvd->szValue));
		return true;
	}
	else if (streq(pkvd->szKeyName, "NoiseAmplitude"))
	{
		SetNoise(atoi(pkvd->szValue));
		return true;
	}
	else if (streq(pkvd->szKeyName, "TextureScroll"))
	{
		SetScrollRate(atoi(pkvd->szValue));
		return true;
	}
	else if (streq(pkvd->szKeyName, "texture"))
	{
		v.model = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "EndSprite"))
	{
		m_iszSpriteName = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "framestart"))
	{
		v.frame = atoi(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "damage"))
	{
		v.dmg = atof(pkvd->szValue);
		return true;
	}

	return CBeam::KeyValue(pkvd);
}


bool CLaser::IsOn()
{
	if ((v.effects & EF_NODRAW) != 0)
		return false;
	return true;
}


void CLaser::TurnOff()
{
	v.effects |= EF_NODRAW;
	v.nextthink = 0;
	if (m_pSprite)
		m_pSprite->TurnOff();
}


void CLaser::TurnOn()
{
	v.effects &= ~EF_NODRAW;
	if (m_pSprite)
		m_pSprite->TurnOn();
	v.dmgtime = gpGlobals->time;
	v.nextthink = gpGlobals->time;
}


void CLaser::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	bool active = IsOn();

	if (!ShouldToggle(useType, active))
		return;
	if (active)
	{
		TurnOff();
	}
	else
	{
		TurnOn();
	}
}


void CLaser::FireAtPoint(TraceResult& tr)
{
	SetEndPos(tr.vecEndPos);
	if (m_pSprite)
		m_pSprite->SetOrigin(tr.vecEndPos);

	BeamDamage(&tr);
	DoSparks(GetStartPos(), tr.vecEndPos);
}

void CLaser::StrikeThink()
{
	CBaseEntity* pEnd = RandomTargetname(STRING(v.message));

	if (pEnd)
		m_firePosition = pEnd->v.origin;

	TraceResult tr;

	util::TraceLine(v.origin, m_firePosition, util::dont_ignore_monsters, nullptr, &tr);
	FireAtPoint(tr);
	v.nextthink = gpGlobals->time + 0.1;
}



class CGlow : public CPointEntity
{
public:
	CGlow(Entity* containingEntity) : CPointEntity(containingEntity) {}

	DECLARE_SAVERESTORE()

	bool Spawn() override;
	void Think() override;
	void Animate(float frames);

	float m_lastTime;
	float m_maxFrame;
};

LINK_ENTITY_TO_CLASS(env_glow, CGlow);

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CGlow)
	DEFINE_FIELD(CGlow, m_lastTime, FIELD_TIME),
	DEFINE_FIELD(CGlow, m_maxFrame, FIELD_FLOAT),
END_SAVERESTORE(CGlow, CPointEntity)
#endif

bool CGlow::Spawn()
{
	v.solid = SOLID_NOT;
	v.movetype = MOVETYPE_NONE;
	v.effects = 0;
	v.frame = 0;

	PRECACHE_MODEL((char*)STRING(v.model));
	SetModel(v.model);

	m_maxFrame = (float)MODEL_FRAMES(v.modelindex) - 1;
	if (m_maxFrame > 1.0 && v.framerate != 0)
		v.nextthink = gpGlobals->time + 0.1;

	m_lastTime = gpGlobals->time;

	return true;
}


void CGlow::Think()
{
	Animate(v.framerate * (gpGlobals->time - m_lastTime));

	v.nextthink = gpGlobals->time + 0.1;
	m_lastTime = gpGlobals->time;
}


void CGlow::Animate(float frames)
{
	if (m_maxFrame > 0)
		v.frame = fmod(v.frame + frames, m_maxFrame);
}


LINK_ENTITY_TO_CLASS(env_sprite, CSprite);

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CSprite)
	DEFINE_FIELD(CSprite, m_lastTime, FIELD_TIME),
	DEFINE_FIELD(CSprite, m_maxFrame, FIELD_FLOAT),
END_SAVERESTORE(CSprite, CPointEntity)
#endif

bool CSprite::Spawn()
{
	v.solid = SOLID_NOT;
	v.movetype = MOVETYPE_NONE;
	v.effects = 0;
	v.frame = 0;

	Precache();
	SetModel(v.model);

	m_maxFrame = (float)MODEL_FRAMES(v.modelindex) - 1;
	if (!FStringNull(v.targetname) && (v.spawnflags & SF_SPRITE_STARTON) == 0)
		TurnOff();
	else
		TurnOn();

	// Worldcraft only sets y rotation, copy to Z
	if (v.angles.y != 0 && v.angles.z == 0)
	{
		v.angles.z = v.angles.y;
		v.angles.y = 0;
	}

	return true;
}


void CSprite::Precache()
{
	PRECACHE_MODEL((char*)STRING(v.model));

	// Reset attachment after save/restore
	if (v.aiment)
		SetAttachment(v.aiment, v.body);
	else
	{
		// Clear attachment
		v.skin = 0;
		v.body = 0;
	}
}


void CSprite::SpriteInit(const char* pSpriteName, const Vector& origin)
{
	v.model = MAKE_STRING(pSpriteName);
	v.origin = origin;
	Spawn();
}

CSprite* CSprite::SpriteCreate(const char* pSpriteName, const Vector& origin, bool animate)
{
	CSprite* pSprite = Entity::Create<CSprite>();
	pSprite->SpriteInit(pSpriteName, origin);
	pSprite->v.classname = MAKE_STRING("env_sprite");
	pSprite->v.solid = SOLID_NOT;
	pSprite->v.movetype = MOVETYPE_NOCLIP;
	if (animate)
		pSprite->TurnOn();

	return pSprite;
}


void CSprite::AnimateThink()
{
	Animate(v.framerate * (gpGlobals->time - m_lastTime));

	v.nextthink = gpGlobals->time + 0.1;
	m_lastTime = gpGlobals->time;
}

void CSprite::AnimateUntilDead()
{
	if (gpGlobals->time > v.dmgtime)
		Remove();
	else
	{
		AnimateThink();
		v.nextthink = gpGlobals->time;
	}
}

void CSprite::Expand(float scaleSpeed, float fadeSpeed)
{
	v.speed = scaleSpeed;
	v.health = fadeSpeed;
	SetThink(&CSprite::ExpandThink);

	v.nextthink = gpGlobals->time;
	m_lastTime = gpGlobals->time;
}


void CSprite::ExpandThink()
{
	float frametime = gpGlobals->time - m_lastTime;
	v.scale += v.speed * frametime;
	v.renderamt -= v.health * frametime;
	if (v.renderamt <= 0)
	{
		v.renderamt = 0;
		Remove();
	}
	else
	{
		v.nextthink = gpGlobals->time + 0.1;
		m_lastTime = gpGlobals->time;
	}
}


void CSprite::Animate(float frames)
{
	v.frame += frames;
	if (v.frame > m_maxFrame)
	{
		if ((v.spawnflags & SF_SPRITE_ONCE) != 0)
		{
			TurnOff();
		}
		else
		{
			if (m_maxFrame > 0)
				v.frame = fmod(v.frame, m_maxFrame);
		}
	}
}


void CSprite::TurnOff()
{
	v.effects = EF_NODRAW;
	v.nextthink = 0;
}


void CSprite::TurnOn()
{
	v.effects = 0;
	if ((0 != v.framerate && m_maxFrame > 1.0) || (v.spawnflags & SF_SPRITE_ONCE) != 0)
	{
		SetThink(&CSprite::AnimateThink);
		v.nextthink = gpGlobals->time;
		m_lastTime = gpGlobals->time;
	}
	v.frame = 0;
}


void CSprite::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	bool on = v.effects != EF_NODRAW;
	if (ShouldToggle(useType, on))
	{
		if (on)
		{
			TurnOff();
		}
		else
		{
			TurnOn();
		}
	}
}


// Screen shake
class CShake : public CPointEntity
{
public:
	CShake(Entity* containingEntity) : CPointEntity(containingEntity) {}

	bool Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	bool KeyValue(KeyValueData* pkvd) override;

	inline float Amplitude() { return v.scale; }
	inline float Frequency() { return v.dmg_save; }
	inline float Duration() { return v.dmg_take; }
	inline float Radius() { return v.dmg; }

	inline void SetAmplitude(float amplitude) { v.scale = amplitude; }
	inline void SetFrequency(float frequency) { v.dmg_save = frequency; }
	inline void SetDuration(float duration) { v.dmg_take = duration; }
	inline void SetRadius(float radius) { v.dmg = radius; }

private:
};

LINK_ENTITY_TO_CLASS(env_shake, CShake);

// v.scale is amplitude
// v.dmg_save is frequency
// v.dmg_take is duration
// v.dmg is radius
// radius of 0 means all players
// NOTE: util::ScreenShake() will only shake players who are on the ground

#define SF_SHAKE_EVERYONE 0x0001 // Don't check radius
#define SF_SHAKE_DISRUPT 0x0002 // Disrupt controls
#define SF_SHAKE_INAIR 0x0004	// Shake players in air

bool CShake::Spawn()
{
	v.solid = SOLID_NOT;
	v.movetype = MOVETYPE_NONE;
	v.effects = 0;
	v.frame = 0;

	if ((v.spawnflags & SF_SHAKE_EVERYONE) != 0)
		v.dmg = 0;

	return true;
}


bool CShake::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "amplitude"))
	{
		SetAmplitude(atof(pkvd->szValue));
		return true;
	}
	else if (streq(pkvd->szKeyName, "frequency"))
	{
		SetFrequency(atof(pkvd->szValue));
		return true;
	}
	else if (streq(pkvd->szKeyName, "duration"))
	{
		SetDuration(atof(pkvd->szValue));
		return true;
	}
	else if (streq(pkvd->szKeyName, "radius"))
	{
		SetRadius(atof(pkvd->szValue));
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}


void CShake::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	util::ScreenShake(v.origin, Amplitude(), Frequency(), Duration(), Radius());
}


class CFade : public CPointEntity
{
public:
	CFade(Entity* containingEntity) : CPointEntity(containingEntity) {}

	bool Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	bool KeyValue(KeyValueData* pkvd) override;

	inline float Duration() { return v.dmg_take; }
	inline float HoldTime() { return v.dmg_save; }

	inline void SetDuration(float duration) { v.dmg_take = duration; }
	inline void SetHoldTime(float hold) { v.dmg_save = hold; }

private:
};

LINK_ENTITY_TO_CLASS(env_fade, CFade);

// v.dmg_take is duration
// v.dmg_save is hold duration
#define SF_FADE_IN 0x0001		// Fade in, not out
#define SF_FADE_MODULATE 0x0002 // Modulate, don't blend
#define SF_FADE_ONLYONE 0x0004

bool CFade::Spawn()
{
	v.solid = SOLID_NOT;
	v.movetype = MOVETYPE_NONE;
	v.effects = 0;
	v.frame = 0;
	return true;
}


bool CFade::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "duration"))
	{
		SetDuration(atof(pkvd->szValue));
		return true;
	}
	else if (streq(pkvd->szKeyName, "holdtime"))
	{
		SetHoldTime(atof(pkvd->szValue));
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}


void CFade::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	int fadeFlags = 0;

	if ((v.spawnflags & SF_FADE_IN) == 0)
		fadeFlags |= FFADE_OUT;

	if ((v.spawnflags & SF_FADE_MODULATE) != 0)
		fadeFlags |= FFADE_MODULATE;

	if ((v.spawnflags & SF_FADE_ONLYONE) != 0)
	{
		if (pActivator->IsNetClient())
		{
			util::ScreenFade(pActivator, v.rendercolor, Duration(), HoldTime(), v.renderamt, fadeFlags);
		}
	}
	else
	{
		util::ScreenFadeAll(v.rendercolor, Duration(), HoldTime(), v.renderamt, fadeFlags);
	}
	UseTargets(this, USE_TOGGLE, 0);
}


class CMessage : public CPointEntity
{
public:
	CMessage(Entity* containingEntity) : CPointEntity(containingEntity) {}

	bool Spawn() override;
	void Precache() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	bool KeyValue(KeyValueData* pkvd) override;

private:
};

LINK_ENTITY_TO_CLASS(env_message, CMessage);


bool CMessage::Spawn()
{
	Precache();

	v.solid = SOLID_NOT;
	v.movetype = MOVETYPE_NONE;

	switch (v.impulse)
	{
	case 1: // Medium radius
		v.speed = ATTN_STATIC;
		break;

	case 2: // Large radius
		v.speed = ATTN_NORM;
		break;

	case 3: //EVERYWHERE
		v.speed = ATTN_NONE;
		break;

	default:
	case 0: // Small radius
		v.speed = ATTN_IDLE;
		break;
	}
	v.impulse = 0;

	// No volume, use normal
	if (v.scale <= 0)
		v.scale = 1.0;

	return true;
}


void CMessage::Precache()
{
	if (!FStringNull(v.noise))
		PRECACHE_SOUND((char*)STRING(v.noise));
}

bool CMessage::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "messagesound"))
	{
		v.noise = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "messagevolume"))
	{
		v.scale = atof(pkvd->szValue) * 0.1;
		return true;
	}
	else if (streq(pkvd->szKeyName, "messageattenuation"))
	{
		v.impulse = atoi(pkvd->szValue);
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}


void CMessage::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	CBaseEntity* pPlayer = nullptr;

	if ((v.spawnflags & SF_MESSAGE_ALL) != 0)
		util::ShowMessageAll(STRING(v.message));
	else
	{
		if (pActivator && pActivator->IsPlayer())
			pPlayer = pActivator;
		else
		{
			pPlayer = util::GetLocalPlayer();
		}
		if (pPlayer)
			util::ShowMessage(STRING(v.message), pPlayer);
	}
	if (!FStringNull(v.noise))
	{
		EmitSound(STRING(v.noise), CHAN_BODY, v.scale, v.speed);
	}
	if ((v.spawnflags & SF_MESSAGE_ONCE) != 0)
		Remove();

	UseTargets(this, USE_TOGGLE, 0);
}



//=========================================================
// FunnelEffect
//=========================================================
class CEnvFunnel : public CBaseEntity
{
public:
	CEnvFunnel(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	bool Spawn() override;
	void Precache() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	int m_iSprite; // Don't save, precache
};

void CEnvFunnel::Precache()
{
	m_iSprite = PRECACHE_MODEL("sprites/flare6.spr");
}

LINK_ENTITY_TO_CLASS(env_funnel, CEnvFunnel);

void CEnvFunnel::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	MessageBegin(MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte(TE_LARGEFUNNEL);
	WriteCoord(v.origin.x);
	WriteCoord(v.origin.y);
	WriteCoord(v.origin.z);
	WriteShort(m_iSprite);

	if ((v.spawnflags & SF_FUNNEL_REVERSE) != 0) // funnel flows in reverse?
	{
		WriteShort(1);
	}
	else
	{
		WriteShort(0);
	}


	MessageEnd();

	Remove();
}

bool CEnvFunnel::Spawn()
{
	Precache();
	v.solid = SOLID_NOT;
	v.effects = EF_NODRAW;
	return true;
}

//=========================================================
// Beverage Dispenser
// overloaded v.frags, is now a flag for whether or not a can is stuck in the dispenser.
// overloaded v.health, is now how many cans remain in the machine.
//=========================================================
class CEnvBeverage : public CBaseEntity
{
public:
	CEnvBeverage(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	bool Spawn() override;
	void Precache() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
};

void CEnvBeverage::Precache()
{
	PRECACHE_MODEL("models/can.mdl");
	PRECACHE_SOUND("weapons/g_bounce3.wav");
}

LINK_ENTITY_TO_CLASS(env_beverage, CEnvBeverage);

void CEnvBeverage::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (v.frags != 0 || v.health <= 0)
	{
		// no more cans while one is waiting in the dispenser, or if I'm out of cans.
		return;
	}

	CBaseEntity* pCan = CBaseEntity::Create("item_sodacan", v.origin, v.angles, v);

	if (v.skin == 6)
	{
		// random
		pCan->v.skin = RANDOM_LONG(0, 5);
	}
	else
	{
		pCan->v.skin = v.skin;
	}

	v.frags = 1;
	v.health--;
}

bool CEnvBeverage::Spawn()
{
	Precache();
	v.solid = SOLID_NOT;
	v.effects = EF_NODRAW;
	v.frags = 0;

	if (v.health == 0)
	{
		v.health = 10;
	}

	return true;
}

//=========================================================
// Soda can
//=========================================================
class CItemSoda : public CBaseEntity
{
public:
	CItemSoda(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	bool Spawn() override;
	void EXPORT CanThink();
	void EXPORT CanTouch(CBaseEntity* pOther);
};

LINK_ENTITY_TO_CLASS(item_sodacan, CItemSoda);

bool CItemSoda::Spawn()
{
	v.solid = SOLID_NOT;
	v.movetype = MOVETYPE_TOSS;

	SetModel("models/can.mdl");
	SetSize(g_vecZero, g_vecZero);

	SetThink(&CItemSoda::CanThink);
	v.nextthink = gpGlobals->time + 0.5;

	return true;
}

void CItemSoda::CanThink()
{
	EmitSound("weapons/g_bounce3.wav", CHAN_WEAPON);

	v.solid = SOLID_TRIGGER;
	SetSize(Vector(-8, -8, 0), Vector(8, 8, 8));
	ClearThink();
	SetTouch(&CItemSoda::CanTouch);
}

void CItemSoda::CanTouch(CBaseEntity* pOther)
{
	if (!pOther->IsPlayer())
	{
		return;
	}

	// spoit sound here

	pOther->GiveHealth(1, DMG_GENERIC); // a bit of health.

	if (v.owner != nullptr)
	{
		// tell the machine the can was taken
		v.owner->frags = 0;
	}

	v.solid = SOLID_NOT;
	v.movetype = MOVETYPE_NONE;
	v.effects = EF_NODRAW;
	ClearTouch();
	Remove();
}
