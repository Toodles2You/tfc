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


CFlame* CFlame::CreateFlame(const Vector& origin, const Vector& dir, const float damage, CBaseEntity* owner)
{
	auto nail = Entity::Create<CFlame>();

	nail->v.origin = origin;
	nail->v.angles = dir;
	nail->v.speed = 600.0F;
	nail->v.dmg = damage;
	nail->v.armortype = DMG_BURN | DMG_IGNITE;
	nail->v.owner = &owner->v;
	nail->v.team = owner->TeamNumber();
	nail->Spawn();

	return nail;
}


bool CFlame::Spawn()
{
	v.classname = MAKE_STRING("flames");
	v.movetype = MOVETYPE_FLYMISSILE;
	v.solid = SOLID_TRIGGER; /* SOLID_BBOX */

	v.model = MAKE_STRING("sprites/fthrow.spr");
	v.modelindex = g_sModelIndexFlame;
	v.effects |= EF_NODRAW;

	SetSize(g_vecZero, g_vecZero);

	SetOrigin(v.origin);

	v.velocity = v.angles * v.speed;
	v.angles = util::VecToAngles(v.angles);

	SetTouch(&CFlame::FlameTouch);

	v.radsuit_finished = v.waterlevel;
	v.v_angle = v.velocity;

	SetThink(&CFlame::PleaseDoNotGoInTheWater);
	v.nextthink = gpGlobals->time + 1.0F / 30.0F;
	v.pain_finished = gpGlobals->time + 1.0F;

	return true;
}


void CFlame::FlameTouch(CBaseEntity* pOther)
{
	if (engine::PointContents(v.origin) != CONTENTS_SKY
     && v.waterlevel == kWaterLevelNone)
	{
		CBaseEntity* owner = this;

		if (v.dmg_inflictor != nullptr)
		{
			owner = v.dmg_inflictor->Get<CBaseEntity>();
		}
		else if (v.owner != nullptr)
		{
			owner = v.owner->Get<CBaseEntity>();

			if (pOther == owner)
			{
				return;
			}
		}

        pOther->TraceAttack(
            owner,
            v.dmg,
            v.velocity.Normalize(),
            gpGlobals->trace_hitgroup,
            v.armortype);

        pOther->ApplyMultiDamage(this, owner);
	}

	Remove();
}


void CFlame::PleaseDoNotGoInTheWater()
{
	if (v.pain_finished <= gpGlobals->time
	 || v.waterlevel != kWaterLevelNone)
	{
		Remove();
		return;
	}
	v.nextthink = gpGlobals->time + 1.0F / 30.0F;
}

