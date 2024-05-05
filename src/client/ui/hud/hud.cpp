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
//
// hud.cpp
//
// implementation of CHud class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include <algorithm>
#include "parsemsg.h"
#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"

#include "demo.h"
#include "demo_api.h"
#include "vgui_ScorePanel.h"

#include "r_studioint.h"
extern engine_studio_api_t IEngineStudio;

hud_player_info_t g_PlayerInfoList[MAX_PLAYERS_HUD + 1];	// player info from the engine
extra_player_info_t g_PlayerExtraInfo[MAX_PLAYERS_HUD + 1]; // additional player info sent directly to the client dll

extern int g_iObserverMode;
extern int g_iObserverTarget;
extern int g_iObserverTarget2;

bool CHudBase::IsActive()
{
	return (m_iFlags & kActive) != 0;
}

bool CHudBase::Init()
{
	gHUD.AddHudElem(this);
	Reset();
	return true;
}

void CHudBase::Think()
{
	m_fFade = std::max(m_fFade - static_cast<float>(gHUD.m_flTimeDelta) * 20.0F, 0.0F);
}

bool CHudStatus::IsActive()
{
	return gHUD.IsAlive()
		&& !gHUD.IsObserver()
		&& !gHUD.IsSpectator();
}

class CHLVoiceStatusHelper : public IVoiceStatusHelper
{
public:
	void GetPlayerTextColor(int entindex, int color[3]) override
	{
		color[0] = color[1] = color[2] = 255;

		if (entindex >= 0 && entindex < sizeof(g_PlayerExtraInfo) / sizeof(g_PlayerExtraInfo[0]))
		{
			auto c = gHUD.GetClientColor(entindex);

			color[0] = c[0] * 255;
			color[1] = c[1] * 255;
			color[2] = c[2] * 255;
		}
	}

	void UpdateCursorState() override
	{
		gViewPort->UpdateCursorState();
	}

	int GetAckIconHeight() override
	{
		return ScreenHeight - gHUD.m_iFontHeight * 3 - 6;
	}

	bool CanShowSpeakerLabels() override
	{
		if (gViewPort && gViewPort->m_pScoreBoard)
			return !gViewPort->m_pScoreBoard->isVisible();
		else
			return false;
	}
};
static CHLVoiceStatusHelper g_VoiceStatusHelper;


extern client_sprite_t* GetSpriteList(client_sprite_t* pList, const char* psz, int iRes, int iCount);

extern cvar_t* sensitivity;
cvar_t* cl_rollangle = nullptr;
cvar_t* cl_rollspeed = nullptr;
cvar_t* cl_bobtilt = nullptr;
cvar_t* r_decals = nullptr;
cvar_t* r_explosionstyle = nullptr;
cvar_t* violence_hblood = nullptr;
cvar_t* violence_hgibs = nullptr;
cvar_t* cl_autowepswitch = nullptr;
cvar_t* cl_grenadetoggle = nullptr;
cvar_t* cl_righthand = nullptr;

void ShutdownInput();

int __MsgFunc_ResetHUD(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_ResetHUD(pszName, iSize, pbuf));
}

int __MsgFunc_InitHUD(const char* pszName, int iSize, void* pbuf)
{
	gHUD.MsgFunc_InitHUD(pszName, iSize, pbuf);
	return 1;
}

int __MsgFunc_ViewMode(const char* pszName, int iSize, void* pbuf)
{
	gHUD.MsgFunc_ViewMode(pszName, iSize, pbuf);
	return 1;
}

int __MsgFunc_Concuss(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_Concuss(pszName, iSize, pbuf));
}

int __MsgFunc_Weapons(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_Weapons(pszName, iSize, pbuf));
}

int __MsgFunc_Ammo(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_Ammo(pszName, iSize, pbuf));
}

int __MsgFunc_GameMode(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_GameMode(pszName, iSize, pbuf));
}

// TFFree Command Menu
void __CmdFunc_OpenCommandMenu()
{
	if (gViewPort)
	{
		gViewPort->ShowCommandMenu(gViewPort->m_StandardMenu);
	}
}

// TFC "special" command
void __CmdFunc_InputPlayerSpecial()
{
	if (gViewPort)
	{
		gViewPort->InputPlayerSpecial();
	}
}

void __CmdFunc_CloseCommandMenu()
{
	if (gViewPort)
	{
		gViewPort->InputSignalHideCommandMenu();
	}
}

void __CmdFunc_ForceCloseCommandMenu()
{
	if (gViewPort)
	{
		gViewPort->HideCommandMenu();
	}
}

// TFFree Command Menu Message Handlers
int __MsgFunc_ValClass(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_ValClass(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_TeamNames(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_TeamNames(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_Feign(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_Feign(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_Detpack(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_Detpack(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_VGUIMenu(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_VGUIMenu(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_MOTD(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_MOTD(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_BuildSt(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_BuildSt(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_RandomPC(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_RandomPC(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_ServerName(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_ServerName(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_ScoreInfo(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_ScoreInfo(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_ExtraInfo(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_ExtraInfo(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_TeamScore(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_TeamScore(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_SpecFade(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_SpecFade(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_ResetFade(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_ResetFade(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_AllowSpec(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_AllowSpec(pszName, iSize, pbuf));
	return 0;
}

// This is called every time the DLL is loaded
void CHud::Init()
{
	HOOK_MESSAGE(ResetHUD);
	HOOK_MESSAGE(GameMode);
	HOOK_MESSAGE(InitHUD);
	HOOK_MESSAGE(ViewMode);
	HOOK_MESSAGE(Concuss);
	HOOK_MESSAGE(Weapons);
	HOOK_MESSAGE(Ammo);

	// TFFree CommandMenu
	HOOK_COMMAND("+commandmenu", OpenCommandMenu);
	HOOK_COMMAND("-commandmenu", CloseCommandMenu);
	HOOK_COMMAND("ForceCloseCommandMenu", ForceCloseCommandMenu);
	HOOK_COMMAND("special", InputPlayerSpecial);

	HOOK_MESSAGE(ValClass);
	HOOK_MESSAGE(TeamNames);
	HOOK_MESSAGE(Feign);
	HOOK_MESSAGE(Detpack);
	HOOK_MESSAGE(MOTD);
	HOOK_MESSAGE(BuildSt);
	HOOK_MESSAGE(RandomPC);
	HOOK_MESSAGE(ServerName);
	HOOK_MESSAGE(ScoreInfo);
	HOOK_MESSAGE(ExtraInfo);
	HOOK_MESSAGE(TeamScore);

	HOOK_MESSAGE(AllowSpec);

	HOOK_MESSAGE(SpecFade);
	HOOK_MESSAGE(ResetFade);

	// VGUI Menus
	HOOK_MESSAGE(VGUIMenu);

	CVAR_CREATE("hud_classautokill", "1", FCVAR_ARCHIVE | FCVAR_USERINFO); // controls whether or not to suicide immediately on TF class switch
	CVAR_CREATE("hud_takesshots", "0", FCVAR_ARCHIVE);					   // controls whether or not to automatically take screenshots at the end of a round

	m_cColors[CHud::COLOR_DEFAULT].r = 255;
	m_cColors[CHud::COLOR_DEFAULT].g = 255;
	m_cColors[CHud::COLOR_DEFAULT].b = 255;
	m_cColors[CHud::COLOR_DEFAULT].a = 255;

	m_cColors[CHud::COLOR_PRIMARY].r = 255;
	m_cColors[CHud::COLOR_PRIMARY].g = 160;
	m_cColors[CHud::COLOR_PRIMARY].b = 0;
	m_cColors[CHud::COLOR_PRIMARY].a = 255;

	m_cColors[CHud::COLOR_SECONDARY].r = 0;
	m_cColors[CHud::COLOR_SECONDARY].g = 160;
	m_cColors[CHud::COLOR_SECONDARY].b = 0;
	m_cColors[CHud::COLOR_SECONDARY].a = 255;

	m_cColors[CHud::COLOR_WARNING].r = 255;
	m_cColors[CHud::COLOR_WARNING].g = 16;
	m_cColors[CHud::COLOR_WARNING].b = 16;
	m_cColors[CHud::COLOR_WARNING].a = 255;

	m_iFOV = 0;

	zoom_sensitivity_ratio = CVAR_CREATE("zoom_sensitivity_ratio", "1.0", 0);
	cl_fov = gEngfuncs.pfnRegisterVariable("cl_fov", "90", FCVAR_ARCHIVE);
	m_pCvarCrosshair = gEngfuncs.pfnGetCvarPointer("crosshair");
	m_pCvarStealMouse = CVAR_CREATE("hud_capturemouse", "0", FCVAR_ARCHIVE);
	m_pCvarDraw = CVAR_CREATE("hud_draw", "1", FCVAR_ARCHIVE);
	m_pCvarWidescreen = CVAR_CREATE("hud_widescreen", "1", FCVAR_ARCHIVE);
	m_pCvarColor = CVAR_CREATE("hud_color", "FFA000", FCVAR_ARCHIVE);
	m_pCvarTeamColor = CVAR_CREATE("hud_teamcolor", "1", FCVAR_ARCHIVE);
	cl_rollangle = CVAR_CREATE("cl_rollangle", "0.0", FCVAR_ARCHIVE);
	cl_rollspeed = CVAR_CREATE("cl_rollspeed", "200", FCVAR_ARCHIVE);
	cl_bobtilt = CVAR_CREATE("cl_bobtilt", "0", FCVAR_ARCHIVE);
	r_decals = gEngfuncs.pfnGetCvarPointer("r_decals");
	r_explosionstyle = gEngfuncs.pfnRegisterVariable("r_explosionstyle", "1", FCVAR_ARCHIVE);
	violence_hblood = gEngfuncs.pfnGetCvarPointer("violence_hblood");
	violence_hgibs = gEngfuncs.pfnGetCvarPointer("violence_hgibs");
	m_pCvarSuitVolume = gEngfuncs.pfnGetCvarPointer("suitvolume");

	cl_autowepswitch = CVAR_CREATE("cl_autowepswitch", "1", FCVAR_ARCHIVE | FCVAR_USERINFO);
	cl_grenadetoggle = gEngfuncs.pfnRegisterVariable("cl_grenadetoggle", "0", FCVAR_ARCHIVE | FCVAR_USERINFO);
	cl_righthand = gEngfuncs.pfnRegisterVariable("cl_righthand", "1", FCVAR_ARCHIVE | FCVAR_USERINFO);

	m_pSpriteList = NULL;

	// Clear any old HUD list
	if (m_pHudList)
	{
		HUDLIST* pList;
		while (m_pHudList)
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free(pList);
		}
		m_pHudList = NULL;
	}

	// In case we get messages before the first update -- time will be valid
	m_flTime = 1.0;

	m_Ammo.Init();
	m_Health.Init();
	m_SayText.Init();
	m_Spectator.Init();
#ifdef HALFLIFE_HUD_GEIGER
	m_Geiger.Init();
#endif
#ifdef HALFLIFE_TRAINCONTROL
	m_Train.Init();
#endif
	m_Battery.Init();
	m_Flash.Init();
	m_Message.Init();
	m_StatusBar.Init();
	m_DeathNotice.Init();
	m_AmmoSecondary.Init();
	m_TextMessage.Init();
	m_StatusIcons.Init();
	GetClientVoiceMgr()->Init(&g_VoiceStatusHelper, (vgui::Panel**)&gViewPort);

	m_Menu.Init();

	MsgFunc_ResetHUD(0, 0, NULL);
}

// CHud destructor
// cleans up memory allocated for m_rg* arrays
CHud::~CHud()
{
	delete[] m_rghSprites;
	delete[] m_rgrcRects;
	delete[] m_rgszSpriteNames;

	if (m_pHudList)
	{
		HUDLIST* pList;
		while (m_pHudList)
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free(pList);
		}
		m_pHudList = NULL;
	}
}

// GetSpriteIndex()
// searches through the sprite list loaded from hud.txt for a name matching SpriteName
// returns an index into the gHUD.m_rghSprites[] array
// returns 0 if sprite not found
int CHud::GetSpriteIndex(const char* SpriteName)
{
	// look through the loaded sprite name list for SpriteName
	for (int i = 0; i < m_iSpriteCount; i++)
	{
		if (strncmp(SpriteName, m_rgszSpriteNames + (i * MAX_SPRITE_NAME_LENGTH), MAX_SPRITE_NAME_LENGTH) == 0)
			return i;
	}

	return -1; // invalid sprite
}

void CHud::VidInit()
{
	m_scrinfo.iSize = sizeof(m_scrinfo);
	GetScreenInfo(&m_scrinfo);

	if (ScreenWidth < 640)
		m_iRes = 320;
	else
		m_iRes = 640;
	
	if (!IEngineStudio.IsHardware())
	{
		m_iConHeight = ScreenHeight;

		m_flScaleY = m_flScaleX = 1.0F;
	}
	else
	{
		m_iConHeight = m_iRes * 0.75F;

		m_flScaleY = ScreenHeight / (float)m_iConHeight;
		m_flScaleX = m_flScaleY;
	}

	m_flOffsetY = 0.0F;

	m_bIsWidescreen = m_pCvarWidescreen->value == 0.0F;

	// Only load this once
	if (!m_pSpriteList)
	{
		// we need to load the hud.txt, and all sprites within
		m_pSpriteList = SPR_GetList("sprites/hud.txt", &m_iSpriteCountAllRes);

		if (m_pSpriteList)
		{
			// count the number of sprites of the appropriate res
			m_iSpriteCount = 0;
			client_sprite_t* p = m_pSpriteList;
			int j;
			for (j = 0; j < m_iSpriteCountAllRes; j++)
			{
				if (p->iRes == m_iRes)
					m_iSpriteCount++;
				p++;
			}

			// allocated memory for sprite handle arrays
			m_rghSprites = new HSPRITE[m_iSpriteCount];
			m_rgrcRects = new Rect[m_iSpriteCount];
			m_rgszSpriteNames = new char[m_iSpriteCount * MAX_SPRITE_NAME_LENGTH];

			p = m_pSpriteList;
			int index = 0;
			for (j = 0; j < m_iSpriteCountAllRes; j++)
			{
				if (p->iRes == m_iRes)
				{
					char sz[256];
					sprintf(sz, "sprites/%s.spr", p->szSprite);
					m_rghSprites[index] = SPR_Load(sz);
					m_rgrcRects[index] = p->rc;
					strncpy(&m_rgszSpriteNames[index * MAX_SPRITE_NAME_LENGTH], p->szName, MAX_SPRITE_NAME_LENGTH);

					index++;
				}

				p++;
			}
		}
	}
	else
	{
		// we have already have loaded the sprite reference from hud.txt, but
		// we need to make sure all the sprites have been loaded (we've gone through a transition, or loaded a save game)
		client_sprite_t* p = m_pSpriteList;
		int index = 0;
		for (int j = 0; j < m_iSpriteCountAllRes; j++)
		{
			if (p->iRes == m_iRes)
			{
				char sz[256];
				sprintf(sz, "sprites/%s.spr", p->szSprite);
				m_rghSprites[index] = SPR_Load(sz);
				index++;
			}

			p++;
		}
	}

	// assumption: number_1, number_2, etc, are all listed and loaded sequentially
	m_HUD_number_0 = GetSpriteIndex("number_0");

	m_hBackground = SPR_Load("sprites/background.spr");
	m_hSniperScope = SPR_Load("sprites/sniper_scope.spr");

	m_iFontHeight = m_rgrcRects[m_HUD_number_0].bottom - m_rgrcRects[m_HUD_number_0].top;

	m_Ammo.VidInit();
	m_Health.VidInit();
	m_Spectator.VidInit();
#ifdef HALFLIFE_HUD_GEIGER
	m_Geiger.VidInit();
#endif
#ifdef HALFLIFE_TRAINCONTROL
	m_Train.VidInit();
#endif
	m_Battery.VidInit();
	m_Flash.VidInit();
	m_Message.VidInit();
	m_StatusBar.VidInit();
	m_DeathNotice.VidInit();
	m_SayText.VidInit();
	m_Menu.VidInit();
	m_AmmoSecondary.VidInit();
	m_TextMessage.VidInit();
	m_StatusIcons.VidInit();
	GetClientVoiceMgr()->VidInit();
}

/*
============
COM_FileBase
============
*/
// Extracts the base name of a file (no path, no extension, assumes '/' as path separator)
void COM_FileBase(const char* in, char* out)
{
	int len, start, end;

	len = strlen(in);

	// scan backward for '.'
	end = len - 1;
	while (0 != end && in[end] != '.' && in[end] != '/' && in[end] != '\\')
		end--;

	if (in[end] != '.') // no '.', copy to end
		end = len - 1;
	else
		end--; // Found ',', copy to left of '.'


	// Scan backward for '/'
	start = len - 1;
	while (start >= 0 && in[start] != '/' && in[start] != '\\')
		start--;

	if (in[start] != '/' && in[start] != '\\')
		start = 0;
	else
		start++;

	// Length of new sting
	len = end - start + 1;

	// Copy partial string
	strncpy(out, &in[start], len);
	// Terminate it
	out[len] = 0;
}

/*
=================
HUD_IsGame

=================
*/
bool HUD_IsGame(const char* game)
{
	const char* gamedir;
	char gd[1024];

	gamedir = gEngfuncs.pfnGetGameDirectory();
	if (gamedir && '\0' != gamedir[0])
	{
		COM_FileBase(gamedir, gd);
		if (!stricmp(gd, game))
			return true;
	}
	return false;
}

/*
=====================
HUD_GetFOV

Returns last FOV
=====================
*/
float HUD_GetFOV()
{
	if (0 != gEngfuncs.pDemoAPI->IsRecording())
	{
		// Write it
		int i = 0;
		unsigned char buf[100];

		// Active
		*(float*)&buf[i] = gHUD.m_iFOV;
		i += sizeof(float);

		Demo_WriteBuffer(TYPE_ZOOM, i, buf);
	}

	if (0 != gEngfuncs.pDemoAPI->IsPlayingback())
	{
		gHUD.m_iFOV = g_demozoom;
	}
	return gHUD.m_iFOV;
}

void CHud::Update_SetFOV(int iFov)
{
	if (cl_fov->value < FOV_MIN || cl_fov->value > FOV_MAX)
	{
		gEngfuncs.Cvar_SetValue(
			"cl_fov", std::clamp(cl_fov->value, FOV_MIN, FOV_MAX));
	}

	if (iFov <= 0)
	{
		m_iFOV = cl_fov->value;
		m_flMouseSensitivity = 0;
	}
	else
	{
		iFov = cl_fov->value * (iFov / 90.0F);

		if (m_iFOV != iFov)
		{
			m_iFOV = iFov;

			m_flMouseSensitivity =
				sensitivity->value
				* ((float)iFov / (float)cl_fov->value)
				* zoom_sensitivity_ratio->value;
		}
	}
}

void CHud::AddHudElem(CHudBase* phudelem)
{
	HUDLIST *pdl, *ptemp;

	//phudelem->Think();

	if (!phudelem)
		return;

	pdl = (HUDLIST*)malloc(sizeof(HUDLIST));
	if (!pdl)
		return;

	memset(pdl, 0, sizeof(HUDLIST));
	pdl->p = phudelem;

	if (!m_pHudList)
	{
		m_pHudList = pdl;
		return;
	}

	ptemp = m_pHudList;

	while (ptemp->pNext)
		ptemp = ptemp->pNext;

	ptemp->pNext = pdl;
}

float CHud::GetSensitivity()
{
	return m_flMouseSensitivity;
}

bool CHud::ImpulseCommands(int impulse)
{
	switch (impulse)
	{
	case 100:
		if (IsAlive())
		{
			m_Flash.Update_Flashlight(!m_Flash.IsFlashlightOn());
		}
		return true;
	}

	return false;
}

static float g_ColorBlue[3] = {0.6, 0.8, 1.0};
static float g_ColorRed[3] = {1.0, 0.25, 0.25};
static float g_ColorGreen[3] = {0.6, 1.0, 0.6};
static float g_ColorYellow[3] = {1.0, 0.7, 0.0};
static float g_ColorGrey[3] = {0.8, 0.8, 0.8};

float* CHud::GetTeamColor(int teamNumber) const
{
	if (m_gameMode < kGamemodeTeamplay)
	{
		switch (teamNumber)
		{
			case 1:
				return g_ColorYellow;
			default:
				break;
		}
		
		return g_ColorGrey;
	}

	switch (teamNumber)
	{
	case 1:
		return g_ColorBlue;
	case 2:
		return g_ColorRed;
	case 3:
		return g_ColorYellow;
	case 4:
		return g_ColorGreen;
	default:
		break;
	}

	return g_ColorGrey;
}

float* CHud::GetClientColor(int clientIndex) const
{
	return GetTeamColor(g_PlayerExtraInfo[clientIndex].teamnumber);
}

bool CHud::IsAlive()
{
	return !g_PlayerExtraInfo[gEngfuncs.GetLocalPlayer()->index].dead;
}

int CHud::GetObserverMode()
{
	return g_iObserverMode;
}

int CHud::GetObserverTarget()
{
	return g_iObserverTarget;
}

int CHud::GetObserverTarget2()
{
	return g_iObserverTarget2;
}

bool CHud::IsObserver()
{
	return g_iObserverMode != OBS_NONE;
}

bool CHud::IsSpectator()
{
	return g_iTeamNumber == TEAM_SPECTATORS || gEngfuncs.IsSpectateOnly() != 0;
}

bool CHud::IsViewZoomed()
{
	return m_iFOV != cl_fov->value;
}
