//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Whatever
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"


CRocket* CRocket::CreateRocket(const Vector& origin, const Vector& dir, const float damage, CBaseEntity* owner)
{
	auto rocket = GetClassPtr((CRocket*)nullptr);

	rocket->pev->origin = origin;
	rocket->pev->angles = dir;
	rocket->pev->dmg = damage;
	rocket->pev->owner = owner->edict();
	rocket->Spawn();

	return rocket;
}


bool CRocket::Spawn()
{
	pev->classname = MAKE_STRING("missile");
	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->solid = SOLID_TRIGGER; /* SOLID_BBOX */

	SetModel("models/rpgrocket.mdl");

	SetSize(g_vecZero, g_vecZero);

	SetOrigin(pev->origin);

	pev->velocity = pev->angles * 1000;
	pev->angles = util::VecToAngles(pev->angles);

	SetTouch(&CRocket::RocketTouch);

	SetThink(&CRocket::Remove);
	pev->nextthink = gpGlobals->time + 5.0F;

	tent::RocketTrail(this);

	return true;
}


void CRocket::RocketTouch(CBaseEntity* pOther)
{
	if (g_engfuncs.pfnPointContents(pev->origin) != CONTENT_SKY)
	{
		CBaseEntity* owner = this;

		if (pev->owner != nullptr)
		{
			owner = CBaseEntity::Instance(pev->owner);

			if (pOther == owner)
			{
				return;
			}
		}

		RadiusDamage(pev->origin, this, owner, pev->dmg, pev->dmg, DMG_BLAST);

		tent::Explosion(pev->origin, -gpGlobals->trace_plane_normal, pev->dmg);
	}

	Remove();
}
