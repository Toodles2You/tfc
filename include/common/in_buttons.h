/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#pragma once

enum
{
    IN_ATTACK    = 1 << 0,
    IN_JUMP      = 1 << 1,
    IN_DUCK      = 1 << 2,
    IN_FORWARD   = 1 << 3,
    IN_BACK      = 1 << 4,
    IN_USE       = 1 << 5,
    IN_CANCEL    = 1 << 6,
    IN_LEFT      = 1 << 7,
    IN_RIGHT     = 1 << 8,
    IN_MOVELEFT  = 1 << 9,
    IN_MOVERIGHT = 1 << 10,
    IN_ATTACK2   = 1 << 11,
    IN_RUN       = 1 << 12,
    IN_RELOAD    = 1 << 13,
};
