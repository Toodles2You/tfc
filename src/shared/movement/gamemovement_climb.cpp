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

typedef enum {
	mod_brush,
	mod_sprite,
	mod_alias,
	mod_studio
} modtype_t;

typedef struct
{
	int planenum;
	short children[2]; // negative numbers are contents
} dclipnode_t;

typedef struct mplane_s
{
	Vector normal; // surface normal
	float dist;	   // closest appoach to origin
	byte type;	   // for texture axis selection and fast side tests
	byte signbits; // signx + signy<<1 + signz<<1
	byte pad[2];
} mplane_t;

typedef struct hull_s
{
	dclipnode_t* clipnodes;
	mplane_t* planes;
	int firstclipnode;
	int lastclipnode;
	Vector clip_mins;
	Vector clip_maxs;
} hull_t;


bool CHalfLifeMovement::Climb(physent_t* ladder)
{
	Vector ladderCenter;
	trace_t trace;
	Vector floor;
	Vector modelmins, modelmaxs;

	pmove->PM_GetModelBounds(ladder->model, modelmins, modelmaxs);

    ladderCenter = (modelmins + modelmaxs) / 2.0F;

	/* On ladder, convert movement to be relative to the ladder. */

    floor = pmove->origin;
	floor.z += pmove->player_mins[pmove->usehull].z - 1;

	const bool onFloor = pmove->PM_PointContents(floor, nullptr) == CONTENTS_SOLID;

	CGameMovement::TraceModel(ladder, pmove->origin, ladderCenter, &trace);
	if (trace.fraction == 1.0F)
    {
        return false;
    }
    
    float forward = 0, right = 0;
    Vector vpn, v_right;

    AngleVectors(pmove->angles, &vpn, &v_right, nullptr);

    if ((pmove->cmd.buttons & IN_BACK) != 0)
    {
        forward -= 1;
    }
    if ((pmove->cmd.buttons & IN_FORWARD) != 0)
    {
        forward += 1;
    }
    if ((pmove->cmd.buttons & IN_MOVELEFT) != 0)
    {
        right -= 1;
    }
    if ((pmove->cmd.buttons & IN_MOVERIGHT) != 0)
    {
        right += 1;
    }

    if ((pmove->cmd.buttons & IN_JUMP) != 0)
    {
        pmove->velocity = trace.plane.normal * 270.0F;
        return false;
    }

    if (forward == 0 && right == 0)
    {
        pmove->velocity = g_vecZero;
        FlyMove();
        return true;
    }

    Vector velocity, perp, cross, lateral, tmp;
    float normal;

    // Calculate player's intended velocity
    velocity = vpn * forward + v_right * right;


    // Perpendicular in the ladder plane
    //					Vector perp = CrossProduct( Vector(0,0,1), trace.vecPlaneNormal );
    //					perp = perp.Normalize();
    tmp = Vector(0.0F, 0.0F, 1.0F);
    CrossProduct(tmp, trace.plane.normal, perp);
    perp.NormalizeInPlace();


    // decompose velocity into ladder plane
    normal = DotProduct(velocity, trace.plane.normal);
    // This is the velocity into the face of the ladder
    cross = trace.plane.normal * normal;


    // This is the player's additional velocity
    lateral = velocity - cross;

    // This turns the velocity into the face of the ladder into velocity that
    // is roughly vertically perpendicular to the face of the ladder.
    // NOTE: It IS possible to face up and move down or face down and move up
    // because the velocity is a sum of the directional velocity and the converted
    // velocity through the face of the ladder -- by design.
    CrossProduct(trace.plane.normal, perp, tmp);
    pmove->velocity = (lateral + tmp * -normal).Normalize();

    float speed = std::min(pmove->maxspeed, 200.0F);

    pmove->velocity.x = pmove->velocity.x * speed;
    pmove->velocity.y = pmove->velocity.y * speed;
    pmove->velocity.z = pmove->velocity.z * speed;

    if (onFloor && normal > 0) // On ground moving away from the ladder
    {
        pmove->velocity = pmove->velocity + trace.plane.normal * 200;
    }

    FlyMove();
    return true;
}


bool CHalfLifeMovement::CheckLadder()
{
	for (int i = 0; i < pmove->nummoveent; i++)
	{
		physent_t* physent = &pmove->moveents[i];

        if (physent->skin != CONTENTS_LADDER)
        {
            continue;
        }

		if (physent->model == nullptr)
        {
            continue;
        }

        if ((modtype_t)pmove->PM_GetModelType(physent->model) != mod_brush)
        {
            continue;
        }

    	Vector testPoint;
        hull_t* hull = (hull_t*)pmove->PM_HullForBsp(physent, testPoint);

        /* Test the player's hull for intersection with this model. */
        if (pmove->PM_HullPointContents(
            hull,
            hull->firstclipnode,
            pmove->origin - testPoint) == CONTENTS_EMPTY)
        {
            continue;
        }

        return Climb(physent);
	}

    return false;
}

