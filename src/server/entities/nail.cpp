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
	auto nail = Entity::Create<CNail>();

	nail->v.origin = origin;
	nail->v.angles = dir;
	nail->v.speed = 1000.0F;
	nail->v.dmg = damage;
	nail->v.armortype = DMG_NAIL | DMG_NEVERGIB;
	nail->v.owner = &owner->v;
	nail->v.team = owner->TeamNumber();
	nail->Spawn();

	if (damage > 9)
	{
		nail->v.classname = MAKE_STRING("supernails");
	}

	return nail;
}


CNail* CNail::CreateNailGrenadeNail(const Vector& origin, const Vector& dir, const float damage, CBaseEntity* owner)
{
	auto nail = Entity::Create<CNail>();

	nail->v.origin = origin;
	nail->v.angles = dir;
	nail->v.speed = 1000.0F;
	nail->v.dmg = damage;
	nail->v.armortype = DMG_NAIL | DMG_NEVERGIB;
	nail->v.dmg_inflictor = &owner->v;
	nail->v.team = owner->TeamNumber();
	nail->Spawn();

	nail->v.classname = MAKE_STRING("nailgrenade");

	return nail;
}


CNail* CNail::CreateTranquilizerDart(const Vector& origin, const Vector& dir, const float damage, CBaseEntity* owner)
{
	auto nail = Entity::Create<CNail>();

	nail->v.origin = origin;
	nail->v.angles = dir;
	nail->v.speed = 1500.0F;
	nail->v.dmg = damage;
	nail->v.armortype = DMG_NAIL | DMG_NEVERGIB | DMG_TRANQ;
	nail->v.owner = &owner->v;
	nail->v.team = owner->TeamNumber();
	nail->Spawn();

	nail->v.model = MAKE_STRING("models/crossbow_bolt.mdl");
	nail->v.modelindex = g_sModelIndexDart;

	return nail;
}


CNail* CNail::CreateLaser(const Vector& origin, const Vector& dir, const float damage, CBaseEntity* owner)
{
	auto nail = Entity::Create<CNail>();

	nail->v.origin = origin;
	nail->v.angles = dir;
	nail->v.speed = 1500.0F;
	nail->v.dmg = damage;
	nail->v.armortype = DMG_CLUB | DMG_NEVERGIB;
	nail->v.owner = &owner->v;
	nail->v.team = owner->TeamNumber();
	nail->Spawn();

	nail->v.classname = MAKE_STRING("railgun");

	return nail;
}


bool CNail::Spawn()
{
	v.classname = MAKE_STRING("nails");
	v.movetype = MOVETYPE_FLYMISSILE;
	v.solid = SOLID_TRIGGER; /* SOLID_BBOX */

	v.model = MAKE_STRING("models/nail.mdl");
	v.modelindex = g_sModelIndexNail;
	v.effects |= EF_NODRAW;

	SetSize(g_vecZero, g_vecZero);

	SetOrigin(v.origin);

	v.velocity = v.angles * v.speed;
	v.angles = util::VecToAngles(v.angles);

	SetTouch(&CNail::NailTouch);

	v.radsuit_finished = v.waterlevel;
	v.v_angle = v.velocity;

	SetThink(&CNail::PleaseGoInTheRightDirection);
	v.nextthink = gpGlobals->time + 1.0F / 30.0F;
	v.pain_finished = gpGlobals->time + 5.0F;

	return true;
}


void CNail::NailTouch(CBaseEntity* pOther)
{
	if (engine::PointContents(v.origin) != CONTENTS_SKY)
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

		if (util::DoDamageResponse(pOther, owner))
		{
			const auto dir = v.velocity.Normalize();

			MessageBegin(MSG_ONE_UNRELIABLE, gmsgBlood, owner);
			WriteFloat(dir.x);
			WriteFloat(dir.y);
			WriteFloat(dir.z);
			WriteByte(0);
			WriteByte((gpGlobals->trace_hitgroup == HITGROUP_HEAD) ? 1 : 0);
			WriteCoord(v.origin);
			MessageEnd();
		}

        pOther->TraceAttack(
            owner,
            v.dmg,
            v.velocity.Normalize(),
            gpGlobals->trace_hitgroup,
            v.armortype);

        pOther->ApplyMultiDamage(this, owner);

		/* Toodles: Have rail gun laser pass through players. */

        if (((int)v.armortype & DMG_CLUB) != 0 && pOther->IsClient())
        {
            /* Don't touch this guy again.*/
            m_touchedPlayers[pOther->v.GetIndex() - 1] = true;

            /* Fix velocity ASAP. */
            v.nextthink = gpGlobals->time + 0.001F;
            return;
        }
	}

	Remove();
}


void CNail::PleaseGoInTheRightDirection()
{
	if (v.pain_finished <= gpGlobals->time)
	{
		Remove();
		return;
	}

	v.velocity = v.v_angle;

	v.nextthink = gpGlobals->time + 1.0F / 30.0F;
}


bool CNail::ShouldCollide(CBaseEntity* other)
{
	if (((int)v.armortype & DMG_CLUB) != 0)
	{
    	return !other->IsClient() || !m_touchedPlayers[other->v.GetIndex() - 1];
	}

	return true;
}

