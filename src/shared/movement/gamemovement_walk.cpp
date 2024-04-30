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

#ifdef CLIENT_DLL
#include "view.h"
#endif


void CHalfLifeMovement::Walk()
{
    AddCorrectGravity();

    if (pmove->onground == -1)
    {
        pmove->flFallVelocity = -pmove->velocity.z;
    }

    if ((pmove->cmd.buttons & IN_JUMP) != 0
     && (pmove->oldbuttons & IN_JUMP) == 0)
    {
        Jump();
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
        CheckStepSound();
        CheckFalling();
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
        pmove->PM_PlayerTraceEx(
            pmove->origin,
            dest,
            PM_STUDIO_BOX,
            CGameMovement::g_ShouldIgnore);

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

    trace = pmove->PM_PlayerTraceEx(
        pmove->origin,
        dest,
        PM_STUDIO_BOX,
        CGameMovement::g_ShouldIgnore);

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

    trace = pmove->PM_PlayerTraceEx(
        pmove->origin,
        dest,
        PM_STUDIO_BOX,
        CGameMovement::g_ShouldIgnore);

    /*
    If we are not on the ground any more then
    use the original movement attempt
    */
    if (trace.plane.normal.z < kGroundPlaneMinZ)
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

    if ((player->m_TFState & kTFStateAiming) != 0)
    {
        return;
    }

    if (pmove->onground == -1)
    {
        return;
    }

    pmove->onground = -1;

    player->SetAction(CBasePlayer::Action::Jump);

    pmove->PM_PlaySound(CHAN_BODY, "player/plyrjmp8.wav", 0.5, ATTN_NORM, 0, PITCH_NORM);

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

    if (IsSubmerged())
    {
        float friction =
            pmove->movevars->friction
                * pmove->movevars->waterfriction
                * pmove->friction;

        drop +=
            std::max(speed, pmove->movevars->stopspeed)
            * friction
            * pmove->frametime;
    }
    else if (pmove->onground != -1)
    {
        Vector start, stop;

        start.x = stop.x = pmove->origin.x + pmove->velocity.x / speed * 16;
        start.y = stop.y = pmove->origin.y + pmove->velocity.y / speed * 16;
        start.z = pmove->origin.z + pmove->player_mins[pmove->usehull].z;
        stop.z = start.z - 34;

        pmtrace_t trace =
            pmove->PM_PlayerTraceEx(
                start,
                stop,
                PM_STUDIO_BOX,
                CGameMovement::g_ShouldIgnore);

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

    float oldZed = pmove->velocity.z;

    float newSpeed = speed - drop;
    if (newSpeed < 0)
    {
        newSpeed = 0;
    }
    newSpeed /= speed;
    pmove->velocity = pmove->velocity * newSpeed;

    /* FIXME: */
    if (IsSubmerged() && oldZed <= 0.0F)
    {
        float gravity =pmove->gravity;
        if (gravity == 0)
        {
            gravity = 1;
        }
        if (oldZed >= gravity * -pmove->movevars->gravity * 0.075F)
        {
            pmove->velocity.z = oldZed;
        }
    }
}


void CHalfLifeMovement::CheckStepSound()
{
    if (pmove->movevars->footsteps == 0)
    {
        return;
    }

    if (pmove->onground != -1)
    {
        if (pmove->velocity.x == 0.0F && pmove->velocity.y == 0.0F)
        {
            return;
        }
    }
    else if (pmove->velocity.z == 0.0F)
    {
        return;
    }

    auto speed = pmove->velocity.Make2D().Length();
    const auto isRunning = speed > 200;

    if (pmove->onground == -1)
    {
        speed = 1.5F;
    }
    else if (isRunning)
    {
        speed = 2.0F * (speed / 300.0F);
    }
    else
    {
        speed = 1.5F * (speed / 100.0F);
    }

    pmove->flTimeStepSound = std::max((int)pmove->flTimeStepSound - (int)(pmove->cmd.msec * speed), -600);

    if (pmove->flTimeStepSound <= 0)
    {
        StepSound();
    }
}


void CHalfLifeMovement::StepSound()
{
    const auto speed = pmove->velocity.Make2D().Length();
    const auto isRunning = speed > 200 || pmove->onground == -1;

    pmove->iStepLeft = (pmove->iStepLeft != 0) ? 0 : 1;

    const auto random = 2 * pmove->iStepLeft + pmove->RandomLong(0, 1);
    const char* sample;

    char c;
    if (pmove->onground == -1)
    {
        c = 'L';
    }
    else if (pmove->waterlevel > kWaterLevelNone)
    {
        c = CHAR_TEX_SLOSH;
    }
    else
    {
        CategorizeGround();
        c = pmove->chtexturetype;
    }

    switch (c)
    {
        default:
        case CHAR_TEX_CONCRETE:
            switch (random)
            {
                default: sample = "player/pl_step1.wav"; break;
                case 1:  sample = "player/pl_step3.wav"; break;
                case 2:  sample = "player/pl_step2.wav"; break;
                case 3:  sample = "player/pl_step4.wav"; break;
            }
            break;
        case CHAR_TEX_METAL:
            switch (random)
            {
                default: sample = "player/pl_metal1.wav"; break;
                case 1:  sample = "player/pl_metal3.wav"; break;
                case 2:  sample = "player/pl_metal2.wav"; break;
                case 3:  sample = "player/pl_metal4.wav"; break;
            }
            break;
        case CHAR_TEX_DIRT:
            switch (random)
            {
                default: sample = "player/pl_dirt1.wav"; break;
                case 1:  sample = "player/pl_dirt3.wav"; break;
                case 2:  sample = "player/pl_dirt2.wav"; break;
                case 3:  sample = "player/pl_dirt4.wav"; break;
            }
            break;
        case CHAR_TEX_VENT:
            switch (random)
            {
                default: sample = "player/pl_duct1.wav"; break;
                case 1:  sample = "player/pl_duct3.wav"; break;
                case 2:  sample = "player/pl_duct2.wav"; break;
                case 3:  sample = "player/pl_duct4.wav"; break;
            }
            break;
        case CHAR_TEX_TILE:
            if (pmove->RandomLong(0, 4) == 0)
            {
                sample = "player/pl_tile5.wav";
                break;
            }

            switch (random)
            {
                default: sample = "player/pl_tile1.wav"; break;
                case 1:  sample = "player/pl_tile3.wav"; break;
                case 2:  sample = "player/pl_tile2.wav"; break;
                case 3:  sample = "player/pl_tile4.wav"; break;
            }
            break;
        case CHAR_TEX_SLOSH:
            if (pmove->waterlevel >= kWaterLevelWaist)
            {
                switch (random)
                {
                    default: sample = "player/pl_wade1.wav"; break;
                    case 1:  sample = "player/pl_wade3.wav"; break;
                    case 2:  sample = "player/pl_wade2.wav"; break;
                    case 3:  sample = "player/pl_wade4.wav"; break;
                }
            }
            else
            {
                switch (random)
                {
                    default: sample = "player/pl_slosh1.wav"; break;
                    case 1:  sample = "player/pl_slosh3.wav"; break;
                    case 2:  sample = "player/pl_slosh2.wav"; break;
                    case 3:  sample = "player/pl_slosh4.wav"; break;
                }
            }
            break;
        case 'L':
            switch (random)
            {
                default: sample = "player/pl_ladder1.wav"; break;
                case 1:  sample = "player/pl_ladder3.wav"; break;
                case 2:  sample = "player/pl_ladder2.wav"; break;
                case 3:  sample = "player/pl_ladder4.wav"; break;
            }
            break;
    }

    pmove->PM_PlaySound(CHAN_BODY, sample, isRunning ? 0.5F : 0.2F, ATTN_IDLE, 0, PITCH_NORM);

    pmove->flTimeStepSound += 600;
}


void CHalfLifeMovement::CategorizeGround()
{   
    Vector start = pmove->origin;
    Vector end = start + Vector(0.0F, 0.0F, -64.0F);

    /* Fill in default values, just in case. */
    pmove->sztexturename[0] = '\0';
    pmove->chtexturetype = CHAR_TEX_CONCRETE;

    const char* name = pmove->PM_TraceTexture(pmove->onground, start, end);

    if (name == nullptr || name[0] == '\0')
    {
        return;
    }

    /* Strip leading characters. */
    if (*name == '-' || *name == '+')
    {
        name += 2;
    }
    if (*name == '{' || *name == '!' || *name == '~' || *name == ' ')
    {
        name++;
    }

    strncpy(pmove->sztexturename, name, CBTEXTURENAMEMAX - 1);
    pmove->sztexturename[CBTEXTURENAMEMAX - 1] = '\0';

    pmove->chtexturetype = PM_FindTextureType(pmove->sztexturename);
}


void CHalfLifeMovement::StayOnGround()
{
    pmtrace_t trace;
    Vector start = pmove->origin;
    Vector end = pmove->origin;
    start.z += 2;
    end.z -= pmove->movevars->stepsize;

    trace = pmove->PM_PlayerTraceEx(
        pmove->origin,
        start,
        PM_STUDIO_BOX,
        CGameMovement::g_ShouldIgnore);

    start = trace.endpos;

    trace = pmove->PM_PlayerTraceEx(
        start,
        end,
        PM_STUDIO_BOX,
        CGameMovement::g_ShouldIgnore);

    if (trace.fraction != 0.0F
     && trace.fraction != 1.0F
     && trace.startsolid == 0
     && trace.plane.normal.z >= kGroundPlaneMinZ
     && fabsf(pmove->origin.z - trace.endpos.z) > 0.5F)
    {
        pmove->origin = trace.endpos;
    }
}


void CHalfLifeMovement::CheckFalling()
{
    if (pmove->dead == 0
     && pmove->flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD
     && pmove->waterlevel <= kWaterLevelNone)
    {
        if (pmove->flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED)
        {
            pmove->PM_PlaySound(CHAN_VOICE, "player/pl_fallpain3.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
#ifdef CLIENT_DLL
            V_PunchAxis(2, std::min(pmove->flFallVelocity * 0.013F, 8.0F));
#endif
        }
        else if (pmove->flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED / 2)
        {
            pmove->PM_PlaySound(CHAN_VOICE, "player/pl_jumpland2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
#ifdef CLIENT_DLL
            V_PunchAxis(2, std::min(pmove->flFallVelocity * 0.0065F, 4.0F));
#endif
        }

        pmove->flTimeStepSound = 0;
    }

    pmove->velocity.z = 0;
    pmove->flFallVelocity = 0;
}
