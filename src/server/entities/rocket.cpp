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


CRocket* CRocket::CreateRocket(
	const Vector& origin,
	const Vector& dir,
	const float damageMax,
	const float damageMin,
	const float radius,
	CBaseEntity* owner)
{
	auto rocket = GetClassPtr((CRocket*)nullptr);

	rocket->pev->origin = origin;
	rocket->pev->angles = dir;
	rocket->pev->dmg = damageMax;
	rocket->pev->dmg_save = damageMin;
	rocket->pev->dmg_take = radius;
	rocket->pev->owner = owner->edict();
	rocket->pev->team = owner->TeamNumber();
	rocket->Spawn();

	return rocket;
}


bool CRocket::Spawn()
{
	pev->classname = MAKE_STRING("rocket");
	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->solid = SOLID_TRIGGER; /* SOLID_BBOX */

	SetModel("models/rpgrocket.mdl");

	SetSize(g_vecZero, g_vecZero);

	SetOrigin(pev->origin);

	pev->velocity = pev->angles * 1000;
	pev->angles = util::VecToAngles(pev->angles);

	SetTouch(&CRocket::RocketTouch);

	pev->radsuit_finished = pev->waterlevel;
	pev->v_angle = pev->velocity;

	SetThink(&CRocket::PleaseGoInTheRightDirection);
	pev->nextthink = gpGlobals->time + 1.0F / 30.0F;
	pev->pain_finished = gpGlobals->time + 5.0F;

	pev->air_finished = gpGlobals->time;

	tent::RocketTrail(this);

	return true;
}


void CRocket::RocketTouch(CBaseEntity* pOther)
{
	if (g_engfuncs.pfnPointContents(pev->origin) != CONTENTS_SKY)
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

		ExplodeTouch(pOther);
	}

	Remove();
}


void CRocket::PleaseGoInTheRightDirection()
{
	if (pev->pain_finished <= gpGlobals->time)
	{
		Remove();
		return;
	}
	if (pev->waterlevel > pev->radsuit_finished)
	{
		pev->velocity = pev->v_angle;
		pev->radsuit_finished = pev->waterlevel;
	}
	pev->nextthink = gpGlobals->time + 1.0F / 30.0F;
}

