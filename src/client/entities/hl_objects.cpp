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
#include "hud.h"
#include "cl_util.h"
#include "demo.h"

#include "demo_api.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "com_weapons.h"

#include "pm_defs.h"
#include "event_api.h"
#include "entity_types.h"
#include "r_efx.h"

extern TEMPENTITY* pLaserDot;

extern Vector g_PunchAngle;

Vector g_CrosshairTarget;

static void GetCrosshairTarget(pmtrace_t* tr, float distance)
{
	Vector forward, vecSrc, vecEnd, origin, angles, right, up;
	Vector view_ofs;

	// Get our exact viewangles from engine
	client::GetViewAngles(angles);
	angles = angles + g_PunchAngle;

	// Determine our last predicted origin
	origin = HUD_GetLastOrg();

	AngleVectors(angles, &forward, &right, &up);

	vecSrc = origin;

	vecEnd = vecSrc + distance * forward;

	client::event::PlayerTrace(vecSrc, vecEnd, PM_STUDIO_BOX, -1, tr);

	if (tr->startsolid || tr->allsolid)
	{
		tr->fraction = 1.0f / distance;
		tr->endpos = vecSrc + forward;
	}
}

static void UpdateLaserDot(const float time, const pmtrace_t *tr)
{
	if (g_CurrentWeaponId != WEAPON_SNIPER_RIFLE)
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

	if (g_iTeamNumber == TEAM_BLUE)
	{
		pLaserDot->entity.curstate.rendercolor.r = 51;
		pLaserDot->entity.curstate.rendercolor.g = 51;
		pLaserDot->entity.curstate.rendercolor.b = 255;
		pLaserDot->entity.curstate.renderamt = 255;
	}
	else
	{
		pLaserDot->entity.curstate.rendercolor.r = 255;
		pLaserDot->entity.curstate.rendercolor.g = 0;
		pLaserDot->entity.curstate.rendercolor.b = 0;
		pLaserDot->entity.curstate.renderamt = 255;
	}
}

/*
=====================
Game_AddObjects

Add game specific, client-side objects here
=====================
*/
void Game_AddObjects()
{
	const auto time = client::GetClientTime();
	pmtrace_t trShort;
	pmtrace_t trLong;

	cl_entity_t* pthisplayer = client::GetLocalPlayer();
	int idx = pthisplayer->index;
	client::event::SetUpPlayerPrediction(false, true);
	client::event::PushPMStates();
	client::event::SetSolidPlayers(idx - 1);
	client::event::SetTraceHull(kHullPoint);

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
	
	if (pLaserDot)
	{
		UpdateLaserDot(time, &trLong);
	}
	
	if (!gHUD.IsObserver())
	{
		if (gHUD.IsAlive() && gHUD.m_Flash.IsFlashlightOn())
		{
			auto light = client::efx::AllocDlight(0);

			if (light)
			{
				light->origin = trShort.endpos;
				light->color.r = 255;
				light->color.g = 255;
				light->color.b = 255;
				light->die = time + 0.001;
				light->radius = 96 * (1.0f - trShort.fraction);
			}
		}

		cl_entity_t* target = nullptr;

		if (gHUD.IsAlive() && trLong.fraction != 1.0f)
		{
			auto entIndex = client::event::IndexFromTrace(&trLong);
			if (entIndex >= 1 && entIndex <= client::GetMaxClients())
			{
				target = client::GetEntityByIndex(entIndex);
			}
		}

		gHUD.m_StatusBar.UpdateStatusBar(target);
	}

	client::event::PopPMStates();
}
