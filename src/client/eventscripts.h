//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// eventscripts.h

#pragma once

void EV_EjectBrass(const Vector& origin, const Vector& velocity, float rotation, int model, int soundtype);
void EV_GetGunPosition(struct event_args_s* args, Vector& pos, const Vector& origin);
void EV_GetDefaultShellInfo(struct event_args_s* args, const Vector& origin, const Vector& velocity, Vector& ShellVelocity, Vector& ShellOrigin, const Vector& forward, const Vector& right, const Vector& up, float forwardScale, float upScale, float rightScale);
bool EV_IsLocal(int idx);
bool EV_IsPlayer(int idx);
void EV_CreateTracer(const Vector& start, const Vector& end);

struct cl_entity_s* GetEntity(int idx);
struct cl_entity_s* GetViewEntity();
void EV_MuzzleFlash();

void EV_TracePush(const int ignoreEntity, const int hullNumber = kHullPoint, const bool doPrediction = false);

void EV_TracePop();

void EV_TraceLine(float* const start, float* const end, const int traceFlags, const int ignoreEntity, pmtrace_t& outTrace);

