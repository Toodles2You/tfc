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

#include <algorithm>

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "animation.h"
#include "weapons.h"
#include "func_break.h"
#include "player.h"
#include "UserMessages.h"
#include "gamerules.h"


bool CBaseEntity::ApplyMultiDamage(CBaseEntity* inflictor, CBaseEntity* attacker)
{
	if (m_multiDamage.amount <= 0.0F)
	{
		return false;
	}

	const bool result =
		TakeDamage(inflictor, attacker, m_multiDamage.amount, m_multiDamage.type);

	m_multiDamage.amount = 0.0F;
	m_multiDamage.type = DMG_GENERIC;

	return result;
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
	const float damageMax,
	const float damageMin,
	const float radius,
	const int damageType)
{
	const float damageFalloff = damageMin - damageMax;
	CBaseEntity* entity = nullptr;
	TraceResult tr;
	float adjusted;

	while ((entity = util::FindEntityInSphere(entity, origin, radius)) != nullptr)
	{
		if (entity->v.takedamage == DAMAGE_NO)
		{
			continue;
		}

		const auto isBrush = entity->IsBSPModel();

		const auto eyes = isBrush ? entity->Center() : entity->EyePosition();

		util::TraceLine(origin, eyes, &tr, inflictor, util::kTraceBox);

		if (tr.flFraction != 1.0F && tr.pHit != &entity->v)
		{
			continue;
		}

		const auto center = isBrush ? tr.vecEndPos : entity->BodyTarget();

		adjusted = (origin - center).Length() / radius;
		adjusted = std::max(damageMax + damageFalloff * adjusted, 0.0F);

		entity->TakeDamage(inflictor, attacker, adjusted, damageType);
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

	util::MakeVectors(v.angles);

	vec2LOS = (pEntity->v.origin - v.origin).Make2D();
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

	util::MakeVectors(v.angles);

	vec2LOS = (vecOrigin - v.origin).Make2D();
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

	if (FBitSet(pEntity->v.flags, FL_NOTARGET))
		return false;

	// don't look through water
	if ((v.waterlevel >= kWaterLevelEyes) != (pEntity->v.waterlevel >= kWaterLevelEyes))
		return false;

	vecLookerOrigin = v.origin + v.view_ofs; //look through the caller's 'eyes'
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
	if (v.takedamage != DAMAGE_NO)
	{
		AddMultiDamage(flDamage, bitsDamageType);
	}
}

static inline float Bezier(float x)
{
	x = std::clamp(x, 0.0F, 1.0F);
    return x * x * (3.0F - 2.0F * x);
}

void CBasePlayer::FireBullets(
	const float damageMax,
	const float damageMin,
	const Vector2D& spread,
	const unsigned int count,
	const float distance)
{
	const float damageFalloff = damageMin - damageMax;
	const auto gun = v.origin + v.view_ofs;
	const auto aim = v.v_angle + v.punchangle;
	float adjusted;

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
			aim.x + spread.y * spreadScale.x,
			aim.y + spread.x * spreadScale.y,
			aim.z,
		};

		Vector dir;
		AngleVectors(angles, &dir, nullptr, nullptr);

		TraceResult tr;
		util::TraceLine(gun, gun + dir * distance, &tr, this, util::kTraceBox | util::kTraceBoxModel);
		
		if (tr.flFraction == 1.0F)
		{
			continue;
		}

		auto hit = tr.pHit->Get<CBaseEntity>();
		
		if (hit == nullptr || hit->v.takedamage == DAMAGE_NO)
		{
			continue;
		}
		
		if (damageFalloff == 0.0F)
		{
			adjusted = damageMax;
		}
		else
		{
			const auto isBrush = hit->IsBSPModel();

			const auto center = isBrush ? tr.vecEndPos : hit->BodyTarget();

			adjusted = std::max((v.origin - center).Length() - 512.0F, 0.0F) / 512.0F;
			adjusted = damageMax + damageFalloff * Bezier(adjusted);
		}

		hit->TraceAttack(
			this,
			adjusted,
			dir,
			tr.iHitgroup,
			DMG_BULLET | DMG_NEVERGIB);

		traceEntities[traceCount] = hit;
		traceCount++;

		if (!hit->IsClient())
		{
			continue;
		}

		auto player = dynamic_cast<CBasePlayer*>(hit);

		if (!util::DoDamageResponse(player, this))
		{
			continue;
		}

		if (traceHits < 8 && player->m_LastHitGroup == HITGROUP_HEAD)
		{
			traceFlags |= 1 << traceHits;
		}

		traceEndPos[traceHits] = tr.vecEndPos;
		traceHits++;
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
			WriteCoord(traceEndPos[i]);
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

	m_pLink = nullptr;

	CBaseEntity* pSightEnt = nullptr; // the current visible entity that we're dealing with

	// See no evil if prisoner is set
	CBaseEntity* pList[100];

	Vector delta = Vector(iDistance, iDistance, iDistance);

	// Find only monsters/clients in box, NOT limited to PVS
	int count = util::EntitiesInBox(pList, 100, v.origin - delta, v.origin + delta, FL_CLIENT | FL_MONSTER);
	for (int i = 0; i < count; i++)
	{
		pSightEnt = pList[i];
		// !!!temporarily only considering other monsters and clients, don't see prisoners
		if (pSightEnt != this &&
			pSightEnt->v.health > 0)
		{
			// the looker will want to consider this entity
			// don't check anything else about an entity that can't be seen, or an entity that you don't care about.
			if (IRelationship(pSightEnt) != R_NO && FInViewCone(pSightEnt) && !FBitSet(pSightEnt->v.flags, FL_NOTARGET) && FVisible(pSightEnt))
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
	pReturn = nullptr;
	// iBestRelationship = R_NO;

	while (pNextEnt != nullptr)
	{
		if (pNextEnt->IsAlive())
		{
			if (IRelationship(pNextEnt) > iBestRelationship)
			{
				// this entity is disliked MORE than the entity that we
				// currently think is the best visible enemy. No need to do
				// a distance check, just get mad at this one for now.
				iBestRelationship = IRelationship(pNextEnt);
				iNearest = (pNextEnt->v.origin - v.origin).Length();
				pReturn = pNextEnt;
			}
			else if (IRelationship(pNextEnt) == iBestRelationship)
			{
				// this entity is disliked just as much as the entity that
				// we currently think is the best visible enemy, so we only
				// get mad at it if it is closer.
				iDist = (pNextEnt->v.origin - v.origin).Length();

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
