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


void CHalfLifeMovement::CheckContents()
{
    pmove->waterlevel = kWaterLevelNone;
    pmove->watertype = CONTENTS_EMPTY;

    if (pmove->movetype == MOVETYPE_NOCLIP)
    {
        return;
    }

    /* Pick a spot just above the players feet. */
    Vector point =
        pmove->origin + (pmove->player_mins[pmove->usehull] + pmove->player_maxs[pmove->usehull]) / 2.0F;

    point.z = pmove->origin.z + pmove->player_mins[pmove->usehull].z + 1;

    /* Grab point contents. */
    int contents;
    int trueContents;
    contents = pmove->PM_PointContents(point, nullptr);

    if (contents <= CONTENTS_WATER && contents >= CONTENTS_SKY)
    {
        /* Set water type. We are at least at level one. */
        pmove->watertype = contents;
        pmove->waterlevel = kWaterLevelFeet;

        /* Now check a point that is at the player hull midpoint. */
        point.z = pmove->origin.z + (pmove->player_mins[pmove->usehull].z + pmove->player_maxs[pmove->usehull].z) / 2.0F;

        contents = pmove->PM_PointContents(point, &trueContents);

        if (contents <= CONTENTS_WATER && contents >= CONTENTS_SKY)
        {
            /* In over our waist. */
            pmove->waterlevel = kWaterLevelWaist;

            /* Now check the eye position. */
            point.z = pmove->origin.z + pmove->view_ofs.z;

            contents = pmove->PM_PointContents(point, nullptr);

            if (contents <= CONTENTS_WATER && contents >= CONTENTS_SKY)
            {
                /* In over our eyes. */
                pmove->waterlevel = kWaterLevelEyes;
            }
        }

        /* Adjust velocity based on water current, if any. */
        if (trueContents <= CONTENTS_CURRENT_0 && trueContents >= CONTENTS_CURRENT_DOWN)
        {
            static constexpr Vector current_table[] =
            {
                {1, 0, 0}, {0, 1, 0}, {-1, 0, 0},
                {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
            };

            /* The deeper we are, the stronger the current. */
            pmove->basevelocity = current_table[CONTENTS_CURRENT_0 - trueContents] * 50.0F * pmove->waterlevel;
        }
    }
}


void CHalfLifeMovement::Swim()
{
    /* If ye ain't swimmin', drift down towards Davy Jones's locker! */
    if (m_freeWishSpeed == 0.0F)
    {
        AddCorrectGravity();
    }
    ApplyFriction();
    CheckVelocity();

    Accelerate(m_freeWishDir, m_freeWishSpeed);
    pmove->velocity = pmove->velocity + pmove->basevelocity;

    FlyMove();
    CategorizePosition();

    pmove->velocity = pmove->velocity - pmove->basevelocity;

    CheckVelocity();

    if (m_freeWishSpeed == 0.0F)
    {
        FixUpGravity();
    }
}

