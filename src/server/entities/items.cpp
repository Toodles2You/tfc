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

	v.air_finished = gpGlobals->time;

	if ((v.spawnflags & SF_NORESPAWN) != 0)
	{
		SetThink(&CItem::Remove);
		v.nextthink = gpGlobals->time + 30.0F;
		return true;
	}

	if (engine::DropToFloor(&v) == 0)
	{
		engine::AlertMessage(at_error, "Item %s fell out of level at %f,%f,%f", STRING(v.classname), v.origin.x, v.origin.y, v.origin.z);
		return false;
	}

	return true;
}

bool CItem::CanHaveItem(CBaseEntity* other)
{
	if (v.air_finished > gpGlobals->time)
	{
		return false;
	}

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
		const auto large = (v.spawnflags & 1) != 0;

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
		else if (streq("item_armor1", classname))
		{
			v.armorvalue = 150.0F;
			v.armortype = 0.3F;
			v.model = MAKE_STRING("models/g_armor.mdl");
			v.noise = MAKE_STRING("items/armoron_1.wav");
			v.netname = MAKE_STRING("item_armor");
		}
		else if (streq("item_armor2", classname))
		{
			v.armorvalue = 200.0F;
			v.armortype = 0.6F;
			v.model = MAKE_STRING("models/y_armor.mdl");
			v.noise = MAKE_STRING("items/armoron_1.wav");
			v.netname = MAKE_STRING("item_armor");
		}
		else if (streq("item_armor3", classname))
		{
			v.armorvalue = 250.0F;
			v.armortype = 0.8F;
			v.model = MAKE_STRING("models/r_armor.mdl");
			v.noise = MAKE_STRING("items/armoron_1.wav");
			v.netname = MAKE_STRING("item_armor");
		}
		else if (streq("item_shells", classname))
		{
			tfv.ammo_shells = large ? 40 : 20;
			v.model = MAKE_STRING("models/w_shotbox.mdl");
			v.noise = MAKE_STRING("items/9mmclip1.wav");
		}
		else if (streq("item_spikes", classname))
		{
			tfv.ammo_nails = large ? 50 : 25;
			v.model = MAKE_STRING("models/w_crossbow_clip.mdl");
			v.noise = MAKE_STRING("items/9mmclip1.wav");
		}
		else if (streq("item_rockets", classname))
		{
			tfv.ammo_rockets = large ? 10 : 5;
			/* Toodles: This was random. And that's stupid! */
			tfv.no_grenades_1 = 1;
			tfv.no_grenades_2 = 1;
			v.model = MAKE_STRING("models/w_rpgammo.mdl");
			v.noise = MAKE_STRING("items/9mmclip1.wav");
		}
		else if (streq("item_cells", classname))
		{
			tfv.ammo_cells = large ? 12 : 5; /* It should be six but, it just isn't. Why?! */
			/* Toodles: This was w_battery.mdl but, I'm pedantic. */
			v.model = MAKE_STRING("models/w_gaussammo.mdl");
			v.noise = MAKE_STRING("items/9mmclip1.wav");
		}
		else
		{
			v.classname = MAKE_STRING("item_backpack");
			v.model = MAKE_STRING("models/backpack.mdl");
			v.noise = MAKE_STRING("items/ammopickup2.wav");
		}

		engine::PrecacheModel(STRING(v.model));
		engine::PrecacheSound(STRING(v.noise));

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

		if (v.armortype != 0.0F)
		{
			/* Team Fortress armor */
			return player->GiveArmor(v.armortype, v.armorvalue);
		}

		/* Half-Life battery */
		if (player->v.armorvalue >= player->m_flArmorMax)
		{
			return false;
		}

		player->v.armorvalue = std::clamp(player->v.armorvalue, 0.0F, player->m_flArmorMax);
		return true;
	}

	bool GiveAmmo(CBasePlayer* player)
	{
		bool result = false;

		if (tfv.ammo_shells != 0
		 && player->GiveAmmo(tfv.ammo_shells, AMMO_SHELLS))
		{
			result = true;
		}
		if (tfv.ammo_nails != 0
		 && player->GiveAmmo(tfv.ammo_nails, AMMO_NAILS))
		{
			result = true;
		}
		if (tfv.ammo_rockets != 0
		 && player->GiveAmmo(tfv.ammo_rockets, AMMO_ROCKETS))
		{
			result = true;
		}
		if (tfv.ammo_cells != 0
		 && player->GiveAmmo(tfv.ammo_cells, AMMO_CELLS))
		{
			result = true;
		}

		return result;
	}

	bool GiveGrenades(CBasePlayer* player)
	{
		bool result = false;

		if (tfv.no_grenades_1 != 0
		 && player->GiveAmmo(tfv.no_grenades_1, AMMO_GRENADES1))
		{
			result = true;
		}
		if (tfv.no_grenades_2 != 0
		 && player->GiveAmmo(tfv.no_grenades_2, AMMO_GRENADES2))
		{
			result = true;
		}

		return result;
	}

public:
	bool MyTouch(CBasePlayer* player) override
	{
		if (!GiveHealth(player)
		 && !GiveArmor(player)
		 && !GiveAmmo(player)
		 && !GiveGrenades(player))
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
LINK_ENTITY_TO_CLASS(item_armor1, CItemBackpack);
LINK_ENTITY_TO_CLASS(item_armor2, CItemBackpack);
LINK_ENTITY_TO_CLASS(item_armor3, CItemBackpack);
LINK_ENTITY_TO_CLASS(item_shells, CItemBackpack);
LINK_ENTITY_TO_CLASS(item_spikes, CItemBackpack);
LINK_ENTITY_TO_CLASS(item_rockets, CItemBackpack);
LINK_ENTITY_TO_CLASS(item_cells, CItemBackpack);


CItem* CItem::DropBackpack(CBaseEntity* owner, int shells, int nails, int rockets, int cells, bool drop)
{
	auto item = Entity::Create<CItemBackpack>();

	item->v.effects |= EF_NOINTERP;
	item->v.spawnflags |= SF_NORESPAWN;
	item->v.owner = owner->v.pContainingEntity;
	item->v.origin = owner->v.origin;

	item->v.health = 0;
	item->v.armorvalue = 0;
	item->tfv.ammo_shells = shells;
	item->tfv.ammo_nails = nails;
	item->tfv.ammo_rockets = rockets;
	item->tfv.ammo_cells = cells;

	item->Spawn();

	item->v.air_finished = gpGlobals->time + 0.75F;

	if (drop)
	{
		item->v.velocity = Vector(0.0F, 0.0F, 200.0F);
	}
	else
	{
		util::MakeVectors(owner->v.v_angle);
		item->v.velocity = gpGlobals->v_forward * 600.0F + gpGlobals->v_up * 200.0F;

		item->v.angles = util::VecToAngles(item->v.velocity);
		item->v.angles.x = item->v.angles.z = 0.0F;

		item->EmitSound("items/ammopickup1.wav", CHAN_ITEM);
	}
}

