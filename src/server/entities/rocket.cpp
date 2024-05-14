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
	auto rocket = Entity::Create<CRocket>();

	rocket->v.origin = origin;
	rocket->v.angles = dir;
	rocket->v.dmg = damageMax;
	rocket->v.dmg_save = damageMin;
	rocket->v.dmg_take = radius;
	rocket->v.owner = &owner->v;
	rocket->v.team = owner->TeamNumber();
	rocket->Spawn();

	return rocket;
}


bool CRocket::Spawn()
{
	v.classname = MAKE_STRING("rocket");
	v.movetype = MOVETYPE_FLYMISSILE;
	v.solid = SOLID_TRIGGER; /* SOLID_BBOX */

	SetModel("models/rpgrocket.mdl");

	SetSize(g_vecZero, g_vecZero);

	SetOrigin(v.origin);

	v.velocity = v.angles * 1000;
	v.angles = util::VecToAngles(v.angles);

	SetTouch(&CRocket::RocketTouch);

	v.radsuit_finished = v.waterlevel;
	v.v_angle = v.velocity;

	SetThink(&CRocket::PleaseGoInTheRightDirection);
	v.nextthink = gpGlobals->time + 1.0F / 30.0F;
	v.pain_finished = gpGlobals->time + 5.0F;

	v.air_finished = gpGlobals->time;

	tent::RocketTrail(this);

	return true;
}


void CRocket::RocketTouch(CBaseEntity* pOther)
{
	if (engine::PointContents(v.origin) != CONTENTS_SKY)
	{
		CBaseEntity* owner = this;

		if (v.owner != nullptr)
		{
			owner = v.owner->Get<CBaseEntity>();

			if (pOther == owner)
			{
				return;
			}
		}

		if (pOther->v.takedamage != DAMAGE_NO && pOther->IsBSPModel())
		{
			pOther->TakeDamage(this, owner, v.dmg, DMG_BLAST);
		}

		ExplodeTouch(pOther);
	}

	Remove();
}


void CRocket::PleaseGoInTheRightDirection()
{
	if (v.pain_finished <= gpGlobals->time)
	{
		Remove();
		return;
	}
	if (v.waterlevel > v.radsuit_finished)
	{
		v.velocity = v.v_angle;
		v.radsuit_finished = v.waterlevel;
	}
	v.nextthink = gpGlobals->time + 1.0F / 30.0F;
}

