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
}


void CHalfLifeMovement::Move()
{
    CGameMovement::Move();

    if (pmove->movetype == MOVETYPE_NONE)
    {
        return;
    }

    CheckParameters();
    CategorizePosition();

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
        Walk();
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

    if (pmove->movetype == MOVETYPE_WALK)
    {
        pmove->friction = 1;
    }
}


void CHalfLifeMovement::CheckParameters()
{
    float speedScale = 1.0F;
    Vector move =
    {
        *reinterpret_cast<int*>(&pmove->cmd.forwardmove),
        *reinterpret_cast<int*>(&pmove->cmd.sidemove),
        *reinterpret_cast<int*>(&pmove->cmd.upmove)
    };

    move = move / 100.0F;

    if (player->m_nLegDamage != 0)
    {
        speedScale *= (10.0F - player->m_nLegDamage) / 10.0F;
    }

    if ((pmove->cmd.buttons & IN_SPEED) != 0
     || (player->m_TFState & kTFStateAiming) != 0)
    {
        speedScale = std::min(speedScale, 0.3F);
    }

    float speed = move.Length();

    if (pmove->clientmaxspeed != 0)
    {
        pmove->maxspeed = pmove->clientmaxspeed;
    }

    if ((pmove->flags & (FL_FROZEN | FL_ONTRAIN)) != 0 || pmove->dead != 0)
    {
        move = g_vecZero;
    }
    else if (speed > speedScale)
    {
        speed = speedScale;
        move = move.Normalize() * speedScale;
    }

    pmove->angles = pmove->cmd.viewangles;
    if (pmove->angles.y > 180.0f)
    {
        pmove->angles.y -= 360.0f;
    }

    if (pmove->dead != 0)
    {
        pmove->view_ofs = VEC_DEAD_VIEW;
    }

    pmove->numtouch = 0;
    pmove->frametime = pmove->cmd.msec / 1000.0F;

    AngleVectors(pmove->angles, &pmove->forward, &pmove->right, &pmove->up);
    AngleVectors(Vector(0, pmove->angles.y, 0), &m_flatForward, &m_flatRight, nullptr);

    if (pmove->movetype == MOVETYPE_NOCLIP
     || pmove->movetype == MOVETYPE_FLY)
    {
        m_wishVel =
            pmove->forward * move.x
            + pmove->right * move.y;
    }
    else
    {
        m_wishVel =
            m_flatForward * move.x
            + m_flatRight * move.y;
    }

    m_wishVel.z += move.z;

    m_wishVel = m_wishVel * pmove->maxspeed;

    m_wishDir = m_wishVel.Normalize();
    m_wishSpeed = m_wishVel.Length();

    if (m_wishSpeed > pmove->maxspeed)
    {
        m_wishSpeed = pmove->maxspeed;
        m_wishVel = m_wishDir * m_wishSpeed;
    }
}


void CHalfLifeMovement::NoClip()
{
    pmove->origin = pmove->origin + pmove->frametime * m_wishVel;
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
    pmove->waterlevel = 0;
    pmove->watertype = CONTENTS_EMPTY;

    if (pmove->velocity.z > 180)
    {
        pmove->onground = -1;
        return;
    }

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

    if (pmove->onground == -1 && wishSpeed > 30)
    {
        wishSpeed = 30;
    }

    const float addSpeed =
        wishSpeed - DotProduct(pmove->velocity, wishDir);

    if (addSpeed <= 0)
    {
        return;
    }

    float acceleration = pmove->movevars->accelerate;
    if (pmove->onground == -1)
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

    pmove->velocity.z -=
        gravity * pmove->movevars->gravity * pmove->frametime * 0.5F;

    CheckVelocity();
}


void CHalfLifeMovement::FixUpGravity()
{
    float gravity = pmove->gravity;
    if (gravity == 0)
    {
        gravity = 1;
    }

    pmove->velocity.z -=
        gravity * pmove->movevars->gravity * pmove->frametime * 0.5F;

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

