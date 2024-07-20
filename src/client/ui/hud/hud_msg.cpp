/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
//  hud_msg.cpp
//

#include "hud.h"
#include "ammohistory.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"

#include "particleman.h"
extern IParticleMan* g_pParticleMan;

extern cvar_t* sensitivity;

void EV_Init();

/// USER-DEFINED SERVER MESSAGE HANDLERS

bool CHud::MsgFunc_ResetHUD(const char* pszName, int iSize, void* pbuf)
{
	// clear all hud data
	HUDLIST* pList = m_pHudList;

	while (pList)
	{
		if (pList->p != nullptr && pList->p->ShouldReset(false))
		{
			pList->p->Reset();
		}
		pList = pList->pNext;
	}

	//Reset weapon bits.
	m_iWeaponBits = 0ULL;

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	m_bTranquilized = false;
	m_flOverrideFOV = 0.0F;
	m_flOverrideSensitivity = 0.0F;

	return true;
}

void CAM_ToFirstPerson();

void CHud::MsgFunc_ViewMode(const char* pszName, int iSize, void* pbuf)
{
	CAM_ToFirstPerson();
}

void CHud::MsgFunc_InitHUD(const char* pszName, int iSize, void* pbuf)
{
	// prepare all hud data
	HUDLIST* pList = m_pHudList;

	while (pList)
	{
		if (pList->p != nullptr && pList->p->ShouldReset(true))
		{
			pList->p->Reset();
		}
		pList = pList->pNext;
	}


	//TODO: needs to be called on every map change, not just when starting a new game
	if (g_pParticleMan)
		g_pParticleMan->ResetParticles();

	EV_Init();
}


bool CHud::MsgFunc_GameMode(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	
	m_gameMode = static_cast<gamemode_e>(READ_BYTE());
	
	client::Con_DPrintf("Gamemode: ");

	switch (m_gameMode)
	{
		default:
			client::Con_DPrintf("Unknown\n");
			break;
		case kGamemodeSingleplayer:
			client::Con_DPrintf("Singleplayer\n");
			break;
		case kGamemodeCooperative:
			client::Con_DPrintf("Cooperative\n");
			break;
		case kGamemodeDeathmatch:
			client::Con_DPrintf("Deathmatch\n");
			break;
		case kGamemodeTeamplay:
			client::Con_DPrintf("Teamplay\n");
			break;
		case kGamemodeTeamFortress:
			client::Con_DPrintf("Team Fortress\n");
			break;
	}

	return true;
}


void CHud::Update_Concuss(int iConcuss)
{
	if ((0 != m_iConcussionEffect) != (0 != iConcuss))
	{
		if (0 != iConcuss)
		{
			m_StatusIcons.EnableIcon("dmg_concuss");
		}
		else
		{
			m_StatusIcons.DisableIcon("dmg_concuss");
		}
	}

	m_iConcussionEffect = iConcuss;
}


void CHud::Update_Tranquilization(const bool bTranquilized)
{
	if (m_bTranquilized != bTranquilized)
	{
		if (bTranquilized)
		{
			m_StatusIcons.EnableIcon("dmg_tranq");

			/*
				Lock values to prevent players from using key
				binds to nullify the tranquilization effects.
			*/
			m_flOverrideFOV = cl_fov->value * (70.0F / 90.0F);
			m_flOverrideSensitivity = sensitivity->value * 0.5F;
		}
		else
		{
			m_StatusIcons.DisableIcon("dmg_tranq");
		}
	}

	m_bTranquilized = bTranquilized;
}


bool CHud::MsgFunc_Concuss(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	Update_Concuss(READ_BYTE());
	return true;
}


bool CHud::MsgFunc_Weapons(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	const std::uint64_t lowerBits = READ_LONG();
	const std::uint64_t upperBits = READ_LONG();

	m_iWeaponBits = (lowerBits & 0XFFFFFFFF) | ((upperBits & 0XFFFFFFFF) << 32ULL);

	return true;
}


bool CHud::MsgFunc_Ammo(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	for (int i = 0; i < AMMO_TYPES; i++)
	{
		int count = READ_BYTE();
		gWR.SetMaxAmmo(i, count);
	}

	return true;
}
