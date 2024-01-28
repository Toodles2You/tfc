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

static const char* sTFTeamColors[] =
{
    "0",
    "159",
    "0",
};


PCInfo sTFClassInfo[PC_LASTCLASS] =
{
    [PC_UNDEFINED] = {},

    [PC_SCOUT] = {
        .maxHealth        = 75,
        .maxSpeed         = 400,
        .maxArmor         = 50,
        .initArmor        = 25,
        .maxArmorType     = 0.3,
        .initArmorType    = 0.3,
        .armorClasses     = AT_SAVESHOT | AT_SAVENAIL,
        .initArmorClasses = 0,
        .weapons = {
            WEAPON_NAILGUN,
            WEAPON_TF_SHOTGUN,
            WEAPON_AXE,
        },
    },

    [PC_SNIPER] = {
        .maxHealth        = 90,
        .maxSpeed         = 300,
        .maxArmor         = 50,
        .initArmor        = 0,
        .maxArmorType     = 0.3,
        .initArmorType    = 0.3,
        .armorClasses     = AT_SAVESHOT | AT_SAVENAIL,
        .initArmorClasses = 0,
        .weapons = {
            WEAPON_SNIPER_RIFLE,
            WEAPON_AUTO_RIFLE,
            WEAPON_NAILGUN,
            WEAPON_AXE,
        },
    },
    
    [PC_SOLDIER] = {
        .maxHealth        = 100,
        .maxSpeed         = 240,
        .maxArmor         = 200,
        .initArmor        = 100,
        .maxArmorType     = 0.8,
        .initArmorType    = 0.8,
        .armorClasses     = AT_SAVESHOT | AT_SAVENAIL | AT_SAVEEXPLOSION | AT_SAVEELECTRICITY | AT_SAVEFIRE,
        .initArmorClasses = 0,
        .weapons = {
            WEAPON_ROCKET_LAUNCHER,
            WEAPON_SUPER_SHOTGUN,
            WEAPON_TF_SHOTGUN,
            WEAPON_AXE,
        },
    },

    [PC_DEMOMAN] = {
        .maxHealth        = 90,
        .maxSpeed         = 280,
        .maxArmor         = 120,
        .initArmor        = 50,
        .maxArmorType     = 0.6,
        .initArmorType    = 0.6,
        .armorClasses     = AT_SAVESHOT | AT_SAVENAIL | AT_SAVEEXPLOSION | AT_SAVEELECTRICITY | AT_SAVEFIRE,
        .initArmorClasses = 0,
        .weapons = {
            WEAPON_GRENADE_LAUNCHER,
            WEAPON_PIPEBOMB_LAUNCHER,
            WEAPON_TF_SHOTGUN,
            WEAPON_AXE,
        },
    },
    
    [PC_MEDIC] = {
        .maxHealth        = 90,
        .maxSpeed         = 320,
        .maxArmor         = 100,
        .initArmor        = 50,
        .maxArmorType     = 0.6,
        .initArmorType    = 0.3,
        .armorClasses     = AT_SAVESHOT | AT_SAVENAIL | AT_SAVEELECTRICITY | AT_SAVEFIRE,
        .initArmorClasses = 0,
        .weapons = {
            WEAPON_MEDIKIT,
            WEAPON_SUPER_NAILGUN,
            WEAPON_SUPER_SHOTGUN,
            WEAPON_TF_SHOTGUN,
        },
    },

    [PC_HVYWEAP] = {
        .maxHealth        = 100,
        .maxSpeed         = 230,
        .maxArmor         = 300,
        .initArmor        = 150,
        .maxArmorType     = 0.8,
        .initArmorType    = 0.8,
        .armorClasses     = AT_SAVESHOT | AT_SAVENAIL | AT_SAVEEXPLOSION | AT_SAVEELECTRICITY | AT_SAVEFIRE,
        .initArmorClasses = 0,
        .weapons = {
            WEAPON_ASSAULT_CANNON,
            WEAPON_SUPER_SHOTGUN,
            WEAPON_TF_SHOTGUN,
            WEAPON_AXE,
        },
    },

    [PC_PYRO] = {
        .maxHealth        = 100,
        .maxSpeed         = 300,
        .maxArmor         = 150,
        .initArmor        = 50,
        .maxArmorType     = 0.6,
        .initArmorType    = 0.6,
        .armorClasses     = AT_SAVESHOT | AT_SAVENAIL | AT_SAVEELECTRICITY | AT_SAVEFIRE,
        .initArmorClasses = AT_SAVEFIRE,
        .weapons = {
            WEAPON_FLAMETHROWER,
            WEAPON_INCENDIARY,
            WEAPON_TF_SHOTGUN,
            WEAPON_AXE,
        },
    },

    [PC_SPY] = {
        .maxHealth        = 90,
        .maxSpeed         = 300,
        .maxArmor         = 100,
        .initArmor        = 25,
        .maxArmorType     = 0.6,
        .initArmorType    = 0.6,
        .armorClasses     = AT_SAVESHOT | AT_SAVENAIL | AT_SAVEELECTRICITY | AT_SAVEFIRE,
        .initArmorClasses = 0,
        .weapons = {
            WEAPON_KNIFE,
            WEAPON_SUPER_SHOTGUN,
            WEAPON_NAILGUN,
            WEAPON_TRANQ,
        },
    },

    [PC_ENGINEER] = {
        .maxHealth        = 80,
        .maxSpeed         = 300,
        .maxArmor         = 50,
        .initArmor        = 25,
        .maxArmorType     = 0.6,
        .initArmorType    = 0.3,
        .armorClasses     = AT_SAVESHOT | AT_SAVENAIL | AT_SAVEEXPLOSION | AT_SAVEELECTRICITY | AT_SAVEFIRE,
        .initArmorClasses = 0,
        .weapons = {
            WEAPON_SPANNER,
            WEAPON_SUPER_SHOTGUN,
            WEAPON_LASER,
        },
    },

    [PC_RANDOM] = {},

    [PC_CIVILIAN] = {
        .maxHealth        = 50,
        .maxSpeed         = 240,
        .maxArmor         = 0,
        .initArmor        = 0,
        .maxArmorType     = 0,
        .initArmorType    = 0,
        .armorClasses     = 0,
        .initArmorClasses = 0,
        .weapons = {
            WEAPON_AXE,
        },
    }
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

	const int originalAutoWepSwitch = pPlayer->m_iAutoWepSwitch;
	pPlayer->m_iAutoWepSwitch = 1;

    PCInfo& info = sTFClassInfo[pPlayer->PCNumber()];

	pPlayer->pev->health = pPlayer->pev->max_health = info.maxHealth;

    g_engfuncs.pfnSetClientMaxspeed(pPlayer->edict(), info.maxSpeed);

    pPlayer->pev->armorvalue = info.initArmor;
    pPlayer->pev->armortype = info.initArmorType;

    for (int i = 0; i < 4; i++)
    {
        if (info.weapons[i] != WEAPON_NONE
         && g_szWeaponNames[info.weapons[i]] != nullptr)
        {
            pPlayer->GiveNamedItem(g_szWeaponNames[info.weapons[i]]);
        }
    }

	pPlayer->m_iAutoWepSwitch = originalAutoWepSwitch;

	tent::TeleportSplash(pPlayer);
}

