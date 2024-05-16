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
#include "view.h"
#include <string.h>
#include <ctype.h>
#include "Exports.h"
#include <algorithm>

#include "vgui_TeamFortressViewport.h"
#include "filesystem_utils.h"


extern int g_weaponselect;

// Defined in pm_math.c
float anglemod(float a);

void Mouse_Init();
void Mouse_Move(float frametime, usercmd_t* cmd);
void Mouse_Shutdown();

#ifdef HALFLIFE_JOYSTICK
void Joy_Init();
void Joy_Move(float frametime, usercmd_t* cmd);

extern cvar_t* in_joystick;
#endif

void V_Init();
int CL_ButtonBits(bool);

bool g_bForceSpecialDown = false;

static int in_impulse = 0;

cvar_t* cl_pitchup;
cvar_t* cl_pitchdown;
cvar_t* cl_yawspeed;
cvar_t* cl_pitchspeed;
cvar_t* cl_anglespeedkey;
/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition

===============================================================================
*/


kbutton_t in_mlook;
kbutton_t in_klook;
kbutton_t in_left;
kbutton_t in_right;
kbutton_t in_forward;
kbutton_t in_back;
kbutton_t in_lookup;
kbutton_t in_lookdown;
kbutton_t in_moveleft;
kbutton_t in_moveright;
kbutton_t in_strafe;
kbutton_t in_speed;
kbutton_t in_use;
kbutton_t in_jump;
kbutton_t in_attack;
kbutton_t in_attack2;
kbutton_t in_up;
kbutton_t in_down;
kbutton_t in_duck;
kbutton_t in_reload;
kbutton_t in_score;
kbutton_t in_break;
kbutton_t in_graph; // Display the netgraph
kbutton_t in_gren1;
kbutton_t in_gren2;
kbutton_t in_det;

typedef struct kblist_s
{
	struct kblist_s* next;
	kbutton_t* pkey;
	char name[32];
} kblist_t;

kblist_t* g_kbkeys = nullptr;

/*
============
KB_ConvertString

Removes references to +use and replaces them with the keyname in the output string.  If
 a binding is unfound, then the original text is retained.
NOTE:  Only works for text with +word in it.
============
*/
bool KB_ConvertString(char* in, char** ppout)
{
	char sz[4096];
	char binding[64];
	char* p;
	char* pOut;
	char* pEnd;
	const char* pBinding;

	if (!ppout)
		return false;

	*ppout = nullptr;
	p = in;
	pOut = sz;
	while ('\0' != *p)
	{
		if (*p == '+')
		{
			pEnd = binding;
			while ('\0' != *p && (0 != isalnum(*p) || (pEnd == binding)) && ((pEnd - binding) < 63))
			{
				*pEnd++ = *p++;
			}

			*pEnd = '\0';

			pBinding = nullptr;
			if (strlen(binding + 1) > 0)
			{
				// See if there is a binding for binding?
				pBinding = client::Key_LookupBinding(binding + 1);
			}

			if (pBinding)
			{
				*pOut++ = '[';
				pEnd = (char*)pBinding;
			}
			else
			{
				pEnd = binding;
			}

			while ('\0' != *pEnd)
			{
				*pOut++ = *pEnd++;
			}

			if (pBinding)
			{
				*pOut++ = ']';
			}
		}
		else
		{
			*pOut++ = *p++;
		}
	}

	*pOut = '\0';

	pOut = (char*)malloc(strlen(sz) + 1);
	strcpy(pOut, sz);
	*ppout = pOut;

	return true;
}

/*
============
KB_Find

Allows the engine to get a kbutton_t directly ( so it can check +mlook state, etc ) for saving out to .cfg files
============
*/
struct kbutton_s* KB_Find(const char* name)
{
	kblist_t* p;
	p = g_kbkeys;
	while (p)
	{
		if (!stricmp(name, p->name))
			return p->pkey;

		p = p->next;
	}
	return nullptr;
}

/*
============
KB_Add

Add a kbutton_t * to the list of pointers the engine can retrieve via KB_Find
============
*/
void KB_Add(const char* name, kbutton_t* pkb)
{
	kblist_t* p;
	kbutton_t* kb;

	kb = KB_Find(name);

	if (kb)
		return;

	p = (kblist_t*)malloc(sizeof(kblist_t));
	memset(p, 0, sizeof(*p));

	strcpy(p->name, name);
	p->pkey = pkb;

	p->next = g_kbkeys;
	g_kbkeys = p;
}

/*
============
KB_Init

Add kbutton_t definitions that the engine can query if needed
============
*/
void KB_Init()
{
	g_kbkeys = nullptr;

	KB_Add("in_graph", &in_graph);
	KB_Add("in_mlook", &in_mlook);
}

/*
============
KB_Shutdown

Clear kblist
============
*/
void KB_Shutdown()
{
	kblist_t *p, *n;
	p = g_kbkeys;
	while (p)
	{
		n = p->next;
		free(p);
		p = n;
	}
	g_kbkeys = nullptr;
}

/*
============
KeyDown
============
*/
void KeyDown(kbutton_t* b)
{
	int k;
	const char* c;

	c = client::Cmd_Argv(1);
	if ('\0' != c[0])
		k = atoi(c);
	else
		k = -1; // typed manually at the console for continuous down

	if (k == b->down[0] || k == b->down[1])
		return; // repeating key

	if (0 == b->down[0])
		b->down[0] = k;
	else if (0 == b->down[1])
		b->down[1] = k;
	else
	{
		client::Con_DPrintf("Three keys down for a button '%c' '%c' '%c'!\n", b->down[0], b->down[1], c);
		return;
	}

	//TODO: define constants
	if ((b->state & 1) != 0)
		return;		   // still down
	b->state |= 1 + 2; // down + impulse down
}

/*
============
KeyUp
============
*/
void KeyUp(kbutton_t* b)
{
	int k;
	const char* c;

	c = client::Cmd_Argv(1);
	if ('\0' != c[0])
		k = atoi(c);
	else
	{ // typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4; // impulse up
		return;
	}

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return; // key up without coresponding down (menu pass through)
	if (0 != b->down[0] || 0 != b->down[1])
	{
		//Con_Printf ("Keys down for button: '%c' '%c' '%c' (%d,%d,%d)!\n", b->down[0], b->down[1], c, b->down[0], b->down[1], c);
		return; // some other key is still holding it down
	}

	if ((b->state & 1) == 0)
		return; // still up (this should not happen)

	b->state &= ~1; // now up
	b->state |= 4;	// impulse up
}

/*
============
HUD_Key_Event

Return 1 to allow engine to process the key, otherwise, act on it as needed
============
*/
int HUD_Key_Event(int down, int keynum, const char* pszCurrentBinding)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->KeyInput(0 != down, keynum, pszCurrentBinding));

	return 1;
}

void IN_BreakDown() { KeyDown(&in_break); }
void IN_BreakUp() { KeyUp(&in_break); }
void IN_KLookDown() { KeyDown(&in_klook); }
void IN_KLookUp() { KeyUp(&in_klook); }
void IN_MLookDown() { KeyDown(&in_mlook); }
void IN_MLookUp() { KeyUp(&in_mlook); }
void IN_UpDown() { KeyDown(&in_up); }
void IN_UpUp() { KeyUp(&in_up); }
void IN_DownDown() { KeyDown(&in_down); }
void IN_DownUp() { KeyUp(&in_down); }
void IN_LeftDown() { KeyDown(&in_left); }
void IN_LeftUp() { KeyUp(&in_left); }
void IN_RightDown() { KeyDown(&in_right); }
void IN_RightUp() { KeyUp(&in_right); }

void IN_ForwardDown()
{
	KeyDown(&in_forward);
	gHUD.m_Spectator.HandleButtonsDown(IN_FORWARD);
}

void IN_ForwardUp()
{
	KeyUp(&in_forward);
	gHUD.m_Spectator.HandleButtonsUp(IN_FORWARD);
}

void IN_BackDown()
{
	KeyDown(&in_back);
	gHUD.m_Spectator.HandleButtonsDown(IN_BACK);
}

void IN_BackUp()
{
	KeyUp(&in_back);
	gHUD.m_Spectator.HandleButtonsUp(IN_BACK);
}
void IN_LookupDown() { KeyDown(&in_lookup); }
void IN_LookupUp() { KeyUp(&in_lookup); }
void IN_LookdownDown() { KeyDown(&in_lookdown); }
void IN_LookdownUp() { KeyUp(&in_lookdown); }
void IN_MoveleftDown()
{
	KeyDown(&in_moveleft);
	gHUD.m_Spectator.HandleButtonsDown(IN_MOVELEFT);
}

void IN_MoveleftUp()
{
	KeyUp(&in_moveleft);
	gHUD.m_Spectator.HandleButtonsUp(IN_MOVELEFT);
}

void IN_MoverightDown()
{
	KeyDown(&in_moveright);
	gHUD.m_Spectator.HandleButtonsDown(IN_MOVERIGHT);
}

void IN_MoverightUp()
{
	KeyUp(&in_moveright);
	gHUD.m_Spectator.HandleButtonsUp(IN_MOVERIGHT);
}
void IN_SpeedDown() { KeyDown(&in_speed); }
void IN_SpeedUp() { KeyUp(&in_speed); }
void IN_StrafeDown() { KeyDown(&in_strafe); }
void IN_StrafeUp() { KeyUp(&in_strafe); }

// needs capture by hud/vgui also
extern void __CmdFunc_InputPlayerSpecial();

void IN_Attack2Down()
{
	KeyDown(&in_attack2);
	__CmdFunc_InputPlayerSpecial();
	gHUD.m_Spectator.HandleButtonsDown(IN_ATTACK2);
}

void IN_Attack2Up() { KeyUp(&in_attack2); }
void IN_UseDown()
{
	KeyDown(&in_use);
	gHUD.m_Spectator.HandleButtonsDown(IN_USE);
}
void IN_UseUp() { KeyUp(&in_use); }
void IN_JumpDown()
{
	KeyDown(&in_jump);
	gHUD.m_Spectator.HandleButtonsDown(IN_JUMP);
}
void IN_JumpUp() { KeyUp(&in_jump); }
void IN_DuckDown()
{
	KeyDown(&in_duck);
	gHUD.m_Spectator.HandleButtonsDown(IN_DUCK);
}
void IN_DuckUp() { KeyUp(&in_duck); }
void IN_ReloadDown() { KeyDown(&in_reload); }
void IN_ReloadUp() { KeyUp(&in_reload); }
void IN_GraphDown() { KeyDown(&in_graph); }
void IN_GraphUp() { KeyUp(&in_graph); }

void IN_AttackDown()
{
	KeyDown(&in_attack);
	gHUD.m_Spectator.HandleButtonsDown(IN_ATTACK);
}

void IN_AttackUp()
{
	KeyUp(&in_attack);
}

void IN_Impulse()
{
	auto impulse = atoi(client::Cmd_Argv(1));

	if (!gHUD.ImpulseCommands(impulse))
	{
		in_impulse = impulse;
	}
}

void IN_ScoreDown()
{
	KeyDown(&in_score);
	if (gViewPort)
	{
		gViewPort->ShowScoreBoard();
	}
}

void IN_ScoreUp()
{
	KeyUp(&in_score);
	if (gViewPort)
	{
		gViewPort->HideScoreBoard();
	}
}

void IN_Gren1Down() { KeyDown(&in_gren1); }
void IN_Gren1Up() { KeyUp(&in_gren1); }

void IN_Gren2Down() { KeyDown(&in_gren2); }
void IN_Gren2Up() { KeyUp(&in_gren2); }

void IN_Det5Down() { KeyDown(&in_det); client::ServerCmd("detstart 5"); }
void IN_Det20Down() { KeyDown(&in_det); client::ServerCmd("detstart 20"); }
void IN_Det50Down() { KeyDown(&in_det); client::ServerCmd("detstart 50"); }
void IN_DetUp() { KeyUp(&in_det); }

void DetStart()
{
	/* Force the special key down. */
	g_bForceSpecialDown = true;

	auto fuse = 5;

	if (client::Cmd_Argc() > 1)
	{
		/* Clamp the user fuse value. */
		fuse = std::clamp(atoi(client::Cmd_Argv(1)), 5, 50);
	}

	/* Forward the command to the server to set the fuse. */
	char detstart[32];

	snprintf(detstart, sizeof(detstart) - 1, "detstart %i", fuse);
	detstart[sizeof(detstart) - 1] = '\0';

	client::ServerCmd(detstart);
}

void DetStop()
{
	/* Release the special key. */
	g_bForceSpecialDown = false;
}

/*
===============
CL_KeyState

Returns 0.25 if a key was pressed and released during the frame,
0.5 if it was pressed and held
0 if held then released, and
1.0 if held for the entire time
===============
*/
float CL_KeyState(kbutton_t* key)
{
#if 0
	const bool impulsedown = (key->state & 2) != 0;
	const bool impulseup = (key->state & 4) != 0;
#endif
	const bool down = (key->state & 1) != 0;

	// clear impulses
	key->state &= 1;

	return down ? 1.0F : 0.0F;
}

/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
void CL_AdjustAngles(float frametime, Vector& viewangles)
{
	float speed;
	float up, down;

	if ((in_speed.state & 1) != 0)
	{
		speed = frametime * cl_anglespeedkey->value;
	}
	else
	{
		speed = frametime;
	}

	viewangles[YAW] -= speed * cl_yawspeed->value * CL_KeyState(&in_right);
	viewangles[YAW] += speed * cl_yawspeed->value * CL_KeyState(&in_left);
	viewangles[YAW] = anglemod(viewangles[YAW]);

	up = CL_KeyState(&in_lookup);
	down = CL_KeyState(&in_lookdown);

	viewangles[PITCH] -= speed * cl_pitchspeed->value * up;
	viewangles[PITCH] += speed * cl_pitchspeed->value * down;

	viewangles[PITCH] = std::clamp(viewangles[PITCH], -cl_pitchup->value, cl_pitchdown->value);
}

/*
================
CL_CreateMove

Send the intended movement message to the server
if active == 1 then we are 1) not playing back demos ( where our commands are ignored ) and
2 ) we have finished signing on to server
================
*/
void CL_CreateMove(float frametime, struct usercmd_s* cmd, int active)
{
	float spd;
	Vector viewangles;

	if (0 != active)
	{
		client::GetViewAngles(viewangles);

		CL_AdjustAngles(frametime, viewangles);

		memset(cmd, 0, sizeof(*cmd));

		client::SetViewAngles(viewangles);

		Vector move =
		{
			CL_KeyState(&in_forward) - CL_KeyState(&in_back),
			CL_KeyState(&in_moveright) - CL_KeyState(&in_moveleft),
			CL_KeyState(&in_up) - CL_KeyState(&in_down),
		};

		if (move.LengthSquared() != 0.0F)
		{
			move = move.Normalize() * 100.0F;

			*reinterpret_cast<int*>(&cmd->forwardmove) = static_cast<int>(move.x);
			*reinterpret_cast<int*>(&cmd->sidemove) = static_cast<int>(move.y);
			*reinterpret_cast<int*>(&cmd->upmove) = static_cast<int>(move.z);
		}
		else
		{
			*reinterpret_cast<int*>(&cmd->forwardmove) = 0;
			*reinterpret_cast<int*>(&cmd->sidemove) = 0;
			*reinterpret_cast<int*>(&cmd->upmove) = 0;
		}

		// Allow mice and other controllers to add their inputs
		Mouse_Move(frametime, cmd);
#ifdef HALFLIFE_JOYSTICK
		Joy_Move(frametime, cmd);
#endif
	}

	cmd->impulse = in_impulse;
	in_impulse = 0;

	cmd->weaponselect = g_weaponselect + 1;
	g_weaponselect = -1;
	//
	// set button and flag bits
	//
	cmd->buttons = CL_ButtonBits(true);

#ifdef HALFLIFE_JOYSTICK
	if (0 != in_joystick->value)
	{
		if (*reinterpret_cast<int*>(&cmd->forwardmove) > 0)
		{
			cmd->buttons |= IN_FORWARD;
		}
		else if (*reinterpret_cast<int*>(&cmd->forwardmove) < 0)
		{
			cmd->buttons |= IN_BACK;
		}
	}
#endif

	// Set current view angles.
	client::GetViewAngles(cmd->viewangles);
}

/*
============
CL_ButtonBits

Returns appropriate button info for keyboard and mouse state
Set bResetState to 1 to clear old state info
============
*/
int CL_ButtonBits(bool bResetState)
{
	int bits = 0;

	if ((in_attack.state & 3) != 0)
	{
		bits |= IN_ATTACK;
	}

	if ((in_duck.state & 3) != 0)
	{
		bits |= IN_DUCK;
	}

	if ((in_jump.state & 3) != 0)
	{
		bits |= IN_JUMP;
	}

	if ((in_forward.state & 3) != 0)
	{
		bits |= IN_FORWARD;
	}

	if ((in_back.state & 3) != 0)
	{
		bits |= IN_BACK;
	}

	if ((in_use.state & 3) != 0)
	{
		bits |= IN_USE;
	}

	if ((in_speed.state & 3) != 0)
	{
		bits |= IN_SPEED;
	}

	if ((in_left.state & 3) != 0)
	{
		bits |= IN_LEFT;
	}

	if ((in_right.state & 3) != 0)
	{
		bits |= IN_RIGHT;
	}

	if ((in_moveleft.state & 3) != 0)
	{
		bits |= IN_MOVELEFT;
	}

	if ((in_moveright.state & 3) != 0)
	{
		bits |= IN_MOVERIGHT;
	}

	if ((in_attack2.state & 3) != 0)
	{
		bits |= IN_ATTACK2;
	}

	if ((in_reload.state & 3) != 0)
	{
		bits |= IN_RELOAD;
	}

	if ((in_gren1.state & 3) != 0)
	{
		bits |= IN_GRENADE;
	}

	if ((in_gren2.state & 3) != 0)
	{
		bits |= IN_GRENADE2;
	}

	if ((in_det.state & 3) != 0
	 || g_bForceSpecialDown)
	{
		bits |= IN_SPECIAL;
	}

	if (bResetState)
	{
		in_attack.state &= ~2;
		in_duck.state &= ~2;
		in_jump.state &= ~2;
		in_forward.state &= ~2;
		in_back.state &= ~2;
		in_use.state &= ~2;
		in_speed.state &= ~2;
		in_left.state &= ~2;
		in_right.state &= ~2;
		in_moveleft.state &= ~2;
		in_moveright.state &= ~2;
		in_attack2.state &= ~2;
		in_reload.state &= ~2;
		in_score.state &= ~2;
		in_gren1.state &= ~2;
		in_gren2.state &= ~2;
		in_det.state &= ~2;
	}

	return bits;
}

/*
============
CL_ResetButtonBits

============
*/
void CL_ResetButtonBits(int bits)
{
	int bitsNew = CL_ButtonBits(false) ^ bits;

	// Has the attack button been changed
	if ((bitsNew & IN_ATTACK) != 0)
	{
		// Was it pressed? or let go?
		if ((bits & IN_ATTACK) != 0)
		{
			KeyDown(&in_attack);
		}
		else
		{
			// totally clear state
			in_attack.state &= ~7;
		}
	}
}

/*
============
InitInput
============
*/
void InitInput()
{
	client::AddCommand("+moveup", IN_UpDown);
	client::AddCommand("-moveup", IN_UpUp);
	client::AddCommand("+movedown", IN_DownDown);
	client::AddCommand("-movedown", IN_DownUp);
	client::AddCommand("+left", IN_LeftDown);
	client::AddCommand("-left", IN_LeftUp);
	client::AddCommand("+right", IN_RightDown);
	client::AddCommand("-right", IN_RightUp);
	client::AddCommand("+forward", IN_ForwardDown);
	client::AddCommand("-forward", IN_ForwardUp);
	client::AddCommand("+back", IN_BackDown);
	client::AddCommand("-back", IN_BackUp);
	client::AddCommand("+lookup", IN_LookupDown);
	client::AddCommand("-lookup", IN_LookupUp);
	client::AddCommand("+lookdown", IN_LookdownDown);
	client::AddCommand("-lookdown", IN_LookdownUp);
	client::AddCommand("+strafe", IN_StrafeDown);
	client::AddCommand("-strafe", IN_StrafeUp);
	client::AddCommand("+moveleft", IN_MoveleftDown);
	client::AddCommand("-moveleft", IN_MoveleftUp);
	client::AddCommand("+moveright", IN_MoverightDown);
	client::AddCommand("-moveright", IN_MoverightUp);
	client::AddCommand("+speed", IN_SpeedDown);
	client::AddCommand("-speed", IN_SpeedUp);
	client::AddCommand("+attack", IN_AttackDown);
	client::AddCommand("-attack", IN_AttackUp);
	client::AddCommand("+attack2", IN_Attack2Down);
	client::AddCommand("-attack2", IN_Attack2Up);
	client::AddCommand("+use", IN_UseDown);
	client::AddCommand("-use", IN_UseUp);
	client::AddCommand("+jump", IN_JumpDown);
	client::AddCommand("-jump", IN_JumpUp);
	client::AddCommand("impulse", IN_Impulse);
	client::AddCommand("+klook", IN_KLookDown);
	client::AddCommand("-klook", IN_KLookUp);
	client::AddCommand("+mlook", IN_MLookDown);
	client::AddCommand("-mlook", IN_MLookUp);
	client::AddCommand("+duck", IN_DuckDown);
	client::AddCommand("-duck", IN_DuckUp);
	client::AddCommand("+reload", IN_ReloadDown);
	client::AddCommand("-reload", IN_ReloadUp);
	client::AddCommand("+score", IN_ScoreDown);
	client::AddCommand("-score", IN_ScoreUp);
	client::AddCommand("+showscores", IN_ScoreDown);
	client::AddCommand("-showscores", IN_ScoreUp);
	client::AddCommand("+graph", IN_GraphDown);
	client::AddCommand("-graph", IN_GraphUp);
	client::AddCommand("+break", IN_BreakDown);
	client::AddCommand("-break", IN_BreakUp);
	client::AddCommand("+gren1", IN_Gren1Down);
	client::AddCommand("-gren1", IN_Gren1Up);
	client::AddCommand("+gren2", IN_Gren2Down);
	client::AddCommand("-gren2", IN_Gren2Up);
	client::AddCommand("+det5", IN_Det5Down);
	client::AddCommand("-det5", IN_DetUp);
	client::AddCommand("+det20", IN_Det20Down);
	client::AddCommand("-det20", IN_DetUp);
	client::AddCommand("+det50", IN_Det50Down);
	client::AddCommand("-det50", IN_DetUp);

	client::AddCommand("detstart", DetStart);
	client::AddCommand("detstop", DetStop);

	cl_anglespeedkey = client::RegisterVariable("cl_anglespeedkey", "0.67", 0);
	cl_yawspeed = client::RegisterVariable("cl_yawspeed", "210", 0);
	cl_pitchspeed = client::RegisterVariable("cl_pitchspeed", "225", 0);
	cl_pitchup = client::RegisterVariable("cl_pitchup", "89", 0);
	cl_pitchdown = client::RegisterVariable("cl_pitchdown", "89", 0);

	// Initialize third person camera controls.
	CAM_Init();
	// Initialize inputs
	Mouse_Init();
#ifdef HALFLIFE_JOYSTICK
	// Initialize game controllers
	Joy_Init();
#endif
	// Initialize keyboard
	KB_Init();
	// Initialize view system
	V_Init();
}

/*
============
ShutdownInput
============
*/
void ShutdownInput()
{
	Mouse_Shutdown();
	KB_Shutdown();
}

#include "interface.h"
void CL_UnloadParticleMan();


void HUD_Shutdown()
{
	ShutdownInput();


	FileSystem_FreeFileSystem();
	CL_UnloadParticleMan();
}
