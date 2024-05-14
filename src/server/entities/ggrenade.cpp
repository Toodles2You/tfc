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

===== generic grenade.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "UserMessages.h"

LINK_ENTITY_TO_CLASS(grenade, CGrenade);

//
// Grenade Explode
//
void CGrenade::Explode(Vector vecSrc, Vector vecAim)
{
	TraceResult tr;
	util::TraceLine(v.origin, v.origin + Vector(0, 0, -32), util::ignore_monsters, this, &tr);

	Explode(&tr, DMG_BLAST);
}

void CGrenade::Explode(TraceResult* pTrace, int bitsDamageType)
{
	// Pull out of the wall a bit
	if (pTrace->flFraction != 1.0)
	{
		v.origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * 0.6);
	}

	tent::Explosion(v.origin, -pTrace->vecPlaneNormal, v.dmg);

	CBaseEntity* owner;
	if (v.owner)
		owner = v.owner->Get<CBaseEntity>();
	else
		owner = this;

	v.owner = nullptr; // can't traceline attack owner if this is set

	RadiusDamage(v.origin, this, owner, v.dmg, v.dmg * 2.5, bitsDamageType);

	Remove();
}


void CGrenade::Detonate()
{
	TraceResult tr;
	Vector vecSpot; // trace starts here!

	vecSpot = v.origin + Vector(0, 0, 8);
	util::TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), util::ignore_monsters, this, &tr);

	Explode(&tr, DMG_BLAST);
}


//
// Contact grenade, explode when it touches something
//
void CGrenade::ExplodeTouch(CBaseEntity* pOther)
{
	TraceResult tr;
	Vector vecSpot; // trace starts here!

	v.enemy = &pOther->v;

	vecSpot = v.origin - v.velocity.Normalize() * 32;
	util::TraceLine(vecSpot, vecSpot + v.velocity.Normalize() * 64, util::ignore_monsters, this, &tr);

	Explode(&tr, DMG_BLAST);
}


void CGrenade::BounceTouch(CBaseEntity* pOther)
{
	// don't hit the guy that launched this grenade
	if (&pOther->v == v.owner)
		return;

	if ((v.flags & FL_ONGROUND) != 0)
	{
		// add a bit of static friction
		v.velocity = v.velocity * 0.8;

		v.sequence = engine::RandomLong(1, 1);
		ResetSequenceInfo();
	}
	else
	{
		if (v.velocity.z > 0
		 && util::GetGlobalTrace().vecPlaneNormal.z >= kGroundPlaneMinZ)
		{
			v.velocity = v.velocity * 0.8;
		}

		// play bounce sound
		BounceSound();
	}
	v.framerate = v.velocity.Length() / 200.0;
	if (v.framerate > 1.0)
		v.framerate = 1;
	else if (v.framerate < 0.5)
		v.framerate = 0;
}


void CGrenade::BounceSound()
{
	switch (engine::RandomLong(0, 2))
	{
	case 0: EmitSound("weapons/grenade_hit1.wav", CHAN_VOICE, 0.25F); break;
	case 1: EmitSound("weapons/grenade_hit2.wav", CHAN_VOICE, 0.25F); break;
	case 2: EmitSound("weapons/grenade_hit3.wav", CHAN_VOICE, 0.25F); break;
	}
}

void CGrenade::TumbleThink()
{
	StudioFrameAdvance();
	v.nextthink = gpGlobals->time + 0.1;

	if (v.dmgtime <= gpGlobals->time)
	{
		SetThink(&CGrenade::Detonate);
	}
	if (v.waterlevel > kWaterLevelNone)
	{
		v.velocity = v.velocity * 0.5;
		v.framerate = 0.2;
	}
	else
	{
		v.framerate = 1.0;
	}
}


bool CGrenade::Spawn()
{
	v.movetype = MOVETYPE_BOUNCE;
	v.classname = MAKE_STRING("grenade");

	v.solid = SOLID_BBOX;

	SetModel("models/grenade.mdl");
	SetSize(g_vecZero, g_vecZero);

	v.dmg = 100;

	return true;
}


CGrenade* CGrenade::ShootContact(CBaseEntity* owner, Vector vecStart, Vector vecVelocity)
{
	CGrenade* pGrenade = Entity::Create<CGrenade>();
	pGrenade->Spawn();
	// contact grenades arc lower
	pGrenade->v.gravity = 0.5; // lower gravity since grenade is aerodynamic and engine doesn't know it.
	pGrenade->SetOrigin(vecStart);
	pGrenade->v.velocity = vecVelocity;
	pGrenade->v.angles = util::VecToAngles(pGrenade->v.velocity);
	pGrenade->v.owner = &owner->v;

	// Tumble in air
	pGrenade->v.avelocity.x = engine::RandomFloat(-100, -500);

	// Explode on contact
	pGrenade->SetTouch(&CGrenade::ExplodeTouch);

	pGrenade->v.dmg = 100;

	return pGrenade;
}


#ifdef HALFLIFE_GRENADES

bool CPrimeGrenade::Spawn()
{
	v.classname = MAKE_STRING("grenade");
	v.movetype = MOVETYPE_NONE;
	v.solid = SOLID_NOT;

	SetSize(g_vecZero, g_vecZero);

	SetOrigin(v.owner->origin);

	SetThink(&CPrimeGrenade::PrimedThink);
	v.nextthink = gpGlobals->time + 0.8;
	v.dmgtime = v.nextthink + 3.0;

	v.dmg = 100;

	return true;
}


void CPrimeGrenade::PrimedThink()
{
	auto owner = v.owner->Get<CBasePlayer>();

	if (owner == nullptr)
	{
		Remove();
		return;
	}

	SetOrigin(owner->v.origin);

	if (!owner->IsAlive())
	{
		Throw(kDrop);
		return;
	}

	if (!owner->IsPlayer())
	{
		Remove();
		return;
	}

	if (!owner->InState(CBasePlayer::State::GrenadeThrowing))
	{
		if (v.dmgtime <= gpGlobals->time)
		{
			Throw(kOvercook);
			return;
		}
		v.nextthink = gpGlobals->time + 0.1;
		return;
	}

	Throw(kThrow);
}


void CPrimeGrenade::Throw(throw_e mode)
{
	auto owner = v.owner->Get<CBasePlayer>();

	v.movetype = MOVETYPE_BOUNCE;
	v.solid = SOLID_BBOX;

	SetOrigin(owner->v.origin);
	v.angles = Vector(0, owner->v.angles.y, 0);

	if (mode == kOvercook)
	{
		v.velocity = g_vecZero;

		SetThink(&CPrimeGrenade::Detonate);
		v.nextthink = gpGlobals->time;
	}
	else
	{
		v.gravity = 0.81;
		v.friction = 0.6;

		SetModel("models/w_grenade.mdl");
		SetSize(g_vecZero, g_vecZero);

		if (mode == kDrop)
		{
			v.velocity = Vector(0, 0, 30);
		}
		else
		{
			util::MakeVectors(owner->v.v_angle);
			v.velocity = gpGlobals->v_forward * 600 + gpGlobals->v_up * 200;

			EmitSound("weapons/grenade.wav", CHAN_BODY, VOL_NORM, ATTN_IDLE);
		}

		SetTouch(&CPrimeGrenade::BounceTouch);
		SetThink(&CPrimeGrenade::TumbleThink);
		v.nextthink = gpGlobals->time + 0.1;

		v.sequence = engine::RandomLong(3, 6);
		v.framerate = 1.0;
		ResetSequenceInfo();
	}

	MessageBegin(MSG_ONE, gmsgStatusIcon, owner);
	WriteByte(0);
	WriteString("grenade");
	MessageEnd();
	
	owner->LeaveState(CBasePlayer::State::Grenade);
}


CPrimeGrenade* CPrimeGrenade::PrimeGrenade(CBaseEntity* owner)
{
	auto grenade = Entity::Create<CPrimeGrenade>();

	grenade->v.owner = &owner->v;
	grenade->Spawn();

	return grenade;
}

#endif

