//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// com_weapons.h
// Shared weapons common function prototypes

#pragma once

#include "Exports.h"
#include "const.h"

void COM_Log(const char* pszFile, const char* fmt, ...);
bool CL_IsDead();

namespace util
{
float SharedRandomFloat(unsigned int seed, float low, float high);
int SharedRandomLong(unsigned int seed, int low, int high);

gamemode_e GetGameMode();
bool IsMultiplayer();
bool IsDeathmatch();
} /* namespace util */

int HUD_GetWeaponAnim();
void HUD_SendWeaponAnim(int iAnim, int body, bool force);
void HUD_PlaySound(const char* sound, float volume);
void HUD_PlaybackEvent(int flags, const struct edict_s* pInvoker, unsigned short eventindex, float delay, const float* origin, const float* angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2);
void HUD_SetMaxSpeed(const struct edict_s* ed, float speed);
int stub_PrecacheModel(const char* s);
int stub_PrecacheSound(const char* s);
unsigned short stub_PrecacheEvent(int type, const char* s);
void stub_SetModel(struct edict_s* e, const char* m);
Vector HUD_GetLastOrg();
bool HUD_FirstTimePredicting();


extern Vector v_angles;
extern int g_CurrentWeaponId;