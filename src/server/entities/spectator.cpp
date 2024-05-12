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

// Spectator functions
//
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "spectator.h"

/*
===========
SpectatorConnect

called when a spectator connects to a server
============
*/
void CBaseSpectator::SpectatorConnect()
{
	v.flags = FL_SPECTATOR;
	v.solid = SOLID_NOT;
	v.movetype = MOVETYPE_NOCLIP;

	m_pGoalEnt = NULL;
}

/*
===========
SpectatorDisconnect

called when a spectator disconnects from a server
============
*/
void CBaseSpectator::SpectatorDisconnect()
{
}

/*
================
SpectatorImpulseCommand

Called by SpectatorThink if the spectator entered an impulse
================
*/
void CBaseSpectator::SpectatorImpulseCommand()
{
	static CBaseEntity* pGoal = NULL;
	CBaseEntity* pPreviousGoal;
	CBaseEntity* pCurrentGoal;
	bool bFound;

	switch (v.impulse)
	{
	case 1:
		// teleport the spectator to the next spawn point
		// note that if the spectator is tracking, this doesn't do
		// much
		pPreviousGoal = pGoal;
		pCurrentGoal = pGoal;
		// Start at the current goal, skip the world, and stop if we looped
		//  back around

		bFound = false;
		while (true)
		{
			pCurrentGoal = util::FindEntityByClassname(pCurrentGoal, "info_player_deathmatch");
			// Looped around, failure
			if (pCurrentGoal == pPreviousGoal)
			{
				ALERT(at_console, "Could not find a spawn spot.\n");
				break;
			}
			// Found a non-world entity, set success, otherwise, look for the next one.
			if (pCurrentGoal != nullptr)
			{
				bFound = true;
				break;
			}
		}

		if (!bFound) // Didn't find a good spot.
			break;

		pGoal = pCurrentGoal;
		SetOrigin(pGoal->v.origin);
		v.angles = pGoal->v.angles;
		v.fixangle = 0;
		break;
	default:
		ALERT(at_console, "Unknown spectator impulse\n");
		break;
	}

	v.impulse = 0;
}

/*
================
SpectatorThink

Called every frame after physics are run
================
*/
void CBaseSpectator::SpectatorThink()
{
	if ((v.flags & FL_SPECTATOR) == 0)
	{
		v.flags = FL_SPECTATOR;
	}

	v.solid = SOLID_NOT;
	v.movetype = MOVETYPE_NOCLIP;

	if (0 != v.impulse)
		SpectatorImpulseCommand();
}

/*
===========
Spawn

  Called when spectator is initialized:
  UNDONE:  Is this actually being called because spectators are not allocated in normal fashion?
============
*/
bool CBaseSpectator::Spawn()
{
	v.flags = FL_SPECTATOR;
	v.solid = SOLID_NOT;
	v.movetype = MOVETYPE_NOCLIP;

	m_pGoalEnt = NULL;

	return true;
}
