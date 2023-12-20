//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Voting manager
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "game.h"
#include "vote_manager.h"
#include <algorithm>

int CountPlayers();


CVoteManager::CVoteManager()
{
    m_Poll = nullptr;
}


CVoteManager::~CVoteManager()
{
    ClearPoll();
}


void CVoteManager::Init()
{
    ClearPoll();

    m_LastUpdateTime = 0;

    m_LevelVoteStatus = LevelVote::NotCalled;
	m_LevelNominees.clear();
    m_LevelChangeRequests = 0;
}


void CVoteManager::Update()
{
    if (gpGlobals->time - m_LastUpdateTime < kUpdateInterval)
    {
        return;
    }

    m_LastUpdateTime = gpGlobals->time;

    if (IsPollRunning() && m_Poll->IsDone())
    {
        m_Poll->End();
        ClearPoll();
    }

    LevelVoteUpdate();
}


void CVoteManager::ClientConnected(const unsigned int playerIndex)
{

}


void CVoteManager::ClientDisconnected(const unsigned int playerIndex)
{
	if (m_LevelVoteStatus == LevelVote::NotCalled)
	{
	    m_LevelChangeRequests &= ~(1 << (playerIndex - 1));

		auto previous =
			std::find_if(m_LevelNominees.begin(), m_LevelNominees.end(), [=](const auto& candidate)
					{ return candidate.playerIndex == playerIndex; });

		if (previous != m_LevelNominees.end())
		{
			m_LevelNominees.erase(previous);
		}
	}
}


void CVoteManager::RequestLevelChange(const unsigned int playerIndex)
{
	if (m_LevelVoteStatus == LevelVote::ChangeImmediately)
	{
		return;
	}

	const auto bit = 1 << (playerIndex - 1);

	if ((m_LevelChangeRequests & bit) != 0)
	{
		return;
	}

	m_LevelChangeRequests |= bit;

	int players = std::floor(CountPlayers() * 0.5F);
	int rocked = 0;

	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		if ((m_LevelChangeRequests & (1 << i)) != 0)
		{
			rocked++;
		}
	}

    const auto name =
        STRING(util::PlayerByIndex(playerIndex)->pev->netname);

	if (rocked >= players)
	{
		util::ClientPrintAll(HUD_PRINTTALK, "#Vote_level_pass", name);
		if (m_LevelVoteStatus == LevelVote::NotCalled)
		{
			LevelVoteBegin();
		}
		m_LevelVoteStatus = LevelVote::ChangeImmediately;
	}
	else
	{
		util::ClientPrintAll(HUD_PRINTTALK, "#Vote_level_rock", name, util::dtos1(players - rocked));
	}
}


void CVoteManager::NominateLevel(
    const unsigned int playerIndex, const std::string& levelName)
{
	if (m_LevelVoteStatus != LevelVote::NotCalled)
	{
		return;
	}

	/* Check for relative paths */
	if (levelName.find_first_of('.') != std::string::npos
	 || levelName.find_first_of('/') != std::string::npos
	 || levelName.find_first_of('\\') != std::string::npos)
	{
		return;
	}

	if (g_engfuncs.pfnIsMapValid(levelName.c_str()) == 0)
	{
		return;
	}

	LevelNominee nomination =
	{
		levelName,
		playerIndex
	};

    const auto name =
        STRING(util::PlayerByIndex(playerIndex)->pev->netname);

	/* See if this player has nominated a level already & overwrite it */
	auto previous =
		std::find_if(m_LevelNominees.begin(), m_LevelNominees.end(), [=](const auto& candidate)
				{ return candidate.playerIndex == playerIndex; });
	
	if (previous != m_LevelNominees.end())
	{
		*previous = nomination;
		util::ClientPrintAll(HUD_PRINTTALK, "#Vote_level_nominate", name, levelName.c_str());
		return;
	}

	/* See if this level has already been nominated */
	if (std::find_if(m_LevelNominees.begin(), m_LevelNominees.end(), [=](const auto& candidate)
			{ return stricmp(candidate.name.c_str(), levelName.c_str()) == 0; }) == m_LevelNominees.end())
	{
		m_LevelNominees.push_back(nomination);
		util::ClientPrintAll(HUD_PRINTTALK, "#Vote_level_nominate", name, levelName.c_str());
	}
}


bool CVoteManager::ClientCommand(const unsigned int playerIndex, const char* cmd)
{
	if (!IsPollRunning() || g_pGameRules->GetState() == GR_STATE_GAME_OVER)
	{
		return false;
	}

	if (strcmp(cmd, "menuselect") == 0)
	{
		if (g_engfuncs.pfnCmd_Argc() > 1)
		{
			int option = atoi(g_engfuncs.pfnCmd_Argv(1));
			m_Poll->ClientVote(playerIndex, option);
		}
		return true;
	}

	return false;
}


bool CVoteManager::SayCommand(
    const unsigned int playerIndex, const int argc, const char** argv)
{
	if (g_pGameRules->GetState() == GR_STATE_GAME_OVER)
	{
		return false;
	}

	if (stricmp(argv[0], "rtv") == 0)
	{
		/* Classic "rock the vote" command to request a level change */
		RequestLevelChange(playerIndex);
		return true;
	}
    else if (stricmp(argv[0], "nominate") == 0)
    {
		/*! Toodles TODO: Bring up map list if no name is provided */
        if (argc > 1)
        {
            NominateLevel(playerIndex, argv[1]);
        }
        return true;
    }
	return false;
}


void CVoteManager::ClearPoll()
{
    if (IsPollRunning())
    {
        delete m_Poll;
        m_Poll = nullptr;
    }
}


void CVoteManager::LevelVoteUpdate()
{
    switch (m_LevelVoteStatus)
    {
    case LevelVote::NotCalled:
    {
		/*
		If the map time limit is set
		& only five minutes are left,
		let players vote for the next map.
		*/
        if (timelimit.value > 0.0F
         && !IsPollRunning()
         && g_pGameRules->GetMapTimeLeft() <= 300)
        {
            LevelVoteBegin();
        }
        break;
    }
    case LevelVote::Called:
    {
        break;
    }
    case LevelVote::ChangeImmediately:
    {
        if (g_pGameRules->GetState() != GR_STATE_GAME_OVER && !IsPollRunning())
        {
            g_pGameRules->EndMultiplayerGame();
        }
        break;
    }
    }
}


void CVoteManager::LevelVoteBegin()
{
    m_LevelVoteStatus = LevelVote::Called;
	m_Poll = new CLevelPoll(m_LevelNominees);
}

