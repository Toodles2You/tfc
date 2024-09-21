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

/*

===== client.cpp ========================================================

  client/server game specific stuff

*/

#include <algorithm>
#include <string>
#include <vector>

#include "extdll.h"
#include "util.h"
#include "filesystem_utils.h"
#include "cbase.h"
#include "com_model.h"
#include "saverestore.h"
#include "player.h"
#include "spectator.h"
#include "client.h"
#include "gamerules.h"
#include "game.h"
#include "customentity.h"
#include "weapons.h"
#include "weaponinfo.h"
#include "usercmd.h"
#include "netadr.h"
#include "pm_shared.h"
#include "pm_defs.h"
#include "UserMessages.h"
#ifdef HALFLIFE_BOTS
#include "bot/hl_bot_manager.h"
#endif
#include "steam_utils.h"

void LinkUserMessages();

extern unsigned short g_usGibbed;
extern unsigned short g_usTeleport;
extern unsigned short g_usExplosion;
extern unsigned short g_usGetNailedIdiot;
extern unsigned short g_usTrail;


/*
===========
ClientConnect

called when a player connects to a server
============
*/
qboolean ClientConnect(Entity* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128])
{
	return static_cast<qboolean>(g_pGameRules->ClientConnected(pEntity, pszName, pszAddress, szRejectReason));
}


/*
===========
ClientDisconnect

called when a player disconnects from a server
============
*/
void ClientDisconnect(Entity* pEntity)
{
	auto player = pEntity->Get<CBasePlayer>();

	if (player != nullptr)
	{
		/* Since the edict doesn't get deleted, fix it so it doesn't interfere. */
		player->v.solid = SOLID_NOT;
		player->v.movetype = MOVETYPE_NONE;
		player->v.takedamage = DAMAGE_NO;

		player->InstallGameMovement(nullptr);

#ifdef HALFLIFE_TANKCONTROL
		if (player->m_pTank != nullptr)
		{
			player->m_pTank->Use(player, player, USE_OFF, 0);
			player->m_pTank = nullptr;
		}
#endif

		player->SetUseObject(nullptr);

		player->m_hTeleporter = nullptr;
	}

	g_pGameRules->ClientDisconnected(pEntity);

	if (player != nullptr)
	{
#ifdef HALFLIFE_BOTS
		if (g_pBotMan != nullptr)
		{
			g_pBotMan->ClientDisconnect(player);
		}
#endif

		player->SetOrigin(g_vecZero);

		engine::FreeEntPrivateData(pEntity);
	}
}


/*
============
ClientKill

Player entered the suicide command
============
*/
void ClientKill(Entity* pEntity)
{
	auto pl = pEntity->Get<CBasePlayer>();

	if (!pl->IsPlayer() || !pl->IsAlive())
	{
		return;
	}

	if (!g_pGameRules->FPlayerCanSuicide(pl))
	{
		return;
	}

	pl->Killed(
		CWorld::World,
		CWorld::World,
		DMG_GENERIC);
}

/*
===========
ClientPutInServer

called each time a player is spawned
============
*/
void ClientPutInServer(Entity* pEntity)
{
	CBasePlayer* pPlayer;

	/* Allocate a player class */
	if ((pEntity->flags & FL_FAKECLIENT) == 0)
	{
		engine::FreeEntPrivateData(pEntity);
		pPlayer = pEntity->GetNew<CBasePlayer>();
	}
	else
	{
		pPlayer = pEntity->Get<CBasePlayer>();
	}

	pPlayer->InstallGameMovement(new CHalfLifeMovement{pmove, pPlayer});

	g_pGameRules->PlayerSpawn(pPlayer);

	if (util::IsMultiplayer())
	{
		util::FireTargets("game_playerjoin", pPlayer, pPlayer, USE_TOGGLE, 0);
	}
}

#include "voice_gamemgr.h"
extern CVoiceGameMgr g_VoiceGameMgr;



#if defined(_MSC_VER) || defined(WIN32)
typedef wchar_t uchar16;
typedef unsigned int uchar32;
#else
typedef unsigned short uchar16;
typedef wchar_t uchar32;
#endif

//-----------------------------------------------------------------------------
// Purpose: determine if a uchar32 represents a valid Unicode code point
//-----------------------------------------------------------------------------
bool Q_IsValidUChar32(uchar32 uVal)
{
	// Values > 0x10FFFF are explicitly invalid; ditto for UTF-16 surrogate halves,
	// values ending in FFFE or FFFF, or values in the 0x00FDD0-0x00FDEF reserved range
	return (uVal < 0x110000u) && ((uVal - 0x00D800u) > 0x7FFu) && ((uVal & 0xFFFFu) < 0xFFFEu) && ((uVal - 0x00FDD0u) > 0x1Fu);
}


// Decode one character from a UTF-8 encoded string. Treats 6-byte CESU-8 sequences
// as a single character, as if they were a correctly-encoded 4-byte UTF-8 sequence.
int Q_UTF8ToUChar32(const char* pUTF8_, uchar32& uValueOut, bool& bErrorOut)
{
	const uint8* pUTF8 = (const uint8*)pUTF8_;

	int nBytes = 1;
	uint32 uValue = pUTF8[0];
	uint32 uMinValue = 0;

	// 0....... single byte
	if (uValue < 0x80)
		goto decodeFinishedNoCheck;

	// Expecting at least a two-byte sequence with 0xC0 <= first <= 0xF7 (110...... and 11110...)
	if ((uValue - 0xC0u) > 0x37u || (pUTF8[1] & 0xC0) != 0x80)
		goto decodeError;

	uValue = (uValue << 6) - (0xC0 << 6) + pUTF8[1] - 0x80;
	nBytes = 2;
	uMinValue = 0x80;

	// 110..... two-byte lead byte
	if ((uValue & (0x20 << 6)) == 0)
		goto decodeFinished;

	// Expecting at least a three-byte sequence
	if ((pUTF8[2] & 0xC0) != 0x80)
		goto decodeError;

	uValue = (uValue << 6) - (0x20 << 12) + pUTF8[2] - 0x80;
	nBytes = 3;
	uMinValue = 0x800;

	// 1110.... three-byte lead byte
	if ((uValue & (0x10 << 12)) == 0)
		goto decodeFinishedMaybeCESU8;

	// Expecting a four-byte sequence, longest permissible in UTF-8
	if ((pUTF8[3] & 0xC0) != 0x80)
		goto decodeError;

	uValue = (uValue << 6) - (0x10 << 18) + pUTF8[3] - 0x80;
	nBytes = 4;
	uMinValue = 0x10000;

	// 11110... four-byte lead byte. fall through to finished.

decodeFinished:
	if (uValue >= uMinValue && Q_IsValidUChar32(uValue))
	{
	decodeFinishedNoCheck:
		uValueOut = uValue;
		bErrorOut = false;
		return nBytes;
	}
decodeError:
	uValueOut = '?';
	bErrorOut = true;
	return nBytes;

decodeFinishedMaybeCESU8:
	// Do we have a full UTF-16 surrogate pair that's been UTF-8 encoded afterwards?
	// That is, do we have 0xD800-0xDBFF followed by 0xDC00-0xDFFF? If so, decode it all.
	if ((uValue - 0xD800u) < 0x400u && pUTF8[3] == 0xED && (uint8)(pUTF8[4] - 0xB0) < 0x10 && (pUTF8[5] & 0xC0) == 0x80)
	{
		uValue = 0x10000 + ((uValue - 0xD800u) << 10) + ((uint8)(pUTF8[4] - 0xB0) << 6) + pUTF8[5] - 0x80;
		nBytes = 6;
		uMinValue = 0x10000;
	}
	goto decodeFinished;
}



//-----------------------------------------------------------------------------
// Purpose: Returns true if UTF-8 string contains invalid sequences.
//-----------------------------------------------------------------------------
bool Q_UnicodeValidate(const char* pUTF8)
{
	bool bError = false;
	while ('\0' != *pUTF8)
	{
		uchar32 uVal;
		// Our UTF-8 decoder silently fixes up 6-byte CESU-8 (improperly re-encoded UTF-16) sequences.
		// However, these are technically not valid UTF-8. So if we eat 6 bytes at once, it's an error.
		int nCharSize = Q_UTF8ToUChar32(pUTF8, uVal, bError);
		if (bError || nCharSize == 6)
			return false;
		pUTF8 += nCharSize;
	}
	return true;
}

//// HOST_SAY
// String comes in as
// say blah blah blah
// or as
// blah blah blah
//
void Host_Say(Entity* pEntity, bool teamonly)
{
	CBasePlayer* client;
	char* p;
	char szTemp[256];
	const char* cpSay = "say";
	const char* cpSayTeam = "say_team";
	const char* pcmd = engine::Cmd_Argv(0);

	// We can get a raw string now, without the "say " prepended
	if (engine::Cmd_Argc() == 0)
		return;

	auto player = pEntity->Get<CBasePlayer>();

	//Not yet.
	if (player->m_flNextChatTime > gpGlobals->time)
		return;

	if (!stricmp(pcmd, cpSay) || !stricmp(pcmd, cpSayTeam))
	{
		if (engine::Cmd_Argc() >= 2)
		{
			p = (char*)engine::Cmd_Args();
		}
		else
		{
			// say with a blank message, nothing to do
			return;
		}
	}
	else // Raw text, need to prepend argv[0]
	{
		if (engine::Cmd_Argc() >= 2)
		{
			sprintf(szTemp, "%s %s", (char*)pcmd, (char*)engine::Cmd_Args());
		}
		else
		{
			// Just a one word command, use the first word...sigh
			sprintf(szTemp, "%s", (char*)pcmd);
		}
		p = szTemp;
	}

	// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[strlen(p) - 1] = 0;
	}

	player->m_flNextChatTime = gpGlobals->time + CHAT_INTERVAL;

	// make sure the text has content

	if (!p || '\0' == p[0] || !Q_UnicodeValidate(p))
		return; // no character found, so say nothing

	if (p[strlen(p) - 1] == '\n')
		p[strlen(p) - 1] = '\0';

	if (p[0] == '!')
	{
		char* argv[8];

		argv[0] = strtok(p + 1, " ");

		if (argv[0] != nullptr && argv[0][0] != '\0')
		{
			int argc = 1;

			do {
				argv[argc] = strtok(nullptr, " ");
			} while (argv[argc] != nullptr && (++argc) < ARRAYSIZE(argv));

			g_pGameRules->SayCommand(player, argc, argv);
		}

		return;
	}

	// loop through all players

	for (auto i = 1; i <= gpGlobals->maxClients; i++)
	{
		client = static_cast<CBasePlayer*>(util::PlayerByIndex(i));

		if (client == nullptr || client == player || client->IsBot())
		{
			continue;
		}

		/* Can the receiver hear the sender, or have they muted them? */
		if (g_VoiceGameMgr.PlayerHasBlockedPlayer(client, player))
		{
			continue;
		}

		if (teamonly
		 && g_pGameRules->PlayerRelationship(client, player) != GR_TEAMMATE)
		{
			continue;
		}

		MessageBegin(MSG_ONE, gmsgSayText, client);
		WriteByte(player->v.GetIndex());
		WriteByte(teamonly);
		WriteString(p);
		MessageEnd();
	}

	// print to the sending client
	MessageBegin(MSG_ONE, gmsgSayText, pEntity->Get<CBaseEntity>());
	WriteByte(player->v.GetIndex());
	WriteByte(teamonly);
	WriteString(p);
	MessageEnd();

	const char* temp;
	if (teamonly)
		temp = "say_team";
	else
		temp = "say";

	util::LogPrintf("\"%s<%i><%s><>\" %s \"%s\"\n",
		STRING(pEntity->netname),
		engine::GetPlayerUserId(pEntity),
		engine::GetPlayerAuthId(pEntity),
		temp,
		p);
}


/*
===========
ClientCommand
called each time a player uses a "cmd" command
============
*/
// Use engine::Cmd_Argv,  engine::Cmd_Argv, and engine::Cmd_Argc to get pointers the character string command.
void ClientCommand(Entity* pEntity)
{
	const char* pcmd = engine::Cmd_Argv(0);
	const char* pstr;

	auto player = pEntity->Get<CBasePlayer>();

	// Is the client spawned yet?
	if (player == nullptr)
	{
		return;
	}

	if (streq(pcmd, "say"))
	{
		Host_Say(pEntity, false);
	}
	else if (streq(pcmd, "say_team"))
	{
		Host_Say(pEntity, true);
	}
	else if (streq(pcmd, "fullupdate"))
	{
		player->ForceClientDllUpdate();
	}
	else if (streq(pcmd, "give"))
	{
		if (g_pGameRules->IsPlayerPrivileged(player))
		{
			int iszItem = engine::AllocString(engine::Cmd_Argv(1)); // Make a copy of the classname
			player->GiveNamedItem(STRING(iszItem));
		}
	}
	else if (streq(pcmd, "drop"))
	{
		// player is dropping an item.
		player->DropPlayerWeapon((char*)engine::Cmd_Argv(1));
	}
	else if (streq(pcmd, "fov"))
	{
		if (g_pGameRules->IsPlayerPrivileged(player))
		{
			if (engine::Cmd_Argc() > 1)
			{
				player->m_iFOV = atoi(engine::Cmd_Argv(1));
			}
			else
			{
				engine::ClientPrintf(pEntity, print_console, util::VarArgs("\"fov\" is \"%d\"\n", (int)player->m_iFOV));
			}
		}
	}
	else if (streq(pcmd, "closemenus"))
	{
	}
	else if (streq(pcmd, "saveme"))
	{
		player->SaveMe();
	}
	else if (streq(pcmd, "dropitems"))
	{
		if (player->IsPlayer())
		{
			player->RemoveGoalItems(false);
		}
	}
	else if (streq(pcmd, "discard"))
	{
		player->DiscardAmmo();
	}
	else if (streq(pcmd, "detstart"))
	{
		if (player->HasPlayerWeapon(WEAPON_DETPACK))
		{
			auto detpack = dynamic_cast<CDetpack*>(player->m_rgpPlayerWeapons[WEAPON_DETPACK]);
			auto fuse = 5;

			if (engine::Cmd_Argc() > 1)
			{
				/* Clamp the user fuse value. */
				fuse = std::clamp(atoi(engine::Cmd_Argv(1)), 5, 50);
			}

			detpack->v.pain_finished = fuse;
		}
	}
	else if (streq(pcmd, "disguise_enemy"))
	{
		if (player->PCNumber() == PC_SPY)
		{
			if (engine::Cmd_Argc() > 1)
			{
				auto playerClass = atoi(engine::Cmd_Argv(1));

				player->StartDisguising(playerClass, false);
			}
		}
	}
	else if (streq(pcmd, "disguise_friendly"))
	{
		if (player->PCNumber() == PC_SPY)
		{
			if (engine::Cmd_Argc() > 1)
			{
				auto playerClass = atoi(engine::Cmd_Argv(1));

				player->StartDisguising(playerClass, true);
			}
		}
	}
	else if (g_pGameRules->ClientCommand(player, pcmd))
	{
	}
#ifdef HALFLIFE_BOTS
	else if (g_pBotMan->ClientCommand(player, pcmd))
	{
	}
#endif
	else
	{
		// tell the user they entered an unknown command
		char command[128];

		// check the length of the command (prevents crash)
		// max total length is 192 ...and we're adding a string below ("Unknown command: %s\n")
		strncpy(command, pcmd, 127);
		command[127] = '\0';

		// tell the user they entered an unknown command
		util::ClientPrint(player, HUD_PRINTCONSOLE, util::VarArgs("Unknown command: %s\n", command));
	}
}


/*
========================
ClientUserInfoChanged

called after the player changes
userinfo - gives dll a chance to modify it before
it gets sent into the rest of the engine.
========================
*/
void ClientUserInfoChanged(Entity* pEntity, char* infobuffer)
{
	// Is the client spawned yet?
	if (pEntity->Get<CBasePlayer>() == nullptr)
	{
		return;
	}

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	if (!FStringNull(pEntity->netname) && STRING(pEntity->netname)[0] != 0 && !streq(pEntity->netname, engine::InfoKeyValue(infobuffer, "name")))
	{
		char sName[256];
		char* pName = engine::InfoKeyValue(infobuffer, "name");
		strncpy(sName, pName, sizeof(sName) - 1);
		sName[sizeof(sName) - 1] = '\0';

		// First parse the name and remove any %'s
		for (char* pApersand = sName; pApersand != nullptr && *pApersand != 0; pApersand++)
		{
			// Replace it with a space
			if (*pApersand == '%')
				*pApersand = ' ';
		}

		// Set the name
		engine::SetClientKeyValue(pEntity->GetIndex(), infobuffer, "name", sName);

		if (util::IsMultiplayer())
		{	
			util::ClientPrintAll(HUD_PRINTTALK, "#Game_name_change", STRING(pEntity->netname), engine::InfoKeyValue(infobuffer, "name"));

			util::LogPrintf("\"%s<%i><%s><>\" changed name to \"%s\"\n",
				STRING(pEntity->netname),
				engine::GetPlayerUserId(pEntity),
				engine::GetPlayerAuthId(pEntity),
				engine::InfoKeyValue(infobuffer, "name"));
		}
	}

	g_pGameRules->ClientUserInfoChanged(pEntity->Get<CBasePlayer>(), infobuffer);
}

static int g_serveractive = 0;

void ServerDeactivate()
{
	// It's possible that the engine will call this function more times than is necessary
	//  Therefore, only run it one time for each call to ServerActivate
	if (g_serveractive != 1)
	{
		return;
	}

	g_serveractive = 0;

#ifdef HALFLIFE_BOTS
	if (g_pBotMan)
	{
		g_pBotMan->ServerDeactivate();
	}
#endif
}

void ServerActivate(Entity* pEdictList, int edictCount, int clientMax)
{
	int i;
	CBaseEntity* pClass;

	// Every call to ServerActivate should be matched by a call to ServerDeactivate
	g_serveractive = 1;

	// Clients have not been initialized yet
	for (i = 0; i < edictCount; i++)
	{
		if (pEdictList[i].IsFree())
			continue;

		// Clients aren't necessarily initialized until ClientPutInServer()
		if (i > 0 && i <= clientMax)
		{
			engine::FreeEntPrivateData(pEdictList + i);
			continue;
		}

		pClass = pEdictList[i].Get<CBaseEntity>();
		// Activate this entity if it's got a class & isn't dormant
#ifdef HALFLIFE_SAVERESTORE
		if (pClass != nullptr && (pClass->v.flags & FL_DORMANT) == 0)
#else
		if (pClass != nullptr)
#endif
		{
			pClass->Activate();
		}
		else
		{
			engine::AlertMessage(at_console, "Can't instance %s\n", STRING(pEdictList[i].classname));
		}
	}

	// Link user messages here to make sure first client can get them...
	LinkUserMessages();

#ifdef HALFLIFE_BOTS
	if (g_pBotMan)
	{
		g_pBotMan->ServerActivate();
	}
#endif
}


/*
================
PlayerPreThink

Called every frame before physics are run
================
*/
void PlayerPreThink(Entity* pEntity)
{
	CBasePlayer* pPlayer = pEntity->Get<CBasePlayer>();

	if (pPlayer)
	{
		pPlayer->PreThink();
	}
}

/*
================
PlayerPostThink

Called every frame after physics are run
================
*/
void PlayerPostThink(Entity* pEntity)
{
	CBasePlayer* pPlayer = pEntity->Get<CBasePlayer>();
	const int msec = static_cast<int>(std::roundf(gpGlobals->frametime * 1000));

	if (pPlayer)
	{
		int netLoss;
		engine::GetPlayerStats(&pPlayer->v, &pPlayer->m_netPing, &netLoss);

		pPlayer->PostThink();

		pPlayer->DecrementTimers(msec);

		for (auto gun : pPlayer->m_lpPlayerWeapons)
		{
			gun->DecrementTimers(msec);
		}
	}
}



void ParmsNewLevel()
{
}


void ParmsChangeLevel()
{
#ifdef HALFLIFE_SAVERESTORE
	// retrieve the pointer to the save data
	SAVERESTOREDATA* pSaveData = (SAVERESTOREDATA*)gpGlobals->pSaveData;

	if (pSaveData)
		pSaveData->connectionCount = util::BuildChangeList(pSaveData->levelList, MAX_LEVEL_CONNECTIONS);
#endif
}

#ifdef HALFLIFE_NODEGRAPH
static std::vector<std::string> g_MapsToLoad;

static void LoadNextMap()
{
	const std::string mapName = std::move(g_MapsToLoad.back());
	g_MapsToLoad.pop_back();

	pmove->Con_Printf("Loading map \"%s\" automatically (%d left)\n",
		mapName.c_str(), static_cast<int>(g_MapsToLoad.size() + 1));

	if (g_MapsToLoad.empty())
	{
		pmove->Con_Printf("Loading last map\n");
		g_MapsToLoad.shrink_to_fit();
	}

	engine::ServerCommand(util::VarArgs("map \"%s\"\n", mapName.c_str()));
}

static void LoadAllMaps()
{
	if (!g_MapsToLoad.empty())
	{
		pmove->Con_Printf("Already loading all maps (%d remaining)\nUse sv_stop_loading_all_maps to stop\n",
			static_cast<int>(g_MapsToLoad.size()));
		return;
	}

	FileFindHandle_t handle = FILESYSTEM_INVALID_FIND_HANDLE;

	const char* fileName = g_pFileSystem->FindFirst("maps/*.bsp", &handle);

	if (fileName != nullptr)
	{
		do
		{
			std::string mapName = fileName;
			mapName.resize(mapName.size() - 4);

			if (std::find_if(g_MapsToLoad.begin(), g_MapsToLoad.end(), [=](const auto& candidate)
					{ return 0 == stricmp(candidate.c_str(), mapName.c_str()); }) == g_MapsToLoad.end())
			{
				g_MapsToLoad.push_back(std::move(mapName));
			}
		} while ((fileName = g_pFileSystem->FindNext(handle)) != nullptr);

		g_pFileSystem->FindClose(handle);

		// Sort in reverse order so the first map in alphabetic order is loaded first.
		std::sort(g_MapsToLoad.begin(), g_MapsToLoad.end(), [](const auto& lhs, const auto& rhs)
			{ return rhs < lhs; });
	}

	if (!g_MapsToLoad.empty())
	{
		if (engine::Cmd_Argc() == 2)
		{
			const char* firstMapToLoad = engine::Cmd_Argv(1);

			// Clear out all maps that would have been loaded before this one.
			if (auto it = std::find(g_MapsToLoad.begin(), g_MapsToLoad.end(), firstMapToLoad);
				it != g_MapsToLoad.end())
			{
				const std::size_t numberOfMapsToSkip = g_MapsToLoad.size() - (it - g_MapsToLoad.begin());

				g_MapsToLoad.erase(it + 1, g_MapsToLoad.end());

				pmove->Con_Printf("Skipping %d maps to start with \"%s\"\n",
					static_cast<int>(numberOfMapsToSkip), g_MapsToLoad.back().c_str());
			}
			else
			{
				pmove->Con_Printf("Unknown map \"%s\", starting from beginning\n", firstMapToLoad);
			}
		}

		pmove->Con_Printf("Loading %d maps one at a time to generate files\n", static_cast<int>(g_MapsToLoad.size()));

		// Load the first map right now.
		LoadNextMap();
	}
	else
	{
		pmove->Con_Printf("No maps to load\n");
	}
}

void InitMapLoadingUtils()
{
	engine::AddServerCommand("sv_load_all_maps", &LoadAllMaps);
	// Escape hatch in case the command is executed in error.
	engine::AddServerCommand("sv_stop_loading_all_maps", []()
		{ g_MapsToLoad.clear(); });
}
#endif

void StartFrame()
{
	Steam_Frame();

	if (g_pGameRules)
	{
		g_pGameRules->Think();
	}
	
#ifdef HALFLIFE_BOTS
	if (g_pBotMan)
	{
		g_pBotMan->StartFrame();
	}
#endif

#ifdef HALFLIFE_NODEGRAPH
	if (g_pGameRules->GetState() == GR_STATE_GAME_OVER)
		return;

	// If we're loading all maps then change maps after 3 seconds (time starts at 1)
	// to give the game time to generate files.
	if (!g_MapsToLoad.empty() && gpGlobals->time > 4)
	{
		LoadNextMap();
	}
#endif
}


void ClientPrecache()
{
	// setup precaches always needed
	engine::PrecacheSound("player/sprayer.wav"); // spray paint sound

	engine::PrecacheSound("player/pl_jumpland2.wav");
	engine::PrecacheSound("player/pl_fallpain3.wav");

	engine::PrecacheSound("player/pl_step1.wav"); // walk on concrete
	engine::PrecacheSound("player/pl_step2.wav");
	engine::PrecacheSound("player/pl_step3.wav");
	engine::PrecacheSound("player/pl_step4.wav");

	engine::PrecacheSound("common/npc_step1.wav"); // NPC walk on concrete
	engine::PrecacheSound("common/npc_step2.wav");
	engine::PrecacheSound("common/npc_step3.wav");
	engine::PrecacheSound("common/npc_step4.wav");

	engine::PrecacheSound("player/pl_metal1.wav"); // walk on metal
	engine::PrecacheSound("player/pl_metal2.wav");
	engine::PrecacheSound("player/pl_metal3.wav");
	engine::PrecacheSound("player/pl_metal4.wav");

	engine::PrecacheSound("player/pl_dirt1.wav"); // walk on dirt
	engine::PrecacheSound("player/pl_dirt2.wav");
	engine::PrecacheSound("player/pl_dirt3.wav");
	engine::PrecacheSound("player/pl_dirt4.wav");

	engine::PrecacheSound("player/pl_duct1.wav"); // walk in duct
	engine::PrecacheSound("player/pl_duct2.wav");
	engine::PrecacheSound("player/pl_duct3.wav");
	engine::PrecacheSound("player/pl_duct4.wav");

	engine::PrecacheSound("player/pl_grate1.wav"); // walk on grate
	engine::PrecacheSound("player/pl_grate2.wav");
	engine::PrecacheSound("player/pl_grate3.wav");
	engine::PrecacheSound("player/pl_grate4.wav");

	engine::PrecacheSound("player/pl_slosh1.wav"); // walk in shallow water
	engine::PrecacheSound("player/pl_slosh2.wav");
	engine::PrecacheSound("player/pl_slosh3.wav");
	engine::PrecacheSound("player/pl_slosh4.wav");

	engine::PrecacheSound("player/pl_tile1.wav"); // walk on tile
	engine::PrecacheSound("player/pl_tile2.wav");
	engine::PrecacheSound("player/pl_tile3.wav");
	engine::PrecacheSound("player/pl_tile4.wav");
	engine::PrecacheSound("player/pl_tile5.wav");

	engine::PrecacheSound("player/pl_swim1.wav"); // breathe bubbles
	engine::PrecacheSound("player/pl_swim2.wav");
	engine::PrecacheSound("player/pl_swim3.wav");
	engine::PrecacheSound("player/pl_swim4.wav");

	engine::PrecacheSound("player/pl_ladder1.wav"); // climb ladder rung
	engine::PrecacheSound("player/pl_ladder2.wav");
	engine::PrecacheSound("player/pl_ladder3.wav");
	engine::PrecacheSound("player/pl_ladder4.wav");

	engine::PrecacheSound("player/pl_wade1.wav"); // wade in water
	engine::PrecacheSound("player/pl_wade2.wav");
	engine::PrecacheSound("player/pl_wade3.wav");
	engine::PrecacheSound("player/pl_wade4.wav");

	engine::PrecacheSound("debris/wood1.wav"); // hit wood texture
	engine::PrecacheSound("debris/wood2.wav");
	engine::PrecacheSound("debris/wood3.wav");

	engine::PrecacheSound("plats/train_use1.wav"); // use a train

	engine::PrecacheSound("buttons/spark5.wav"); // hit computer texture
	engine::PrecacheSound("buttons/spark6.wav");
	engine::PrecacheSound("debris/glass1.wav");
	engine::PrecacheSound("debris/glass2.wav");
	engine::PrecacheSound("debris/glass3.wav");

	// player gib sounds
	engine::PrecacheSound("common/bodysplat.wav");

	// player pain sounds
	engine::PrecacheSound("player/pain1.wav");
	engine::PrecacheSound("player/pain2.wav");
	engine::PrecacheSound("player/pain3.wav");
	engine::PrecacheSound("player/pain4.wav");
	engine::PrecacheSound("player/pain5.wav");
	engine::PrecacheSound("player/pain6.wav");

	engine::PrecacheSound("player/drown1.wav");
	engine::PrecacheSound("player/drown2.wav");

	engine::PrecacheSound("player/death1.wav");
	engine::PrecacheSound("player/death2.wav");
	engine::PrecacheSound("player/death3.wav");
	engine::PrecacheSound("player/death4.wav");
	engine::PrecacheSound("player/death5.wav");

	engine::PrecacheSound("player/h2odeath.wav");

	g_sModelIndexPlayer = engine::PrecacheModel("models/player/scout/scout.mdl");
	g_sModelIndexGibs = engine::PrecacheModel("models/hgibs.mdl");

	// hud sounds

	engine::PrecacheSound("common/wpn_hudoff.wav");
	engine::PrecacheSound("common/wpn_hudon.wav");
	engine::PrecacheSound("common/wpn_moveselect.wav");
	engine::PrecacheSound("common/wpn_select.wav");
	engine::PrecacheSound("common/wpn_denyselect.wav");

	engine::PrecacheSound("misc/r_tele1.wav");

	engine::PrecacheSound("common/null.wav");

	engine::PrecacheSound("player/plyrjmp8.wav");

	engine::PrecacheSound("speech/saveme1.wav");
	engine::PrecacheSound("speech/saveme2.wav");
	engine::PrecacheSound("speech/saveme3.wav");

	engine::PrecacheModel("models/backpack.mdl");
	engine::PrecacheSound("items/ammopickup1.wav");
	engine::PrecacheSound("items/ammopickup2.wav");

	engine::PrecacheSound("debris/beamstart1.wav");
	engine::PrecacheSound("debris/beamstart2.wav");

	g_usGibbed = engine::PrecacheEvent(1, "events/gibs.sc");
	g_usTeleport = engine::PrecacheEvent(1, "events/teleport.sc");
	g_usExplosion = engine::PrecacheEvent(1, "events/explosion.sc");
	g_usConcBlast = engine::PrecacheEvent(1, "events/explode/tf_concuss.sc");
	g_usEMP = engine::PrecacheEvent(1, "events/explode/tf_emp.sc");
	g_usFlash = engine::PrecacheEvent(1, "events/explode/tf_gas.sc");
	g_usGetNailedIdiot = engine::PrecacheEvent(1, "events/explode/tf_nailgren.sc");
	g_usTrail = engine::PrecacheEvent(1, "events/trail.sc");
}

/*
===============
GetGameDescription

Returns the descriptive name of this .dll.  E.g., Half-Life, or Team Fortress 2
===============
*/
const char* GetGameDescription()
{
	if (g_pGameRules) // this function may be called before the world has spawned, and the game rules initialized
		return g_pGameRules->GetGameDescription();
	else
		return HALFLIFE_GAMEDESC;
}

/*
================
Sys_Error

Engine is going to shut down, allows setting a breakpoint in game .dll to catch that occasion
================
*/
void Sys_Error(const char* error_string)
{
	// Default case, do nothing.  MOD AUTHORS:  Add code ( e.g., _asm { int 3 }; here to cause a breakpoint for debugging your game .dlls
}

/*
================
PlayerCustomization

A new player customization has been registered on the server
================
*/
void PlayerCustomization(Entity* pEntity, customization_t* pCust)
{
	CBasePlayer* pPlayer = pEntity->Get<CBasePlayer>();

	if (!pPlayer)
	{
		engine::AlertMessage(at_console, "PlayerCustomization:  Couldn't get player!\n");
		return;
	}

	if (!pCust)
	{
		engine::AlertMessage(at_console, "PlayerCustomization:  NULL customization!\n");
		return;
	}

	switch (pCust->resource.type)
	{
	case t_decal:
		pPlayer->SetCustomDecalFrames(pCust->nUserData2); // Second int is max # of frames.
		break;
	case t_sound:
	case t_skin:
	case t_model:
		// Ignore for now.
		break;
	default:
		engine::AlertMessage(at_console, "PlayerCustomization:  Unknown customization type!\n");
		break;
	}
}

/*
================
SpectatorConnect

A spectator has joined the game
================
*/
void SpectatorConnect(Entity* pEntity)
{
	CBaseSpectator* pPlayer = pEntity->Get<CBaseSpectator>();

	if (pPlayer)
		pPlayer->SpectatorConnect();
}

/*
================
SpectatorConnect

A spectator has left the game
================
*/
void SpectatorDisconnect(Entity* pEntity)
{
	CBaseSpectator* pPlayer = pEntity->Get<CBaseSpectator>();

	if (pPlayer)
		pPlayer->SpectatorDisconnect();
}

/*
================
SpectatorConnect

A spectator has sent a usercmd
================
*/
void SpectatorThink(Entity* pEntity)
{
	CBaseSpectator* pPlayer = pEntity->Get<CBaseSpectator>();

	if (pPlayer)
		pPlayer->SpectatorThink();
}

////////////////////////////////////////////////////////
// PAS and PVS routines for client messaging
//

/*
================
SetupVisibility

A client can have a separate "view entity" indicating that his/her view should depend on the origin of that
view entity.  If that's the case, then pViewEntity will be non-nullptr and will be used.  Otherwise, the current
entity's origin is used.  Either is offset by the view_ofs to get the eye position.

From the eye position, we set up the PAS and PVS to use for filtering network messages to the client.  At this point, we could
 override the actual PAS or PVS values, or use a different origin.

NOTE:  Do not cache the values of pas and pvs, as they depend on reusable memory in the engine, they are only good for this one frame
================
*/
void SetupVisibility(Entity* pViewEntity, Entity* pClient, unsigned char** pvs, unsigned char** pas)
{
	Vector org;
	Entity* pView = pClient;

	// Find the client's PVS
	if (pViewEntity)
	{
		pView = pViewEntity;
	}

	if ((pClient->flags & FL_PROXY) != 0)
	{
		*pvs = nullptr; // the spectator proxy sees
		*pas = nullptr; // and hears everything
		return;
	}

	org = pView->origin + pView->view_ofs;
	if ((pView->flags & FL_DUCKING) != 0)
	{
		org = org + (VEC_HULL_MIN - VEC_DUCK_HULL_MIN);
	}

	*pvs = engine::SetFatPVS(org);
	*pas = engine::SetFatPAS(org);
}

#include "entity_state.h"

/*
AddToFullPack

Return 1 if the entity state has been filled in for the ent and the entity will be propagated to the client, 0 otherwise

state is the server maintained copy of the state info that is transmitted to the client
a MOD could alter values copied into state to send the "host" a different look for a particular entity update, etc.
e and ent are the entity that is being added to the update, if 1 is returned
host is the player's edict of the player whom we are sending the update to
player is 1 if the ent/e is a player and 0 otherwise
pSet is either the PAS or PVS that we previous set up.  We can use it to ask the engine to filter the entity against the PAS or PVS.
we could also use the pas/ pvs that we set in SetupVisibility, if we wanted to.  Caching the value is valid in that case, but still only for the current frame
*/
int AddToFullPack(struct entity_state_s* state, int e, Entity* ent, Entity* host, int hostflags, int player, unsigned char* pSet)
{
	// Entities with an index greater than this will corrupt the client's heap because 
	// the index is sent with only 11 bits of precision (2^11 == 2048).
	// So we don't send them, just like having too many entities would result
	// in the entity not being sent.
	if (e >= MAX_EDICTS)
	{
		return 0;
	}

	auto entity = ent->Get<CBaseEntity>();
	auto other = host->Get<CBasePlayer>();

	if (entity == nullptr || other == nullptr)
	{
		return 0;
	}

	if (ent != host)
	{
		// don't send if flagged for NODRAW and it's not the host getting the message
		if ((ent->effects & EF_NODRAW) != 0)
		{
			return 0;
		}

		// Ignore ents without valid / visible models
		if (0 == ent->modelindex || !STRING(ent->model))
		{
			return 0;
		}

		// Don't send spectators to other players
		if ((ent->flags & FL_SPECTATOR) != 0)
		{
			return 0;
		}

		// Ignore if not the host and not touching a PVS/PAS leaf
		// If pSet is nullptr, then the test will always succeed and the entity will be added to the update
		if ((entity->ObjectCaps() & FCAP_NET_ALWAYS_SEND) == 0)
		{
			if (!engine::CheckVisibility(ent, pSet))
			{
				return 0;
			}
		}
	}


	// Don't send entity to local client if the client says it's predicting the entity itself.
	if ((ent->flags & FL_SKIPLOCALHOST) != 0)
	{
		if ((hostflags & 1) != 0 && (ent->owner == host))
			return 0;
	}

	if (0 != host->groupinfo)
	{
		util::SetGroupTrace(host->groupinfo, util::GROUP_OP_AND);

		// Should always be set, of course
		if (0 != ent->groupinfo)
		{
			if (util::g_groupop == util::GROUP_OP_AND)
			{
				if ((ent->groupinfo & host->groupinfo) == 0)
					return 0;
			}
			else if (util::g_groupop == util::GROUP_OP_NAND)
			{
				if ((ent->groupinfo & host->groupinfo) != 0)
					return 0;
			}
		}

		util::UnsetGroupTrace();
	}

	memset(state, 0, sizeof(entity_state_t));

	// Assign index so we can track this entity from frame to frame and
	// delta from it.
	state->number = e;

	entity->GetEntityState(*state, other);

	return 1;
}

/*
===================
CreateBaseline

Creates baselines used for network encoding, especially for player data since players are not spawned until connect time.
===================
*/
void CreateBaseline(int player, int eindex, struct entity_state_s* baseline, Entity* entity, int playermodelindex, Vector* player_mins, Vector* player_maxs)
{
	baseline->origin = entity->origin;
	baseline->angles = entity->angles;
	baseline->frame = entity->frame;
	baseline->skin = (short)entity->skin;

	// render information
	baseline->rendermode = (byte)entity->rendermode;
	baseline->renderamt = (byte)entity->renderamt;
	baseline->rendercolor.r = (byte)entity->rendercolor.x;
	baseline->rendercolor.g = (byte)entity->rendercolor.y;
	baseline->rendercolor.b = (byte)entity->rendercolor.z;
	baseline->renderfx = (byte)entity->renderfx;

	if (0 != player)
	{
		baseline->mins = *player_mins;
		baseline->maxs = *player_maxs;

		baseline->colormap = eindex;
		baseline->modelindex = playermodelindex;
		baseline->friction = 1.0;
		baseline->movetype = MOVETYPE_WALK;

		baseline->scale = entity->scale;
		baseline->solid = SOLID_SLIDEBOX;
		baseline->framerate = 1.0;
		baseline->gravity = 1.0;
	}
	else
	{
		baseline->mins = entity->mins;
		baseline->maxs = entity->maxs;

		baseline->colormap = 0;
		baseline->modelindex = entity->modelindex;
		baseline->movetype = entity->movetype;

		baseline->scale = entity->scale;
		baseline->solid = entity->solid;
		baseline->framerate = entity->framerate;
		baseline->gravity = entity->gravity;
	}
}

typedef struct
{
	char name[32];
	int field;
} entity_field_alias_t;

#define FIELD_ORIGIN0 0
#define FIELD_ORIGIN1 1
#define FIELD_ORIGIN2 2
#define FIELD_ANGLES0 3
#define FIELD_ANGLES1 4
#define FIELD_ANGLES2 5

static entity_field_alias_t entity_field_alias[] =
	{
		{"origin[0]", 0},
		{"origin[1]", 0},
		{"origin[2]", 0},
		{"angles[0]", 0},
		{"angles[1]", 0},
		{"angles[2]", 0},
};

void Entity_FieldInit(struct delta_s* pFields)
{
	entity_field_alias[FIELD_ORIGIN0].field = engine::DeltaFindField(pFields, entity_field_alias[FIELD_ORIGIN0].name);
	entity_field_alias[FIELD_ORIGIN1].field = engine::DeltaFindField(pFields, entity_field_alias[FIELD_ORIGIN1].name);
	entity_field_alias[FIELD_ORIGIN2].field = engine::DeltaFindField(pFields, entity_field_alias[FIELD_ORIGIN2].name);
	entity_field_alias[FIELD_ANGLES0].field = engine::DeltaFindField(pFields, entity_field_alias[FIELD_ANGLES0].name);
	entity_field_alias[FIELD_ANGLES1].field = engine::DeltaFindField(pFields, entity_field_alias[FIELD_ANGLES1].name);
	entity_field_alias[FIELD_ANGLES2].field = engine::DeltaFindField(pFields, entity_field_alias[FIELD_ANGLES2].name);
}

/*
==================
Entity_Encode

Callback for sending entity_state_t info over network. 
FIXME:  Move to script
==================
*/
void Entity_Encode(struct delta_s* pFields, const unsigned char* from, const unsigned char* to)
{
	entity_state_t *f, *t;
	static bool initialized = false;

	if (!initialized)
	{
		Entity_FieldInit(pFields);
		initialized = true;
	}

	f = (entity_state_t*)from;
	t = (entity_state_t*)to;

	// Never send origin to local player, it's sent with more resolution in clientdata_t structure
	const bool localplayer = (t->number - 1) == engine::GetCurrentPlayer();
	if (localplayer)
	{
		engine::DeltaUnsetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		engine::DeltaUnsetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		engine::DeltaUnsetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}

	if ((t->impacttime != 0) && (t->starttime != 0))
	{
		engine::DeltaUnsetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		engine::DeltaUnsetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		engine::DeltaUnsetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN2].field);

		engine::DeltaUnsetFieldByIndex(pFields, entity_field_alias[FIELD_ANGLES0].field);
		engine::DeltaUnsetFieldByIndex(pFields, entity_field_alias[FIELD_ANGLES1].field);
		engine::DeltaUnsetFieldByIndex(pFields, entity_field_alias[FIELD_ANGLES2].field);
	}

	if ((t->movetype == MOVETYPE_FOLLOW) &&
		(t->aiment != 0))
	{
		engine::DeltaUnsetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		engine::DeltaUnsetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		engine::DeltaUnsetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
	else if (t->aiment != f->aiment)
	{
		engine::DeltaSetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		engine::DeltaSetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		engine::DeltaSetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
}

static entity_field_alias_t player_field_alias[] =
	{
		{"origin[0]", 0},
		{"origin[1]", 0},
		{"origin[2]", 0},
};

void Player_FieldInit(struct delta_s* pFields)
{
	player_field_alias[FIELD_ORIGIN0].field = engine::DeltaFindField(pFields, player_field_alias[FIELD_ORIGIN0].name);
	player_field_alias[FIELD_ORIGIN1].field = engine::DeltaFindField(pFields, player_field_alias[FIELD_ORIGIN1].name);
	player_field_alias[FIELD_ORIGIN2].field = engine::DeltaFindField(pFields, player_field_alias[FIELD_ORIGIN2].name);
}

/*
==================
Player_Encode

Callback for sending entity_state_t for players info over network. 
==================
*/
void Player_Encode(struct delta_s* pFields, const unsigned char* from, const unsigned char* to)
{
	entity_state_t *f, *t;
	static bool initialized = false;

	if (!initialized)
	{
		Player_FieldInit(pFields);
		initialized = true;
	}

	f = (entity_state_t*)from;
	t = (entity_state_t*)to;

	// Never send origin to local player, it's sent with more resolution in clientdata_t structure
	const bool localplayer = (t->number - 1) == engine::GetCurrentPlayer();
	if (localplayer)
	{
		engine::DeltaUnsetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		engine::DeltaUnsetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		engine::DeltaUnsetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}

	if ((t->movetype == MOVETYPE_FOLLOW) &&
		(t->aiment != 0))
	{
		engine::DeltaUnsetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		engine::DeltaUnsetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		engine::DeltaUnsetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
	else if (t->aiment != f->aiment)
	{
		engine::DeltaSetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		engine::DeltaSetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		engine::DeltaSetFieldByIndex(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
}

#define CUSTOMFIELD_ORIGIN0 0
#define CUSTOMFIELD_ORIGIN1 1
#define CUSTOMFIELD_ORIGIN2 2
#define CUSTOMFIELD_ANGLES0 3
#define CUSTOMFIELD_ANGLES1 4
#define CUSTOMFIELD_ANGLES2 5
#define CUSTOMFIELD_SKIN 6
#define CUSTOMFIELD_SEQUENCE 7
#define CUSTOMFIELD_ANIMTIME 8

entity_field_alias_t custom_entity_field_alias[] =
	{
		{"origin[0]", 0},
		{"origin[1]", 0},
		{"origin[2]", 0},
		{"angles[0]", 0},
		{"angles[1]", 0},
		{"angles[2]", 0},
		{"skin", 0},
		{"sequence", 0},
		{"animtime", 0},
};

void Custom_Entity_FieldInit(struct delta_s* pFields)
{
	custom_entity_field_alias[CUSTOMFIELD_ORIGIN0].field = engine::DeltaFindField(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN0].name);
	custom_entity_field_alias[CUSTOMFIELD_ORIGIN1].field = engine::DeltaFindField(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN1].name);
	custom_entity_field_alias[CUSTOMFIELD_ORIGIN2].field = engine::DeltaFindField(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN2].name);
	custom_entity_field_alias[CUSTOMFIELD_ANGLES0].field = engine::DeltaFindField(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES0].name);
	custom_entity_field_alias[CUSTOMFIELD_ANGLES1].field = engine::DeltaFindField(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES1].name);
	custom_entity_field_alias[CUSTOMFIELD_ANGLES2].field = engine::DeltaFindField(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES2].name);
	custom_entity_field_alias[CUSTOMFIELD_SKIN].field = engine::DeltaFindField(pFields, custom_entity_field_alias[CUSTOMFIELD_SKIN].name);
	custom_entity_field_alias[CUSTOMFIELD_SEQUENCE].field = engine::DeltaFindField(pFields, custom_entity_field_alias[CUSTOMFIELD_SEQUENCE].name);
	custom_entity_field_alias[CUSTOMFIELD_ANIMTIME].field = engine::DeltaFindField(pFields, custom_entity_field_alias[CUSTOMFIELD_ANIMTIME].name);
}

/*
==================
Custom_Encode

Callback for sending entity_state_t info ( for custom entities ) over network. 
FIXME:  Move to script
==================
*/
void Custom_Encode(struct delta_s* pFields, const unsigned char* from, const unsigned char* to)
{
	entity_state_t *f, *t;
	int beamType;
	static bool initialized = false;

	if (!initialized)
	{
		Custom_Entity_FieldInit(pFields);
		initialized = true;
	}

	f = (entity_state_t*)from;
	t = (entity_state_t*)to;

	beamType = t->rendermode & 0x0f;

	if (beamType != BEAM_POINTS && beamType != BEAM_ENTPOINT)
	{
		engine::DeltaUnsetFieldByIndex(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN0].field);
		engine::DeltaUnsetFieldByIndex(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN1].field);
		engine::DeltaUnsetFieldByIndex(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN2].field);
	}

	if (beamType != BEAM_POINTS)
	{
		engine::DeltaUnsetFieldByIndex(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES0].field);
		engine::DeltaUnsetFieldByIndex(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES1].field);
		engine::DeltaUnsetFieldByIndex(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES2].field);
	}

	if (beamType != BEAM_ENTS && beamType != BEAM_ENTPOINT)
	{
		engine::DeltaUnsetFieldByIndex(pFields, custom_entity_field_alias[CUSTOMFIELD_SKIN].field);
		engine::DeltaUnsetFieldByIndex(pFields, custom_entity_field_alias[CUSTOMFIELD_SEQUENCE].field);
	}

	// animtime is compared by rounding first
	// see if we really shouldn't actually send it
	if ((int)f->animtime == (int)t->animtime)
	{
		engine::DeltaUnsetFieldByIndex(pFields, custom_entity_field_alias[CUSTOMFIELD_ANIMTIME].field);
	}
}

/*
=================
RegisterEncoders

Allows game .dll to override network encoding of certain types of entities and tweak values, etc.
=================
*/
void RegisterEncoders()
{
	engine::DeltaAddEncoder("Entity_Encode", Entity_Encode);
	engine::DeltaAddEncoder("Custom_Encode", Custom_Encode);
	engine::DeltaAddEncoder("Player_Encode", Player_Encode);
}

int GetWeaponData(Entity* player, struct weapon_data_s* info)
{
	memset(info, 0, WEAPON_TYPES * sizeof(weapon_data_t));

	CBasePlayer* pl = player->Get<CBasePlayer>();

	if (!pl)
	{
		return 1;
	}

	for (auto gun : pl->m_lpPlayerWeapons)
	{
		gun->GetWeaponData(info[gun->GetID()]);
	}
	return 1;
}

/*
=================
UpdateClientData

Data sent to current client only
engine sets cd to 0 before calling.
=================
*/
void UpdateClientData(const Entity* ent, int sendweapons, struct clientdata_s* cd)
{
	if (ent == nullptr)
	{
		return;
	}

	auto player = const_cast<Entity*>(ent)->Get<CBasePlayer>();

	if (player == nullptr)
	{
		return;
	}

	cd->pushmsec = ent->pushmsec;
	strcpy(cd->physinfo, engine::GetPhysicsInfoString(ent));

	player->GetClientData(*cd, sendweapons != 0);
}

/*
=================
CmdStart

We're about to run this usercmd for the specified player.  We can set up groupinfo and masking here, etc.
This is the time to examine the usercmd for anything extra.  This call happens even if think does not.
=================
*/
void CmdStart(const Entity* ent, const struct usercmd_s* cmd, unsigned int random_seed)
{
	if (ent == nullptr)
	{
		return;
	}

	auto player = const_cast<Entity*>(ent)->Get<CBasePlayer>();

	if (player == nullptr)
	{
		return;
	}

	player->CmdStart(*cmd, random_seed);

	if (player->v.groupinfo != 0)
	{
		util::SetGroupTrace(player->v.groupinfo, util::GROUP_OP_AND);
	}
}

/*
=================
CmdEnd

Each cmdstart is exactly matched with a cmd end, clean up any group trace flags, etc. here
=================
*/
void CmdEnd(const Entity* player)
{
	if (player == nullptr)
	{
		return;
	}

	if (player->groupinfo != 0)
	{
		util::UnsetGroupTrace();
	}
}

/*
================================
ConnectionlessPacket

 Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
  size of the response_buffer, so you must zero it out if you choose not to respond.
================================
*/
int ConnectionlessPacket(const struct netadr_s* net_from, const char* args, char* response_buffer, int* response_buffer_size)
{
	// Parse stuff from args
	int max_buffer_size = *response_buffer_size;

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}

/*
================================
GetHullBounds

  Engine calls this to enumerate player collision hulls, for prediction.  Return 0 if the hullnumber doesn't exist.
================================
*/
int GetHullBounds(int hullnumber, float* mins, float* maxs)
{
	return static_cast<int>(PM_GetHullBounds(hullnumber, mins, maxs));
}

/*
================================
CreateInstancedBaselines

Create pseudo-baselines for items that aren't placed in the map at spawn time, but which are likely
to be created during play ( e.g., grenades, ammo packs, projectiles, corpses, etc. )
================================
*/
void CreateInstancedBaselines()
{
}

/*
================================
InconsistentFile

One of the pfnForceUnmodified files failed the consistency check for the specified player
 Return 0 to allow the client to continue, 1 to force immediate disconnection ( with an optional disconnect message of up to 256 characters )
================================
*/
int InconsistentFile(const Entity* player, const char* filename, char* disconnect_message)
{
	// Server doesn't care?
	if (mp_consistency->value != 1.0F)
	{
		return 0;
	}

	// Default behavior is to kick the player
	snprintf(disconnect_message, 255, "Server is enforcing file consistency for \"%s\"\n", filename);
	disconnect_message[255] = '\0';

	// Kick now with specified disconnect message.
	return 1;
}

/*
================================
AllowLagCompensation

 The game .dll should return 1 if lag compensation should be allowed ( could also just set
  the sv_unlag cvar.
 Most games right now should return 0, until client-side weapon prediction code is written
  and tested for them ( note you can predict weapons, but not do lag compensation, too, 
  if you want.
================================
*/
int AllowLagCompensation()
{
	return sv_unlag->value;
}

/*
This module implements the shared player physics code between any particular game and 
the engine.  The same PM_Move routine is built into the game .dll and the client .dll and is
invoked by each side as appropriate.  There should be no distinction, internally, between server
and client.  This will ensure that prediction behaves appropriately.
*/
void PM_Move(struct playermove_s* ppmove, qboolean server)
{
	ppmove->server = 1;

	auto player = dynamic_cast<CBasePlayer*>(util::PlayerByIndex(ppmove->player_index + 1));

	if (player != nullptr && player->GetGameMovement() != nullptr)
	{
		player->GetGameMovement()->Move();
	}
}
