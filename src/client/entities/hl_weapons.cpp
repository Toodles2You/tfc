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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"

#include "usercmd.h"
#include "entity_state.h"
#include "demo_api.h"
#include "pm_defs.h"
#include "event_api.h"
#include "r_efx.h"
#include "pm_shared.h"
#include "gamemovement.h"

#include "cl_dll.h"
#include "com_weapons.h"
#include "demo.h"
#include "hud.h"
#include "ammohistory.h"

extern struct local_state_s g_finalstate;

static bool firstTimePredicting = false;

// Pool of client side entities/entvars_t
static entvars_t ev[WEAPON_LAST + 1];
static int num_ents = 0;

// The entity we'll use to represent the local client
static CBasePlayer player;

// Local version of game .dll global variables ( time, etc. )
static globalvars_t Globals;

static CCrowbar crowbar;
static CMP5 mp5;
static CBasePlayerWeapon* weapons[] =
{
	&crowbar,
	&mp5,
};

static Vector previousorigin;

Vector g_PunchAngle;


/*
======================
AlertMessage

Print debug messages to console
======================
*/
void AlertMessage(ALERT_TYPE atype, const char* szFmt, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, szFmt);
	vsprintf(string, szFmt, argptr);
	va_end(argptr);

	gEngfuncs.Con_Printf("cl:  ");
	gEngfuncs.Con_Printf(string);
}

//Just loads a v_ model.
void LoadVModel(const char* szViewModel, CBasePlayer* m_pPlayer)
{
	gEngfuncs.CL_LoadModel(szViewModel, &m_pPlayer->pev->viewmodel);
}

/*
=====================
HUD_PrepEntity

Links the raw entity to an entvars_s holder.  If a player is passed in as the owner, then
we set up the m_pPlayer field.
=====================
*/
void HUD_PrepEntity(CBaseEntity* pEntity, CBasePlayer* pWeaponOwner)
{
	memset(&ev[num_ents], 0, sizeof(entvars_t));
	pEntity->pev = &ev[num_ents++];

	pEntity->Precache();
	pEntity->Spawn();

	if (pWeaponOwner)
	{
		const auto weapon = dynamic_cast<CBasePlayerWeapon*>(pEntity);

		WeaponInfo info;

		memset(&info, 0, sizeof(info));

		weapon->m_pPlayer = pWeaponOwner;

		weapon->GetWeaponInfo(&info);

		CBasePlayerWeapon::WeaponInfoArray[weapon->GetID()] = info;

		pWeaponOwner->m_rgpPlayerWeapons[weapon->GetID()] = (CBasePlayerWeapon*)pEntity;
	}
}

/*
=====================
CBaseEntity:: Killed

If weapons code "kills" an entity, just set its effects to EF_NODRAW
=====================
*/
void CBaseEntity::Killed(CBaseEntity* inflictor, CBaseEntity* attacker, int bitsDamageType)
{
	pev->effects |= EF_NODRAW;
}

/*
=====================
util::TraceLine

Don't actually trace, but act like the trace didn't hit anything.
=====================
*/
void util::TraceLine(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, CBaseEntity* ignore, TraceResult* ptr)
{
	memset(ptr, 0, sizeof(*ptr));
	ptr->flFraction = 1.0;
}

bool util::TraceLine(const Vector& start, const Vector& end, TraceResult* tr, CBaseEntity* ignore, int flags, int hull)
{
	memset(tr, 0, sizeof(*tr));
	tr->flFraction = 1.0;
	return false;
}

void HUD_TraceLine(const float* v1, const float* v2, int fNoMonsters, edict_t* pentToSkip, TraceResult* ptr)
{
	pmtrace_t tr;
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(true, true);
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);
	gEngfuncs.pEventAPI->EV_SetTraceHull(kHullPoint);
	gEngfuncs.pEventAPI->EV_PlayerTrace((float*)v1, (float*)v2, fNoMonsters, -1, &tr);
	gEngfuncs.pEventAPI->EV_PopPMStates();
	ptr->fAllSolid = tr.allsolid;
	ptr->fStartSolid = tr.startsolid;
	ptr->fInOpen = tr.inopen;
	ptr->fInWater = tr.inwater;
	ptr->flFraction = tr.fraction;
	ptr->vecEndPos = tr.endpos;
	ptr->flPlaneDist = tr.plane.dist;
	ptr->vecPlaneNormal = tr.plane.normal;
	ptr->pHit = nullptr;
	ptr->iHitgroup = tr.hitgroup;
}

/*
=====================
HUD_PlaybackEvent

Directly queue up an event on the client
=====================
*/
void HUD_PlaybackEvent(int flags, const edict_t* pInvoker, unsigned short eventindex, float delay,
	const float* origin, const float* angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2)
{
	Vector org;
	Vector ang;

	if (!HUD_FirstTimePredicting())
		return;

	// Weapon prediction events are assumed to occur at the player's origin
	org = g_finalstate.playerstate.origin;
	ang = player.pev->v_angle + player.pev->punchangle * 2;
	gEngfuncs.pfnPlaybackEvent(flags, pInvoker, eventindex, delay, org, ang, fparam1, fparam2, iparam1, iparam2, bparam1, bparam2);
}

/*
=====================
HUD_InitClientWeapons

Set up weapons, player and functions needed to run weapons code client-side.
=====================
*/
void HUD_InitClientWeapons()
{
	static bool initialized = false;
	if (initialized)
		return;

	initialized = true;

	// Set up pointer ( dummy object )
	gpGlobals = &Globals;

	// Fill in current time ( probably not needed )
	gpGlobals->time = gEngfuncs.GetClientTime();

	// Fake functions
	g_engfuncs.pfnPrecacheModel = stub_PrecacheModel;
	g_engfuncs.pfnPrecacheSound = stub_PrecacheSound;
	g_engfuncs.pfnPrecacheEvent = stub_PrecacheEvent;
	g_engfuncs.pfnSetModel = stub_SetModel;
	g_engfuncs.pfnSetClientMaxspeed = HUD_SetMaxSpeed;

	// Handled locally
	g_engfuncs.pfnTraceLine = HUD_TraceLine;
	g_engfuncs.pfnPlaybackEvent = HUD_PlaybackEvent;
	g_engfuncs.pfnAlertMessage = AlertMessage;

	// Pass through to engine
	g_engfuncs.pfnPrecacheEvent = gEngfuncs.pfnPrecacheEvent;
	g_engfuncs.pfnRandomFloat = gEngfuncs.pfnRandomFloat;
	g_engfuncs.pfnRandomLong = gEngfuncs.pfnRandomLong;
	g_engfuncs.pfnCVarGetPointer = gEngfuncs.pfnGetCvarPointer;
	g_engfuncs.pfnCVarGetString = gEngfuncs.pfnGetCvarString;
	g_engfuncs.pfnCVarGetFloat = gEngfuncs.pfnGetCvarFloat;

	// Allocate a slot for the local player
	HUD_PrepEntity(&player, nullptr);

	// Allocate slots for each weapon that we are going to be predicting
	for (auto i = 0; i < ARRAYSIZE(weapons); i++)
	{
		HUD_PrepEntity(weapons[i], &player);
	}
}

/*
=====================
HUD_GetLastOrg

Retruns the last position that we stored for egon beam endpoint.
=====================
*/
Vector HUD_GetLastOrg()
{
	return previousorigin;
}

/*
=====================
HUD_SetLastOrg

Remember our exact predicted origin so we can draw the egon to the right position.
=====================
*/
void HUD_SetLastOrg()
{
	previousorigin = g_finalstate.playerstate.origin + g_finalstate.client.view_ofs;
}

/*
=====================
HUD_PostRunCmd

Client calls this during prediction, after it has moved the player and updated any info changed into to->
time is the current client clock based on prediction
cmd is the command that caused the movement, etc
runfuncs is 1 if this is the first time we've predicted this command.  If so, sounds and effects should play, otherwise, they should
be ignored
=====================
*/
void HUD_PostRunCmd(struct local_state_s* from, struct local_state_s* to, struct usercmd_s* cmd, int runfuncs, double time, unsigned int random_seed)
{
	int i;

	firstTimePredicting = runfuncs != 0;

	HUD_InitClientWeapons();

	gpGlobals->time = gEngfuncs.GetClientTime();
	gpGlobals->frametime = cmd->msec / 1000.0f;

	// Store pointer to our destination entity_state_t so we can get our origin, etc. from it
	//  for setting up events on the client
	g_finalstate = *to;

	// If we are running events/etc. go ahead and see if we
	//  managed to die between last frame and this one
	// If so, run the appropriate player killed or spawn function
	if (HUD_FirstTimePredicting())
	{
		if (to->client.health <= 0 && player.pev->health > 0)
		{
			player.Killed(nullptr, nullptr, DMG_GENERIC);
		}
		else if (to->client.health > 0 && player.pev->health <= 0)
		{
			player.Spawn();
		}
		player.pev->health = to->client.health;
	}

	gEngfuncs.GetViewAngles(player.pev->v_angle);
	player.pev->button = cmd->buttons;
	player.m_afButtonLast = from->playerstate.oldbuttons;
	player.m_WeaponBits = gHUD.m_iWeaponBits;

	player.SetClientData(from->client);

	for (i = 0; i < ARRAYSIZE(weapons); i++)
	{
		auto weapon = weapons[i];
		weapon->SetWeaponData(from->weapondata[weapon->GetID()]);
	}

	player.CmdStart(*cmd, random_seed);
	player.PreThink();

	player.PostThink();

	for (i = 0; i < ARRAYSIZE(weapons); i++)
	{
		auto weapon = weapons[i];
		weapon->DecrementTimers(cmd->msec);
		weapon->GetWeaponData(to->weapondata[weapon->GetID()]);
	}

	player.UpdateHudData();
	player.DecrementTimers(cmd->msec);
	player.GetClientData(to->client, true);

	// Store off the last position from the predicted state.
	HUD_SetLastOrg();

	g_finalstate = *to;

	g_CurrentWeaponId = to->client.m_iId;
	g_PunchAngle = to->client.punchangle * 2;
}

void HUD_PlayerMoveInit(struct playermove_s* ppmove)
{
	PM_Init(ppmove);
	player.InstallGameMovement(new CHalfLifeMovement{ppmove, &player});
}

void HUD_PlayerMove(struct playermove_s* ppmove, int server)
{
	ppmove->server = 0;
	player.GetGameMovement()->Move();
}

bool HUD_FirstTimePredicting()
{
	return firstTimePredicting;
}

/*! Toodles FIXME: Lots-a spaghetti */
void WeaponsResource::Init()
{
	memset(rgWeapons, 0, sizeof(rgWeapons));
	Reset();

	for (auto i = 0; i < ARRAYSIZE(weapons); i++)
	{
		WeaponInfo info;
		memset(&info, 0, sizeof(info));

		weapons[i]->GetWeaponInfo(&info);

		WEAPON w;
		memset(&w, 0, sizeof(w));

		strcpy(w.szName, info.pszName);
		w.iAmmoType = info.iAmmo1;
		w.iAmmo2Type = info.iAmmo2;
		w.iMax1 = info.iMaxAmmo1;
		w.iMax2 = info.iMaxAmmo2;
		w.iSlot = info.iSlot;
		w.iSlotPos = info.iPosition;
		w.iFlags = info.iFlags;
		w.iId = weapons[i]->GetID();

		rgWeapons[weapons[i]->GetID()] = w;
	}
}
