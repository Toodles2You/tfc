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

#include <algorithm>


enum
{
    CAM_MODE_RELAX = 1,
    CAM_MODE_FOCUS,
};

int PM_GetPhysEntInfo(int ent);
void NormalizeAngles(float* angles);
float Distance(const float* v1, const float* v2);
void VectorAngles(const float* forward, float* angles);

extern Vector vJumpOrigin;
extern Vector vJumpAngles;

extern Vector v_origin;
extern Vector v_angles;
extern Vector v_cl_angles;
extern Vector v_lastAngles;

extern cvar_t* cl_chasedist;

Vector v_sim_org;
float v_frametime;
float v_lastDistance;
float v_cameraRelaxAngle = 5.0f;
float v_cameraFocusAngle = 35.0f;
int v_cameraMode = CAM_MODE_FOCUS;
bool v_resetCamera = true;


static void V_SmoothInterpolateAngles(float* startAngle, float* endAngle, float* finalAngle, float degreesPerSec)
{
	float absd, frac, d, threshhold;

	NormalizeAngles(startAngle);
	NormalizeAngles(endAngle);

	for (int i = 0; i < 3; i++)
	{
		d = endAngle[i] - startAngle[i];

		if (d > 180.0f)
		{
			d -= 360.0f;
		}
		else if (d < -180.0f)
		{
			d += 360.0f;
		}

		absd = fabs(d);

		if (absd > 0.01f)
		{
			frac = degreesPerSec * v_frametime;

			threshhold = degreesPerSec / 4;

			if (absd < threshhold)
			{
				float h = absd / threshhold;
				h *= h;
				frac *= h; // slow down last degrees
			}

			if (frac > absd)
			{
				finalAngle[i] = endAngle[i];
			}
			else
			{
				if (d > 0)
					finalAngle[i] = startAngle[i] + frac;
				else
					finalAngle[i] = startAngle[i] - frac;
			}
		}
		else
		{
			finalAngle[i] = endAngle[i];
		}
	}

	NormalizeAngles(finalAngle);
}


// Get the origin of the Observer based around the target's position and angles
static void V_GetChaseOrigin(float* angles, float* origin, float distance, float* returnvec)
{
	Vector vecEnd;
	Vector forward;
	Vector vecStart;
	pmtrace_t* trace;
	int maxLoops = 8;

	int ignoreent = -1; // first, ignore no entity

	cl_entity_t* ent = NULL;

	// Trace back from the target using the player's view angles
	AngleVectors(angles, forward, NULL, NULL);

	VectorScale(forward, -1, forward);

	vecStart = origin;

	VectorMA(vecStart, distance, forward, vecEnd);

	while (maxLoops > 0)
	{
		trace = gEngfuncs.PM_TraceLine(vecStart, vecEnd, PM_TRACELINE_PHYSENTSONLY, 2, ignoreent);

		// WARNING! trace->ent is is the number in physent list not the normal entity number

		if (trace->ent <= 0)
			break; // we hit the world or nothing, stop trace

		ent = gEngfuncs.GetEntityByIndex(PM_GetPhysEntInfo(trace->ent));

		if (ent == NULL)
			break;

		// hit non-player solid BSP , stop here
		if (ent->curstate.solid == SOLID_BSP && 0 == ent->player)
			break;

		// if close enought to end pos, stop, otherwise continue trace
		if (Distance(trace->endpos, vecEnd) < 1.0f)
		{
			break;
		}
		else
		{
			ignoreent = trace->ent; // ignore last hit entity
			vecStart = trace->endpos;
		}

		maxLoops--;
	}

	VectorMA(trace->endpos, 4, trace->plane.normal, returnvec);

	v_lastDistance = Distance(trace->endpos, origin); // real distance without offset
}


static void V_GetDeathCam(int victim, int killer, float* cl_angles, float* origin, float* angle)
{
	Vector newAngle;
	Vector newOrigin;

	float distance = 168.0f;

	cl_entity_t* victimEnt = gEngfuncs.GetEntityByIndex(victim);
	cl_entity_t* killerEnt = nullptr;
	if (killer != 0)
	{
		killerEnt = gEngfuncs.GetEntityByIndex(killer);
	}

	v_lastDistance += v_frametime * 96.0f; // move unit per seconds back

	if (v_resetCamera)
		v_lastDistance = 64.0f;

	if (distance > v_lastDistance)
		distance = v_lastDistance;

	newOrigin = victimEnt->origin;

	if (victimEnt->player)
		newOrigin[2] += 17; // head level of living player

	// get new angle towards second target
	if (killerEnt != nullptr && killerEnt != victimEnt)
	{
		VectorSubtract(killerEnt->origin, victimEnt->origin, newAngle);
		VectorAngles(newAngle, newAngle);
		newAngle[0] = -newAngle[0];
		cl_angles = newAngle;
	}
	else
	{
		// if no second target is given, look down to dead player
		newAngle[PITCH] = 90.0F;
		newAngle[YAW] = angle[YAW];
		newAngle[ROLL] = 0.0F;
	}

	// and smooth view
	V_SmoothInterpolateAngles(v_lastAngles, newAngle, angle, 120.0f);

	V_GetChaseOrigin(angle, newOrigin, distance, origin);

	v_lastAngles = angle;
}


static void V_GetSingleTargetCam(cl_entity_t* ent1, float* angle, float* origin)
{
	Vector newAngle;
	Vector newOrigin;

	int flags = gHUD.m_Spectator.m_iObserverFlags;

	// see is target is a dead player
	bool deadPlayer = 0 != ent1->player && (ent1->curstate.solid == SOLID_NOT);

	float dfactor = (flags & DRC_FLAG_DRAMATIC) != 0 ? -1.0f : 1.0f;

	float distance = 112.0f + (16.0f * dfactor); // get close if dramatic;

	// go away in final scenes or if player just died
	if ((flags & DRC_FLAG_FINAL) != 0)
		distance *= 2.0f;
	else if (deadPlayer)
		distance *= 1.5f;

	// let v_lastDistance float smoothly away
	v_lastDistance += v_frametime * 32.0f; // move unit per seconds back

	if (distance > v_lastDistance)
		distance = v_lastDistance;

	newOrigin = ent1->origin;

	if (0 != ent1->player)
	{
		if (deadPlayer)
			newOrigin[2] += 2; //laying on ground
		else
			newOrigin[2] += 17; // head level of living player
	}
	else
		newOrigin[2] += 8; // object, tricky, must be above bomb in CS

	// we have no second target, choose view direction based on
	// show front of primary target
	newAngle = ent1->angles;

	// show dead players from front, normal players back
	if ((flags & DRC_FLAG_FACEPLAYER) != 0)
		newAngle[1] += 180.0f;


	newAngle[0] += 12.5f * dfactor; // lower angle if dramatic

	// if final scene (bomb), show from real high pos
	if ((flags & DRC_FLAG_FINAL) != 0)
		newAngle[0] = 22.5f;

	// choose side of object/player
	if ((flags & DRC_FLAG_SIDE) != 0)
		newAngle[1] += 22.5f;
	else
		newAngle[1] -= 22.5f;

	V_SmoothInterpolateAngles(v_lastAngles, newAngle, angle, 120.0f);

	// HACK, if player is dead don't clip against his dead body, can't check this
	V_GetChaseOrigin(angle, newOrigin, distance, origin);
}


static float MaxAngleBetweenAngles(float* a1, float* a2)
{
	float d, maxd = 0.0f;

	NormalizeAngles(a1);
	NormalizeAngles(a2);

	for (int i = 0; i < 3; i++)
	{
		d = a2[i] - a1[i];
		if (d > 180)
		{
			d -= 360;
		}
		else if (d < -180)
		{
			d += 360;
		}

		d = fabs(d);

		if (d > maxd)
			maxd = d;
	}

	return maxd;
}

void V_GetDoubleTargetsCam(cl_entity_t* ent1, cl_entity_t* ent2, float* angle, float* origin)
{
	Vector newAngle;
	Vector newOrigin;
	Vector tempVec;

	int flags = gHUD.m_Spectator.m_iObserverFlags;

	float dfactor = (flags & DRC_FLAG_DRAMATIC) != 0 ? -1.0f : 1.0f;

	float distance = 112.0f + (16.0f * dfactor); // get close if dramatic;

	// go away in final scenes or if player just died
	if ((flags & DRC_FLAG_FINAL) != 0)
		distance *= 2.0f;

	// let v_lastDistance float smoothly away
	v_lastDistance += v_frametime * 32.0f; // move unit per seconds back

	if (distance > v_lastDistance)
		distance = v_lastDistance;

	newOrigin = ent1->origin;

	if (0 != ent1->player)
		newOrigin[2] += 17; // head level of living player
	else
		newOrigin[2] += 8; // object, tricky, must be above bomb in CS

	// get new angle towards second target
	VectorSubtract(ent2->origin, ent1->origin, newAngle);

	VectorAngles(newAngle, newAngle);
	newAngle[0] = -newAngle[0];

	// set angle diffrent in Dramtaic scenes
	newAngle[0] += 12.5f * dfactor; // lower angle if dramatic

	if ((flags & DRC_FLAG_SIDE) != 0)
		newAngle[1] += 22.5f;
	else
		newAngle[1] -= 22.5f;

	float d = MaxAngleBetweenAngles(v_lastAngles, newAngle);

	if ((d < v_cameraFocusAngle) && (v_cameraMode == CAM_MODE_RELAX))
	{
		// difference is too small and we are in relax camera mode, keep viewangles
		newAngle = v_lastAngles;
	}
	else if ((d < v_cameraRelaxAngle) && (v_cameraMode == CAM_MODE_FOCUS))
	{
		// we catched up with our target, relax again
		v_cameraMode = CAM_MODE_RELAX;
	}
	else
	{
		// target move too far away, focus camera again
		v_cameraMode = CAM_MODE_FOCUS;
	}

	// and smooth view, if not a scene cut
	if (v_resetCamera || (v_cameraMode == CAM_MODE_RELAX))
	{
		angle = newAngle;
	}
	else
	{
		V_SmoothInterpolateAngles(v_lastAngles, newAngle, angle, 180.0f);
	}

	V_GetChaseOrigin(newAngle, newOrigin, distance, origin);

	// move position up, if very close at target
	if (v_lastDistance < 64.0f)
		origin[2] += 16.0f * (1.0f - (v_lastDistance / 64.0f));

	// calculate angle to second target
	VectorSubtract(ent2->origin, origin, tempVec);
	VectorAngles(tempVec, tempVec);
	tempVec[0] = -tempVec[0];
}

void V_GetDirectedChasePosition(cl_entity_t* ent1, cl_entity_t* ent2, float* angle, float* origin)
{

	if (v_resetCamera)
	{
		v_lastDistance = 4096.0f;
		// v_cameraMode = CAM_MODE_FOCUS;
	}

	if ((ent2 == (cl_entity_t*)0xFFFFFFFF) || (0 != ent1->player && (ent1->curstate.solid == SOLID_NOT)))
	{
		// we have no second target or player just died
		V_GetSingleTargetCam(ent1, angle, origin);
	}
	else if (ent2)
	{
		// keep both target in view
		V_GetDoubleTargetsCam(ent1, ent2, angle, origin);
	}
	else
	{
		// second target disappeard somehow (dead)

		// keep last good viewangle
		Vector newOrigin;

		int flags = gHUD.m_Spectator.m_iObserverFlags;

		float dfactor = (flags & DRC_FLAG_DRAMATIC) != 0 ? -1.0f : 1.0f;

		float distance = 112.0f + (16.0f * dfactor); // get close if dramatic;

		// go away in final scenes or if player just died
		if ((flags & DRC_FLAG_FINAL) != 0)
			distance *= 2.0f;

		// let v_lastDistance float smoothly away
		v_lastDistance += v_frametime * 32.0f; // move unit per seconds back

		if (distance > v_lastDistance)
			distance = v_lastDistance;

		newOrigin = ent1->origin;

		if (0 != ent1->player)
			newOrigin[2] += 17; // head level of living player
		else
			newOrigin[2] += 8; // object, tricky, must be above bomb in CS

		V_GetChaseOrigin(angle, newOrigin, distance, origin);
	}

	v_lastAngles = angle;
}

void V_GetChasePos(int target, float* cl_angles, Vector& origin, Vector& angles)
{
	cl_entity_t* ent = NULL;

	if (0 != target)
	{
		ent = gEngfuncs.GetEntityByIndex(target);
	}

	if (!ent)
	{
		// just copy a save in-map position
		angles = vJumpAngles;
		origin = vJumpOrigin;
		return;
	}



	if (0 != gHUD.m_Spectator.m_autoDirector->value)
	{
		if (0 != gHUD.GetObserverTarget2())
			V_GetDirectedChasePosition(ent, gEngfuncs.GetEntityByIndex(gHUD.GetObserverTarget2()),
				angles, origin);
		else
			V_GetDirectedChasePosition(ent, (cl_entity_t*)0xFFFFFFFF,
				angles, origin);
	}
	else
	{
		if (cl_angles == nullptr) // no mouse angles given, use entity angles ( locked mode )
		{
			angles = ent->angles;
			angles[0] *= -1;
		}
		else
		{
			angles = cl_angles;
		}


		origin = ent->origin;

		VectorAdd(origin, VEC_VIEW, origin); // some offset

		V_GetChaseOrigin(angles, origin, cl_chasedist->value, origin);
	}

	v_resetCamera = false;
}

void V_ResetChaseCam()
{
	v_resetCamera = true;
}


void V_GetInEyePos(int target, Vector& origin, Vector& angles)
{
	if (0 == target)
	{
		// just copy a save in-map position
		angles = vJumpAngles;
		origin = vJumpOrigin;
		return;
	}


	cl_entity_t* ent = gEngfuncs.GetEntityByIndex(target);

	if (!ent)
		return;

	origin = ent->origin;
	angles = ent->angles;

	angles[PITCH] *= -3.0f; // see CL_ProcessEntityUpdate()

	if (ent->curstate.solid == SOLID_NOT)
	{
		angles[ROLL] = 80; // dead view angle
		VectorAdd(origin, VEC_DEAD_VIEW, origin);
	}
	else if (ent->curstate.usehull == 1)
	{
		VectorAdd(origin, VEC_DUCK_VIEW, origin);
	}
	else
		// exacty eye position can't be caluculated since it depends on
		// client values like cl_bobcycle, this offset matches the default values
		VectorAdd(origin, VEC_VIEW, origin);
}

void V_GetMapFreePosition(float* cl_angles, float* origin, float* angles)
{
	Vector forward;
	Vector zScaledTarget;

	angles = cl_angles;

	// modify angles since we don't wanna see map's bottom
	angles[0] = 51.25f + 38.75f * (angles[0] / 90.0f);

	zScaledTarget[0] = gHUD.m_Spectator.m_mapOrigin[0];
	zScaledTarget[1] = gHUD.m_Spectator.m_mapOrigin[1];
	zScaledTarget[2] = gHUD.m_Spectator.m_mapOrigin[2] * ((90.0f - angles[0]) / 90.0f);


	AngleVectors(angles, forward, NULL, NULL);

	VectorNormalize(forward);

	VectorMA(zScaledTarget, -(4096.0f / gHUD.m_Spectator.m_mapZoom), forward, origin);
}

void V_GetMapChasePosition(int target, Vector& cl_angles, Vector& origin, Vector& angles)
{
	Vector forward;

	if (0 != target)
	{
		cl_entity_t* ent = gEngfuncs.GetEntityByIndex(target);

		if (0 != gHUD.m_Spectator.m_autoDirector->value)
		{
			// this is done to get the angles made by director mode
			V_GetChasePos(target, cl_angles, origin, angles);
			origin = ent->origin;

			// keep fix chase angle horizontal
			angles[0] = 45.0f;
		}
		else
		{
			angles = cl_angles;
			origin = ent->origin;

			// modify angles since we don't wanna see map's bottom
			angles[0] = 51.25f + 38.75f * (angles[0] / 90.0f);
		}
	}
	else
	{
		// keep out roaming position, but modify angles
		angles = cl_angles;
		angles[0] = 51.25f + 38.75f * (angles[0] / 90.0f);
	}

	origin[2] *= ((90.0f - angles[0]) / 90.0f);
	angles[2] = 0.0f; // don't roll angle (if chased player is dead)

	AngleVectors(angles, forward, NULL, NULL);

	VectorNormalize(forward);

	VectorMA(origin, -1536, forward, origin);
}


void V_CalcSpectatorRefdef(ref_params_t* pparams)
{
	static Vector velocity(0.0f, 0.0f, 0.0f);

	static int lastWeaponModelIndex = 0;
	static int lastViewModelIndex = 0;

	cl_entity_t* ent = gEngfuncs.GetEntityByIndex(gHUD.GetObserverTarget());

	pparams->onlyClientDraw = 0;

	// refresh position
	v_sim_org = pparams->simorg;

	// get old values
	v_cl_angles = pparams->cl_viewangles;
	v_angles = pparams->viewangles;
	v_origin = pparams->vieworg;

	v_frametime = pparams->frametime;

	if (pparams->nextView == 0)
	{
		// first renderer cycle, full screen

		switch (gHUD.GetObserverMode())
		{
		case OBS_CHASE_FREE:
			V_GetChasePos(gHUD.GetObserverTarget(), v_cl_angles, v_origin, v_angles);
			break;

		case OBS_ROAMING:
			v_angles = v_cl_angles;
			v_origin = v_sim_org;

			// override values if director is active
			gHUD.m_Spectator.GetDirectorCamera(v_origin, v_angles);
			break;

		case OBS_MAP_FREE:
			pparams->onlyClientDraw = 1;
			V_GetMapFreePosition(v_cl_angles, v_origin, v_angles);
			break;

		case OBS_MAP_CHASE:
			pparams->onlyClientDraw = 1;
			V_GetMapChasePosition(gHUD.GetObserverTarget(), v_cl_angles, v_origin, v_angles);
			break;

		case OBS_DEATHCAM:
#if 1
			V_GetChasePos(gHUD.GetObserverTarget(), v_cl_angles, v_origin, v_angles);
#else
			V_GetDeathCam(gHUD.GetObserverTarget(), gHUD.GetObserverTarget2(), v_cl_angles, v_origin, v_angles);
#endif
			break;
		}

		if (0 != gHUD.m_Spectator.m_pip->value)
			pparams->nextView = 1; // force a second renderer view

		gHUD.m_Spectator.m_iDrawCycle = 0;
	}
	else
	{
		// second renderer cycle, inset window

		// set inset parameters
		pparams->viewport[0] = XRES(gHUD.m_Spectator.m_OverviewData.insetWindowX); // change viewport to inset window
		pparams->viewport[1] = YRES(gHUD.m_Spectator.m_OverviewData.insetWindowY);
		pparams->viewport[2] = XRES(gHUD.m_Spectator.m_OverviewData.insetWindowWidth);
		pparams->viewport[3] = YRES(gHUD.m_Spectator.m_OverviewData.insetWindowHeight);
		pparams->nextView = 0; // on further view

		// override some settings in certain modes
		switch ((int)gHUD.m_Spectator.m_pip->value)
		{
		case INSET_CHASE_FREE:
			V_GetChasePos(gHUD.GetObserverTarget(), v_cl_angles, v_origin, v_angles);
			break;

		case INSET_MAP_FREE:
			pparams->onlyClientDraw = 1;
			V_GetMapFreePosition(v_cl_angles, v_origin, v_angles);
			break;

		case INSET_MAP_CHASE:
			pparams->onlyClientDraw = 1;

			if (gHUD.GetObserverMode() == OBS_ROAMING)
				V_GetMapChasePosition(0, v_cl_angles, v_origin, v_angles);
			else
				V_GetMapChasePosition(gHUD.GetObserverTarget(), v_cl_angles, v_origin, v_angles);

			break;
		}

		gHUD.m_Spectator.m_iDrawCycle = 1;
	}

	// write back new values into pparams
	pparams->cl_viewangles = v_cl_angles;
	pparams->viewangles = v_angles;
	pparams->vieworg = v_origin;
}
