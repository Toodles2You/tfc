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

	v.movetype = MOVETYPE_TOSS;
	v.solid = SOLID_TRIGGER;
	SetOrigin(v.origin);
	SetSize(Vector(-16, -16, 0), Vector(16, 16, 16));
	SetTouch(&CItem::ItemTouch);

	if (g_engfuncs.pfnDropToFloor(&v) == 0)
	{
		ALERT(at_error, "Item %s fell out of level at %f,%f,%f", STRING(v.classname), v.origin.x, v.origin.y, v.origin.z);
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
		ClearTouch();

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
	ClearTouch();
	v.effects |= EF_NODRAW;

	SetOrigin(g_pGameRules->VecItemRespawnSpot(this)); // blip to whereever you should respawn.

	SetThink(&CItem::Materialize);
	v.nextthink = g_pGameRules->FlItemRespawnTime(this);
	return this;
}

void CItem::Materialize()
{
	if ((v.effects & EF_NODRAW) != 0)
	{
		EmitSound("items/itembk2.wav", CHAN_WEAPON);
		v.effects &= ~EF_NODRAW;
	}

	SetTouch(&CItem::ItemTouch);
}


class CItemBackpack : public CItem
{
public:
	CItemBackpack(Entity* containingEntity) : CItem(containingEntity) {}

	void Precache() override
	{
		const auto classname = STRING(v.classname);

		if (streq("item_healthkit", classname))
		{
			v.health = 15.0F;
			v.model = MAKE_STRING("models/w_medkit.mdl");
			v.noise = MAKE_STRING("items/smallmedkit1.wav");
			v.netname = v.classname;
		}
		else if (streq("item_battery", classname))
		{
			v.armorvalue = 15.0F;
			v.model = MAKE_STRING("models/w_battery.mdl");
			v.noise = MAKE_STRING("items/gunpickup2.wav");
			v.netname = v.classname;
		}

		g_engfuncs.pfnPrecacheModel(STRING(v.model));
		g_engfuncs.pfnPrecacheSound(STRING(v.noise));

		SetModel(v.model);
	}

protected:
	bool GiveHealth(CBasePlayer* player)
	{
		return v.health != 0.0F && player->GiveHealth(v.health, DMG_GENERIC);
	}

	bool GiveArmor(CBasePlayer* player)
	{
		if (v.armorvalue == 0.0F)
		{
			return false;
		}

		if (player->v.armorvalue >= 100)
		{
			return false;
		}

		player->v.armorvalue = std::clamp(player->v.armorvalue, 0.0F, 100.0F);
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

		EmitSound(STRING(v.noise), CHAN_ITEM);

		if (!FStringNull(v.netname))
		{
			MessageBegin(MSG_ONE, gmsgItemPickup, player);
			WriteString(STRING(v.netname));
			MessageEnd();
		}

		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_healthkit, CItemBackpack);
LINK_ENTITY_TO_CLASS(item_battery, CItemBackpack);

