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
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "game.h"
#include "UserMessages.h"

CHalfLifeTeamplay::CHalfLifeTeamplay()
{
	m_teams.clear();
	m_numTeams = 0;

	char teams[TEAMPLAY_TEAMLISTLENGTH];

	// Copy over the team from the server config
	strcpy(teams, teamlist.string);

	if (teamoverride.value != 0
	 && CWorld::World
	 && !FStringNull(CWorld::World->pev->team))
	{
		const char* worldTeams = STRING(CWorld::World->pev->team);
		if (strlen(worldTeams) != 0)
		{
			strcpy(teams, worldTeams);
		}
	}

	char *name = strtok(teams, ";");

	while (name != nullptr && '\0' != *name)
	{
		if (GetTeamIndex(name) == TEAM_UNASSIGNED)
		{
			m_teams.push_back(CTeam{name});
			m_numTeams++;
		}
		name = strtok(nullptr, ";");
	}
}

#include "voice_gamemgr.h"
extern CVoiceGameMgr g_VoiceGameMgr;

void CHalfLifeTeamplay::Think()
{
	///// Check game rules /////
	static int last_frags;
	static int last_time;

	int frags_remaining = 0;
	int time_remaining = 0;

	g_VoiceGameMgr.Update(gpGlobals->frametime);

	if (g_fGameOver) // someone else quit the game already
	{
		CHalfLifeMultiplay::Think();
		return;
	}

	float flTimeLimit = timelimit.value * 60;

	time_remaining = (int)(0 != flTimeLimit ? (flTimeLimit - gpGlobals->time) : 0);

	if (flTimeLimit != 0 && gpGlobals->time >= flTimeLimit)
	{
		GoToIntermission();
		return;
	}

	float flFragLimit = fraglimit.value;
	if (0 != flFragLimit)
	{
		int bestfrags = 9999;
		int remain;

		// check if any team is over the frag limit
		for (auto t = m_teams.begin(); t != m_teams.end(); t++)
		{
			if ((*t).m_score >= flFragLimit)
			{
				GoToIntermission();
				return;
			}

			remain = flFragLimit - (*t).m_score;
			if (remain < bestfrags)
			{
				bestfrags = remain;
			}
		}
		frags_remaining = bestfrags;
	}

	last_frags = frags_remaining;
	last_time = time_remaining;
}

//=========================================================
// ClientCommand
// the user has typed a command which is unrecognized by everything else;
// this check to see if the gamerules knows anything about the command
//=========================================================
bool CHalfLifeTeamplay::ClientCommand(CBasePlayer* pPlayer, const char* pcmd)
{
	if (g_VoiceGameMgr.ClientCommand(pPlayer, pcmd))
		return true;

	if (FStrEq(pcmd, "menuselect"))
	{
		if (CMD_ARGC() < 2)
			return true;

		int slot = atoi(CMD_ARGV(1));

		// select the item from the current menu

		return true;
	}

	return CHalfLifeMultiplay::ClientCommand(pPlayer, pcmd);
}


void CHalfLifeTeamplay::SetDefaultPlayerTeam(CBasePlayer* pPlayer)
{
	// copy out the team name from the model
	char* mdls = g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model");

	int i = GetTeamIndex(mdls);

	// update the current player of the team he is joining
	if (mdls[0] == '\0' || !IsValidTeam(mdls) || defaultteam.value != 0)
	{
		if (defaultteam.value != 0)
		{
			i = TEAM_DEFAULT;
		}
		else
		{
			i = TeamWithFewestPlayers();
		}
	}

	pPlayer->pev->team = i;
	m_teams[i - 1].AddPlayer(pPlayer);

	pPlayer->Spawn();

	int clientIndex = pPlayer->entindex();
	g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model", pPlayer->TeamID());
	g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "team", pPlayer->TeamID());
}


//=========================================================
// InitHUD
//=========================================================
void CHalfLifeTeamplay::InitHUD(CBasePlayer* pPlayer)
{
	int i;

	CHalfLifeMultiplay::InitHUD(pPlayer);

	// Send down the team names
	MESSAGE_BEGIN(MSG_ONE, gmsgTeamNames, NULL, pPlayer->edict());
	WRITE_BYTE(m_numTeams);
	for (auto t = m_teams.begin(); t != m_teams.end(); t++)
	{
		WRITE_STRING((*t).m_name.c_str());
	}
	MESSAGE_END();

	for (auto t = m_teams.begin(); t != m_teams.end(); t++)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgTeamScore, nullptr, pPlayer->edict());
		WRITE_STRING((*t).m_name.c_str());
		WRITE_SHORT((*t).m_score);
		WRITE_SHORT(0);
		MESSAGE_END();
	}

	int clientIndex = pPlayer->entindex();
	// update this player with all the other players team info
	// loop through all active players and send their team info to the new client
	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* plr = UTIL_PlayerByIndex(i);
		if (plr && IsValidTeam(plr->TeamID()))
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgTeamInfo, NULL, pPlayer->edict());
			WRITE_BYTE(plr->entindex());
			WRITE_STRING(plr->TeamID());
			MESSAGE_END();
		}
	}
}


void CHalfLifeTeamplay::ChangePlayerTeam(CBasePlayer* pPlayer, const char* pTeamName, bool bKill, bool bGib)
{
	CHalfLifeMultiplay::ChangePlayerTeam(pPlayer, pTeamName, bKill, bGib);

	int clientIndex = pPlayer->entindex();
	g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model", pPlayer->TeamID());
	g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "team", pPlayer->TeamID());
}


//=========================================================
// ClientUserInfoChanged
//=========================================================
void CHalfLifeTeamplay::ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer)
{
	if (pPlayer->TeamNumber() == TEAM_UNASSIGNED)
	{
		return;
	}

	char text[1024];

	// prevent skin/color/model changes
	char* mdls = g_engfuncs.pfnInfoKeyValue(infobuffer, "model");

	if (!stricmp(mdls, pPlayer->TeamID()))
	{
		return;
	}

	if (0 != defaultteam.value)
	{
		int clientIndex = pPlayer->entindex();

		g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model", pPlayer->TeamID());
		g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "team", pPlayer->TeamID());
		sprintf(text, "* Not allowed to change teams in this game!\n");
		UTIL_SayText(text, pPlayer);
		return;
	}

	if (!IsValidTeam(mdls))
	{
		int clientIndex = pPlayer->entindex();

		g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model", pPlayer->TeamID());
		g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "team", pPlayer->TeamID());
		sprintf(text, "* Can't change team to \'%s\'\n", mdls);
		UTIL_SayText(text, pPlayer);
		return;
	}
	// notify everyone of the team change
	sprintf(text, "* %s has changed to team \'%s\'\n", STRING(pPlayer->pev->netname), mdls);
	UTIL_SayTextAll(text, pPlayer);

	UTIL_LogPrintf("\"%s<%i><%s><%s>\" joined team \"%s\"\n",
		STRING(pPlayer->pev->netname),
		GETPLAYERUSERID(pPlayer->edict()),
		GETPLAYERAUTHID(pPlayer->edict()),
		pPlayer->TeamID(),
		mdls);

	ChangePlayerTeam(pPlayer, mdls, true, true);
}


//=========================================================
// IsTeamplay
//=========================================================
bool CHalfLifeTeamplay::IsTeamplay()
{
	return true;
}

bool CHalfLifeTeamplay::FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker)
{
	if (pAttacker && PlayerRelationship(pPlayer, pAttacker) == GR_TEAMMATE)
	{
		// my teammate hit me.
		if ((friendlyfire.value == 0) && (pAttacker != pPlayer))
		{
			// friendly fire is off, and this hit came from someone other than myself,  then don't get hurt
			return false;
		}
	}

	return CHalfLifeMultiplay::FPlayerCanTakeDamage(pPlayer, pAttacker);
}

//=========================================================
//=========================================================
int CHalfLifeTeamplay::PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget)
{
	// half life multiplay has a simple concept of Player Relationships.
	// you are either on another player's team, or you are not.
	if (!pPlayer || !pTarget || !pTarget->IsPlayer())
	{
		return GR_NOTTEAMMATE;
	}

	if (pPlayer->TeamNumber() == pTarget->TeamNumber())
	{
		return GR_TEAMMATE;
	}

	return GR_NOTTEAMMATE;
}

//=========================================================
//=========================================================
bool CHalfLifeTeamplay::ShouldAutoAim(CBasePlayer* pPlayer, edict_t* target)
{
	// always autoaim, unless target is a teammate
	CBaseEntity* pTgt = CBaseEntity::Instance(target);
	if (pTgt && pTgt->IsPlayer())
	{
		if (PlayerRelationship(pPlayer, pTgt) >= GR_ALLY)
			return false; // don't autoaim at teammates
	}

	return CHalfLifeMultiplay::ShouldAutoAim(pPlayer, target);
}

//=========================================================
//=========================================================
int CHalfLifeTeamplay::IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled)
{
	if (pAttacker != pKilled && PlayerRelationship(pAttacker, pKilled) >= GR_ALLY)
	{
		return -1;
	}
	return CHalfLifeMultiplay::IPointsForKill(pAttacker, pKilled);
}

int CHalfLifeTeamplay::TeamWithFewestPlayers()
{
	int minPlayers = gpGlobals->maxClients + 1;
	int team = 0;

	for (int i = 0; i < m_numTeams; i++)
	{
		if (minPlayers > m_teams[i].m_numPlayers)
		{
			minPlayers = m_teams[i].m_numPlayers;
			team = i + 1;
		}
	}

	if (team == 0)
	{
		team = g_engfuncs.pfnRandomLong(1, m_numTeams);
	}

	return team;
}
