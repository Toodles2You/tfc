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


enum { kMouseButtonCount = 5 };

// Set this to 1 to show mouse cursor.  Experimental
bool g_iVisibleMouse = false;

extern bool iMouseInUse;

extern cvar_t* cl_pitchdown;
extern cvar_t* cl_pitchup;
extern cvar_t* cl_yawspeed;
extern cvar_t* cl_pitchspeed;

static cvar_t* m_rawinput = nullptr;

static bool Mouse_UseRawInput()
{
	return m_rawinput->value != 0;
}

static SDL_bool mouseRelative = SDL_TRUE;

static void Mouse_SetRelative(bool enable)
{
	const SDL_bool value = enable ? SDL_TRUE : SDL_FALSE;

	SDL_SetRelativeMouseMode(value);
	mouseRelative = value;
}

static cvar_t* m_filter;
static cvar_t* m_pitch;
static cvar_t* m_yaw;
cvar_t* sensitivity;

static int mouse_oldbuttonstate;
static int old_mouse_x, old_mouse_y, mx_accum, my_accum;
float mouse_x, mouse_y;

static bool mouseactive;
static bool mouseinitialized;

#ifdef WIN32
static bool m_bMouseThread = false;

// if threaded mouse is enabled then the time to sleep between polls
static cvar_t* m_mousethread_sleep;

static bool restore_spi;
static int originalmouseparms[3], newmouseparms[3] = {0, 0, 1};
static bool mouseparmsvalid;

struct MouseThread
{
	std::thread Thread;
	std::mutex Mutex;
	std::condition_variable Condition;
	bool QuittingTime = false;
};

MouseThread s_MouseThread;

std::atomic<Point> s_mouseDelta;
std::atomic<Point> current_pos;
std::atomic<Point> old_mouse_pos;

Point GetMousePosition()
{
	POINT mouse_pos;
	GetCursorPos(&mouse_pos);

	return {
		static_cast<int>(mouse_pos.x),
		static_cast<int>(mouse_pos.y)};
}

void MousePos_ThreadFunction()
{
	while (true)
	{
		{
			std::unique_lock lock{s_MouseThread.Mutex};

			//TODO: accessing the cvar value is a race condition
			if (s_MouseThread.Condition.wait_for(
					lock,
					std::chrono::milliseconds{(int)m_mousethread_sleep->value},
					[]()
					{ return s_MouseThread.QuittingTime; }))
			{
				break;
			}
		}

		if (mouseactive)
		{
			const auto mouse_pos = GetMousePosition();

			const auto oldPos = old_mouse_pos.load();
			const auto delta = s_mouseDelta.load();

			const Point pos{
				mouse_pos.x - oldPos.x + delta.x,
				mouse_pos.y - oldPos.y + delta.y};

			old_mouse_pos = mouse_pos;
			s_mouseDelta = pos;
		}
	}
}
#endif


void Mouse_Reset()
{
	// no work to do in SDL
#ifdef WIN32
	if (Mouse_UseRawInput() && !g_iVisibleMouse)
	{
		Mouse_SetRelative(true);
	}

	if (!Mouse_UseRawInput() && mouseactive && client::GetWindowCenterX && client::GetWindowCenterY)
	{
		SetCursorPos(client::GetWindowCenterX(), client::GetWindowCenterY());

		const Point center{client::GetWindowCenterX(), client::GetWindowCenterY()};
		old_mouse_pos = center;
	}
#endif
}


void Mouse_Activate()
{
	if (mouseinitialized)
	{
#ifdef WIN32
		if (mouseparmsvalid)
			restore_spi = SystemParametersInfo(SPI_SETMOUSE, 0, newmouseparms, 0) != FALSE;

#endif
		mouseactive = true;
	}

	if (g_iVisibleMouse
#ifdef WIN32
		|| !Mouse_UseRawInput()
#endif
	)
	{
		Mouse_SetRelative(false);
	}
	else
	{
		Mouse_SetRelative(true);
	}

	// Clear out accumulated mouse input from main menu movement.
	Mouse_Reset();
}


void Mouse_Deactivate()
{
	if (mouseinitialized)
	{
#ifdef WIN32
		if (restore_spi)
			SystemParametersInfo(SPI_SETMOUSE, 0, originalmouseparms, 0);

#endif

		mouseactive = false;
	}

	Mouse_SetRelative(false);
}


static void Mouse_Startup()
{
	if (0 != client::CheckParm("-nomouse", nullptr))
		return;

	g_iVisibleMouse = true;
	mouseinitialized = true;
#ifdef WIN32
	mouseparmsvalid = SystemParametersInfo(SPI_GETMOUSE, 0, originalmouseparms, 0) != FALSE;

	if (mouseparmsvalid)
	{
		if (0 != client::CheckParm("-noforcemspd", nullptr))
			newmouseparms[2] = originalmouseparms[2];

		if (0 != client::CheckParm("-noforcemaccel", nullptr))
		{
			newmouseparms[0] = originalmouseparms[0];
			newmouseparms[1] = originalmouseparms[1];
		}

		if (0 != client::CheckParm("-noforcemparms", nullptr))
		{
			newmouseparms[0] = originalmouseparms[0];
			newmouseparms[1] = originalmouseparms[1];
			newmouseparms[2] = originalmouseparms[2];
		}
	}
#endif
}


void Mouse_Shutdown()
{
	Mouse_Deactivate();

#ifdef WIN32
	if (s_MouseThread.Thread.joinable())
	{
		//Mouse thread is active, signal it to quit and wait.
		{
			std::lock_guard guard{s_MouseThread.Mutex};
			s_MouseThread.QuittingTime = true;
		}

		s_MouseThread.Condition.notify_one();
		s_MouseThread.Thread.join();
	}
#endif
}


void Mouse_Event(int mstate)
{
	int i;

	if (iMouseInUse || g_iVisibleMouse)
	{
		mouse_oldbuttonstate = mstate;
		return;
	}

	// perform button actions
	for (i = 0; i < kMouseButtonCount; i++)
	{
		if ((mstate & (1 << i)) != 0 &&
			(mouse_oldbuttonstate & (1 << i)) == 0)
		{
			client::Key_Event(K_MOUSE1 + i, 1);
		}

		if ((mstate & (1 << i)) == 0 &&
			(mouse_oldbuttonstate & (1 << i)) != 0)
		{
			client::Key_Event(K_MOUSE1 + i, 0);
		}
	}

	mouse_oldbuttonstate = mstate;
}


static void Mouse_Scale(float* x, float* y)
{
	// This is the default sensitivity
	float mouse_senstivity =
		(gHUD.GetSensitivity() != 0) ? gHUD.GetSensitivity() : gHUD.GetDefaultSensitivity();

	// Just apply the default
	*x *= mouse_senstivity;
	*y *= mouse_senstivity;
}


static void Mouse_MoveInternal(float frametime, usercmd_t* cmd)
{
	Point pos;
	Vector viewangles;

	client::GetViewAngles(viewangles);

	//jjb - this disbles normal mouse control if the user is trying to
	//      move the camera, or if the mouse cursor is visible or if we're in intermission
	if (!iMouseInUse && !gHUD.m_iIntermission && !g_iVisibleMouse)
	{
		int deltaX, deltaY;
#ifdef WIN32
		if (!Mouse_UseRawInput())
		{
			if (m_bMouseThread)
			{
				current_pos = s_mouseDelta.load();
				s_mouseDelta = Point{};
			}
			else
			{
				current_pos = GetMousePosition();
			}
		}
		else
#endif
		{
			SDL_GetRelativeMouseState(&deltaX, &deltaY);
#ifdef WIN32
			current_pos = {deltaX, deltaY};
#endif
		}

#ifdef WIN32
		if (!Mouse_UseRawInput())
		{
			pos = current_pos.load();

			if (!m_bMouseThread)
			{
				pos.x = pos.x - client::GetWindowCenterX() + mx_accum;
				pos.y = pos.y - client::GetWindowCenterY() + my_accum;
			}
		}
		else
#endif
		{
			pos.x = deltaX + mx_accum;
			pos.y = deltaY + my_accum;
		}

		mx_accum = 0;
		my_accum = 0;

		if (m_filter && 0 != m_filter->value)
		{
			mouse_x = (pos.x + old_mouse_x) * 0.5;
			mouse_y = (pos.y + old_mouse_y) * 0.5;
		}
		else
		{
			mouse_x = pos.x;
			mouse_y = pos.y;
		}

		old_mouse_x = pos.x;
		old_mouse_y = pos.y;

		// Apply custom mouse scaling/acceleration
		Mouse_Scale(&mouse_x, &mouse_y);

		viewangles.y -= m_yaw->value * mouse_x;
		viewangles.x += m_pitch->value * mouse_y;

		viewangles.x = std::clamp(viewangles.x, -cl_pitchup->value, cl_pitchdown->value);

		// if the mouse has moved, force it to the center, so there's room to move
		if (0 != pos.x || 0 != pos.y)
		{
			Mouse_Reset();
		}
	}

	V_SetViewAngles(viewangles);

#ifdef WIN32
	if ((!Mouse_UseRawInput() && SDL_FALSE != mouseRelative) || g_iVisibleMouse)
	{
		Mouse_SetRelative(false);
	}
	else if (Mouse_UseRawInput() && SDL_FALSE == mouseRelative)
	{
		Mouse_SetRelative(true);
	}
#endif
}


void Mouse_Accumulate()
{
	//only accumulate mouse if we are not moving the camera with the mouse
	if (!iMouseInUse && !g_iVisibleMouse)
	{
		if (mouseactive)
		{
#ifdef WIN32
			if (!Mouse_UseRawInput())
			{
				if (!m_bMouseThread)
				{
					const auto pos = GetMousePosition();
					current_pos = pos;

					mx_accum += pos.x - client::GetWindowCenterX();
					my_accum += pos.y - client::GetWindowCenterY();
				}
			}
			else
#endif
			{
				int deltaX, deltaY;
				SDL_GetRelativeMouseState(&deltaX, &deltaY);
				mx_accum += deltaX;
				my_accum += deltaY;
			}
			// force the mouse to the center, so there's room to move
			Mouse_Reset();
		}
	}
}


void Mouse_ClearStates()
{
	if (!mouseactive)
		return;

	mx_accum = 0;
	my_accum = 0;
	mouse_oldbuttonstate = 0;
}


void Mouse_Move(float frametime, usercmd_t* cmd)
{
	if (mouseactive)
	{	
#ifndef WIN32
		if (g_iVisibleMouse == SDL_GetRelativeMouseMode())
		{
			SDL_SetRelativeMouseMode(g_iVisibleMouse ? SDL_FALSE : SDL_TRUE);
#else
		static bool bWasVisibleMouse = false;
		if (g_iVisibleMouse != bWasVisibleMouse)
		{
			bWasVisibleMouse = g_iVisibleMouse;
			if (Mouse_UseRawInput())
			{
				SDL_SetRelativeMouseMode(g_iVisibleMouse ? SDL_FALSE : SDL_TRUE);
			}
#endif
			if (g_iVisibleMouse)
			{
				for (int i = 0; i < kMouseButtonCount; i++)
				{
					if ((mouse_oldbuttonstate & (1 << i)) != 0)
					{
						client::Key_Event(K_MOUSE1 + i, 0);
					}
				}
				client::SetMousePos(
					client::GetWindowCenterX(),
					client::GetWindowCenterY());
			}
		}
		if (!iMouseInUse)
		{
			Mouse_MoveInternal(frametime, cmd);
		}
	}
}


void Mouse_Init()
{
	m_filter = client::RegisterVariable("m_filter", "0", FCVAR_ARCHIVE);
	m_pitch = client::RegisterVariable("m_pitch", "0.022", FCVAR_ARCHIVE);
	m_yaw = client::RegisterVariable("m_yaw", "0.022", FCVAR_ARCHIVE);
	sensitivity = client::RegisterVariable("sensitivity", "3", FCVAR_ARCHIVE);

#ifdef WIN32
	m_rawinput = client::GetCvarPointer("m_rawinput");
	m_bMouseThread = client::CheckParm("-mousethread", nullptr) != nullptr;
	m_mousethread_sleep = client::RegisterVariable("m_mousethread_sleep", "10", FCVAR_ARCHIVE);

	if (!Mouse_UseRawInput() && m_bMouseThread && m_mousethread_sleep)
	{
		s_mouseDelta = Point{};

		s_MouseThread.Thread = std::thread{&MousePos_ThreadFunction};
	}
#endif

	Mouse_Startup();
}

