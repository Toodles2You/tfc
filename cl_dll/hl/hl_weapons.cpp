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

#include "cl_dll.h"
#include "../com_weapons.h"
#include "../demo.h"
#include "../hud.h"

extern int g_iUser1;

// Pool of client side entities/entvars_t
static entvars_t ev[MAX_WEAPONS + 1];
static int num_ents = 0;

// The entity we'll use to represent the local client
static CBasePlayer player;

// Local version of game .dll global variables ( time, etc. )
static globalvars_t Globals;

static CBasePlayerWeapon* g_pWpns[MAX_WEAPONS];

float g_flApplyVel = 0.0;
bool g_irunninggausspred = false;

Vector previousorigin;

// HLDM Weapon placeholder entities.
CGlock g_Glock;
CCrowbar g_Crowbar;
CPython g_Python;
CMP5 g_Mp5;
CCrossbow g_Crossbow;
CShotgun g_Shotgun;
CRpg g_Rpg;
CGauss g_Gauss;
CEgon g_Egon;
CHgun g_HGun;
CHandGrenade g_HandGren;
CSatchel g_Satchel;
CTripmine g_Tripmine;
CSqueak g_Snark;


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
		ItemInfo info;

		memset(&info, 0, sizeof(info));

		((CBasePlayerWeapon*)pEntity)->m_pPlayer = pWeaponOwner;

		((CBasePlayerWeapon*)pEntity)->GetItemInfo(&info);

		CBasePlayerItem::ItemInfoArray[info.iId] = info;

		g_pWpns[info.iId] = (CBasePlayerWeapon*)pEntity;
	}
}

/*
=====================
CBaseEntity:: Killed

If weapons code "kills" an entity, just set its effects to EF_NODRAW
=====================
*/
void CBaseEntity::Killed(entvars_t* pevAttacker, int iGib)
{
	pev->effects |= EF_NODRAW;
}

/*
=====================
CBasePlayerWeapon:: DefaultDeploy

=====================
*/
bool CBasePlayerWeapon::DefaultDeploy(const char* szViewModel, const char* szWeaponModel, int iAnim, const char* szAnimExt, int body)
{
	if (!CanDeploy())
		return false;

	gEngfuncs.CL_LoadModel(szViewModel, &m_pPlayer->pev->viewmodel);

	SendWeaponAnim(iAnim, body);

	g_irunninggausspred = false;
	m_pPlayer->m_iNextAttack = 500;
	m_iNextPrimaryAttack = V_max(m_iNextPrimaryAttack, 500);
	m_iNextSecondaryAttack = V_max(m_iNextSecondaryAttack, 500);
	m_iTimeWeaponIdle = 1000;
	m_bPlayEmptySound = true;
	return true;
}

bool CBasePlayerWeapon::DefaultHolster(int iAnim, int body)
{
	if (!CanHolster())
		return false;

	if (iAnim >= 0)
		SendWeaponAnim(iAnim, body);

	m_pPlayer->m_iNextAttack = 500;
	m_iNextPrimaryAttack = V_max(m_iNextPrimaryAttack, 500);
	m_iNextSecondaryAttack = V_max(m_iNextSecondaryAttack, 500);
	m_iTimeWeaponIdle = 1000;
	m_fInReload = false; // Cancel any reload in progress.
	m_fInSpecialReload = 0;
	m_pPlayer->m_iFOV = 0;
	g_irunninggausspred = false;

	return true;
}

/*
=====================
CBasePlayerWeapon:: PlayEmptySound

=====================
*/
void CBasePlayerWeapon::PlayEmptySound()
{
	if (!g_runfuncs)
		return;

	if (m_bPlayEmptySound)
	{
		PlayWeaponSound(CHAN_ITEM, "weapons/357_cock1.wav", 0.8);
		m_bPlayEmptySound = false;
	}
}

/*
=====================
CBasePlayerWeapon::Holster

Put away weapon
=====================
*/
bool CBasePlayerWeapon::Holster()
{
	m_fInReload = false; // cancel any reload in progress.
	m_fInSpecialReload = 0;
	g_irunninggausspred = false;
	m_pPlayer->pev->viewmodel = 0;
	return true;
}

/*
=====================
CBasePlayerWeapon::SendWeaponAnim

Animate weapon model
=====================
*/
void CBasePlayerWeapon::SendWeaponAnim(int iAnim, int body)
{
	m_pPlayer->pev->weaponanim = iAnim;

	HUD_SendWeaponAnim(iAnim, body, false);
}

void CBasePlayerWeapon::PlayWeaponSound(int iChannel, const char* szSound, float flVolume, float flAttn, int iFlags, float flPitch)
{
	if (!g_runfuncs)
		return;

	auto player = gEngfuncs.GetLocalPlayer();

	if (!player)
		return;

	gEngfuncs.pEventAPI->EV_PlaySound(player->index, player->origin, iChannel, szSound, flVolume, flAttn, iFlags, flPitch);
}

/*
=====================
CBaseEntity::FireBulletsPlayer

Only produces random numbers to match the server ones.
=====================
*/
void CBaseEntity::FireBulletsPlayer(unsigned int cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq, int iDamage, entvars_t* pevAttacker, int shared_rand)
{
}

/*
=====================
CBasePlayer::Killed

=====================
*/
void CBasePlayer::Killed(entvars_t* pevAttacker, int iGib)
{
	// Holster weapon immediately, to allow it to cleanup
	if (m_pActiveItem)
		m_pActiveItem->Holster();

	g_irunninggausspred = false;
}

/*
=====================
CBasePlayer::Spawn

=====================
*/
void CBasePlayer::Spawn()
{
	if (m_pActiveItem)
		m_pActiveItem->Deploy();

	g_irunninggausspred = false;
}

/*
=====================
UTIL_TraceLine

Don't actually trace, but act like the trace didn't hit anything.
=====================
*/
void UTIL_TraceLine(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, edict_t* pentIgnore, TraceResult* ptr)
{
	memset(ptr, 0, sizeof(*ptr));
	ptr->flFraction = 1.0;
}

/*
=====================
UTIL_ParticleBox

For debugging, draw a box around a player made out of particles
=====================
*/
void UTIL_ParticleBox(CBasePlayer* player, float* mins, float* maxs, float life, unsigned char r, unsigned char g, unsigned char b)
{
	int i;
	Vector mmin, mmax;

	for (i = 0; i < 3; i++)
	{
		mmin[i] = player->pev->origin[i] + mins[i];
		mmax[i] = player->pev->origin[i] + maxs[i];
	}

	gEngfuncs.pEfxAPI->R_ParticleBox((float*)&mmin, (float*)&mmax, 5.0, 0, 255, 0);
}

/*
=====================
UTIL_ParticleBoxes

For debugging, draw boxes for other collidable players
=====================
*/
void UTIL_ParticleBoxes()
{
	int idx;
	physent_t* pe;
	cl_entity_t* player;
	Vector mins, maxs;

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(0, 1);

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	player = gEngfuncs.GetLocalPlayer();
	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(player->index - 1);

	for (idx = 1; idx < 100; idx++)
	{
		pe = gEngfuncs.pEventAPI->EV_GetPhysent(idx);
		if (!pe)
			break;

		if (pe->info >= 1 && pe->info <= gEngfuncs.GetMaxClients())
		{
			mins = pe->origin + pe->mins;
			maxs = pe->origin + pe->maxs;

			gEngfuncs.pEfxAPI->R_ParticleBox((float*)&mins, (float*)&maxs, 0, 0, 255, 2.0);
		}
	}

	gEngfuncs.pEventAPI->EV_PopPMStates();
}

/*
=====================
UTIL_ParticleLine

For debugging, draw a line made out of particles
=====================
*/
void UTIL_ParticleLine(CBasePlayer* player, float* start, float* end, float life, unsigned char r, unsigned char g, unsigned char b)
{
	gEngfuncs.pEfxAPI->R_ParticleLine(start, end, r, g, b, life);
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
	g_engfuncs.pfnNameForFunction = stub_NameForFunction;
	g_engfuncs.pfnSetModel = stub_SetModel;
	g_engfuncs.pfnSetClientMaxspeed = HUD_SetMaxSpeed;

	// Handled locally
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
	HUD_PrepEntity(&player, NULL);

	// Allocate slot(s) for each weapon that we are going to be predicting
	HUD_PrepEntity(&g_Glock, &player);
	HUD_PrepEntity(&g_Crowbar, &player);
	HUD_PrepEntity(&g_Python, &player);
	HUD_PrepEntity(&g_Mp5, &player);
	HUD_PrepEntity(&g_Crossbow, &player);
	HUD_PrepEntity(&g_Shotgun, &player);
	HUD_PrepEntity(&g_Rpg, &player);
	HUD_PrepEntity(&g_Gauss, &player);
	HUD_PrepEntity(&g_Egon, &player);
	HUD_PrepEntity(&g_HGun, &player);
	HUD_PrepEntity(&g_HandGren, &player);
	HUD_PrepEntity(&g_Satchel, &player);
	HUD_PrepEntity(&g_Tripmine, &player);
	HUD_PrepEntity(&g_Snark, &player);
}

/*
=====================
HUD_GetLastOrg

Retruns the last position that we stored for egon beam endpoint.
=====================
*/
void HUD_GetLastOrg(float* org)
{
	int i;

	// Return last origin
	for (i = 0; i < 3; i++)
	{
		org[i] = previousorigin[i];
	}
}

/*
=====================
HUD_SetLastOrg

Remember our exact predicted origin so we can draw the egon to the right position.
=====================
*/
void HUD_SetLastOrg()
{
	int i;

	// Offset final origin by view_offset
	for (i = 0; i < 3; i++)
	{
		previousorigin[i] = g_finalstate->playerstate.origin[i] + g_finalstate->client.view_ofs[i];
	}
}

/*
=====================
HUD_WeaponsPostThink

Run Weapon firing code on client
=====================
*/
void HUD_WeaponsPostThink(local_state_s* from, local_state_s* to, usercmd_t* cmd, double time, unsigned int random_seed)
{
	int i;
	int buttonsChanged;
	CBasePlayerWeapon* pWeapon = NULL;
	CBasePlayerWeapon* pCurrent;
	weapon_data_t nulldata, *pfrom, *pto;
	static int lasthealth;

	memset(&nulldata, 0, sizeof(nulldata));

	// Get current clock
	//Use actual time instead of prediction frame time because that time value breaks anything that uses absolute time values.
	gpGlobals->time = gEngfuncs.GetClientTime(); //time;

	//Lets weapons code use frametime to decrement timers and stuff.
	gpGlobals->frametime = cmd->msec / 1000.0f;

	// Fill in data based on selected weapon
	// FIXME, make this a method in each weapon?  where you pass in an entity_state_t *?
	switch (from->client.m_iId)
	{
	case WEAPON_CROWBAR:
		pWeapon = &g_Crowbar;
		break;

	case WEAPON_GLOCK:
		pWeapon = &g_Glock;
		break;

	case WEAPON_PYTHON:
		pWeapon = &g_Python;
		break;

	case WEAPON_MP5:
		pWeapon = &g_Mp5;
		break;

	case WEAPON_CROSSBOW:
		pWeapon = &g_Crossbow;
		break;

	case WEAPON_SHOTGUN:
		pWeapon = &g_Shotgun;
		break;

	case WEAPON_RPG:
		pWeapon = &g_Rpg;
		break;

	case WEAPON_GAUSS:
		pWeapon = &g_Gauss;
		break;

	case WEAPON_EGON:
		pWeapon = &g_Egon;
		break;

	case WEAPON_HORNETGUN:
		pWeapon = &g_HGun;
		break;

	case WEAPON_HANDGRENADE:
		pWeapon = &g_HandGren;
		break;

	case WEAPON_SATCHEL:
		pWeapon = &g_Satchel;
		break;

	case WEAPON_TRIPMINE:
		pWeapon = &g_Tripmine;
		break;

	case WEAPON_SNARK:
		pWeapon = &g_Snark;
		break;
	}

	// Store pointer to our destination entity_state_t so we can get our origin, etc. from it
	//  for setting up events on the client
	g_finalstate = to;

	// If we are running events/etc. go ahead and see if we
	//  managed to die between last frame and this one
	// If so, run the appropriate player killed or spawn function
	if (g_runfuncs)
	{
		if (to->client.health <= 0 && lasthealth > 0)
		{
			player.Killed(NULL, 0);
			gHUD.m_Ammo.Update_CurWeapon(0, -1, -1);
		}
		else if (to->client.health > 0 && lasthealth <= 0)
		{
			player.Spawn();
		}

		lasthealth = to->client.health;

		gHUD.m_Health.Update_Health(to->client.health);
		gHUD.m_Battery.Update_Battery(to->client.vuser4.z);
	}

	// We are not predicting the current weapon, just bow out here.
	if (!pWeapon)
		return;

	for (i = 0; i < MAX_WEAPONS; i++)
	{
		pCurrent = g_pWpns[i];
		if (!pCurrent)
		{
			continue;
		}

		pfrom = &from->weapondata[i];

		pCurrent->m_fInReload = 0 != pfrom->m_fInReload;
		pCurrent->m_fInSpecialReload = pfrom->m_fInSpecialReload;
		//		pCurrent->m_flPumpTime			= pfrom->m_flPumpTime;
		pCurrent->m_iClip = pfrom->m_iClip;
		pCurrent->m_iNextPrimaryAttack = *(int*)&pfrom->m_flNextPrimaryAttack;
		pCurrent->m_iNextSecondaryAttack = *(int*)&pfrom->m_flNextSecondaryAttack;
		pCurrent->m_iTimeWeaponIdle = *(int*)&pfrom->m_flTimeWeaponIdle;
		pCurrent->pev->fuser1 = pfrom->fuser1;
		pCurrent->m_flStartThrow = pfrom->fuser2;
		pCurrent->m_flReleaseThrow = pfrom->fuser3;
		pCurrent->m_chargeReady = pfrom->iuser1;
		pCurrent->m_fInAttack = pfrom->iuser2;
		pCurrent->m_fireState = pfrom->iuser3;

		pCurrent->SetWeaponData(*pfrom);
	}

	// For random weapon events, use this seed to seed random # generator
	player.random_seed = random_seed;

	// Get old buttons from previous state.
	player.m_afButtonLast = from->playerstate.oldbuttons;

	// Which buttsons chave changed
	buttonsChanged = (player.m_afButtonLast ^ cmd->buttons); // These buttons have changed this frame

	// Debounced button codes for pressed/released
	// The changed ones still down are "pressed"
	player.m_afButtonPressed = buttonsChanged & cmd->buttons;
	// The ones not down are "released"
	player.m_afButtonReleased = buttonsChanged & (~cmd->buttons);

	// Set player variables that weapons code might check/alter
	player.pev->button = cmd->buttons;

	player.pev->velocity = from->client.velocity;
	player.pev->flags = from->client.flags;

	player.pev->deadflag = from->client.deadflag;
	player.pev->waterlevel = from->client.waterlevel;
	player.pev->maxspeed = from->client.maxspeed;
	player.m_iFOV = from->client.fov;
	player.pev->weaponanim = from->client.weaponanim;
	player.pev->viewmodel = from->client.viewmodel;
	player.m_iNextAttack = *(int*)&from->client.m_flNextAttack;
	player.m_flNextAmmoBurn = from->client.fuser2;
	player.m_flAmmoStartCharge = from->client.fuser3;

	//Stores all our ammo info, so the client side weapons can use them.
	byte* ammo_shells = (byte*)&from->client.ammo_shells;
	byte* ammo_nails = (byte*)&from->client.ammo_nails;
	byte* ammo_cells = (byte*)&from->client.ammo_cells;
	byte* ammo_rockets = (byte*)&from->client.ammo_rockets;

	player.m_rgAmmo[AMMO_9MM] = ammo_shells[0];
	player.m_rgAmmo[AMMO_357] = ammo_shells[1];
	player.m_rgAmmo[AMMO_ARGRENADES] = ammo_shells[2];
	player.m_rgAmmo[AMMO_BOLTS] = ammo_shells[3];

	player.m_rgAmmo[AMMO_BUCKSHOT] = ammo_nails[0];
	player.m_rgAmmo[AMMO_URANIUM] = ammo_nails[1];
	player.m_rgAmmo[AMMO_ROCKETS] = ammo_nails[2];
	player.m_rgAmmo[AMMO_HORNETS] = ammo_nails[3];

	player.m_rgAmmo[AMMO_HANDGRENADES] = ammo_cells[0];
	player.m_rgAmmo[AMMO_SATCHELS] = ammo_cells[1];
	player.m_rgAmmo[AMMO_TRIPMINES] = ammo_cells[2];
	player.m_rgAmmo[AMMO_SNARKS] = ammo_cells[3];


	// Point to current weapon object
	if (WEAPON_NONE != from->client.m_iId)
	{
		player.m_pActiveItem = g_pWpns[from->client.m_iId];
	}

	if (player.m_pActiveItem->m_iId == WEAPON_RPG)
	{
		((CRpg*)player.m_pActiveItem)->m_fSpotActive = static_cast<bool>(from->client.vuser2[1]);
		((CRpg*)player.m_pActiveItem)->m_cActiveRockets = (int)from->client.vuser2[2];
	}

	// Don't go firing anything if we have died or are spectating
	// Or if we don't have a weapon model deployed
	if ((player.pev->deadflag != (DEAD_DISCARDBODY + 1)) &&
		!CL_IsDead() && 0 != player.pev->viewmodel && 0 == g_iUser1)
	{
		if (player.m_iNextAttack <= 0)
		{
			pWeapon->ItemPostFrame();
		}
	}

	// Assume that we are not going to switch weapons
	to->client.m_iId = from->client.m_iId;

	// Now see if we issued a changeweapon command ( and we're not dead )
	if (0 != cmd->weaponselect && (player.pev->deadflag != (DEAD_DISCARDBODY + 1)))
	{
		// Switched to a different weapon?
		if (from->weapondata[cmd->weaponselect].m_iId == cmd->weaponselect)
		{
			CBasePlayerWeapon* pNew = g_pWpns[cmd->weaponselect];
			if (pNew && (pNew != pWeapon))
			{
				// Put away old weapon
				// Deploy new weapon
				if ((!player.m_pActiveItem || player.m_pActiveItem->Holster()) && pNew->Deploy())
				{
					player.m_pActiveItem = pNew;
					// Update weapon id so we can predict things correctly.
					to->client.m_iId = cmd->weaponselect;
				}
			}
		}
	}

	// Copy in results of prediction code
	to->client.viewmodel = player.pev->viewmodel;
	to->client.fov = player.m_iFOV;
	to->client.weaponanim = player.pev->weaponanim;
	*(int*)&to->client.m_flNextAttack = player.m_iNextAttack;
	to->client.fuser2 = player.m_flNextAmmoBurn;
	to->client.fuser3 = player.m_flAmmoStartCharge;
	to->client.maxspeed = player.pev->maxspeed;

	//HL Weapons
	ammo_shells = (byte*)&to->client.ammo_shells;
	ammo_nails = (byte*)&to->client.ammo_nails;
	ammo_cells = (byte*)&to->client.ammo_cells;
	ammo_rockets = (byte*)&to->client.ammo_rockets;

	ammo_shells[0] = player.m_rgAmmo[AMMO_9MM];
	ammo_shells[1] = player.m_rgAmmo[AMMO_357];
	ammo_shells[2] = player.m_rgAmmo[AMMO_ARGRENADES];
	ammo_shells[3] = player.m_rgAmmo[AMMO_BOLTS];

	ammo_nails[0] = player.m_rgAmmo[AMMO_BUCKSHOT];
	ammo_nails[1] = player.m_rgAmmo[AMMO_URANIUM];
	ammo_nails[2] = player.m_rgAmmo[AMMO_ROCKETS];
	ammo_nails[3] = player.m_rgAmmo[AMMO_HORNETS];

	ammo_cells[0] = player.m_rgAmmo[AMMO_HANDGRENADES];
	ammo_cells[1] = player.m_rgAmmo[AMMO_SATCHELS];
	ammo_cells[2] = player.m_rgAmmo[AMMO_TRIPMINES];
	ammo_cells[3] = player.m_rgAmmo[AMMO_SNARKS];

	if (player.m_pActiveItem->m_iId == WEAPON_RPG)
	{
		to->client.vuser2[1] = static_cast<float>(((CRpg*)player.m_pActiveItem)->m_fSpotActive);
		to->client.vuser2[2] = ((CRpg*)player.m_pActiveItem)->m_cActiveRockets;
	}

	// Make sure that weapon animation matches what the game .dll is telling us
	//  over the wire ( fixes some animation glitches )
	if (g_runfuncs && (HUD_GetWeaponAnim() != to->client.weaponanim))
	{
		//Make sure the 357 has the right body
		g_Python.pev->body = UTIL_IsDeathmatch() ? 1 : 0;

		// Force a fixed anim down to viewmodel
		HUD_SendWeaponAnim(to->client.weaponanim, pWeapon->pev->body, true);
	}

	if (g_runfuncs)
	{
		gHUD.Update_SetFOV(to->client.fov);
	}

	static bool bAmmoUpdated[MAX_AMMO_SLOTS];
	int iAmmoType, iAmmo2Type;

	memset(bAmmoUpdated, 0, sizeof(bAmmoUpdated));

	for (i = 0; i < MAX_WEAPONS; i++)
	{
		pCurrent = g_pWpns[i];

		pto = &to->weapondata[i];

		if (!pCurrent)
		{
			memset(pto, 0, sizeof(weapon_data_t));
			continue;
		}

		if (g_runfuncs)
		{
			gHUD.m_Ammo.Update_CurWeapon(to->client.m_iId == pCurrent->m_iId ? 1 : 0, pCurrent->m_iId, pCurrent->m_iClip);

			iAmmoType = pCurrent->iAmmo1();

			if (iAmmoType > AMMO_NONE && !bAmmoUpdated[iAmmoType])
			{
				gHUD.m_Ammo.Update_AmmoX(iAmmoType, player.m_rgAmmo[iAmmoType]);
				bAmmoUpdated[iAmmoType] = true;
			}

			iAmmo2Type = pCurrent->iAmmo2();

			if (iAmmo2Type > AMMO_NONE && !bAmmoUpdated[iAmmo2Type])
			{
				gHUD.m_Ammo.Update_AmmoX(iAmmo2Type, player.m_rgAmmo[iAmmo2Type]);
				bAmmoUpdated[iAmmo2Type] = true;
			}
		}

		pto->m_fInReload = static_cast<int>(pCurrent->m_fInReload);
		pto->m_fInSpecialReload = pCurrent->m_fInSpecialReload;
		//		pto->m_flPumpTime				= pCurrent->m_flPumpTime;
		pto->m_iClip = pCurrent->m_iClip;
		*(int*)&pto->m_flNextPrimaryAttack = pCurrent->m_iNextPrimaryAttack;
		*(int*)&pto->m_flNextSecondaryAttack = pCurrent->m_iNextSecondaryAttack;
		*(int*)&pto->m_flTimeWeaponIdle = pCurrent->m_iTimeWeaponIdle;
		pto->fuser1 = pCurrent->pev->fuser1;
		pto->fuser2 = pCurrent->m_flStartThrow;
		pto->fuser3 = pCurrent->m_flReleaseThrow;
		pto->iuser1 = pCurrent->m_chargeReady;
		pto->iuser2 = pCurrent->m_fInAttack;
		pto->iuser3 = pCurrent->m_fireState;

		// Decrement weapon counters, server does this at same time ( during post think, after doing everything else )
		pto->m_flNextReload -= cmd->msec / 1000.0;
		pto->m_fNextAimBonus -= cmd->msec / 1000.0;
		*(int*)&pto->m_flNextPrimaryAttack -= cmd->msec;
		*(int*)&pto->m_flNextSecondaryAttack -= cmd->msec;
		*(int*)&pto->m_flTimeWeaponIdle -= cmd->msec;
		pto->fuser1 -= cmd->msec / 1000.0;

		pCurrent->DecrementTimers(cmd->msec);

		pCurrent->GetWeaponData(*pto);

		if (pto->m_fNextAimBonus < -1.0)
		{
			pto->m_fNextAimBonus = -1.0;
		}

		if (*(int*)&pto->m_flNextPrimaryAttack < -1100)
		{
			*(int*)&pto->m_flNextPrimaryAttack = -1100;
		}

		if (*(int*)&pto->m_flNextSecondaryAttack < -1)
		{
			*(int*)&pto->m_flNextSecondaryAttack = -1;
		}

		if (*(int*)&pto->m_flTimeWeaponIdle < -1)
		{
			*(int*)&pto->m_flTimeWeaponIdle = -1;
		}

		if (pto->m_flNextReload < -0.001)
		{
			pto->m_flNextReload = -0.001;
		}

		if (pto->fuser1 < -0.001)
		{
			pto->fuser1 = -0.001;
		}
	}

	// m_flNextAttack is now part of the weapons, but is part of the player instead
	*(int*)&to->client.m_flNextAttack -= cmd->msec;
	if (*(int*)&to->client.m_flNextAttack < -1)
	{
		*(int*)&to->client.m_flNextAttack = -1;
	}

	to->client.fuser2 -= cmd->msec / 1000.0;
	if (to->client.fuser2 < -0.001)
	{
		to->client.fuser2 = -0.001;
	}

	to->client.fuser3 -= cmd->msec / 1000.0;
	if (to->client.fuser3 < -0.001)
	{
		to->client.fuser3 = -0.001;
	}

	// Store off the last position from the predicted state.
	HUD_SetLastOrg();

	// Wipe it so we can't use it after this frame
	g_finalstate = NULL;
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
void DLLEXPORT HUD_PostRunCmd(struct local_state_s* from, struct local_state_s* to, struct usercmd_s* cmd, int runfuncs, double time, unsigned int random_seed)
{
	//	RecClPostRunCmd(from, to, cmd, runfuncs, time, random_seed);

	g_runfuncs = runfuncs != 0;

	//Event code depends on this stuff, so always initialize it.
	HUD_InitClientWeapons();
	HUD_WeaponsPostThink(from, to, cmd, time, random_seed);

	if (g_irunninggausspred)
	{
		Vector forward;
		gEngfuncs.pfnAngleVectors(v_angles, forward, NULL, NULL);
		to->client.velocity = to->client.velocity - forward * g_flApplyVel * 5;
		g_irunninggausspred = false;
	}

	g_CurrentWeaponId = to->client.m_iId;
}
