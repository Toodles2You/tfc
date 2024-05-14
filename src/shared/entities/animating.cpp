/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
#include "animation.h"
#include "saverestore.h"

#ifdef GAME_DLL

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CBaseAnimating)
	DEFINE_FIELD(CBaseAnimating, m_flFrameRate, FIELD_FLOAT),
	DEFINE_FIELD(CBaseAnimating, m_flGroundSpeed, FIELD_FLOAT),
	DEFINE_FIELD(CBaseAnimating, m_flLastEventCheck, FIELD_TIME),
	DEFINE_FIELD(CBaseAnimating, m_fSequenceFinished, FIELD_BOOLEAN),
	DEFINE_FIELD(CBaseAnimating, m_fSequenceLoops, FIELD_BOOLEAN),
END_SAVERESTORE(CBaseAnimating, CBaseDelay)
#endif

#endif


//=========================================================
// StudioFrameAdvance - advance the animation frame up to the current time
// if an flInterval is passed in, only advance animation that number of seconds
//=========================================================
float CBaseAnimating::StudioFrameAdvance(float flInterval)
{
	if (flInterval == 0.0)
	{
		flInterval = (gpGlobals->time - v.animtime);
		if (flInterval <= 0.001)
		{
			v.animtime = gpGlobals->time;
			return 0.0;
		}
	}
	if (0 == v.animtime)
		flInterval = 0.0;

	v.frame += flInterval * m_flFrameRate * v.framerate;
	v.animtime = gpGlobals->time;

	if (v.frame < 0.0 || v.frame >= 256.0)
	{
		if (m_fSequenceLoops)
			v.frame -= (int)(v.frame / 256.0) * 256.0;
		else
			v.frame = (v.frame < 0.0) ? 0 : 255;
		m_fSequenceFinished = true; // just in case it wasn't caught in GetEvents
        HandleSequenceFinished();
	}

	return flInterval;
}

//=========================================================
// LookupActivity
//=========================================================
int CBaseAnimating::LookupActivity(int activity)
{
	void* pmodel = engine::GetModelPtr(&v);

	return studio::LookupActivity(pmodel, activity);
}

//=========================================================
// LookupActivityHeaviest
//
// Get activity with highest 'weight'
//
//=========================================================
int CBaseAnimating::LookupActivityHeaviest(int activity)
{
	void* pmodel = engine::GetModelPtr(&v);

	return studio::LookupActivityHeaviest(pmodel, activity);
}

//=========================================================
//=========================================================
int CBaseAnimating::LookupSequence(const char* label)
{
	void* pmodel = engine::GetModelPtr(&v);

	return studio::LookupSequence(pmodel, label);
}


//=========================================================
//=========================================================
void CBaseAnimating::ResetSequenceInfo()
{
	void* pmodel = engine::GetModelPtr(&v);

	studio::GetSequenceInfo(pmodel, v.sequence, &m_flFrameRate, &m_flGroundSpeed);
	m_fSequenceLoops = ((GetSequenceFlags() & STUDIO_LOOPING) != 0);
	v.animtime = gpGlobals->time;
	v.framerate = 1.0;
	m_fSequenceFinished = false;
	m_flLastEventCheck = gpGlobals->time;
}



//=========================================================
//=========================================================
int CBaseAnimating::GetSequenceFlags()
{
	void* pmodel = engine::GetModelPtr(&v);

	return studio::GetSequenceFlags(pmodel, v.sequence);
}

//=========================================================
// DispatchAnimEvents
//=========================================================
void CBaseAnimating::DispatchAnimEvents(float flInterval)
{
	MonsterEvent_t event;

	void* pmodel = engine::GetModelPtr(&v);

	if (!pmodel)
	{
		engine::AlertMessage(at_aiconsole, "Gibbed monster is thinking!\n");
		return;
	}

	// FIXME: I have to do this or some events get missed, and this is probably causing the problem below
	flInterval = 0.1;

	// FIX: this still sometimes hits events twice
	float flStart = v.frame + (m_flLastEventCheck - v.animtime) * m_flFrameRate * v.framerate;
	float flEnd = v.frame + flInterval * m_flFrameRate * v.framerate;
	m_flLastEventCheck = v.animtime + flInterval;

	m_fSequenceFinished = false;
	if (flEnd >= 256 || flEnd <= 0.0)
    {
		m_fSequenceFinished = true;
        HandleSequenceFinished();
    }

	int index = 0;

	while ((index = studio::GetAnimationEvent(pmodel, v.sequence, &event, flStart, flEnd, index)) != 0)
	{
		HandleAnimEvent(&event);
	}
}


//=========================================================
//=========================================================
float CBaseAnimating::SetBoneController(int iController, float flValue)
{
	void* pmodel = engine::GetModelPtr(&v);

	return studio::SetController(pmodel, v.controller, iController, flValue);
}

//=========================================================
//=========================================================
void CBaseAnimating::InitBoneControllers()
{
	void* pmodel = engine::GetModelPtr(&v);

	studio::SetController(pmodel, v.controller, 0, 0.0);
	studio::SetController(pmodel, v.controller, 1, 0.0);
	studio::SetController(pmodel, v.controller, 2, 0.0);
	studio::SetController(pmodel, v.controller, 3, 0.0);
}

//=========================================================
//=========================================================
float CBaseAnimating::SetBlending(int iBlender, float flValue)
{
	void* pmodel = engine::GetModelPtr(&v);

	return studio::SetBlending(pmodel, v.sequence, v.blending, iBlender, flValue);
}

//=========================================================
//=========================================================
void CBaseAnimating::GetBonePosition(int iBone, Vector& origin, Vector& angles)
{
	engine::GetBonePosition(&v, iBone, origin, angles);
}

//=========================================================
//=========================================================
void CBaseAnimating::GetAttachment(int iAttachment, Vector& origin, Vector& angles)
{
	engine::GetAttachment(&v, iAttachment, origin, angles);
}

//=========================================================
//=========================================================
int CBaseAnimating::FindTransition(int iEndingSequence, int iGoalSequence, int* piDir)
{
	void* pmodel = engine::GetModelPtr(&v);

	if (piDir == nullptr)
	{
		int iDir;
		int sequence = studio::FindTransition(pmodel, iEndingSequence, iGoalSequence, &iDir);
		if (iDir != 1)
			return -1;
		else
			return sequence;
	}

	return studio::FindTransition(pmodel, iEndingSequence, iGoalSequence, piDir);
}

void CBaseAnimating::SetBodygroup(int iGroup, int iValue)
{
	studio::SetBodygroup(engine::GetModelPtr(&v), v.body, iGroup, iValue);
}

int CBaseAnimating::GetBodygroup(int iGroup)
{
	return studio::GetBodygroup(engine::GetModelPtr(&v), v.body, iGroup);
}


bool CBaseAnimating::ExtractBbox(int sequence, Vector& mins, Vector& maxs)
{
	return studio::ExtractBbox(engine::GetModelPtr(&v), sequence, mins, maxs);
}

//=========================================================
//=========================================================

void CBaseAnimating::SetSequenceBox()
{
	Vector mins, maxs;

	// Get sequence bbox
	if (ExtractBbox(v.sequence, mins, maxs))
	{
		// expand box for rotation
		// find min / max for rotations
		float yaw = v.angles.y * (M_PI / 180.0);

		Vector xvector, yvector;
		xvector.x = cos(yaw);
		xvector.y = sin(yaw);
		yvector.x = -sin(yaw);
		yvector.y = cos(yaw);
		Vector bounds[2];

		bounds[0] = mins;
		bounds[1] = maxs;

		Vector rmin(9999, 9999, 9999);
		Vector rmax(-9999, -9999, -9999);
		Vector base, transformed;

		for (int i = 0; i <= 1; i++)
		{
			base.x = bounds[i].x;
			for (int j = 0; j <= 1; j++)
			{
				base.y = bounds[j].y;
				for (int k = 0; k <= 1; k++)
				{
					base.z = bounds[k].z;

					// transform the point
					transformed.x = xvector.x * base.x + yvector.x * base.y;
					transformed.y = xvector.y * base.x + yvector.y * base.y;
					transformed.z = base.z;

					if (transformed.x < rmin.x)
						rmin.x = transformed.x;
					if (transformed.x > rmax.x)
						rmax.x = transformed.x;
					if (transformed.y < rmin.y)
						rmin.y = transformed.y;
					if (transformed.y > rmax.y)
						rmax.y = transformed.y;
					if (transformed.z < rmin.z)
						rmin.z = transformed.z;
					if (transformed.z > rmax.z)
						rmax.z = transformed.z;
				}
			}
		}
		rmin.z = 0;
		rmax.z = rmin.z + 1;
		SetSize(rmin, rmax);
	}
}
