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

//
// pm_shared.h
//

#pragma once

#include "Platform.h"

struct playermove_s;

void PM_Init(playermove_s* ppmove);
#ifdef GAME_DLL
void PM_Move(playermove_s* ppmove, qboolean server);
#endif
char PM_FindTextureType(const char* name);

/**
*	@brief Engine calls this to enumerate player collision hulls, for prediction. Return false if the hullnumber doesn't exist.
*/
bool PM_GetHullBounds(int hullnumber, float* mins, float* maxs);

extern playermove_s* pmove;

inline bool g_CheckForPlayerStuck = false;
