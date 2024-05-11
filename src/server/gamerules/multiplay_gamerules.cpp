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
#include "weaponbox.h"
#include "gamerules.h"

#include "game.h"
#include "items.h"
#include "voice_gamemgr.h"
#include "vote_manager.h"
#include "hltv.h"
#include "UserMessages.h"


#define ITEM_RESPAWN_TIME 30
#define WEAPON_RESPAWN_TIME 20
#define AMMO_RESPAWN_TIME 20

CVoiceGameMgr g_VoiceGameMgr;
CVoteManager g_VoteManager;

class CMultiplayGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	bool CanPlayerHearPlayer(CBasePlayer* pListener, CBasePlayer* pTalker) override
	{
		if (g_pGameRules->IsTeamplay())
		{
			if (g_pGameRules->PlayerRelationship(pListener, pTalker) != GR_TEAMMATE)
			{
				return false;
			}
		}

		return true;
	}
};
static CMultiplayGameMgrHelper g_GameMgrHelper;


CTeam::CTeam()
	: m_index{TEAM_SPECTATORS}, m_name{"spectators"}, m_score{0}, m_numPlayers{0}
{
	m_players.clear();
}


CTeam::CTeam(short index, std::string name)
	: m_index{index}, m_name{name}, m_score{0}, m_numPlayers{0}
{
	m_players.clear();
}


void CTeam::AddPlayer(CBasePlayer *player)
{
	if (player->m_team)
	{
		player->m_team->RemovePlayer(player);
	}

	player->m_team = this;

	player->SendExtraInfo();

	m_players.push_back(player);
	m_numPlayers = m_players.size();
}


void CTeam::RemovePlayer(CBasePlayer *player)
{
	for (auto p = m_players.begin(); p != m_players.end(); p++)
	{
		if (*p == player)
		{
			m_players.erase(p);
			break;
		}
	}
	m_numPlayers = m_players.size();
}


void CTeam::AddPoints(float score, bool allowNegative)
{
	// Positive score always adds
	if (score <= 0 && !allowNegative)
	{
		if (m_score <= 0) // Can't go more negative
		{
			return;
		}

		if (-score > m_score) // Will this go negative?
		{
			score = -m_score; // Sum will be 0
		}
	}

	m_score += score;

	MessageBegin(MSG_ALL, gmsgTeamScore);
	WriteByte(m_index);
	WriteShort(m_score);
	MessageEnd();
}


CHalfLifeMultiplay::CHalfLifeMultiplay()
{
	g_VoiceGameMgr.Init(&g_GameMgrHelper, gpGlobals->maxClients);
	g_VoteManager.Init();

	m_deathmatch = (int)gpGlobals->deathmatch != 0;
	m_coop = (int)gpGlobals->coop != 0;
	m_allowMonsters = (int)allowmonsters.value != 0;
	m_allowSpectators = (int)allow_spectators.value != 0;

	m_teams.clear();
	m_teams.push_back(CTeam{TEAM_DEFAULT, "players"});
	m_numTeams = 1;

	m_intermissionTime = 0.0F;

	if (!g_engfuncs.pfnIsDedicatedServer())
	{
		char* lservercfgfile = (char*)CVAR_GET_STRING("lservercfgfile");

		if (lservercfgfile && '\0' != lservercfgfile[0])
		{
			char szCommand[256];

			ALERT(at_console, "Executing listen server config file\n");
			sprintf(szCommand, "exec %s\n", lservercfgfile);
			SERVER_COMMAND(szCommand);
		}
	}

	EnterState(GR_STATE_RND_RUNNING);
}


CHalfLifeMultiplay::~CHalfLifeMultiplay()
{
	for (auto spawn : m_spawnPoints)
	{
		delete spawn;
	}

	m_spawnPoints.clear();
}


bool CHalfLifeMultiplay::PrivilegedCommand(CBasePlayer* pPlayer, const char* pcmd)
{
	if (strcmp(pcmd, "nc") == 0)
	{
		if (pPlayer->v.movetype != MOVETYPE_NOCLIP)
		{
			pPlayer->v.movetype = MOVETYPE_NOCLIP;
			g_engfuncs.pfnClientPrintf(pPlayer->edict(), print_console, "noclip ON\n");
		}
		else
		{
			pPlayer->v.movetype = MOVETYPE_WALK;
			g_engfuncs.pfnClientPrintf(pPlayer->edict(), print_console, "noclip OFF\n");
		}
		return true;
	}
	else if (strcmp(pcmd, "gaben") == 0)
	{
		if ((pPlayer->v.flags & FL_GODMODE) == 0)
		{
			pPlayer->v.flags |= FL_GODMODE;
			g_engfuncs.pfnClientPrintf(pPlayer->edict(), print_console, "godmode ON\n");
		}
		else
		{
			pPlayer->v.flags &= ~FL_GODMODE;
			g_engfuncs.pfnClientPrintf(pPlayer->edict(), print_console, "godmode OFF\n");
		}
		return true;
	}
	else if (strcmp(pcmd, "nt") == 0)
	{
		if ((pPlayer->v.flags & FL_NOTARGET) == 0)
		{
			pPlayer->v.flags |= FL_NOTARGET;
			g_engfuncs.pfnClientPrintf(pPlayer->edict(), print_console, "notarget ON\n");
		}
		else
		{
			pPlayer->v.flags &= ~FL_NOTARGET;
			g_engfuncs.pfnClientPrintf(pPlayer->edict(), print_console, "notarget OFF\n");
		}
		return true;
	}

	return false;
}


bool CHalfLifeMultiplay::ClientCommand(CBasePlayer* pPlayer, const char* pcmd)
{
	if (g_VoiceGameMgr.ClientCommand(pPlayer, pcmd))
	{
		return true;
	}

	if (g_VoteManager.ClientCommand(pPlayer->entindex(), pcmd))
	{
		return true;
	}

	if (strcmp(pcmd, "jointeam") == 0)
	{
		if (CMD_ARGC() > 1)
		{
			auto teamIndex = atoi(CMD_ARGV(1));
			bool autoTeam = false;

			if (teamIndex == 5)
			{
				teamIndex = GetDefaultPlayerTeam(pPlayer);
				autoTeam = true;
			}

			if (!ChangePlayerTeam(pPlayer, teamIndex, true, false, autoTeam))
			{
				MessageBegin(MSG_ONE, gmsgVGUIMenu, pPlayer);
				WriteByte(MENU_TEAM);
				MessageEnd();
			}
		}
		return true;
	}
	else if (strcmp(pcmd, "spectate") == 0)
	{
		if (!ChangePlayerTeam(pPlayer, TEAM_SPECTATORS, true, false, false))
		{
            MessageBegin(MSG_ONE, gmsgVGUIMenu, pPlayer);
            WriteByte(MENU_TEAM);
            MessageEnd();
		}
		return true;
	}
	else if (pPlayer->IsObserver()
	 && pPlayer->v.iuser1 != OBS_DEATHCAM
	 && pPlayer->v.iuser1 != OBS_FIXED)
	{
		if (CMD_ARGC() > 1)
		{
			if (strcmp(pcmd, "specmode") == 0)
			{
				pPlayer->Observer_SetMode(atoi(CMD_ARGV(1)));
				return true;
			}
			else if (strcmp(pcmd, "follownext") == 0)
			{
				pPlayer->Observer_FindNextPlayer(atoi(CMD_ARGV(1)) != 0);
				return true;
			}
		}
	}
	else if (IsPlayerPrivileged(pPlayer) && PrivilegedCommand(pPlayer, pcmd))
	{
		return true;
	}

	return CGameRules::ClientCommand(pPlayer, pcmd);
}


bool CHalfLifeMultiplay::SayCommand(CBasePlayer* player, const int argc, const char** argv)
{
	return g_VoteManager.SayCommand(player->entindex(), argc, argv);
}


void CHalfLifeMultiplay::ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer)
{
	pPlayer->SetPrefsFromUserinfo(infobuffer);
}


void CHalfLifeMultiplay::Think()
{
	g_VoiceGameMgr.Update(gpGlobals->frametime);
	g_VoteManager.Update();

	switch (GetState())
	{
		case GR_STATE_RND_RUNNING:
			Think_RND_RUNNING();
			break;
		case GR_STATE_GAME_OVER:
			Think_GAME_OVER();
			break;
		default:
			break;
	}
}


bool CHalfLifeMultiplay::FShouldSwitchWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon)
{
	if (!pWeapon->CanDeploy())
	{
		return false;
	}

	if (pPlayer->m_pActiveWeapon == nullptr)
	{
		return true;
	}

	if (!pPlayer->m_pActiveWeapon->CanHolster())
	{
		return false;
	}

	if (pPlayer->m_iAutoWepSwitch == 0)
	{
		return false;
	}

	if (pPlayer->m_iAutoWepSwitch == 2
	 && (pPlayer->m_afButtonLast & (IN_ATTACK | IN_ATTACK2)) != 0)
	{
		return false;
	}

	if (pWeapon->iWeight() > pPlayer->m_pActiveWeapon->iWeight())
	{
		return true;
	}

	return false;
}


bool CHalfLifeMultiplay::ClientConnected(Entity* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128])
{
	g_VoiceGameMgr.ClientConnected(pEntity);
	g_VoteManager.ClientConnected(ENTINDEX(pEntity));
	return true;
}


void CHalfLifeMultiplay::ClientPutInServer(CBasePlayer* pPlayer)
{
	pPlayer->m_team = nullptr;
	pPlayer->v.team = TEAM_UNASSIGNED;
	pPlayer->v.playerclass = PC_UNDEFINED;
	pPlayer->m_fDeadTime = gpGlobals->time - 60.0F;
}


void CHalfLifeMultiplay::InitHUD(CBasePlayer* pl)
{
	const char *name = "unconnected";
	
	if (!FStringNull(pl->v.netname) && STRING(pl->v.netname)[0] != '\0')
	{
		name = STRING(pl->v.netname);
	}

	// notify other clients of player joining the game
	util::ClientPrintAll(HUD_PRINTTALK, "#Game_connected", name);

	util::LogPrintf("\"%s<%i><%s><>\" connected\n",
		name,
		g_engfuncs.pfnGetPlayerUserId(pl->edict()),
		g_engfuncs.pfnGetPlayerAuthId(pl->edict()));

	CGameRules::InitHUD(pl);

	// Send down the team names
	MessageBegin(MSG_ONE, gmsgTeamNames, pl);
	WriteByte(m_numTeams);
	for (auto t = m_teams.begin(); t != m_teams.end(); t++)
	{
		WriteString(("#" + (*t).m_name).c_str());
	}
	MessageEnd();

	MessageBegin(MSG_ONE, gmsgAllowSpec, pl);
	WriteByte(AllowSpectators() ? 1 : 0);
	MessageEnd();

	// sending just one score makes the hud scoreboard active;  otherwise
	// it is just disabled for single play
	MessageBegin(MSG_ONE, gmsgScoreInfo, pl);
	WriteByte(pl->entindex());
	WriteShort(0);
	WriteShort(0);
	MessageEnd();

	pl->SendExtraInfo(pl);

	SendMOTDToClient(pl);

	// loop through all active players and send their score info to the new client
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer* plr = (CBasePlayer*)util::PlayerByIndex(i);

		if (plr)
		{
			MessageBegin(MSG_ONE, gmsgScoreInfo, pl);
			WriteByte(i); // client number
			WriteShort(plr->v.frags);
			WriteShort(plr->m_iDeaths);
			MessageEnd();

			plr->SendExtraInfo(pl);
			pl->SendExtraInfo(plr);
		}
	}

	if (GetState() == GR_STATE_GAME_OVER)
	{
		MessageBegin(MSG_ONE, SVC_INTERMISSION, pl);
		MessageEnd();
	}

	SendMenusToClient(pl);
}


void CHalfLifeMultiplay::ClientDisconnected(Entity* pClient)
{
	if (pClient)
	{
		g_VoteManager.ClientDisconnected(ENTINDEX(pClient));

		const char *name = "unconnected";
		
		if (!FStringNull(pClient->netname) && STRING(pClient->netname)[0] != '\0')
		{
			name = STRING(pClient->netname);
		}
		
		util::ClientPrintAll(HUD_PRINTTALK, "#Game_disconnected", name);

		CBasePlayer* pPlayer = pClient->Get<CBasePlayer>();

		if (pPlayer)
		{
			if (pPlayer->m_team)
			{
				pPlayer->m_team->RemovePlayer(pPlayer);
				pPlayer->m_team = nullptr;
			}

			util::FireTargets("game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0);

			util::LogPrintf("\"%s<%i><%s><>\" disconnected\n",
				STRING(pPlayer->v.netname),
				g_engfuncs.pfnGetPlayerUserId(pPlayer->edict()),
				g_engfuncs.pfnGetPlayerAuthId(pPlayer->edict()));

			pPlayer->RemoveAllWeapons(); // destroy all of the players weapons and items
		}

		pClient->team = TEAM_UNASSIGNED;
		pClient->playerclass = PC_UNDEFINED;
		pClient->netname = MAKE_STRING("Disconnected");
	}
}


float CHalfLifeMultiplay::FlPlayerFallDamage(CBasePlayer* pPlayer)
{
	int iFallDamage = (int)falldamage.value;

	switch (iFallDamage)
	{
	case 1: //progressive
		pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
		return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
		break;
	default:
	case 0: // fixed
		return 10;
		break;
	}
}


void CHalfLifeMultiplay::PlayerThink(CBasePlayer* pPlayer)
{
	if (GetState() == GR_STATE_GAME_OVER)
	{
		// clear attack/use commands from player
		pPlayer->m_afButtonPressed = 0;
		pPlayer->v.button = 0;
		pPlayer->m_afButtonReleased = 0;
	}
}


void CHalfLifeMultiplay::PlayerSpawn(CBasePlayer* pPlayer)
{
	if (pPlayer->TeamNumber() == TEAM_UNASSIGNED)
	{
		pPlayer->m_hObserverTarget = nullptr;
		pPlayer->m_iObserverLastMode = OBS_FIXED;
		pPlayer->StartObserver();
		pPlayer->v.iuser1 = OBS_FIXED;
		pPlayer->v.iuser2 = 0;
		pPlayer->v.iuser3 = 0;
		return;
	}

	pPlayer->SendExtraInfo();

	bool addDefault;
	CBaseEntity* pWeaponEntity = NULL;

	//Ensure the player switches to the Glock on spawn regardless of setting
	const int originalAutoWepSwitch = pPlayer->m_iAutoWepSwitch;
	pPlayer->m_iAutoWepSwitch = 1;

	addDefault = true;

	while (pWeaponEntity = util::FindEntityByClassname(pWeaponEntity, "game_player_equip"))
	{
		pWeaponEntity->Touch(pPlayer);
		addDefault = false;
	}

	if (addDefault)
	{
		pPlayer->GiveNamedItem("weapon_crowbar");
		pPlayer->GiveNamedItem("weapon_9mmAR");

		for (int i = 0; i < AMMO_TYPES; i++)
		{
			pPlayer->m_rgAmmo[i] = GetMaxAmmo(pPlayer, i);
		}
	}

	pPlayer->m_iAutoWepSwitch = originalAutoWepSwitch;

	tent::TeleportSplash(pPlayer);
}


bool CHalfLifeMultiplay::FPlayerCanRespawn(CBasePlayer* pPlayer)
{
	return FlPlayerSpawnTime(pPlayer) <= gpGlobals->time;
}


float CHalfLifeMultiplay::FlPlayerSpawnTime(CBasePlayer* pPlayer)
{
	return pPlayer->m_fDeadTime + 3.0f;
}


bool CHalfLifeMultiplay::FPlayerCanSuicide(CBasePlayer* pPlayer)
{
	return pPlayer->m_fNextSuicideTime <= gpGlobals->time;
}


float CHalfLifeMultiplay::GetPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled, bool assist)
{
	if (pAttacker == pKilled)
	{
		return 0;
	}
	if (assist)
	{
		return 0.5F;
	}
	return 1;
}


void CHalfLifeMultiplay::PlayerKilled(CBasePlayer* pVictim, CBaseEntity* killer, CBaseEntity* inflictor, CBaseEntity* accomplice, int bitsDamageType)
{
	DeathNotice(pVictim, killer, inflictor, accomplice, bitsDamageType);

	pVictim->m_iDeaths += 1;

	util::FireTargets("game_playerdie", pVictim, pVictim, USE_TOGGLE, 0);

	if (killer->IsClient())
	{
		// if a player dies in a deathmatch game and the killer is a client, award the killer some points
		g_pGameRules->AddPointsToPlayer(
			dynamic_cast<CBasePlayer*>(killer),
			GetPointsForKill(dynamic_cast<CBasePlayer*>(killer), pVictim),
			false);

		if (accomplice != nullptr && accomplice->IsClient())
		{
			g_pGameRules->AddPointsToPlayer(
				dynamic_cast<CBasePlayer*>(accomplice),
				GetPointsForKill(dynamic_cast<CBasePlayer*>(accomplice), pVictim, true),
				false);
		}

		if (pVictim != killer)
		{
			util::FireTargets("game_playerkill", killer, killer, USE_TOGGLE, 0);
		}
	}

	// update the scores
	// killed scores
	MessageBegin(MSG_ALL, gmsgScoreInfo);
	WriteByte(pVictim->entindex());
	WriteShort(pVictim->v.frags);
	WriteShort(pVictim->m_iDeaths);
	MessageEnd();

	// killers score, if it's a player
	if (killer->IsClient())
	{
		MessageBegin(MSG_ALL, gmsgScoreInfo);
		WriteByte(killer->entindex());
		WriteShort(killer->v.frags);
		WriteShort(((CBasePlayer *)killer)->m_iDeaths);
		MessageEnd();

		// let the killer paint another decal as soon as they'd like.
		((CBasePlayer *)killer)->m_flNextDecalTime = -decalfrequency.value;
	}

	pVictim->SendExtraInfo();
}


void CHalfLifeMultiplay::DeathNotice(CBasePlayer* pVictim, CBaseEntity* killer, CBaseEntity* inflictor, CBaseEntity* accomplice, int bitsDamageType)
{
	const char* killerWeapon = "world";
	int killerIndex = 0;
	int accompliceIndex = 0;

	if (killer->IsClient())
	{
		killerIndex = killer->entindex();

		if (inflictor != nullptr)
		{
			if (inflictor == killer)
			{
				// If the inflictor is the killer, then it must be their current weapon doing the damage
				if (dynamic_cast<CBasePlayer*>(killer)->m_pActiveWeapon != nullptr)
				{
					killerWeapon = dynamic_cast<CBasePlayer*>(killer)->m_pActiveWeapon->pszName();
				}
			}
			else
			{
				killerWeapon = STRING(inflictor->v.classname);
			}
		}
	}
	else
	{
		killerWeapon = STRING(inflictor->v.classname);
	}

	// strip the monster_* or weapon_* from the inflictor's classname
	auto c = strrchr(killerWeapon, '_');
	if (c)
	{
		killerWeapon = c + 1;
	}

	if (accomplice != nullptr && accomplice->IsClient())
	{
		accompliceIndex = accomplice->entindex();
	}

	int flags = kDamageFlagDead;

	if ((bitsDamageType & DMG_AIMED) != 0
	 && pVictim->m_LastHitGroup == HITGROUP_HEAD)
	{
		flags |= kDamageFlagHeadshot;
	}

	if (pVictim == killer)
	{
		flags |= kDamageFlagSelf;
	}
	else if (PlayerRelationship(pVictim, killer) == GR_TEAMMATE)
	{
		flags |= kDamageFlagFriendlyFire;
	}

	MessageBegin(MSG_ALL, gmsgDeathMsg);
	WriteByte(killerIndex);
	WriteByte(accompliceIndex);
	WriteByte(pVictim->entindex());
	WriteByte(flags);
	WriteString(killerWeapon);
	MessageEnd();

	if (pVictim == killer)
	{
		util::LogPrintf("\"%s<%i><%s><>\" committed suicide with \"%s\"\n",
			STRING(pVictim->v.netname),
			g_engfuncs.pfnGetPlayerUserId(pVictim->edict()),
			g_engfuncs.pfnGetPlayerAuthId(pVictim->edict()),
			killerWeapon);
	}
	else if (killer->IsClient())
	{
		util::LogPrintf("\"%s<%i><%s><>\" killed \"%s<%i><%s><>\" with \"%s\"\n",
			STRING(killer->v.netname),
			g_engfuncs.pfnGetPlayerUserId(killer->edict()),
			g_engfuncs.pfnGetPlayerAuthId(killer->edict()),
			STRING(killer->v.netname),
			g_engfuncs.pfnGetPlayerUserId(pVictim->edict()),
			g_engfuncs.pfnGetPlayerAuthId(pVictim->edict()),
			killerWeapon);
	}
	else
	{
		util::LogPrintf("\"%s<%i><%s><>\" died\n",
			STRING(pVictim->v.netname),
			g_engfuncs.pfnGetPlayerUserId(pVictim->edict()),
			g_engfuncs.pfnGetPlayerAuthId(pVictim->edict()),
			killerWeapon);
	}

	MessageBegin(MSG_SPEC, SVC_DIRECTOR);
	WriteByte(9);							 // command length in bytes
	WriteByte(DRC_CMD_EVENT);				 // player killed
	WriteShort(pVictim->entindex()); // index number of primary entity
	WriteShort(inflictor->entindex()); // index number of secondary entity
	WriteLong(7 | DRC_FLAG_DRAMATIC);		 // eventflags (priority and flags)
	MessageEnd();
}


void CHalfLifeMultiplay::AddPointsToPlayer(CBasePlayer* player, float score, bool allowNegative)
{
	// Positive score always adds
	if (score <= 0 && !allowNegative)
	{
		if (player->v.frags <= 0) // Can't go more negative
		{
			return;
		}

		if (-score > player->v.frags) // Will this go negative?
		{
			score = -player->v.frags; // Sum will be 0
		}
	}

	player->v.frags += score;

	MessageBegin(MSG_ALL, gmsgScoreInfo);
	WriteByte(player->entindex());
	WriteShort(player->v.frags);
	WriteShort(player->m_iDeaths);
	MessageEnd();
}


void CHalfLifeMultiplay::AddPointsToTeam(int teamIndex, float score, bool allowNegative)
{
	for (auto t = m_teams.begin(); t != m_teams.end(); t++)
	{
		if ((*t).m_index == teamIndex)
		{
			(*t).AddPoints(score, allowNegative);
			break;
		}
	}
}


void CHalfLifeMultiplay::PlayerGotWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon)
{
}


float CHalfLifeMultiplay::FlWeaponRespawnTime(CBasePlayerWeapon* pWeapon)
{
	if (weaponstay.value > 0)
	{
		// make sure it's only certain weapons
		if ((pWeapon->iFlags() & WEAPON_FLAG_LIMITINWORLD) == 0)
		{
			return gpGlobals->time + 0; // weapon respawns almost instantly
		}
	}

	return gpGlobals->time + WEAPON_RESPAWN_TIME;
}


// when we are within this close to running out of entities,  items
// marked with the WEAPON_FLAG_LIMITINWORLD will delay their respawn
#define ENTITY_INTOLERANCE 100

float CHalfLifeMultiplay::FlWeaponTryRespawn(CBasePlayerWeapon* pWeapon)
{
	if (pWeapon && (pWeapon->iFlags() & WEAPON_FLAG_LIMITINWORLD) != 0)
	{
		if (NUMBER_OF_ENTITIES() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE))
			return 0;

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime(pWeapon);
	}

	return 0;
}


Vector CHalfLifeMultiplay::VecWeaponRespawnSpot(CBasePlayerWeapon* pWeapon)
{
	return pWeapon->v.origin;
}


int CHalfLifeMultiplay::WeaponShouldRespawn(CBasePlayerWeapon* pWeapon)
{
	if ((pWeapon->v.spawnflags & SF_NORESPAWN) != 0)
	{
		return GR_WEAPON_RESPAWN_NO;
	}

	return GR_WEAPON_RESPAWN_YES;
}


bool CHalfLifeMultiplay::CanHavePlayerWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon)
{
	if (weaponstay.value > 0)
	{
		if ((pWeapon->iFlags() & WEAPON_FLAG_LIMITINWORLD) != 0)
		{
			return CGameRules::CanHavePlayerWeapon(pPlayer, pWeapon);
		}

		if (pPlayer->HasPlayerWeapon(pWeapon->GetID()))
		{
			return false;
		}
	}

	return CGameRules::CanHavePlayerWeapon(pPlayer, pWeapon);
}


int CHalfLifeMultiplay::ItemShouldRespawn(CItem* pItem)
{
	if ((pItem->v.spawnflags & SF_NORESPAWN) != 0)
	{
		return GR_ITEM_RESPAWN_NO;
	}

	return GR_ITEM_RESPAWN_YES;
}


float CHalfLifeMultiplay::FlItemRespawnTime(CItem* pItem)
{
	return gpGlobals->time + ITEM_RESPAWN_TIME;
}


Vector CHalfLifeMultiplay::VecItemRespawnSpot(CItem* pItem)
{
	return pItem->v.origin;
}


bool CHalfLifeMultiplay::IsAllowedToSpawn(CBaseEntity* pEntity)
{
	if (!FAllowMonsters() && (pEntity->v.flags & FL_MONSTER) != 0)
	{
		return false;
	}
	return true;
}


int CHalfLifeMultiplay::AmmoShouldRespawn(CBasePlayerAmmo* pAmmo)
{
	if ((pAmmo->v.spawnflags & SF_NORESPAWN) != 0)
	{
		return GR_AMMO_RESPAWN_NO;
	}

	return GR_AMMO_RESPAWN_YES;
}


float CHalfLifeMultiplay::FlAmmoRespawnTime(CBasePlayerAmmo* pAmmo)
{
	return gpGlobals->time + AMMO_RESPAWN_TIME;
}


Vector CHalfLifeMultiplay::VecAmmoRespawnSpot(CBasePlayerAmmo* pAmmo)
{
	return pAmmo->v.origin;
}


bool CHalfLifeMultiplay::IsSpawnSpotValid(CSpawnPoint *pSpawn, CBasePlayer *pPlayer, int attempt)
{
	if (!CGameRules::IsSpawnSpotValid(pSpawn, pPlayer, attempt))
	{
		return false;
	}
	return pSpawn->IsValid(pPlayer, attempt);
}


CSpawnPoint *CHalfLifeMultiplay::GetPlayerSpawnSpot(CBasePlayer* pPlayer)
{
	std::size_t numValid;
	std::size_t i;
	int attempt;

	/* Each successive attempt should have more lenient conditions. */
	for (attempt = 0; attempt < 3; attempt++)
	{
		numValid = 0;
		
		for (i = 0; i < m_numSpawnPoints; i++)
		{
			if (IsSpawnSpotValid(m_spawnPoints[i], pPlayer, attempt))
			{
				m_validSpawnPoints[numValid] = m_spawnPoints[i];
				numValid++;
			}
		}
		
		if (numValid != 0)
		{
			break;
		}
	}

	/*
	Default to the singleplayer spawn point.
	This should only happen in a dire situation.
	*/
	if (numValid == 0)
	{
		return CGameRules::GetPlayerSpawnSpot(pPlayer);
	}

	auto index = g_engfuncs.pfnRandomLong(0, numValid - 1);
	auto spawn = m_validSpawnPoints[index];

	spawn->m_lastSpawnTime = gpGlobals->time;

	/* Fire targets, if any. */
	if (!FStringNull(spawn->m_target))
	{
		util::FireTargets(STRING(spawn->m_target), pPlayer, pPlayer, USE_TOGGLE, 0);
	}

	/* Telefrag! */
	CBaseEntity *entity = nullptr;
	while ((entity = util::FindEntityInSphere(entity, spawn->m_origin, 128.0F)) != nullptr)
	{
		if (entity->IsPlayer() && entity != pPlayer)
		{
			if (FPlayerCanTakeDamage((CBasePlayer *)entity, pPlayer))
			{
				entity->Killed(CWorld::World, CWorld::World, DMG_ALWAYSGIB);
			}
		}
	}

	return spawn;
}


void CHalfLifeMultiplay::AddPlayerSpawnSpot(CBaseEntity *pEntity)
{
	if (FStrEq(STRING(pEntity->v.classname), "info_player_start"))
	{
		CGameRules::AddPlayerSpawnSpot(pEntity);
		return;
	}

	auto spawn = new CSpawnPoint {pEntity};

#if 0
	ALERT(
		at_aiconsole,
		"%s %lu at (%g, %g, %g)\n",
		STRING(pEntity->v.classname),
		m_numSpawnPoints,
		spawn->m_origin.x,
		spawn->m_origin.y,
		spawn->m_origin.z);
#endif
	
	m_spawnPoints.push_back(spawn);
	
	m_numSpawnPoints++;
	m_validSpawnPoints.reserve(m_numSpawnPoints);
}


int CHalfLifeMultiplay::PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget)
{
	// half life deathmatch has only enemies
	return GR_NOTTEAMMATE;
}


int CHalfLifeMultiplay::GetTeamIndex(const char* pTeamName)
{
	auto i = 0;
	for (auto t = m_teams.begin(); t != m_teams.end(); t++)
	{
		if (strcmp(pTeamName, (*t).m_name.c_str()) == 0)
		{
			return i + 1;
		}
		i++;
	}

	if (AllowSpectators() && strcmp(pTeamName, "spectators") == 0)
	{
		return TEAM_SPECTATORS;
	}

	return TEAM_UNASSIGNED;
}


const char* CHalfLifeMultiplay::GetIndexedTeamName(int teamIndex)
{
	if (AllowSpectators() && teamIndex == TEAM_SPECTATORS)
	{
		return "spectators";
	}

	if (teamIndex <= TEAM_UNASSIGNED || teamIndex > m_numTeams)
	{
		return "";
	}

	return m_teams[teamIndex - 1].m_name.c_str();
}


bool CHalfLifeMultiplay::IsValidTeam(const char* pTeamName)
{
	if (AllowSpectators() && strcmp(pTeamName, "spectators") == 0)
	{
		return true;
	}

	return GetTeamIndex(pTeamName) > TEAM_UNASSIGNED;
}


int CHalfLifeMultiplay::GetDefaultPlayerTeam(CBasePlayer* pPlayer)
{
	if (m_numTeams == 1)
	{
		return TEAM_DEFAULT;
	}

	int minPlayers = gpGlobals->maxClients + 1;
	int candidates[TEAM_SPECTATORS - 1];
	int numCandidates = 0;
	bool currentTeamIsCandidate = false;

	for (int i = 0; i < m_numTeams; i++)
	{
		if (!CanJoinTeam(pPlayer, i + 1))
		{
			continue;
		}

		/* The player is on this team */
		bool currentTeam = (pPlayer->TeamNumber() == (i + 1));
		int numPlayers = m_teams[i].m_numPlayers;

		/* Subtract one to account for the player leaving */
		if (currentTeam)
		{
			numPlayers--;
		}

		if (numPlayers == minPlayers)
		{
			/* Matches minimum, add to the list */
			candidates[numCandidates] = i + 1;
			numCandidates++;

			if (currentTeam)
			{
				currentTeamIsCandidate = true;
			}
		}
		else if (numPlayers < minPlayers)
		{
			/* New minimum, reset the list & put this team first */
			minPlayers = numPlayers;

			candidates[0] = i + 1;
			numCandidates = 1;
			currentTeamIsCandidate = currentTeam;
		}
	}

	if (currentTeamIsCandidate || numCandidates == 0)
	{
		return pPlayer->TeamNumber();
	}

	/* Choose a team */
	return candidates[g_engfuncs.pfnRandomLong(1, numCandidates) - 1];
}


bool CHalfLifeMultiplay::CanJoinTeam(CBasePlayer* player, int teamIndex)
{
	if (teamIndex == TEAM_SPECTATORS)
	{
		if (!AllowSpectators())
		{
			return false;
		}
	}
	else if (teamIndex <= TEAM_UNASSIGNED || teamIndex > m_numTeams)
	{
		return false;
	}

	return true;
}


bool CHalfLifeMultiplay::ChangePlayerTeam(CBasePlayer* pPlayer, int teamIndex, bool bKill, bool bGib, bool bAutoTeam)
{
	if (teamIndex == pPlayer->TeamNumber())
	{
		return true;
	}

	if (!bAutoTeam && !CanJoinTeam(pPlayer, teamIndex))
	{
		return false;
	}

	if (!pPlayer->IsPlayer() || !pPlayer->IsAlive())
	{
		bKill = false;
	}

	if (bKill)
	{
		if (!g_pGameRules->FPlayerCanSuicide(pPlayer))
		{
			return false;
		}

		pPlayer->Killed(
			CWorld::World,
			CWorld::World,
			bGib ? DMG_ALWAYSGIB : DMG_GENERIC);
	}

	pPlayer->v.team = teamIndex;

	if (teamIndex != TEAM_SPECTATORS)
	{
		m_teams[pPlayer->v.team - 1].AddPlayer(pPlayer);

		if (!bKill && g_pGameRules->FPlayerCanRespawn(pPlayer))
		{
			pPlayer->Spawn();
		}
	}
	else
	{
		m_spectators.AddPlayer(pPlayer);

		pPlayer->StartObserver();
	}

	const char* msg;
	if (m_numTeams > 1 || teamIndex == TEAM_SPECTATORS)
	{
		msg = bAutoTeam ? "#Game_auto_team" : "#Game_join_team";
	}
	else
	{
		msg = "#Game_join";
	}

	std::string localizeName =
		std::string{"#"} + GetIndexedTeamName(teamIndex);

	util::ClientPrintAll(
		HUD_PRINTTALK,
		msg,
		STRING(pPlayer->v.netname),
		localizeName.c_str());
	
	util::LogPrintf("\"%s<%i><%s><>\" joined team %s\n",
		STRING(pPlayer->v.netname),
		g_engfuncs.pfnGetPlayerUserId(pPlayer->edict()),
		g_engfuncs.pfnGetPlayerAuthId(pPlayer->edict()),
		GetIndexedTeamName(teamIndex));

	return true;
}


bool CHalfLifeMultiplay::ChangePlayerTeam(CBasePlayer* pPlayer, const char* pTeamName, bool bKill, bool bGib, bool bAutoTeam)
{
	return ChangePlayerTeam(pPlayer, GetTeamIndex(pTeamName), bKill, bGib, bAutoTeam);
}


void CHalfLifeMultiplay::EndMultiplayerGame(float intermissionTime)
{
	m_intermissionTime = intermissionTime;
	EnterState(GR_STATE_GAME_OVER);
}


float CHalfLifeMultiplay::GetMapTimeLeft()
{
	if (timelimit.value <= 0.0F)
	{
		return -1.0F;
	}

	const auto timeLimit = timelimit.value * 60.0F;

	return std::max(m_stateChangeTime + timeLimit - gpGlobals->time, 0.0F);
}


void CHalfLifeMultiplay::EnterState(gamerules_state_e state)
{
	CGameRules::EnterState(state);

	m_stateChangeTime = gpGlobals->time;

	switch (GetState())
	{
		case GR_STATE_RND_RUNNING:
			Enter_RND_RUNNING();
			break;
		case GR_STATE_GAME_OVER:
			Enter_GAME_OVER();
			break;
		default:
			break;
	}
}


void CHalfLifeMultiplay::CheckTimeLimit()
{
	const auto timeLimit = timelimit.value * 60.0F;

	if (timeLimit <= 0.0F
	 || m_stateChangeTime + timeLimit > gpGlobals->time)
	{
		return;
	}

	EnterState(GR_STATE_GAME_OVER);
}


void CHalfLifeMultiplay::Enter_RND_RUNNING()
{
}


void CHalfLifeMultiplay::Think_RND_RUNNING()
{
	if ((int)fraglimit.value > 0)
	{
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBaseEntity* player = util::PlayerByIndex(i);

			if (player == nullptr)
			{
				continue;
			}

			if ((int)player->v.frags >= (int)fraglimit.value)
			{
				EnterState(GR_STATE_GAME_OVER);
				return;
			}
		}
	}

	CheckTimeLimit();
}


void CHalfLifeMultiplay::Enter_GAME_OVER()
{
	if (m_intermissionTime <= 0.0F)
	{
		m_intermissionTime = mp_chattime.value;
	}

	MessageBegin(MSG_ALL, SVC_INTERMISSION);
	MessageEnd();
}


void CHalfLifeMultiplay::Think_GAME_OVER()
{
	if (m_stateChangeTime + m_intermissionTime <= gpGlobals->time)
	{
		ChangeLevel();
	}
}


#define MAX_MOTD_CHUNK 60
#define MAX_MOTD_LENGTH 1536

void CHalfLifeMultiplay::SendMOTDToClient(CBasePlayer* player)
{
	// read from the MOTD.txt file
	int length, char_count = 0;
	char* pFileList;
	char* aFileList = pFileList = (char*)LOAD_FILE_FOR_ME((char*)CVAR_GET_STRING("motdfile"), &length);

	// send the server name
	MessageBegin(MSG_ONE, gmsgServerName, player);
	WriteString(CVAR_GET_STRING("hostname"));
	WriteString(STRING(gpGlobals->mapname));
	MessageEnd();

	if (pFileList == nullptr)
	{
		MessageBegin(MSG_ONE, gmsgMOTD, player);
		WriteByte(2);
		MessageEnd();
	}

	// Send the message of the day
	// read it chunk-by-chunk,  and send it in parts

	while (pFileList && '\0' != *pFileList && char_count < MAX_MOTD_LENGTH)
	{
		char chunk[MAX_MOTD_CHUNK + 1];

		if (strlen(pFileList) < MAX_MOTD_CHUNK)
		{
			strcpy(chunk, pFileList);
		}
		else
		{
			strncpy(chunk, pFileList, MAX_MOTD_CHUNK);
			chunk[MAX_MOTD_CHUNK] = 0; // strncpy doesn't always append the null terminator
		}

		char_count += strlen(chunk);
		if (char_count < MAX_MOTD_LENGTH)
			pFileList = aFileList + char_count;
		else
			*pFileList = 0;

		MessageBegin(MSG_ONE, gmsgMOTD, player);
		WriteByte(static_cast<int>('\0' == *pFileList)); // false means there is still more message to come
		WriteString(chunk);
		MessageEnd();
	}

	FREE_FILE(aFileList);
}


void CHalfLifeMultiplay::SendMenusToClient(CBasePlayer* player)
{
	MessageBegin(MSG_ONE, gmsgVGUIMenu, player);
	WriteByte(MENU_MAPBRIEFING);
	MessageEnd();
}

