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
#include "weapons.h"

LINK_ENTITY_TO_CLASS(grenade, CGrenade);

//
// Grenade Explode
//
void CGrenade::Explode(Vector vecSrc, Vector vecAim)
{
	TraceResult tr;
	util::TraceLine(pev->origin, pev->origin + Vector(0, 0, -32), util::ignore_monsters, this, &tr);

	Explode(&tr, DMG_BLAST);
}

void CGrenade::Explode(TraceResult* pTrace, int bitsDamageType)
{
	// Pull out of the wall a bit
	if (pTrace->flFraction != 1.0)
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * 0.6);
	}

	tent::Explosion(pev->origin, -pTrace->vecPlaneNormal, pev->dmg);

	CBaseEntity* owner;
	if (pev->owner)
		owner = CBaseEntity::Instance(pev->owner);
	else
		owner = this;

	pev->owner = nullptr; // can't traceline attack owner if this is set

	RadiusDamage(pev->origin, this, owner, pev->dmg, pev->dmg * 2.5, bitsDamageType);

	Remove();
}


void CGrenade::Detonate()
{
	TraceResult tr;
	Vector vecSpot; // trace starts here!

	vecSpot = pev->origin + Vector(0, 0, 8);
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

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	util::TraceLine(vecSpot, vecSpot + pev->velocity.Normalize() * 64, util::ignore_monsters, this, &tr);

	Explode(&tr, DMG_BLAST);
}


void CGrenade::BounceTouch(CBaseEntity* pOther)
{
	// don't hit the guy that launched this grenade
	if (pOther->edict() == pev->owner)
		return;

	if ((pev->flags & FL_ONGROUND) != 0)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.8;

		pev->sequence = RANDOM_LONG(1, 1);
		ResetSequenceInfo();
	}
	else
	{
		// play bounce sound
		BounceSound();
	}
	pev->framerate = pev->velocity.Length() / 200.0;
	if (pev->framerate > 1.0)
		pev->framerate = 1;
	else if (pev->framerate < 0.5)
		pev->framerate = 0;
}


void CGrenade::BounceSound()
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0: EmitSound("weapons/grenade_hit1.wav", CHAN_VOICE, 0.25F); break;
	case 1: EmitSound("weapons/grenade_hit2.wav", CHAN_VOICE, 0.25F); break;
	case 2: EmitSound("weapons/grenade_hit3.wav", CHAN_VOICE, 0.25F); break;
	}
}

void CGrenade::TumbleThink()
{
	if (!IsInWorld())
	{
		Remove();
		return;
	}

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->dmgtime <= gpGlobals->time)
	{
		SetThink(&CGrenade::Detonate);
	}
	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2;
	}
}


bool CGrenade::Spawn()
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->classname = MAKE_STRING("grenade");

	pev->solid = SOLID_BBOX;

	SetModel("models/grenade.mdl");
	SetSize(g_vecZero, g_vecZero);

	pev->dmg = 100;

	return true;
}


CGrenade* CGrenade::ShootContact(CBaseEntity* owner, Vector vecStart, Vector vecVelocity)
{
	CGrenade* pGrenade = GetClassPtr((CGrenade*)NULL);
	pGrenade->Spawn();
	// contact grenades arc lower
	pGrenade->pev->gravity = 0.5; // lower gravity since grenade is aerodynamic and engine doesn't know it.
	pGrenade->SetOrigin(vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = util::VecToAngles(pGrenade->pev->velocity);
	pGrenade->pev->owner = owner->edict();

	// Tumble in air
	pGrenade->pev->avelocity.x = RANDOM_FLOAT(-100, -500);

	// Explode on contact
	pGrenade->SetTouch(&CGrenade::ExplodeTouch);

	pGrenade->pev->dmg = 100;

	return pGrenade;
}

