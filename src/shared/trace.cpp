//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Trace wrapper
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

#include "trace.h"

#ifdef CLIENT_DLL
#include "cl_dll.h"
#include "event_api.h"
#endif

Trace::Trace()
{
    result = 0;
    fraction = 1.0F;
    endpos = g_vecZero;
    normal = g_vecZero;
    entity = -1;
    hitgroup = 0;
}

#ifdef GAME_DLL

Trace::Trace(const Vector& start, const Vector& end, int ignore, int traceFlags)
{
    int flags = 0;
    if ((traceFlags & kIgnoreMonsters) != 0) flags |= 1;
    if ((traceFlags & kIgnoreGlass) != 0)    flags |= 256;
    if ((traceFlags & kBox) != 0)            gpGlobals->trace_flags |= FTRACE_SIMPLEBOX;

	TraceResult trace;

    engine::TraceLine(
        const_cast<Vector&>(start),
        const_cast<Vector&>(end),
        flags,
        engine::PEntityOfEntIndex(ignore),
        &trace);

    result = 0;
    if (trace.fAllSolid)   result |= kResultAllSolid;
    if (trace.fStartSolid) result |= kResultStartSolid;
    if (trace.fInOpen)     result |= kResultInOpen;
    if (trace.fInWater)    result |= kResultInWater;

    fraction = trace.flFraction;
    endpos = trace.vecEndPos;
    normal = trace.vecPlaneNormal;
    if (trace.pHit != nullptr)
    {
        entity = engine::IndexOfEdict(trace.pHit);
    }
    else
    {
        entity = -1;
    }
    hitgroup = trace.iHitgroup;
}

#endif

#ifdef CLIENT_DLL

Trace::Trace(const Vector& start, const Vector& end, int ignore, int traceFlags)
{
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(ignore - 1);
	gEngfuncs.pEventAPI->EV_SetTraceHull(kHullPoint);

    int flags = PM_NORMAL;
    if ((traceFlags & kIgnoreMonsters) != 0) flags |= PM_STUDIO_IGNORE;
    if ((traceFlags & kIgnoreGlass) != 0)    flags |= PM_GLASS_IGNORE;
    if ((traceFlags & kBox) != 0)            flags |= PM_STUDIO_BOX;

    pmtrace_t trace;

    gEngfuncs.pEventAPI->EV_PlayerTrace(
        const_cast<Vector&>(start),
        const_cast<Vector&>(end),
        flags,
        -1,
        &trace);

    result = 0;
    if (trace.allsolid)   result |= kResultAllSolid;
    if (trace.startsolid) result |= kResultStartSolid;
    if (trace.inopen)     result |= kResultInOpen;
    if (trace.inwater)    result |= kResultInWater;

    fraction = trace.fraction;
    endpos = trace.endpos;
    normal = trace.plane.normal;
    entity = trace.ent;
    hitgroup = trace.hitgroup;
}

#endif

