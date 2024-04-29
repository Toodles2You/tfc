//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Whatever
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "shake.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "UserMessages.h"


unsigned short g_usTeleport;
unsigned short g_usExplosion;
unsigned short g_usConcBlast;
unsigned short g_usGibbed;
unsigned short g_usTrail;


void tent::Sparks(const Vector& position)
{
	MessageBegin(MSG_PVS, SVC_TEMPENTITY, position);
	WriteByte(TE_SPARKS);
	WriteCoord(position.x);
	WriteCoord(position.y);
	WriteCoord(position.z);
	MessageEnd();
}


void tent::Ricochet(const Vector& position, float scale)
{
	MessageBegin(MSG_PVS, SVC_TEMPENTITY, position);
	WriteByte(TE_ARMOR_RICOCHET);
	WriteCoord(position.x);
	WriteCoord(position.y);
	WriteCoord(position.z);
	WriteByte((int)(scale * 10));
	MessageEnd();
}


void tent::TeleportSplash(CBaseEntity* entity)
{
	g_engfuncs.pfnPlaybackEvent(
		FEV_GLOBAL | FEV_RELIABLE,
		entity->edict(),
		g_usTeleport,
		0.0F,
		entity->Center(),
		g_vecZero,
		0.0F,
		0.0F,
		0,
		0,
		false,
		false);
}


void tent::Explosion(
	const Vector& origin,
	const Vector& velocity,
	ExplosionType type,
	float damage,
	bool smoke,
	bool sparks)
{
	unsigned short eventIndex;

	switch (type)
	{
		default:
		case ExplosionType::Normal:
			eventIndex = g_usExplosion;
			break;
		case ExplosionType::Concussion:
			eventIndex = g_usConcBlast;
			break;
	}

	g_engfuncs.pfnPlaybackEvent(
		FEV_GLOBAL | FEV_RELIABLE,
		CWorld::World->edict(),
		eventIndex,
		0.0F,
		origin,
		velocity,
		0.0F,
		0.0F,
		damage,
		0,
		smoke,
		sparks
	);
}


void tent::SpawnCorpse(CBaseEntity* entity, const int gibMode)
{
	g_engfuncs.pfnPlaybackEvent(
		FEV_GLOBAL | FEV_RELIABLE,
		entity->edict(),
		g_usGibbed,
		0.0F,
		entity->pev->origin,
		util::VecToAngles(g_vecAttackDir),
		0.0F,
		entity->pev->health,
		entity->pev->sequence,
		gibMode,
		false,
		false
	);
}


void tent::RocketTrail(CBaseEntity* entity, const bool flare)
{
	g_engfuncs.pfnPlaybackEvent(
		FEV_GLOBAL,
		entity->edict(),
		g_usTrail,
		0.0F,
		entity->pev->origin,
		g_vecZero,
		0.0F,
		0.0F,
		0,
		0,
		flare,
		false);
}


/*
==============
PlayerDecalTrace

Custom player decal for the spray can
==============
*/
void tent::PlayerDecalTrace(TraceResult* pTrace, int playernum, int decalNumber)
{
	if (pTrace->flFraction == 1.0)
		return;

	MessageBegin(MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte(TE_PLAYERDECAL);
	WriteByte(playernum);
	WriteCoord(pTrace->vecEndPos.x);
	WriteCoord(pTrace->vecEndPos.y);
	WriteCoord(pTrace->vecEndPos.z);
	WriteShort((short)ENTINDEX(pTrace->pHit));
	WriteByte(decalNumber);
	MessageEnd();
}

