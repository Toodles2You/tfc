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
#include "../hud.h"
#include "../cl_util.h"
#include "../demo.h"

#include "demo_api.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "com_weapons.h"

#include "pm_defs.h"
#include "event_api.h"
#include "entity_types.h"
#include "r_efx.h"

extern BEAM* pBeam;
extern BEAM* pBeam2;
extern TEMPENTITY* pFlare; // Vit_amiN: egon's energy flare
extern TEMPENTITY* pLaserDot;
void HUD_GetLastOrg(float* org);

extern Vector g_PunchAngle;

Vector g_CrosshairTarget;

static void GetCrosshairTarget(pmtrace_t* tr, float distance)
{
	Vector forward, vecSrc, vecEnd, origin, angles, right, up;
	Vector view_ofs;

	// Get our exact viewangles from engine
	gEngfuncs.GetViewAngles((float*)angles);
	angles = angles + g_PunchAngle;

	// Determine our last predicted origin
	HUD_GetLastOrg((float*)&origin);

	AngleVectors(angles, forward, right, up);

	VectorCopy(origin, vecSrc);

	VectorMA(vecSrc, distance, forward, vecEnd);

	gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecEnd, PM_STUDIO_BOX, -1, tr);
}

static void UpdateBeams(const float time, const pmtrace_t *tr)
{
	if (g_CurrentWeaponId != WEAPON_EGON)
	{
		if (pBeam)
		{
			pBeam->die = time;
			pBeam = nullptr;
		}
		if (pBeam2)
		{
			pBeam2->die = time;
			pBeam2 = nullptr;
		}
		if (pFlare)
		{
			pFlare->die = time;
			pFlare = nullptr;
		}
		return;
	}

	if (pBeam)
	{
		pBeam->target = tr->endpos;
		pBeam->die = time + 0.1; // We keep it alive just a little bit forward in the future, just in case.
	}

	if (pBeam2)
	{
		pBeam2->target = tr->endpos;
		pBeam2->die = time + 0.1; // We keep it alive just a little bit forward in the future, just in case.
	}

	if (pFlare) // Vit_amiN: beam flare
	{
		pFlare->entity.origin = tr->endpos;
		pFlare->die = time + 0.1f; // We keep it alive just a little bit forward in the future, just in case.

		if (util::IsDeathmatch()) // Singleplayer always draws the egon's energy beam flare
		{
			pFlare->flags |= FTENT_NOMODEL;

			if (tr->fraction != 1.0F) // Beam hit some non-world entity
			{
				physent_t* pEntity = gEngfuncs.pEventAPI->EV_GetPhysent(tr->ent);

				// Not the world, let's assume that we hit something organic ( dog, cat, uncle joe, etc )
				if (pEntity
				 && pEntity->solid != SOLID_BSP
				 && pEntity->movetype != MOVETYPE_PUSHSTEP)
				{
					pFlare->flags &= ~FTENT_NOMODEL;
				}
			}
		}
	}
}

static void UpdateLaserDot(const float time, const pmtrace_t *tr)
{
	if (g_CurrentWeaponId != WEAPON_RPG)
	{
		pLaserDot->die = time;
		pLaserDot = nullptr;
		return;
	}

	pLaserDot->die = time + 0.1f;

	if ((pLaserDot->flags & FTENT_NOMODEL) != 0)
	{
		if (pLaserDot->entity.baseline.fuser4 > time)
		{
			return;
		}
		pLaserDot->entity.baseline.fuser4 = 0.0f;
		pLaserDot->flags &= ~FTENT_NOMODEL;
	}

	pLaserDot->entity.origin = tr->endpos;
}

/*
=====================
Game_AddObjects

Add game specific, client-side objects here
=====================
*/
void Game_AddObjects()
{
	const auto time = gEngfuncs.GetClientTime();
	pmtrace_t trShort;
	pmtrace_t trLong;

	cl_entity_t* pthisplayer = gEngfuncs.GetLocalPlayer();
	int idx = pthisplayer->index;
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(0, 1);
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);

	GetCrosshairTarget(&trShort, 2048);

	if (trShort.fraction == 1.0f)
	{
		GetCrosshairTarget(&trLong, 8192);
	}
	else
	{
		trLong = trShort;
	}

	g_CrosshairTarget = trLong.endpos;

	if (pBeam || pBeam2 || pFlare)
	{
		UpdateBeams(time, &trShort);
	}
	
	if (pLaserDot)
	{
		UpdateLaserDot(time, &trLong);
	}
	
	if (gHUD.m_Flash.IsFlashlightOn())
	{
		auto light = gEngfuncs.pEfxAPI->CL_AllocDlight(0);

		if (light)
		{
			light->origin = trShort.endpos;
			light->color.r = 255;
			light->color.g = 255;
			light->color.b = 255;
			light->die = time + 0.001;
			light->radius = 64 * (1.0f - trShort.fraction);
		}
	}

	gEngfuncs.pEventAPI->EV_PopPMStates();
}
