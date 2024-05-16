/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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

#define ACTIVITY_NOT_AVAILABLE -1

#include "monsterevent.h"

namespace studio
{
int LookupActivity(void* pmodel, int activity);
int LookupActivityHeaviest(void* pmodel, int activity);
int LookupSequence(void* pmodel, const char* label);
void GetSequenceInfo(void* pmodel, int sequence, float* pflFrameRate, float* pflGroundSpeed);
int GetSequenceFlags(void* pmodel, int sequence);
float SetController(void* pmodel, byte* controller, int iController, float flValue);
float SetBlending(void* pmodel, int sequence, byte* blending, int iBlender, float flValue);
void GetEyePosition(void* pmodel, Vector& vecEyePosition);
#ifdef GAME_DLL
void SequencePrecache(void* pmodel, const char* pSequenceName);
#endif
int FindTransition(void* pmodel, int iEndingAnim, int iGoalAnim, int* piDir);
void SetBodygroup(void* pmodel, int& body, int iGroup, int iValue);
int GetBodygroup(void* pmodel, int body, int iGroup);

int GetAnimationEvent(void* pmodel, int sequence, MonsterEvent_t* pMonsterEvent, float flStart, float flEnd, int index);
bool ExtractBbox(void* pmodel, int sequence, Vector& mins, Vector& maxs);
} /* namespace util */

// From /engine/studio.h
#define STUDIO_LOOPING 0x0001
