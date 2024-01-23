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
#include "animation.h"
#include "weapons.h"
#include "func_break.h"
#include "player.h"
#include "UserMessages.h"
#include "gamerules.h"


void CBaseEntity::ApplyMultiDamage(CBaseEntity* inflictor, CBaseEntity* attacker)
{
	if (m_multiDamage.amount <= 0.0F)
	{
		return;
	}

	TakeDamage(inflictor, attacker, m_multiDamage.amount, m_multiDamage.type);

	m_multiDamage.amount = 0.0F;
	m_multiDamage.type = DMG_GENERIC;
}


void CBaseEntity::AddMultiDamage(float damage, int damageType)
{
	m_multiDamage.amount += damage;
	m_multiDamage.type |= damageType;
}


void RadiusDamage(
	const Vector& origin,
	CBaseEntity* inflictor,
	CBaseEntity* attacker,
	const float damage,
	const float radius,
	const int damageType)
{
	CBaseEntity* entity = nullptr;
	TraceResult tr;
	float ajdusted;

	while ((entity = util::FindEntityInSphere(entity, origin, radius)) != nullptr)
	{
		if (entity->pev->takedamage == DAMAGE_NO)
		{
			continue;
		}

		util::TraceLine(origin, entity->EyePosition(), &tr, inflictor, util::kTraceBox);

		if (tr.flFraction != 1.0F && tr.pHit != entity->edict())
		{
			continue;
		}

		ajdusted = 0.5F * (origin - entity->BodyTarget()).Length();
		ajdusted = std::max(damage - ajdusted, 0.0F);

		entity->TakeDamage(inflictor, attacker, ajdusted, damageType);
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
void CBaseEntity::TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, int hitgroup, int bitsDamageType)
{
	if (pev->takedamage != DAMAGE_NO)
	{
		AddMultiDamage(flDamage, bitsDamageType);
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
	auto traceEndPos = static_cast<Vector*>(alloca(count * sizeof(Vector)));
	auto traceCount = 0;
	auto traceEntities = static_cast<CBaseEntity**>(alloca(count * sizeof(CBaseEntity*)));

	for (auto i = 0; i < count; i++)
	{
		const Vector2D spreadScale
		{
			util::SharedRandomFloat(m_randomSeed + i * 4, -0.5, 0.5)
				+ util::SharedRandomFloat(m_randomSeed + 1 + i * 4, -0.5, 0.5),
			util::SharedRandomFloat(m_randomSeed + 2 + i * 4, -0.5, 0.5)
				+ util::SharedRandomFloat(m_randomSeed + 3 + i * 4, -0.5, 0.5)
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
		util::TraceLine(gun, gun + dir * distance, &tr, this, util::kTraceBox | util::kTraceBoxModel);
		
		if (tr.flFraction != 1.0F)
		{
			const auto hit =
				reinterpret_cast<CBasePlayer *>(CBaseEntity::Instance(tr.pHit));
			
			hit->TraceAttack(
				this,
				damage,
				dir,
				tr.iHitgroup,
				DMG_BULLET | DMG_NEVERGIB);

			traceEntities[traceCount] = hit;
			traceCount++;
			
			if (hit->IsClient() && g_pGameRules->FPlayerCanTakeDamage(hit, this))
			{
				traceEndPos[traceHits] = tr.vecEndPos;
				traceHits++;
				if (i < 8 && hit->m_LastHitGroup == HITGROUP_HEAD)
				{
					traceFlags |= 1 << i;
				}
			}
		}
	}

	for (auto i = 0; i < traceCount; i++)
	{
		traceEntities[i]->ApplyMultiDamage(this, this);
	}

	Vector dir;
	AngleVectors(aim, &dir, nullptr, nullptr);
	
	if (traceHits != 0)
	{
		MessageBegin(MSG_ONE_UNRELIABLE, gmsgBlood, this);
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
