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
#include "decals.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "UserMessages.h"


unsigned short g_usTeleport;
unsigned short g_usExplosion;


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
	float damage,
	bool smoke,
	bool sparks)
{
	g_engfuncs.pfnPlaybackEvent(
		FEV_GLOBAL | FEV_RELIABLE,
		CWorld::World->edict(),
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


void tent::BloodStream(const Vector& origin, const Vector& direction, int color, int amount)
{
	if (color == DONT_BLEED || amount == 0)
	{
		return;
	}

	MessageBegin(MSG_PVS, SVC_TEMPENTITY, origin);
	WriteByte(TE_BLOODSTREAM);
	WriteCoord(origin.x);
	WriteCoord(origin.y);
	WriteCoord(origin.z);
	WriteCoord(direction.x);
	WriteCoord(direction.y);
	WriteCoord(direction.z);
	WriteByte(color);
	WriteByte(std::min(amount, 255));
	MessageEnd();
}


void tent::BloodDrips(const Vector& origin, const Vector& direction, int color, int amount)
{
	if (color == DONT_BLEED || amount == 0)
	{
		return;
	}

	if (util::IsDeathmatch())
	{
		// scale up blood effect in multiplayer for better visibility
		amount *= 2;
	}

	amount = std::min(amount, 255);

	MessageBegin(MSG_PVS, SVC_TEMPENTITY, origin);
	WriteByte(TE_BLOODSPRITE);
	WriteCoord(origin.x); // pos
	WriteCoord(origin.y);
	WriteCoord(origin.z);
	WriteShort(g_sModelIndexBloodSpray);		  // initial sprite model
	WriteShort(g_sModelIndexBloodDrop);		  // droplet sprite models
	WriteByte(color);							  // color index into host_basepal
	WriteByte(std::min(std::max(3, amount / 10), 16)); // size
	MessageEnd();
}


void tent::BloodDecalTrace(TraceResult* pTrace, int bloodColor)
{
	if (bloodColor == DONT_BLEED)
	{
		return;
	}
	if (bloodColor == BLOOD_COLOR_RED)
	{
		tent::DecalTrace(pTrace, DECAL_BLOOD1 + RANDOM_LONG(0, 5));
	}
	else
	{
		tent::DecalTrace(pTrace, DECAL_YBLOOD1 + RANDOM_LONG(0, 5));
	}
}


void tent::DecalTrace(TraceResult* pTrace, int decalNumber)
{
	short entityIndex;
	int index;
	int message;

	if (decalNumber < 0)
		return;

	index = gDecals[decalNumber].index;

	if (index < 0)
		return;

	if (pTrace->flFraction == 1.0)
		return;

	// Only decal BSP models
	if (pTrace->pHit)
	{
		CBaseEntity* pEntity = CBaseEntity::Instance(pTrace->pHit);
		if (pEntity && !pEntity->IsBSPModel())
			return;
		entityIndex = ENTINDEX(pTrace->pHit);
	}
	else
		entityIndex = 0;

	message = TE_DECAL;
	if (entityIndex != 0)
	{
		if (index > 255)
		{
			message = TE_DECALHIGH;
			index -= 256;
		}
	}
	else
	{
		message = TE_WORLDDECAL;
		if (index > 255)
		{
			message = TE_WORLDDECALHIGH;
			index -= 256;
		}
	}

	MessageBegin(MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte(message);
	WriteCoord(pTrace->vecEndPos.x);
	WriteCoord(pTrace->vecEndPos.y);
	WriteCoord(pTrace->vecEndPos.z);
	WriteByte(index);
	if (0 != entityIndex)
		WriteShort(entityIndex);
	MessageEnd();
}


/*
==============
tent::PlayerDecalTrace

A player is trying to apply his custom decal for the spray can.
Tell connected clients to display it, or use the default spray can decal
if the custom can't be loaded.
==============
*/
void tent::PlayerDecalTrace(TraceResult* pTrace, int playernum, int decalNumber, bool bIsCustom)
{
	int index;

	if (!bIsCustom)
	{
		if (decalNumber < 0)
			return;

		index = gDecals[decalNumber].index;
		if (index < 0)
			return;
	}
	else
		index = decalNumber;

	if (pTrace->flFraction == 1.0)
		return;

	MessageBegin(MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte(TE_PLAYERDECAL);
	WriteByte(playernum);
	WriteCoord(pTrace->vecEndPos.x);
	WriteCoord(pTrace->vecEndPos.y);
	WriteCoord(pTrace->vecEndPos.z);
	WriteShort((short)ENTINDEX(pTrace->pHit));
	WriteByte(index);
	MessageEnd();
}

