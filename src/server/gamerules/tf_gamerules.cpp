//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Whatever
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "tf_gamerules.h"
#include "game.h"
#include "UserMessages.h"


static const char* sTFClassModels[] =
{
    "civilian",
    "scout",
    "sniper",
    "soldier",
    "demo",
    "medic",
    "hvyweapon",
    "pyro",
    "spy",
    "engineer",
    "civilian",
};

static const char* sTFClassSelection[] =
{
    "civilian",
    "scout",
    "sniper",
    "soldier",
    "demoman",
    "medic",
    "hwguy",
    "pyro",
    "spy",
    "engineer",
    "randompc",
    "civilian",
};


CTeamFortress::CTeamFortress()
{
    m_teams.clear();
    m_numTeams = 2;

    m_teams.push_back(CTeam{TEAM_BLUE, "Team_Blue"});
    m_teams.push_back(CTeam{TEAM_RED, "Team_Red"});
}


bool CTeamFortress::ClientCommand(CBasePlayer* pPlayer, const char* pcmd)
{
    for (int i = PC_SCOUT; i <= PC_ENGINEER; i++)
    {
        if (strcmp(pcmd, sTFClassSelection[i]) == 0)
        {
            ChangePlayerClass(pPlayer, i);
            return true;
        }
    }

    return CHalfLifeMultiplay::ClientCommand(pPlayer, pcmd);
}


void CTeamFortress::InitHUD(CBasePlayer* pPlayer)
{
    CHalfLifeMultiplay::InitHUD(pPlayer);

    for (auto t = m_teams.begin(); t != m_teams.end(); t++)
    {
        MessageBegin(MSG_ONE, gmsgTeamScore, pPlayer);
        WriteByte((*t).m_index);
        WriteShort((*t).m_score);
        MessageEnd();
    }

    MessageBegin(MSG_ONE, gmsgValClass, pPlayer);
    for (int i = 0; i < 5; i++)
    {
        WriteShort(0);
    }
    MessageEnd();
}


bool CTeamFortress::ChangePlayerTeam(CBasePlayer* pPlayer, int teamIndex, bool bKill, bool bGib, bool bAutoTeam)
{
    if (!CHalfLifeMultiplay::ChangePlayerTeam(pPlayer, teamIndex, bKill, bGib, bAutoTeam))
    {
        return false;
    }

    pPlayer->pev->playerclass = PC_UNDEFINED;

    if (pPlayer->TeamNumber() != TEAM_SPECTATORS)
    {
        MessageBegin(MSG_ONE, gmsgVGUIMenu, pPlayer);
        WriteByte(MENU_CLASS);
        MessageEnd();
    }

    return true;
}


bool CTeamFortress::ChangePlayerClass(CBasePlayer* pPlayer, int classIndex)
{
    if (pPlayer->TeamNumber() <= TEAM_UNASSIGNED || pPlayer->TeamNumber() > m_numTeams)
    {
        return false;
    }
    else if (classIndex < PC_SCOUT || classIndex > PC_ENGINEER)
    {
        return false;
    }

	if (classIndex == pPlayer->PCNumber())
	{
		return true;
	}

    bool bKill = true;

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
			DMG_GENERIC);
	}

    pPlayer->pev->playerclass = classIndex;

    g_engfuncs.pfnSetClientKeyValue(
        pPlayer->entindex(),
        g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()),
        "model",
        sTFClassModels[pPlayer->PCNumber()]);

    if (!bKill)
    {
        pPlayer->Spawn();
    }

	util::LogPrintf("\"%s<%i><%s><>\" became a %s\n",
		STRING(pPlayer->pev->netname),
		g_engfuncs.pfnGetPlayerUserId(pPlayer->edict()),
		g_engfuncs.pfnGetPlayerAuthId(pPlayer->edict()),
		sTFClassModels[pPlayer->PCNumber()]);

    return true;
}


void CTeamFortress::ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer)
{
    CHalfLifeMultiplay::ClientUserInfoChanged(pPlayer, infobuffer);

    if (pPlayer->TeamNumber() != TEAM_SPECTATORS)
    {
        g_engfuncs.pfnSetClientKeyValue(
            pPlayer->entindex(),
            infobuffer,
            "model",
            sTFClassModels[pPlayer->PCNumber()]);
    }
}


bool CTeamFortress::FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker)
{
    if (pAttacker && PlayerRelationship(pPlayer, pAttacker) >= GR_ALLY)
    {
        if (friendlyfire.value == 0 && pAttacker != pPlayer)
        {
            return false;
        }
    }

    return CHalfLifeMultiplay::FPlayerCanTakeDamage(pPlayer, pAttacker);
}


int CTeamFortress::PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget)
{
    if (pPlayer == nullptr || pTarget == nullptr || !pTarget->IsClient())
    {
        return GR_NOTTEAMMATE;
    }

    if (pPlayer->TeamNumber() == pTarget->TeamNumber())
    {
        return GR_TEAMMATE;
    }

    return GR_NOTTEAMMATE;
}


float CTeamFortress::GetPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled, bool assist)
{
    if (pAttacker != pKilled && PlayerRelationship(pAttacker, pKilled) >= GR_ALLY)
    {
        if (assist)
        {
            return 0;
        }
        return -1;
    }
    return CHalfLifeMultiplay::GetPointsForKill(pAttacker, pKilled, assist);
}


void CTeamFortress::Enter_RND_RUNNING()
{
}


void CTeamFortress::Think_RND_RUNNING()
{
    CheckTimeLimit();
}


void CTeamFortress::SendMenusToClient(CBasePlayer* player)
{
    MessageBegin(MSG_ONE, gmsgVGUIMenu, player);
    WriteByte(MENU_TEAM);
    MessageEnd();
}


void CTeamFortress::PlayerSpawn(CBasePlayer* pPlayer)
{
	MessageBegin(MSG_ALL, gmsgExtraInfo);
	WriteByte(pPlayer->entindex());
	WriteByte(pPlayer->PCNumber());
	WriteByte(pPlayer->TeamNumber());
	WriteByte(false);
	MessageEnd();

	if (pPlayer->TeamNumber() == TEAM_UNASSIGNED || pPlayer->PCNumber() == PC_UNDEFINED)
	{
		pPlayer->pev->effects |= EF_NODRAW;
		pPlayer->pev->solid = SOLID_NOT;
		pPlayer->pev->takedamage = DAMAGE_NO;
		pPlayer->pev->movetype = MOVETYPE_NONE;
		pPlayer->m_iHideHUD |= HIDEHUD_WEAPONS | HIDEHUD_FLASHLIGHT | HIDEHUD_HEALTH;
		return;
	}

	pPlayer->m_iHideHUD &= ~(HIDEHUD_WEAPONS | HIDEHUD_FLASHLIGHT | HIDEHUD_HEALTH);

	//Ensure the player switches to the Glock on spawn regardless of setting
	const int originalAutoWepSwitch = pPlayer->m_iAutoWepSwitch;
	pPlayer->m_iAutoWepSwitch = 1;

    pPlayer->GiveNamedItem("weapon_crowbar");
    pPlayer->GiveNamedItem("weapon_9mmAR");

    switch (pPlayer->PCNumber())
    {
	case PC_SCOUT:    ScoutSpawn(pPlayer);    break;
	case PC_SNIPER:   SniperSpawn(pPlayer);   break;
	case PC_SOLDIER:  SoldierSpawn(pPlayer);  break;
	case PC_DEMOMAN:  DemomanSpawn(pPlayer);  break;
	case PC_MEDIC:    MedicSpawn(pPlayer);    break;
	case PC_HVYWEAP:  HvyweapSpawn(pPlayer);  break;
	case PC_PYRO:     PyroSpawn(pPlayer);     break;
	case PC_SPY:      SpySpawn(pPlayer);      break;
	case PC_ENGINEER: EngineerSpawn(pPlayer); break;
    default:          CivilianSpawn(pPlayer); break;
    }

	pPlayer->m_iAutoWepSwitch = originalAutoWepSwitch;

	tent::TeleportSplash(pPlayer);
}


void CTeamFortress::ScoutSpawn(CBasePlayer* player)
{
	player->pev->health = player->pev->max_health = 75;
    g_engfuncs.pfnSetClientMaxspeed(player->edict(), 400);
}


void CTeamFortress::SniperSpawn(CBasePlayer* player)
{
	player->pev->health = player->pev->max_health = 90;
    g_engfuncs.pfnSetClientMaxspeed(player->edict(), 300);
}


void CTeamFortress::SoldierSpawn(CBasePlayer* player)
{
	player->pev->health = player->pev->max_health = 100;
    g_engfuncs.pfnSetClientMaxspeed(player->edict(), 240);
}


void CTeamFortress::DemomanSpawn(CBasePlayer* player)
{
	player->pev->health = player->pev->max_health = 90;
    g_engfuncs.pfnSetClientMaxspeed(player->edict(), 280);
}


void CTeamFortress::MedicSpawn(CBasePlayer* player)
{
	player->pev->health = player->pev->max_health = 90;
    g_engfuncs.pfnSetClientMaxspeed(player->edict(), 320);
}


void CTeamFortress::HvyweapSpawn(CBasePlayer* player)
{
	player->pev->health = player->pev->max_health = 100;
    g_engfuncs.pfnSetClientMaxspeed(player->edict(), 230);
}


void CTeamFortress::PyroSpawn(CBasePlayer* player)
{
	player->pev->health = player->pev->max_health = 100;
    g_engfuncs.pfnSetClientMaxspeed(player->edict(), 300);
}


void CTeamFortress::SpySpawn(CBasePlayer* player)
{
	player->pev->health = player->pev->max_health = 90;
    g_engfuncs.pfnSetClientMaxspeed(player->edict(), 300);
}


void CTeamFortress::EngineerSpawn(CBasePlayer* player)
{
	player->pev->health = player->pev->max_health = 80;
    g_engfuncs.pfnSetClientMaxspeed(player->edict(), 300);
}


void CTeamFortress::CivilianSpawn(CBasePlayer* player)
{
	player->pev->health = player->pev->max_health = 50;
    g_engfuncs.pfnSetClientMaxspeed(player->edict(), 240);
}


