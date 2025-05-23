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
unsigned short g_usTrail;


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
	ExplosionType type,
	float damage,
	bool smoke,
	bool sparks,
	CBaseEntity* entity)
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
		case ExplosionType::EMP:
			eventIndex = g_usEMP;
			break;
		case ExplosionType::Flash:
			eventIndex = g_usFlash;
			break;
	}

	auto flags = FEV_GLOBAL | FEV_RELIABLE;

	if (entity != nullptr)
	{
		flags |= FEV_NOTHOST;
	}
	else
	{
		entity = CWorld::World;
	}

	engine::PlaybackEvent(
		flags,
		&entity->v,
		eventIndex,
		0.0F,
		origin,
		velocity,
		0.0F,
		0.0F,
		std::max(static_cast<int>(damage), 0),
		static_cast<int>(type),
		smoke,
		sparks
	);
}


void tent::SpawnCorpse(CBaseEntity* entity, const int gibMode, const int sequence)
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
		sequence != -1 ? sequence : entity->v.sequence,
		gibMode,
		false,
		false
	);
}


void tent::RocketTrail(CBaseEntity* entity, const bool flare, const int team)
{
	engine::PlaybackEvent(
		FEV_GLOBAL,
		&entity->v,
		g_usTrail,
		0.0F,
		entity->v.origin,
		g_vecZero,
		0.0F,
		0.0F,
		team,
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
	WriteCoord(pTrace->vecEndPos);
	WriteShort(pTrace->pHit->GetIndex());
	WriteByte(decalNumber);
	MessageEnd();
}


void tent::FireField(const Vector& origin)
{
	MessageBegin(MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte(TE_FIREFIELD);
	WriteCoord(origin + Vector(0.0F, 0.0F, 24.0F));
	WriteShort(100);
	WriteShort(g_sModelIndexFire);
	WriteByte(6);
	WriteByte(TEFIRE_FLAG_SOMEFLOAT | TEFIRE_FLAG_PLANAR);
	WriteByte(8);
	MessageEnd();
}

