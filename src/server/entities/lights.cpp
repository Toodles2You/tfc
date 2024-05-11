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

===== lights.cpp ========================================================

  spawn and think functions for editor-placed lights

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#define SF_LIGHT_START_OFF 1


class CLight : public CPointEntity
{
public:
	CLight(Entity* containingEntity) : CPointEntity(containingEntity) {}

	DECLARE_SAVERESTORE()

	bool KeyValue(KeyValueData* pkvd) override;
	bool Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

private:
	int m_iStyle;
	int m_iszPattern;
};
LINK_ENTITY_TO_CLASS(light, CLight);

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CLight)
	DEFINE_FIELD(CLight, m_iStyle, FIELD_INTEGER),
	DEFINE_FIELD(CLight, m_iszPattern, FIELD_STRING),
END_SAVERESTORE(CLight, CPointEntity)
#endif

//
// Cache user-entity-field values until spawn is called.
//
bool CLight::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "style"))
	{
		m_iStyle = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "pitch"))
	{
		v.angles.x = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "pattern"))
	{
		m_iszPattern = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}


bool CLight::Spawn()
{
	if (FStringNull(v.targetname))
	{
		return false;
	}

	if (m_iStyle >= 32)
	{
		if (FBitSet(v.spawnflags, SF_LIGHT_START_OFF))
			LIGHT_STYLE(m_iStyle, "a");
		else if (!FStringNull(m_iszPattern))
			LIGHT_STYLE(m_iStyle, (char*)STRING(m_iszPattern));
		else
			LIGHT_STYLE(m_iStyle, "m");
	}

	return true;
}


void CLight::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (m_iStyle >= 32)
	{
		if (!ShouldToggle(useType, !FBitSet(v.spawnflags, SF_LIGHT_START_OFF)))
			return;

		if (FBitSet(v.spawnflags, SF_LIGHT_START_OFF))
		{
			if (!FStringNull(m_iszPattern))
				LIGHT_STYLE(m_iStyle, (char*)STRING(m_iszPattern));
			else
				LIGHT_STYLE(m_iStyle, "m");
			ClearBits(v.spawnflags, SF_LIGHT_START_OFF);
		}
		else
		{
			LIGHT_STYLE(m_iStyle, "a");
			SetBits(v.spawnflags, SF_LIGHT_START_OFF);
		}
	}
}

//
// shut up spawn functions for new spotlights
//
LINK_ENTITY_TO_CLASS(light_spot, CLight);


class CEnvLight : public CLight
{
public:
	CEnvLight(Entity* containingEntity) : CLight(containingEntity) {}

	bool KeyValue(KeyValueData* pkvd) override;
	bool Spawn() override;
};

LINK_ENTITY_TO_CLASS(light_environment, CEnvLight);

bool CEnvLight::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "_light"))
	{
		int r = 0, g = 0, b = 0, v = 0;
		char szColor[64];
		const int j = sscanf(pkvd->szValue, "%d %d %d %d\n", &r, &g, &b, &v);
		if (j == 1)
		{
			g = b = r;
		}
		else if (j == 4)
		{
			r = r * (v / 255.0);
			g = g * (v / 255.0);
			b = b * (v / 255.0);
		}

		// simulate qrad direct, ambient,and gamma adjustments, as well as engine scaling
		r = pow(r / 114.0, 0.6) * 264;
		g = pow(g / 114.0, 0.6) * 264;
		b = pow(b / 114.0, 0.6) * 264;

		sprintf(szColor, "%d", r);
		CVAR_SET_STRING("sv_skycolor_r", szColor);
		sprintf(szColor, "%d", g);
		CVAR_SET_STRING("sv_skycolor_g", szColor);
		sprintf(szColor, "%d", b);
		CVAR_SET_STRING("sv_skycolor_b", szColor);

		return true;
	}

	return CLight::KeyValue(pkvd);
}


bool CEnvLight::Spawn()
{
	char szVector[64];
	util::MakeAimVectors(v.angles);

	sprintf(szVector, "%f", gpGlobals->v_forward.x);
	CVAR_SET_STRING("sv_skyvec_x", szVector);
	sprintf(szVector, "%f", gpGlobals->v_forward.y);
	CVAR_SET_STRING("sv_skyvec_y", szVector);
	sprintf(szVector, "%f", gpGlobals->v_forward.z);
	CVAR_SET_STRING("sv_skyvec_z", szVector);

	return CLight::Spawn();
}
