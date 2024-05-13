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

===== mortar.cpp ========================================================

  the "LaBuznik" mortar device              

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "weapons.h"

class CFuncMortarField : public CBaseToggle
{
public:
	CFuncMortarField(Entity* containingEntity) : CBaseToggle(containingEntity) {}

	DECLARE_SAVERESTORE()

	bool Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;

	// Bmodels don't go across transitions
	int ObjectCaps() override { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	void EXPORT FieldUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	int m_iszXController;
	int m_iszYController;
	float m_flSpread;
	float m_flDelay;
	int m_iCount;
	int m_fControl;
};

LINK_ENTITY_TO_CLASS(func_mortar_field, CFuncMortarField);

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CFuncMortarField)
	DEFINE_FIELD(CFuncMortarField, m_iszXController, FIELD_STRING),
	DEFINE_FIELD(CFuncMortarField, m_iszYController, FIELD_STRING),
	DEFINE_FIELD(CFuncMortarField, m_flSpread, FIELD_FLOAT),
	DEFINE_FIELD(CFuncMortarField, m_flDelay, FIELD_FLOAT),
	DEFINE_FIELD(CFuncMortarField, m_iCount, FIELD_INTEGER),
	DEFINE_FIELD(CFuncMortarField, m_fControl, FIELD_INTEGER),
END_SAVERESTORE(CFuncMortarField, CBaseToggle)
#endif


bool CFuncMortarField::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "m_iszXController"))
	{
		m_iszXController = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "m_iszYController"))
	{
		m_iszYController = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "m_flSpread"))
	{
		m_flSpread = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "m_fControl"))
	{
		m_fControl = atoi(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "m_iCount"))
	{
		m_iCount = atoi(pkvd->szValue);
		return true;
	}

	return false;
}


// Drop bombs from above
bool CFuncMortarField::Spawn()
{
	v.solid = SOLID_NOT;
	SetModel(v.model); // set size and link into world
	v.movetype = MOVETYPE_NONE;
	SetBits(v.effects, EF_NODRAW);
	SetUse(&CFuncMortarField::FieldUse);
	Precache();
	return true;
}


void CFuncMortarField::Precache()
{
	PRECACHE_SOUND("weapons/mortar.wav");
	PRECACHE_SOUND("weapons/mortarhit.wav");
	PRECACHE_MODEL("sprites/lgtning.spr");
}


// If connected to a table, then use the table controllers, else hit where the trigger is.
void CFuncMortarField::FieldUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	Vector vecStart;

	vecStart.x = RANDOM_FLOAT(v.mins.x, v.maxs.x);
	vecStart.y = RANDOM_FLOAT(v.mins.y, v.maxs.y);
	vecStart.z = v.maxs.z;

	switch (m_fControl)
	{
	case 0: // random
		break;
	case 1: // Trigger Activator
		if (pActivator != nullptr)
		{
			vecStart.x = pActivator->v.origin.x;
			vecStart.y = pActivator->v.origin.y;
		}
		break;
	case 2: // table
	{
		CBaseEntity* pController;

		if (!FStringNull(m_iszXController))
		{
			pController = util::FindEntityByTargetname(nullptr, STRING(m_iszXController));
			if (pController != nullptr)
			{
				vecStart.x = v.mins.x + pController->v.ideal_yaw * (v.size.x);
			}
		}
		if (!FStringNull(m_iszYController))
		{
			pController = util::FindEntityByTargetname(nullptr, STRING(m_iszYController));
			if (pController != nullptr)
			{
				vecStart.y = v.mins.y + pController->v.ideal_yaw * (v.size.y);
			}
		}
	}
	break;
	}

	EmitSound("weapons/mortar.wav", CHAN_VOICE, VOL_NORM, ATTN_NONE, RANDOM_LONG(95, 124));

	float t = 2.5;
	for (int i = 0; i < m_iCount; i++)
	{
		Vector vecSpot = vecStart;
		vecSpot.x += RANDOM_FLOAT(-m_flSpread, m_flSpread);
		vecSpot.y += RANDOM_FLOAT(-m_flSpread, m_flSpread);

		TraceResult tr;
		util::TraceLine(vecSpot, vecSpot + Vector(0, 0, -1) * 4096, util::ignore_monsters, this, &tr);

		Entity* pentOwner = nullptr;
		if (pActivator)
			pentOwner = pActivator->edict();

		CBaseEntity* pMortar = Create("monster_mortar", tr.vecEndPos, Vector(0, 0, 0), *pentOwner);
		pMortar->v.nextthink = gpGlobals->time + t;
		t += RANDOM_FLOAT(0.2, 0.5);
	}
}


class CMortar : public CGrenade
{
public:
	CMortar(Entity* containingEntity) : CGrenade(containingEntity) {}

	bool Spawn() override;
	void Precache() override;

	void EXPORT MortarExplode();

	int m_spriteTexture;
};

LINK_ENTITY_TO_CLASS(monster_mortar, CMortar);

bool CMortar::Spawn()
{
	v.movetype = MOVETYPE_NONE;
	v.solid = SOLID_NOT;

	v.dmg = 200;

	SetThink(&CMortar::MortarExplode);
	v.nextthink = 0;

	Precache();

	return true;
}


void CMortar::Precache()
{
	m_spriteTexture = PRECACHE_MODEL("sprites/lgtning.spr");
}

void CMortar::MortarExplode()
{
	// mortar beam
	MessageBegin(MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte(TE_BEAMPOINTS);
	WriteCoord(v.origin.x);
	WriteCoord(v.origin.y);
	WriteCoord(v.origin.z);
	WriteCoord(v.origin.x);
	WriteCoord(v.origin.y);
	WriteCoord(v.origin.z + 1024);
	WriteShort(m_spriteTexture);
	WriteByte(0);	 // framerate
	WriteByte(0);	 // framerate
	WriteByte(1);	 // life
	WriteByte(40);	 // width
	WriteByte(0);	 // noise
	WriteByte(255); // r, g, b
	WriteByte(160); // r, g, b
	WriteByte(100); // r, g, b
	WriteByte(128); // brightness
	WriteByte(0);	 // speed
	MessageEnd();

	TraceResult tr;
	util::TraceLine(v.origin + Vector(0, 0, 1024), v.origin - Vector(0, 0, 1024), util::dont_ignore_monsters, this, &tr);

	Explode(&tr, DMG_BLAST | DMG_MORTAR);
	util::ScreenShake(tr.vecEndPos, 25.0, 150.0, 1.0, 750);
}
