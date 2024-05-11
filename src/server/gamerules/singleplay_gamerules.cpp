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
//
// teamplay_gamerules.cpp
//
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "weaponbox.h"
#include "gamerules.h"
#include "items.h"
#include "UserMessages.h"


CHalfLifeRules::CHalfLifeRules()
{
}


void CHalfLifeRules::Think()
{
}


bool CHalfLifeRules::FShouldSwitchWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon)
{
	if (!pPlayer->m_pActiveWeapon)
	{
		return true;
	}

	if (!pPlayer->m_pActiveWeapon->CanHolster())
	{
		return false;
	}

	return true;
}


bool CHalfLifeRules::ClientConnected(Entity* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128])
{
	return true;
}


void CHalfLifeRules::ClientPutInServer(CBasePlayer* pPlayer)
{
}


void CHalfLifeRules::ClientDisconnected(Entity* pClient)
{
}


float CHalfLifeRules::FlPlayerFallDamage(CBasePlayer* pPlayer)
{
	// subtract off the speed at which a player is allowed to fall without being hurt,
	// so damage will be based on speed beyond that, not the entire fall
	pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
}


void CHalfLifeRules::PlayerSpawn(CBasePlayer* pPlayer)
{
}


void CHalfLifeRules::PlayerThink(CBasePlayer* pPlayer)
{
}


bool CHalfLifeRules::FPlayerCanRespawn(CBasePlayer* pPlayer)
{
	return true;
}


float CHalfLifeRules::FlPlayerSpawnTime(CBasePlayer* pPlayer)
{
	return gpGlobals->time;
}


bool CHalfLifeRules::FPlayerCanSuicide(CBasePlayer* pPlayer)
{
	return false;
}


float CHalfLifeRules::GetPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled, bool assist)
{
	if (assist)
	{
		return 0.5F;
	}
	return 1;
}


void CHalfLifeRules::PlayerKilled(CBasePlayer* pVictim, CBaseEntity* killer, CBaseEntity* inflictor, CBaseEntity* accomplice, int bitsDamageType)
{
}


void CHalfLifeRules::DeathNotice(CBasePlayer* pVictim, CBaseEntity* killer, CBaseEntity* inflictor, CBaseEntity* accomplice, int bitsDamageType)
{
}


void CHalfLifeRules::PlayerGotWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon)
{
}


float CHalfLifeRules::FlWeaponRespawnTime(CBasePlayerWeapon* pWeapon)
{
	return -1;
}


float CHalfLifeRules::FlWeaponTryRespawn(CBasePlayerWeapon* pWeapon)
{
	return 0;
}


Vector CHalfLifeRules::VecWeaponRespawnSpot(CBasePlayerWeapon* pWeapon)
{
	return pWeapon->v.origin;
}


int CHalfLifeRules::WeaponShouldRespawn(CBasePlayerWeapon* pWeapon)
{
	return GR_WEAPON_RESPAWN_NO;
}


int CHalfLifeRules::ItemShouldRespawn(CItem* pItem)
{
	return GR_ITEM_RESPAWN_NO;
}


float CHalfLifeRules::FlItemRespawnTime(CItem* pItem)
{
	return -1;
}


Vector CHalfLifeRules::VecItemRespawnSpot(CItem* pItem)
{
	return pItem->v.origin;
}


bool CHalfLifeRules::IsAllowedToSpawn(CBaseEntity* pEntity)
{
	return true;
}


int CHalfLifeRules::AmmoShouldRespawn(CBasePlayerAmmo* pAmmo)
{
	return GR_AMMO_RESPAWN_NO;
}


float CHalfLifeRules::FlAmmoRespawnTime(CBasePlayerAmmo* pAmmo)
{
	return -1;
}


Vector CHalfLifeRules::VecAmmoRespawnSpot(CBasePlayerAmmo* pAmmo)
{
	return pAmmo->v.origin;
}


int CHalfLifeRules::PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget)
{
	// why would a single player in half life need this?
	return GR_NOTTEAMMATE;
}
