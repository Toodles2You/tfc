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

#pragma once

void GameDLLInit();
void GameDLLShutdown();


// multiplayer server rules
extern cvar_t teamplay;
extern cvar_t fraglimit;
extern cvar_t timelimit;
extern cvar_t friendlyfire;
extern cvar_t falldamage;
extern cvar_t weaponstay;
extern cvar_t forcerespawn;
extern cvar_t aimcrosshair;
extern cvar_t decalfrequency;
extern cvar_t allowmonsters;
extern cvar_t allow_spectators;
extern cvar_t mp_chattime;

// Engine Cvars
inline cvar_t* g_psv_cheats;
inline cvar_t* mp_consistency;

inline bool g_bDeveloperMode;
