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

===== items.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "items.h"
#include "gamerules.h"
#include "UserMessages.h"


bool CItem::Spawn()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	SetOrigin(pev->origin);
	SetSize(Vector(-16, -16, 0), Vector(16, 16, 16));
	SetTouch(&CItem::ItemTouch);

	if (DROP_TO_FLOOR(ENT(pev)) == 0)
	{
		ALERT(at_error, "Item %s fell out of level at %f,%f,%f", STRING(pev->classname), pev->origin.x, pev->origin.y, pev->origin.z);
		return false;
	}

	return true;
}

void CItem::ItemTouch(CBaseEntity* pOther)
{
	// if it's not a player, ignore
	if (!pOther->IsPlayer())
	{
		return;
	}

	CBasePlayer* pPlayer = (CBasePlayer*)pOther;

	// ok, a player is touching this item, but can he have it?
	if (!g_pGameRules->CanHaveItem(pPlayer, this))
	{
		// no? Ignore the touch.
		return;
	}

	if (MyTouch(pPlayer))
	{
		UseTargets(pOther, USE_TOGGLE, 0);
		SetTouch(NULL);

		// player grabbed the item.
		g_pGameRules->PlayerGotItem(pPlayer, this);
		if (g_pGameRules->ItemShouldRespawn(this) == GR_ITEM_RESPAWN_YES)
		{
			Respawn();
		}
		else
		{
			Remove();
		}
	}
	else if (gEvilImpulse101)
	{
		Remove();
	}
}

CBaseEntity* CItem::Respawn()
{
	SetTouch(NULL);
	pev->effects |= EF_NODRAW;

	SetOrigin(g_pGameRules->VecItemRespawnSpot(this)); // blip to whereever you should respawn.

	SetThink(&CItem::Materialize);
	pev->nextthink = g_pGameRules->FlItemRespawnTime(this);
	return this;
}

void CItem::Materialize()
{
	if ((pev->effects & EF_NODRAW) != 0)
	{
		EmitSound("items/itembk2.wav", CHAN_WEAPON);
		pev->effects &= ~EF_NODRAW;
	}

	SetTouch(&CItem::ItemTouch);
}

class CItemBattery : public CItem
{
	bool Spawn() override
	{
		Precache();
		SetModel("models/w_battery.mdl");
		return CItem::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_battery.mdl");
		PRECACHE_SOUND("items/gunpickup2.wav");
	}
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		if (pPlayer->pev->deadflag != DEAD_NO)
		{
			return false;
		}

		if (pPlayer->pev->armorvalue < 100.0F)
		{
			int pct;
			char szcharge[64];

			pPlayer->pev->armorvalue += 15;
			pPlayer->pev->armorvalue = std::min(pPlayer->pev->armorvalue, 100.0F);

			pPlayer->EmitSound("items/gunpickup2.wav", CHAN_ITEM);

			MessageBegin(MSG_ONE, gmsgItemPickup, pPlayer);
			WriteString(STRING(pev->classname));
			MessageEnd();
			return true;
		}
		return false;
	}
};

LINK_ENTITY_TO_CLASS(item_battery, CItemBattery);


class CHealthKit : public CItem
{
	bool Spawn() override;
	void Precache() override;
	bool MyTouch(CBasePlayer* pPlayer) override;
};

LINK_ENTITY_TO_CLASS(item_healthkit, CHealthKit);

bool CHealthKit::Spawn()
{
	Precache();
	SetModel("models/w_medkit.mdl");

	return CItem::Spawn();
}

void CHealthKit::Precache()
{
	PRECACHE_MODEL("models/w_medkit.mdl");
	PRECACHE_SOUND("items/smallmedkit1.wav");
}

bool CHealthKit::MyTouch(CBasePlayer* pPlayer)
{
	if (pPlayer->pev->deadflag != DEAD_NO)
	{
		return false;
	}

	if (pPlayer->GiveHealth(15, DMG_GENERIC))
	{
		MessageBegin(MSG_ONE, gmsgItemPickup, pPlayer);
		WriteString(STRING(pev->classname));
		MessageEnd();

		pPlayer->EmitSound("items/smallmedkit1.wav", CHAN_ITEM);
		return true;
	}

	return false;
}
