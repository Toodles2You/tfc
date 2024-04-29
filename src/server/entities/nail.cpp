//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Whatever
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "UserMessages.h"
#include "gamerules.h"


CNail* CNail::CreateNail(const Vector& origin, const Vector& dir, const float damage, CBaseEntity* owner)
{
	auto nail = GetClassPtr((CNail*)nullptr);

	nail->pev->origin = origin;
	nail->pev->angles = dir;
	nail->pev->dmg = damage;
	nail->pev->owner = owner->edict();
	nail->Spawn();

	if (damage > 9)
	{
		nail->pev->classname = MAKE_STRING("supernails");
	}

	return nail;
}


CNail* CNail::CreateNailGrenadeNail(const Vector& origin, const Vector& dir, const float damage, CBaseEntity* owner)
{
	auto nail = GetClassPtr((CNail*)nullptr);

	nail->pev->origin = origin;
	nail->pev->angles = dir;
	nail->pev->dmg = damage;
	nail->pev->dmg_inflictor = owner->edict();
	nail->Spawn();

	nail->pev->classname = MAKE_STRING("nailgrenade");

	return nail;
}


bool CNail::Spawn()
{
	pev->classname = MAKE_STRING("nails");
	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->solid = SOLID_TRIGGER; /* SOLID_BBOX */

	pev->model = MAKE_STRING("models/nail.mdl");
	pev->modelindex = g_sModelIndexNail;
	pev->effects |= EF_NODRAW;

	SetSize(g_vecZero, g_vecZero);

	SetOrigin(pev->origin);

	pev->velocity = pev->angles * 1000;
	pev->angles = util::VecToAngles(pev->angles);

	SetTouch(&CNail::NailTouch);

	pev->radsuit_finished = pev->waterlevel;
	pev->v_angle = pev->velocity;

	SetThink(&CNail::PleaseGoInTheRightDirection);
	pev->nextthink = gpGlobals->time + 1.0F / 30.0F;
	pev->pain_finished = gpGlobals->time + 5.0F;

	return true;
}


void CNail::NailTouch(CBaseEntity* pOther)
{
	if (g_engfuncs.pfnPointContents(pev->origin) != CONTENTS_SKY)
	{
		CBaseEntity* owner = this;

		if (pev->dmg_inflictor != nullptr)
		{
			owner = CBaseEntity::Instance(pev->dmg_inflictor);
		}
		else if (pev->owner != nullptr)
		{
			owner = CBaseEntity::Instance(pev->owner);

			if (pOther == owner)
			{
				return;
			}
		}

        pOther->TraceAttack(
            owner,
            pev->dmg,
            pev->velocity.Normalize(),
            gpGlobals->trace_hitgroup,
            DMG_NAIL | DMG_NEVERGIB);

        pOther->ApplyMultiDamage(this, owner);
	}

	Remove();
}


void CNail::PleaseGoInTheRightDirection()
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

