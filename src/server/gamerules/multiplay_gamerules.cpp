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
#include "hltv.h"
#include "UserMessages.h"

#define ITEM_RESPAWN_TIME 30
#define WEAPON_RESPAWN_TIME 20
#define AMMO_RESPAWN_TIME 20

CVoiceGameMgr g_VoiceGameMgr;

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

	MessageBegin(MSG_ALL, gmsgExtraInfo);
	WriteByte(player->entindex());
	WriteByte(player->PCNumber());
	WriteByte(player->TeamNumber());
	WriteByte(player->IsPlayer() && !player->IsAlive());
	MessageEnd();

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

void CTeam::AddPoints(float score)
{
	if (score < 0)
	{
		return;
	}

	m_score += score;

	MessageBegin(MSG_ALL, gmsgTeamScore);
	WriteByte(m_index);
	WriteShort(m_score);
	MessageEnd();
}

//*********************************************************
// Rules for the half-life multiplayer game.
//*********************************************************

CHalfLifeMultiplay::CHalfLifeMultiplay()
{
	g_VoiceGameMgr.Init(&g_GameMgrHelper, gpGlobals->maxClients);

	m_deathmatch = (int)gpGlobals->deathmatch != 0;
	m_coop = (int)gpGlobals->coop != 0;
	m_allowMonsters = (int)allowmonsters.value != 0;
	m_allowSpectators = (int)allow_spectators.value != 0;

	m_teams.clear();
	m_teams.push_back(CTeam{TEAM_DEFAULT, "players"});
	m_numTeams = 1;

	RefreshSkillData();

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
}

bool CHalfLifeMultiplay::PrivilegedCommand(CBasePlayer* pPlayer, const char* pcmd)
{
	if (strcmp(pcmd, "nc") == 0)
	{
		if (pPlayer->pev->movetype != MOVETYPE_NOCLIP)
		{
			pPlayer->pev->movetype = MOVETYPE_NOCLIP;
			g_engfuncs.pfnClientPrintf(pPlayer->edict(), print_console, "noclip ON\n");
		}
		else
		{
			pPlayer->pev->movetype = MOVETYPE_WALK;
			g_engfuncs.pfnClientPrintf(pPlayer->edict(), print_console, "noclip OFF\n");
		}
		return true;
	}
	else if (strcmp(pcmd, "gaben") == 0)
	{
		if ((pPlayer->pev->flags & FL_GODMODE) == 0)
		{
			pPlayer->pev->flags |= FL_GODMODE;
			g_engfuncs.pfnClientPrintf(pPlayer->edict(), print_console, "godmode ON\n");
		}
		else
		{
			pPlayer->pev->flags &= ~FL_GODMODE;
			g_engfuncs.pfnClientPrintf(pPlayer->edict(), print_console, "godmode OFF\n");
		}
		return true;
	}
	else if (strcmp(pcmd, "nt") == 0)
	{
		if ((pPlayer->pev->flags & FL_NOTARGET) == 0)
		{
			pPlayer->pev->flags |= FL_NOTARGET;
			g_engfuncs.pfnClientPrintf(pPlayer->edict(), print_console, "notarget ON\n");
		}
		else
		{
			pPlayer->pev->flags &= ~FL_NOTARGET;
			g_engfuncs.pfnClientPrintf(pPlayer->edict(), print_console, "notarget OFF\n");
		}
		return true;
	}

	return false;
}

bool CHalfLifeMultiplay::ClientCommand(CBasePlayer* pPlayer, const char* pcmd)
{
	if (g_VoiceGameMgr.ClientCommand(pPlayer, pcmd))
		return true;

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

			ChangePlayerTeam(pPlayer, teamIndex, true, false, autoTeam);
		}
		return true;
	}
	else if (strcmp(pcmd, "spectate") == 0)
	{
		ChangePlayerTeam(pPlayer, TEAM_SPECTATORS, true, false, false);
		return true;
	}
	else if (pPlayer->IsObserver())
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

void CHalfLifeMultiplay::ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer)
{
	pPlayer->SetPrefsFromUserinfo(infobuffer);
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::RefreshSkillData()
{
	// load all default values
	CGameRules::RefreshSkillData();

	// override some values for deathmatch
	if (!IsDeathmatch())
	{
		return;
	}
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::Think()
{
	g_VoiceGameMgr.Update(gpGlobals->frametime);

	///// Check game rules /////
	static int last_frags;
	static int last_time;

	int frags_remaining = 0;
	int time_remaining = 0;

	if (g_fGameOver)
	{
		if (m_flIntermissionTime + mp_chattime.value <= gpGlobals->time)
		{
			ChangeLevel();
		}
		return;
	}

	float flTimeLimit = timelimit.value * 60;
	float flFragLimit = fraglimit.value;

	time_remaining = (int)(0 != flTimeLimit ? (flTimeLimit - gpGlobals->time) : 0);

	if (flTimeLimit != 0 && gpGlobals->time >= flTimeLimit)
	{
		GoToIntermission();
		return;
	}

	if (0 != flFragLimit)
	{
		int bestfrags = 9999;
		int remain;

		// check if any player is over the frag limit
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBaseEntity* pPlayer = util::PlayerByIndex(i);

			if (pPlayer && pPlayer->pev->frags >= flFragLimit)
			{
				GoToIntermission();
				return;
			}


			if (pPlayer)
			{
				remain = flFragLimit - pPlayer->pev->frags;
				if (remain < bestfrags)
				{
					bestfrags = remain;
				}
			}
		}
		frags_remaining = bestfrags;
	}

	last_frags = frags_remaining;
	last_time = time_remaining;
}

//=========================================================
//=========================================================
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

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::ClientConnected(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128])
{
	g_VoiceGameMgr.ClientConnected(pEntity);
	return true;
}

void CHalfLifeMultiplay::ClientPutInServer(CBasePlayer* pPlayer)
{
	pPlayer->m_team = nullptr;
	pPlayer->pev->team = TEAM_UNASSIGNED;
}

void CHalfLifeMultiplay::InitHUD(CBasePlayer* pl)
{
	const char *name = "unconnected";
	
	if (!FStringNull(pl->pev->netname) && STRING(pl->pev->netname)[0] != '\0')
	{
		name = STRING(pl->pev->netname);
	}

	// notify other clients of player joining the game
	util::ClientPrintAll(HUD_PRINTTALK, "#Game_connected", name);

	// team match?
	if (IsTeamplay())
	{
		util::LogPrintf("\"%s<%i><%s><%s>\" entered the game\n",
			name,
			GETPLAYERUSERID(pl->edict()),
			GETPLAYERAUTHID(pl->edict()),
			g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pl->edict()), "model"));
	}
	else
	{
		util::LogPrintf("\"%s<%i><%s><%i>\" entered the game\n",
			name,
			GETPLAYERUSERID(pl->edict()),
			GETPLAYERAUTHID(pl->edict()),
			GETPLAYERUSERID(pl->edict()));
	}

	CGameRules::InitHUD(pl);

	// Send down the team names
	MessageBegin(MSG_ONE, gmsgTeamNames, pl);
	WriteByte(m_numTeams);
	for (auto t = m_teams.begin(); t != m_teams.end(); t++)
	{
		WriteString((*t).m_name.c_str());
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

	MessageBegin(MSG_ONE, gmsgExtraInfo, pl);
	WriteByte(pl->entindex());
	WriteByte(PC_UNDEFINED);
	WriteByte(TEAM_UNASSIGNED);
	WriteByte(false);
	MessageEnd();

	SendMOTDToClient(pl);

	// loop through all active players and send their score info to the new client
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		// FIXME:  Probably don't need to cast this just to read m_iDeaths
		CBasePlayer* plr = (CBasePlayer*)util::PlayerByIndex(i);

		if (plr)
		{
			MessageBegin(MSG_ONE, gmsgScoreInfo, pl);
			WriteByte(i); // client number
			WriteShort(plr->pev->frags);
			WriteShort(plr->m_iDeaths);
			MessageEnd();

			MessageBegin(MSG_ONE, gmsgExtraInfo, pl);
			WriteByte(i);
			WriteByte(plr->PCNumber());
			WriteByte(plr->TeamNumber());
			WriteByte(plr->IsPlayer() && !plr->IsAlive());
			MessageEnd();
		}
	}

	if (g_fGameOver)
	{
		MessageBegin(MSG_ONE, SVC_INTERMISSION, pl);
		MessageEnd();
	}

	if (!IsTeamplay())
	{
		MessageBegin(MSG_ONE, gmsgVGUIMenu, pl);
		WriteByte(MENU_MAPBRIEFING);
		MessageEnd();
	}
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::ClientDisconnected(edict_t* pClient)
{
	if (pClient)
	{
		const char *name = "unconnected";
		
		if (!FStringNull(pClient->v.netname) && STRING(pClient->v.netname)[0] != '\0')
		{
			name = STRING(pClient->v.netname);
		}
		
		util::ClientPrintAll(HUD_PRINTTALK, "#Game_disconnected", name);

		CBasePlayer* pPlayer = (CBasePlayer*)CBaseEntity::Instance(pClient);

		if (pPlayer)
		{
			if (pPlayer->m_team)
			{
				pPlayer->m_team->RemovePlayer(pPlayer);
				pPlayer->m_team = nullptr;
			}

			util::FireTargets("game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0);

			// team match?
			if (IsTeamplay())
			{
				util::LogPrintf("\"%s<%i><%s><%s>\" disconnected\n",
					STRING(pPlayer->pev->netname),
					GETPLAYERUSERID(pPlayer->edict()),
					GETPLAYERAUTHID(pPlayer->edict()),
					g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model"));
			}
			else
			{
				util::LogPrintf("\"%s<%i><%s><%i>\" disconnected\n",
					STRING(pPlayer->pev->netname),
					GETPLAYERUSERID(pPlayer->edict()),
					GETPLAYERAUTHID(pPlayer->edict()),
					GETPLAYERUSERID(pPlayer->edict()));
			}

			pPlayer->RemoveAllWeapons(); // destroy all of the players weapons and items
		}
	}
}

//=========================================================
//=========================================================
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

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker)
{
	return true;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerThink(CBasePlayer* pPlayer)
{
	if (g_fGameOver)
	{
		// clear attack/use commands from player
		pPlayer->m_afButtonPressed = 0;
		pPlayer->pev->button = 0;
		pPlayer->m_afButtonReleased = 0;
	}
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerSpawn(CBasePlayer* pPlayer)
{
	MessageBegin(MSG_ALL, gmsgExtraInfo);
	WriteByte(pPlayer->entindex());
	WriteByte(pPlayer->PCNumber());
	WriteByte(pPlayer->TeamNumber());
	WriteByte(pPlayer->IsPlayer() && !pPlayer->IsAlive());
	MessageEnd();

	if (pPlayer->TeamNumber() == TEAM_UNASSIGNED)
	{
		pPlayer->pev->effects |= EF_NODRAW;
		pPlayer->pev->solid = SOLID_NOT;
		pPlayer->pev->takedamage = DAMAGE_NO;
		pPlayer->pev->movetype = MOVETYPE_NONE;
		pPlayer->m_iHideHUD |= HIDEHUD_WEAPONS | HIDEHUD_FLASHLIGHT | HIDEHUD_HEALTH;
		return;
	}

	pPlayer->m_iHideHUD &= ~(HIDEHUD_WEAPONS | HIDEHUD_FLASHLIGHT | HIDEHUD_HEALTH);

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
		// pPlayer->GiveAmmo(250, AMMO_9MM, 250);
		// pPlayer->GiveAmmo(10, AMMO_ARGRENADES, 10);
	}

	pPlayer->m_iAutoWepSwitch = originalAutoWepSwitch;

	tent::TeleportSplash(pPlayer);
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::FPlayerCanRespawn(CBasePlayer* pPlayer)
{
	return FlPlayerSpawnTime(pPlayer) <= gpGlobals->time;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlPlayerSpawnTime(CBasePlayer* pPlayer)
{
	return pPlayer->m_fDeadTime + 3.0f;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::FPlayerCanSuicide(CBasePlayer* pPlayer)
{
	return pPlayer->m_fNextSuicideTime <= gpGlobals->time;
}

//=========================================================
// GetPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
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


//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CHalfLifeMultiplay::PlayerKilled(CBasePlayer* pVictim, CBaseEntity* killer, CBaseEntity* inflictor, CBaseEntity* accomplice, int bitsDamageType)
{
	DeathNotice(pVictim, killer, inflictor, accomplice, bitsDamageType);

	pVictim->m_iDeaths += 1;

	util::FireTargets("game_playerdie", pVictim, pVictim, USE_TOGGLE, 0);

	if (killer->IsClient())
	{
		// if a player dies in a deathmatch game and the killer is a client, award the killer some points
		killer->AddPoints(GetPointsForKill((CBasePlayer *)killer, pVictim), false);

		if (accomplice != nullptr && accomplice->IsClient())
		{
			killer->AddPoints(GetPointsForKill((CBasePlayer *)accomplice, pVictim, true), false);
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
	WriteShort(pVictim->pev->frags);
	WriteShort(pVictim->m_iDeaths);
	MessageEnd();

	// killers score, if it's a player
	if (killer->IsClient())
	{
		MessageBegin(MSG_ALL, gmsgScoreInfo);
		WriteByte(killer->entindex());
		WriteShort(killer->pev->frags);
		WriteShort(((CBasePlayer *)killer)->m_iDeaths);
		MessageEnd();

		// let the killer paint another decal as soon as they'd like.
		((CBasePlayer *)killer)->m_flNextDecalTime = -decalfrequency.value;
	}

	MessageBegin(MSG_ALL, gmsgExtraInfo);
	WriteByte(pVictim->entindex());
	WriteByte(pVictim->PCNumber());
	WriteByte(pVictim->TeamNumber());
	WriteByte(true);
	MessageEnd();
}

//=========================================================
// Deathnotice.
//=========================================================
void CHalfLifeMultiplay::DeathNotice(CBasePlayer* pVictim, CBaseEntity* killer, CBaseEntity* inflictor, CBaseEntity* accomplice, int bitsDamageType)
{
	const char* killerWeapon = "world";
	int killerIndex = 0;
	int accompliceIndex = 0;

	if (killer->IsClient())
	{
		killerIndex = killer->entindex();

		if (inflictor)
		{
			if (inflictor == killer)
			{
				// If the inflictor is the killer, then it must be their current weapon doing the damage
				if (((CBasePlayer *)killer)->m_pActiveWeapon)
				{
					killerWeapon = ((CBasePlayer *)killer)->m_pActiveWeapon->pszName();
				}
			}
			else
			{
				killerWeapon = STRING(inflictor->pev->classname);
			}
		}
	}
	else
	{
		killerWeapon = STRING(inflictor->pev->classname);
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
		if (IsTeamplay())
		{
			util::LogPrintf("\"%s<%i><%s><%s>\" committed suicide with \"%s\"\n",
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pVictim->edict()), "model"),
				killerWeapon);
		}
		else
		{
			util::LogPrintf("\"%s<%i><%s><%i>\" committed suicide with \"%s\"\n",
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				GETPLAYERUSERID(pVictim->edict()),
				killerWeapon);
		}
	}
	else if (killer->IsClient())
	{
		if (IsTeamplay())
		{
			util::LogPrintf("\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\"\n",
				STRING(killer->pev->netname),
				GETPLAYERUSERID(killer->edict()),
				GETPLAYERAUTHID(killer->edict()),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(killer->edict()), "model"),
				STRING(killer->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pVictim->edict()), "model"),
				killerWeapon);
		}
		else
		{
			util::LogPrintf("\"%s<%i><%s><%i>\" killed \"%s<%i><%s><%i>\" with \"%s\"\n",
				STRING(killer->pev->netname),
				GETPLAYERUSERID(killer->edict()),
				GETPLAYERAUTHID(killer->edict()),
				GETPLAYERUSERID(killer->edict()),
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				GETPLAYERUSERID(pVictim->edict()),
				killerWeapon);
		}
	}
	else
	{
		if (IsTeamplay())
		{
			util::LogPrintf("\"%s<%i><%s><%s>\" committed suicide with \"%s\" (world)\n",
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pVictim->edict()), "model"),
				killerWeapon);
		}
		else
		{
			util::LogPrintf("\"%s<%i><%s><%i>\" committed suicide with \"%s\" (world)\n",
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				GETPLAYERUSERID(pVictim->edict()),
				killerWeapon);
		}
	}

	MessageBegin(MSG_SPEC, SVC_DIRECTOR);
	WriteByte(9);							 // command length in bytes
	WriteByte(DRC_CMD_EVENT);				 // player killed
	WriteShort(pVictim->entindex()); // index number of primary entity
	WriteShort(inflictor->entindex()); // index number of secondary entity
	WriteLong(7 | DRC_FLAG_DRAMATIC);		 // eventflags (priority and flags)
	MessageEnd();
}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CHalfLifeMultiplay::PlayerGotWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon)
{
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
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

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
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

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeMultiplay::VecWeaponRespawnSpot(CBasePlayerWeapon* pWeapon)
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHalfLifeMultiplay::WeaponShouldRespawn(CBasePlayerWeapon* pWeapon)
{
	if ((pWeapon->pev->spawnflags & SF_NORESPAWN) != 0)
	{
		return GR_WEAPON_RESPAWN_NO;
	}

	return GR_WEAPON_RESPAWN_YES;
}

//=========================================================
// CanHaveWeapon - returns false if the player is not allowed
// to pick up this weapon
//=========================================================
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

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::CanHaveItem(CBasePlayer* pPlayer, CItem* pItem)
{
	return true;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerGotItem(CBasePlayer* pPlayer, CItem* pItem)
{
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::ItemShouldRespawn(CItem* pItem)
{
	if ((pItem->pev->spawnflags & SF_NORESPAWN) != 0)
	{
		return GR_ITEM_RESPAWN_NO;
	}

	return GR_ITEM_RESPAWN_YES;
}


//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHalfLifeMultiplay::FlItemRespawnTime(CItem* pItem)
{
	return gpGlobals->time + ITEM_RESPAWN_TIME;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeMultiplay::VecItemRespawnSpot(CItem* pItem)
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerGotAmmo(CBasePlayer* pPlayer, char* szName, int iCount)
{
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::IsAllowedToSpawn(CBaseEntity* pEntity)
{
	if (!FAllowMonsters() && pEntity->pev->flags & FL_MONSTER)
	{
		return false;
	}
	return true;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::AmmoShouldRespawn(CBasePlayerAmmo* pAmmo)
{
	if ((pAmmo->pev->spawnflags & SF_NORESPAWN) != 0)
	{
		return GR_AMMO_RESPAWN_NO;
	}

	return GR_AMMO_RESPAWN_YES;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlAmmoRespawnTime(CBasePlayerAmmo* pAmmo)
{
	return gpGlobals->time + AMMO_RESPAWN_TIME;
}

//=========================================================
//=========================================================
Vector CHalfLifeMultiplay::VecAmmoRespawnSpot(CBasePlayerAmmo* pAmmo)
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlHealthChargerRechargeTime()
{
	return 60;
}


float CHalfLifeMultiplay::FlHEVChargerRechargeTime()
{
	return 30;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerWeapons(CBasePlayer* pPlayer)
{
	return GR_PLR_DROP_GUN_ACTIVE;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerAmmo(CBasePlayer* pPlayer)
{
	return GR_PLR_DROP_AMMO_ACTIVE;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::IsSpawnSpotValid(CSpawnPoint *pSpawn, CBasePlayer *pPlayer, int attempt)
{
	if (!CGameRules::IsSpawnSpotValid(pSpawn, pPlayer, attempt))
	{
		return false;
	}
	return pSpawn->IsValid(pPlayer, attempt);
}

//=========================================================
//=========================================================
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
			if (IsSpawnSpotValid(&m_spawnPoints[i], pPlayer, attempt))
			{
				m_validSpawnPoints[numValid] = &m_spawnPoints[i];
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
				entity->TakeDamage(pPlayer, pPlayer, 300.0F, DMG_ALWAYSGIB);
			}
		}
	}

	return spawn;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::AddPlayerSpawnSpot(CBaseEntity *pEntity)
{
	if (FStrEq(STRING(pEntity->pev->classname), "info_player_start"))
	{
		CGameRules::AddPlayerSpawnSpot(pEntity);
		return;
	}

	CSpawnPoint spawn{pEntity};

#if 0
	ALERT(
		at_aiconsole,
		"%s %lu at (%g, %g, %g)\n",
		STRING(pEntity->pev->classname),
		m_numSpawnPoints,
		spawn.m_origin.x,
		spawn.m_origin.y,
		spawn.m_origin.z);
#endif
	
	m_spawnPoints.push_back(spawn);
	
	m_numSpawnPoints++;
	m_validSpawnPoints.reserve(m_numSpawnPoints);
}

//=========================================================
//=========================================================
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

bool CHalfLifeMultiplay::ChangePlayerTeam(CBasePlayer* pPlayer, int teamIndex, bool bKill, bool bGib, bool bAutoTeam)
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

	if (teamIndex == pPlayer->TeamNumber())
	{
		return true;
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

	pPlayer->pev->team = teamIndex;

	if (teamIndex != TEAM_SPECTATORS)
	{
		m_teams[pPlayer->pev->team - 1].AddPlayer(pPlayer);

		if (!bKill)
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

	util::ClientPrintAll(
		HUD_PRINTTALK,
		msg,
		STRING(pPlayer->pev->netname),
		GetIndexedTeamName(teamIndex));

	return true;
}

bool CHalfLifeMultiplay::ChangePlayerTeam(CBasePlayer* pPlayer, const char* pTeamName, bool bKill, bool bGib, bool bAutoTeam)
{
	return ChangePlayerTeam(pPlayer, GetTeamIndex(pTeamName), bKill, bGib, bAutoTeam);
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::FAllowMonsters()
{
	return m_allowMonsters;
}

//=========================================================
//======== CHalfLifeMultiplay private functions ===========
#define INTERMISSION_TIME 6

void CHalfLifeMultiplay::GoToIntermission()
{
	if (g_fGameOver)
		return; // intermission has already been triggered, so ignore.

	MessageBegin(MSG_ALL, SVC_INTERMISSION);
	MessageEnd();

	m_flIntermissionTime = gpGlobals->time;

	g_fGameOver = true;
}

#define MAX_MOTD_CHUNK 60
#define MAX_MOTD_LENGTH 1536 // (MAX_MOTD_CHUNK * 4)

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
