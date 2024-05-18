//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Steam interface
//
// $NoKeywords: $
//=============================================================================

#pragma once

#define VERSION_SAFE_STEAM_API_INTERFACES

#include "Platform.h"
#include "steam/steam_api.h"

#ifdef VERSION_SAFE_STEAM_API_INTERFACES
extern CSteamAPIContext* g_pSteamContext;
#endif

bool Steam_LoadSteamAPI();
void Steam_FreeSteamAPI();
void Steam_Frame();
