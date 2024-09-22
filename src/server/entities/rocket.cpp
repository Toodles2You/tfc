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
	rocket->v.speed = 1000.0F;
	rocket->v.dmg = damageMax;
	rocket->v.dmg_save = damageMin;
	rocket->v.dmg_take = radius;
	rocket->v.owner = &owner->v;
	rocket->v.team = owner->TeamNumber();
	rocket->Spawn();

	tent::RocketTrail(rocket);

	return rocket;
}


CRocket* CRocket::CreateIncendiaryRocket(
	const Vector& origin,
	const Vector& dir,
	const float damageMax,
	const float damageMin,
	const float radius,
	CBaseEntity* owner)
{
	auto rocket = Entity::Create<CIncendiaryRocket>();

	rocket->v.origin = origin;
	rocket->v.angles = dir;
	rocket->v.speed = 600.0F;
	rocket->v.dmg = damageMax;
	rocket->v.dmg_save = damageMin;
	rocket->v.dmg_take = radius;
	rocket->v.owner = &owner->v;
	rocket->v.team = owner->TeamNumber();
	rocket->Spawn();

	tent::RocketTrail(rocket, true, rocket->v.team);

	return rocket;
}


CRocket* CRocket::CreateSentryRocket(
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
	rocket->v.speed = 800.0F;
	rocket->v.dmg = damageMax;
	rocket->v.dmg_save = damageMin;
	rocket->v.dmg_take = radius;
	rocket->v.owner = &owner->v;
	rocket->v.team = owner->TeamNumber();
	rocket->Spawn();

	rocket->v.classname = MAKE_STRING("building_sentrygun");

	tent::RocketTrail(rocket);

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

	v.velocity = v.angles * v.speed;
	v.angles = util::VecToAngles(v.angles);

	SetTouch(&CRocket::RocketTouch);

	v.radsuit_finished = v.waterlevel;
	v.v_angle = v.velocity;

	SetThink(&CRocket::PleaseGoInTheRightDirection);
	v.nextthink = gpGlobals->time + 1.0F / 30.0F;
	v.pain_finished = gpGlobals->time + 5.0F;

	v.air_finished = gpGlobals->time;

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


void CIncendiaryRocket::Explode(TraceResult* pTrace, int bitsDamageType)
{
	CBaseEntity* owner;
	if (v.owner)
		owner = v.owner->Get<CBaseEntity>();
	else
		owner = this;

	v.owner = nullptr; // can't traceline attack owner if this is set

	CBaseEntity* other;
	if (v.enemy)
		other = v.enemy->Get<CBaseEntity>();
	else
		other = nullptr;

	if (other != nullptr && other->v.takedamage != DAMAGE_NO)
	{
		/* Direct hit. Deal the maximum damage. */
		other->TakeDamage(this, owner, v.dmg, DMG_BURN | DMG_IGNITE | DMG_NO_KNOCKBACK);
	}

	// Pull out of the wall a bit
	if (pTrace->flFraction != 1.0)
	{
		v.origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * 0.6);
	}

	tent::Explosion(v.origin, -pTrace->vecPlaneNormal, tent::ExplosionType::Incendiary);

	CBaseEntity* entity = nullptr;
	TraceResult tr;

	while ((entity = util::FindEntityInSphere(entity, v.origin, v.dmg_take)) != nullptr)
	{
		/* Ignore the entity that was hit. */
		if (entity->v.takedamage == DAMAGE_NO || entity == other)
		{
			continue;
		}

		util::TraceLine(v.origin, entity->EyePosition(), &tr, this, util::kTraceBox);

		/* Toodles: Check wall piercing. Pass if the fraction is less than about one-third of the radius. */
		if (tr.flFraction != 1.0F && tr.flFraction > 0.35F && tr.pHit != &entity->v)
		{
			continue;
		}

		/* Deal the minimum damage. */
		entity->TakeDamage(this, owner,  v.dmg_save, DMG_BURN | DMG_IGNITE | DMG_RESIST_SELF | DMG_NO_KNOCKBACK);
	}

	Remove();
}

