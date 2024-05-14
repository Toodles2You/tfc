//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Team Fortress map entities
//
// $NoKeywords: $
//=============================================================================

#include "tf_goal.h"
#include "player.h"
#include "game.h"
#include "gamerules.h"

//==================================================
// CTFGoalTimer
//==================================================

LINK_ENTITY_TO_CLASS(info_tfgoal_timer, CTFGoalTimer);
LINK_ENTITY_TO_CLASS(i_t_t, CTFGoalTimer);

CTFGoalTimer::CTFGoalTimer(Entity* containingEntity) : CTFGoal(containingEntity)
{
}

bool CTFGoalTimer::Spawn()
{
    if (search_time <= 0.0F)
    {
        engine::AlertMessage(at_console, "Timer goal created with no specified time\n");
        return false;
    }

    if (!CTFGoal::Spawn())
    {
        return false;
    }

    v.solid = SOLID_NOT;
    return true;
}

void CTFGoalTimer::PlaceTimer()
{
    SetThink(&CTFGoalTimer::TimerTick);
    v.nextthink = gpGlobals->time + search_time;
    CTFGoal::PlaceGoal();
}

void CTFGoalTimer::StartGoal()
{
    SetUse(&CTFGoal::GoalUse);
    SetThink(&CTFGoalTimer::PlaceTimer);
    v.nextthink = gpGlobals->time + 0.2F;
    if (InGoalState(TFGS_REMOVED))
    {
        RemoveGoal();
    }
}

void CTFGoalTimer::SetupRespawn()
{
    if (HasGoalResults(TFGR_SINGLE))
    {
        RemoveGoal();
        return;
    }
    InactivateGoal();
    SetThink(&CTFGoalTimer::TimerTick);
    v.nextthink = gpGlobals->time + search_time;
}

void CTFGoalTimer::TimerTick()
{
    if (InGoalState(TFGS_REMOVED))
        return;

    if (tfv.PlayerMeetsCriteria(nullptr))
    {
        DoResults(CWorld::World, true);
    }
    else
    {
        InactivateGoal();
        v.nextthink = gpGlobals->time + search_time;
    }
}
