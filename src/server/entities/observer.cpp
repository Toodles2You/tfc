//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Functionality for the observer chase camera
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "pm_shared.h"
#include "UserMessages.h"
#include "gamerules.h"

// Find the next client in the game for this player to spectate
void CBasePlayer::Observer_FindNextPlayer(bool bReverse)
{
	// MOD AUTHORS: Modify the logic of this function if you want to restrict the observer to watching
	//				only a subset of the players. e.g. Make it check the target's team.

	int iStart;
	if (m_hObserverTarget)
		iStart = ENTINDEX(m_hObserverTarget->edict());
	else
		iStart = ENTINDEX(edict());
	int iCurrent = iStart;
	m_hObserverTarget = NULL;
	int iDir = bReverse ? -1 : 1;

	do
	{
		iCurrent += iDir;

		// Loop through the clients
		if (iCurrent > gpGlobals->maxClients)
			iCurrent = 1;
		if (iCurrent < 1)
			iCurrent = gpGlobals->maxClients;

		CBaseEntity* pEnt = util::PlayerByIndex(iCurrent);
		if (!pEnt)
			continue;
		if (pEnt == this)
			continue;
		// Don't spec observers or players who haven't picked a class yet
		if (!pEnt->IsPlayer())
			continue;

		// MOD AUTHORS: Add checks on target here.
		if (!IsSpectator()
		 && g_pGameRules->IsDeathmatch()
		 && g_pGameRules->IsTeamplay()
		 && g_pGameRules->PlayerRelationship(this, pEnt) < GR_ALLY)
			continue;

		m_hObserverTarget = pEnt;
		break;

	} while (iCurrent != iStart);

	// Did we find a target?
	if (m_hObserverTarget)
	{
		// Move to the target
		SetOrigin(m_hObserverTarget->pev->origin);

		// ALERT( at_console, "Now Tracking %s\n", STRING( m_hObserverTarget->pev->netname ) );

		// Store the target in pev so the physics DLL can get to it
		if (pev->iuser1 != OBS_ROAMING)
			pev->iuser2 = ENTINDEX(m_hObserverTarget->edict());
	}
}

// Handle buttons in observer mode
void CBasePlayer::Observer_HandleButtons()
{
	// Slow down mouse clicks
	if (m_flNextObserverInput > gpGlobals->time)
		return;

	if (pev->iuser1 == OBS_DEATHCAM || pev->iuser1 == OBS_FIXED)
		return;

	// Jump changes from modes: Chase to Roaming
	if ((m_afButtonPressed & IN_JUMP) != 0)
	{
		if (IsSpectator())
		{
			Observer_SetMode((pev->iuser1 + 1) % OBS_DEATHCAM);
		}
		else
		{
			Observer_SetMode((pev->iuser1 + 1) % OBS_ROAMING);
		}

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}

	// Attack moves to the next player
	if ((m_afButtonPressed & IN_ATTACK) != 0) //&& pev->iuser1 != OBS_ROAMING )
	{
		Observer_FindNextPlayer(false);

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}

	// Attack2 moves to the prev player
	if ((m_afButtonPressed & IN_ATTACK2) != 0) // && pev->iuser1 != OBS_ROAMING )
	{
		Observer_FindNextPlayer(true);

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}
}

void CBasePlayer::Observer_CheckTarget()
{
	if (pev->iuser1 == OBS_ROAMING || pev->iuser1 == OBS_DEATHCAM || pev->iuser1 == OBS_FIXED)
		return;

	// try to find a traget if we have no current one
	if (m_hObserverTarget == NULL)
	{
		Observer_FindNextPlayer(false);

		if (m_hObserverTarget == NULL)
		{
			// no target found at all

			int lastMode = pev->iuser1;

			Observer_SetMode(OBS_ROAMING);

			m_iObserverLastMode = lastMode; // don't overwrite users lastmode

			return; // we still have np target return
		}
	}

	CBasePlayer* target = (CBasePlayer*)(util::PlayerByIndex(ENTINDEX(m_hObserverTarget->edict())));

	if (!target)
	{
		Observer_FindNextPlayer(false);
		return;
	}

	if (!IsSpectator()
	 && g_pGameRules->IsDeathmatch()
	 && g_pGameRules->IsTeamplay()
	 && g_pGameRules->PlayerRelationship(this, target) < GR_ALLY)
	{
		Observer_FindNextPlayer(false);
		return;
	}

	// check taget
	if (!target->IsAlive())
	{
		if ((target->m_fDeadTime + 3.0f) < gpGlobals->time)
		{
			// 3 secs after death change target
			Observer_FindNextPlayer(false);
			return;
		}
	}
}

// Attempt to change the observer mode
void CBasePlayer::Observer_SetMode(int iMode)
{
	// Just abort if we're changing to the mode we're already in
	if (iMode == pev->iuser1)
		return;

	// is valid mode ?
	if (IsSpectator())
	{
		if (iMode < OBS_CHASE_FREE || iMode > OBS_MAP_CHASE)
			iMode = OBS_CHASE_FREE;
	}
	else
	{
		if (iMode != OBS_CHASE_FREE)
			iMode = OBS_CHASE_FREE;
	}
	// verify observer target again
	if (m_hObserverTarget != NULL)
	{
		CBaseEntity* pEnt = m_hObserverTarget;

		if ((pEnt == this) || (pEnt == NULL))
			m_hObserverTarget = NULL;
		else if (!pEnt->IsPlayer())
			m_hObserverTarget = NULL;
	}

	// set spectator mode
	pev->iuser1 = iMode;

	// if we are not roaming, we need a valid target to track
	if ((iMode != OBS_ROAMING) && (m_hObserverTarget == NULL))
	{
		Observer_FindNextPlayer(false);

		// if we didn't find a valid target switch to roaming
		if (m_hObserverTarget == NULL)
		{
			if (IsSpectator())
			{
				util::ClientPrint(this, HUD_PRINTCENTER, "#Spec_NoTarget");
			}
			pev->iuser1 = OBS_ROAMING;
		}
	}

	// set target if not roaming
	if (pev->iuser1 == OBS_ROAMING)
	{
		pev->iuser2 = 0;
	}
	else
	{
		pev->iuser2 = ENTINDEX(m_hObserverTarget->edict());
	}

	pev->iuser3 = 0; // clear second target from death cam

	/*! Toodles FIXME: */
#if 0
	// print spectator mode on client screen
	if (IsSpectator())
	{
		char modemsg[16];
		sprintf(modemsg, "#Spec_Mode%i", pev->iuser1);
		util::ClientPrint(this, HUD_PRINTCENTER, modemsg);
	}
#endif

	m_iObserverLastMode = iMode;
}
