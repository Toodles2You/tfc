//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Stupid bipedal dumb ass movement
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


void CHalfLifeMovement::Walk()
{
    AddCorrectGravity();

    if ((pmove->cmd.buttons & IN_JUMP) != 0)
    {
        Jump();
    }
    else if ((pmove->flags & FL_JUMPING) != 0)
    {
        pmove->flags &= ~FL_JUMPING;
    }

    if (pmove->onground != -1)
    {
        pmove->velocity.z = 0;
        ApplyFriction();
    }

    CheckVelocity();

    if (pmove->onground != -1)
    {
        WalkMove();
    }
    else
    {
        AirMove();
    }

    CategorizePosition();

    pmove->velocity = pmove->velocity - pmove->basevelocity;

    CheckVelocity();
    FixUpGravity();

    if (pmove->onground != -1)
    {
        if (m_wishSpeed != 0.0F)
        {
            player->SetAnimation(PLAYER_WALK);
        }
        else
        {
            player->SetAnimation(PLAYER_IDLE);
        }
        pmove->velocity.z = 0;
    }
}


void CHalfLifeMovement::WalkMove()
{
    Accelerate(m_wishDir, m_wishSpeed);
    pmove->velocity = pmove->velocity + pmove->basevelocity;

    if (pmove->velocity.LengthSquared() < 1.0f)
    {
        pmove->velocity = g_vecZero;
        return;
    }

    int oldonground = pmove->onground;
    Vector dest = pmove->origin + pmove->velocity * pmove->frametime;
    pmtrace_t trace =
        pmove->PM_PlayerTrace(
            pmove->origin,
            dest,
            PM_STUDIO_BOX,
            -1);

    if (trace.fraction == 1)
    {
        pmove->origin = trace.endpos;

        if (pmove->movevars->stepsize >= 1)
        {
            StayOnGround();
        }
        return;
    }

    if (oldonground == -1)
    {
        return;
    }

    /*
    Try sliding forward both on ground and up 16 pixels
    take the move that goes farthest
    */
    const Vector originalOrigin = pmove->origin;
    const Vector originalVelocity = pmove->velocity;

    FlyMove();

    if (pmove->movevars->stepsize < 1)
    {
        return;
    }

    Vector downOrigin = pmove->origin;
    Vector downVelocity = pmove->velocity;

    pmove->origin = originalOrigin;
    pmove->velocity = originalVelocity;

    /* Start out up one stair height */
    dest = pmove->origin;
    dest.z += pmove->movevars->stepsize;

    trace = pmove->PM_PlayerTrace(
        pmove->origin,
        dest,
        PM_STUDIO_BOX,
        -1);

    /*
    If we started okay and made it part of the way at least,
    copy the results to the movement start position and then
    run another move try.
    */
    if (trace.startsolid == 0 && trace.allsolid == 0)
    {
        pmove->origin = trace.endpos;
    }

    FlyMove();

    /*
    Now try going back down from the end point
    press down the stepheight
    */
    dest = pmove->origin;
    dest.z -= pmove->movevars->stepsize;

    trace = pmove->PM_PlayerTrace(
        pmove->origin,
        dest,
        PM_STUDIO_BOX,
        -1);

    /*
    If we are not on the ground any more then
    use the original movement attempt
    */
    if (trace.plane.normal.z < 0.7)
    {
        pmove->origin = downOrigin;
        pmove->velocity = downVelocity;
        StayOnGround();
        return;
    }

    if (trace.startsolid == 0 && trace.allsolid == 0)
    {
        pmove->origin = trace.endpos;
    }

    /* Decide which one went farther */
    if ((downOrigin - originalOrigin).LengthSquared()
     > (pmove->origin - originalOrigin).LengthSquared())
    {
        pmove->origin = downOrigin;
        pmove->velocity = downVelocity;
    }
    else
    {
        /* Copy z value from slide move */
        pmove->velocity.z = downVelocity.z;
    }

    StayOnGround();
}


void CHalfLifeMovement::AirMove()
{
    Accelerate(m_wishDir, m_wishSpeed);
    pmove->velocity = pmove->velocity + pmove->basevelocity;

    FlyMove();
}


void CHalfLifeMovement::Jump()
{
    if (pmove->dead != 0)
    {
        return;
    }

    if (pmove->onground == -1 || (pmove->flags & FL_JUMPING) != 0)
    {
        return;
    }

    pmove->onground = -1;
    pmove->flags |= FL_JUMPING;

    player->SetAnimation(PLAYER_JUMP);

    pmove->velocity.z = sqrtf(2 * 800 * 45);

    if (pmove->basevelocity != g_vecZero)
    {
        pmove->velocity = pmove->velocity + pmove->basevelocity;
        pmove->basevelocity = g_vecZero;
    }
    
    FixUpGravity();
}


void CHalfLifeMovement::ApplyFriction()
{
    float speed = pmove->velocity.LengthSquared();

    if (speed < 0.1F)
    {
        return;
    }

    speed = sqrtf(speed);

    float drop = 0.0F;

    if (pmove->onground != -1)
    {
        Vector start, stop;

        start.x = stop.x = pmove->origin.x + pmove->velocity.x / speed * 16;
        start.y = stop.y = pmove->origin.y + pmove->velocity.y / speed * 16;
        start.z = pmove->origin.z + pmove->player_mins[pmove->usehull].z;
        stop.z = start.z - 34;

        pmtrace_t trace =
            pmove->PM_PlayerTrace(
                start,
                stop,
                PM_STUDIO_BOX,
                -1);

        float friction = pmove->movevars->friction;
        if (trace.fraction == 1)
        {
            friction *= pmove->movevars->edgefriction;
        }
        friction *= pmove->friction;

        drop +=
            std::max(speed, pmove->movevars->stopspeed)
            * friction
            * pmove->frametime;
    }

    float newSpeed = speed - drop;
    if (newSpeed < 0)
    {
        newSpeed = 0;
    }
    newSpeed /= speed;
    pmove->velocity = pmove->velocity * newSpeed;
}


void CHalfLifeMovement::StayOnGround()
{
    pmtrace_t trace;
    Vector start = pmove->origin;
    Vector end = pmove->origin;
    start.z += 2;
    end.z -= pmove->movevars->stepsize;

    trace = pmove->PM_PlayerTrace(pmove->origin, start, PM_STUDIO_BOX, -1);
    start = trace.endpos;
    trace = pmove->PM_PlayerTrace(start, end, PM_STUDIO_BOX, -1);

    if (trace.fraction != 0.0F
     && trace.fraction != 1.0F
     && trace.startsolid == 0
     && trace.plane.normal.z >= 0.7
     && fabsf(pmove->origin.z - trace.endpos.z) > 0.5F)
    {
        pmove->origin = trace.endpos;
    }
}

