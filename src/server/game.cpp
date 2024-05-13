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
cvar_t teamlist = {"mp_teamlist", "hgrunt;scientist", FCVAR_SERVER};
cvar_t teamoverride = {"mp_teamoverride", "1"};
cvar_t defaultteam = {"mp_defaultteam", "0"};
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
	g_psv_cheats = g_engfuncs.pfnCVarGetPointer("sv_cheats");
	mp_consistency = g_engfuncs.pfnCVarGetPointer("mp_consistency");

	if (!SV_InitServer())
	{
		g_engfuncs.pfnServerPrint("Error initializing server\n");
		//Shut the game down as soon as possible.
		g_engfuncs.pfnServerCommand("quit\n");
		return;
	}

	g_engfuncs.pfnCVarRegister(&allow_spectators);

	g_engfuncs.pfnCVarRegister(&teamplay);
	g_engfuncs.pfnCVarRegister(&fraglimit);
	g_engfuncs.pfnCVarRegister(&timelimit);

	g_engfuncs.pfnCVarRegister(&friendlyfire);
	g_engfuncs.pfnCVarRegister(&falldamage);
	g_engfuncs.pfnCVarRegister(&weaponstay);
	g_engfuncs.pfnCVarRegister(&forcerespawn);
	g_engfuncs.pfnCVarRegister(&aimcrosshair);
	g_engfuncs.pfnCVarRegister(&decalfrequency);
	g_engfuncs.pfnCVarRegister(&teamlist);
	g_engfuncs.pfnCVarRegister(&teamoverride);
	g_engfuncs.pfnCVarRegister(&defaultteam);
	g_engfuncs.pfnCVarRegister(&allowmonsters);

	g_engfuncs.pfnCVarRegister(&mp_chattime);

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
