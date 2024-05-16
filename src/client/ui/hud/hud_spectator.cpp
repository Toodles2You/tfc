//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "cl_util.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_SpectatorPanel.h"
#include "hltv.h"

#include "pm_shared.h"
#include "pm_defs.h"
#include "pmtrace.h"
#include "parsemsg.h"
#include "entity_types.h"

// these are included for the math functions
#include "com_model.h"
#include "demo_api.h"
#include "event_api.h"
#include "studio_util.h"
#include "screenfade.h"


#pragma warning(disable : 4244)

extern bool iJumpSpectator;
extern Vector vJumpOrigin;
extern Vector vJumpAngles;


extern void V_GetInEyePos(int entity, Vector& origin, Vector& angles);
extern void V_ResetChaseCam();
extern void V_GetChasePos(int target, float* cl_angles, Vector& origin, Vector& angles);

extern Vector v_origin;	   // last view origin
extern Vector v_angles;	   // last view angle
extern Vector v_cl_angles; // last client/mouse angle
extern Vector v_sim_org;   // last sim origin

int g_iObserverMode = OBS_NONE;
int g_iObserverTarget = 0;
int g_iObserverTarget2 = 0;

const char* GetSpectatorLabel(int iMode);

void SpectatorMode()
{
	if (client::Cmd_Argc() <= 1)
	{
		client::Con_Printf("usage:  spec_mode <Main Mode> [<Inset Mode>]\n");
		return;
	}

	// SetModes() will decide if command is executed on server or local
	if (client::Cmd_Argc() == 2)
		gHUD.m_Spectator.SetModes(atoi(client::Cmd_Argv(1)), -1);
	else if (client::Cmd_Argc() == 3)
		gHUD.m_Spectator.SetModes(atoi(client::Cmd_Argv(1)), atoi(client::Cmd_Argv(2)));
}

void SpectatorSpray()
{
	Vector forward;
	char string[128];

	if (0 == client::IsSpectateOnly())
		return;

	AngleVectors(v_angles, &forward, nullptr, nullptr);
	forward = forward * 128.0F + v_origin;
	pmtrace_t* trace = client::PM_TraceLine(v_origin, forward, PM_TRACELINE_PHYSENTSONLY, 2, -1);
	if (trace->fraction != 1.0)
	{
		sprintf(string, "drc_spray %.2f %.2f %.2f %i",
			trace->endpos[0], trace->endpos[1], trace->endpos[2], trace->ent);
		client::ServerCmd(string);
	}
}
void SpectatorHelp()
{
	if (gViewPort)
	{
		gViewPort->ShowVGUIMenu(MENU_SPECHELP);
	}
	else
	{
		char* text = CHudTextMessage::BufferedLocaliseTextString("#Spec_Help_Text");

		if (text)
		{
			while ('\0' != *text)
			{
				if (*text != 13)
					client::Con_Printf("%c", *text);
				text++;
			}
		}
	}
}

void SpectatorMenu()
{
	if (client::Cmd_Argc() <= 1)
	{
		client::Con_Printf("usage:  spec_menu <0|1>\n");
		return;
	}

	gViewPort->m_pSpectatorPanel->ShowMenu(atoi(client::Cmd_Argv(1)) != 0);
}

void ToggleScores()
{
	if (gViewPort)
	{
		if (gViewPort->IsScoreBoardVisible())
		{
			gViewPort->HideScoreBoard();
		}
		else
		{
			gViewPort->ShowScoreBoard();
		}
	}
}

bool CHudSpectator::IsActive()
{
	return gHUD.IsSpectator();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CHudSpectator::Init()
{
	m_flNextObserverInput = 0.0f;
	m_zoomDelta = 0.0f;
	m_moveDelta = 0.0f;
	m_FOV = 90.0f;
	m_chatEnabled = (gHUD.m_SayText.m_HUD_saytext->value != 0);
	iJumpSpectator = false;

	memset(&m_OverviewData, 0, sizeof(m_OverviewData));
	memset(&m_OverviewEntities, 0, sizeof(m_OverviewEntities));
	m_lastPrimaryObject = m_lastSecondaryObject = 0;

	client::AddCommand("spec_mode", SpectatorMode);
	client::AddCommand("spec_decal", SpectatorSpray);
	client::AddCommand("spec_help", SpectatorHelp);
	client::AddCommand("spec_menu", SpectatorMenu);
	client::AddCommand("togglescores", ToggleScores);

	m_drawnames = client::RegisterVariable("spec_drawnames", "1", 0);
	m_drawcone = client::RegisterVariable("spec_drawcone", "1", 0);
	m_drawstatus = client::RegisterVariable("spec_drawstatus", "1", 0);
	m_autoDirector = client::RegisterVariable("spec_autodirector", "1", 0);
	m_pip = client::RegisterVariable("spec_pip", "1", 0);

	if (!m_drawnames || !m_drawcone || !m_drawstatus || !m_autoDirector || !m_pip)
	{
		client::Con_Printf("ERROR! Couldn't register all spectator variables.\n");
		return false;
	}

	return CHudBase::Init();
}


//-----------------------------------------------------------------------------
// UTIL_StringToVector originally from ..\dlls\util.cpp, slightly changed
//-----------------------------------------------------------------------------

void UTIL_StringToVector(Vector& pVector, const char* pString)
{
	char *pstr, *pfront, tempString[128];
	int j;

	strcpy(tempString, pString);
	pstr = pfront = tempString;

	for (j = 0; j < 3; j++)
	{
		pVector[j] = atof(pfront);

		while ('\0' != *pstr && *pstr != ' ')
			pstr++;
		if ('\0' == *pstr)
			break;
		pstr++;
		pfront = pstr;
	}

	if (j < 2)
	{
		for (j = j + 1; j < 3; j++)
			pVector[j] = 0;
	}
}

bool UTIL_FindEntityInMap(const char* name, Vector& origin, Vector& angle)
{
	int n;
	bool found = false;
	char keyname[256];
	char token[1024];

	cl_entity_t* pEnt = client::GetEntityByIndex(0); // get world model

	if (!pEnt)
		return false;

	if (!pEnt->model)
		return false;

	char* data = pEnt->model->entities;

	while (data)
	{
		data = client::COM_ParseFile(data, token);

		if ((token[0] == '}') || (token[0] == 0))
			break;

		if (!data)
		{
			client::Con_DPrintf("UTIL_FindEntityInMap: EOF without closing brace\n");
			return false;
		}

		if (token[0] != '{')
		{
			client::Con_DPrintf("UTIL_FindEntityInMap: expected {\n");
			return false;
		}

		// we parse the first { now parse entities properties

		while (true)
		{
			// parse key
			data = client::COM_ParseFile(data, token);
			if (token[0] == '}')
				break; // finish parsing this entity

			if (!data)
			{
				client::Con_DPrintf("UTIL_FindEntityInMap: EOF without closing brace\n");
				return false;
			}

			strcpy(keyname, token);

			// another hack to fix keynames with trailing spaces
			n = strlen(keyname);
			while (0 != n && keyname[n - 1] == ' ')
			{
				keyname[n - 1] = 0;
				n--;
			}

			// parse value
			data = client::COM_ParseFile(data, token);
			if (!data)
			{
				client::Con_DPrintf("UTIL_FindEntityInMap: EOF without closing brace\n");
				return false;
			}

			if (token[0] == '}')
			{
				client::Con_DPrintf("UTIL_FindEntityInMap: closing brace without data");
				return false;
			}

			if (0 == strcmp(keyname, "classname"))
			{
				if (0 == strcmp(token, name))
				{
					found = true; // thats our entity
				}
			}

			if (0 == strcmp(keyname, "angle"))
			{
				float y = atof(token);

				if (y >= 0)
				{
					angle[0] = 0.0f;
					angle[1] = y;
				}
				else if ((int)y == -1)
				{
					angle[0] = -90.0f;
					angle[1] = 0.0f;
				}
				else
				{
					angle[0] = 90.0f;
					angle[1] = 0.0f;
				}

				angle[2] = 0.0f;
			}

			if (0 == strcmp(keyname, "angles"))
			{
				UTIL_StringToVector(angle, token);
			}

			if (0 == strcmp(keyname, "origin"))
			{
				UTIL_StringToVector(origin, token);
			}
		} // while (1)

		if (found)
			return true;
	}

	return false; // we search all entities, but didn't found the correct
}

//-----------------------------------------------------------------------------
// SetSpectatorStartPosition():
// Get valid map position and 'beam' spectator to this position
//-----------------------------------------------------------------------------

void CHudSpectator::SetSpectatorStartPosition()
{
	// search for info_player start
	if (UTIL_FindEntityInMap("trigger_camera", m_cameraOrigin, m_cameraAngles))
		iJumpSpectator = true;

	else if (UTIL_FindEntityInMap("info_player_start", m_cameraOrigin, m_cameraAngles))
		iJumpSpectator = true;

	else if (UTIL_FindEntityInMap("info_player_deathmatch", m_cameraOrigin, m_cameraAngles))
		iJumpSpectator = true;

	else if (UTIL_FindEntityInMap("info_player_coop", m_cameraOrigin, m_cameraAngles))
		iJumpSpectator = true;
	else
	{
		// jump to 0,0,0 if no better position was found
		m_cameraOrigin = g_vecZero;
		m_cameraAngles = g_vecZero;
	}

	vJumpOrigin = m_cameraOrigin;
	vJumpAngles = m_cameraAngles;

	iJumpSpectator = true; // jump anyway
}


void CHudSpectator::SetCameraView(Vector pos, Vector angle, float fov)
{
	m_FOV = fov;
	vJumpOrigin = pos;
	vJumpAngles = angle;
	client::SetViewAngles(vJumpAngles);
	iJumpSpectator = true; // jump anyway
}

void CHudSpectator::AddWaypoint(float time, Vector pos, Vector angle, float fov, int flags)
{
	//TODO: this flags check is incorrect, fix it. Comment contains original code before bool fix.
	if (/*!flags == 0*/ flags == 0 && time == 0.0f)
	{
		// switch instantly to this camera view
		SetCameraView(pos, angle, fov);
		return;
	}

	if (m_NumWayPoints >= MAX_CAM_WAYPOINTS)
	{
		client::Con_Printf("Too many camera waypoints!\n");
		return;
	}

	m_CamPath[m_NumWayPoints].angle = angle;
	m_CamPath[m_NumWayPoints].position = pos;
	m_CamPath[m_NumWayPoints].flags = flags;
	m_CamPath[m_NumWayPoints].fov = fov;
	m_CamPath[m_NumWayPoints].time = time;

	client::Con_DPrintf("Added waypoint %i\n", m_NumWayPoints);

	m_NumWayPoints++;
}

void CHudSpectator::SetWayInterpolation(cameraWayPoint_t* prev, cameraWayPoint_t* start, cameraWayPoint_t* end, cameraWayPoint_t* next)
{
	m_WayInterpolation.SetViewAngles(start->angle, end->angle);

	m_WayInterpolation.SetFOVs(start->fov, end->fov);

	m_WayInterpolation.SetSmoothing((start->flags & DRC_FLAG_SLOWSTART) != 0,
		(start->flags & DRC_FLAG_SLOWEND) != 0);

	if (prev && next)
	{
		m_WayInterpolation.SetWaypoints(&prev->position, start->position, end->position, &next->position);
	}
	else if (prev)
	{
		m_WayInterpolation.SetWaypoints(&prev->position, start->position, end->position, nullptr);
	}
	else if (next)
	{
		m_WayInterpolation.SetWaypoints(nullptr, start->position, end->position, &next->position);
	}
	else
	{
		m_WayInterpolation.SetWaypoints(nullptr, start->position, end->position, nullptr);
	}
}

bool CHudSpectator::GetDirectorCamera(Vector& position, Vector& angle)
{
	float now = gHUD.m_flTime;
	float fov = 90.0f;

	if (0 != m_ChaseEntity)
	{
		cl_entity_t* ent = client::GetEntityByIndex(m_ChaseEntity);

		if (ent)
		{
			Vector vt = ent->curstate.origin;

			if (m_ChaseEntity <= client::GetMaxClients())
			{
				if (ent->curstate.solid == SOLID_NOT)
				{
					vt = vt + VEC_DEAD_VIEW;
				}
				else if (ent->curstate.usehull == 1)
				{
					vt = vt + VEC_DUCK_VIEW;
				}
				else
				{
					vt = vt + VEC_VIEW;
				}
			}

			vt = vt - position;
			VectorAngles(vt, angle);
			angle[0] = -angle[0];
			return true;
		}
		else
		{
			return false;
		}
	}

	if (!m_IsInterpolating)
		return false;

	if (m_WayPoint < 0 || m_WayPoint >= (m_NumWayPoints - 1))
		return false;

	cameraWayPoint_t* wp1 = &m_CamPath[m_WayPoint];
	cameraWayPoint_t* wp2 = &m_CamPath[m_WayPoint + 1];

	if (now < wp1->time)
		return false;

	while (now > wp2->time)
	{
		// go to next waypoint, if possible
		m_WayPoint++;

		if (m_WayPoint >= (m_NumWayPoints - 1))
		{
			m_IsInterpolating = false;
			return false; // there is no following waypoint
		}

		wp1 = wp2;
		wp2 = &m_CamPath[m_WayPoint + 1];

		if (m_WayPoint > 0)
		{
			// we have a predecessor

			if (m_WayPoint < (m_NumWayPoints - 1))
			{
				// we have also a successor
				SetWayInterpolation(&m_CamPath[m_WayPoint - 1], wp1, wp2, &m_CamPath[m_WayPoint + 2]);
			}
			else
			{
				SetWayInterpolation(&m_CamPath[m_WayPoint - 1], wp1, wp2, nullptr);
			}
		}
		else if (m_WayPoint < (m_NumWayPoints - 1))
		{
			// we only have a successor
			SetWayInterpolation(nullptr, wp1, wp2, &m_CamPath[m_WayPoint + 2]);
		}
		else
		{
			// we have only two waypoints
			SetWayInterpolation(nullptr, wp1, wp2, nullptr);
		}
	}

	if (wp2->time <= wp1->time)
		return false;

	float fraction = (now - wp1->time) / (wp2->time - wp1->time);

	if (fraction < 0.0f)
		fraction = 0.0f;
	else if (fraction > 1.0f)
		fraction = 1.0f;

	m_WayInterpolation.Interpolate(fraction, position, angle, &fov);

	// client::Con_Printf("Interpolate time: %.2f, fraction %.2f, point : %.2f,%.2f,%.2f\n", now, fraction, position[0], position[1], position[2] );

	SetCameraView(position, angle, fov);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Loads new icons
//-----------------------------------------------------------------------------
void CHudSpectator::VidInit()
{
	m_hsprPlayer = client::SPR_Load("sprites/iplayer.spr");
	m_hsprPlayerBlue = client::SPR_Load("sprites/iplayerblue.spr");
	m_hsprPlayerRed = client::SPR_Load("sprites/iplayerred.spr");
	m_hsprPlayerDead = client::SPR_Load("sprites/iplayerdead.spr");
	m_hsprUnkownMap = client::SPR_Load("sprites/tile.spr");
	m_hsprBeam = client::SPR_Load("sprites/laserbeam.spr");
	m_hsprCamera = client::SPR_Load("sprites/camera.spr");

	m_lastPrimaryObject = m_lastSecondaryObject = 0;
	m_flNextObserverInput = 0.0f;
	m_lastHudMessage = 0;
	m_iSpectatorNumber = 0;
	iJumpSpectator = false;
	g_iObserverMode = OBS_NONE;
	g_iObserverTarget = g_iObserverTarget2 = 0;
}

float CHudSpectator::GetFOV()
{
	return m_FOV;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : flTime -
//			intermission -
//-----------------------------------------------------------------------------
void CHudSpectator::Draw(const float time)
{
	int lx;

	char string[256];
	Vector color;

	// if user pressed zoom, aplly changes
	if ((m_zoomDelta != 0.0f) && (g_iObserverMode == OBS_MAP_FREE))
	{
		m_mapZoom += m_zoomDelta;

		if (m_mapZoom > 3.0f)
			m_mapZoom = 3.0f;

		if (m_mapZoom < 0.5f)
			m_mapZoom = 0.5f;
	}

	// if user moves in map mode, change map origin
	if ((m_moveDelta != 0.0f) && (g_iObserverMode != OBS_ROAMING))
	{
		Vector right;
		AngleVectors(v_angles, nullptr, &right, nullptr);

		m_mapOrigin = m_mapOrigin + right.Normalize() * m_moveDelta;
	}

	// Only draw the icon names only if map mode is in Main Mode
	if (g_iObserverMode < OBS_MAP_FREE)
		return;

	if (0 == m_drawnames->value)
		return;

	// make sure we have player info
	gViewPort->GetAllPlayersInfo();


	// loop through all the players and draw additional infos to their sprites on the map
	for (int i = 0; i < MAX_PLAYERS_HUD; i++)
	{

		if (m_vPlayerPos[i][2] < 0) // marked as invisible ?
			continue;

		// check if name would be in inset window
		if (m_pip->value != INSET_OFF)
		{
			if (m_vPlayerPos[i][0] > XRES(m_OverviewData.insetWindowX) &&
				m_vPlayerPos[i][1] > YRES(m_OverviewData.insetWindowY) &&
				m_vPlayerPos[i][0] < XRES(m_OverviewData.insetWindowX + m_OverviewData.insetWindowWidth) &&
				m_vPlayerPos[i][1] < YRES(m_OverviewData.insetWindowY + m_OverviewData.insetWindowHeight))
				continue;
		}

		color = gHUD.GetClientColor(i + 1);

		// draw the players name and health underneath
		sprintf(string, "%s", g_PlayerInfoList[i + 1].name);

		lx = strlen(string) * 3; // 3 is avg. character length :)

		client::DrawSetTextColor(color[0], color[1], color[2]);
		DrawConsoleString(m_vPlayerPos[i][0] - lx, m_vPlayerPos[i][1], string);
	}
}


void CHudSpectator::DirectorMessage(int iSize, void* pbuf)
{
	float f1, f2;
	char* string;
	Vector v1, v2;
	int i1, i2, i3;

	BEGIN_READ(pbuf, iSize);

	int cmd = READ_BYTE();

	switch (cmd) // director command byte
	{
	case DRC_CMD_START:
		// now we have to do some things clientside, since the proxy doesn't know our mod
		g_iPlayerClass = PC_UNDEFINED;
		g_iTeamNumber = TEAM_UNASSIGNED;

		// fake a InitHUD & ResetHUD message
		gHUD.MsgFunc_InitHUD(nullptr, 0, nullptr);
		gHUD.MsgFunc_ResetHUD(nullptr, 0, nullptr);

		break;

	case DRC_CMD_EVENT: // old director style message
		m_lastPrimaryObject = READ_WORD();
		m_lastSecondaryObject = READ_WORD();
		m_iObserverFlags = READ_LONG();

		if (0 != m_autoDirector->value)
		{
			if ((g_iObserverTarget != m_lastPrimaryObject) || (g_iObserverTarget2 != m_lastSecondaryObject))
				V_ResetChaseCam();

			g_iObserverTarget = m_lastPrimaryObject;
			g_iObserverTarget2 = m_lastSecondaryObject;
			m_IsInterpolating = false;
			m_ChaseEntity = 0;
		}

		// client::Con_Printf("Director Camera: %i %i\n", firstObject, secondObject);
		break;
	case DRC_CMD_MODE:
		if (0 != m_autoDirector->value)
		{
			SetModes(READ_BYTE(), -1);
		}
		break;


	case DRC_CMD_CAMERA:
		v1[0] = READ_COORD(); // position
		v1[1] = READ_COORD();
		v1[2] = READ_COORD(); // vJumpOrigin

		v2[0] = READ_COORD(); // view angle
		v2[1] = READ_COORD(); // vJumpAngles
		v2[2] = READ_COORD();
		f1 = READ_BYTE(); // fov
		i1 = READ_WORD(); // target

		if (0 != m_autoDirector->value)
		{
			SetModes(OBS_ROAMING, -1);
			SetCameraView(v1, v2, f1);
			m_ChaseEntity = i1;
		}
		break;

	case DRC_CMD_MESSAGE:
	{
		client_textmessage_t* msg = &m_HUDMessages[m_lastHudMessage];

		msg->effect = READ_BYTE(); // effect

		UnpackRGB((int&)msg->r1, (int&)msg->g1, (int&)msg->b1, READ_LONG()); // color
		msg->r2 = msg->r1;
		msg->g2 = msg->g1;
		msg->b2 = msg->b1;
		msg->a2 = msg->a1 = 0xFF; // not transparent

		msg->x = READ_FLOAT(); // x pos
		msg->y = READ_FLOAT(); // y pos

		msg->fadein = READ_FLOAT();	  // fadein
		msg->fadeout = READ_FLOAT();  // fadeout
		msg->holdtime = READ_FLOAT(); // holdtime
		msg->fxtime = READ_FLOAT();	  // fxtime;

		strncpy(m_HUDMessageText[m_lastHudMessage], READ_STRING(), 128);
		m_HUDMessageText[m_lastHudMessage][127] = 0; // text

		msg->pMessage = m_HUDMessageText[m_lastHudMessage];
		msg->pName = "HUD_MESSAGE";

		gHUD.m_Message.MessageAdd(msg);

		m_lastHudMessage++;
		m_lastHudMessage %= MAX_SPEC_HUD_MESSAGES;
	}

	break;

	case DRC_CMD_SOUND:
		string = READ_STRING();
		f1 = READ_FLOAT();

		// client::Con_Printf("DRC_CMD_FX_SOUND: %s %.2f\n", string, value );
		client::event::PlaySound(0, v_origin, CHAN_BODY, string, f1, ATTN_NORM, 0, PITCH_NORM);

		break;

	case DRC_CMD_TIMESCALE:
		f1 = READ_FLOAT(); // ignore this command (maybe show slowmo sign)
		break;



	case DRC_CMD_STATUS:
		READ_LONG();					  // total number of spectator slots
		m_iSpectatorNumber = READ_LONG(); // total number of spectator
		READ_WORD();					  // total number of relay proxies

		gViewPort->UpdateSpectatorPanel();
		break;

	case DRC_CMD_BANNER:
		// client::Con_DPrintf("GUI: Banner %s\n",READ_STRING() ); // name of banner tga eg gfx/temp/7454562234563475.tga
		gViewPort->m_pSpectatorPanel->m_TopBanner->LoadImage(READ_STRING());
		gViewPort->UpdateSpectatorPanel();
		break;

	case DRC_CMD_STUFFTEXT:
		client::FilteredClientCmd(READ_STRING());
		break;

	case DRC_CMD_CAMPATH:
		v1[0] = READ_COORD(); // position
		v1[1] = READ_COORD();
		v1[2] = READ_COORD(); // vJumpOrigin

		v2[0] = READ_COORD(); // view angle
		v2[1] = READ_COORD(); // vJumpAngles
		v2[2] = READ_COORD();
		f1 = READ_BYTE(); // FOV
		i1 = READ_BYTE(); // flags

		if (0 != m_autoDirector->value)
		{
			SetModes(OBS_ROAMING, -1);
			SetCameraView(v1, v2, f1);
		}
		break;

	case DRC_CMD_WAYPOINTS:
		i1 = READ_BYTE();
		m_NumWayPoints = 0;
		m_WayPoint = 0;
		for (i2 = 0; i2 < i1; i2++)
		{
			f1 = gHUD.m_flTime + (float)(READ_SHORT()) / 100.0f;

			v1[0] = READ_COORD(); // position
			v1[1] = READ_COORD();
			v1[2] = READ_COORD(); // vJumpOrigin

			v2[0] = READ_COORD(); // view angle
			v2[1] = READ_COORD(); // vJumpAngles
			v2[2] = READ_COORD();
			f2 = READ_BYTE(); // fov
			i3 = READ_BYTE(); // flags

			AddWaypoint(f1, v1, v2, f2, i3);
		}

		// client::Con_Printf("CHudSpectator::DirectorMessage: waypoints %i.\n", m_NumWayPoints );
		if (0 == m_autoDirector->value)
		{
			// ignore waypoints
			m_NumWayPoints = 0;
			break;
		}

		SetModes(OBS_ROAMING, -1);

		m_IsInterpolating = true;

		if (m_NumWayPoints > 2)
		{
			SetWayInterpolation(nullptr, &m_CamPath[0], &m_CamPath[1], &m_CamPath[2]);
		}
		else
		{
			SetWayInterpolation(nullptr, &m_CamPath[0], &m_CamPath[1], nullptr);
		}
		break;

	default:
		client::Con_DPrintf("CHudSpectator::DirectorMessage: unknown command %i.\n", cmd);
	}
}

void CHudSpectator::FindNextPlayer(bool bReverse)
{
	// MOD AUTHORS: Modify the logic of this function if you want to restrict the observer to watching
	//				only a subset of the players. e.g. Make it check the target's team.

	int iStart;
	cl_entity_t* pEnt = nullptr;

	// if we are NOT in HLTV mode, spectator targets are set on server
	if (0 == client::IsSpectateOnly())
	{
		char cmdstring[32];
		// forward command to server
		sprintf(cmdstring, "follownext %i", bReverse ? 1 : 0);
		client::ServerCmd(cmdstring);
		return;
	}

	if (0 != g_iObserverTarget)
		iStart = g_iObserverTarget;
	else
		iStart = 1;

	g_iObserverTarget = 0;

	int iCurrent = iStart;

	int iDir = bReverse ? -1 : 1;

	// make sure we have player info
	gViewPort->GetAllPlayersInfo();


	do
	{
		iCurrent += iDir;

		// Loop through the clients
		if (iCurrent > MAX_PLAYERS_HUD)
			iCurrent = 1;
		if (iCurrent < 1)
			iCurrent = MAX_PLAYERS_HUD;

		pEnt = client::GetEntityByIndex(iCurrent);

		if (!IsActivePlayer(pEnt))
			continue;

		// MOD AUTHORS: Add checks on target here.

		g_iObserverTarget = iCurrent;
		break;

	} while (iCurrent != iStart);

	// Did we find a target?
	if (0 == g_iObserverTarget)
	{
		client::Con_DPrintf("No observer targets.\n");
		// take save camera position
		vJumpOrigin = m_cameraOrigin;
		vJumpAngles = m_cameraAngles;
	}
	else
	{
		// use new entity position for roaming
		vJumpOrigin = pEnt->origin;
		vJumpAngles = pEnt->angles;
	}

	iJumpSpectator = true;
	gViewPort->MsgFunc_ResetFade(nullptr, 0, nullptr);
}


void CHudSpectator::FindPlayer(const char* name)
{
	// MOD AUTHORS: Modify the logic of this function if you want to restrict the observer to watching
	//				only a subset of the players. e.g. Make it check the target's team.

	// if we are NOT in HLTV mode, spectator targets are set on server
	if (0 == client::IsSpectateOnly())
	{
		char cmdstring[32];
		// forward command to server
		sprintf(cmdstring, "follow %s", name);
		client::ServerCmd(cmdstring);
		return;
	}

	g_iObserverTarget = 0;

	// make sure we have player info
	gViewPort->GetAllPlayersInfo();

	cl_entity_t* pEnt = nullptr;

	for (int i = 1; i < MAX_PLAYERS_HUD; i++)
	{

		pEnt = client::GetEntityByIndex(i);

		if (!IsActivePlayer(pEnt))
			continue;

		if (!stricmp(g_PlayerInfoList[pEnt->index].name, name))
		{
			g_iObserverTarget = i;
			break;
		}
	}

	// Did we find a target?
	if (0 == g_iObserverTarget)
	{
		client::Con_DPrintf("No observer targets.\n");
		// take save camera position
		vJumpOrigin = m_cameraOrigin;
		vJumpAngles = m_cameraAngles;
	}
	else
	{
		// use new entity position for roaming
		vJumpOrigin = pEnt->origin;
		vJumpAngles = pEnt->angles;
	}

	iJumpSpectator = true;
	gViewPort->MsgFunc_ResetFade(nullptr, 0, nullptr);
}

void CHudSpectator::HandleButtonsDown(int ButtonPressed)
{
	double time = client::GetClientTime();

	int newMainMode = g_iObserverMode;
	int newInsetMode = m_pip->value;

	// client::Con_Printf(" HandleButtons:%i\n", ButtonPressed );

	if (!gViewPort)
		return;

	//Not in intermission.
	if (gHUD.m_iIntermission)
		return;

	if (!gHUD.IsSpectator())
		return; // dont do anything if not in spectator mode

	// don't handle buttons during normal demo playback
	if (0 != client::demo::IsPlayingback() && 0 == client::IsSpectateOnly())
		return;
	// Slow down mouse clicks.
	if (m_flNextObserverInput > time)
		return;

	// enable spectator screen
	if ((ButtonPressed & IN_DUCK) != 0)
		gViewPort->m_pSpectatorPanel->ShowMenu(!gViewPort->m_pSpectatorPanel->m_menuVisible);

	//  'Use' changes inset window mode
	if ((ButtonPressed & IN_USE) != 0)
	{
		newInsetMode = ToggleInset(true);
	}

	// if not in HLTV mode, buttons are handled server side
	if (0 != client::IsSpectateOnly())
	{
		// changing target or chase mode not in overviewmode without inset window

		// Jump changes main window modes
		if ((ButtonPressed & IN_JUMP) != 0)
		{
			newMainMode = (newMainMode + 1) % OBS_DEATHCAM;
		}

		// Attack moves to the next player
		if ((ButtonPressed & (IN_ATTACK | IN_ATTACK2)) != 0)
		{
			FindNextPlayer((ButtonPressed & IN_ATTACK2) != 0);

			if (g_iObserverMode == OBS_ROAMING)
			{
				client::SetViewAngles(vJumpAngles);
				iJumpSpectator = true;
			}
			// release directed mode if player wants to see another player
			m_autoDirector->value = 0.0f;
		}
	}

	SetModes(newMainMode, newInsetMode);

	if (g_iObserverMode == OBS_MAP_FREE)
	{
		if ((ButtonPressed & IN_FORWARD) != 0)
			m_zoomDelta = 0.01f;

		if ((ButtonPressed & IN_BACK) != 0)
			m_zoomDelta = -0.01f;

		if ((ButtonPressed & IN_MOVELEFT) != 0)
			m_moveDelta = -12.0f;

		if ((ButtonPressed & IN_MOVERIGHT) != 0)
			m_moveDelta = 12.0f;
	}

	m_flNextObserverInput = time + 0.2;
}

void CHudSpectator::HandleButtonsUp(int ButtonPressed)
{
	if (!gViewPort)
		return;

	if (!gViewPort->m_pSpectatorPanel->isVisible())
		return; // dont do anything if not in spectator mode

	if ((ButtonPressed & (IN_FORWARD | IN_BACK)) != 0)
		m_zoomDelta = 0.0f;

	if ((ButtonPressed & (IN_MOVELEFT | IN_MOVERIGHT)) != 0)
		m_moveDelta = 0.0f;
}

void CHudSpectator::SetModes(int iNewMainMode, int iNewInsetMode)
{
	// if value == -1 keep old value
	if (iNewMainMode == -1)
		iNewMainMode = g_iObserverMode;

	if (iNewInsetMode == -1)
		iNewInsetMode = m_pip->value;

	// inset mode is handled only clients side
	m_pip->value = iNewInsetMode;

	if (iNewMainMode < OBS_CHASE_FREE || iNewMainMode > OBS_MAP_CHASE)
	{
		client::Con_Printf("Invalid spectator mode.\n");
		return;
	}

	m_IsInterpolating = false;
	m_ChaseEntity = 0;

	// main mode settings will override inset window settings
	if (iNewMainMode != g_iObserverMode)
	{
		// if we are NOT in HLTV mode, main spectator mode is set on server
		if (0 == client::IsSpectateOnly())
		{
			char cmdstring[32];
			// forward command to server
			sprintf(cmdstring, "specmode %i", iNewMainMode);
			client::ServerCmd(cmdstring);
			return;
		}

		if (0 == g_iObserverTarget && (iNewMainMode != OBS_ROAMING)) // make sure we have a target
		{
			// choose last Director object if still available
			if (IsActivePlayer(client::GetEntityByIndex(m_lastPrimaryObject)))
			{
				g_iObserverTarget = m_lastPrimaryObject;
				g_iObserverTarget2 = m_lastSecondaryObject;
			}
			else
				FindNextPlayer(false); // find any target
		}

		switch (iNewMainMode)
		{
		case OBS_CHASE_FREE:
			g_iObserverMode = OBS_CHASE_FREE;
			break;

		case OBS_ROAMING: // jump to current vJumpOrigin/angle
			g_iObserverMode = OBS_ROAMING;
			if (0 != g_iObserverTarget)
			{
				V_GetChasePos(g_iObserverTarget, v_cl_angles, vJumpOrigin, vJumpAngles);
				client::SetViewAngles(vJumpAngles);
				iJumpSpectator = true;
			}
			break;

		case OBS_MAP_FREE:
			g_iObserverMode = OBS_MAP_FREE;
			// reset user values
			m_mapZoom = m_OverviewData.zoom;
			m_mapOrigin = m_OverviewData.origin;
			break;

		case OBS_MAP_CHASE:
			g_iObserverMode = OBS_MAP_CHASE;
			// reset user values
			m_mapZoom = m_OverviewData.zoom;
			m_mapOrigin = m_OverviewData.origin;
			break;
		}

		gViewPort->MsgFunc_ResetFade(nullptr, 0, nullptr);

		char string[128];
		strcpy(string, GetSpectatorLabel(g_iObserverMode));
		sprintf(string, "%c%s", HUD_PRINTCENTER, CHudTextMessage::BufferedLocaliseTextString(string));
		gHUD.m_TextMessage.MsgFunc_TextMsg(nullptr, strlen(string) + 1, string);
	}

	gViewPort->UpdateSpectatorPanel();
}

bool CHudSpectator::IsActivePlayer(cl_entity_t* ent)
{
	return (nullptr != ent &&
			0 != ent->player &&
			ent->curstate.solid != SOLID_NOT &&
			ent != client::GetLocalPlayer() &&
			g_PlayerInfoList[ent->index].name != nullptr);
}


bool CHudSpectator::ParseOverviewFile()
{
	char filename[255];
	char levelname[255];
	char token[1024];
	float height;

	char* pfile = nullptr;

	memset(&m_OverviewData, 0, sizeof(m_OverviewData));

	// fill in standrd values
	m_OverviewData.insetWindowX = 4; // upper left corner
	m_OverviewData.insetWindowY = 4;
	m_OverviewData.insetWindowHeight = 180;
	m_OverviewData.insetWindowWidth = 240;
	m_OverviewData.origin[0] = 0.0f;
	m_OverviewData.origin[1] = 0.0f;
	m_OverviewData.origin[2] = 0.0f;
	m_OverviewData.zoom = 1.0f;
	m_OverviewData.layers = 0;
	m_OverviewData.layersHeights[0] = 0.0f;
	strcpy(m_OverviewData.map, client::GetLevelName());

	if (strlen(m_OverviewData.map) == 0)
		return false; // not active yet

	strcpy(levelname, m_OverviewData.map + 5);
	levelname[strlen(levelname) - 4] = 0;

	sprintf(filename, "overviews/%s.txt", levelname);

	pfile = (char*)client::COM_LoadFile(filename, 5, nullptr);

	if (!pfile)
	{
		client::Con_DPrintf("Couldn't open file %s. Using default values for overiew mode.\n", filename);
		return false;
	}


	while (true)
	{
		pfile = client::COM_ParseFile(pfile, token);

		if (!pfile)
			break;

		if (!stricmp(token, "global"))
		{
			// parse the global data
			pfile = client::COM_ParseFile(pfile, token);
			if (stricmp(token, "{"))
			{
				client::Con_Printf("Error parsing overview file %s. (expected { )\n", filename);
				return false;
			}

			pfile = client::COM_ParseFile(pfile, token);

			while (stricmp(token, "}"))
			{
				if (!stricmp(token, "zoom"))
				{
					pfile = client::COM_ParseFile(pfile, token);
					m_OverviewData.zoom = atof(token);
				}
				else if (!stricmp(token, "origin"))
				{
					pfile = client::COM_ParseFile(pfile, token);
					m_OverviewData.origin[0] = atof(token);
					pfile = client::COM_ParseFile(pfile, token);
					m_OverviewData.origin[1] = atof(token);
					pfile = client::COM_ParseFile(pfile, token);
					m_OverviewData.origin[2] = atof(token);
				}
				else if (!stricmp(token, "rotated"))
				{
					pfile = client::COM_ParseFile(pfile, token);
					m_OverviewData.rotated = atoi(token) != 0;
				}
				else if (!stricmp(token, "inset"))
				{
					pfile = client::COM_ParseFile(pfile, token);
					m_OverviewData.insetWindowX = atof(token);
					pfile = client::COM_ParseFile(pfile, token);
					m_OverviewData.insetWindowY = atof(token);
					pfile = client::COM_ParseFile(pfile, token);
					m_OverviewData.insetWindowWidth = atof(token);
					pfile = client::COM_ParseFile(pfile, token);
					m_OverviewData.insetWindowHeight = atof(token);
				}
				else
				{
					client::Con_Printf("Error parsing overview file %s. (%s unkown)\n", filename, token);
					return false;
				}

				pfile = client::COM_ParseFile(pfile, token); // parse next token
			}
		}
		else if (!stricmp(token, "layer"))
		{
			// parse a layer data

			if (m_OverviewData.layers == OVERVIEW_MAX_LAYERS)
			{
				client::Con_Printf("Error parsing overview file %s. ( too many layers )\n", filename);
				return false;
			}

			pfile = client::COM_ParseFile(pfile, token);


			if (stricmp(token, "{"))
			{
				client::Con_Printf("Error parsing overview file %s. (expected { )\n", filename);
				return false;
			}

			pfile = client::COM_ParseFile(pfile, token);

			while (stricmp(token, "}"))
			{
				if (!stricmp(token, "image"))
				{
					pfile = client::COM_ParseFile(pfile, token);
					strcpy(m_OverviewData.layersImages[m_OverviewData.layers], token);
				}
				else if (!stricmp(token, "height"))
				{
					pfile = client::COM_ParseFile(pfile, token);
					height = atof(token);
					m_OverviewData.layersHeights[m_OverviewData.layers] = height;
				}
				else
				{
					client::Con_Printf("Error parsing overview file %s. (%s unkown)\n", filename, token);
					return false;
				}

				pfile = client::COM_ParseFile(pfile, token); // parse next token
			}

			m_OverviewData.layers++;
		}
	}

	client::COM_FreeFile(pfile);

	m_mapZoom = m_OverviewData.zoom;
	m_mapOrigin = m_OverviewData.origin;

	return true;
}

void CHudSpectator::LoadMapSprites()
{
	// right now only support for one map layer
	if (m_OverviewData.layers > 0)
	{
		m_MapSprite = client::LoadMapSprite(m_OverviewData.layersImages[0]);
	}
	else
		m_MapSprite = nullptr; // the standard "unkown map" sprite will be used instead
}

void CHudSpectator::DrawOverviewLayer()
{
	float screenaspect, xs, ys, xStep, yStep, x, y, z;
	int ix, iy, i, xTiles, yTiles, frame;

	bool hasMapImage = nullptr != m_MapSprite;
	model_t* dummySprite = (struct model_s*)client::GetSpritePointer(m_hsprUnkownMap);

	if (hasMapImage)
	{
		i = m_MapSprite->numframes / (4 * 3);
		i = sqrt((float)i);
		xTiles = i * 4;
		yTiles = i * 3;
	}
	else
	{
		xTiles = 8;
		yTiles = 6;
	}


	screenaspect = 4.0f / 3.0f;


	xs = m_OverviewData.origin[0];
	ys = m_OverviewData.origin[1];
	z = (90.0f - v_angles[0]) / 90.0f;
	z *= m_OverviewData.layersHeights[0]; // gOverviewData.z_min - 32;

	// i = r_overviewTexture + ( layer*OVERVIEW_X_TILES*OVERVIEW_Y_TILES );

	client::tri::RenderMode(kRenderTransTexture);
	client::tri::CullFace(TRI_NONE);
	client::tri::Color4f(1.0, 1.0, 1.0, 1.0);

	frame = 0;


	// rotated view ?
	if (m_OverviewData.rotated)
	{
		xStep = (2 * 4096.0f / m_OverviewData.zoom) / xTiles;
		yStep = -(2 * 4096.0f / (m_OverviewData.zoom * screenaspect)) / yTiles;

		y = ys + (4096.0f / (m_OverviewData.zoom * screenaspect));

		for (iy = 0; iy < yTiles; iy++)
		{
			x = xs - (4096.0f / (m_OverviewData.zoom));

			for (ix = 0; ix < xTiles; ix++)
			{
				if (hasMapImage)
					client::tri::SpriteTexture(m_MapSprite, frame);
				else
					client::tri::SpriteTexture(dummySprite, 0);

				client::tri::Begin(TRI_QUADS);
				client::tri::TexCoord2f(0, 0);
				client::tri::Vertex3f(x, y, z);

				client::tri::TexCoord2f(1, 0);
				client::tri::Vertex3f(x + xStep, y, z);

				client::tri::TexCoord2f(1, 1);
				client::tri::Vertex3f(x + xStep, y + yStep, z);

				client::tri::TexCoord2f(0, 1);
				client::tri::Vertex3f(x, y + yStep, z);
				client::tri::End();

				frame++;
				x += xStep;
			}

			y += yStep;
		}
	}
	else
	{
		xStep = -(2 * 4096.0f / m_OverviewData.zoom) / xTiles;
		yStep = -(2 * 4096.0f / (m_OverviewData.zoom * screenaspect)) / yTiles;


		x = xs + (4096.0f / (m_OverviewData.zoom * screenaspect));



		for (ix = 0; ix < yTiles; ix++)
		{

			y = ys + (4096.0f / (m_OverviewData.zoom));

			for (iy = 0; iy < xTiles; iy++)
			{
				if (hasMapImage)
					client::tri::SpriteTexture(m_MapSprite, frame);
				else
					client::tri::SpriteTexture(dummySprite, 0);

				client::tri::Begin(TRI_QUADS);
				client::tri::TexCoord2f(0, 0);
				client::tri::Vertex3f(x, y, z);

				client::tri::TexCoord2f(0, 1);
				client::tri::Vertex3f(x + xStep, y, z);

				client::tri::TexCoord2f(1, 1);
				client::tri::Vertex3f(x + xStep, y + yStep, z);

				client::tri::TexCoord2f(1, 0);
				client::tri::Vertex3f(x, y + yStep, z);
				client::tri::End();

				frame++;

				y += yStep;
			}

			x += xStep;
		}
	}
}

void CHudSpectator::DrawOverviewEntities()
{
	int i, ir, ig, ib;
	struct model_s* hSpriteModel;
	Vector origin, angles, point, forward, right, left, up, world, screen, offset;
	float x, y, z, r, g, b, sizeScale = 4.0f;
	cl_entity_t* ent;
	float rmatrix[3][4]; // transformation matrix

	float zScale = (90.0f - v_angles[0]) / 90.0f;


	z = m_OverviewData.layersHeights[0] * zScale;
	// get yellow/brown HUD color
	UnpackRGB(ir, ig, ib, RGB_YELLOWISH);
	r = (float)ir / 255.0f;
	g = (float)ig / 255.0f;
	b = (float)ib / 255.0f;

	client::tri::CullFace(TRI_NONE);

	for (i = 0; i < MAX_PLAYERS_HUD; i++)
		m_vPlayerPos[i][2] = -1; // mark as invisible

	// draw all players
	for (i = 0; i < MAX_OVERVIEW_ENTITIES; i++)
	{
		if (0 == m_OverviewEntities[i].hSprite)
			continue;

		hSpriteModel = (struct model_s*)client::GetSpritePointer(m_OverviewEntities[i].hSprite);
		ent = m_OverviewEntities[i].entity;

		client::tri::SpriteTexture(hSpriteModel, 0);
		client::tri::RenderMode(kRenderTransTexture);

		// see R_DrawSpriteModel
		// draws players sprite

		AngleVectors(ent->angles, &right, &up, nullptr);

		origin = ent->origin;

		client::tri::Begin(TRI_QUADS);

		client::tri::Color4f(1.0, 1.0, 1.0, 1.0);

		client::tri::TexCoord2f(1, 0);
		point = origin + 16.0f * sizeScale * up;
		point = point + 16.0f * sizeScale * right;
		point[2] *= zScale;
		client::tri::Vertex3fv(point);

		client::tri::TexCoord2f(0, 0);

		point = origin + 16.0f * sizeScale * up;
		point = point + -16.0f * sizeScale * right;
		point[2] *= zScale;
		client::tri::Vertex3fv(point);

		client::tri::TexCoord2f(0, 1);
		point = origin + -16.0f * sizeScale * up;
		point = point + -16.0f * sizeScale * right;
		point[2] *= zScale;
		client::tri::Vertex3fv(point);

		client::tri::TexCoord2f(1, 1);
		point = origin + -16.0f * sizeScale * up;
		point = point + 16.0f * sizeScale * right;
		point[2] *= zScale;
		client::tri::Vertex3fv(point);

		client::tri::End();


		if (0 == ent->player)
			continue;
		// draw line under player icons
		origin[2] *= zScale;

		client::tri::RenderMode(kRenderTransAdd);

		hSpriteModel = (struct model_s*)client::GetSpritePointer(m_hsprBeam);
		client::tri::SpriteTexture(hSpriteModel, 0);

		client::tri::Color4f(r, g, b, 0.3);

		client::tri::Begin(TRI_QUADS);
		client::tri::TexCoord2f(1, 0);
		client::tri::Vertex3f(origin[0] + 4, origin[1] + 4, origin[2] - zScale);
		client::tri::TexCoord2f(0, 0);
		client::tri::Vertex3f(origin[0] - 4, origin[1] - 4, origin[2] - zScale);
		client::tri::TexCoord2f(0, 1);
		client::tri::Vertex3f(origin[0] - 4, origin[1] - 4, z);
		client::tri::TexCoord2f(1, 1);
		client::tri::Vertex3f(origin[0] + 4, origin[1] + 4, z);
		client::tri::End();

		client::tri::Begin(TRI_QUADS);
		client::tri::TexCoord2f(1, 0);
		client::tri::Vertex3f(origin[0] - 4, origin[1] + 4, origin[2] - zScale);
		client::tri::TexCoord2f(0, 0);
		client::tri::Vertex3f(origin[0] + 4, origin[1] - 4, origin[2] - zScale);
		client::tri::TexCoord2f(0, 1);
		client::tri::Vertex3f(origin[0] + 4, origin[1] - 4, z);
		client::tri::TexCoord2f(1, 1);
		client::tri::Vertex3f(origin[0] - 4, origin[1] + 4, z);
		client::tri::End();

		// calculate screen position for name and infromation in hud::draw()
		if (0 != client::tri::WorldToScreen(origin, screen))
			continue; // object is behind viewer

		screen[0] = XPROJECT(screen[0]);
		screen[1] = YPROJECT(screen[1]);
		screen[2] = 0.0f;

		// calculate some offset under the icon
		origin[0] += 32.0f;
		origin[1] += 32.0f;

		client::tri::WorldToScreen(origin, offset);

		offset[0] = XPROJECT(offset[0]);
		offset[1] = YPROJECT(offset[1]);
		offset[2] = 0.0f;

		offset = offset - screen;

		int playerNum = ent->index - 1;

		m_vPlayerPos[playerNum][0] = screen[0];
		m_vPlayerPos[playerNum][1] = screen[1] + offset.Length();
		m_vPlayerPos[playerNum][2] = 1; // mark player as visible
	}

	if (0 == m_pip->value || 0 == m_drawcone->value)
		return;

	// get current camera position and angle

	if (m_pip->value == INSET_CHASE_FREE || g_iObserverMode == OBS_CHASE_FREE)
	{
		V_GetChasePos(g_iObserverTarget, v_cl_angles, origin, angles);
	}
	else if (g_iObserverMode == OBS_ROAMING)
	{
		origin = v_sim_org;
		angles = v_cl_angles;
	}
	else
		V_GetChasePos(g_iObserverTarget, nullptr, origin, angles);


	// draw camera sprite

	x = origin[0];
	y = origin[1];
	z = origin[2];

	angles[0] = 0; // always show horizontal camera sprite

	hSpriteModel = (struct model_s*)client::GetSpritePointer(m_hsprCamera);
	client::tri::RenderMode(kRenderTransAdd);
	client::tri::SpriteTexture(hSpriteModel, 0);


	client::tri::Color4f(r, g, b, 1.0);

	AngleVectors(angles, &forward, nullptr, nullptr);
	forward = forward * 512.0F;

	offset[0] = 0.0f;
	offset[1] = 45.0f;
	offset[2] = 0.0f;

	AngleMatrix(offset, rmatrix);
	VectorTransform(forward, rmatrix, right);

	offset[1] = -45.0f;
	AngleMatrix(offset, rmatrix);
	VectorTransform(forward, rmatrix, left);

	client::tri::Begin(TRI_TRIANGLES);
	client::tri::TexCoord2f(0, 0);
	client::tri::Vertex3f(x + right[0], y + right[1], (z + right[2]) * zScale);

	client::tri::TexCoord2f(0, 1);
	client::tri::Vertex3f(x, y, z * zScale);

	client::tri::TexCoord2f(1, 1);
	client::tri::Vertex3f(x + left[0], y + left[1], (z + left[2]) * zScale);
	client::tri::End();
}



void CHudSpectator::DrawOverview()
{
	// draw only in sepctator mode
	if (!gHUD.IsSpectator())
		return;

	// Only draw the overview if Map Mode is selected for this view
	if (m_iDrawCycle == 0 && ((g_iObserverMode != OBS_MAP_FREE) && (g_iObserverMode != OBS_MAP_CHASE)))
		return;

	if (m_iDrawCycle == 1 && m_pip->value < INSET_MAP_FREE)
		return;

	DrawOverviewLayer();
	DrawOverviewEntities();
	CheckOverviewEntities();
}
void CHudSpectator::CheckOverviewEntities()
{
	double time = client::GetClientTime();

	// removes old entities from list
	for (int i = 0; i < MAX_OVERVIEW_ENTITIES; i++)
	{
		// remove entity from list if it is too old
		if (m_OverviewEntities[i].killTime < time)
		{
			memset(&m_OverviewEntities[i], 0, sizeof(overviewEntity_t));
		}
	}
}

bool CHudSpectator::AddOverviewEntity(int type, struct cl_entity_s* ent, const char* modelname)
{
	HSPRITE hSprite = 0;
	double duration = -1.0f; // duration -1 means show it only this frame;

	if (!ent)
		return false;

	if (type == ET_PLAYER)
	{
		if (ent->curstate.solid != SOLID_NOT)
		{
			switch (g_PlayerExtraInfo[ent->index].teamnumber)
			{
			// blue and red teams are swapped in CS and TFC
			case 1:
				hSprite = m_hsprPlayerBlue;
				break;
			case 2:
				hSprite = m_hsprPlayerRed;
				break;
			default:
				hSprite = m_hsprPlayer;
				break;
			}
		}
		else
			return false; // it's an spectator
	}
	else if (type == ET_NORMAL)
	{
		return false;
	}
	else
		return false;

	return AddOverviewEntityToList(hSprite, ent, client::GetClientTime() + duration);
}

void CHudSpectator::DeathMessage(int victim)
{
	// find out where the victim is
	cl_entity_t* pl = client::GetEntityByIndex(victim);

	if (pl && 0 != pl->player)
		AddOverviewEntityToList(m_hsprPlayerDead, pl, client::GetClientTime() + 2.0f);
}

bool CHudSpectator::AddOverviewEntityToList(HSPRITE sprite, cl_entity_t* ent, double killTime)
{
	for (int i = 0; i < MAX_OVERVIEW_ENTITIES; i++)
	{
		// find empty entity slot
		if (m_OverviewEntities[i].entity == nullptr)
		{
			m_OverviewEntities[i].entity = ent;
			m_OverviewEntities[i].hSprite = sprite;
			m_OverviewEntities[i].killTime = killTime;
			return true;
		}
	}

	return false; // maximum overview entities reached
}
void CHudSpectator::CheckSettings()
{
	// disallow same inset mode as main mode:

	m_pip->value = (int)m_pip->value;

	if ((g_iObserverMode < OBS_MAP_FREE) && m_pip->value == INSET_CHASE_FREE)
	{
		// otherwise both would show in World picures
		m_pip->value = INSET_MAP_FREE;
	}

	if ((g_iObserverMode >= OBS_MAP_FREE) && (m_pip->value >= INSET_MAP_FREE))
	{
		// both would show map views
		m_pip->value = INSET_CHASE_FREE;
	}

	// disble in intermission screen
	if (gHUD.m_iIntermission)
		m_pip->value = INSET_OFF;

	// check chat mode
	if (m_chatEnabled != (gHUD.m_SayText.m_HUD_saytext->value != 0))
	{
		// hud_saytext changed
		m_chatEnabled = (gHUD.m_SayText.m_HUD_saytext->value != 0);

		if (0 != client::IsSpectateOnly())
		{
			// tell proxy our new chat mode
			char chatcmd[32];
			sprintf(chatcmd, "ignoremsg %i", m_chatEnabled ? 0 : 1);
			client::ServerCmd(chatcmd);
		}
	}

	// draw small border around inset view, adjust upper black bar
	gViewPort->m_pSpectatorPanel->EnableInsetView(m_pip->value != INSET_OFF);
}

int CHudSpectator::ToggleInset(bool allowOff)
{
	int newInsetMode = (int)m_pip->value + 1;

	if (g_iObserverMode < OBS_MAP_FREE)
	{
		if (newInsetMode > INSET_MAP_CHASE)
		{
			if (allowOff)
				newInsetMode = INSET_OFF;
			else
				newInsetMode = INSET_MAP_FREE;
		}

		if (newInsetMode == INSET_CHASE_FREE)
			newInsetMode = INSET_MAP_FREE;
	}
	else
	{
		if (newInsetMode > INSET_CHASE_FREE)
		{
			if (allowOff)
				newInsetMode = INSET_OFF;
			else
				newInsetMode = INSET_CHASE_FREE;
		}
	}

	return newInsetMode;
}
void CHudSpectator::Reset()
{
	m_lastPrimaryObject = m_lastSecondaryObject = 0;
	m_flNextObserverInput = 0.0f;
	m_lastHudMessage = 0;
	m_iSpectatorNumber = 0;
	iJumpSpectator = false;
	g_iObserverMode = OBS_NONE;
	g_iObserverTarget = g_iObserverTarget2 = 0;

	memset(&m_OverviewData, 0, sizeof(m_OverviewData));
	memset(&m_OverviewEntities, 0, sizeof(m_OverviewEntities));

	if (0 != client::IsSpectateOnly() || 0 != client::demo::IsPlayingback())
		m_autoDirector->value = 1.0f;
	else
		m_autoDirector->value = 0.0f;

	// Reset HUD
	if (0 != strcmp(m_OverviewData.map, client::GetLevelName()))
	{
		// update level overview if level changed
		ParseOverviewFile();
		LoadMapSprites();
	}

	memset(&m_OverviewEntities, 0, sizeof(m_OverviewEntities));

	m_FOV = 90.0f;

	m_IsInterpolating = false;

	m_ChaseEntity = 0;

	SetSpectatorStartPosition();

	SetModes(OBS_CHASE_FREE, INSET_OFF);

	g_iObserverTarget = 0; // fake not target until first camera command

	// reset HUD FOV
	gHUD.Update_SetFOV(0);
}
