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
#include "tf_defs.h"
#include "tf_goal.h"


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

static const char* sTFTeamColors[] =
{
    "0",
    "159",
    "0",
    "127",
    "63",
    "0",
};

static const char* sTFTeamNames[] =
{
    "Team_Blue",
    "Team_Red",
    "Team_Green",
    "Team_Yellow"
};


CTFSpawnPoint::CTFSpawnPoint()
    : CSpawnPoint()
{
    m_TFSpawn = nullptr;
}


CTFSpawnPoint::CTFSpawnPoint(CBaseEntity *pEntity)
    : CSpawnPoint(pEntity)
{
    m_TFSpawn = (CTFSpawn*)pEntity;
}


bool CTFSpawnPoint::IsValid(CBasePlayer *pPlayer, int attempt)
{
    /* Inactive means... valid. :v */
    if (!m_TFSpawn->InGoalState(TFGS_INACTIVE))
    {
        return false;
    }

    int team = m_TFSpawn->tfv.GetTeam();

    if (team != TEAM_UNASSIGNED && team != pPlayer->TeamNumber())
    {
        return false;
    }

    int pc = m_TFSpawn->tfv.playerclass;

    if (pc != PC_UNDEFINED && pc != pPlayer->PCNumber())
    {
        return false;
    }

	return CSpawnPoint::IsValid(pPlayer, attempt);
}


/* Called by info_tfdetect if one is present in the map. */
void CTeamFortress::InitializeTeams()
{
    m_teams.clear();

    for (int i = 0; i < m_numTeams; i++)
    {
        auto& info = m_TFTeamInfo[i];

        char c = info.m_szTeamName[0];
        if (c == '\0')
        {
            /* Use the default name. */
            strcpy (info.m_szTeamName, sTFTeamNames[i]);
        }
        else if (c == '#')
        {
            /* Remove the # prefix. */
            memmove (info.m_szTeamName, info.m_szTeamName + 1, 15);
        }

        m_teams.push_back(CTeam {TEAM_DEFAULT + i, info.m_szTeamName});
    }
}


CTeamFortress::CTeamFortress()
{
    m_teams.clear();
    m_numTeams = 2;

    m_teams.push_back(CTeam{TEAM_BLUE, "Team_Blue"});
    m_teams.push_back(CTeam{TEAM_RED, "Team_Red"});
}


bool CTeamFortress::ClientCommand(CBasePlayer* pPlayer, const char* pcmd)
{
    for (int i = PC_SCOUT; i <= PC_MEDIC; i++)
    {
        if (strcmp(pcmd, sTFClassSelection[i]) == 0)
        {
            if (!ChangePlayerClass(pPlayer, i))
            {
                MessageBegin(MSG_ONE, gmsgVGUIMenu, pPlayer);
                WriteByte(MENU_CLASS);
                MessageEnd();
            }
            return true;
        }
    }
    if (strcmp(pcmd, sTFClassSelection[PC_CIVILIAN]) == 0)
    {
        if (!ChangePlayerClass(pPlayer, PC_CIVILIAN))
        {
            MessageBegin(MSG_ONE, gmsgVGUIMenu, pPlayer);
            WriteByte(MENU_CLASS);
            MessageEnd();
        }
        return true;
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
    WriteShort(0);
    for (int i = 0; i < 4; i++)
    {
        /* Toodles TODO: */
        WriteShort(992 | m_TFTeamInfo[i].m_afInvalidClasses);
    }
    MessageEnd();
}


bool CTeamFortress::ChangePlayerTeam(CBasePlayer* pPlayer, int teamIndex, bool bKill, bool bGib, bool bAutoTeam)
{
    if (teamIndex > TEAM_UNASSIGNED && teamIndex <= m_numTeams)
    {
        const auto& info = m_TFTeamInfo[teamIndex - 1];

        if (info.m_iMaxPlayers > 0
         && m_teams[teamIndex - 1].m_numPlayers >= info.m_iMaxPlayers)
        {
            return false;
        }
    }

    if (!CHalfLifeMultiplay::ChangePlayerTeam(pPlayer, teamIndex, bKill, bGib, bAutoTeam))
    {
        return false;
    }

    pPlayer->pev->playerclass = PC_UNDEFINED;

    if (pPlayer->TeamNumber() != TEAM_SPECTATORS)
    {
        char* infobuffer = g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict());

        g_engfuncs.pfnSetClientKeyValue(
            pPlayer->entindex(),
            infobuffer,
            "topcolor",
            sTFTeamColors[pPlayer->TeamNumber()]);

        g_engfuncs.pfnSetClientKeyValue(
            pPlayer->entindex(),
            infobuffer,
            "bottomcolor",
            sTFTeamColors[pPlayer->TeamNumber()]);

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

    const auto invalidClasses = m_TFTeamInfo[pPlayer->TeamNumber() - 1].m_afInvalidClasses;

    if (classIndex == PC_CIVILIAN)
    {
        if (invalidClasses != -1)
        {
            return false;
        }
    }
    else if (invalidClasses != 0)
    {
        int check_flag = classIndex - 1;
        if (classIndex >= PC_SPY)
        {
            check_flag++;
        }
        if (invalidClasses != 0)
        {
            return false;
        }
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

    if (!bKill && g_pGameRules->FPlayerCanRespawn(pPlayer))
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

		g_engfuncs.pfnSetClientKeyValue(
			pPlayer->entindex(),
			infobuffer,
			"topcolor",
			sTFTeamColors[pPlayer->TeamNumber()]);

		g_engfuncs.pfnSetClientKeyValue(
			pPlayer->entindex(),
			infobuffer,
			"bottomcolor",
			sTFTeamColors[pPlayer->TeamNumber()]);
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

    if ((m_TFTeamInfo[pPlayer->TeamNumber() - 1].m_afAlliedTeams & (1 << (pTarget->TeamNumber() - 1))) != 0)
    {
        return GR_ALLY;
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
    pPlayer->SendExtraInfo();

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

	const int originalAutoWepSwitch = pPlayer->m_iAutoWepSwitch;
	pPlayer->m_iAutoWepSwitch = 1;

    PCInfo& info = sTFClassInfo[pPlayer->PCNumber()];

	pPlayer->pev->health = pPlayer->pev->max_health = info.maxHealth;

    g_engfuncs.pfnSetClientMaxspeed(pPlayer->edict(), info.maxSpeed);

    pPlayer->pev->armorvalue = info.initArmor;
    pPlayer->pev->armortype = info.initArmorType;
	pPlayer->m_flArmorMax = info.maxArmor;
	pPlayer->m_flArmorTypeMax = info.maxArmorType;
    pPlayer->m_afArmorClass = info.initArmorClasses;

    for (int i = 0; i < 4; i++)
    {
        if (info.weapons[i] != -1)
        {
            pPlayer->GiveNamedItem(g_szWeaponNames[info.weapons[i]]);
        }
    }

    for (int i = 0; i < AMMO_TYPES; i++)
    {
		if (info.initAmmo[i] != 0)
		{
            pPlayer->GiveAmmo(info.initAmmo[i], i);
        }
    }

	pPlayer->m_iAutoWepSwitch = originalAutoWepSwitch;

	tent::TeleportSplash(pPlayer);
}


bool CTeamFortress::FPlayerCanRespawn(CBasePlayer* pPlayer)
{
    if (pPlayer->TeamNumber() == TEAM_UNASSIGNED)
    {
        return false;
    }

    if (pPlayer->PCNumber() == PC_UNDEFINED)
    {
        return false;
    }

    return CHalfLifeMultiplay::FPlayerCanRespawn(pPlayer);
}

void CTeamFortress::AddPlayerSpawnSpot(CBaseEntity *pEntity)
{
    if (FStrEq(STRING(pEntity->pev->classname), "info_player_start"))
    {
        CGameRules::AddPlayerSpawnSpot(pEntity);
        return;
    }

    if (!FStrEq(STRING(pEntity->pev->classname), "info_player_teamspawn"))
    {
        return;
    }

    auto spawn = new CTFSpawnPoint {pEntity};

#if 1
    ALERT(
        at_aiconsole,
        "%s %lu for team %i (state %i, goal %i, group %i), at (%g, %g, %g)\n",
        STRING(pEntity->pev->classname),
        m_numSpawnPoints,
        spawn->m_TFSpawn->tfv.GetTeam(),
        spawn->m_TFSpawn->tfv.goal_state,
        spawn->m_TFSpawn->GetNumber(),
        spawn->m_TFSpawn->GetGroup(),
        spawn->m_origin.x,
        spawn->m_origin.y,
        spawn->m_origin.z);
#endif

    m_spawnPoints.push_back(spawn);

    m_numSpawnPoints++;
    m_validSpawnPoints.reserve(m_numSpawnPoints);
}


int CTeamFortress::GetMaxAmmo(CBasePlayer* pPlayer, int iAmmoType)
{
    PCInfo& info = sTFClassInfo[pPlayer->PCNumber()];
    return info.maxAmmo[iAmmoType];
}

