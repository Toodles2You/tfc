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

===== weapons.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "UserMessages.h"


//=========================================================
// MaxAmmoCarry - pass in a name and this function will tell
// you the maximum amount of that type of ammunition that a
// player can carry.
//=========================================================
int MaxAmmoCarry(int iType)
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


/*
==============================================================================

MULTI-DAMAGE

Collects multiple small damages into a single damage

==============================================================================
*/

//
// ClearMultiDamage - resets the global multi damage accumulator
//
void ClearMultiDamage()
{
	gMultiDamage.pEntity = nullptr;
	gMultiDamage.amount = 0;
	gMultiDamage.type = 0;
}


//
// ApplyMultiDamage - inflicts contents of global multi damage register on gMultiDamage.pEntity
//
// GLOBALS USED:
//		gMultiDamage

void ApplyMultiDamage(CBaseEntity* inflictor, CBaseEntity* attacker)
{
	Vector vecSpot1; //where blood comes from
	Vector vecDir;	 //direction blood should go
	TraceResult tr;

	if (!gMultiDamage.pEntity)
		return;

	gMultiDamage.pEntity->TakeDamage(inflictor, attacker, gMultiDamage.amount, gMultiDamage.type);
}


// GLOBALS USED:
//		gMultiDamage

void AddMultiDamage(CBaseEntity *inflictor, CBaseEntity *attacker, CBaseEntity* pEntity, float flDamage, int bitsDamageType)
{
	if (!pEntity)
		return;

	gMultiDamage.type |= bitsDamageType;

	if (pEntity != gMultiDamage.pEntity)
	{
		ApplyMultiDamage(inflictor, attacker); // UNDONE: wrong attacker!
		gMultiDamage.pEntity = pEntity;
		gMultiDamage.amount = 0;
	}

	gMultiDamage.amount += flDamage;
}

// Precaches the weapon and queues the weapon info for sending to clients
void util::PrecacheWeapon(const char* szClassname)
{
	edict_t* pent;

	pent = CREATE_NAMED_ENTITY(MAKE_STRING(szClassname));
	if (FNullEnt(pent))
	{
		ALERT(at_console, "NULL Ent in UTIL_PrecacheOtherWeapon\n");
		return;
	}

	CBaseEntity* pEntity = CBaseEntity::Instance(VARS(pent));

	if (pEntity)
	{
		const auto weapon = dynamic_cast<CBasePlayerWeapon*>(pEntity);
		WeaponInfo II;
		pEntity->Precache();
		memset(&II, 0, sizeof II);
		if (weapon->GetWeaponInfo(&II))
		{
			CBasePlayerWeapon::WeaponInfoArray[weapon->GetID()] = II;

			memset(&II, 0, sizeof(II));
		}
	}

	g_engfuncs.pfnRemoveEntity(pent);
}

// called by worldspawn
void W_Precache()
{
	memset(CBasePlayerWeapon::WeaponInfoArray, 0, sizeof(CBasePlayerWeapon::WeaponInfoArray));

	// weapons
	util::PrecacheWeapon("weapon_crowbar");
	util::PrecacheWeapon("weapon_9mmAR");

	// common world objects
	util::PrecacheOther("item_battery");
	util::PrecacheOther("item_healthkit");

	if (util::IsMultiplayer())
	{
		util::PrecacheOther("weaponbox");
	}

	g_sModelIndexShell = PRECACHE_MODEL("models/shell.mdl");

	g_sModelIndexLaser = PRECACHE_MODEL("sprites/laserbeam.spr");
	g_sModelIndexLaserDot = PRECACHE_MODEL("sprites/laserdot.spr");
	g_sModelIndexFireball = PRECACHE_MODEL("sprites/zerogxplode.spr");
	g_sModelIndexWExplosion = PRECACHE_MODEL("sprites/WXplo1.spr");
	g_sModelIndexSmoke = PRECACHE_MODEL("sprites/steam1.spr");
	g_sModelIndexBubbles = PRECACHE_MODEL("sprites/bubble.spr");
	g_sModelIndexBloodSpray = PRECACHE_MODEL("sprites/bloodspray.spr");
	g_sModelIndexBloodDrop = PRECACHE_MODEL("sprites/blood.spr");

	PRECACHE_MODEL("models/grenade.mdl");
	PRECACHE_MODEL("sprites/explode1.spr");

	PRECACHE_MODEL("models/shell.mdl");
	PRECACHE_MODEL("models/shotgunshell.mdl");

	PRECACHE_SOUND("weapons/debris1.wav");
	PRECACHE_SOUND("weapons/debris2.wav");
	PRECACHE_SOUND("weapons/debris3.wav");

	PRECACHE_SOUND("weapons/grenade_hit1.wav");
	PRECACHE_SOUND("weapons/grenade_hit2.wav");
	PRECACHE_SOUND("weapons/grenade_hit3.wav");

	PRECACHE_SOUND("weapons/bullet_hit1.wav");
	PRECACHE_SOUND("weapons/bullet_hit2.wav");

	PRECACHE_SOUND("items/weapondrop1.wav");

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");

	PRECACHE_SOUND("weapons/desert_eagle_sight.wav");
	PRECACHE_SOUND("weapons/desert_eagle_sight2.wav");
}


TYPEDESCRIPTION CBasePlayerWeapon::m_SaveData[] =
{
	DEFINE_FIELD(CBasePlayerWeapon, m_pPlayer, FIELD_CLASSPTR),
	DEFINE_FIELD(CBasePlayerWeapon, m_iClip, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CBasePlayerWeapon, CBaseAnimating);


bool CBasePlayerWeapon::Spawn()
{
	const auto info = GetInfo();

	Precache();

	SetModel(info.pszWorld);

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	g_engfuncs.pfnDropToFloor(edict());

	m_iClip = iMaxClip();

	Materialize();

	return true;
}


void CBasePlayerWeapon::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector(-24, -24, 0);
	pev->absmax = pev->origin + Vector(24, 24, 16);
}


//=========================================================
// Materialize - make a CBasePlayerWeapon visible and tangible
//=========================================================
void CBasePlayerWeapon::Materialize()
{
	if ((pev->effects & EF_NODRAW) != 0)
	{
		EmitSound("items/itembk2.wav", CHAN_WEAPON);
		pev->effects &= ~EF_NODRAW;
	}

	pev->solid = SOLID_TRIGGER;

	SetOrigin(pev->origin);
	SetTouch(&CBasePlayerWeapon::DefaultTouch);
	SetThink(nullptr);
}


//=========================================================
// AttemptToMaterialize - the weapon is trying to rematerialize,
// should it do so now or wait longer?
//=========================================================
void CBasePlayerWeapon::AttemptToMaterialize()
{
	float time = g_pGameRules->FlWeaponTryRespawn(this);

	if (time == 0)
	{
		Materialize();
		return;
	}

	pev->nextthink = time;
}


//=========================================================
// CheckRespawn - a player is taking this weapon, should
// it respawn?
//=========================================================
void CBasePlayerWeapon::CheckRespawn()
{
	switch (g_pGameRules->WeaponShouldRespawn(this))
	{
	case GR_WEAPON_RESPAWN_YES:
		Respawn();
		break;
	case GR_WEAPON_RESPAWN_NO:
		break;
	}
}


//=========================================================
// Respawn- this weapon is already in the world, but it is
// invisible and intangible. Make it visible and tangible.
//=========================================================
CBaseEntity* CBasePlayerWeapon::Respawn()
{
	// make a copy of this weapon that is invisible and inaccessible to players (no touch function). The weapon spawn/respawn code
	// will decide when to make the weapon visible and touchable.
	CBaseEntity* pNewWeapon =
		CBaseEntity::Create(
			(char*)STRING(pev->classname),
			g_pGameRules->VecWeaponRespawnSpot(this),
			pev->angles,
			pev->owner);

	if (pNewWeapon != nullptr)
	{
		pNewWeapon->pev->effects |= EF_NODRAW;
		pNewWeapon->SetTouch(nullptr);
		pNewWeapon->SetThink(&CBasePlayerWeapon::AttemptToMaterialize);

		g_engfuncs.pfnDropToFloor(edict());

		pNewWeapon->pev->nextthink = g_pGameRules->FlWeaponRespawnTime(this);
	}

	return pNewWeapon;
}


void CBasePlayerWeapon::DefaultTouch(CBaseEntity* pOther)
{
	if (!pOther->IsPlayer() || !pOther->IsAlive())
	{
		return;
	}

	CBasePlayer* pPlayer = (CBasePlayer*)pOther;

	if (AddToPlayer(pPlayer))
	{
		if (!pPlayer->m_bIsSpawning)
		{
			pPlayer->EmitSound("items/gunpickup2.wav", CHAN_ITEM);
		}

		UseTargets(pOther, USE_TOGGLE, 0);
	}
}


void CBasePlayerWeapon::UpdateOnRemove()
{
	RemoveFromPlayer();
	CBaseAnimating::UpdateOnRemove();
}


bool CBasePlayerWeapon::AddToPlayer(CBasePlayer* pPlayer)
{
	if (!g_pGameRules->CanHavePlayerWeapon(pPlayer, this))
	{
		return false;
	}

	if (pPlayer->HasPlayerWeapon(GetID()))
	{
		return AddDuplicate(pPlayer->m_rgpPlayerWeapons[GetID()]);
	}

	m_pPlayer = pPlayer;

	pev->owner = pev->aiment = pPlayer->edict();

	pev->movetype = MOVETYPE_FOLLOW;
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	SetTouch(nullptr);
	SetThink(nullptr);

	pPlayer->AddPlayerWeapon(this);

	CheckRespawn();

	if (gEvilImpulse101)
	{
		Remove();
	}

	return true;
}


void CBasePlayerWeapon::RemoveFromPlayer()
{
	if (m_pPlayer == nullptr)
	{
		return;
	}

	m_pPlayer->RemovePlayerWeapon(this);

	pev->owner = pev->aiment = nullptr;
	m_pPlayer = nullptr;
}


void CBasePlayerWeapon::SendWeaponAnim(int iAnim)
{
	const bool skiplocal = !m_ForceSendAnimations;

	m_pPlayer->pev->weaponanim = iAnim;

	if (skiplocal && ENGINE_CANSKIP(m_pPlayer->edict()))
		return;

	MessageBegin(MSG_ONE, SVC_WEAPONANIM, m_pPlayer);
	WriteByte(iAnim);
	WriteByte(pev->body);
	MessageEnd();
}


void CBasePlayerWeapon::PlayWeaponSound(int iChannel, const char* szSound, float flVolume, float flAttn, int iFlags, float flPitch)
{
	m_pPlayer->EmitSoundPredicted(szSound, iChannel, flVolume, flAttn, flPitch, iFlags);
}


void CBasePlayerWeapon::Deploy()
{
	const auto info = GetInfo();

	m_pPlayer->pev->viewmodel = g_engfuncs.pfnModelIndex(info.pszView);
	m_pPlayer->pev->weaponmodel = g_engfuncs.pfnModelIndex(info.pszPlayer);
	strcpy(m_pPlayer->m_szAnimExtention, info.pszAnimExt);

	SendWeaponAnim(info.iAnims[kWeaponAnimDeploy]);

	m_iNextPrimaryAttack = std::max(m_iNextPrimaryAttack, 500);
}

void CBasePlayerWeapon::Holster()
{
	const auto info = GetInfo();

	SendWeaponAnim(info.iAnims[kWeaponAnimHolster]);

	m_iNextPrimaryAttack = std::max(m_iNextPrimaryAttack, 500);
	m_fInReload = false;
	m_pPlayer->m_iFOV = 0;
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


//*********************************************************
// weaponbox code:
//*********************************************************

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
