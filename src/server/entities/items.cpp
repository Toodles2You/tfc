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

#include <algorithm>

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
	Precache();

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	SetOrigin(pev->origin);
	SetSize(Vector(-16, -16, 0), Vector(16, 16, 16));
	SetTouch(&CItem::ItemTouch);

	if (g_engfuncs.pfnDropToFloor(pev->pContainingEntity) == 0)
	{
		ALERT(at_error, "Item %s fell out of level at %f,%f,%f", STRING(pev->classname), pev->origin.x, pev->origin.y, pev->origin.z);
		return false;
	}

	return true;
}

bool CItem::CanHaveItem(CBaseEntity* other)
{
	if (!other->IsPlayer())
	{
		return false;
	}

	if (!g_pGameRules->CanHaveItem(dynamic_cast<CBasePlayer*>(other), this))
	{
		return false;
	}

	if (!AttemptToActivate(other))
	{
		return false;
	}

	return true;
}

void CItem::ItemTouch(CBaseEntity* pOther)
{
	if (!CanHaveItem(pOther))
	{
		return;
	}

	CBasePlayer* pPlayer = dynamic_cast<CBasePlayer*>(pOther);

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


class CItemBackpack : public CItem
{
public:
	void Precache() override
	{
		const auto classname = STRING(pev->classname);

		if (FStrEq("item_healthkit", classname))
		{
			pev->health = 15.0F;
			pev->model = MAKE_STRING("models/w_medkit.mdl");
			pev->noise = MAKE_STRING("items/smallmedkit1.wav");
			pev->netname = pev->classname;
		}
		else if (FStrEq("item_battery", classname))
		{
			pev->armorvalue = 15.0F;
			pev->model = MAKE_STRING("models/w_battery.mdl");
			pev->noise = MAKE_STRING("items/gunpickup2.wav");
			pev->netname = pev->classname;
		}
		else if (FStrEq("item_armor1", classname))
		{
			pev->armorvalue = 150.0F;
			pev->armortype = 0.3F;
			pev->model = MAKE_STRING("models/g_armor.mdl");
			pev->noise = MAKE_STRING("items/armoron_1.wav");
		}
		else if (FStrEq("item_armor2", classname))
		{
			pev->armorvalue = 200.0F;
			pev->armortype = 0.6F;
			pev->model = MAKE_STRING("models/y_armor.mdl");
			pev->noise = MAKE_STRING("items/armoron_1.wav");
		}
		else if (FStrEq("item_armor3", classname))
		{
			pev->armorvalue = 250.0F;
			pev->armortype = 0.8F;
			pev->model = MAKE_STRING("models/r_armor.mdl");
			pev->noise = MAKE_STRING("items/armoron_1.wav");
		}

		g_engfuncs.pfnPrecacheModel(STRING(pev->model));
		g_engfuncs.pfnPrecacheSound(STRING(pev->noise));

		SetModel(STRING(pev->model));
	}

protected:
	bool GiveHealth(CBasePlayer* player)
	{
		return pev->health != 0.0F && player->GiveHealth(pev->health, DMG_GENERIC);
	}

	bool GiveArmor(CBasePlayer* player)
	{
		if (pev->armorvalue == 0.0F)
		{
			return false;
		}

		if (pev->armortype != 0.0F)
		{
			/* Team Fortress armor */
			return player->GiveArmor(pev->armortype, pev->armorvalue);
		}

		/* Half-Life battery */
		if (player->pev->armorvalue >= player->m_flArmorMax)
		{
			return false;
		}

		player->pev->armorvalue = std::clamp(player->pev->armorvalue, 0.0F, player->m_flArmorMax);
		return true;
	}

public:
	bool MyTouch(CBasePlayer* player) override
	{
		if (!GiveHealth(player)
		 && !GiveArmor(player))
		{
			return false;
		}

		EmitSound(STRING(pev->noise), CHAN_ITEM);

		if (!FStringNull(pev->netname))
		{
			MessageBegin(MSG_ONE, gmsgItemPickup, player);
			WriteString(STRING(pev->netname));
			MessageEnd();
		}

		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_healthkit, CItemBackpack);
LINK_ENTITY_TO_CLASS(item_battery, CItemBackpack);
LINK_ENTITY_TO_CLASS(item_armor1, CItemBackpack);
LINK_ENTITY_TO_CLASS(item_armor2, CItemBackpack);
LINK_ENTITY_TO_CLASS(item_armor3, CItemBackpack);

