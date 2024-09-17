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
#include "screenfade.h"
#include "shake.h"

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
	m_flFlashTime = 0.0F;

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


bool CHud::MsgFunc_Flash(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	const auto full = READ_BYTE() != 0;
	const auto time = READ_BYTE() / 255.0F;

	screenfade_t sf;
	client::GetScreenFade(&sf);

	if (time == 0.0F)
	{
		sf.fadeEnd = client::GetClientTime();

		client::SetScreenFade(&sf);

		m_StatusIcons.DisableIcon("dmg_haluc");
		m_flFlashTime = sf.fadeEnd;

		return;
	}

	/* Prevent this from overriding previous flashes. */

	const auto prevFadeEnd = sf.fadeEnd;
	const auto prevFadeReset = sf.fadeReset;

	sf.fader = 0;
	sf.fadeg = 0;
	sf.fadeb = 0;
	sf.fadealpha = 255;

	if (full)
	{
		sf.fadeEnd = 9.0F;
		sf.fadeReset = 2.0F;
	}
	else
	{
		/* Make sure the flash is never too abrupt. */

		sf.fadeEnd = std::max(3.0F * time, 1.0F);
		sf.fadeReset = time;
	}

	sf.fadeSpeed = 0.0F;

	sf.fadeFlags = FFADE_IN;

	if (sf.fadeEnd != 0.0F)
	{
		sf.fadeSpeed = (float)sf.fadealpha / sf.fadeEnd;
	}

	sf.fadeReset += client::GetClientTime();
	sf.fadeEnd += sf.fadeReset;

	sf.fadeEnd = std::max(sf.fadeEnd, prevFadeEnd);
	sf.fadeReset = std::max(sf.fadeReset, prevFadeReset);

	client::SetScreenFade(&sf);

	m_StatusIcons.EnableIcon("dmg_haluc");
	m_flFlashTime = sf.fadeEnd;

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
