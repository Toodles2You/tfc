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

===== combat.cpp ========================================================

  functions dealing with damage infliction & death

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "decals.h"
#include "animation.h"
#include "weapons.h"
#include "func_break.h"
#include "player.h"
#include "UserMessages.h"

//
// RadiusDamage - this entity is exploding, or otherwise needs to inflict damage upon entities within a certain range.
//
// only damage ents that can clearly be seen by the explosion!


void RadiusDamage(Vector vecSrc, CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType)
{
	CBaseEntity* pEntity = NULL;
	TraceResult tr;
	float flAdjustedDamage, falloff;
	Vector vecSpot;

	if (0 != flRadius)
		falloff = flDamage / flRadius;
	else
		falloff = 1.0;

	const bool bInWater = (util::PointContents(vecSrc) == CONTENTS_WATER);

	vecSrc.z += 1; // in case grenade is lying on the ground

	// iterate on all entities in the vicinity.
	while ((pEntity = util::FindEntityInSphere(pEntity, vecSrc, flRadius)) != NULL)
	{
		if (pEntity->pev->takedamage != DAMAGE_NO)
		{
			if (iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore)
			{ // houndeyes don't hurt other houndeyes with their attack
				continue;
			}

			// blast's don't tavel into or out of water
			if (bInWater && pEntity->pev->waterlevel == 0)
				continue;
			if (!bInWater && pEntity->pev->waterlevel == 3)
				continue;

			vecSpot = pEntity->BodyTarget(vecSrc);

			util::TraceLine(vecSrc, vecSpot, util::dont_ignore_monsters, inflictor, &tr);

			if (tr.flFraction == 1.0 || tr.pHit == pEntity->edict())
			{ // the explosion can 'see' this entity, so hurt them!
				if (0 != tr.fStartSolid)
				{
					// if we're stuck inside them, fixup the position and distance
					tr.vecEndPos = vecSrc;
					tr.flFraction = 0.0;
				}

				// decrease damage for an ent that's farther from the bomb.
				flAdjustedDamage = (vecSrc - tr.vecEndPos).Length() * falloff;
				flAdjustedDamage = flDamage - flAdjustedDamage;

				if (flAdjustedDamage < 0)
				{
					flAdjustedDamage = 0;
				}

				// ALERT( at_console, "hit %s\n", STRING( pEntity->pev->classname ) );
				if (tr.flFraction != 1.0)
				{
					ClearMultiDamage();
					pEntity->TraceAttack(inflictor, flAdjustedDamage, (tr.vecEndPos - vecSrc).Normalize(), &tr, bitsDamageType);
					ApplyMultiDamage(inflictor, attacker);
				}
				else
				{
					pEntity->TakeDamage(inflictor, attacker, flAdjustedDamage, bitsDamageType);
				}
			}
		}
	}
}

//=========================================================
// FInViewCone - returns true is the passed ent is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall.
//=========================================================
bool CBaseEntity::FInViewCone(CBaseEntity* pEntity)
{
	Vector2D vec2LOS;
	float flDot;

	util::MakeVectors(pev->angles);

	vec2LOS = (pEntity->pev->origin - pev->origin).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct(vec2LOS, gpGlobals->v_forward.Make2D());

	if (flDot > m_flFieldOfView)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//=========================================================
// FInViewCone - returns true is the passed vector is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall.
//=========================================================
bool CBaseEntity::FInViewCone(const Vector& vecOrigin)
{
	Vector2D vec2LOS;
	float flDot;

	util::MakeVectors(pev->angles);

	vec2LOS = (vecOrigin - pev->origin).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct(vec2LOS, gpGlobals->v_forward.Make2D());

	if (flDot > m_flFieldOfView)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the target
//=========================================================
bool CBaseEntity::FVisible(CBaseEntity* pEntity)
{
	TraceResult tr;
	Vector vecLookerOrigin;
	Vector vecTargetOrigin;

	if (FBitSet(pEntity->pev->flags, FL_NOTARGET))
		return false;

	// don't look through water
	if ((pev->waterlevel != 3 && pEntity->pev->waterlevel == 3) || (pev->waterlevel == 3 && pEntity->pev->waterlevel == 0))
		return false;

	vecLookerOrigin = pev->origin + pev->view_ofs; //look through the caller's 'eyes'
	vecTargetOrigin = pEntity->EyePosition();

	util::TraceLine(vecLookerOrigin, vecTargetOrigin, util::ignore_monsters, util::ignore_glass, this, &tr);

	if (tr.flFraction != 1.0)
	{
		return false; // Line of sight is not established
	}
	else
	{
		return true; // line of sight is valid.
	}
}

//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the target vector
//=========================================================
bool CBaseEntity::FVisible(const Vector& vecOrigin)
{
	TraceResult tr;
	Vector vecLookerOrigin;

	vecLookerOrigin = EyePosition(); //look through the caller's 'eyes'

	util::TraceLine(vecLookerOrigin, vecOrigin, util::ignore_monsters, util::ignore_glass, this, &tr);

	if (tr.flFraction != 1.0)
	{
		return false; // Line of sight is not established
	}
	else
	{
		return true; // line of sight is valid.
	}
}

/*
================
TraceAttack
================
*/
void CBaseEntity::TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	Vector vecOrigin = ptr->vecEndPos - vecDir * 4;

	if (pev->takedamage != DAMAGE_NO)
	{
		AddMultiDamage(attacker, attacker, this, flDamage, bitsDamageType);

		int blood = BloodColor();

		if (blood != DONT_BLEED)
		{
			SpawnBlood(vecOrigin, blood, flDamage); // a little surface blood.
			TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
		}
	}
}

void CBasePlayer::FireBullets(
	const float damage,
	const Vector2D& spread,
	const unsigned int count,
	const float distance)
{
	const auto gun = pev->origin + pev->view_ofs;
	const auto aim = pev->v_angle + pev->punchangle;

	auto traceHits = 0;
	auto traceFlags = 0;
	auto traceEndPos = static_cast<Vector *>(alloca(count * sizeof(Vector)));

	ClearMultiDamage();
	gMultiDamage.type = DMG_BULLET | DMG_AIMED | DMG_NEVERGIB;

	for (auto i = 0; i < count; i++)
	{
		const Vector2D spreadScale
		{
			util::SharedRandomFloat(random_seed + i * 4, -0.5, 0.5)
				+ util::SharedRandomFloat(random_seed + 1 + i * 4, -0.5, 0.5),
			util::SharedRandomFloat(random_seed + 2 + i * 4, -0.5, 0.5)
				+ util::SharedRandomFloat(random_seed + 3 + i * 4, -0.5, 0.5)
		};

		const Vector angles
		{
			aim.x + spread.y * 0.5 * spreadScale.x,
			aim.y + spread.x * 0.5 * spreadScale.y,
			aim.z,
		};

		Vector dir;
		AngleVectors(angles, &dir, nullptr, nullptr);

		TraceResult tr;
		util::TraceLine(gun, gun + dir * distance, util::dont_ignore_monsters, this, &tr);
		
		if (tr.flFraction != 1.0F)
		{
			const auto hit =
				reinterpret_cast<CBasePlayer *>(CBaseEntity::Instance(tr.pHit));
			
			hit->TraceAttack(
				this,
				damage,
				dir,
				&tr,
				DMG_BULLET | DMG_AIMED | DMG_NEVERGIB);
			
			if ((hit->pev->flags & FL_CLIENT) != 0)
			{
				traceEndPos[traceHits] = tr.vecEndPos;
				traceHits++;
				if (hit->m_LastHitGroup == HITGROUP_HEAD)
				{
					traceFlags |= 1 << i;
				}
			}
		}
	}
	
	ApplyMultiDamage(this, this);

	Vector dir;
	AngleVectors(aim, &dir, nullptr, nullptr);
	
	if (traceHits != 0)
	{
		MessageBegin(MSG_PVS, gmsgBlood, gun);
		WriteFloat(dir.x);
		WriteFloat(dir.y);
		WriteFloat(dir.z);
		WriteByte(traceHits - 1);
		WriteByte(traceFlags);
		for (auto i = 0; i < traceHits; i++)
		{
			WriteCoord(traceEndPos[i].x);
			WriteCoord(traceEndPos[i].y);
			WriteCoord(traceEndPos[i].z);
		}
		MessageEnd();
	}
}

void CBaseEntity::TraceBleed(float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	if (BloodColor() == DONT_BLEED)
		return;

	if (flDamage == 0)
		return;

	if ((bitsDamageType & (DMG_CRUSH | DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB | DMG_MORTAR)) == 0)
		return;

	// make blood decal on the wall!
	TraceResult Bloodtr;
	Vector vecTraceDir;
	float flNoise;
	int cCount;
	int i;

	/*
	if ( !IsAlive() )
	{
		// dealing with a dead monster. 
		if ( pev->max_health <= 0 )
		{
			// no blood decal for a monster that has already decalled its limit.
			return; 
		}
		else
		{
			pev->max_health--;
		}
	}
*/

	if (flDamage < 10)
	{
		flNoise = 0.1;
		cCount = 1;
	}
	else if (flDamage < 25)
	{
		flNoise = 0.2;
		cCount = 2;
	}
	else
	{
		flNoise = 0.3;
		cCount = 4;
	}

	for (i = 0; i < cCount; i++)
	{
		vecTraceDir = vecDir * -1; // trace in the opposite direction the shot came from (the direction the shot is going)

		vecTraceDir.x += RANDOM_FLOAT(-flNoise, flNoise);
		vecTraceDir.y += RANDOM_FLOAT(-flNoise, flNoise);
		vecTraceDir.z += RANDOM_FLOAT(-flNoise, flNoise);

		util::TraceLine(ptr->vecEndPos, ptr->vecEndPos + vecTraceDir * -172, util::ignore_monsters, this, &Bloodtr);

		if (Bloodtr.flFraction != 1.0)
		{
			util::BloodDecalTrace(&Bloodtr, BloodColor());
		}
	}
}

//=========================================================
// Look - Base class monster function to find enemies or
// food by sight. iDistance is distance ( in units ) that the
// monster can see.
//
// Sets the sight bits of the m_afConditions mask to indicate
// which types of entities were sighted.
// Function also sets the Looker's m_pLink
// to the head of a link list that contains all visible ents.
// (linked via each ent's m_pLink field)
//
//=========================================================
void CBaseEntity::Look(int iDistance)
{
	// int iSighted = 0;

	// DON'T let visibility information from last frame sit around!
	// ClearConditions(bits_COND_SEE_HATE | bits_COND_SEE_DISLIKE | bits_COND_SEE_ENEMY | bits_COND_SEE_FEAR | bits_COND_SEE_NEMESIS | bits_COND_SEE_CLIENT);

	m_pLink = NULL;

	CBaseEntity* pSightEnt = NULL; // the current visible entity that we're dealing with

	// See no evil if prisoner is set
	CBaseEntity* pList[100];

	Vector delta = Vector(iDistance, iDistance, iDistance);

	// Find only monsters/clients in box, NOT limited to PVS
	int count = util::EntitiesInBox(pList, 100, pev->origin - delta, pev->origin + delta, FL_CLIENT | FL_MONSTER);
	for (int i = 0; i < count; i++)
	{
		pSightEnt = pList[i];
		// !!!temporarily only considering other monsters and clients, don't see prisoners
		if (pSightEnt != this &&
			pSightEnt->pev->health > 0)
		{
			// the looker will want to consider this entity
			// don't check anything else about an entity that can't be seen, or an entity that you don't care about.
			if (IRelationship(pSightEnt) != R_NO && FInViewCone(pSightEnt) && !FBitSet(pSightEnt->pev->flags, FL_NOTARGET) && FVisible(pSightEnt))
			{
				pSightEnt->m_pLink = m_pLink;
				m_pLink = pSightEnt;
			}
		}
	}
}

//=========================================================
// BestVisibleEnemy - this functions searches the link
// list whose head is the caller's m_pLink field, and returns
// a pointer to the enemy entity in that list that is nearest the
// caller.
//=========================================================
CBaseEntity* CBaseEntity::BestVisibleEnemy()
{
	CBaseEntity* pReturn;
	CBaseEntity* pNextEnt;
	int iNearest;
	int iDist;
	int iBestRelationship;

	iNearest = 8192; // so first visible entity will become the closest.
	pNextEnt = m_pLink;
	pReturn = NULL;
	// iBestRelationship = R_NO;

	while (pNextEnt != NULL)
	{
		if (pNextEnt->IsAlive())
		{
			if (IRelationship(pNextEnt) > iBestRelationship)
			{
				// this entity is disliked MORE than the entity that we
				// currently think is the best visible enemy. No need to do
				// a distance check, just get mad at this one for now.
				iBestRelationship = IRelationship(pNextEnt);
				iNearest = (pNextEnt->pev->origin - pev->origin).Length();
				pReturn = pNextEnt;
			}
			else if (IRelationship(pNextEnt) == iBestRelationship)
			{
				// this entity is disliked just as much as the entity that
				// we currently think is the best visible enemy, so we only
				// get mad at it if it is closer.
				iDist = (pNextEnt->pev->origin - pev->origin).Length();

				if (iDist <= iNearest)
				{
					iNearest = iDist;
					iBestRelationship = IRelationship(pNextEnt);
					pReturn = pNextEnt;
				}
			}
		}

		pNextEnt = pNextEnt->m_pLink;
	}

	return pReturn;
}
