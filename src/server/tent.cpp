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
unsigned short g_usGibbed;


void tent::Sparks(const Vector& position)
{
	MessageBegin(MSG_PVS, SVC_TEMPENTITY, position);
	WriteByte(TE_SPARKS);
	WriteCoord(position);
	MessageEnd();
}


void tent::Ricochet(const Vector& position, float scale)
{
	MessageBegin(MSG_PVS, SVC_TEMPENTITY, position);
	WriteByte(TE_ARMOR_RICOCHET);
	WriteCoord(position);
	WriteByte((int)(scale * 10));
	MessageEnd();
}


void tent::TeleportSplash(CBaseEntity* entity)
{
	engine::PlaybackEvent(
		FEV_GLOBAL | FEV_RELIABLE,
		&entity->v,
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
	float damage,
	bool smoke,
	bool sparks)
{
	engine::PlaybackEvent(
		FEV_GLOBAL | FEV_RELIABLE,
		&CWorld::World->v,
		g_usExplosion,
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
	Vector attackAngles = util::VecToAngles(g_vecAttackDir);

	engine::PlaybackEvent(
		FEV_GLOBAL | FEV_RELIABLE,
		&entity->v,
		g_usGibbed,
		0.0F,
		entity->v.origin,
		attackAngles,
		0.0F,
		std::max(entity->v.health, -99.0F),
		entity->v.sequence,
		gibMode,
		false,
		false
	);
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
	WriteCoord(pTrace->vecEndPos);
	WriteShort(pTrace->pHit->GetIndex());
	WriteByte(decalNumber);
	MessageEnd();
}

