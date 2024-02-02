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
	SetModel(STRING(pev->model)); // Set size

	pev->solid = SOLID_NOT; // Remove model & collisions
	pev->renderamt = 0;		// The engine won't draw this model if this is set to 0 and blending is on
	pev->rendermode = kRenderTransTexture;
	int speed = pev->speed > 0 ? pev->speed : -pev->speed;

	// HACKHACK!!! - Speed in rendercolor
	pev->rendercolor.x = speed >> 8;
	pev->rendercolor.y = speed & 255;
	pev->rendercolor.z = (pev->speed < 0) ? 1 : 0;


	if ((pev->spawnflags & SF_BUBBLES_STARTOFF) == 0)
	{
		SetThink(&CBubbling::FizzThink);
		pev->nextthink = gpGlobals->time + 2.0;
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
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		SetThink(NULL);
		pev->nextthink = 0;
	}
}


bool CBubbling::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "density"))
	{
		m_density = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "frequency"))
	{
		m_frequency = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "current"))
	{
		pev->speed = atoi(pkvd->szValue);
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
		pev->nextthink = gpGlobals->time + 0.5;
	else
		pev->nextthink = gpGlobals->time + 2.5 - (0.1 * m_frequency);
}

// --------------------------------------------------
//
// Beams
//
// --------------------------------------------------

bool CBeam::Spawn()
{
	pev->solid = SOLID_NOT; // Remove model & collisions
	Precache();

	return true;
}

void CBeam::Precache()
{
	if (pev->owner)
		SetStartEntity(ENTINDEX(pev->owner));
	if (pev->aiment)
		SetEndEntity(ENTINDEX(pev->aiment));
}

void CBeam::SetStartEntity(int entityIndex)
{
	pev->sequence = (entityIndex & 0x0FFF) | (pev->sequence & 0xF000);
	pev->owner = g_engfuncs.pfnPEntityOfEntIndex(entityIndex);
}

void CBeam::SetEndEntity(int entityIndex)
{
	pev->skin = (entityIndex & 0x0FFF) | (pev->skin & 0xF000);
	pev->aiment = g_engfuncs.pfnPEntityOfEntIndex(entityIndex);
}


// These don't take attachments into account
const Vector& CBeam::GetStartPos()
{
	if (GetType() == BEAM_ENTS)
	{
		edict_t* pent = g_engfuncs.pfnPEntityOfEntIndex(GetStartEntity());
		return pent->v.origin;
	}
	return pev->origin;
}


const Vector& CBeam::GetEndPos()
{
	int type = GetType();
	if (type == BEAM_POINTS || type == BEAM_HOSE)
	{
		return pev->angles;
	}

	edict_t* pent = g_engfuncs.pfnPEntityOfEntIndex(GetEndEntity());
	if (pent)
		return pent->v.origin;
	return pev->angles;
}


CBeam* CBeam::BeamCreate(const char* pSpriteName, int width)
{
	// Create a new entity with CBeam private data
	CBeam* pBeam = GetClassPtr((CBeam*)NULL);
	pBeam->pev->classname = MAKE_STRING("beam");

	pBeam->BeamInit(pSpriteName, width);

	return pBeam;
}


void CBeam::BeamInit(const char* pSpriteName, int width)
{
	pev->flags |= FL_CUSTOMENTITY;
	SetColor(255, 255, 255);
	SetBrightness(255);
	SetNoise(0);
	SetFrame(0);
	SetScrollRate(0);
	pev->model = MAKE_STRING(pSpriteName);
	SetTexture(PRECACHE_MODEL((char*)pSpriteName));
	SetWidth(width);
	pev->skin = 0;
	pev->sequence = 0;
	pev->rendermode = 0;
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

	pev->mins.x = std::min(startPos.x, endPos.x);
	pev->mins.y = std::min(startPos.y, endPos.y);
	pev->mins.z = std::min(startPos.z, endPos.z);
	pev->maxs.x = std::max(startPos.x, endPos.x);
	pev->maxs.y = std::max(startPos.y, endPos.y);
	pev->maxs.z = std::max(startPos.z, endPos.z);
	pev->mins = pev->mins - pev->origin;
	pev->maxs = pev->maxs - pev->origin;

	SetSize(pev->mins, pev->maxs);
	SetOrigin(pev->origin);
}

#if 0
void CBeam::SetObjectCollisionBox()
{
	const Vector &startPos = GetStartPos(), &endPos = GetEndPos();

	pev->absmin.x = std::min( startPos.x, endPos.x );
	pev->absmin.y = std::min( startPos.y, endPos.y );
	pev->absmin.z = std::min( startPos.z, endPos.z );
	pev->absmax.x = std::max( startPos.x, endPos.x );
	pev->absmax.y = std::max( startPos.y, endPos.y );
	pev->absmax.z = std::max( startPos.z, endPos.z );
}
#endif


void CBeam::TriggerTouch(CBaseEntity* pOther)
{
	if ((pOther->pev->flags & (FL_CLIENT | FL_MONSTER)) != 0)
	{
		if (pev->owner)
		{
			CBaseEntity* pOwner = CBaseEntity::Instance(pev->owner);
			pOwner->Use(pOther, this, USE_TOGGLE, 0);
		}
		ALERT(at_console, "Firing targets!!!\n");
	}
}


CBaseEntity* CBeam::RandomTargetname(const char* szName)
{
	int total = 0;

	CBaseEntity* pEntity = NULL;
	CBaseEntity* pNewEntity = NULL;
	while ((pNewEntity = util::FindEntityByTargetname(pNewEntity, szName)) != NULL)
	{
		total++;
		if (RANDOM_LONG(0, total - 1) < 1)
			pEntity = pNewEntity;
	}
	return pEntity;
}


void CBeam::DoSparks(const Vector& start, const Vector& end)
{
	if ((pev->spawnflags & (SF_BEAM_SPARKSTART | SF_BEAM_SPARKEND)) != 0)
	{
		if ((pev->spawnflags & SF_BEAM_SPARKSTART) != 0)
		{
			tent::Sparks(start);
		}
		if ((pev->spawnflags & SF_BEAM_SPARKEND) != 0)
		{
			tent::Sparks(end);
		}
	}
}


class CLightning : public CBeam
{
public:
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
		if (m_life == 0 && (pev->spawnflags & SF_BEAM_RING) == 0)
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
	pev->solid = SOLID_NOT; // Remove model & collisions
	Precache();

	pev->dmgtime = gpGlobals->time;

	if (ServerSide())
	{
		SetThink(NULL);
		if (pev->dmg > 0)
		{
			SetThink(&CLightning::DamageThink);
			pev->nextthink = gpGlobals->time + 0.1;
		}
		if (!FStringNull(pev->targetname))
		{
			if ((pev->spawnflags & SF_BEAM_STARTON) == 0)
			{
				pev->effects = EF_NODRAW;
				m_active = false;
				pev->nextthink = 0;
			}
			else
				m_active = true;

			SetUse(&CLightning::ToggleUse);
		}
	}
	else
	{
		m_active = false;
		if (!FStringNull(pev->targetname))
		{
			SetUse(&CLightning::StrikeUse);
		}
		if (FStringNull(pev->targetname) || FBitSet(pev->spawnflags, SF_BEAM_STARTON))
		{
			SetThink(&CLightning::StrikeThink);
			pev->nextthink = gpGlobals->time + 1.0;
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
	if (FStrEq(pkvd->szKeyName, "LightningStart"))
	{
		m_iszStartEntity = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "LightningEnd"))
	{
		m_iszEndEntity = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "life"))
	{
		m_life = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "BoltWidth"))
	{
		m_boltWidth = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "NoiseAmplitude"))
	{
		m_noiseAmplitude = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "TextureScroll"))
	{
		m_speed = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "StrikeTime"))
	{
		m_restrike = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "texture"))
	{
		m_iszSpriteName = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "framestart"))
	{
		m_frameStart = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "Radius"))
	{
		m_radius = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "damage"))
	{
		pev->dmg = atof(pkvd->szValue);
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
		pev->effects |= EF_NODRAW;
		pev->nextthink = 0;
	}
	else
	{
		m_active = true;
		pev->effects &= ~EF_NODRAW;
		DoSparks(GetStartPos(), GetEndPos());
		if (pev->dmg > 0)
		{
			pev->nextthink = gpGlobals->time;
			pev->dmgtime = gpGlobals->time;
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
		SetThink(NULL);
	}
	else
	{
		SetThink(&CLightning::StrikeThink);
		pev->nextthink = gpGlobals->time + 0.1;
	}

	if (!FBitSet(pev->spawnflags, SF_BEAM_TOGGLE))
		SetUse(NULL);
}


bool IsPointEntity(CBaseEntity* pEnt)
{
	if (0 == pEnt->pev->modelindex)
		return true;
	if (FClassnameIs(pEnt->pev, "info_target") || FClassnameIs(pEnt->pev, "info_landmark") || FClassnameIs(pEnt->pev, "path_corner"))
		return true;

	return false;
}


void CLightning::StrikeThink()
{
	if (m_life != 0)
	{
		if ((pev->spawnflags & SF_BEAM_RANDOM) != 0)
			pev->nextthink = gpGlobals->time + m_life + RANDOM_FLOAT(0, m_restrike);
		else
			pev->nextthink = gpGlobals->time + m_life + m_restrike;
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
			if (pStart != NULL)
				RandomPoint(pStart->pev->origin);
			else
				ALERT(at_console, "env_beam: unknown entity \"%s\"\n", STRING(m_iszStartEntity));
		}
		return;
	}

	CBaseEntity* pStart = RandomTargetname(STRING(m_iszStartEntity));
	CBaseEntity* pEnd = RandomTargetname(STRING(m_iszEndEntity));

	if (pStart != NULL && pEnd != NULL)
	{
		if (IsPointEntity(pStart) || IsPointEntity(pEnd))
		{
			if ((pev->spawnflags & SF_BEAM_RING) != 0)
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
				WriteCoord(pEnd->pev->origin.x);
				WriteCoord(pEnd->pev->origin.y);
				WriteCoord(pEnd->pev->origin.z);
			}
			else
			{
				WriteByte(TE_BEAMPOINTS);
				WriteCoord(pStart->pev->origin.x);
				WriteCoord(pStart->pev->origin.y);
				WriteCoord(pStart->pev->origin.z);
				WriteCoord(pEnd->pev->origin.x);
				WriteCoord(pEnd->pev->origin.y);
				WriteCoord(pEnd->pev->origin.z);
			}
		}
		else
		{
			if ((pev->spawnflags & SF_BEAM_RING) != 0)
				WriteByte(TE_BEAMRING);
			else
				WriteByte(TE_BEAMENTS);
			WriteShort(pStart->entindex());
			WriteShort(pEnd->entindex());
		}

		WriteShort(m_spriteTexture);
		WriteByte(m_frameStart);			 // framestart
		WriteByte((int)pev->framerate);	 // framerate
		WriteByte((int)(m_life * 10.0));	 // life
		WriteByte(m_boltWidth);			 // width
		WriteByte(m_noiseAmplitude);		 // noise
		WriteByte((int)pev->rendercolor.x); // r, g, b
		WriteByte((int)pev->rendercolor.y); // r, g, b
		WriteByte((int)pev->rendercolor.z); // r, g, b
		WriteByte(pev->renderamt);			 // brightness
		WriteByte(m_speed);				 // speed
		MessageEnd();
		DoSparks(pStart->pev->origin, pEnd->pev->origin);
		if (pev->dmg > 0)
		{
			TraceResult tr;
			util::TraceLine(pStart->pev->origin, pEnd->pev->origin, util::dont_ignore_monsters, nullptr, &tr);
			BeamDamageInstant(&tr, pev->dmg);
		}
	}
}


void CBeam::BeamDamage(TraceResult* ptr)
{
	RelinkBeam();
	if (ptr->flFraction != 1.0 && ptr->pHit != NULL)
	{
		CBaseEntity* pHit = CBaseEntity::Instance(ptr->pHit);
		if (pHit)
		{
			pHit->TraceAttack(this, pev->dmg * (gpGlobals->time - pev->dmgtime), (ptr->vecEndPos - pev->origin).Normalize(), ptr->iHitgroup, DMG_ENERGYBEAM);
			pHit->ApplyMultiDamage(this, this);
		}
	}
	pev->dmgtime = gpGlobals->time;
}


void CLightning::DamageThink()
{
	pev->nextthink = gpGlobals->time + 0.1;
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
	WriteByte((int)pev->framerate);	 // framerate
	WriteByte((int)(m_life * 10.0));	 // life
	WriteByte(m_boltWidth);			 // width
	WriteByte(m_noiseAmplitude);		 // noise
	WriteByte((int)pev->rendercolor.x); // r, g, b
	WriteByte((int)pev->rendercolor.y); // r, g, b
	WriteByte((int)pev->rendercolor.z); // r, g, b
	WriteByte(pev->renderamt);			 // brightness
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
		Vector vecSrc = pev->origin;

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

	edict_t* pStart = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(m_iszStartEntity));
	edict_t* pEnd = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(m_iszEndEntity));
	pointStart = IsPointEntity(CBaseEntity::Instance(pStart));
	pointEnd = IsPointEntity(CBaseEntity::Instance(pEnd));

	pev->skin = 0;
	pev->sequence = 0;
	pev->rendermode = 0;
	pev->flags |= FL_CUSTOMENTITY;
	pev->model = m_iszSpriteName;
	SetTexture(m_spriteTexture);

	beamType = BEAM_ENTS;
	if (pointStart || pointEnd)
	{
		if (!pointStart) // One point entity must be in pStart
		{
			edict_t* pTemp;
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
			SetEndEntity(ENTINDEX(pEnd));
	}
	else
	{
		SetStartEntity(ENTINDEX(pStart));
		SetEndEntity(ENTINDEX(pEnd));
	}

	RelinkBeam();

	SetWidth(m_boltWidth);
	SetNoise(m_noiseAmplitude);
	SetFrame(m_frameStart);
	SetScrollRate(m_speed);
	if ((pev->spawnflags & SF_BEAM_SHADEIN) != 0)
		SetFlags(BEAM_FSHADEIN);
	else if ((pev->spawnflags & SF_BEAM_SHADEOUT) != 0)
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
	if (FStringNull(pev->model))
	{
		return false;
	}
	pev->solid = SOLID_NOT; // Remove model & collisions
	Precache();

	SetThink(&CLaser::StrikeThink);
	pev->flags |= FL_CUSTOMENTITY;

	PointsInit(pev->origin, pev->origin);

	if (!m_pSprite && !FStringNull(m_iszSpriteName))
		m_pSprite = CSprite::SpriteCreate(STRING(m_iszSpriteName), pev->origin, true);
	else
		m_pSprite = NULL;

	if (m_pSprite)
		m_pSprite->SetTransparency(kRenderGlow, pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z, pev->renderamt, pev->renderfx);

	if (!FStringNull(pev->targetname) && (pev->spawnflags & SF_BEAM_STARTON) == 0)
		TurnOff();
	else
		TurnOn();

	return true;
}

void CLaser::Precache()
{
	pev->modelindex = PRECACHE_MODEL((char*)STRING(pev->model));
	if (!FStringNull(m_iszSpriteName))
		PRECACHE_MODEL((char*)STRING(m_iszSpriteName));
}


bool CLaser::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "LaserTarget"))
	{
		pev->message = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "width"))
	{
		SetWidth((int)atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "NoiseAmplitude"))
	{
		SetNoise(atoi(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "TextureScroll"))
	{
		SetScrollRate(atoi(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "texture"))
	{
		pev->model = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "EndSprite"))
	{
		m_iszSpriteName = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "framestart"))
	{
		pev->frame = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "damage"))
	{
		pev->dmg = atof(pkvd->szValue);
		return true;
	}

	return CBeam::KeyValue(pkvd);
}


bool CLaser::IsOn()
{
	if ((pev->effects & EF_NODRAW) != 0)
		return false;
	return true;
}


void CLaser::TurnOff()
{
	pev->effects |= EF_NODRAW;
	pev->nextthink = 0;
	if (m_pSprite)
		m_pSprite->TurnOff();
}


void CLaser::TurnOn()
{
	pev->effects &= ~EF_NODRAW;
	if (m_pSprite)
		m_pSprite->TurnOn();
	pev->dmgtime = gpGlobals->time;
	pev->nextthink = gpGlobals->time;
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
	CBaseEntity* pEnd = RandomTargetname(STRING(pev->message));

	if (pEnd)
		m_firePosition = pEnd->pev->origin;

	TraceResult tr;

	util::TraceLine(pev->origin, m_firePosition, util::dont_ignore_monsters, nullptr, &tr);
	FireAtPoint(tr);
	pev->nextthink = gpGlobals->time + 0.1;
}



class CGlow : public CPointEntity
{
public:
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
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;

	PRECACHE_MODEL((char*)STRING(pev->model));
	SetModel(STRING(pev->model));

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;
	if (m_maxFrame > 1.0 && pev->framerate != 0)
		pev->nextthink = gpGlobals->time + 0.1;

	m_lastTime = gpGlobals->time;

	return true;
}


void CGlow::Think()
{
	Animate(pev->framerate * (gpGlobals->time - m_lastTime));

	pev->nextthink = gpGlobals->time + 0.1;
	m_lastTime = gpGlobals->time;
}


void CGlow::Animate(float frames)
{
	if (m_maxFrame > 0)
		pev->frame = fmod(pev->frame + frames, m_maxFrame);
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
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;

	Precache();
	SetModel(STRING(pev->model));

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;
	if (!FStringNull(pev->targetname) && (pev->spawnflags & SF_SPRITE_STARTON) == 0)
		TurnOff();
	else
		TurnOn();

	// Worldcraft only sets y rotation, copy to Z
	if (pev->angles.y != 0 && pev->angles.z == 0)
	{
		pev->angles.z = pev->angles.y;
		pev->angles.y = 0;
	}

	return true;
}


void CSprite::Precache()
{
	PRECACHE_MODEL((char*)STRING(pev->model));

	// Reset attachment after save/restore
	if (pev->aiment)
		SetAttachment(pev->aiment, pev->body);
	else
	{
		// Clear attachment
		pev->skin = 0;
		pev->body = 0;
	}
}


void CSprite::SpriteInit(const char* pSpriteName, const Vector& origin)
{
	pev->model = MAKE_STRING(pSpriteName);
	pev->origin = origin;
	Spawn();
}

CSprite* CSprite::SpriteCreate(const char* pSpriteName, const Vector& origin, bool animate)
{
	CSprite* pSprite = GetClassPtr((CSprite*)NULL);
	pSprite->SpriteInit(pSpriteName, origin);
	pSprite->pev->classname = MAKE_STRING("env_sprite");
	pSprite->pev->solid = SOLID_NOT;
	pSprite->pev->movetype = MOVETYPE_NOCLIP;
	if (animate)
		pSprite->TurnOn();

	return pSprite;
}


void CSprite::AnimateThink()
{
	Animate(pev->framerate * (gpGlobals->time - m_lastTime));

	pev->nextthink = gpGlobals->time + 0.1;
	m_lastTime = gpGlobals->time;
}

void CSprite::AnimateUntilDead()
{
	if (gpGlobals->time > pev->dmgtime)
		Remove();
	else
	{
		AnimateThink();
		pev->nextthink = gpGlobals->time;
	}
}

void CSprite::Expand(float scaleSpeed, float fadeSpeed)
{
	pev->speed = scaleSpeed;
	pev->health = fadeSpeed;
	SetThink(&CSprite::ExpandThink);

	pev->nextthink = gpGlobals->time;
	m_lastTime = gpGlobals->time;
}


void CSprite::ExpandThink()
{
	float frametime = gpGlobals->time - m_lastTime;
	pev->scale += pev->speed * frametime;
	pev->renderamt -= pev->health * frametime;
	if (pev->renderamt <= 0)
	{
		pev->renderamt = 0;
		Remove();
	}
	else
	{
		pev->nextthink = gpGlobals->time + 0.1;
		m_lastTime = gpGlobals->time;
	}
}


void CSprite::Animate(float frames)
{
	pev->frame += frames;
	if (pev->frame > m_maxFrame)
	{
		if ((pev->spawnflags & SF_SPRITE_ONCE) != 0)
		{
			TurnOff();
		}
		else
		{
			if (m_maxFrame > 0)
				pev->frame = fmod(pev->frame, m_maxFrame);
		}
	}
}


void CSprite::TurnOff()
{
	pev->effects = EF_NODRAW;
	pev->nextthink = 0;
}


void CSprite::TurnOn()
{
	pev->effects = 0;
	if ((0 != pev->framerate && m_maxFrame > 1.0) || (pev->spawnflags & SF_SPRITE_ONCE) != 0)
	{
		SetThink(&CSprite::AnimateThink);
		pev->nextthink = gpGlobals->time;
		m_lastTime = gpGlobals->time;
	}
	pev->frame = 0;
}


void CSprite::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	bool on = pev->effects != EF_NODRAW;
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
	bool Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	bool KeyValue(KeyValueData* pkvd) override;

	inline float Amplitude() { return pev->scale; }
	inline float Frequency() { return pev->dmg_save; }
	inline float Duration() { return pev->dmg_take; }
	inline float Radius() { return pev->dmg; }

	inline void SetAmplitude(float amplitude) { pev->scale = amplitude; }
	inline void SetFrequency(float frequency) { pev->dmg_save = frequency; }
	inline void SetDuration(float duration) { pev->dmg_take = duration; }
	inline void SetRadius(float radius) { pev->dmg = radius; }

private:
};

LINK_ENTITY_TO_CLASS(env_shake, CShake);

// pev->scale is amplitude
// pev->dmg_save is frequency
// pev->dmg_take is duration
// pev->dmg is radius
// radius of 0 means all players
// NOTE: util::ScreenShake() will only shake players who are on the ground

#define SF_SHAKE_EVERYONE 0x0001 // Don't check radius
#define SF_SHAKE_DISRUPT 0x0002 // Disrupt controls
#define SF_SHAKE_INAIR 0x0004	// Shake players in air

bool CShake::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;

	if ((pev->spawnflags & SF_SHAKE_EVERYONE) != 0)
		pev->dmg = 0;

	return true;
}


bool CShake::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "amplitude"))
	{
		SetAmplitude(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "frequency"))
	{
		SetFrequency(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "duration"))
	{
		SetDuration(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "radius"))
	{
		SetRadius(atof(pkvd->szValue));
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}


void CShake::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	util::ScreenShake(pev->origin, Amplitude(), Frequency(), Duration(), Radius());
}


class CFade : public CPointEntity
{
public:
	bool Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	bool KeyValue(KeyValueData* pkvd) override;

	inline float Duration() { return pev->dmg_take; }
	inline float HoldTime() { return pev->dmg_save; }

	inline void SetDuration(float duration) { pev->dmg_take = duration; }
	inline void SetHoldTime(float hold) { pev->dmg_save = hold; }

private:
};

LINK_ENTITY_TO_CLASS(env_fade, CFade);

// pev->dmg_take is duration
// pev->dmg_save is hold duration
#define SF_FADE_IN 0x0001		// Fade in, not out
#define SF_FADE_MODULATE 0x0002 // Modulate, don't blend
#define SF_FADE_ONLYONE 0x0004

bool CFade::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;
	return true;
}


bool CFade::KeyValue(KeyValueData* pkvd)
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

	return CPointEntity::KeyValue(pkvd);
}


void CFade::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	int fadeFlags = 0;

	if ((pev->spawnflags & SF_FADE_IN) == 0)
		fadeFlags |= FFADE_OUT;

	if ((pev->spawnflags & SF_FADE_MODULATE) != 0)
		fadeFlags |= FFADE_MODULATE;

	if ((pev->spawnflags & SF_FADE_ONLYONE) != 0)
	{
		if (pActivator->IsNetClient())
		{
			util::ScreenFade(pActivator, pev->rendercolor, Duration(), HoldTime(), pev->renderamt, fadeFlags);
		}
	}
	else
	{
		util::ScreenFadeAll(pev->rendercolor, Duration(), HoldTime(), pev->renderamt, fadeFlags);
	}
	UseTargets(this, USE_TOGGLE, 0);
}


class CMessage : public CPointEntity
{
public:
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

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	switch (pev->impulse)
	{
	case 1: // Medium radius
		pev->speed = ATTN_STATIC;
		break;

	case 2: // Large radius
		pev->speed = ATTN_NORM;
		break;

	case 3: //EVERYWHERE
		pev->speed = ATTN_NONE;
		break;

	default:
	case 0: // Small radius
		pev->speed = ATTN_IDLE;
		break;
	}
	pev->impulse = 0;

	// No volume, use normal
	if (pev->scale <= 0)
		pev->scale = 1.0;

	return true;
}


void CMessage::Precache()
{
	if (!FStringNull(pev->noise))
		PRECACHE_SOUND((char*)STRING(pev->noise));
}

bool CMessage::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "messagesound"))
	{
		pev->noise = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "messagevolume"))
	{
		pev->scale = atof(pkvd->szValue) * 0.1;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "messageattenuation"))
	{
		pev->impulse = atoi(pkvd->szValue);
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}


void CMessage::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	CBaseEntity* pPlayer = NULL;

	if ((pev->spawnflags & SF_MESSAGE_ALL) != 0)
		util::ShowMessageAll(STRING(pev->message));
	else
	{
		if (pActivator && pActivator->IsPlayer())
			pPlayer = pActivator;
		else
		{
			pPlayer = util::GetLocalPlayer();
		}
		if (pPlayer)
			util::ShowMessage(STRING(pev->message), pPlayer);
	}
	if (!FStringNull(pev->noise))
	{
		EmitSound(STRING(pev->noise), CHAN_BODY, pev->scale, pev->speed);
	}
	if ((pev->spawnflags & SF_MESSAGE_ONCE) != 0)
		Remove();

	UseTargets(this, USE_TOGGLE, 0);
}



//=========================================================
// FunnelEffect
//=========================================================
class CEnvFunnel : public CBaseEntity
{
public:
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
	WriteCoord(pev->origin.x);
	WriteCoord(pev->origin.y);
	WriteCoord(pev->origin.z);
	WriteShort(m_iSprite);

	if ((pev->spawnflags & SF_FUNNEL_REVERSE) != 0) // funnel flows in reverse?
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
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	return true;
}

//=========================================================
// Beverage Dispenser
// overloaded pev->frags, is now a flag for whether or not a can is stuck in the dispenser.
// overloaded pev->health, is now how many cans remain in the machine.
//=========================================================
class CEnvBeverage : public CBaseEntity
{
public:
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
	if (pev->frags != 0 || pev->health <= 0)
	{
		// no more cans while one is waiting in the dispenser, or if I'm out of cans.
		return;
	}

	CBaseEntity* pCan = CBaseEntity::Create("item_sodacan", pev->origin, pev->angles, edict());

	if (pev->skin == 6)
	{
		// random
		pCan->pev->skin = RANDOM_LONG(0, 5);
	}
	else
	{
		pCan->pev->skin = pev->skin;
	}

	pev->frags = 1;
	pev->health--;
}

bool CEnvBeverage::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->frags = 0;

	if (pev->health == 0)
	{
		pev->health = 10;
	}

	return true;
}

//=========================================================
// Soda can
//=========================================================
class CItemSoda : public CBaseEntity
{
public:
	bool Spawn() override;
	void EXPORT CanThink();
	void EXPORT CanTouch(CBaseEntity* pOther);
};

LINK_ENTITY_TO_CLASS(item_sodacan, CItemSoda);

bool CItemSoda::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_TOSS;

	SetModel("models/can.mdl");
	SetSize(g_vecZero, g_vecZero);

	SetThink(&CItemSoda::CanThink);
	pev->nextthink = gpGlobals->time + 0.5;

	return true;
}

void CItemSoda::CanThink()
{
	EmitSound("weapons/g_bounce3.wav", CHAN_WEAPON);

	pev->solid = SOLID_TRIGGER;
	SetSize(Vector(-8, -8, 0), Vector(8, 8, 8));
	SetThink(NULL);
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

	if (!FNullEnt(pev->owner))
	{
		// tell the machine the can was taken
		pev->owner->v.frags = 0;
	}

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
	SetTouch(NULL);
	Remove();
}
