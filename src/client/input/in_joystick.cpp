//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <algorithm>

#include "hud.h"
#include "cl_util.h"
#include "camera.h"
#include "kbutton.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"
#include "camera.h"
#include "in_defs.h"
#include "keydefs.h"
#include "view.h"
#include "Exports.h"

#include "PlatformHeaders.h"

#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_gamecontroller.h>


enum
{
	JOY_ABSOLUTE_AXIS = 0x00000000, // control like a joystick
	JOY_RELATIVE_AXIS = 0x00000010, // control like a mouse, spinner, trackball
};

enum
{
	JOY_AXIS_X = 0,
	JOY_AXIS_Y,
	JOY_AXIS_Z,
	JOY_AXIS_R,
	JOY_AXIS_U,
	JOY_AXIS_V,
	JOY_MAX_AXES
};

enum _ControlList
{
	AxisNada = 0,
	AxisForward,
	AxisLook,
	AxisSide,
	AxisTurn
};

std::uint32_t dwAxisMap[JOY_MAX_AXES];
std::uint32_t dwControlMap[JOY_MAX_AXES];
int pdwRawValue[JOY_MAX_AXES];
std::uint32_t joy_oldbuttonstate, joy_oldpovstate;

int joy_id;
std::uint32_t joy_numbuttons;

SDL_GameController* s_pJoystick = NULL;

// none of these cvars are saved over a session
// this means that advanced controller configuration needs to be executed
// each time.  this avoids any problems with getting back to a default usage
// or when changing from one controller to another.  this way at least something
// works.
cvar_t* in_joystick;
cvar_t* joy_name;
cvar_t* joy_advanced;
cvar_t* joy_advaxisx;
cvar_t* joy_advaxisy;
cvar_t* joy_advaxisz;
cvar_t* joy_advaxisr;
cvar_t* joy_advaxisu;
cvar_t* joy_advaxisv;
cvar_t* joy_forwardthreshold;
cvar_t* joy_sidethreshold;
cvar_t* joy_pitchthreshold;
cvar_t* joy_yawthreshold;
cvar_t* joy_forwardsensitivity;
cvar_t* joy_sidesensitivity;
cvar_t* joy_pitchsensitivity;
cvar_t* joy_yawsensitivity;
cvar_t* joy_wwhack1;
cvar_t* joy_wwhack2;

extern cvar_t* cl_pitchdown;
extern cvar_t* cl_pitchup;
extern cvar_t* cl_yawspeed;
extern cvar_t* cl_pitchspeed;

static bool joy_avail, joy_advancedinit, joy_haspov;


static void IN_StartupJoystick()
{
	// abort startup if user requests no joystick
	if (0 != gEngfuncs.CheckParm("-nojoy", NULL))
		return;

	// assume no joystick
	joy_avail = false;

	int nJoysticks = SDL_NumJoysticks();
	if (nJoysticks > 0)
	{
		for (int i = 0; i < nJoysticks; i++)
		{
			if (SDL_FALSE != SDL_IsGameController(i))
			{
				s_pJoystick = SDL_GameControllerOpen(i);
				if (s_pJoystick)
				{
					//save the joystick's number of buttons and POV status
					joy_numbuttons = SDL_CONTROLLER_BUTTON_MAX;
					joy_haspov = false;

					// old button and POV states default to no buttons pressed
					joy_oldbuttonstate = joy_oldpovstate = 0;

					// mark the joystick as available and advanced initialization not completed
					// this is needed as cvars are not available during initialization
					gEngfuncs.Con_Printf("joystick found\n\n", SDL_GameControllerName(s_pJoystick));
					joy_avail = true;
					joy_advancedinit = false;
					break;
				}
			}
		}
	}
	else
	{
		gEngfuncs.Con_DPrintf("joystick not found -- driver not present\n\n");
	}
}


static int RawValuePointer(int axis)
{
	switch (axis)
	{
	default:
	case JOY_AXIS_X:
		return SDL_GameControllerGetAxis(s_pJoystick, SDL_CONTROLLER_AXIS_LEFTX);
	case JOY_AXIS_Y:
		return SDL_GameControllerGetAxis(s_pJoystick, SDL_CONTROLLER_AXIS_LEFTY);
	case JOY_AXIS_Z:
		return SDL_GameControllerGetAxis(s_pJoystick, SDL_CONTROLLER_AXIS_RIGHTX);
	case JOY_AXIS_R:
		return SDL_GameControllerGetAxis(s_pJoystick, SDL_CONTROLLER_AXIS_RIGHTY);
	}
}


static void Joy_AdvancedUpdate_f()
{
	int i;
	std::uint32_t dwTemp;

	// initialize all the maps
	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		dwAxisMap[i] = AxisNada;
		dwControlMap[i] = JOY_ABSOLUTE_AXIS;
		pdwRawValue[i] = RawValuePointer(i);
	}

	if (joy_advanced->value == 0.0)
	{
		// default joystick initialization
		// 2 axes only with joystick control
		dwAxisMap[JOY_AXIS_X] = AxisTurn;
		// dwControlMap[JOY_AXIS_X] = JOY_ABSOLUTE_AXIS;
		dwAxisMap[JOY_AXIS_Y] = AxisForward;
		// dwControlMap[JOY_AXIS_Y] = JOY_ABSOLUTE_AXIS;
	}
	else
	{
		if (strcmp(joy_name->string, "joystick") != 0)
		{
			// notify user of advanced controller
			gEngfuncs.Con_Printf("\n%s configured\n\n", joy_name->string);
		}

		// advanced initialization here
		// data supplied by user via joy_axisn cvars
		dwTemp = (std::uint32_t)joy_advaxisx->value;
		dwAxisMap[JOY_AXIS_X] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_X] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (std::uint32_t)joy_advaxisy->value;
		dwAxisMap[JOY_AXIS_Y] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Y] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (std::uint32_t)joy_advaxisz->value;
		dwAxisMap[JOY_AXIS_Z] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Z] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (std::uint32_t)joy_advaxisr->value;
		dwAxisMap[JOY_AXIS_R] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_R] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (std::uint32_t)joy_advaxisu->value;
		dwAxisMap[JOY_AXIS_U] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_U] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (std::uint32_t)joy_advaxisv->value;
		dwAxisMap[JOY_AXIS_V] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_V] = dwTemp & JOY_RELATIVE_AXIS;
	}
}


void Joy_Commands()
{
	int i, key_index;

	if (!joy_avail)
	{
		return;
	}

	std::uint32_t buttonstate, povstate;

	// loop through the joystick buttons
	// key a joystick event or auxillary event for higher number buttons for each state change
	buttonstate = 0;
	for (i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++)
	{
		if (0 != SDL_GameControllerGetButton(s_pJoystick, (SDL_GameControllerButton)i))
		{
			buttonstate |= 1 << i;
		}
	}

	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		pdwRawValue[i] = RawValuePointer(i);
	}

	for (i = 0; i < (int)joy_numbuttons; i++)
	{
		if ((buttonstate & (1 << i)) != 0 && (joy_oldbuttonstate & (1 << i)) == 0)
		{
			key_index = (i < 4) ? K_JOY1 : K_AUX1;
			gEngfuncs.Key_Event(key_index + i, 1);
		}

		if ((buttonstate & (1 << i)) == 0 && (joy_oldbuttonstate & (1 << i)) != 0)
		{
			key_index = (i < 4) ? K_JOY1 : K_AUX1;
			gEngfuncs.Key_Event(key_index + i, 0);
		}
	}
	joy_oldbuttonstate = buttonstate;

	if (joy_haspov)
	{
		// convert POV information into 4 bits of state information
		// this avoids any potential problems related to moving from one
		// direction to another without going through the center position
		povstate = 0;
		// determine which bits have changed and key an auxillary event for each change
		for (i = 0; i < 4; i++)
		{
			if ((povstate & (1 << i)) != 0 && (joy_oldpovstate & (1 << i)) == 0)
			{
				gEngfuncs.Key_Event(K_AUX29 + i, 1);
			}

			if ((povstate & (1 << i)) == 0 && (joy_oldpovstate & (1 << i)) != 0)
			{
				gEngfuncs.Key_Event(K_AUX29 + i, 0);
			}
		}
		joy_oldpovstate = povstate;
	}
}


void Joy_Move(float frametime, usercmd_t* cmd)
{
	float lookspeed, aspeed;
	float fAxisValue, fTemp;
	int i;
	Vector viewangles;

	gEngfuncs.GetViewAngles((float*)viewangles);

	// complete initialization if first time in
	// this is needed as cvars are not available at initialization time
	if (!joy_advancedinit)
	{
		Joy_AdvancedUpdate_f();
		joy_advancedinit = true;
	}

	// verify joystick is available and that the user wants to use it
	if (!joy_avail || 0 == in_joystick->value)
	{
		return;
	}

	// collect the joystick data
	SDL_JoystickUpdate();

	if ((in_speed.state & 1) != 0)
		lookspeed = 0.3;
	else
		lookspeed = 1;

	aspeed = lookspeed * frametime;

	// loop through the axes
	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		// get the floating point zero-centered, potentially-inverted data for the current axis
		fAxisValue = (float)pdwRawValue[i];

		if (joy_wwhack2->value != 0.0)
		{
			if (dwAxisMap[i] == AxisTurn)
			{
				// this is a special formula for the Logitech WingMan Warrior
				// y=ax^b; where a = 300 and b = 1.3
				// also x values are in increments of 800 (so this is factored out)
				// then bounds check result to level out excessively high spin rates
				fTemp = 300.0 * pow(fabs(fAxisValue) / 800.0, 1.3);
				if (fTemp > 14000.0)
					fTemp = 14000.0;
				// restore direction information
				fAxisValue = (fAxisValue > 0.0) ? fTemp : -fTemp;
			}
		}

		// convert range from -32768..32767 to -1..1
		fAxisValue /= 32768.0;

		switch (dwAxisMap[i])
		{
		case AxisForward:
			if (fabs(fAxisValue) > joy_forwardthreshold->value)
			{
				*reinterpret_cast<int*>(&cmd->forwardmove) += (fAxisValue * joy_forwardsensitivity->value) * 100;
			}
			break;

		case AxisSide:
			if (fabs(fAxisValue) > joy_sidethreshold->value)
			{
				*reinterpret_cast<int*>(&cmd->sidemove) += (fAxisValue * joy_sidesensitivity->value) * 100;
			}
			break;

		case AxisTurn:
            if (fabs(fAxisValue) > joy_yawthreshold->value)
            {
                if (dwControlMap[i] == JOY_ABSOLUTE_AXIS)
                {
                    viewangles.y += (fAxisValue * joy_yawsensitivity->value) * aspeed * cl_yawspeed->value;
                }
                else
                {
                    viewangles.y += (fAxisValue * joy_yawsensitivity->value) * lookspeed * 180.0;
                }
            }
			break;

		case AxisLook:
            if (fabs(fAxisValue) > joy_pitchthreshold->value)
            {
                if (dwControlMap[i] == JOY_ABSOLUTE_AXIS)
                {
                    viewangles.x += (fAxisValue * joy_pitchsensitivity->value) * aspeed * cl_pitchspeed->value;
                }
                else
                {
                    viewangles.x += (fAxisValue * joy_pitchsensitivity->value) * lookspeed * 180.0;
                }
            }
			break;

		default:
			break;
		}
	}

	viewangles.x = std::clamp(viewangles.x, -cl_pitchup->value, cl_pitchdown->value);

	gEngfuncs.SetViewAngles((float*)viewangles);
}


void Joy_Init()
{
	in_joystick = gEngfuncs.pfnRegisterVariable("joystick", "0", FCVAR_ARCHIVE);
	joy_name = gEngfuncs.pfnRegisterVariable("joyname", "joystick", 0);
	joy_advanced = gEngfuncs.pfnRegisterVariable("joyadvanced", "0", 0);
	joy_advaxisx = gEngfuncs.pfnRegisterVariable("joyadvaxisx", "0", 0);
	joy_advaxisy = gEngfuncs.pfnRegisterVariable("joyadvaxisy", "0", 0);
	joy_advaxisz = gEngfuncs.pfnRegisterVariable("joyadvaxisz", "0", 0);
	joy_advaxisr = gEngfuncs.pfnRegisterVariable("joyadvaxisr", "0", 0);
	joy_advaxisu = gEngfuncs.pfnRegisterVariable("joyadvaxisu", "0", 0);
	joy_advaxisv = gEngfuncs.pfnRegisterVariable("joyadvaxisv", "0", 0);
	joy_forwardthreshold = gEngfuncs.pfnRegisterVariable("joyforwardthreshold", "0.15", 0);
	joy_sidethreshold = gEngfuncs.pfnRegisterVariable("joysidethreshold", "0.15", 0);
	joy_pitchthreshold = gEngfuncs.pfnRegisterVariable("joypitchthreshold", "0.15", 0);
	joy_yawthreshold = gEngfuncs.pfnRegisterVariable("joyyawthreshold", "0.15", 0);
	joy_forwardsensitivity = gEngfuncs.pfnRegisterVariable("joyforwardsensitivity", "-1.0", 0);
	joy_sidesensitivity = gEngfuncs.pfnRegisterVariable("joysidesensitivity", "-1.0", 0);
	joy_pitchsensitivity = gEngfuncs.pfnRegisterVariable("joypitchsensitivity", "1.0", 0);
	joy_yawsensitivity = gEngfuncs.pfnRegisterVariable("joyyawsensitivity", "-1.0", 0);
	joy_wwhack1 = gEngfuncs.pfnRegisterVariable("joywwhack1", "0.0", 0);
	joy_wwhack2 = gEngfuncs.pfnRegisterVariable("joywwhack2", "0.0", 0);

	gEngfuncs.pfnAddCommand("joyadvancedupdate", Joy_AdvancedUpdate_f);
}

