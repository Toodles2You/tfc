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
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"

#include "particleman.h"
extern IParticleMan* g_pParticleMan;

void EV_Init();

/// USER-DEFINED SERVER MESSAGE HANDLERS

bool CHud::MsgFunc_ResetHUD(const char* pszName, int iSize, void* pbuf)
{
	// clear all hud data
	HUDLIST* pList = m_pHudList;

	while (pList)
	{
		if (pList->p)
			pList->p->Reset();
		pList = pList->pNext;
	}

	//Reset weapon bits.
	m_iWeaponBits = 0ULL;

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

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
		if (pList->p)
			pList->p->InitHUDData();
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
	
	gEngfuncs.Con_DPrintf("Gamemode: ");

	switch (m_gameMode)
	{
		default:
			gEngfuncs.Con_DPrintf("Unknown\n");
			break;
		case kGamemodeSingleplayer:
			gEngfuncs.Con_DPrintf("Singleplayer\n");
			break;
		case kGamemodeCooperative:
			gEngfuncs.Con_DPrintf("Cooperative\n");
			break;
		case kGamemodeDeathmatch:
			gEngfuncs.Con_DPrintf("Deathmatch\n");
			break;
		case kGamemodeTeamplay:
			gEngfuncs.Con_DPrintf("Teamplay\n");
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
			this->m_StatusIcons.EnableIcon("dmg_concuss");
		}
		else
		{
			this->m_StatusIcons.DisableIcon("dmg_concuss");
		}
	}

	m_iConcussionEffect = iConcuss;
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
