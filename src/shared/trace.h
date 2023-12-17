//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Trace wrapper
//
// $NoKeywords: $
//=============================================================================

#pragma once

#include "vector.h"


class Trace
{
public:
    enum
    {
        kIgnoreMonsters = 1,
        kIgnoreGlass    = 2,
        kBox            = 4, /* Trace against bounding boxes, rather than models */
        kBoxModel       = 8, /* Perform a second trace after the box trace to check for a body part hit */
    };

    Trace();
    Trace(const Vector& start, const Vector& end, int ignore = -1, int traceFlags = 0);

    enum
    {
        kResultAllSolid   = 1,
        kResultStartSolid = 2,
        kResultInOpen     = 4,
        kResultInWater    = 8,
    };

    int result;
    float fraction;
    Vector endpos;
    Vector normal;
    int entity;
    int hitgroup;
};

