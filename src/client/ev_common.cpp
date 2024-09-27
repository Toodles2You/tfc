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
// shared event functions
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"

#include "r_efx.h"

#include "eventscripts.h"
#include "event_api.h"
#include "pm_shared.h"

/*
=================
GetEntity

Return's the requested cl_entity_t
=================
*/
struct cl_entity_s* GetEntity(int idx)
{
	return client::GetEntityByIndex(idx);
}

/*
=================
GetViewEntity

Return's the current weapon/view model
=================
*/
struct cl_entity_s* GetViewEntity()
{
	return client::GetViewModel();
}

/*
=================
EV_CreateTracer

Creates a tracer effect
=================
*/
void EV_CreateTracer(const Vector& start, const Vector& end)
{
	client::efx::TracerEffect(const_cast<Vector&>(start), const_cast<Vector&>(end));
}

/*
=================
EV_IsPlayer

Is the entity's index in the player range?
=================
*/
bool EV_IsPlayer(int idx)
{
	if (idx >= 1 && idx <= client::GetMaxClients())
		return true;

	return false;
}

/*
=================
EV_IsLocal

Is the entity == the local player
=================
*/
bool EV_IsLocal(int idx)
{
	// check if we are in some way in first person spec mode
	return client::event::IsLocal(idx - 1) != 0;
}

/*
=================
EV_HasSuperDamage

Toodles FIXME: Predict this & set as an event parameter.
=================
*/
bool EV_HasSuperDamage(int idx)
{
	if (!EV_IsPlayer(idx))
	{
		return false;
	}

	const auto player = client::GetEntityByIndex(idx);

	if (player == nullptr)
	{
		return false;
	}

	return (player->curstate.eflags & EFLAG_SUPER_DAMAGE) != 0;
}

/*
=================
EV_GetGunPosition

Figure out the height of the gun
=================
*/
void EV_GetGunPosition(event_args_t* args, Vector& pos, const Vector& origin)
{
	int idx;

	idx = args->entindex;

	Vector view_ofs = VEC_VIEW;

	if (EV_IsPlayer(idx))
	{
		// in spec mode use entity viewheigh, not own
		if (EV_IsLocal(idx))
		{
			// Grab predicted result for local player
			client::event::LocalPlayerViewheight(view_ofs);
		}
		else
		{
			view_ofs.z = args->fparam2;
		}
	}

	pos = origin + view_ofs;
}

/*
=================
EV_EjectBrass

Bullet shell casings
=================
*/
void EV_EjectBrass(const Vector& origin, const Vector& velocity, float rotation, int model, int soundtype)
{
	Vector endpos {0.0F, rotation, 0.0F};

	auto shell =
		client::efx::TempModel(
			const_cast<Vector&>(origin),
			const_cast<Vector&>(velocity),
			endpos,
			2.5F,
			model,
			soundtype);

	if (shell != nullptr)
	{
		shell->flags |= FTENT_FADEOUT;
	}
}

/*
=================
EV_GetDefaultShellInfo

Determine where to eject shells from
=================
*/
void EV_GetDefaultShellInfo(struct event_args_s* args, const Vector& origin, const Vector& velocity, Vector& ShellVelocity, Vector& ShellOrigin, const Vector& forward, const Vector& right, const Vector& up, float forwardScale, float upScale, float rightScale)
{
	int i;
	float fR, fU;

	int idx;

	idx = args->entindex;

	Vector view_ofs = VEC_VIEW;

	fR = client::RandomFloat(50, 70);
	fU = client::RandomFloat(100, 150);

	if (EV_IsPlayer(idx))
	{
		if (EV_IsLocal(idx))
		{
			client::event::LocalPlayerViewheight(view_ofs);
		}
		else if (args->ducking == 1)
		{
			view_ofs = VEC_DUCK_VIEW;
		}

		if (g_PlayerExtraInfo[idx].lefthanded)
		{
			rightScale = -rightScale;
			fR = -fR;
		}
	}

	for (i = 0; i < 3; i++)
	{
		ShellVelocity[i] = velocity[i] + right[i] * fR + up[i] * fU + forward[i] * 25;
		ShellOrigin[i] = origin[i] + view_ofs[i] + up[i] * upScale + forward[i] * forwardScale + right[i] * rightScale;
	}
}

/*
=================
EV_MuzzleFlash

Flag weapon/view model for muzzle flash
=================
*/
void EV_MuzzleFlash()
{
	// Add muzzle flash to current weapon model
	cl_entity_t* ent = GetViewEntity();
	if (!ent)
	{
		return;
	}

	// Or in the muzzle flash
	ent->curstate.effects |= EF_MUZZLEFLASH;
}
