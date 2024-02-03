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

static const char* sTFTeamColors[] =
{
    "0",
    "159",
    "0",
};


CTFSpawnPoint::CTFSpawnPoint(int iTeamNumber)
    : CSpawnPoint()
{
    m_teamNumber = iTeamNumber;
}


CTFSpawnPoint::CTFSpawnPoint(CBaseEntity *pEntity, int iTeamNumber)
    : CSpawnPoint(pEntity)
{
    m_teamNumber = iTeamNumber;
}


bool CTFSpawnPoint::IsValid(CBasePlayer *pPlayer, int attempt)
{	
    if (m_teamNumber != TEAM_UNASSIGNED
     && pPlayer->TeamNumber() != m_teamNumber)
    {
        return false;
    }

	return CSpawnPoint::IsValid(pPlayer, attempt);
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
        WriteShort(992);
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
    pPlayer->m_afArmorClass = info.initArmorClasses;

    for (int i = 0; i < 4; i++)
    {
        if (info.weapons[i] != WEAPON_NONE
         && g_szWeaponNames[info.weapons[i]] != nullptr)
        {
            pPlayer->GiveNamedItem(g_szWeaponNames[info.weapons[i]]);
        }

        pPlayer->GiveAmmo(info.initAmmo[i], 1 + i, info.maxAmmo[i]);
    }

	pPlayer->m_iAutoWepSwitch = originalAutoWepSwitch;

	tent::TeleportSplash(pPlayer);
}


void CTeamFortress::AddPlayerSpawnSpot(CBaseEntity *pEntity)
{
    if (FStrEq(STRING(pEntity->pev->classname), "info_player_start"))
    {
        CGameRules::AddPlayerSpawnSpot(pEntity);
        return;
    }

    if (!FStrEq(STRING(pEntity->pev->classname), "info_player_teamspawn")
     && !FStrEq(STRING(pEntity->pev->classname), "i_p_t"))
    {
        return;
    }

    auto spawn = new CTFSpawnPoint {pEntity, pEntity->pev->team};

#if 1
    ALERT(
        at_aiconsole,
        "%s %lu for team %i at (%g, %g, %g)\n",
        STRING(pEntity->pev->classname),
        m_numSpawnPoints,
        spawn->m_teamNumber,
        spawn->m_origin.x,
        spawn->m_origin.y,
        spawn->m_origin.z);
#endif

    m_spawnPoints.push_back(spawn);

    m_numSpawnPoints++;
    m_validSpawnPoints.reserve(m_numSpawnPoints);
}

