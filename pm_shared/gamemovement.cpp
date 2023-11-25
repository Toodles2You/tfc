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


CGameMovement::CGameMovement(struct playermove_s* _pmove, CBasePlayer* _player)
    : pmove{_pmove}, player{_player}
{
#ifndef CLIENT_DLL
    pmove->Con_Printf("CGameMovement: Initialized\n");
#endif
}


CGameMovement::~CGameMovement()
{
#ifndef CLIENT_DLL
    pmove->Con_Printf("CGameMovement: Cleaned up\n");
#endif
}


void CGameMovement::Move()
{
    if (pmove->movetype == MOVETYPE_NONE)
    {
        return;
    }

    CheckParameters();
    NoClip();

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
        pmove->friction = 1.0f;
    }
}


void CGameMovement::CheckParameters()
{
    auto speed =
        sqrtf((pmove->cmd.forwardmove * pmove->cmd.forwardmove)
            + (pmove->cmd.sidemove * pmove->cmd.sidemove)
            + (pmove->cmd.upmove * pmove->cmd.upmove));

    if (pmove->clientmaxspeed != 0.0)
    {
        pmove->maxspeed = std::min(pmove->clientmaxspeed, pmove->maxspeed);
    }

    if (speed != 0.0 && speed > pmove->maxspeed)
    {
        auto rat = pmove->maxspeed / speed;
        pmove->cmd.forwardmove *= rat;
        pmove->cmd.sidemove *= rat;
        pmove->cmd.upmove *= rat;
    }

    if ((pmove->flags & (FL_FROZEN | FL_ONTRAIN)) != 0 || pmove->dead != 0)
    {
        pmove->cmd.forwardmove = 0;
        pmove->cmd.sidemove = 0;
        pmove->cmd.upmove = 0;
    }

    pmove->angles = pmove->cmd.viewangles;
    if (pmove->angles.y > 180.0f)
    {
        pmove->angles.y -= 360.0f;
    }

    if (0 != pmove->dead)
    {
        pmove->view_ofs = VEC_DEAD_VIEW;
    }

#ifndef CLIENT_DLL
    pmove->server = true;
#else
    pmove->server = false;
#endif

    pmove->numtouch = 0;
    pmove->frametime = pmove->cmd.msec * 0.001;

    AngleVectors(pmove->angles, &pmove->forward, &pmove->right, &pmove->up);
}


void CGameMovement::NoClip()
{
    auto wishvel =
        pmove->forward * pmove->cmd.forwardmove
        + pmove->right * pmove->cmd.sidemove;

    wishvel.z += pmove->cmd.upmove;

    pmove->origin = pmove->origin + pmove->frametime * wishvel;

    pmove->velocity = g_vecZero;
}
