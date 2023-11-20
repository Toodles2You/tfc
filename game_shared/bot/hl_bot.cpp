//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "gamerules.h"
#include "player.h"
#include "client.h"
#include "pm_shared.h"

#include "bot.h"
#include "bot_util.h"
#include "bot_profile.h"

#include "hl_bot.h"


CHLBot::CHLBot()
{
    m_pEnemy = nullptr;
}


bool CHLBot::Initialize(const BotProfile* profile)
{
    if (!CBot::Initialize(profile))
    {
        return false;
    }
    auto model = TheBotProfiles->GetCustomSkinModelname(profile->GetSkin());
    auto clientIndex = entindex();
    if (model)
    {
        g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(edict()), "model", model);
    }
    auto color = g_engfuncs.pfnRandomLong(0, 255);
    auto topColor = UTIL_dtos1(color);
    auto bottomColor = UTIL_dtos2((color + 32) % 256);
    g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(edict()), "topcolor", topColor);
    g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(edict()), "bottomcolor", bottomColor);
    return true;
}


void CHLBot::SpawnBot()
{
    m_pEnemy = nullptr;
}


void CHLBot::Upkeep()
{
    if (!m_pEnemy)
    {
        return;
    }
    auto start = GetGunPosition();
    auto end = m_pEnemy->BodyTarget(start);
    pev->v_angle = UTIL_VecToAngles(end - start);
    pev->v_angle.x = -pev->v_angle.x;
}


void CHLBot::Update()
{
    if (TeamNumber() == TEAM_UNASSIGNED)
    {
        g_pGameRules->SetDefaultPlayerTeam(this);
        return;
    }
    if (!m_pActiveWeapon || m_pActiveWeapon->m_iClip == 0)
    {
        ClearPrimaryAttack();
        return;
    }
    float distance;
    if (!m_pEnemy)
    {
        m_pEnemy = UTIL_GetClosestEnemyPlayer(this, &distance);
    }
    if (!m_pEnemy || !IsVisible(m_pEnemy))
    {
        m_pEnemy = nullptr;
        ClearPrimaryAttack();
        return;
    }
    PrimaryAttack();
}


bool CHLBot::IsVisible(const Vector* pos, bool testFOV = false)
{
    return true;
}


bool CHLBot::IsVisible(CBasePlayer* player, bool testFOV = false, unsigned char* visParts = NULL)
{
    TraceResult tr;
    auto start = GetGunPosition();
    auto end = player->BodyTarget(start);
    UTIL_TraceLine(start, end, ignore_monsters, edict(), &tr);
    if (tr.flFraction != 1.0f && tr.pHit != player->edict())
    {
        return false;
    }
    return true;
}


bool CHLBot::IsEnemyPartVisible(VisiblePartType part)
{
    return false;
}

