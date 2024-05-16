//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "cl_util.h"
#include "camera.h"
#include "kbutton.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"
#include "camera.h"
#include "in_defs.h"
#include "Exports.h"
#include "com_weapons.h"

#include "SDL2/SDL_mouse.h"

float CL_KeyState(kbutton_t* key);

//-------------------------------------------------- Constants

#define CAM_DIST_DELTA 1.0
#define CAM_ANGLE_DELTA 2.5
#define CAM_ANGLE_SPEED 2.5
#define CAM_MIN_DIST 30.0
#define CAM_ANGLE_MOVE .5
#define MAX_ANGLE_DIFF 10.0
#define PITCH_MAX 90.0
#define PITCH_MIN 0
#define YAW_MAX 135.0
#define YAW_MIN -135.0

enum ECAM_Command
{
	CAM_COMMAND_NONE = 0,
	CAM_COMMAND_TOTHIRDPERSON = 1,
	CAM_COMMAND_TOFIRSTPERSON = 2
};

//-------------------------------------------------- Global Variables

cvar_t* cam_command;
cvar_t* cam_snapto;
cvar_t* cam_idealyaw;
cvar_t* cam_idealpitch;
cvar_t* cam_idealdist;
cvar_t* cam_contain;

cvar_t* c_maxpitch;
cvar_t* c_minpitch;
cvar_t* c_maxyaw;
cvar_t* c_minyaw;
cvar_t* c_maxdistance;
cvar_t* c_mindistance;

// pitch, yaw, dist
Vector cam_ofs;


// In third person
bool cam_thirdperson;
bool cam_mousemove; //true if we are moving the cam with the mouse, False if not
bool iMouseInUse = false;
bool cam_distancemove;
extern int mouse_x, mouse_y;		  //used to determine what the current x and y values are
int cam_old_mouse_x, cam_old_mouse_y; //holds the last ticks mouse movement
Point cam_mouse;
//-------------------------------------------------- Local Variables

static kbutton_t cam_pitchup, cam_pitchdown, cam_yawleft, cam_yawright;
static kbutton_t cam_in, cam_out, cam_move;

//-------------------------------------------------- Prototypes

void CAM_ToThirdPerson();
void CAM_ToFirstPerson();
void CAM_StartDistance();
void CAM_EndDistance();

void SDL_GetCursorPos(Point* p)
{
	client::GetMousePosition(&p->x, &p->y);
	//	SDL_GetMouseState( &p->x, &p->y );
}

void SDL_SetCursorPos(const int x, const int y)
{
}

//-------------------------------------------------- Local Functions

float MoveToward(float cur, float goal, float maxspeed)
{
	if (cur != goal)
	{
		if (fabs(cur - goal) > 180.0)
		{
			if (cur < goal)
				cur += 360.0;
			else
				cur -= 360.0;
		}

		if (cur < goal)
		{
			if (cur < goal - 1.0)
				cur += (goal - cur) / 4.0;
			else
				cur = goal;
		}
		else
		{
			if (cur > goal + 1.0)
				cur -= (cur - goal) / 4.0;
			else
				cur = goal;
		}
	}


	// bring cur back into range
	if (cur < 0)
		cur += 360.0;
	else if (cur >= 360)
		cur -= 360;

	return cur;
}


//-------------------------------------------------- Gobal Functions

typedef struct
{
	Vector boxmins, boxmaxs; // enclose the test object along entire move
	float *mins, *maxs;		 // size of the moving object
	Vector mins2, maxs2;	 // size when clipping against mosnters
	float *start, *end;
	trace_t trace;
	int type;
	Entity* passedict;
	qboolean monsterclip;
} moveclip_t;

extern trace_t SV_ClipMoveToEntity(Entity* ent, Vector start, Vector mins, Vector maxs, Vector end);

void CAM_Think()
{
	Vector origin;
	Vector ext, pnt, camForward, camRight, camUp;
	moveclip_t clip;
	float dist;
	Vector camAngles;
	float flSensitivity;
#ifdef LATER
	int i;
#endif
	Vector viewangles;

	switch ((int)cam_command->value)
	{
	case CAM_COMMAND_TOTHIRDPERSON:
		CAM_ToThirdPerson();
		break;

	case CAM_COMMAND_TOFIRSTPERSON:
		CAM_ToFirstPerson();
		break;

	case CAM_COMMAND_NONE:
	default:
		break;
	}

	if (!cam_thirdperson)
		return;

#ifdef LATER
	if (cam_contain->value)
	{
		client::GetClientOrigin(origin);
		ext[0] = ext[1] = ext[2] = 0.0;
	}
#endif

	camAngles[PITCH] = cam_idealpitch->value;
	camAngles[YAW] = cam_idealyaw->value;
	dist = cam_idealdist->value;
	//
	//movement of the camera with the mouse
	//
	if (cam_mousemove)
	{
		//get windows cursor position
		SDL_GetCursorPos(&cam_mouse);
		//check for X delta values and adjust accordingly
		//eventually adjust YAW based on amount of movement
		//don't do any movement of the cam using YAW/PITCH if we are zooming in/out the camera
		if (!cam_distancemove)
		{

			//keep the camera within certain limits around the player (ie avoid certain bad viewing angles)
			if (cam_mouse.x > client::GetWindowCenterX())
			{
				//if ((camAngles[YAW]>=225.0)||(camAngles[YAW]<135.0))
				if (camAngles[YAW] < c_maxyaw->value)
				{
					camAngles[YAW] += (CAM_ANGLE_MOVE) * ((cam_mouse.x - client::GetWindowCenterX()) / 2);
				}
				if (camAngles[YAW] > c_maxyaw->value)
				{

					camAngles[YAW] = c_maxyaw->value;
				}
			}
			else if (cam_mouse.x < client::GetWindowCenterX())
			{
				//if ((camAngles[YAW]<=135.0)||(camAngles[YAW]>225.0))
				if (camAngles[YAW] > c_minyaw->value)
				{
					camAngles[YAW] -= (CAM_ANGLE_MOVE) * ((client::GetWindowCenterX() - cam_mouse.x) / 2);
				}
				if (camAngles[YAW] < c_minyaw->value)
				{
					camAngles[YAW] = c_minyaw->value;
				}
			}

			//check for y delta values and adjust accordingly
			//eventually adjust PITCH based on amount of movement
			//also make sure camera is within bounds
			if (cam_mouse.y > client::GetWindowCenterY())
			{
				if (camAngles[PITCH] < c_maxpitch->value)
				{
					camAngles[PITCH] += (CAM_ANGLE_MOVE) * ((cam_mouse.y - client::GetWindowCenterY()) / 2);
				}
				if (camAngles[PITCH] > c_maxpitch->value)
				{
					camAngles[PITCH] = c_maxpitch->value;
				}
			}
			else if (cam_mouse.y < client::GetWindowCenterY())
			{
				if (camAngles[PITCH] > c_minpitch->value)
				{
					camAngles[PITCH] -= (CAM_ANGLE_MOVE) * ((client::GetWindowCenterY() - cam_mouse.y) / 2);
				}
				if (camAngles[PITCH] < c_minpitch->value)
				{
					camAngles[PITCH] = c_minpitch->value;
				}
			}

			//set old mouse coordinates to current mouse coordinates
			//since we are done with the mouse

			if ((flSensitivity = gHUD.GetSensitivity()) != 0)
			{
				cam_old_mouse_x = cam_mouse.x * flSensitivity;
				cam_old_mouse_y = cam_mouse.y * flSensitivity;
			}
			else
			{
				cam_old_mouse_x = cam_mouse.x;
				cam_old_mouse_y = cam_mouse.y;
			}
			SDL_SetCursorPos(client::GetWindowCenterX(), client::GetWindowCenterY());
		}
	}

	//Nathan code here
	if (0 != CL_KeyState(&cam_pitchup))
		camAngles[PITCH] += CAM_ANGLE_DELTA;
	else if (0 != CL_KeyState(&cam_pitchdown))
		camAngles[PITCH] -= CAM_ANGLE_DELTA;

	if (0 != CL_KeyState(&cam_yawleft))
		camAngles[YAW] -= CAM_ANGLE_DELTA;
	else if (0 != CL_KeyState(&cam_yawright))
		camAngles[YAW] += CAM_ANGLE_DELTA;

	if (0 != CL_KeyState(&cam_in))
	{
		dist -= CAM_DIST_DELTA;
		if (dist < CAM_MIN_DIST)
		{
			// If we go back into first person, reset the angle
			camAngles[PITCH] = 0;
			camAngles[YAW] = 0;
			dist = CAM_MIN_DIST;
		}
	}
	else if (0 != CL_KeyState(&cam_out))
		dist += CAM_DIST_DELTA;

	if (cam_distancemove)
	{
		if (cam_mouse.y > client::GetWindowCenterY())
		{
			if (dist < c_maxdistance->value)
			{
				dist += CAM_DIST_DELTA * ((cam_mouse.y - client::GetWindowCenterY()) / 2);
			}
			if (dist > c_maxdistance->value)
			{
				dist = c_maxdistance->value;
			}
		}
		else if (cam_mouse.y < client::GetWindowCenterY())
		{
			if (dist > c_mindistance->value)
			{
				dist -= (CAM_DIST_DELTA) * ((client::GetWindowCenterY() - cam_mouse.y) / 2);
			}
			if (dist < c_mindistance->value)
			{
				dist = c_mindistance->value;
			}
		}
		//set old mouse coordinates to current mouse coordinates
		//since we are done with the mouse
		cam_old_mouse_x = cam_mouse.x * gHUD.GetSensitivity();
		cam_old_mouse_y = cam_mouse.y * gHUD.GetSensitivity();
		SDL_SetCursorPos(client::GetWindowCenterX(), client::GetWindowCenterY());
	}
#ifdef LATER
	if (cam_contain->value)
	{
		// check new ideal
		pnt = origin;
		AngleVectors(camAngles, &camForward, &camRight, &camUp);
		for (i = 0; i < 3; i++)
			pnt[i] += -dist * camForward[i];

		// check line from r_refdef.vieworg to pnt
		memset(&clip, 0, sizeof(moveclip_t));
		clip.trace = SV_ClipMoveToEntity(sv.edicts, r_refdef.vieworg, ext, ext, pnt);
		if (clip.trace.fraction == 1.0)
		{
			// update ideal
			cam_idealpitch->value = camAngles[PITCH];
			cam_idealyaw->value = camAngles[YAW];
			cam_idealdist->value = dist;
		}
	}
	else
#endif
	{
		// update ideal
		cam_idealpitch->value = camAngles[PITCH];
		cam_idealyaw->value = camAngles[YAW];
		cam_idealdist->value = dist;
	}

	// Move towards ideal
	camAngles = cam_ofs;

	client::GetViewAngles(viewangles);

	if (0 != cam_snapto->value)
	{
		camAngles[YAW] = cam_idealyaw->value + viewangles[YAW];
		camAngles[PITCH] = cam_idealpitch->value + viewangles[PITCH];
		camAngles[2] = cam_idealdist->value;
	}
	else
	{
		if (camAngles[YAW] - viewangles[YAW] != cam_idealyaw->value)
			camAngles[YAW] = MoveToward(camAngles[YAW], cam_idealyaw->value + viewangles[YAW], CAM_ANGLE_SPEED);

		if (camAngles[PITCH] - viewangles[PITCH] != cam_idealpitch->value)
			camAngles[PITCH] = MoveToward(camAngles[PITCH], cam_idealpitch->value + viewangles[PITCH], CAM_ANGLE_SPEED);

		if (fabs(camAngles[2] - cam_idealdist->value) < 2.0)
			camAngles[2] = cam_idealdist->value;
		else
			camAngles[2] += (cam_idealdist->value - camAngles[2]) / 4.0;
	}
#ifdef LATER
	if (cam_contain->value)
	{
		// Test new position
		dist = camAngles[ROLL];
		camAngles[ROLL] = 0;

		pnt = origin;
		AngleVectors(camAngles, &camForward, &camRight, &camUp);
		for (i = 0; i < 3; i++)
			pnt[i] += -dist * camForward[i];

		// check line from r_refdef.vieworg to pnt
		memset(&clip, 0, sizeof(moveclip_t));
		ext[0] = ext[1] = ext[2] = 0.0;
		clip.trace = SV_ClipMoveToEntity(sv.edicts, r_refdef.vieworg, ext, ext, pnt);
		if (clip.trace.fraction != 1.0)
			return;
	}
#endif
	cam_ofs[0] = camAngles[0];
	cam_ofs[1] = camAngles[1];
	cam_ofs[2] = dist;
}

extern void KeyDown(kbutton_t* b); // HACK
extern void KeyUp(kbutton_t* b);   // HACK

void CAM_PitchUpDown() { KeyDown(&cam_pitchup); }
void CAM_PitchUpUp() { KeyUp(&cam_pitchup); }
void CAM_PitchDownDown() { KeyDown(&cam_pitchdown); }
void CAM_PitchDownUp() { KeyUp(&cam_pitchdown); }
void CAM_YawLeftDown() { KeyDown(&cam_yawleft); }
void CAM_YawLeftUp() { KeyUp(&cam_yawleft); }
void CAM_YawRightDown() { KeyDown(&cam_yawright); }
void CAM_YawRightUp() { KeyUp(&cam_yawright); }
void CAM_InDown() { KeyDown(&cam_in); }
void CAM_InUp() { KeyUp(&cam_in); }
void CAM_OutDown() { KeyDown(&cam_out); }
void CAM_OutUp() { KeyUp(&cam_out); }

void CAM_ToThirdPerson()
{
	Vector viewangles;

#ifdef NDEBUG
	if (util::IsDeathmatch())
	{
		// no thirdperson in multiplayer.
		return;
	}
#endif

	client::GetViewAngles(viewangles);

	if (!cam_thirdperson)
	{
		cam_thirdperson = true;

		cam_ofs[YAW] = viewangles[YAW];
		cam_ofs[PITCH] = viewangles[PITCH];
		cam_ofs[2] = CAM_MIN_DIST;
	}

	client::Cvar_SetValue("cam_command", 0);
}

void CAM_ToFirstPerson()
{
	cam_thirdperson = false;

	client::Cvar_SetValue("cam_command", 0);
}

void CAM_ToggleSnapto()
{
	cam_snapto->value = 0 != cam_snapto->value ? 0 : 1;
}

void CAM_Init()
{
	client::AddCommand("+campitchup", CAM_PitchUpDown);
	client::AddCommand("-campitchup", CAM_PitchUpUp);
	client::AddCommand("+campitchdown", CAM_PitchDownDown);
	client::AddCommand("-campitchdown", CAM_PitchDownUp);
	client::AddCommand("+camyawleft", CAM_YawLeftDown);
	client::AddCommand("-camyawleft", CAM_YawLeftUp);
	client::AddCommand("+camyawright", CAM_YawRightDown);
	client::AddCommand("-camyawright", CAM_YawRightUp);
	client::AddCommand("+camin", CAM_InDown);
	client::AddCommand("-camin", CAM_InUp);
	client::AddCommand("+camout", CAM_OutDown);
	client::AddCommand("-camout", CAM_OutUp);
	client::AddCommand("thirdperson", CAM_ToThirdPerson);
	client::AddCommand("firstperson", CAM_ToFirstPerson);
	client::AddCommand("+cammousemove", CAM_StartMouseMove);
	client::AddCommand("-cammousemove", CAM_EndMouseMove);
	client::AddCommand("+camdistance", CAM_StartDistance);
	client::AddCommand("-camdistance", CAM_EndDistance);
	client::AddCommand("snapto", CAM_ToggleSnapto);

	cam_command = client::RegisterVariable("cam_command", "0", 0);		  // tells camera to go to thirdperson
	cam_snapto = client::RegisterVariable("cam_snapto", "0", 0);		  // snap to thirdperson view
	cam_idealyaw = client::RegisterVariable("cam_idealyaw", "0", 0);	  // thirdperson yaw
	cam_idealpitch = client::RegisterVariable("cam_idealpitch", "0", 0); // thirperson pitch
	cam_idealdist = client::RegisterVariable("cam_idealdist", "64", 0);  // thirdperson distance
	cam_contain = client::RegisterVariable("cam_contain", "0", 0);		  // contain camera to world

	c_maxpitch = client::RegisterVariable("c_maxpitch", "90.0", 0);
	c_minpitch = client::RegisterVariable("c_minpitch", "0.0", 0);
	c_maxyaw = client::RegisterVariable("c_maxyaw", "135.0", 0);
	c_minyaw = client::RegisterVariable("c_minyaw", "-135.0", 0);
	c_maxdistance = client::RegisterVariable("c_maxdistance", "200.0", 0);
	c_mindistance = client::RegisterVariable("c_mindistance", "30.0", 0);
}

void CAM_ClearStates()
{
	Vector viewangles;

	client::GetViewAngles(viewangles);

	cam_pitchup.state = 0;
	cam_pitchdown.state = 0;
	cam_yawleft.state = 0;
	cam_yawright.state = 0;
	cam_in.state = 0;
	cam_out.state = 0;

	cam_thirdperson = false;
	cam_command->value = 0;
	cam_mousemove = false;

	cam_snapto->value = 0;
	cam_distancemove = false;

	cam_ofs[0] = 0.0;
	cam_ofs[1] = 0.0;
	cam_ofs[2] = CAM_MIN_DIST;

	cam_idealpitch->value = viewangles[PITCH];
	cam_idealyaw->value = viewangles[YAW];
	cam_idealdist->value = CAM_MIN_DIST;
}

void CAM_StartMouseMove()
{
	float flSensitivity;

	//only move the cam with mouse if we are in third person.
	if (cam_thirdperson)
	{
		//set appropriate flags and initialize the old mouse position
		//variables for mouse camera movement
		if (!cam_mousemove)
		{
			cam_mousemove = true;
			iMouseInUse = true;
			SDL_GetCursorPos(&cam_mouse);

			if ((flSensitivity = gHUD.GetSensitivity()) != 0)
			{
				cam_old_mouse_x = cam_mouse.x * flSensitivity;
				cam_old_mouse_y = cam_mouse.y * flSensitivity;
			}
			else
			{
				cam_old_mouse_x = cam_mouse.x;
				cam_old_mouse_y = cam_mouse.y;
			}
		}
	}
	//we are not in 3rd person view..therefore do not allow camera movement
	else
	{
		cam_mousemove = false;
		iMouseInUse = false;
	}
}

//the key has been released for camera movement
//tell the engine that mouse camera movement is off
void CAM_EndMouseMove()
{
	cam_mousemove = false;
	iMouseInUse = false;
}


//----------------------------------------------------------
//routines to start the process of moving the cam in or out
//using the mouse
//----------------------------------------------------------
void CAM_StartDistance()
{
	//only move the cam with mouse if we are in third person.
	if (cam_thirdperson)
	{
		//set appropriate flags and initialize the old mouse position
		//variables for mouse camera movement
		if (!cam_distancemove)
		{
			cam_distancemove = true;
			cam_mousemove = true;
			iMouseInUse = true;
			SDL_GetCursorPos(&cam_mouse);
			cam_old_mouse_x = cam_mouse.x * gHUD.GetSensitivity();
			cam_old_mouse_y = cam_mouse.y * gHUD.GetSensitivity();
		}
	}
	//we are not in 3rd person view..therefore do not allow camera movement
	else
	{
		cam_distancemove = false;
		cam_mousemove = false;
		iMouseInUse = false;
	}
}

//the key has been released for camera movement
//tell the engine that mouse camera movement is off
void CAM_EndDistance()
{
	cam_distancemove = false;
	cam_mousemove = false;
	iMouseInUse = false;
}

int CL_IsThirdPerson()
{
	return static_cast<int>(cam_thirdperson || (gHUD.IsObserver() && (gHUD.GetObserverTarget() == client::GetLocalPlayer()->index)));
}

void CL_CameraOffset(float* ofs)
{
	cam_ofs.CopyToArray(ofs);
}
