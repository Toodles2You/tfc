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
#include "decals.h"
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
	gMultiDamage.pEntity = NULL;
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
		WeaponInfo II;
		pEntity->Precache();
		memset(&II, 0, sizeof II);
		if (((CBasePlayerWeapon*)pEntity)->GetWeaponInfo(&II))
		{
			CBasePlayerWeapon::WeaponInfoArray[II.iId] = II;

			memset(&II, 0, sizeof II);
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
	util::PrecacheOther("item_antidote");
	util::PrecacheOther("item_security");
	util::PrecacheOther("item_longjump");

	if (util::IsMultiplayer())
	{
		util::PrecacheOther("weaponbox"); // container for dropped deathmatch weapons
	}

	g_sModelIndexShell = PRECACHE_MODEL("models/shell.mdl");

	g_sModelIndexLaser = PRECACHE_MODEL("sprites/laserbeam.spr");
	g_sModelIndexLaserDot = PRECACHE_MODEL("sprites/laserdot.spr");
	g_sModelIndexFireball = PRECACHE_MODEL("sprites/zerogxplode.spr");	// fireball
	g_sModelIndexWExplosion = PRECACHE_MODEL("sprites/WXplo1.spr");		// underwater fireball
	g_sModelIndexSmoke = PRECACHE_MODEL("sprites/steam1.spr");			// smoke
	g_sModelIndexBubbles = PRECACHE_MODEL("sprites/bubble.spr");		//bubbles
	g_sModelIndexBloodSpray = PRECACHE_MODEL("sprites/bloodspray.spr"); // initial blood
	g_sModelIndexBloodDrop = PRECACHE_MODEL("sprites/blood.spr");		// splattered blood

	// used by explosions
	PRECACHE_MODEL("models/grenade.mdl");
	PRECACHE_MODEL("sprites/explode1.spr");

	PRECACHE_MODEL("models/shell.mdl"); // brass shell
	PRECACHE_MODEL("models/shotgunshell.mdl"); // shotgun shell

	PRECACHE_SOUND("weapons/debris1.wav"); // explosion aftermaths
	PRECACHE_SOUND("weapons/debris2.wav"); // explosion aftermaths
	PRECACHE_SOUND("weapons/debris3.wav"); // explosion aftermaths

	PRECACHE_SOUND("weapons/grenade_hit1.wav"); //grenade
	PRECACHE_SOUND("weapons/grenade_hit2.wav"); //grenade
	PRECACHE_SOUND("weapons/grenade_hit3.wav"); //grenade

	PRECACHE_SOUND("weapons/bullet_hit1.wav"); // hit by bullet
	PRECACHE_SOUND("weapons/bullet_hit2.wav"); // hit by bullet

	PRECACHE_SOUND("weapons/357_cock1.wav");

	PRECACHE_SOUND("items/weapondrop1.wav"); // weapon falls to the ground

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");

	PRECACHE_SOUND("weapons/desert_eagle_sight.wav");
	PRECACHE_SOUND("weapons/desert_eagle_sight2.wav");
}


TYPEDESCRIPTION CBasePlayerWeapon::m_SaveData[] =
{
	DEFINE_FIELD(CBasePlayerWeapon, m_pPlayer, FIELD_CLASSPTR),
	//DEFINE_FIELD( CBasePlayerWeapon, m_fKnown, FIELD_INTEGER ),Reset to zero on load
	DEFINE_FIELD(CBasePlayerWeapon, m_iId, FIELD_INTEGER),
	// DEFINE_FIELD( CBasePlayerWeapon, m_iIdPrimary, FIELD_INTEGER ),
	// DEFINE_FIELD( CBasePlayerWeapon, m_iIdSecondary, FIELD_INTEGER ),
	DEFINE_FIELD(CBasePlayerWeapon, m_iNextPrimaryAttack, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayerWeapon, m_iNextSecondaryAttack, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayerWeapon, m_iTimeWeaponIdle, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayerWeapon, m_iClip, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayerWeapon, m_iDefaultAmmo, FIELD_INTEGER),
	//	DEFINE_FIELD( CBasePlayerWeapon, m_iClientClip, FIELD_INTEGER )	 , reset to zero on load so hud gets updated correctly
	//  DEFINE_FIELD( CBasePlayerWeapon, m_iClientWeaponState, FIELD_INTEGER ), reset to zero on load so hud gets updated correctly
};

IMPLEMENT_SAVERESTORE(CBasePlayerWeapon, CBaseAnimating);


void CBasePlayerWeapon::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector(-24, -24, 0);
	pev->absmax = pev->origin + Vector(24, 24, 16);
}


//=========================================================
// Sets up movetype, size, solidtype for a new weapon.
//=========================================================
void CBasePlayerWeapon::FallInit()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	SetSize(g_vecZero, g_vecZero); //pointsize until it lands on the ground.
	SetOrigin(pev->origin);

	SetTouch(&CBasePlayerWeapon::DefaultTouch);
	SetThink(&CBasePlayerWeapon::FallThink);

	pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
// FallThink - Items that have just spawned run this think
// to catch them when they hit the ground. Once we're sure
// that the object is grounded, we change its solid type
// to trigger and set it in a large box that helps the
// player get it.
//=========================================================
void CBasePlayerWeapon::FallThink()
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ((pev->flags & FL_ONGROUND) != 0)
	{
		// clatter if we have an owner (i.e., dropped by someone)
		// don't clatter if the gun is waiting to respawn (if it's waiting, it is invisible!)
		if (!FNullEnt(pev->owner))
		{
			EmitSound("items/weapondrop1.wav", CHAN_VOICE, VOL_NORM, ATTN_NORM, 95 + RANDOM_LONG(0, 29));
		}

		// lie flat
		pev->angles.x = 0;
		pev->angles.z = 0;

		Materialize();
	}
}

//=========================================================
// Materialize - make a CBasePlayerWeapon visible and tangible
//=========================================================
void CBasePlayerWeapon::Materialize()
{
	if ((pev->effects & EF_NODRAW) != 0)
	{
		// changing from invisible state to visible.
		EmitSound("items/itembk2.wav", CHAN_WEAPON);
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	pev->solid = SOLID_TRIGGER;

	SetOrigin(pev->origin);
	SetTouch(&CBasePlayerWeapon::DefaultTouch);
	SetThink(NULL);
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
		return;
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
	CBaseEntity* pNewWeapon = CBaseEntity::Create((char*)STRING(pev->classname), g_pGameRules->VecWeaponRespawnSpot(this), pev->angles, pev->owner);

	if (pNewWeapon)
	{
		pNewWeapon->pev->effects |= EF_NODRAW; // invisible for now
		pNewWeapon->SetTouch(NULL);			   // no touch
		pNewWeapon->SetThink(&CBasePlayerWeapon::AttemptToMaterialize);

		DROP_TO_FLOOR(ENT(pev));

		// not a typo! We want to know when the weapon the player just picked up should respawn! This new entity we created is the replacement,
		// but when it should respawn is based on conditions belonging to the weapon that was taken.
		pNewWeapon->pev->nextthink = g_pGameRules->FlWeaponRespawnTime(this);
	}
	else
	{
		ALERT(at_console, "Respawn failed to create %s!\n", STRING(pev->classname));
	}

	return pNewWeapon;
}

void CBasePlayerWeapon::DefaultTouch(CBaseEntity* pOther)
{
	// if it's not a player, ignore
	if (!pOther->IsPlayer())
		return;

	CBasePlayer* pPlayer = (CBasePlayer*)pOther;

	// can I have this?
	if (!g_pGameRules->CanHavePlayerWeapon(pPlayer, this))
	{
		if (gEvilImpulse101)
		{
			Remove();
		}
		return;
	}

	if (pOther->AddPlayerWeapon(this))
	{
		AttachToPlayer(pPlayer);
		if (!pPlayer->m_bIsSpawning)
		{
			pPlayer->EmitSound("items/gunpickup2.wav", CHAN_ITEM);
		}
	}

	UseTargets(pOther, USE_TOGGLE, 0); // UNDONE: when should this happen?
}

void CBasePlayerWeapon::DestroyWeapon()
{
	if (m_pPlayer)
	{
		// if attached to a player, remove.
		m_pPlayer->RemovePlayerWeapon(this);
	}

	Kill();
}

void CBasePlayerWeapon::Drop()
{
	SetTouch(NULL);
	Remove();
}

void CBasePlayerWeapon::Kill()
{
	SetTouch(NULL);
	Remove();
}

void CBasePlayerWeapon::AttachToPlayer(CBasePlayer* pPlayer)
{
	pev->movetype = MOVETYPE_FOLLOW;
	pev->solid = SOLID_NOT;
	pev->aiment = pPlayer->edict();
	pev->effects = EF_NODRAW; // ??
	pev->modelindex = 0;	  // server won't send down to clients if modelindex == 0
	pev->model = iStringNull;
	pev->owner = pPlayer->edict();
	pev->nextthink = gpGlobals->time + .1;
	SetTouch(NULL);
	SetThink(NULL); // Clear FallThink function so it can't run while attached to player.
}

// CALLED THROUGH the newly-touched weapon's instance. The existing player weapon is pOriginal
bool CBasePlayerWeapon::AddDuplicate(CBasePlayerWeapon* pOriginal)
{
	if (0 != m_iDefaultAmmo)
	{
		return ExtractAmmo(pOriginal);
	}
	else
	{
		// a dead player dropped this.
		return ExtractClipAmmo(pOriginal);
	}
}


void CBasePlayerWeapon::AddToPlayer(CBasePlayer* pPlayer)
{
	/*
	if ((iFlags() & WEAPON_FLAG_EXHAUSTIBLE) != 0 && m_iDefaultAmmo == 0 && m_iClip <= 0)
	{
		//This is an exhaustible weapon that has no ammo left. Don't add it, queue it up for destruction instead.
		SetThink(&CSatchel::DestroyWeapon);
		pev->nextthink = gpGlobals->time + 0.1;
		return false;
	}
	*/

	m_pPlayer = pPlayer;

	pPlayer->SetWeaponBit(m_iId);
}


void CBasePlayerWeapon::SendWeaponAnim(int iAnim, int body)
{
	const bool skiplocal = !m_ForceSendAnimations;

	m_pPlayer->pev->weaponanim = iAnim;

	if (skiplocal && ENGINE_CANSKIP(m_pPlayer->edict()))
		return;

	MessageBegin(MSG_ONE, SVC_WEAPONANIM, m_pPlayer);
	WriteByte(iAnim);	   // sequence number
	WriteByte(pev->body); // weaponmodel bodygroup.
	MessageEnd();
}

void CBasePlayerWeapon::PlayWeaponSound(int iChannel, const char* szSound, float flVolume, float flAttn, int iFlags, float flPitch)
{
	m_pPlayer->EmitSoundPredicted(szSound, iChannel, flVolume, flAttn, flPitch, iFlags);
}

bool CBasePlayerWeapon::AddPrimaryAmmo(CBasePlayerWeapon* origin, int iCount, int iType, int iMaxClip, int iMaxCarry)
{
	int iIdAmmo;

	if (iMaxClip < 1)
	{
		m_iClip = -1;
		iIdAmmo = m_pPlayer->GiveAmmo(iCount, iType, iMaxCarry);
	}
	else if (m_iClip == 0 && this == origin)
	{
		int i;
		i = std::min(m_iClip + iCount, iMaxClip) - m_iClip;
		m_iClip += i;
		iIdAmmo = m_pPlayer->GiveAmmo(iCount - i, iType, iMaxCarry);
	}
	else
	{
		iIdAmmo = m_pPlayer->GiveAmmo(iCount, iType, iMaxCarry);
	}

	// m_pPlayer->m_rgAmmo[iAmmo1()] = iMaxCarry; // hack for testing

	if (iIdAmmo > AMMO_NONE)
	{
		if (this != origin)
		{
			// play the "got ammo" sound only if we gave some ammo to a player that already had this gun.
			// if the player is just getting this gun for the first time, DefaultTouch will play the "picked up gun" sound for us.
			EmitSound("items/9mmclip1.wav", CHAN_ITEM);
		}
	}

	return iIdAmmo > 0 ? true : false;
}


bool CBasePlayerWeapon::AddSecondaryAmmo(int iCount, int iType, int iMax)
{
	int iIdAmmo;

	iIdAmmo = m_pPlayer->GiveAmmo(iCount, iType, iMax);

	if (iIdAmmo > AMMO_NONE)
	{
		EmitSound("items/9mmclip1.wav", CHAN_ITEM);
	}
	return iIdAmmo > 0 ? true : false;
}

//=========================================================
// IsUseable - this function determines whether or not a
// weapon is useable by the player in its current state.
// (does it have ammo loaded? do I have any ammo for the
// weapon?, etc)
//=========================================================
bool CBasePlayerWeapon::IsUseable()
{
	if (m_iClip > 0)
	{
		return true;
	}

	//Player has unlimited ammo for this weapon or does not use magazines
	if (iMaxAmmo1() == WEAPON_NOCLIP)
	{
		return true;
	}

	if (m_pPlayer->m_rgAmmo[iAmmo1()] > 0)
	{
		return true;
	}

	if (iAmmo2() > AMMO_NONE)
	{
		//Player has unlimited ammo for this weapon or does not use magazines
		if (iMaxAmmo2() == WEAPON_NOCLIP)
		{
			return true;
		}

		if (m_pPlayer->m_rgAmmo[iAmmo2()] > 0)
		{
			return true;
		}
	}

	// clip is empty (or nonexistant) and the player has no more ammo of this type.
	return false;
}

bool CBasePlayerWeapon::DefaultDeploy(const char* szViewModel, const char* szWeaponModel, int iAnim, const char* szAnimExt, int body)
{
	if (!CanDeploy())
		return false;

	m_pPlayer->pev->viewmodel = g_engfuncs.pfnModelIndex(szViewModel);
	m_pPlayer->pev->weaponmodel = g_engfuncs.pfnModelIndex(szWeaponModel);
	strcpy(m_pPlayer->m_szAnimExtention, szAnimExt);
	SendWeaponAnim(iAnim, body);

	m_pPlayer->m_iNextAttack = 500;
	m_iNextPrimaryAttack = std::max(m_iNextPrimaryAttack, 500);
	m_iNextSecondaryAttack = std::max(m_iNextSecondaryAttack, 500);
	m_iTimeWeaponIdle = 1000;
	m_bPlayEmptySound = true;

	return true;
}

bool CBasePlayerWeapon::DefaultHolster(int iAnim, int body)
{
	if (!CanHolster())
		return false;

	if (iAnim >= 0)
		SendWeaponAnim(iAnim, body);

	m_pPlayer->m_iNextAttack = 500;
	m_iNextPrimaryAttack = std::max(m_iNextPrimaryAttack, 500);
	m_iNextSecondaryAttack = std::max(m_iNextSecondaryAttack, 500);
	m_iTimeWeaponIdle = 1000;
	m_fInReload = false; // Cancel any reload in progress.
	m_fInSpecialReload = 0;
	m_pPlayer->m_iFOV = 0;

	return true;
}

void CBasePlayerWeapon::PlayEmptySound()
{
	if (m_bPlayEmptySound)
	{
		PlayWeaponSound(CHAN_ITEM, "weapons/357_cock1.wav", 0.8);
		m_bPlayEmptySound = false;
	}
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
	SetTouch(NULL);

	SetOrigin(g_pGameRules->VecAmmoRespawnSpot(this)); // move to wherever I'm supposed to repawn.

	SetThink(&CBasePlayerAmmo::Materialize);
	pev->nextthink = g_pGameRules->FlAmmoRespawnTime(this);

	return this;
}

void CBasePlayerAmmo::Materialize()
{
	if ((pev->effects & EF_NODRAW) != 0)
	{
		// changing from invisible state to visible.
		EmitSound("items/itembk2.wav", CHAN_WEAPON);
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
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
			SetTouch(NULL);
			Remove();
		}
	}
	else if (gEvilImpulse101)
	{
		// evil impulse 101 hack, kill always
		SetTouch(NULL);
		Remove();
	}
}

//=========================================================
// called by the new weapon with the existing weapon as parameter
//
// if we call ExtractAmmo(), it's because the player is picking up this type of weapon for
// the first time. If it is spawned by the world, m_iDefaultAmmo will have a default ammo amount in it.
// if  this is a weapon dropped by a dying player, has 0 m_iDefaultAmmo, which means only the ammo in
// the weapon clip comes along.
//=========================================================
bool CBasePlayerWeapon::ExtractAmmo(CBasePlayerWeapon* pWeapon)
{
	bool iReturn = false;

	if (iAmmo1() > AMMO_NONE)
	{
		// blindly call with m_iDefaultAmmo. It's either going to be a value or zero. If it is zero,
		// we only get the ammo in the weapon's clip, which is what we want.
		iReturn = pWeapon->AddPrimaryAmmo(this, m_iDefaultAmmo, iAmmo1(), iMaxClip(), iMaxAmmo1());
		m_iDefaultAmmo = 0;
	}

	if (iAmmo2() > AMMO_NONE)
	{
		iReturn |= pWeapon->AddSecondaryAmmo(0, iAmmo2(), iMaxAmmo2());
	}

	return iReturn;
}

//=========================================================
// called by the new weapon's class with the existing weapon as parameter
//=========================================================
bool CBasePlayerWeapon::ExtractClipAmmo(CBasePlayerWeapon* pWeapon)
{
	int iAmmo;

	if (m_iClip == WEAPON_NOCLIP)
	{
		iAmmo = 0; // guns with no clips always come empty if they are second-hand
	}
	else
	{
		iAmmo = m_iClip;
	}

	//TODO: should handle -1 return as well (only return true if ammo was taken)
	return pWeapon->m_pPlayer->GiveAmmo(iAmmo, iAmmo1(), iMaxAmmo1()) != 0; // , &m_iPrimaryAmmoType
}

//=========================================================
// RetireWeapon - no more ammo for this gun, put it away.
//=========================================================
void CBasePlayerWeapon::RetireWeapon()
{
	SetThink(&CBasePlayerWeapon::CallDoRetireWeapon);
	pev->nextthink = gpGlobals->time + 0.01f;
}

void CBasePlayerWeapon::DoRetireWeapon()
{
	if (!m_pPlayer || m_pPlayer->m_pActiveWeapon != this)
	{
		// Already retired?
		return;
	}

	// first, no viewmodel at all.
	m_pPlayer->pev->viewmodel = 0;
	m_pPlayer->pev->weaponmodel = 0;

	g_pGameRules->GetNextBestWeapon(m_pPlayer, this);

	//If we're still equipped and we couldn't switch to another weapon, dequip this one
	if (CanHolster() && m_pPlayer->m_pActiveWeapon == this)
	{
		m_pPlayer->SwitchWeapon(nullptr);
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

		if (pPlayer->AddPlayerWeapon(pWeapon))
		{
			pWeapon->AttachToPlayer(pPlayer);
		}
	}

	pOther->EmitSound("items/gunpickup2.wav", CHAN_ITEM);
	SetTouch(NULL);
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
		return false; // box can only hold one of each weapon type
	}

	if (pWeapon->m_pPlayer)
	{
		if (!pWeapon->m_pPlayer->RemovePlayerWeapon(pWeapon))
		{
			// failed to unhook the weapon from the player!
			return false;
		}
	}

	m_rgpPlayerWeapons[pWeapon->m_iId] = pWeapon;

	pWeapon->pev->spawnflags |= SF_NORESPAWN; // never respawn
	pWeapon->pev->movetype = MOVETYPE_NONE;
	pWeapon->pev->solid = SOLID_NOT;
	pWeapon->pev->effects = EF_NODRAW;
	pWeapon->pev->modelindex = 0;
	pWeapon->pev->model = iStringNull;
	pWeapon->pev->owner = edict();
	pWeapon->SetThink(NULL); // crowbar may be trying to swing again, etc.
	pWeapon->SetTouch(NULL);
	pWeapon->m_pPlayer = NULL;

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
int CWeaponBox::GiveAmmo(int iCount, int iType, int iMax, int* pIndex /* = NULL*/)
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
	if (pCheckWeapon->m_iId <= WEAPON_NONE || pCheckWeapon->m_iId >= WEAPON_LAST)
	{
		return false;
	}
	return m_rgpPlayerWeapons[pCheckWeapon->m_iId] != nullptr;
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
