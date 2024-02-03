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
#include "com_model.h"
#include "demo.h"
#include "hud.h"
#include "ammohistory.h"
#include "view.h"
#include "entity_types.h"

extern int g_iObserverMode;
extern int g_iObserverTarget;
extern int g_iObserverTarget2;
extern struct local_state_s g_finalstate;

static bool firstTimePredicting = false;

// Pool of client side entities
static edict_t theEdicts[256];
static int numEntities = 0;

static CBasePlayer players[MAX_PLAYERS + 1];

// The entity we'll use to represent the local client
static CBasePlayer* player = players;

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
HUD_AlertMessage

Print debug messages to console
======================
*/
static void HUD_AlertMessage(ALERT_TYPE atype, const char* szFmt, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, szFmt);
	vsprintf(string, szFmt, argptr);
	va_end(argptr);

	gEngfuncs.Con_Printf("cl:  ");
	gEngfuncs.Con_Printf(string);
}


/*
=====================
HUD_PrepEntity

Links the raw entity to an entvars_s holder.  If a player is passed in as the owner, then
we set up the m_pPlayer field.
=====================
*/
static void HUD_PrepEntity(CBaseEntity* entity, CBasePlayer* owner)
{
	auto edict = theEdicts + numEntities;
	numEntities++;

	memset(edict, 0, sizeof(edict_t));

	edict->free = false;
	edict->pvPrivateData = entity;
	entity->pev = &edict->v;
	entity->pev->pContainingEntity = edict;

	entity->Precache();
	entity->Spawn();

	if (owner)
	{
		const auto weapon = dynamic_cast<CBasePlayerWeapon*>(entity);

		WeaponInfo info;
		memset(&info, 0, sizeof(info));

		weapon->m_pPlayer = owner;

		weapon->GetWeaponInfo(info);

		CBasePlayerWeapon::WeaponInfoArray[weapon->GetID()] = info;

		owner->m_rgpPlayerWeapons[weapon->GetID()] = (CBasePlayerWeapon*)entity;
	}
	else
	{
		entity->pev->flags |= FL_CLIENT;
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


int CBaseEntity::entindex()
{
	return gEngfuncs.GetLocalPlayer()->index;
}


/*
=====================
HUD_PlaybackEvent

Directly queue up an event on the client
=====================
*/
static void HUD_PlaybackEvent(int flags, const edict_t* pInvoker, unsigned short eventindex, float delay,
	const float* origin, const float* angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2)
{
	Vector org;
	Vector ang;

	if (!HUD_FirstTimePredicting())
		return;

	// Weapon prediction events are assumed to occur at the player's origin
	org = g_finalstate.playerstate.origin;
	ang = player->pev->v_angle + player->pev->punchangle * 2;
	gEngfuncs.pfnPlaybackEvent(flags, pInvoker, eventindex, delay, org, ang, fparam1, fparam2, iparam1, iparam2, bparam1, bparam2);
}


static edict_t* HUD_GetEntityByIndex(int index)
{
	return theEdicts + index;
}


static void* HUD_GetModelPtr(edict_t* edict)
{
	model_t* model = gEngfuncs.hudGetModelByIndex(edict->v.modelindex);

	if (model == nullptr || model->type != mod_studio)
	{
		return nullptr;
	}

	return model->cache.data;
}


/*
=====================
HUD_InitClientWeapons

Set up weapons, player and functions needed to run weapons code client-side.
=====================
*/
static void HUD_InitClientWeapons()
{
	static bool initialized = false;
	if (initialized)
		return;

	initialized = true;

	// Local version of game .dll global variables ( time, etc. )
	static globalvars_t globals;
	gpGlobals = &globals;

	// Fill in current time
	gpGlobals->time = gEngfuncs.GetClientTime();

	// Handled locally
	g_engfuncs.pfnAlertMessage = HUD_AlertMessage;
	g_engfuncs.pfnPlaybackEvent = HUD_PlaybackEvent;
	g_engfuncs.pfnPEntityOfEntIndex = HUD_GetEntityByIndex;
	g_engfuncs.pfnGetModelPtr = HUD_GetModelPtr;
	g_engfuncs.pfnGetBonePosition = nullptr; /*! Toodles FIXME: */
	g_engfuncs.pfnGetAttachment = nullptr; /*! Toodles FIXME: */
	g_engfuncs.pfnPEntityOfEntIndexAllEntities = HUD_GetEntityByIndex;

	// Pass through to engine
	g_engfuncs.pfnPrecacheEvent = gEngfuncs.pfnPrecacheEvent;
	g_engfuncs.pfnRandomFloat = gEngfuncs.pfnRandomFloat;
	g_engfuncs.pfnRandomLong = gEngfuncs.pfnRandomLong;
	g_engfuncs.pfnCVarGetPointer = gEngfuncs.pfnGetCvarPointer;
	g_engfuncs.pfnCVarGetString = gEngfuncs.pfnGetCvarString;
	g_engfuncs.pfnCVarGetFloat = gEngfuncs.pfnGetCvarFloat;

	// Allocate slots for the players
	for (auto i = 0; i <= MAX_PLAYERS; i++)
	{
		HUD_PrepEntity(players + i, nullptr);
	}

	// Allocate slots for each weapon that we are going to be predicting
	for (auto i = 0; i < ARRAYSIZE(weapons); i++)
	{
		HUD_PrepEntity(weapons[i], player);
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
=========================
HUD_ProcessPlayerState

We have received entity_state_t for this player over the network.  We need to copy appropriate fields to the
playerstate structure
=========================
*/
void HUD_ProcessPlayerState(struct entity_state_s* dst, const struct entity_state_s* src)
{
	HUD_InitClientWeapons();

	// Copy in network data
	dst->origin = src->origin;
	dst->angles = src->angles;
	dst->velocity = src->velocity;

	dst->frame = src->frame;
	dst->modelindex = src->modelindex;
	dst->skin = src->skin;
	dst->effects = src->effects;
	dst->weaponmodel = src->weaponmodel;
	dst->movetype = src->movetype;
	dst->sequence = src->sequence;
	dst->animtime = src->animtime;

	dst->solid = src->solid;

	dst->rendermode = src->rendermode;
	dst->renderamt = src->renderamt;
	dst->rendercolor.r = src->rendercolor.r;
	dst->rendercolor.g = src->rendercolor.g;
	dst->rendercolor.b = src->rendercolor.b;
	dst->renderfx = src->renderfx;

	dst->framerate = src->framerate;
	dst->body = src->body;

	memcpy(&dst->controller[0], &src->controller[0], 4 * sizeof(byte));
	memcpy(&dst->blending[0], &src->blending[0], 2 * sizeof(byte));

	VectorCopy(src->basevelocity, dst->basevelocity);

	dst->friction = src->friction;
	dst->gravity = src->gravity;
	dst->gaitsequence = src->gaitsequence;
	dst->spectator = src->spectator;
	dst->usehull = src->usehull;
	dst->playerclass = src->playerclass;
	dst->team = src->team;
	dst->colormap = src->colormap;
	dst->health = src->health;

	g_PlayerExtraInfo[dst->number].health = dst->health;
	g_PlayerExtraInfo[dst->number].dead = dst->health <= 0;

	// Save off some data so other areas of the Client DLL can get to it
	cl_entity_t* localPlayer = gEngfuncs.GetLocalPlayer(); // Get the local player's index
	if (dst->number == localPlayer->index)
	{
		g_iPlayerClass = dst->playerclass;
		g_iTeamNumber = dst->team;

		if (g_iObserverMode != src->iuser1
		 || g_iObserverTarget != src->iuser2
		 || g_iObserverTarget2 != src->iuser3)
		{
			V_ResetChaseCam();
		}

		g_iObserverMode = src->iuser1;
		g_iObserverTarget = src->iuser2;
		g_iObserverTarget2 = src->iuser3;
	}

	if (dst->number >= 1 && dst->number <= MAX_PLAYERS)
	{
		players[dst->number].SetEntityState(*dst);
	}
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
		if (to->client.health <= 0 && player->pev->health > 0)
		{
			player->Killed(nullptr, nullptr, DMG_GENERIC);
		}
		else if (to->client.health > 0 && player->pev->health <= 0)
		{
			player->Spawn();
		}
		player->pev->health = to->client.health;
	}

	player->SetPrefsFromUserinfo(nullptr);

	gEngfuncs.GetViewAngles(player->pev->v_angle);
	player->pev->button = cmd->buttons;
	player->m_afButtonLast = from->playerstate.oldbuttons;
	player->m_WeaponBits = gHUD.m_iWeaponBits;

	player->SetEntityState(from->playerstate);
	player->SetClientData(from->client);

	for (i = 0; i < ARRAYSIZE(weapons); i++)
	{
		auto weapon = weapons[i];
		weapon->SetWeaponData(from->weapondata[weapon->GetID()]);
	}

	player->CmdStart(*cmd, random_seed);
	player->PreThink();

	player->PostThink();

	for (i = 0; i < ARRAYSIZE(weapons); i++)
	{
		auto weapon = weapons[i];
		weapon->DecrementTimers(cmd->msec);
		weapon->GetWeaponData(to->weapondata[weapon->GetID()]);
	}

	player->UpdateHudData();
	player->DecrementTimers(cmd->msec);
	player->GetClientData(to->client, true);
	player->GetEntityState(to->playerstate);

	// Store off the last position from the predicted state.
	HUD_SetLastOrg();

	g_finalstate = *to;

	g_CurrentWeaponId = to->client.m_iId;
	g_PunchAngle = to->client.punchangle * 2;
}


void HUD_PlayerMoveInit(struct playermove_s* ppmove)
{
	PM_Init(ppmove);
	player->InstallGameMovement(new CHalfLifeMovement{ppmove, player});
}


void HUD_PlayerMove(struct playermove_s* ppmove, int server)
{
	ppmove->server = 0;
	player->GetGameMovement()->Move();
}


bool HUD_FirstTimePredicting()
{
	return firstTimePredicting;
}


/*
========================
HUD_AddEntity
	Return 0 to filter entity from visible list for rendering
========================
*/
int HUD_AddEntity(int type, struct cl_entity_s* ent, const char* modelname)
{
	switch (type)
	{
		case ET_PLAYER:
		case ET_BEAM:
		case ET_TEMPENTITY:
			break;
		case ET_NORMAL:
		case ET_FRAGMENTED:
		default:
			break;
	}

	return 1;
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

		weapons[i]->GetWeaponInfo(info);

		WEAPON w;
		memset(&w, 0, sizeof(w));

		strcpy(w.szName, info.pszName);
		w.iAmmoType = info.iAmmo1;
		w.iAmmo2Type = info.iAmmo2;
		w.iSlot = info.iSlot;
		w.iSlotPos = info.iPosition;
		w.iFlags = info.iFlags;
		w.iId = weapons[i]->GetID();

		rgWeapons[weapons[i]->GetID()] = w;
	}
}

