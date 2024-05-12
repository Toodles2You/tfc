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

bool CBasePlayerAmmo::Spawn()
{
	v.movetype = MOVETYPE_TOSS;
	v.solid = SOLID_TRIGGER;
	SetSize(Vector(-16, -16, 0), Vector(16, 16, 16));
	SetOrigin(v.origin);

	SetTouch(&CBasePlayerAmmo::DefaultTouch);

	return true;
}

CBaseEntity* CBasePlayerAmmo::Respawn()
{
	v.effects |= EF_NODRAW;
	ClearTouch();

	SetOrigin(g_pGameRules->VecAmmoRespawnSpot(this)); // move to wherever I'm supposed to repawn.

	SetThink(&CBasePlayerAmmo::Materialize);
	v.nextthink = g_pGameRules->FlAmmoRespawnTime(this);

	return this;
}

void CBasePlayerAmmo::Materialize()
{
	if ((v.effects & EF_NODRAW) != 0)
	{
		EmitSound("items/itembk2.wav", CHAN_WEAPON);
		v.effects &= ~EF_NODRAW;
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
			ClearTouch();
			Remove();
		}
	}
	else if (gEvilImpulse101)
	{
		// evil impulse 101 hack, kill always
		ClearTouch();
		Remove();
	}
}

LINK_ENTITY_TO_CLASS(weaponbox, CWeaponBox);

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CWeaponBox)
	DEFINE_ARRAY(CWeaponBox, m_rgAmmo, FIELD_INTEGER, AMMO_TYPES),
	DEFINE_ARRAY(CWeaponBox, m_rgpPlayerWeapons, FIELD_CLASSPTR, WEAPON_TYPES),
	DEFINE_FIELD(CWeaponBox, m_cAmmoTypes, FIELD_INTEGER),
END_SAVERESTORE(CWeaponBox, CBaseEntity)
#endif

CWeaponBox::~CWeaponBox()
{
	RemoveWeapons();
}

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
	if (m_cAmmoTypes != AMMO_TYPES)
	{
		// Toodles FIXME:
		m_cAmmoTypes++; // count this new ammo type.
		return true;
	}

	return false;
}

//=========================================================
// CWeaponBox - Spawn
//=========================================================
bool CWeaponBox::Spawn()
{
	Precache();

	v.movetype = MOVETYPE_TOSS;
	v.solid = SOLID_TRIGGER;

	SetSize(g_vecZero, g_vecZero);

	SetModel("models/w_weaponbox.mdl");

	return true;
}

//=========================================================
// CWeaponBox - Kill - the think function that removes the
// box from the world.
//=========================================================
void CWeaponBox::RemoveWeapons()
{
	CBaseEntity* pWeapon;
	int i;

	// destroy the weapons
	for (i = 0; i < WEAPON_TYPES; i++)
	{
		pWeapon = m_hPlayerWeapons[i];
		if (pWeapon != nullptr)
		{
			pWeapon->Remove();
			m_hPlayerWeapons[i] = nullptr;
		}
	}
}

//=========================================================
// CWeaponBox - Touch: try to add my contents to the toucher
// if the toucher is a player.
//=========================================================
void CWeaponBox::Touch(CBaseEntity* pOther)
{
	if ((v.flags & FL_ONGROUND) == 0)
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
	CBaseEntity* pWeapon;
	int i;

	// dole out ammo
	for (int i = 0; i < AMMO_TYPES; i++)
	{
		if (m_rgAmmo[i] != 0)
		{
			// there's some ammo of this type.
			pPlayer->GiveAmmo(m_rgAmmo[i], i);
		}
	}

	// go through my weapons and try to give the usable ones to the player.
	// it's important the the player be given ammo first, so the weapons code doesn't refuse
	// to deploy a better weapon that the player may pick up because he has no ammo for it.
	for (int i = 0; i < WEAPON_TYPES; i++)
	{
		pWeapon = m_hPlayerWeapons[i];

		if (pWeapon != nullptr)
		{
			if (dynamic_cast<CBasePlayerWeapon*>(pWeapon)->AddToPlayer(pPlayer))
			{
				m_hPlayerWeapons[i] = nullptr;
			}
		}
	}

	pOther->EmitSound("items/gunpickup2.wav", CHAN_ITEM);
	ClearTouch();
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

	m_hPlayerWeapons[pWeapon->GetID()] = pWeapon;

	pWeapon->v.spawnflags |= SF_NORESPAWN;
	pWeapon->v.movetype = MOVETYPE_NONE;
	pWeapon->v.solid = SOLID_NOT;
	pWeapon->v.effects = EF_NODRAW;
	pWeapon->v.owner = edict();
	pWeapon->ClearThink();
	pWeapon->ClearTouch();

	return true;
}

//=========================================================
// CWeaponBox - PackAmmo
//=========================================================
bool CWeaponBox::PackAmmo(int iType, int iCount)
{
	if (iCount != 0)
	{
		//ALERT ( at_console, "Packed %d rounds of %i\n", iCount, iType );
		m_rgAmmo[iType] += iCount;
		return true;
	}

	return false;
}

//=========================================================
// CWeaponBox::HasWeapon - is a weapon of this type already
// packed in this box?
//=========================================================
bool CWeaponBox::HasWeapon(CBasePlayerWeapon* pCheckWeapon)
{
	return m_hPlayerWeapons[pCheckWeapon->GetID()] != nullptr;
}

//=========================================================
// CWeaponBox::IsEmpty - is there anything in this box?
//=========================================================
bool CWeaponBox::IsEmpty()
{
	for (int i = 0; i < WEAPON_TYPES; i++)
	{
		if (m_hPlayerWeapons[i] != nullptr)
		{
			return false;
		}
	}

	for (int i = 0; i < AMMO_TYPES; i++)
	{
		if (m_rgAmmo[i] != 0)
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
	v.absmin = v.origin + Vector(-16, -16, 0);
	v.absmax = v.origin + Vector(16, 16, 16);
}
