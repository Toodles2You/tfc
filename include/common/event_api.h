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

#define EVENT_API_VERSION 1

typedef struct event_api_s
{
	int version;
	void (*EV_PlaySound)(int ent, float* origin, int channel, const char* sample, float volume, float attenuation, int fFlags, int pitch);
	void (*EV_StopSound)(int ent, int channel, const char* sample);
	int (*EV_FindModelIndex)(const char* pmodel);
	int (*EV_IsLocal)(int playernum);
	int (*EV_LocalPlayerDucking)(void);
	void (*EV_LocalPlayerViewheight)(float*);
	void (*EV_LocalPlayerBounds)(int hull, float* mins, float* maxs);
	int (*EV_IndexFromTrace)(struct pmtrace_s* pTrace);
	struct physent_s* (*EV_GetPhysent)(int idx);
	void (*EV_SetUpPlayerPrediction)(int dopred, int bIncludeLocalClient);
	void (*EV_PushPMStates)(void);
	void (*EV_PopPMStates)(void);
	void (*EV_SetSolidPlayers)(int playernum);
	void (*EV_SetTraceHull)(int hull);
	void (*EV_PlayerTrace)(float* start, float* end, int traceFlags, int ignore_pe, struct pmtrace_s* tr);
	void (*EV_WeaponAnimation)(int sequence, int body);
	unsigned short (*EV_PrecacheEvent)(int type, const char* psz);
	void (*EV_PlaybackEvent)(int flags, const Entity* pInvoker, unsigned short eventindex, float delay, float* origin, float* angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2);
	const char* (*EV_TraceTexture)(int ground, float* vstart, float* vend);
	void (*EV_StopAllSounds)(int entnum, int entchannel);
	void (*EV_KillEvents)(int entnum, const char* eventname);
} event_api_t;

typedef struct physent_s physent_t;

namespace client::event {
inline void (*PlaySound)(int ent, float* origin, int channel, const char* sample, float volume, float attenuation, int fFlags, int pitch);
inline void (*StopSound)(int ent, int channel, const char* sample);
inline int (*FindModelIndex)(const char* pmodel);
inline int (*IsLocal)(int playernum);
inline int (*LocalPlayerDucking)(void);
inline void (*LocalPlayerViewheight)(float*);
inline void (*LocalPlayerBounds)(int hull, float* mins, float* maxs);
inline int (*IndexFromTrace)(struct pmtrace_s* pTrace);
inline physent_t* (*GetPhysent)(int idx);
inline void (*SetUpPlayerPrediction)(int dopred, int bIncludeLocalClient);
inline void (*PushPMStates)(void);
inline void (*PopPMStates)(void);
inline void (*SetSolidPlayers)(int playernum);
inline void (*SetTraceHull)(int hull);
inline void (*PlayerTrace)(float* start, float* end, int traceFlags, int ignore_pe, struct pmtrace_s* tr);
inline void (*WeaponAnimation)(int sequence, int body);
inline unsigned short (*PrecacheEvent)(int type, const char* psz);
inline void (*PlaybackEvent)(int flags, const Entity* pInvoker, unsigned short eventindex, float delay, float* origin, float* angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2);
inline const char* (*TraceTexture)(int ground, float* vstart, float* vend);
inline void (*StopAllSounds)(int entnum, int entchannel);
inline void (*KillEvents)(int entnum, const char* eventname);
} /* namespace client::event */
