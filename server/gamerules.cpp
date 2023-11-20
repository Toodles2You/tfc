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
//=========================================================
// GameRules.cpp
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "skill.h"
#include "game.h"
#include "UserMessages.h"

CSpawnPoint::CSpawnPoint() :
	m_origin{Vector(0, 0, -VEC_HULL_MIN.z + 1)},
	m_angles{Vector(0, 0, 0)},
	m_target{iStringNull},
	m_master{iStringNull},
	m_lastSpawnTime{-1000.0F}
{
}

CSpawnPoint::CSpawnPoint(CBaseEntity *pEntity) :
	m_origin{pEntity->pev->origin + Vector(0, 0, 1)},
	m_angles{pEntity->pev->angles},
	m_target{pEntity->pev->target},
	m_master{pEntity->pev->netname},
	m_lastSpawnTime{-1000.0F}
{
}

bool CSpawnPoint::IsValid(CBasePlayer *pPlayer, int attempt)
{
	if (!FStringNull(m_master)
	 && !UTIL_IsMasterTriggered(m_master, pPlayer))
	{
		return false;
	}

	if (attempt < 2)
	{
		if (attempt == 0)
		{
			if (gpGlobals->time - m_lastSpawnTime < 10.0F)
			{
				return false;
			}
		}
		else if (gpGlobals->time - m_lastSpawnTime < 3.0F)
		{
			return false;
		}
		
		CBaseEntity *entity = nullptr;
		while ((entity = UTIL_FindEntityInSphere(entity, m_origin, 128.0F)) != nullptr)
		{
			if (entity->IsPlayer() && entity != pPlayer)
			{
				return false;
			}
		}
	}
	
	return true;
}

void CGameRules::UpdateGameMode(CBasePlayer* pPlayer)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgGameMode, NULL, pPlayer->edict());
	WRITE_BYTE(GetGameMode());
	MESSAGE_END();
}

void CGameRules::InitHUD(CBasePlayer* pl)
{
	UpdateGameMode(pl);
}

CBasePlayerWeapon* CGameRules::FindNextBestWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pCurrentWeapon)
{
	if (pCurrentWeapon != nullptr && !pCurrentWeapon->CanHolster())
	{
		// can't put this gun away right now, so can't switch.
		return nullptr;
	}

	const int currentWeight = pCurrentWeapon != nullptr ? pCurrentWeapon->iWeight() : -1;

	CBasePlayerWeapon* pBest = nullptr; // this will be used in the event that we don't find a weapon in the same category.

	int iBestWeight = -1; // no weapon lower than -1 can be autoswitched to

	for (auto pCheck : pPlayer->m_lpPlayerWeapons)
	{
		// don't reselect the weapon we're trying to get rid of
		if (pCheck == pCurrentWeapon)
		{
			continue;
		}

		if (pCheck->iWeight() > -1 && pCheck->iWeight() == currentWeight)
		{
			// this weapon is from the same category.
			if (pCheck->CanDeploy())
			{
				if (pPlayer->SwitchWeapon(pCheck))
				{
					return pCheck;
				}
			}
		}
		else if (pCheck->iWeight() > iBestWeight)
		{
			//ALERT ( at_console, "Considering %s\n", STRING( pCheck->pev->classname ) );
			// we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
			// that the player was using. This will end up leaving the player with his heaviest-weighted
			// weapon.
			if (pCheck->CanDeploy())
			{
				// if this weapon is useable, flag it as the best
				iBestWeight = pCheck->iWeight();
				pBest = pCheck;
			}
		}
	}

	// if we make it here, we've checked all the weapons and found no useable
	// weapon in the same catagory as the current weapon.

	// if pBest is nullptr, we didn't find ANYTHING. Shouldn't be possible- should always
	// at least get the crowbar, but ya never know.

	return pBest;
}

bool CGameRules::GetNextBestWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pCurrentWeapon, bool alwaysSearch)
{
	if (auto pBest = FindNextBestWeapon(pPlayer, pCurrentWeapon); pBest != nullptr)
	{
		pPlayer->SwitchWeapon(pBest);
		return true;
	}

	return false;
}

//=========================================================
//=========================================================
bool CGameRules::CanHaveAmmo(CBasePlayer* pPlayer, int iAmmoType, int iMaxCarry)
{
	if (iAmmoType > AMMO_NONE)
	{
		if (pPlayer->AmmoInventory(iAmmoType) < iMaxCarry)
		{
			// player has room for more of this type of ammo
			return true;
		}
	}

	return false;
}

//=========================================================
//=========================================================
CSpawnPoint *CGameRules::GetPlayerSpawnSpot(CBasePlayer* pPlayer)
{
	return &m_startPoint;
}

//=========================================================
//=========================================================
void CGameRules::AddPlayerSpawnSpot(CBaseEntity *pEntity)
{
	if (!FStrEq(STRING(pEntity->pev->classname), "info_player_start"))
	{
		return;
	}
	m_startPoint = CSpawnPoint{pEntity};
}

//=========================================================
//=========================================================
bool CGameRules::CanHavePlayerWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon)
{
	// only living players can have weapons
	if (pPlayer->pev->deadflag != DEAD_NO)
		return false;

	if (pWeapon->iAmmo1() > AMMO_NONE)
	{
		if (!CanHaveAmmo(pPlayer, pWeapon->iAmmo1(), pWeapon->iMaxAmmo1()))
		{
			// we can't carry anymore ammo for this gun. We can only
			// have the gun if we aren't already carrying one of this type
			if (pPlayer->HasPlayerWeapon(pWeapon))
			{
				return false;
			}
		}
	}
	else
	{
		// weapon doesn't use ammo, don't take another if you already have it.
		if (pPlayer->HasPlayerWeapon(pWeapon))
		{
			return false;
		}
	}

	// note: will fall through to here if GetWeaponInfo doesn't fill the struct!
	return true;
}

//=========================================================
// load the SkillData struct with the proper values based on the skill level.
//=========================================================
void CGameRules::RefreshSkillData()
{
}

//=========================================================
//=========================================================
bool CGameRules::IsPlayerPrivileged(CBasePlayer* pPlayer)
{
	return g_psv_cheats->value != 0;
}

//=========================================================
// instantiate the proper game rules object
//=========================================================

CGameRules* InstallGameRules()
{
	SERVER_COMMAND("exec game.cfg\n");
	SERVER_EXECUTE();

	gpGlobals->teamplay = teamplay.value;

	if ((int)gpGlobals->coop != 0)
	{
		// coop
		return new CHalfLifeMultiplay;
	}
	
	if ((int)gpGlobals->deathmatch != 0)
	{
		if ((int)gpGlobals->teamplay != 0)
		{
			// teamplay
			return new CHalfLifeTeamplay;
		}
		// vanilla deathmatch
		return new CHalfLifeMultiplay;
	}
	
	// generic half-life
	return new CHalfLifeRules;
}
