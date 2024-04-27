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
			m_numTeams++;
			m_teams.push_back(CTeam{m_numTeams, name});

			if (m_numTeams == TEAM_SPECTATORS - 1)
			{
				break;
			}
		}
		name = strtok(nullptr, ";");
	}
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


void CHalfLifeTeamplay::InitHUD(CBasePlayer* pPlayer)
{
	CHalfLifeMultiplay::InitHUD(pPlayer);

	for (auto t = m_teams.begin(); t != m_teams.end(); t++)
	{
		MessageBegin(MSG_ONE, gmsgTeamScore, pPlayer);
		WriteByte((*t).m_index);
		WriteShort((*t).m_score);
		MessageEnd();
	}
}


bool CHalfLifeTeamplay::ChangePlayerTeam(CBasePlayer* pPlayer, int teamIndex, bool bKill, bool bGib, bool bAutoTeam)
{
	if (!CHalfLifeMultiplay::ChangePlayerTeam(pPlayer, teamIndex, bKill, bGib, bAutoTeam))
	{
		return false;
	}

	if (pPlayer->TeamNumber() != TEAM_SPECTATORS)
	{
		g_engfuncs.pfnSetClientKeyValue(
			pPlayer->entindex(),
			g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()),
			"model",
			pPlayer->TeamID());
	}

	return true;
}


void CHalfLifeTeamplay::ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer)
{
	CHalfLifeMultiplay::ClientUserInfoChanged(pPlayer, infobuffer);

	if (pPlayer->TeamNumber() != TEAM_SPECTATORS)
	{
		g_engfuncs.pfnSetClientKeyValue(
			pPlayer->entindex(),
			infobuffer,
			"model",
			pPlayer->TeamID());
	}
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


int CHalfLifeTeamplay::PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget)
{
	// half life multiplay has a simple concept of Player Relationships.
	// you are either on another player's team, or you are not.
	if (!pPlayer || !pTarget || !pTarget->IsClient())
	{
		return GR_NOTTEAMMATE;
	}

	if (pPlayer->TeamNumber() == pTarget->TeamNumber())
	{
		return GR_TEAMMATE;
	}

	return GR_NOTTEAMMATE;
}


float CHalfLifeTeamplay::GetPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled, bool assist)
{
	if (pAttacker != pKilled && PlayerRelationship(pAttacker, pKilled) >= GR_ALLY)
	{
		if (assist)
		{
			return 0;
		}
		return -1;
	}
	return CHalfLifeMultiplay::GetPointsForKill(pAttacker, pKilled, assist);
}


void CHalfLifeTeamplay::Enter_RND_RUNNING()
{
}


void CHalfLifeTeamplay::Think_RND_RUNNING()
{
	if ((int)fraglimit.value > 0)
	{
		for (auto t = m_teams.begin(); t != m_teams.end(); t++)
		{
			if ((*t).m_score >= (int)fraglimit.value)
			{
				EnterState(GR_STATE_GAME_OVER);
				return;
			}
		}
	}

	CheckTimeLimit();
}


void CHalfLifeTeamplay::SendMenusToClient(CBasePlayer* player)
{
	MessageBegin(MSG_ONE, gmsgVGUIMenu, player);
	WriteByte(MENU_TEAM);
	MessageEnd();
}

void CHalfLifeTeamplay::AddPointsToPlayer(CBasePlayer* player, float score, bool allowNegative)
{
	CHalfLifeMultiplay::AddPointsToPlayer(player, score, allowNegative);

	if (score > 0)
	{
		AddPointsToTeam(player->TeamNumber(), score);
	}
}

