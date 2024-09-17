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
#include "tf_gamerules.h"
#include "player.h"
#include "client.h"
#include "pm_shared.h"

#include "bot.h"
#include "bot_util.h"
#include "bot_profile.h"

#include "hl_bot.h"


CHLBot::CHLBot(Entity* containingEntity) : CBot(containingEntity)
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
    auto clientIndex = v.GetIndex();
    if (model)
    {
        engine::SetClientKeyValue(clientIndex, engine::GetInfoKeyBuffer(&v), "model", model);
    }
    auto color = engine::RandomLong(0, 255);
    auto topColor = util::dtos1(color);
    auto bottomColor = util::dtos2((color + 32) % 256);
    engine::SetClientKeyValue(clientIndex, engine::GetInfoKeyBuffer(&v), "topcolor", topColor);
    engine::SetClientKeyValue(clientIndex, engine::GetInfoKeyBuffer(&v), "bottomcolor", bottomColor);
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
    auto end = m_pEnemy->BodyTarget();
    v.v_angle = util::VecToAngles(end - start);
    v.v_angle.x = -v.v_angle.x;
}


void CHLBot::Update()
{
    if (TeamNumber() == TEAM_UNASSIGNED)
    {
        g_pGameRules->ChangePlayerTeam(this, g_pGameRules->GetDefaultPlayerTeam(this), false, false, true);
        if (g_pGameRules->GetGameMode() >= kGamemodeTeamFortress)
        {
            dynamic_cast<CTeamFortress*>(g_pGameRules)->ChangePlayerClass(this, engine::RandomLong(PC_SCOUT, PC_ENGINEER));
        }
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
    Upkeep();
    PrimaryAttack();
}


bool CHLBot::IsVisible(const Vector* pos, bool testFOV = false)
{
    return true;
}


bool CHLBot::IsVisible(CBasePlayer* player, bool testFOV = false, unsigned char* visParts = nullptr)
{
    if (!player->IsPlayer() || !player->IsAlive())
    {
        return false;
    }

    /* Player is in "no target" mode. */
    if ((player->v.flags & FL_NOTARGET) != 0)
    {
        return false;
    }

    /* Player is invisible. */
    if ((player->v.effects & EF_NODRAW) != 0
     || (player->v.rendermode != kRenderNormal && player->v.renderamt <= 5))
    {
        return false;
    }

    if (player->InState(State::Disguised) && !g_pGameRules->CanSeeThroughDisguise(this, player))
    {
        return false;
    }

    util::MakeVectors(v.angles);

	const auto los = (player->BodyTarget() - EyePosition()).Normalize();
	const auto dot = DotProduct(los, gpGlobals->v_forward);

    /* The player is out of our field of view. */
	if (dot <= 0.1F)
	{
		return false;
	}

    TraceResult tr;
    auto start = GetGunPosition();
    auto end = player->BodyTarget();

    util::TraceLine(start, end, util::ignore_monsters, this, &tr);

    if (tr.flFraction != 1.0f && tr.pHit != &player->v)
    {
        return false;
    }

    return true;
}


bool CHLBot::IsEnemyPartVisible(VisiblePartType part)
{
    return false;
}

