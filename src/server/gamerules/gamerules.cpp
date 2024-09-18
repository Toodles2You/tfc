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
#include "tf_gamerules.h"
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
	m_origin{pEntity->v.origin + Vector(0, 0, 1)},
	m_angles{pEntity->v.angles},
	m_target{pEntity->v.target},
	m_master{pEntity->v.netname},
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


int CGameRules::GetMaxAmmo(CBasePlayer* pPlayer, int iAmmoType)
{
	auto iMaxAmmo = 0;

	switch (iAmmoType)
	{
		case AMMO_SHELLS:  iMaxAmmo = 100;
		case AMMO_NAILS:   iMaxAmmo = 200;
		case AMMO_ROCKETS: iMaxAmmo = 100;
		case AMMO_CELLS:   iMaxAmmo = 100;
	}

	return iMaxAmmo + pPlayer->GetUnloadedAmmo(iAmmoType);
}


bool CGameRules::CanHaveAmmo(CBasePlayer* pPlayer, int iAmmoType)
{
	return pPlayer->m_rgAmmo[iAmmoType] < GetMaxAmmo(pPlayer, iAmmoType);
}


CSpawnPoint *CGameRules::GetPlayerSpawnSpot(CBasePlayer* pPlayer)
{
	return &m_startPoint;
}


void CGameRules::AddPlayerSpawnSpot(CBaseEntity *pEntity)
{
	if (!streq(pEntity->v.classname, "info_player_start"))
	{
		return;
	}
	m_startPoint = CSpawnPoint{pEntity};
}


bool CGameRules::IsPlayerPrivileged(CBasePlayer* pPlayer)
{
	return g_psv_cheats->value != 0;
}


CGameRules* InstallGameRules()
{
	engine::ServerCommand("exec game.cfg\n");
	engine::ServerExecute();

	gpGlobals->teamplay = teamplay.value;

	return new CTeamFortress;
}

