//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "gamerules.h"
#include "player.h"
#include "client.h"
#include "perf_counter.h"

#include "bot.h"
#include "bot_manager.h"
#include "nav_area.h"
#include "bot_util.h"
#include "bot_profile.h"

#include "hl_bot.h"
#include "hl_bot_manager.h"

#include <string>


cvar_t cv_bot_traceview					= {"cv_bot_traceview",				"0",			FCVAR_SERVER};
cvar_t cv_bot_stop						= {"cv_bot_stop",					"0",			FCVAR_SERVER};
cvar_t cv_bot_show_nav					= {"cv_bot_show_nav",				"0",			FCVAR_SERVER};
cvar_t cv_bot_show_danger				= {"cv_bot_show_danger",			"0",			FCVAR_SERVER};
cvar_t cv_bot_nav_edit					= {"cv_bot_nav_edit",				"0",			FCVAR_SERVER};
cvar_t cv_bot_nav_zdraw					= {"cv_bot_nav_zdraw",				"0",			FCVAR_SERVER};
cvar_t cv_bot_walk						= {"cv_bot_walk",					"0",			FCVAR_SERVER};
cvar_t cv_bot_difficulty				= {"cv_bot_difficulty",				"1",			FCVAR_SERVER};
cvar_t cv_bot_debug						= {"cv_bot_debug",					"0",			FCVAR_SERVER};
cvar_t cv_bot_quicksave					= {"cv_bot_quicksave",				"0",			FCVAR_SERVER};
cvar_t cv_bot_quota						= {"cv_bot_quota",					"0",			FCVAR_SERVER};
cvar_t cv_bot_quota_match				= {"cv_bot_quota_match",			"0",			FCVAR_SERVER};
cvar_t cv_bot_prefix					= {"cv_bot_prefix",					"",				FCVAR_SERVER};
cvar_t cv_bot_join_team					= {"cv_bot_join_team",				"0",			FCVAR_SERVER};
cvar_t cv_bot_join_after_player			= {"cv_bot_join_after_player",		"0",			FCVAR_SERVER};
cvar_t cv_bot_auto_vacate				= {"cv_bot_auto_vacate",			"0",			FCVAR_SERVER};
cvar_t cv_bot_zombie					= {"cv_bot_zombie",					"0",			FCVAR_SERVER};
cvar_t cv_bot_defer_to_human			= {"cv_bot_defer_to_human",			"0",			FCVAR_SERVER};
cvar_t cv_bot_chatter					= {"cv_bot_chatter",				"0",			FCVAR_SERVER};
cvar_t cv_bot_profile_db				= {"cv_bot_profile_db",				"BotProfile.db",FCVAR_SERVER};


CHLBotManager::CHLBotManager()
{
}


void CHLBotManager::ClientDisconnect(CBasePlayer *pPlayer)
{
	m_NextQuotaCheckTime = gpGlobals->time;
}


void CHLBotManager::AddBot(const char *profileName)
{
	const BotProfile *profile = nullptr;
	if (profileName)
	{
		profile = TheBotProfiles->GetProfile(profileName, BOT_TEAM_ANY);
	}
	if (!profile)
	{
		profile = TheBotProfiles->GetRandomProfile((BotDifficultyType)cv_bot_difficulty.value, BOT_TEAM_ANY);
	}
	char szRejectReason[128];
	auto bot = CreateBot<CHLBot>(profile);
	if (bot != nullptr)
	{
		if (ClientConnect(&bot->v, STRING(bot->v.netname), "127.0.0.1", szRejectReason))
		{
			ClientPutInServer(&bot->v);
		}
	}
}


bool CHLBotManager::ClientCommand(CBasePlayer *pPlayer, const char *pcmd)
{
	if (streq(pcmd, "bot"))
	{
		AddBot(g_engfuncs.pfnCmd_Argc() > 1 ? g_engfuncs.pfnCmd_Argv(1) : nullptr);
		return true;
	}
	return false;
}


void CHLBotManager::StartFrame()
{
	if (m_NextQuotaCheckTime <= gpGlobals->time)
	{
		auto humans = UTIL_HumansInGame();
		auto quota = static_cast<int>(cv_bot_quota.value);
		if (cv_bot_quota_match.value > 0.0f || humans == 0)
		{
			quota = humans;
		}
		quota = std::min(quota, gpGlobals->maxClients - humans);
		auto bots = UTIL_BotsInGame();

		while (bots < quota)
		{
			AddBot(nullptr);
			bots++;
		}

		while (bots > quota)
		{
			UTIL_KickBotFromTeam(-1);
			bots--;
		}

		m_NextQuotaCheckTime = gpGlobals->time + 1.0f;
	}

	CBotManager::StartFrame();
}


void CHLBotManager::ServerActivate()
{
	TheBotProfiles = new BotProfileManager();
	TheBotProfiles->Init(cv_bot_profile_db.string);

	m_NextQuotaCheckTime = gpGlobals->time + 3.0f;
}


void CHLBotManager::ServerDeactivate()
{
	int i;
	CBasePlayer *player;
	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		player = static_cast<CBasePlayer *>(util::PlayerByIndex(i));
		if (!IsEntityValid(player))
		{
			continue;
		}
		if (!player->IsBot())
		{
			continue;
		}
		player->v.takedamage = DAMAGE_NO;
		player->v.solid = SOLID_NOT;
	}
	delete TheBotProfiles;
	TheBotProfiles = nullptr;
}


void CHLBotManager::ServerCommand(const char *pcmd)
{
}


void CHLBotManager::AddServerCommand(const char *cmd)
{
}


void CHLBotManager::AddServerCommands()
{
}


unsigned int CHLBotManager::GetPlayerPriority(CBasePlayer *player) const
{
	return 0;
}


void InstallBotControl()
{

}


void Bot_ServerCommand()
{

}


void Bot_RegisterCvars()
{
	CVAR_REGISTER(&cv_bot_traceview);
	CVAR_REGISTER(&cv_bot_stop);
	CVAR_REGISTER(&cv_bot_show_nav);
	CVAR_REGISTER(&cv_bot_show_danger);
	CVAR_REGISTER(&cv_bot_nav_edit);
	CVAR_REGISTER(&cv_bot_nav_zdraw);
	CVAR_REGISTER(&cv_bot_walk);
	CVAR_REGISTER(&cv_bot_difficulty);
	CVAR_REGISTER(&cv_bot_debug);
	CVAR_REGISTER(&cv_bot_quicksave);
	CVAR_REGISTER(&cv_bot_quota);
	CVAR_REGISTER(&cv_bot_quota_match);
	CVAR_REGISTER(&cv_bot_prefix);
	CVAR_REGISTER(&cv_bot_join_team);
	CVAR_REGISTER(&cv_bot_join_after_player);
	CVAR_REGISTER(&cv_bot_auto_vacate);
	CVAR_REGISTER(&cv_bot_zombie);
	CVAR_REGISTER(&cv_bot_defer_to_human);
	CVAR_REGISTER(&cv_bot_chatter);
	CVAR_REGISTER(&cv_bot_profile_db);
}
