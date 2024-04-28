//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: View / refresh setup functions
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "cl_util.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"

#include "entity_state.h"
#include "cl_entity.h"
#include "com_weapons.h"
#include "ref_params.h"
#include "in_defs.h"
#include "pm_movevars.h"
#include "pm_shared.h"
#include "pm_defs.h"
#include "event_api.h"
#include "pmtrace.h"
#include "screenfade.h"
#include "shake.h"
#include "hltv.h"
#include "Exports.h"
#include "view.h"

#include "r_studioint.h"
#include "com_model.h"
#include "kbutton.h"

#include <algorithm>

int CL_IsThirdPerson();
void CL_CameraOffset(float* ofs);
void VectorAngles(const float* forward, float* angles);

void V_CalcSpectatorRefdef(ref_params_t* pparams);

extern cvar_t* cl_rollangle;
extern cvar_t* cl_rollspeed;
extern cvar_t* cl_bobtilt;

Vector v_origin;
Vector v_angles;
Vector v_cl_angles;
Vector v_lastAngles;

static Vector ev_punchangle;
static Vector ev_punchangleVel;

static cvar_t* v_oldpunch;

cvar_t* cl_bobcycle;
cvar_t* cl_bob;
cvar_t* cl_bobup;
cvar_t* cl_waterdist;
cvar_t* cl_chasedist;
cvar_t* cl_bobview;

// These cvars are not registered (so users can't cheat), so set the ->value field directly
// Register these cvars in V_Init() if needed for easy tweaking
cvar_t v_iyaw_cycle = {"v_iyaw_cycle", "2", 0, 2};
cvar_t v_iroll_cycle = {"v_iroll_cycle", "0.5", 0, 0.5};
cvar_t v_ipitch_cycle = {"v_ipitch_cycle", "1", 0, 1};
cvar_t v_iyaw_level = {"v_iyaw_level", "0.3", 0, 0.3};
cvar_t v_iroll_level = {"v_iroll_level", "0.1", 0, 0.1};
cvar_t v_ipitch_level = {"v_ipitch_level", "0.3", 0, 0.3};

float v_idlescale; // used by TFC for concussion grenade effect


typedef struct {
	float time;
	float lastTime;
	float value;
	float cycleValue;
} bob_params_t;

static float V_CalcBob(
	ref_params_t* pparams,
	bob_params_t& bob,
	const float clBob,
	const float clBobCycle,
	const float clBobUp)
{
	float cycle;
	Vector2D vel;

	if (pparams->onground == -1
	 || pparams->time == bob.lastTime)
	{
		// just use old value
		return bob.value;
	}

	bob.lastTime = pparams->time;

	//TODO: bobtime will eventually become a value so large that it will no longer behave properly.
	//Consider resetting the variable if a level change is detected (pparams->time < lasttime might do the trick).
	bob.time += pparams->frametime;
	cycle = bob.time - (int)(bob.time / clBobCycle) * clBobCycle;
	cycle /= clBobCycle;

	if (cycle < clBobUp)
	{
		cycle = M_PI * cycle / clBobUp;
	}
	else
	{
		cycle = M_PI + M_PI * (cycle - clBobUp) / (1.0 - clBobUp);
	}

	// bob is proportional to simulated velocity in the xy plane
	// (don't count Z, or jumping messes it up)
	vel = Vector2D(pparams->simvel[0], pparams->simvel[1]);

	bob.value = std::min(vel.Length(), 400.0F) * clBob;
	bob.cycleValue = bob.value * sin(cycle);

	return bob.value;
}


static float V_CalcRoll(Vector angles, Vector velocity, float rollangle, float rollspeed)
{
	float sign;
	float side;
	float value;
	Vector forward, right, up;

	AngleVectors(angles, forward, right, up);

	side = DotProduct(velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabs(side);

	value = rollangle;
	if (side < rollspeed)
	{
		side = side * value / rollspeed;
	}
	else
	{
		side = value;
	}
	return side * sign;
}


/*
==================
V_CalcGunAngle
==================
*/
void V_CalcGunAngle(ref_params_t* pparams)
{
	cl_entity_t* viewent;

	viewent = gEngfuncs.GetViewModel();
	if (!viewent)
		return;

	viewent->angles[YAW] = pparams->viewangles[YAW] + pparams->punchangle[YAW] * 2;
	viewent->angles[PITCH] = -(pparams->viewangles[PITCH] + pparams->punchangle[PITCH] * 2);
	viewent->angles[ROLL] -= v_idlescale * sin(pparams->time * v_iroll_cycle.value) * v_iroll_level.value;

	// don't apply all of the v_ipitch to prevent normally unseen parts of viewmodel from coming into view.
	viewent->angles[PITCH] += v_idlescale * sin(pparams->time * v_ipitch_cycle.value) * v_ipitch_level.value;
	viewent->angles[YAW] -= v_idlescale * sin(pparams->time * v_iyaw_cycle.value) * v_iyaw_level.value;

	VectorCopy(viewent->angles, viewent->curstate.angles);
	VectorCopy(viewent->angles, viewent->latched.prevangles);
}


/*
==============
V_AddIdle

Idle swaying
==============
*/
void V_AddIdle(ref_params_t* pparams)
{
	pparams->viewangles[ROLL] += v_idlescale * sin(pparams->time * v_iroll_cycle.value) * v_iroll_level.value;
	pparams->viewangles[PITCH] += v_idlescale * sin(pparams->time * v_ipitch_cycle.value) * v_ipitch_level.value;
	pparams->viewangles[YAW] += v_idlescale * sin(pparams->time * v_iyaw_cycle.value) * v_iyaw_level.value;
}


static void V_CalcViewRoll(ref_params_t* pparams)
{
	if (cl_rollangle->value == 0.0F || cl_rollspeed->value == 0.0F)
		return;

	float side;
	cl_entity_t* viewentity;

	viewentity = gEngfuncs.GetEntityByIndex(pparams->viewentity);
	if (!viewentity)
		return;

	side = V_CalcRoll(viewentity->angles, pparams->simvel, cl_rollangle->value, cl_rollspeed->value);

	pparams->viewangles[ROLL] += side;
}


static void V_CalcIntermissionRefdef(ref_params_t* pparams)
{
	cl_entity_t *ent, *view;
	float old;

	// ent is the player model ( visible when out of body )
	ent = gEngfuncs.GetLocalPlayer();

	// view is the weapon model (only visible from inside body )
	view = gEngfuncs.GetViewModel();

	VectorCopy(pparams->simorg, pparams->vieworg);
	VectorCopy(pparams->cl_viewangles, pparams->viewangles);

	view->model = NULL;

	// always idle in intermission
	old = v_idlescale;
	v_idlescale = 1;

	V_AddIdle(pparams);

	if (0 != gEngfuncs.IsSpectateOnly())
	{
		// in HLTV we must go to 'intermission' position by ourself
		VectorCopy(gHUD.m_Spectator.m_cameraOrigin, pparams->vieworg);
		VectorCopy(gHUD.m_Spectator.m_cameraAngles, pparams->viewangles);
	}

	v_idlescale = old;

	v_cl_angles = pparams->cl_viewangles;
	v_origin = pparams->vieworg;
	v_angles = pparams->viewangles;
}


/*
Bigger number makes the response more damped, smaller is less damped.
Currently the system will overshoot, with larger damping values it won't.
*/
#define PUNCH_DAMPING 9.0f

/*
Bigger number increases the speed at which the view corrects.
*/
#define PUNCH_SPRING_CONSTANT 65.0f

static void V_DropPunchAngle(float frametime)
{
	if (v_oldpunch->value == 1)
	{
		float len;
		len = VectorNormalize(ev_punchangle);
		len -= (10.0 + len * 0.5) * frametime;
		len = std::max(len, 0.0F);
		VectorScale(ev_punchangle, len, ev_punchangle);
		return;
	}

	if (ev_punchangle.LengthSquared() > 0.001F || ev_punchangleVel.LengthSquared() > 0.001F)
	{
		ev_punchangle = ev_punchangle + ev_punchangleVel * frametime;
		float damping = std::max(1 - (PUNCH_DAMPING * frametime), 0.0F);

		ev_punchangleVel = ev_punchangleVel * damping;

		// Torsional spring.
		float springForceMagnitude = PUNCH_SPRING_CONSTANT * frametime;
		springForceMagnitude = std::clamp(springForceMagnitude, 0.0F, 2.0F);
		ev_punchangleVel = ev_punchangleVel - ev_punchangle * springForceMagnitude;

		// Don't wrap around.
		ev_punchangle.x = std::clamp(ev_punchangle.x, -89.F, 89.F),
		ev_punchangle.y = std::clamp(ev_punchangle.y, -179.F, 179.F),
		ev_punchangle.z = std::clamp(ev_punchangle.z, -89.F, 89.F);
	}
	else
	{
		ev_punchangle = g_vecZero;
		ev_punchangleVel = g_vecZero;
	}
}


void V_PunchAxis(int axis, float punch)
{
	ev_punchangle[axis] = punch;
}


void V_ResetPunchAngle()
{
	ev_punchangle = g_vecZero;
	ev_punchangleVel = g_vecZero;
}


static void V_CalcNormalRefdef(ref_params_t* pparams)
{
	cl_entity_t *ent, *view;
	int i;
	Vector angles;
	float bob, waterOffset;
	Vector camAngles, camForward, camRight, camUp;
	cl_entity_t* pwater;
	static bob_params_t bobParams;

	if (0 != gEngfuncs.IsSpectateOnly())
	{
		ent = gEngfuncs.GetEntityByIndex(gHUD.GetObserverTarget());
	}
	else
	{
		// ent is the player model ( visible when out of body )
		ent = gEngfuncs.GetLocalPlayer();
	}

	// view is the weapon model (only visible from inside body )
	view = gEngfuncs.GetViewModel();

	// refresh position
	VectorCopy(pparams->simorg, pparams->vieworg);

	if (cl_bob->value != 0.0F)
	{
		// transform the view offset by the model's matrix to get the offset from
		// model origin for the view
		V_CalcBob(pparams, bobParams, cl_bob->value, cl_bobcycle->value, cl_bobup->value);

		bob = bobParams.value * 0.3 + bobParams.cycleValue * 0.7;

		if (0 != cl_bobview->value)
		{
			pparams->vieworg[2] += bob;
		}
	}

	VectorAdd(pparams->vieworg, pparams->viewheight, pparams->vieworg);

	VectorCopy(pparams->cl_viewangles, pparams->viewangles);

	gEngfuncs.V_CalcShake();
	gEngfuncs.V_ApplyShake(pparams->vieworg, pparams->viewangles, 1.0);

	// Check for problems around water, move the viewer artificially if necessary
	// -- this prevents drawing errors in GL due to waves

	waterOffset = 0;
	if (pparams->waterlevel >= kWaterLevelWaist)
	{
		int i, contents, waterDist, waterEntity;
		Vector point;
		waterDist = cl_waterdist->value;

		if (0 != pparams->hardware)
		{
			waterEntity = gEngfuncs.PM_WaterEntity(pparams->simorg);
			if (waterEntity >= 0 && waterEntity < pparams->max_entities)
			{
				pwater = gEngfuncs.GetEntityByIndex(waterEntity);
				if (pwater && (pwater->model != NULL))
				{
					waterDist += (pwater->curstate.scale * 16); // Add in wave height
				}
			}
		}
		else
		{
			waterEntity = 0; // Don't need this in software
		}

		VectorCopy(pparams->vieworg, point);

		// Eyes are above water, make sure we're above the waves
		if (pparams->waterlevel == kWaterLevelWaist)
		{
			point[2] -= waterDist;
			for (i = 0; i < waterDist; i++)
			{
				contents = gEngfuncs.PM_PointContents(point, NULL);
				if (contents > CONTENTS_WATER)
					break;
				point[2] += 1;
			}
			waterOffset = (point[2] + waterDist) - pparams->vieworg[2];
		}
		else
		{
			// eyes are under water.  Make sure we're far enough under
			point[2] += waterDist;

			for (i = 0; i < waterDist; i++)
			{
				contents = gEngfuncs.PM_PointContents(point, NULL);
				if (contents <= CONTENTS_WATER)
					break;
				point[2] -= 1;
			}
			waterOffset = (point[2] - waterDist) - pparams->vieworg[2];
		}
	}

	pparams->vieworg[2] += waterOffset;

	V_CalcViewRoll(pparams);

	V_AddIdle(pparams);

	// offsets
	VectorCopy(pparams->cl_viewangles, angles);

	AngleVectors(angles, pparams->forward, pparams->right, pparams->up);

	// Treating cam_ofs[2] as the distance
	if (0 != CL_IsThirdPerson())
	{
		Vector ofs;

		ofs[0] = ofs[1] = ofs[2] = 0.0;

		CL_CameraOffset((float*)&ofs);

		VectorCopy(ofs, camAngles);
		camAngles[ROLL] = 0;

		AngleVectors(camAngles, camForward, camRight, camUp);

		for (i = 0; i < 3; i++)
		{
			pparams->vieworg[i] += -ofs[2] * camForward[i];
		}
	}

	// Give gun our viewangles
	VectorCopy(pparams->cl_viewangles, view->angles);

	// set up gun position
	V_CalcGunAngle(pparams);

	// Use predicted origin as view origin.
	VectorCopy(pparams->simorg, view->origin);
	view->origin[2] += (waterOffset);
	VectorAdd(view->origin, pparams->viewheight, view->origin);

	// Let the viewmodel shake at about 10% of the amplitude
	gEngfuncs.V_ApplyShake(view->origin, view->angles, 0.9);

	if (cl_bob->value != 0.0F)
	{
		for (i = 0; i < 3; i++)
		{
			view->origin[i] += bob * 0.4 * pparams->forward[i] - pparams->up[i];
		}

		if (0 != cl_bobview->value)
		{
			view->origin[2] += bob;
		}

		// throw in a little tilt.
		if (0 != cl_bobtilt->value)
		{
			view->angles[YAW] -= bob * 0.5;
			view->angles[ROLL] -= bob * 1;
			view->angles[PITCH] -= bob * 0.3;

			VectorCopy(view->angles, view->curstate.angles);
		}
	}
	else
	{
		for (i = 0; i < 3; i++)
		{
			view->origin[i] += -pparams->up[i];
		}
	}

	// Add in the punchangle, if any
	VectorAdd(pparams->viewangles, pparams->punchangle, pparams->viewangles);

	// Include client side punch, too
	VectorAdd(pparams->viewangles, (float*)&ev_punchangle, pparams->viewangles);

	V_DropPunchAngle(pparams->frametime);

	// Store off v_angles before munging for third person
	v_angles = pparams->viewangles;
	v_lastAngles = pparams->viewangles;
	if (0 != CL_IsThirdPerson())
	{
		VectorCopy(camAngles, pparams->viewangles);
	}

	//Apply this at all times
	{
		float pitch = pparams->viewangles[0];

		// Normalize angles
		if (pitch > 180)
			pitch -= 360.0;
		else if (pitch < -180)
			pitch += 360;

		// Player pitch is inverted
		pitch /= -3.0;

		// Slam local player's pitch value
		ent->angles[0] = pitch;
		ent->curstate.angles[0] = pitch;
		ent->prevstate.angles[0] = pitch;
		ent->latched.prevangles[0] = pitch;
	}

	// override all previous settings if the viewent isn't the client
	if (pparams->viewentity > pparams->maxclients)
	{
		cl_entity_t* viewentity;
		viewentity = gEngfuncs.GetEntityByIndex(pparams->viewentity);
		if (viewentity)
		{
			VectorCopy(viewentity->origin, pparams->vieworg);
			VectorCopy(viewentity->angles, pparams->viewangles);

			// Store off overridden viewangles
			v_angles = pparams->viewangles;
		}
	}

	v_origin = pparams->vieworg;
}


void V_CalcRefdef(ref_params_t* pparams)
{
	// intermission / finale rendering
	if (0 != pparams->intermission)
	{
		V_ResetPunchAngle();
		V_CalcIntermissionRefdef(pparams);
	}
	else if (0 != pparams->spectator || gHUD.IsObserver())
	{
		V_ResetPunchAngle();
		V_CalcSpectatorRefdef(pparams);
	}
	else if (0 == pparams->paused)
	{
		V_CalcNormalRefdef(pparams);
	}
}


void V_Init()
{
	v_oldpunch = gEngfuncs.pfnRegisterVariable("v_oldpunch", "0", 0);

	cl_bobcycle = gEngfuncs.pfnRegisterVariable("cl_bobcycle", "0.8", 0);
	cl_bob = gEngfuncs.pfnRegisterVariable("cl_bob", "0.00", 0);
	cl_bobup = gEngfuncs.pfnRegisterVariable("cl_bobup", "0.5", 0);
	cl_waterdist = gEngfuncs.pfnRegisterVariable("cl_waterdist", "4", 0);
	cl_chasedist = gEngfuncs.pfnRegisterVariable("cl_chasedist", "112", 0);
	cl_bobview = gEngfuncs.pfnRegisterVariable("cl_bobview", "0", 0);
}

