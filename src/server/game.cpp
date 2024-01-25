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
#include "extdll.h"
#include "eiface.h"
#include "util.h"
#include "client.h"
#include "game.h"
#include "filesystem_utils.h"
#ifdef HALFLIFE_BOTS
#include "bot/hl_bot.h"
#endif
#include "vote_manager.h"

// multiplayer server rules
cvar_t teamplay = {"mp_teamplay", "0", FCVAR_SERVER};
cvar_t fraglimit = {"mp_fraglimit", "0", FCVAR_SERVER};
cvar_t timelimit = {"mp_timelimit", "0", FCVAR_SERVER};
cvar_t friendlyfire = {"mp_friendlyfire", "0", FCVAR_SERVER};
cvar_t falldamage = {"mp_falldamage", "0", FCVAR_SERVER};
cvar_t weaponstay = {"mp_weaponstay", "0", FCVAR_SERVER};
cvar_t forcerespawn = {"mp_forcerespawn", "0", FCVAR_SERVER};
cvar_t aimcrosshair = {"mp_autocrosshair", "1", FCVAR_SERVER};
cvar_t decalfrequency = {"decalfrequency", "30", FCVAR_SERVER};
cvar_t allowmonsters = {"mp_allowmonsters", "0", FCVAR_SERVER};

cvar_t allow_spectators = {"allow_spectators", "1", FCVAR_SERVER};

cvar_t mp_chattime = {"mp_chattime", "10", FCVAR_SERVER};

static bool SV_InitServer()
{
	if (!FileSystem_LoadFileSystem())
	{
		return false;
	}

	if (UTIL_IsValveGameDirectory())
	{
		g_engfuncs.pfnServerPrint("This mod has detected that it is being run from a Valve game directory which is not supported\n"
			"Run this mod from its intended location\n\nThe game will now shut down\n");
		return false;
	}

	return true;
}

// Register your console variables here
// This gets called one time when the game is initialied
void GameDLLInit()
{
	g_psv_cheats = CVAR_GET_POINTER("sv_cheats");

	if (!SV_InitServer())
	{
		g_engfuncs.pfnServerPrint("Error initializing server\n");
		//Shut the game down as soon as possible.
		SERVER_COMMAND("quit\n");
		return;
	}

	CVAR_REGISTER(&allow_spectators);

	CVAR_REGISTER(&teamplay);
	CVAR_REGISTER(&fraglimit);
	CVAR_REGISTER(&timelimit);

	CVAR_REGISTER(&friendlyfire);
	CVAR_REGISTER(&falldamage);
	CVAR_REGISTER(&weaponstay);
	CVAR_REGISTER(&forcerespawn);
	CVAR_REGISTER(&aimcrosshair);
	CVAR_REGISTER(&decalfrequency);
	CVAR_REGISTER(&allowmonsters);

	CVAR_REGISTER(&mp_chattime);

	CVoteManager::RegisterCvars();

#ifdef HALFLIFE_BOTS
	Bot_RegisterCvars();
#endif
	
#ifdef HALFLIFE_NODEGRAPH
	InitMapLoadingUtils();
#endif
}

void GameDLLShutdown()
{
	FileSystem_FreeFileSystem();
}
