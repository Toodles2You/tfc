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

extern Vector VecBModelOrigin(entvars_t* pevBModel);

//
// fade out - slowly fades a entity out, then removes it.
//
// DON'T USE ME FOR GIBS AND STUFF IN MULTIPLAYER!
// SET A FUTURE THINK AND A RENDERMODE!!
void CBaseEntity::SUB_StartFadeOut()
{
	if (pev->rendermode == kRenderNormal)
	{
		pev->renderamt = 255;
		pev->rendermode = kRenderTransTexture;
	}

	pev->solid = SOLID_NOT;
	pev->avelocity = g_vecZero;

	pev->nextthink = gpGlobals->time + 0.1;
	SetThink(&CBaseEntity::SUB_FadeOut);
}

void CBaseEntity::SUB_FadeOut()
{
	if (pev->renderamt > 7)
	{
		pev->renderamt -= 7;
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		pev->renderamt = 0;
		pev->nextthink = gpGlobals->time + 0.2;
		SetThink(&CBaseEntity::SUB_Remove);
	}
}

//
// RadiusDamage - this entity is exploding, or otherwise needs to inflict damage upon entities within a certain range.
//
// only damage ents that can clearly be seen by the explosion!


void RadiusDamage(Vector vecSrc, entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType)
{
	CBaseEntity* pEntity = NULL;
	TraceResult tr;
	float flAdjustedDamage, falloff;
	Vector vecSpot;

	if (0 != flRadius)
		falloff = flDamage / flRadius;
	else
		falloff = 1.0;

	const bool bInWater = (UTIL_PointContents(vecSrc) == CONTENTS_WATER);

	vecSrc.z += 1; // in case grenade is lying on the ground

	if (!pevAttacker)
		pevAttacker = pevInflictor;

	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, vecSrc, flRadius)) != NULL)
	{
		if (pEntity->pev->takedamage != DAMAGE_NO)
		{
			// UNDONE: this should check a damage mask, not an ignore
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

			UTIL_TraceLine(vecSrc, vecSpot, dont_ignore_monsters, ENT(pevInflictor), &tr);

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
					pEntity->TraceAttack(pevInflictor, flAdjustedDamage, (tr.vecEndPos - vecSrc).Normalize(), &tr, bitsDamageType);
					ApplyMultiDamage(pevInflictor, pevAttacker);
				}
				else
				{
					pEntity->TakeDamage(pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType);
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

	UTIL_MakeVectors(pev->angles);

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

	UTIL_MakeVectors(pev->angles);

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

	UTIL_TraceLine(vecLookerOrigin, vecTargetOrigin, ignore_monsters, ignore_glass, ENT(pev) /*pentIgnore*/, &tr);

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

	UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, ENT(pev) /*pentIgnore*/, &tr);

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
void CBaseEntity::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	Vector vecOrigin = ptr->vecEndPos - vecDir * 4;

	if (pev->takedamage != DAMAGE_NO)
	{
		AddMultiDamage(pevAttacker, this, flDamage, bitsDamageType);

		int blood = BloodColor();

		if (blood != DONT_BLEED)
		{
			SpawnBlood(vecOrigin, blood, flDamage); // a little surface blood.
			TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
		}
	}
}

/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.

This version is used by Monsters.
================
*/
void CBaseEntity::FireBullets(unsigned int cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq, int iDamage, entvars_t* pevAttacker)
{
	static int tracerCount;
	TraceResult tr;
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;

	if (pevAttacker == NULL)
		pevAttacker = pev; // the default attacker is ourselves

	ClearMultiDamage();
	gMultiDamage.type = DMG_BULLET | DMG_NEVERGIB;

	for (unsigned int iShot = 1; iShot <= cShots; iShot++)
	{
		// get circular gaussian spread
		float x, y, z;
		do
		{
			x = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
			y = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
			z = x * x + y * y;
		} while (z > 1);

		Vector vecDir = vecDirShooting +
						x * vecSpread.x * vecRight +
						y * vecSpread.y * vecUp;
		Vector vecEnd;

		vecEnd = vecSrc + vecDir * flDistance;
		UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(pev) /*pentIgnore*/, &tr);

		if (iTracerFreq != 0 && (tracerCount++ % iTracerFreq) == 0)
		{
			Vector vecTracerSrc;

			if (IsPlayer())
			{ // adjust tracer position for player
				vecTracerSrc = vecSrc + Vector(0, 0, -4) + gpGlobals->v_right * 2 + gpGlobals->v_forward * 16;
			}
			else
			{
				vecTracerSrc = vecSrc;
			}

			switch (iBulletType)
			{
			case BULLET_MONSTER_MP5:
			case BULLET_MONSTER_9MM:
			case BULLET_MONSTER_12MM:
			default:
				MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, vecTracerSrc);
				WRITE_BYTE(TE_TRACER);
				WRITE_COORD(vecTracerSrc.x);
				WRITE_COORD(vecTracerSrc.y);
				WRITE_COORD(vecTracerSrc.z);
				WRITE_COORD(tr.vecEndPos.x);
				WRITE_COORD(tr.vecEndPos.y);
				WRITE_COORD(tr.vecEndPos.z);
				MESSAGE_END();
				break;
			}
		}
		// do damage, paint decals
		if (tr.flFraction != 1.0)
		{
			CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

			if (0 != iDamage)
			{
				pEntity->TraceAttack(pevAttacker, iDamage, vecDir, &tr, DMG_BULLET | ((iDamage > 16) ? DMG_ALWAYSGIB : DMG_NEVERGIB));

				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot(&tr, iBulletType);
			}
			else
				switch (iBulletType)
				{
				case BULLET_PLAYER_BUCKSHOT:
					// make distance based!
					pEntity->TraceAttack(pevAttacker, gSkillData.plrDmgBuckshot, vecDir, &tr, DMG_BULLET);

					TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
					DecalGunshot(&tr, iBulletType);
					break;

				default:
				case BULLET_MONSTER_9MM:
					pEntity->TraceAttack(pevAttacker, gSkillData.monDmg9MM, vecDir, &tr, DMG_BULLET);

					TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
					DecalGunshot(&tr, iBulletType);

					break;

				case BULLET_MONSTER_MP5:
					pEntity->TraceAttack(pevAttacker, gSkillData.monDmgMP5, vecDir, &tr, DMG_BULLET);

					TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
					DecalGunshot(&tr, iBulletType);

					break;

				case BULLET_MONSTER_12MM:
					pEntity->TraceAttack(pevAttacker, gSkillData.monDmg12MM, vecDir, &tr, DMG_BULLET);
					TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
					DecalGunshot(&tr, iBulletType);
					break;

				case BULLET_NONE: // FIX
					pEntity->TraceAttack(pevAttacker, 50, vecDir, &tr, DMG_CLUB);
					TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
					// only decal glass
					if (!FNullEnt(tr.pHit) && VARS(tr.pHit)->rendermode != 0)
					{
						UTIL_DecalTrace(&tr, DECAL_GLASSBREAK1 + RANDOM_LONG(0, 2));
					}

					break;
				}
		}
		// make bullet trails
		UTIL_BubbleTrail(vecSrc, tr.vecEndPos, (flDistance * tr.flFraction) / 64.0);
	}
	ApplyMultiDamage(pev, pevAttacker);
}


/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.

This version is used by Players, uses the random seed generator to sync client and server side shots.
================
*/
void CBaseEntity::FireBulletsPlayer(unsigned int cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq, int iDamage, entvars_t* pevAttacker, int shared_rand)
{
	static int tracerCount;
	TraceResult tr;
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;
	float x = 0, y = 0, z;

	if (pevAttacker == NULL)
		pevAttacker = pev; // the default attacker is ourselves

	ClearMultiDamage();
	gMultiDamage.type = DMG_BULLET | DMG_AIMED | DMG_NEVERGIB;

	for (unsigned int iShot = 1; iShot <= cShots; iShot++)
	{
		//Use player's random seed.
		// get circular gaussian spread
		x = UTIL_SharedRandomFloat(shared_rand + iShot, -0.5, 0.5) + UTIL_SharedRandomFloat(shared_rand + (1 + iShot), -0.5, 0.5);
		y = UTIL_SharedRandomFloat(shared_rand + (2 + iShot), -0.5, 0.5) + UTIL_SharedRandomFloat(shared_rand + (3 + iShot), -0.5, 0.5);
		z = x * x + y * y;

		Vector vecDir = vecDirShooting +
						x * vecSpread.x * vecRight +
						y * vecSpread.y * vecUp;
		Vector vecEnd;

		vecEnd = vecSrc + vecDir * flDistance;
		UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(pev) /*pentIgnore*/, &tr);

		// do damage, paint decals
		if (tr.flFraction != 1.0)
		{
			CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

			if (0 != iDamage)
			{
				pEntity->TraceAttack(pevAttacker, iDamage, vecDir, &tr, DMG_BULLET | DMG_AIMED | ((iDamage > 16) ? DMG_ALWAYSGIB : DMG_NEVERGIB));

				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot(&tr, iBulletType);
			}
			else
				switch (iBulletType)
				{
				default:
				case BULLET_PLAYER_9MM:
					pEntity->TraceAttack(pevAttacker, gSkillData.plrDmg9MM, vecDir, &tr, DMG_BULLET | DMG_AIMED);
					break;

				case BULLET_PLAYER_MP5:
					pEntity->TraceAttack(pevAttacker, gSkillData.plrDmgMP5, vecDir, &tr, DMG_BULLET | DMG_AIMED);
					break;

				case BULLET_PLAYER_BUCKSHOT:
					// make distance based!
					pEntity->TraceAttack(pevAttacker, gSkillData.plrDmgBuckshot, vecDir, &tr, DMG_BULLET | DMG_AIMED);
					break;

				case BULLET_PLAYER_357:
					pEntity->TraceAttack(pevAttacker, gSkillData.plrDmg357, vecDir, &tr, DMG_BULLET | DMG_AIMED);
					break;

				case BULLET_NONE: // FIX
					pEntity->TraceAttack(pevAttacker, 50, vecDir, &tr, DMG_CLUB | DMG_AIMED);
					TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
					// only decal glass
					if (!FNullEnt(tr.pHit) && VARS(tr.pHit)->rendermode != 0)
					{
						UTIL_DecalTrace(&tr, DECAL_GLASSBREAK1 + RANDOM_LONG(0, 2));
					}

					break;
				}
		}
		// make bullet trails
		UTIL_BubbleTrail(vecSrc, tr.vecEndPos, (flDistance * tr.flFraction) / 64.0);
	}
	ApplyMultiDamage(pev, pevAttacker);
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

		UTIL_TraceLine(ptr->vecEndPos, ptr->vecEndPos + vecTraceDir * -172, ignore_monsters, ENT(pev), &Bloodtr);

		if (Bloodtr.flFraction != 1.0)
		{
			UTIL_BloodDecalTrace(&Bloodtr, BloodColor());
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
	int count = UTIL_EntitiesInBox(pList, 100, pev->origin - delta, pev->origin + delta, FL_CLIENT | FL_MONSTER);
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
//
// !!!UNDONE - currently, this only returns the closest enemy.
// we'll want to consider distance, relationship, attack types, back turned, etc.
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
