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

#include <algorithm>


CHalfLifeMovement::CHalfLifeMovement(playermove_t* _pmove, CBasePlayer* _player)
    : CGameMovement{_pmove, _player}
{
    m_flatForward = g_vecZero;
    m_flatRight = g_vecZero;
    m_wishVel = g_vecZero;
    m_wishDir = g_vecZero;
    m_wishSpeed = 0.0F;
    m_freeWishDir = g_vecZero;
    m_freeWishSpeed = 0.0F;
}


void CHalfLifeMovement::Move()
{
    CGameMovement::Move();

    CheckParameters();
    CategorizePosition();
    CheckDucking();

    if (pmove->movetype == MOVETYPE_NONE)
    {
        return;
    }

    if (pmove->movetype != MOVETYPE_NOCLIP && IsStuck())
    {
        return;
    }

    switch (pmove->movetype)
    {
    case MOVETYPE_NOCLIP:
        NoClip();
        break;
    case MOVETYPE_WALK:
        if (!CheckLadder())
        {
            if (pmove->waterjumptime != 0)
            {
                WaterJump();
            }
            else if (IsSubmerged())
            {
                Swim();
            }
            else
            {
                Walk();
            }
        }
        pmove->friction = 1;
        break;
    default:
        break;
    }

    if (pmove->onground != -1)
    {
        pmove->flags |= FL_ONGROUND;
    }
    else
    {
        pmove->flags &= ~FL_ONGROUND;
    }
}


void CHalfLifeMovement::BuildWishMove(const Vector& move)
{
    /* Direction for walking movement. */

    m_wishVel =
        m_flatForward * move.x + m_flatRight * move.y;

    m_wishVel = m_wishVel * pmove->maxspeed;

    m_wishDir = m_wishVel.Normalize();
    m_wishSpeed = m_wishVel.Length();

    if (m_wishSpeed > pmove->maxspeed)
    {
        m_wishSpeed = pmove->maxspeed;
        m_wishVel = m_wishDir * m_wishSpeed;
    }
}


void CHalfLifeMovement::BuildFreeWishMove(const Vector& move)
{
    /* Direction for flying & swimming movement. */

    float moveZed = move.z;
    if ((pmove->cmd.buttons & IN_JUMP) != 0)
    {
        moveZed = 1.0F;
    }
#if 0
    else if ((pmove->cmd.buttons & IN_DUCK) != 0)
    {
        moveZed = -1.0F;
    }
#endif

    m_freeWishDir =
        pmove->forward * move.x + pmove->right * move.y + Vector(0.0F, 0.0F, moveZed);

    m_freeWishDir = m_freeWishDir * pmove->maxspeed;

    m_freeWishSpeed = std::min(m_freeWishDir.Length(), pmove->maxspeed);

    if (IsSubmerged())
    {
        m_freeWishSpeed *= 0.8F;
    }

    m_freeWishDir = m_freeWishDir.Normalize();
}


float CHalfLifeMovement::GetSpeedModifier()
{
    if (pmove->movetype == MOVETYPE_NOCLIP
     || pmove->movetype == MOVETYPE_FLY)
    {
        return 1.0F;
    }

    float speed = 1.0F;

    if (player->m_nLegDamage != 0)
    {
        speed *= (10.0F - player->m_nLegDamage) / 10.0F;
    }

    if ((pmove->cmd.buttons & IN_SPEED) != 0
     || (player->m_TFState & kTFStateAiming) != 0)
    {
        speed = std::min(speed, 0.3F);
    }

    if ((pmove->flags & FL_DUCKING) != 0)
    {
        speed /= 3.0F;
    }

    return speed;
}


void CHalfLifeMovement::CheckParameters()
{
    Vector move =
    {
        *reinterpret_cast<int*>(&pmove->cmd.forwardmove),
        *reinterpret_cast<int*>(&pmove->cmd.sidemove),
        *reinterpret_cast<int*>(&pmove->cmd.upmove)
    };

    move = move / 100.0F;

    if (pmove->clientmaxspeed != 0)
    {
        pmove->maxspeed = pmove->clientmaxspeed;
    }

#ifdef HALFLIFE_TRAINCONTROL
    if ((pmove->flags & (FL_FROZEN | FL_ONTRAIN)) != 0
#else
    if ((pmove->flags & FL_FROZEN) != 0
#endif
     || pmove->dead != 0
     || (player->m_TFState & kTFStateBuilding) != 0)
    {
        move = g_vecZero;
    }
    else
    {
        const auto speed = move.Length();
        const auto speedModifier = GetSpeedModifier();

        if (speed > speedModifier)
        {
            move = move.Normalize() * speedModifier;
        }
    }

    pmove->angles = pmove->cmd.viewangles;
    if (pmove->angles.y > 180.0F)
    {
        pmove->angles.y -= 360.0F;
    }

    if (pmove->dead != 0)
    {
        pmove->view_ofs = VEC_DEAD_VIEW;
    }

    pmove->flFallVelocity = 0.0F;

    pmove->numtouch = 0;
    pmove->frametime = pmove->cmd.msec / 1000.0F;

    AngleVectors(pmove->angles, &pmove->forward, &pmove->right, &pmove->up);
    AngleVectors(Vector(0.0F, pmove->angles.y, 0.0F), &m_flatForward, &m_flatRight, nullptr);

    BuildWishMove(move);
    BuildFreeWishMove(move);
}


void CHalfLifeMovement::NoClip()
{
    pmove->origin = pmove->origin + pmove->frametime * m_freeWishDir * m_freeWishSpeed;
    pmove->velocity = g_vecZero;
}


int CHalfLifeMovement::FlyMove()
{
    Vector originalOrigin = pmove->origin;
    Vector originalVelocity = pmove->velocity;
    const Vector primalVelocity = pmove->velocity;
    int blocked = 0;
    Vector planes[MAX_CLIP_PLANES];
    int numPlanes = 0;
    float allFraction = 0.0F;
    float timeLeft = pmove->frametime;
    int p, j, i;
    Vector end;
    Vector newVelocity;
    Vector dir;
    float d;
    pmtrace_t trace;

    for (p = 0; p < 4; p++)
    {
        if (pmove->velocity == g_vecZero)
        {
            break;
        }

        end = pmove->origin + timeLeft * pmove->velocity;

        trace = pmove->PM_PlayerTraceEx(
            pmove->origin,
            end,
            PM_STUDIO_BOX,
            CGameMovement::g_ShouldIgnore);

        allFraction += trace.fraction;

        /*
        If we started in a solid object, or we were in solid space
        the whole way, zero out our velocity and return that we
        are blocked by floor and wall
        */
        if (trace.allsolid != 0)
        {
            pmove->velocity = g_vecZero;
            pmove->Con_DPrintf("Trapped\n");
            return 4;
        }

        /*
        If we moved some portion of the total distance, then
        copy the end position into the pmove->origin and
        zero the plane counter
        */
        if (trace.fraction > 0)
        {
            pmove->origin = trace.endpos;
            originalVelocity = pmove->velocity;
            numPlanes = 0;
        }

        if (trace.fraction == 1)
        {
            /* Moved the entire distance */
            break;
        }

        AddToTouched(trace, pmove->velocity);

        if (trace.plane.normal.z >= kGroundPlaneMinZ)
        {
            /* Floor */
            blocked |= 1;
        }
        else if (trace.plane.normal.z == 0)
        {
            /* Step or wall */
            blocked |= 2;
            // pmove->Con_DPrintf("Blocked by %i\n", trace.ent);
        }

        timeLeft -= timeLeft * trace.fraction;

        if (numPlanes >= MAX_CLIP_PLANES)
        {
            pmove->velocity = g_vecZero;
            pmove->Con_DPrintf("Too many planes\n");
            break;
        }

        planes[numPlanes] = trace.plane.normal;
        numPlanes++;

        /* Modify original velocity so it parallels all of the clip planes */
        if (pmove->movetype == MOVETYPE_WALK &&
            (pmove->onground == -1 || pmove->friction != 1))
        {
            for (i = 0; i < numPlanes; i++)
            {
                if (planes[i].z >= kGroundPlaneMinZ)
                {
                    CHalfLifeMovement::ClipVelocity(
                        originalVelocity,
                        planes[i],
                        newVelocity,
                        1.0F);

                    originalVelocity = newVelocity;
                }
                else
                {
                    CHalfLifeMovement::ClipVelocity(
                        originalVelocity,
                        planes[i],
                        newVelocity,
                        1.0F + pmove->movevars->bounce * (1.0F - pmove->friction));
                }
            }

            pmove->velocity = newVelocity;
            originalVelocity = newVelocity;
        }
        else
        {
            for (i = 0; i < numPlanes; i++)
            {
                CHalfLifeMovement::ClipVelocity(
                    originalVelocity,
                    planes[i],
                    pmove->velocity,
                    1.0F);

                for (j = 0; j < numPlanes; j++)
                {
                    if (j != i)
                    {
                        /* Are we now moving against this plane? */
                        if (DotProduct(pmove->velocity, planes[j]) < 0)
                        {
                            break;
                        }
                    }
                }

                /* Didn't have to clip, so we're ok */
                if (j == numPlanes)
                {
                    break;
                }
            }

            /* Did we go all the way through plane set? */
            if (i == numPlanes)
            {
                if (numPlanes != 2)
                {
                    pmove->velocity = g_vecZero;
                    pmove->Con_DPrintf("More than 2 planes\n");
                    break;
                }
                CrossProduct(planes[0], planes[1], dir);
                d = DotProduct(dir, pmove->velocity);
                pmove->velocity = dir * d;
            }

            /*
            If original velocity is against the original velocity, stop dead
            to avoid tiny occilations in sloping corners
            */
            if (DotProduct(pmove->velocity, primalVelocity) <= 0)
            {
                pmove->velocity = g_vecZero;
                // pmove->Con_DPrintf("Back\n");
                break;
            }
        }
    }

    if (allFraction == 0)
    {
        pmove->velocity = g_vecZero;
        // pmove->Con_DPrintf("Don't stick\n");
    }

    return blocked;
}


void CHalfLifeMovement::CategorizePosition()
{
    if (pmove->movetype == MOVETYPE_NONE
     || pmove->movetype == MOVETYPE_NOCLIP
     || pmove->movetype == MOVETYPE_FLY)
    {
        pmove->waterlevel = kWaterLevelNone;
        pmove->watertype = CONTENTS_EMPTY;
        pmove->onground = -1;
        return;
    }

    CheckContents();

    if (IsSubmerged())
    {
        pmove->onground = -1;
        return;
    }

    if (pmove->velocity.z > 180)
    {
        pmove->onground = -1;
        return;
    }

    const auto wasOnGround = pmove->onground != -1;

    Vector point = pmove->origin;
    point.z -= 2;
    pmtrace_t trace = pmove->PM_PlayerTraceEx(
        pmove->origin,
        point,
        PM_STUDIO_BOX,
        CGameMovement::g_ShouldIgnore);

    if (trace.plane.normal.z < kGroundPlaneMinZ)
    {
        pmove->onground = -1;
    }
    else
    {
        pmove->onground = trace.ent;
    }

    if (pmove->onground != -1)
    {
        if (trace.startsolid == 0 && trace.allsolid == 0)
        {
            pmove->origin = trace.endpos;
        }

        if (!wasOnGround)
        {
            pmove->flTimeStepSound = 0;
        }
    }

    if (trace.ent > 0)
    {
        AddToTouched(trace, pmove->velocity);
    }
}


void CHalfLifeMovement::Accelerate(const Vector& wishDir, float wishSpeed)
{
    if (wishSpeed <= 0)
    {
        return;
    }

    float testSpeed = wishSpeed;

    if (!IsSubmerged() && pmove->onground == -1 && testSpeed > 30)
    {
        testSpeed = 30;
    }

    const float addSpeed =
        testSpeed - DotProduct(pmove->velocity, wishDir);

    if (addSpeed <= 0)
    {
        return;
    }

    float acceleration = pmove->movevars->accelerate;
    if (IsSubmerged())
    {
        acceleration = pmove->movevars->wateraccelerate;
    }
    else if (pmove->onground == -1)
    {
        acceleration = pmove->movevars->airaccelerate;
    }

    const float accelSpeed =
        std::min(
            acceleration * pmove->frametime * wishSpeed * pmove->friction,
            addSpeed);

    pmove->velocity = pmove->velocity + accelSpeed * wishDir;
}


int PM_GetRandomStuckOffsets(int nIndex, Vector& offset);
void PM_ResetStuckOffsets(int nIndex);
bool PM_TryToUnstuck(Vector base, int (*pfnIgnore)(physent_t *pe));

bool CHalfLifeMovement::IsStuck()
{
    int r, i;
    pmtrace_t trace;

    int hitent = pmove->PM_TestPlayerPositionEx(
        pmove->origin,
        &trace,
        CGameMovement::g_ShouldIgnore);

    if (hitent == -1)
    {
        PM_ResetStuckOffsets(pmove->player_index);
        return false;
    }

    const Vector originalOrigin = pmove->origin;
    Vector testPosition, offset;

#ifdef CLIENT_DLL
    PM_ResetStuckOffsets(pmove->player_index);
    for (r = 0; r < 54; r++)
    {
        i = PM_GetRandomStuckOffsets(pmove->player_index, offset);
        testPosition = originalOrigin + offset;

        if (pmove->PM_TestPlayerPositionEx(
            testPosition,
            &trace,
            CGameMovement::g_ShouldIgnore) == -1)
        {
            PM_ResetStuckOffsets(pmove->player_index);
            pmove->origin = testPosition;
            return false;
        }
    }
#endif

    pmove->PM_StuckTouch(hitent, &trace);

    i = PM_GetRandomStuckOffsets(pmove->player_index, offset);
    testPosition = originalOrigin + offset;

    if (pmove->PM_TestPlayerPositionEx(
        testPosition,
        nullptr,
        CGameMovement::g_ShouldIgnore) == -1)
    {
        PM_ResetStuckOffsets(pmove->player_index);
        if (i >= 27)
        {
            pmove->origin = testPosition;
        }
        return false;
    }

    /*
    If player is flailing while stuck in another player (should never happen),
    then see if we can't "unstick" them forceably.
    */
#ifdef GAME_DLL
    if (pmove->cmd.buttons != 0 && pmove->physents[hitent].player != 0)
    {
        if (!PM_TryToUnstuck(originalOrigin, CGameMovement::g_ShouldIgnore))
        {
            return false;
        }
    }
#endif

    return true;
}


void CHalfLifeMovement::CheckVelocity()
{
    const float speed = pmove->velocity.Make2D().LengthSquared();

    if (speed < 0.1)
    {
        pmove->velocity.x = pmove->velocity.y = 0;
        return;
    }

    float maxSpeed = pmove->maxspeed;

    if (pmove->onground == -1
     || player->pev->playerclass == PC_SCOUT)
    {
        maxSpeed = pmove->movevars->maxvelocity;
    }
    else if (m_wishSpeed > pmove->movevars->stopspeed
     && m_wishSpeed < maxSpeed)
    {
        maxSpeed = m_wishSpeed;
    }

    const float oldZed =
        std::clamp(
            pmove->velocity.z,
            -pmove->movevars->maxvelocity,
            pmove->movevars->maxvelocity);

    if (speed > maxSpeed * maxSpeed)
    {
        pmove->velocity =
            pmove->velocity / sqrtf(speed) * maxSpeed;
    }

    pmove->velocity.z = oldZed;
}


void CHalfLifeMovement::AddCorrectGravity()
{
    float gravity = pmove->gravity;
    if (gravity == 0)
    {
        gravity = 1;
    }

    float envGravity = pmove->movevars->gravity;
    if (IsSubmerged())
    {
        envGravity *= 0.075F;
    }

    pmove->velocity.z -=
        gravity * envGravity * pmove->frametime * 0.5F;

    CheckVelocity();
}


void CHalfLifeMovement::FixUpGravity()
{
    float gravity = pmove->gravity;
    if (gravity == 0)
    {
        gravity = 1;
    }

    float envGravity = pmove->movevars->gravity;
    if (IsSubmerged())
    {
        envGravity *= 0.075F;
    }

    pmove->velocity.z -=
        gravity * envGravity * pmove->frametime * 0.5F;

    CheckVelocity();
}


bool CHalfLifeMovement::AddToTouched(pmtrace_t& tr, Vector& velocity)
{
    int i;
    for (i = 0; i < pmove->numtouch; i++)
    {
        if (pmove->touchindex[i].ent == tr.ent)
        {
            break;
        }
    }
    if (i != pmove->numtouch)
    {
        return false;
    }

    tr.deltavelocity = velocity;

    if (pmove->numtouch >= MAX_PHYSENTS)
    {
        pmove->Con_DPrintf("Too many entities were touched!\n");
    }

    pmove->touchindex[pmove->numtouch++] = tr;
    return true;
}


void CHalfLifeMovement::ClipVelocity(
    const Vector& in,
    const Vector& normal,
    Vector& out,
    const float overbounce)
{
    const float backoff = DotProduct(in, normal) * overbounce;

    out = in - normal * backoff;

    if (out.x > -0.1 && out.x < 0.1)
    {
        out.x = 0;
    }
    if (out.y > -0.1 && out.y < 0.1)
    {
        out.y = 0;
    }
    if (out.z > -0.1 && out.z < 0.1)
    {
        out.z = 0;
    }
}

