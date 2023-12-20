//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Poll
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
#include "UserMessages.h"


IPoll::IPoll()
{
	m_flStartTime = gpGlobals->time;
	m_IgnorePlayers = 0;
}


IPoll::~IPoll()
{
	MessageBegin(MSG_ALL, gmsgVoteMenu);
	WriteByte(0);
	MessageEnd();
}


void IPoll::Begin(const std::string& title, const std::vector<std::string>& options)
{
	const auto numOptions = options.size();

    m_Tally.resize(numOptions, 0);

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		auto player = dynamic_cast<CBasePlayer*>(util::PlayerByIndex(i));

		if (!CanPlayerVote(player))
		{
            /* Mask them off */
			m_IgnorePlayers |= 1 << (i - 1);
			continue;
		}

        /* Show them the voting menu */
		MessageBegin(MSG_ONE, gmsgVoteMenu, player);

		WriteByte(numOptions);
		WriteByte(kDuration);

		WriteString(title.c_str());

        for (auto option = options.begin(); option != options.end(); option++)
        {
            WriteString((*option).c_str());
        }

		MessageEnd();
	}

	util::LogPrintf("Vote beginning\n");
}


void IPoll::ClientVote(unsigned int playerIndex, unsigned int option)
{
	const auto bit = 1 << (playerIndex - 1);

	if ((m_IgnorePlayers & bit) != 0)
	{
		return;
	}

	if (option < 1 || option > m_Tally.size())
	{
		return;
	}

	auto player = dynamic_cast<CBasePlayer*>(util::PlayerByIndex(playerIndex));

	if (!CanPlayerVote(player))
	{
		m_IgnorePlayers |= bit;
		return;
	}

	m_IgnorePlayers |= bit;
	m_Tally[option - 1]++;

	util::LogPrintf("\"%s<%i><%s><>\" voted for option %i\n",
		STRING(player->pev->netname),
		g_engfuncs.pfnGetPlayerUserId(player->edict()),
		g_engfuncs.pfnGetPlayerAuthId(player->edict()),
		option - 1);
}


bool IPoll::IsDone()
{
	if (g_pGameRules->GetState() == GR_STATE_GAME_OVER)
	{
		return true;
	}

	if (m_flStartTime - gpGlobals->time >= kDuration)
	{
		return true;
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if ((m_IgnorePlayers & (1 << (i - 1))) != 0)
		{
			continue;
		}

		if (!CanPlayerVote(dynamic_cast<CBasePlayer*>(util::PlayerByIndex(i))))
		{
			m_IgnorePlayers |= 1 << (i - 1);
			continue;
		}

		return false;
	}

	return true;
}


void IPoll::End()
{
    const auto numOptions = m_Tally.size();

	util::LogPrintf("Vote ended\n");

	int totalVotes = 0;
	int minVotes = -1;

	std::vector<int> candidates;
    candidates.reserve(numOptions);

	for (int i = 0; i < numOptions; i++)
	{
		totalVotes += m_Tally[i];

		if (m_Tally[i] > minVotes)
		{
            candidates.resize(1);
			candidates[0] = i;
			minVotes = m_Tally[i];
		}
		else if (m_Tally[i] == minVotes)
		{
			candidates.push_back(i);
		}
	}

	const int winner =
        candidates[g_engfuncs.pfnRandomLong(1, candidates.size()) - 1];
	
	float percent = 100.0F;

	if (totalVotes != 0)
	{
		percent = (minVotes / (float)totalVotes) * 100.0F;
	}

	util::LogPrintf("Option %i won with %i / %i (%g%%) votes\n",
		winner,
		m_Tally[winner],
		totalVotes,
		percent);

	MessageBegin(MSG_ALL, gmsgVoteMenu);
	WriteByte(0);
	MessageEnd();

    DoResults(winner, m_Tally);
}


bool IPoll::CanPlayerVote(CBasePlayer* player)
{
	return player != nullptr
		&& player->TeamNumber() != TEAM_UNASSIGNED
		&& player->TeamNumber() != TEAM_SPECTATORS
		&& player->IsBot() == false;
}

