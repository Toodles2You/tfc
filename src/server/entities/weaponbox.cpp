//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Whatever
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "UserMessages.h"
#include "weaponbox.h"

//=========================================================
// MaxAmmoCarry - pass in a name and this function will tell
// you the maximum amount of that type of ammunition that a
// player can carry.
//=========================================================
static int MaxAmmoCarry(int iType)
{
	for (int i = 0; i < WEAPON_LAST; i++)
	{
		if (CBasePlayerWeapon::WeaponInfoArray[i].iAmmo1 == iType)
			return CBasePlayerWeapon::WeaponInfoArray[i].iMaxAmmo1;
		if (CBasePlayerWeapon::WeaponInfoArray[i].iAmmo2 == iType)
			return CBasePlayerWeapon::WeaponInfoArray[i].iMaxAmmo2;
	}

	ALERT(at_console, "MaxAmmoCarry() doesn't recognize '%i'!\n", iType);
	return -1;
}

bool CBasePlayerAmmo::Spawn()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	SetSize(Vector(-16, -16, 0), Vector(16, 16, 16));
	SetOrigin(pev->origin);

	SetTouch(&CBasePlayerAmmo::DefaultTouch);

	return true;
}

CBaseEntity* CBasePlayerAmmo::Respawn()
{
	pev->effects |= EF_NODRAW;
	SetTouch(nullptr);

	SetOrigin(g_pGameRules->VecAmmoRespawnSpot(this)); // move to wherever I'm supposed to repawn.

	SetThink(&CBasePlayerAmmo::Materialize);
	pev->nextthink = g_pGameRules->FlAmmoRespawnTime(this);

	return this;
}

void CBasePlayerAmmo::Materialize()
{
	if ((pev->effects & EF_NODRAW) != 0)
	{
		EmitSound("items/itembk2.wav", CHAN_WEAPON);
		pev->effects &= ~EF_NODRAW;
	}

	SetTouch(&CBasePlayerAmmo::DefaultTouch);
}

void CBasePlayerAmmo::DefaultTouch(CBaseEntity* pOther)
{
	if (!pOther->IsPlayer())
	{
		return;
	}

	if (AddAmmo(pOther))
	{
		EmitSound("items/9mmclip1.wav", CHAN_ITEM);

		if (g_pGameRules->AmmoShouldRespawn(this) == GR_AMMO_RESPAWN_YES)
		{
			Respawn();
		}
		else
		{
			SetTouch(nullptr);
			Remove();
		}
	}
	else if (gEvilImpulse101)
	{
		// evil impulse 101 hack, kill always
		SetTouch(nullptr);
		Remove();
	}
}

LINK_ENTITY_TO_CLASS(weaponbox, CWeaponBox);

TYPEDESCRIPTION CWeaponBox::m_SaveData[] =
	{
		DEFINE_ARRAY(CWeaponBox, m_rgAmmo, FIELD_INTEGER, AMMO_LAST),
		DEFINE_ARRAY(CWeaponBox, m_rgpPlayerWeapons, FIELD_CLASSPTR, WEAPON_LAST),
		DEFINE_FIELD(CWeaponBox, m_cAmmoTypes, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CWeaponBox, CBaseEntity);

//=========================================================
//
//=========================================================
void CWeaponBox::Precache()
{
	PRECACHE_MODEL("models/w_weaponbox.mdl");
}

//=========================================================
//=========================================================
bool CWeaponBox::KeyValue(KeyValueData* pkvd)
{
	if (m_cAmmoTypes < AMMO_LAST)
	{
		// Toodles FIXME:
		// PackAmmo(ALLOC_STRING(pkvd->szKeyName), atoi(pkvd->szValue));
		m_cAmmoTypes++; // count this new ammo type.

		return true;
	}
	else
	{
		ALERT(at_console, "WeaponBox too full! only %d ammotypes allowed\n", AMMO_LAST);
	}

	return false;
}

//=========================================================
// CWeaponBox - Spawn
//=========================================================
bool CWeaponBox::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	SetSize(g_vecZero, g_vecZero);

	SetModel("models/w_weaponbox.mdl");

	return true;
}

//=========================================================
// CWeaponBox - Kill - the think function that removes the
// box from the world.
//=========================================================
void CWeaponBox::Kill()
{
	CBasePlayerWeapon* pWeapon;
	int i;

	// destroy the weapons
	for (i = 0; i < WEAPON_LAST; i++)
	{
		pWeapon = m_rgpPlayerWeapons[i];
		if (!pWeapon)
		{
			continue;
		}
		pWeapon->Remove();
	}

	// remove the box
	Remove();
}

//=========================================================
// CWeaponBox - Touch: try to add my contents to the toucher
// if the toucher is a player.
//=========================================================
void CWeaponBox::Touch(CBaseEntity* pOther)
{
	if ((pev->flags & FL_ONGROUND) == 0)
	{
		return;
	}

	if (!pOther->IsPlayer())
	{
		// only players may touch a weaponbox.
		return;
	}

	if (!pOther->IsAlive())
	{
		// no dead guys.
		return;
	}

	CBasePlayer* pPlayer = (CBasePlayer*)pOther;
	CBasePlayerWeapon* pWeapon;
	int i;

	// dole out ammo
	for (i = 0; i < AMMO_LAST; i++)
	{
		if (m_rgAmmo[i] > 0)
		{
			// there's some ammo of this type.
			pPlayer->GiveAmmo(m_rgAmmo[i], i, MaxAmmoCarry(i));
		}
	}

	// go through my weapons and try to give the usable ones to the player.
	// it's important the the player be given ammo first, so the weapons code doesn't refuse
	// to deploy a better weapon that the player may pick up because he has no ammo for it.
	for (i = 0; i < WEAPON_LAST; i++)
	{
		pWeapon = m_rgpPlayerWeapons[i];

		if (!pWeapon)
		{
			continue;
		}

		//ALERT ( at_console, "trying to give %s\n", STRING( pWeapon[ i ]->pev->classname ) );

		pWeapon->AddToPlayer(pPlayer);
	}

	pOther->EmitSound("items/gunpickup2.wav", CHAN_ITEM);
	SetTouch(nullptr);
	Remove();
}

//=========================================================
// CWeaponBox - PackWeapon: Add this weapon to the box
//=========================================================
bool CWeaponBox::PackWeapon(CBasePlayerWeapon* pWeapon)
{
	// is one of these weapons already packed in this box?
	if (HasWeapon(pWeapon))
	{
		return false;
	}

	pWeapon->RemoveFromPlayer();

	m_rgpPlayerWeapons[pWeapon->GetID()] = pWeapon;

	pWeapon->pev->spawnflags |= SF_NORESPAWN;
	pWeapon->pev->movetype = MOVETYPE_NONE;
	pWeapon->pev->solid = SOLID_NOT;
	pWeapon->pev->effects = EF_NODRAW;
	pWeapon->pev->owner = edict();
	pWeapon->SetThink(nullptr);
	pWeapon->SetTouch(nullptr);

	//ALERT ( at_console, "packed %s\n", STRING(pWeapon->pev->classname) );

	return true;
}

//=========================================================
// CWeaponBox - PackAmmo
//=========================================================
bool CWeaponBox::PackAmmo(int iType, int iCount)
{
	int iMaxCarry;

	if (iType <= AMMO_NONE)
	{
		return false;
	}

	iMaxCarry = MaxAmmoCarry(iType);

	if (iMaxCarry != -1 && iCount > 0)
	{
		//ALERT ( at_console, "Packed %d rounds of %i\n", iCount, iType );
		GiveAmmo(iCount, iType, iMaxCarry);
		return true;
	}

	return false;
}

//=========================================================
// CWeaponBox - GiveAmmo
//=========================================================
int CWeaponBox::GiveAmmo(int iCount, int iType, int iMax, int* pIndex /* = nullptr*/)
{
	int iAdd = std::min(iCount, iMax - m_rgAmmo[iType]);
	if (iCount == 0 || iAdd > 0)
	{
		m_rgAmmo[iType] += iAdd;

		return iType;
	}
	return -1;
}

//=========================================================
// CWeaponBox::HasWeapon - is a weapon of this type already
// packed in this box?
//=========================================================
bool CWeaponBox::HasWeapon(CBasePlayerWeapon* pCheckWeapon)
{
	return m_rgpPlayerWeapons[pCheckWeapon->GetID()] != nullptr;
}

//=========================================================
// CWeaponBox::IsEmpty - is there anything in this box?
//=========================================================
bool CWeaponBox::IsEmpty()
{
	int i;

	for (i = 0; i < WEAPON_LAST; i++)
	{
		if (m_rgpPlayerWeapons[i] != nullptr)
		{
			return false;
		}
	}

	for (i = 0; i < AMMO_LAST; i++)
	{
		if (m_rgAmmo[i] > 0)
		{
			// still have a bit of this type of ammo
			return false;
		}
	}

	return true;
}

//=========================================================
//=========================================================
void CWeaponBox::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector(-16, -16, 0);
	pev->absmax = pev->origin + Vector(16, 16, 16);
}
