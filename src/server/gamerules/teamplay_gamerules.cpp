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

			if (m_numTeams == TEAM_SPECTATORS - 1)
			{
				break;
			}
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


int CHalfLifeTeamplay::GetDefaultPlayerTeam(CBasePlayer* pPlayer)
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
			i = CHalfLifeMultiplay::GetDefaultPlayerTeam(pPlayer);
		}
	}

	if (i == TEAM_UNASSIGNED)
	{
		i = TEAM_DEFAULT;
	}

	return i;
}


//=========================================================
// InitHUD
//=========================================================
void CHalfLifeTeamplay::InitHUD(CBasePlayer* pPlayer)
{
	int i;

	CHalfLifeMultiplay::InitHUD(pPlayer);

	for (auto t = m_teams.begin(); t != m_teams.end(); t++)
	{
		MessageBegin(MSG_ONE, gmsgTeamScore, pPlayer);
		WriteString((*t).m_name.c_str());
		WriteShort((*t).m_score);
		WriteShort(0);
		MessageEnd();
	}

	int clientIndex = pPlayer->entindex();
	// update this player with all the other players team info
	// loop through all active players and send their team info to the new client
	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* plr = util::PlayerByIndex(i);
		if (plr && IsValidTeam(plr->TeamID()))
		{
			MessageBegin(MSG_ONE, gmsgTeamInfo, pPlayer);
			WriteByte(plr->entindex());
			WriteString(plr->TeamID());
			MessageEnd();
		}
	}
}


bool CHalfLifeTeamplay::ChangePlayerTeam(CBasePlayer* pPlayer, const char* pTeamName, bool bKill, bool bGib)
{
	if (!CHalfLifeMultiplay::ChangePlayerTeam(pPlayer, pTeamName, bKill, bGib))
	{
		return false;
	}

	if (pPlayer->TeamNumber() != TEAM_SPECTATORS)
	{
		int clientIndex = pPlayer->entindex();
		g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model", pPlayer->TeamID());
		g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "team", pPlayer->TeamID());
	}

	return true;
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
		util::SayText(text, pPlayer);
		return;
	}

	if (!IsValidTeam(mdls))
	{
		int clientIndex = pPlayer->entindex();

		g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model", pPlayer->TeamID());
		g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "team", pPlayer->TeamID());
		sprintf(text, "* Can't change team to \'%s\'\n", mdls);
		util::SayText(text, pPlayer);
		return;
	}
	// notify everyone of the team change
	sprintf(text, "* %s has changed to team \'%s\'\n", STRING(pPlayer->pev->netname), mdls);
	util::SayTextAll(text, pPlayer);

	util::LogPrintf("\"%s<%i><%s><%s>\" joined team \"%s\"\n",
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
