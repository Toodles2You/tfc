//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Steam interface
//
// $NoKeywords: $
//=============================================================================

#include "steam_utils.h"

#ifdef VERSION_SAFE_STEAM_API_INTERFACES
CSteamAPIContext* g_pSteamContext;
#else
static bool bSteamInitialized = false;
#endif

static bool Steam_HasSteamAPI()
{
#ifdef VERSION_SAFE_STEAM_API_INTERFACES
    return g_pSteamContext != nullptr;
#else
    return bSteamInitialized;
#endif
}

bool Steam_LoadSteamAPI()
{
    if (Steam_HasSteamAPI())
    {
        return true;
    }

#ifdef VERSION_SAFE_STEAM_API_INTERFACES
    if (!SteamAPI_InitSafe())
    {
        return false;
    }

    g_pSteamContext = new CSteamAPIContext();

    if (g_pSteamContext == nullptr || !g_pSteamContext->Init())
    {
        return false;
    }
#else
    if (!SteamAPI_Init())
    {
        return false;
    }

    bSteamInitialized = true;
#endif

	return true;
}

void Steam_FreeSteamAPI()
{
    if (!Steam_HasSteamAPI())
    {
        return;
    }

#ifdef VERSION_SAFE_STEAM_API_INTERFACES
    g_pSteamContext->Clear();

    delete g_pSteamContext;
    g_pSteamContext = nullptr;
#else
    bSteamInitialized = false;
#endif

    SteamAPI_Shutdown();
}

void Steam_Frame()
{
    if (Steam_HasSteamAPI())
    {
        SteamAPI_RunCallbacks();
    }
}
