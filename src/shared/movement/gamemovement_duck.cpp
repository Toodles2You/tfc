//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Player movement interface
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

#include "usercmd.h"
#include "pm_defs.h"
#include "pm_shared.h"
#include "pm_materials.h"
#include "pm_movevars.h"
#include "pm_debug.h"
#include "gamemovement.h"


void CHalfLifeMovement::CheckDucking()
{
    if (pmove->dead != 0)
    {
        return;
    }

    const auto bWantToDuck = (pmove->cmd.buttons & IN_DUCK) != 0 || IsSubmerged() || pmove->waterjumptime != 0;
    const auto bIsFullyDucked = pmove->usehull == 1;

    /* Check if the player wants to change duck state. */
    if (bWantToDuck)
    {
        if (BeginDucking())
        {
            pmove->bInDuck = true;
            pmove->flDuckTime = 400.0F;
        }
    }
    else if ((bIsFullyDucked || pmove->bInDuck) && BeginUnducking())
    {
        pmove->bInDuck = false;
        /* Invert the duck time so the view smoothly lerps. */
        pmove->flDuckTime = 150.0F * (1.0F - (pmove->flDuckTime / 400.0F));
    }

    if (pmove->flDuckTime != 0)
    {
        pmove->flDuckTime = std::max((int)pmove->flDuckTime - pmove->cmd.msec, 0);

        /* Check if the duck time has elapsed, or the player has left the ground during the duck. */
        if (pmove->flDuckTime == 0 || (!IsSubmerged() && pmove->onground == -1))
        {
            /* Finish changing duck state. */
            pmove->bInDuck ? FinishDucking() : FinishUnducking();
            return;
        }

        /* Lerp the player view, taking the different hull sizes into account. */
        const auto flDuckView =
            VEC_DUCK_VIEW.z - (pmove->player_mins[1].z - pmove->player_mins[0].z);

        float flFraction;

        if (pmove->bInDuck)
        {
            flFraction = 1.0F - pmove->flDuckTime / 400.0F;
        }
        else
        {
            flFraction = pmove->flDuckTime / 150.0F;
        }

        pmove->view_ofs.z =
            VEC_VIEW.z + (flDuckView - VEC_VIEW.z) * SplineFraction(flFraction);
    }
}


bool CHalfLifeMovement::BeginDucking()
{
    return (pmove->flags & FL_DUCKING) == 0 && pmove->flDuckTime == 0;
}


void CHalfLifeMovement::FinishDucking()
{
    pmove->usehull = 1;

    if (IsSubmerged() || pmove->onground != -1)
    {
        pmove->origin.z -= pmove->player_mins[1].z - pmove->player_mins[0].z;
        FixDuckStuck(1);
    }
#if 0
    else
    {
        pmove->origin.z += VEC_VIEW.z - VEC_DUCK_VIEW.z;
        FixDuckStuck(-1);
    }
#endif

    pmove->flDuckTime = 0;
    pmove->bInDuck = false;

    pmove->flags |= FL_DUCKING;
    pmove->view_ofs.z = VEC_DUCK_VIEW.z;

    CategorizePosition();

    if (player->m_Action != CBasePlayer::Action::Jump || player->m_fSequenceFinished)
    {
        player->pev->flags |= FL_DUCKING;
        player->SetAction(CBasePlayer::Action::Idle, true);
    }
}


void CHalfLifeMovement::FixDuckStuck(int direction)
{
    int hitent;
    int i;

    hitent = pmove->PM_TestPlayerPositionEx(
        pmove->origin,
        nullptr,
        CHalfLifeMovement::g_ShouldIgnore);

    if (hitent == -1)
    {
        return;
    }

    Vector vecOrigin = pmove->origin;

    for (i = 0; i < 36; i++)
    {
        pmove->origin.z += direction;

        hitent = pmove->PM_TestPlayerPositionEx(
            pmove->origin,
            nullptr,
            CHalfLifeMovement::g_ShouldIgnore);

        if (hitent == -1)
        {
            return;
        }
    }

    pmove->origin = vecOrigin;
}


bool CHalfLifeMovement::BeginUnducking()
{
    if (pmove->usehull != 1)
    {
        return true;
    }

    Vector vecOrigin = pmove->origin;

    if (pmove->onground != -1)
    {
        vecOrigin.z += pmove->player_mins[1].z - pmove->player_mins[0].z;
    }
#if 0
    else
    {
        vecOrigin.z -= VEC_VIEW.z - VEC_DUCK_VIEW.z;
    }
#endif

    pmtrace_t trace = pmove->PM_PlayerTraceEx(
        vecOrigin,
        vecOrigin,
        PM_STUDIO_BOX,
        CHalfLifeMovement::g_ShouldIgnore);

    if (trace.startsolid != 0)
    {
        return false;
    }
    
    pmove->usehull = 0;

    trace = pmove->PM_PlayerTraceEx(
        vecOrigin,
        vecOrigin,
        PM_STUDIO_BOX,
        CHalfLifeMovement::g_ShouldIgnore);

    if (trace.startsolid != 0)
    {
        pmove->usehull = 1;
        return false;
    }

    pmove->flags &= ~FL_DUCKING;

    pmove->origin = vecOrigin;

    CategorizePosition();

    if (player->m_Action != CBasePlayer::Action::Jump || player->m_fSequenceFinished)
    {
        player->pev->flags &= ~FL_DUCKING;
        player->SetAction(CBasePlayer::Action::Idle, true);
    }

    return true;
}


void CHalfLifeMovement::FinishUnducking()
{
    pmove->view_ofs.z = VEC_VIEW.z;
}

