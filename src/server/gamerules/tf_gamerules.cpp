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

    memset(display_item_status, 0, sizeof(display_item_status));

    team_str_home = iStringNull;
    team_str_moved = iStringNull;
    team_str_carried = iStringNull;

    non_team_str_home = iStringNull;
    non_team_str_moved = iStringNull;
    non_team_str_carried = iStringNull;
}


/* Toodles FIXME: This is redundant but, I'm lazy. */
void CTeamFortress::DisplayItemStatus(CBasePlayer* player, const int goalNo)
{
    auto goal = util::FindItem(goalNo);

    if (goal == nullptr)
    {
        return;
    }

    const auto isGoalOwner = player->TeamNumber() == goal->tfv.GetOwningTeam();
    const char* message = nullptr;
    const char* carrier = nullptr;

    if (goal->InGoalState(TFGS_ACTIVE))
    {
        /* Goal is being carried! */
        if (isGoalOwner)
        {
            if (!FStringNull(team_str_carried))
            {
                message = STRING(team_str_carried);
            }
        }
        else if (!FStringNull(non_team_str_carried))
        {
            message = STRING(non_team_str_carried);
        }

        /* Find the carrier. */
        if (goal->v.owner != nullptr)
        {
            auto goalCarrier = goal->v.owner->Get<CBasePlayer>();

            if (goalCarrier != nullptr)
            {
                carrier = STRING(goalCarrier->v.netname);
            }
        }
    }
    else if (goal->v.origin != goal->v.oldorigin)
    {
        /* Goal is dropped! */
        if (isGoalOwner)
        {
            if (!FStringNull(team_str_moved))
            {
                message = STRING(team_str_moved);
            }
        }
        else if (!FStringNull(non_team_str_moved))
        {
            message = STRING(non_team_str_moved);
        }
    }
    else
    {
        /* Goal is at home! */
        if (isGoalOwner)
        {
            if (!FStringNull(team_str_home))
            {
                message = STRING(team_str_home);
            }
        }
        else if (!FStringNull(non_team_str_home))
        {
            message = STRING(non_team_str_home);
        }
    }

    if (message == nullptr)
    {
        return;
    }

    util::ClientPrint(player, HUD_PRINTTALK, message, carrier);
}


bool CTeamFortress::ClientCommand(CBasePlayer* pPlayer, const char* pcmd)
{
    if (streq(pcmd, "flaginfo"))
    {
        for (int i = 0; i < TEAM_SPECTATORS - 1; i++)
        {
            if (display_item_status[i] > 0)
            {
                DisplayItemStatus(pPlayer, display_item_status[i]);
            }
        }

        return true;
    }

    for (int i = PC_SCOUT; i <= PC_ENGINEER; i++)
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
    for (auto i = 0; i < 4; i++)
    {
        /* Toodles TODO: */
        WriteShort(128 | m_TFTeamInfo[i].m_afInvalidClasses);
    }
    MessageEnd();
}


bool CTeamFortress::CanJoinTeam(CBasePlayer* player, int teamIndex)
{
    if (!CHalfLifeMultiplay::CanJoinTeam(player, teamIndex))
    {
        return false;
    }

    if (teamIndex != TEAM_SPECTATORS)
    {
        const auto& info = m_TFTeamInfo[teamIndex - 1];

        if (info.m_iMaxPlayers > 0
         && m_teams[teamIndex - 1].m_numPlayers >= info.m_iMaxPlayers)
        {
            return false;
        }
    }

    return true;
}


bool CTeamFortress::ChangePlayerTeam(CBasePlayer* pPlayer, int teamIndex, bool bKill, bool bGib, bool bAutoTeam)
{
	if (teamIndex == pPlayer->TeamNumber())
	{
		return true;
	}

    if (!CHalfLifeMultiplay::ChangePlayerTeam(pPlayer, teamIndex, bKill, bGib, bAutoTeam))
    {
        return false;
    }

    pPlayer->v.playerclass = PC_UNDEFINED;

	pPlayer->RemoveAllBuildings(bKill);

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

    pPlayer->v.playerclass = classIndex;

    if (!bKill && g_pGameRules->FPlayerCanRespawn(pPlayer))
    {
        pPlayer->Spawn();
    }

	util::LogPrintf("\"%s<%i><%s><>\" changed role to \"%s\"\n",
		STRING(pPlayer->v.netname),
		engine::GetPlayerUserId(&pPlayer->v),
		engine::GetPlayerAuthId(&pPlayer->v),
		sTFClassSelection[pPlayer->PCNumber()]);

    return true;
}


void CTeamFortress::ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer)
{
    CHalfLifeMultiplay::ClientUserInfoChanged(pPlayer, infobuffer);
}


bool CTeamFortress::FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker, CBaseEntity* inflictor)
{
    if (pAttacker == nullptr || !pAttacker->IsClient())
    {
        return true;
    }

    CBaseEntity* check = (inflictor != nullptr) ? inflictor : pAttacker;

    if (check != nullptr && PlayerRelationship(pPlayer, check) >= GR_ALLY)
    {
        if (friendlyfire.value == 0 && pAttacker != pPlayer)
        {
            return false;
        }
    }

    return CHalfLifeMultiplay::FPlayerCanTakeDamage(pPlayer, pAttacker, inflictor);
}


int CTeamFortress::PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget)
{
    if (pPlayer == nullptr || pTarget == nullptr)
    {
        return GR_NOTTEAMMATE;
    }

    if (pPlayer->TeamNumber() == pTarget->TeamNumber())
    {
        return GR_TEAMMATE;
    }

    if (!pTarget->IsClient())
    {
        return GR_NOTTEAMMATE;
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
		return;
	}

	const int originalAutoWepSwitch = pPlayer->m_iAutoWepSwitch;
	pPlayer->m_iAutoWepSwitch = 1;

    PCInfo& info = sTFClassInfo[pPlayer->PCNumber()];

	pPlayer->v.health = pPlayer->v.max_health = info.maxHealth;

    engine::SetClientMaxspeed(&pPlayer->v, info.maxSpeed);

    pPlayer->v.armorvalue = info.initArmor;
    pPlayer->v.armortype = info.initArmorType;
	pPlayer->m_flArmorMax = info.maxArmor;
	pPlayer->m_flArmorTypeMax = info.maxArmorType;
    pPlayer->m_afArmorClass = info.initArmorClasses;

    if (pPlayer->PCNumber() == PC_DEMOMAN)
    {
        pPlayer->GiveNamedItem(g_szWeaponNames[WEAPON_DETPACK]);
    }

    if (pPlayer->PCNumber() == PC_ENGINEER)
    {
        pPlayer->GiveNamedItem(g_szWeaponNames[WEAPON_BUILDER]);
    }
    else
    {
        pPlayer->RemoveAllBuildings(false);
    }

    for (int i = 0; i < 4; i++)
    {
        if (info.weapons[i] != -1)
        {
            pPlayer->GiveNamedItem(g_szWeaponNames[info.weapons[i]]);
        }
    }

    pPlayer->SelectWeapon(info.weapons[0]);
    pPlayer->m_iLastWeapon = info.weapons[1];

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
    if (streq(STRING(pEntity->v.classname), "info_player_start"))
    {
        CGameRules::AddPlayerSpawnSpot(pEntity);
        return;
    }

    if (!streq(STRING(pEntity->v.classname), "info_player_teamspawn"))
    {
        return;
    }

    auto spawn = new CTFSpawnPoint {pEntity};

#if 1
    engine::AlertMessage(
        at_aiconsole,
        "%s %lu for team %i (state %i, goal %i, group %i), at (%g, %g, %g)\n",
        STRING(pEntity->v.classname),
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

    return info.maxAmmo[iAmmoType] + pPlayer->GetUnloadedAmmo(iAmmoType);
}


float CTeamFortress::FlPlayerFallDamage(CBasePlayer* player)
{
    auto damage = player->m_flFallVelocity / 60.0F;

    if (player->PCNumber() == PC_SCOUT)
    {
        damage /= 2.0F;
    }
    else if (player->PCNumber() == PC_HVYWEAP)
    {
        damage *= 1.5F;
    }

    return damage;
}

