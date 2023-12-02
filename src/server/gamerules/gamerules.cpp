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
	 && !util::IsMasterTriggered(m_master, pPlayer))
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
		while ((entity = util::FindEntityInSphere(entity, m_origin, 128.0F)) != nullptr)
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
	MessageBegin(MSG_ONE, gmsgGameMode, pPlayer);
	WriteByte(GetGameMode());
	MessageEnd();
}

void CGameRules::InitHUD(CBasePlayer* pl)
{
	UpdateGameMode(pl);
}

//=========================================================
//=========================================================
bool CGameRules::CanHaveAmmo(CBasePlayer* pPlayer, int iAmmoType, int iMaxCarry)
{
	return pPlayer->AmmoInventory(iAmmoType) < iMaxCarry;
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
			if (pPlayer->HasPlayerWeapon(pWeapon->GetID()))
			{
				return false;
			}
		}
	}
	else
	{
		// weapon doesn't use ammo, don't take another if you already have it.
		if (pPlayer->HasPlayerWeapon(pWeapon->GetID()))
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
