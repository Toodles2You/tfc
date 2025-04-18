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
#include "eventscripts.h"

extern TEMPENTITY* pLaserDot;

extern Vector g_PunchAngle;

extern short g_sModelIndexFire;
extern short g_sModelIndexFireLoop;

Vector g_CrosshairTarget;

static void GetCrosshairTarget(pmtrace_t* tr, float distance, const int ignoreEntity)
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

	EV_TraceLine(vecSrc, vecEnd, PM_STUDIO_BOX, ignoreEntity, *tr);

	if (tr->startsolid || tr->allsolid)
	{
		tr->fraction = 8.0f / distance;
		tr->endpos = vecSrc + 8.0f * forward;
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

	switch (g_iTeamNumber)
	{
	case TEAM_BLUE:
		pLaserDot->entity.curstate.rendercolor.r = 51;
		pLaserDot->entity.curstate.rendercolor.g = 51;
		pLaserDot->entity.curstate.rendercolor.b = 255;
		pLaserDot->entity.curstate.renderamt = 255;
		break;
	case TEAM_RED:
		pLaserDot->entity.curstate.rendercolor.r = 255;
		pLaserDot->entity.curstate.rendercolor.g = 0;
		pLaserDot->entity.curstate.rendercolor.b = 0;
		pLaserDot->entity.curstate.renderamt = 255;
		break;
	case TEAM_YELLOW:
		pLaserDot->entity.curstate.rendercolor.r = 255;
		pLaserDot->entity.curstate.rendercolor.g = 255;
		pLaserDot->entity.curstate.rendercolor.b = 0;
		pLaserDot->entity.curstate.renderamt = 191;
		break;
	case TEAM_GREEN:
		pLaserDot->entity.curstate.rendercolor.r = 51;
		pLaserDot->entity.curstate.rendercolor.g = 255;
		pLaserDot->entity.curstate.rendercolor.b = 51;
		pLaserDot->entity.curstate.renderamt = 191;
		break;
	default:
		pLaserDot->entity.curstate.rendercolor.r = 255;
		pLaserDot->entity.curstate.rendercolor.g = 0;
		pLaserDot->entity.curstate.rendercolor.b = 0;
		pLaserDot->entity.curstate.renderamt = 255;
		break;
	}
}

static void EV_PlayerFlameThink(TEMPENTITY* ent, float frametime, float currenttime)
{
	const auto& playerInfo = g_PlayerExtraInfo[ent->clientIndex];

	/* Player is no longer burning. */
	if (playerInfo.dead)
	{
		ent->die = currenttime;
		return;
	}

	auto player = client::GetEntityByIndex(ent->clientIndex);

	/* Player is no longer burning. */
	if (player == nullptr || (player->curstate.eflags & EFLAG_BURNING) == 0)
	{
		ent->die = currenttime;
		return;
	}

	ent->die = currenttime + 0.1F;

	if (ent->entity.curstate.fuser1 <= currenttime)
	{
		/* Spawn a couple of extra flames. */
		client::efx::FireField(
			player->origin + Vector(0.0F, 0.0F, client::RandomFloat(-20.0F, 20.0F)),
			16,
			g_sModelIndexFire,
			1,
			TEFIRE_FLAG_SOMEFLOAT,
			0.001F);

		ent->entity.curstate.fuser1 = currenttime + 0.1F;
	}
}

static void EV_PlayerFlameSpawn(const float time, cl_entity_t *entity)
{
	auto fire = client::efx::TempSprite(
		entity->origin,
		g_vecZero,
		1.0F,
		g_sModelIndexFireLoop,
		kRenderTransAlpha,
		kRenderFxNoDissipation,
		1.0F,
		0.2F,
		FTENT_SPRANIMATE | FTENT_PERSIST | FTENT_PLYRATTACHMENT | FTENT_SPRANIMATELOOP | FTENT_CLIENTCUSTOM);

	if (fire != nullptr)
	{
		fire->entity.baseline.scale = 4.0F;
		fire->entity.curstate.fuser1 = time;
		fire->clientIndex = entity->index;
		fire->callback = EV_PlayerFlameThink;
	}
}

static void UpdateEntityEffects(const float time, cl_entity_t *entity)
{
	/* This player has just started burning. Spawn a flame on them. */
	if ((entity->curstate.eflags & EFLAG_BURNING) != 0
	 && (entity->prevstate.eflags & EFLAG_BURNING) == 0)
	{
		EV_PlayerFlameSpawn(time, entity);
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
	const auto maxClients = client::GetMaxClients();
	pmtrace_t trShort;
	pmtrace_t trLong;

	const auto ignoreEntity = client::GetLocalPlayer()->index;

	EV_TracePush(ignoreEntity);

	GetCrosshairTarget(&trShort, 2048, ignoreEntity);

	if (trShort.fraction == 1.0f)
	{
		GetCrosshairTarget(&trLong, 8192, ignoreEntity);
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

	cl_entity_t* target = nullptr;

	if (gHUD.IsAlive())
	{
		const auto entIndex = client::event::IndexFromTrace(&trLong);

		if (entIndex > 0)
		{
			target = client::GetEntityByIndex(entIndex);
		}
	}

	gHUD.m_StatusBar.UpdateStatusBar(target);

	EV_TracePop();

	for (auto entIndex = 1; entIndex <= maxClients; entIndex++)
	{
		if (g_PlayerExtraInfo[entIndex].dead)
		{
			continue;
		}

		auto player = client::GetEntityByIndex(entIndex);

		if (player == nullptr)
		{
			continue;
		}

		UpdateEntityEffects(time, player);
	}
}
