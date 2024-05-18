/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
//  cdll_int.c
//
// this implementation handles the linking of the engine to the DLL
//

#include <SDL2/SDL_messagebox.h>

#include "hud.h"
#include "cl_util.h"
#include "netadr.h"
#include "interface.h"
//#include "vgui_schememanager.h"

#include "pm_shared.h"

#include <string.h>
#include "vgui_int.h"

#include "Platform.h"
#include "Exports.h"

#include "tri.h"
#include "vgui_TeamFortressViewport.h"
#include "filesystem_utils.h"
#include "steam_utils.h"

#include "triangleapi.h"
#include "r_efx.h"
#include "event_api.h"
#include "demo_api.h"
#include "net_api.h"
#include "ivoicetweak.h"

CHud gHUD;
TeamFortressViewport* gViewPort = nullptr;


#include "particleman.h"
IParticleMan* g_pParticleMan = nullptr;

void CL_LoadParticleMan();
void CL_UnloadParticleMan();

void InitInput();
void EV_HookEvents();

#ifdef HALFLIFE_JOYSTICK
void Joy_Commands();
#endif

/*
================================
HUD_GetHullBounds

  Engine calls this to enumerate player collision hulls, for prediction.  Return 0 if the hullnumber doesn't exist.
================================
*/
int HUD_GetHullBounds(int hullnumber, float* mins, float* maxs)
{
	return static_cast<int>(PM_GetHullBounds(hullnumber, mins, maxs));
}

/*
================================
HUD_ConnectionlessPacket

 Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
  size of the response_buffer, so you must zero it out if you choose not to respond.
================================
*/
int HUD_ConnectionlessPacket(const struct netadr_s* net_from, const char* args, char* response_buffer, int* response_buffer_size)
{
	// Parse stuff from args
	int max_buffer_size = *response_buffer_size;

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}

char HUD_PlayerMoveTexture(char* name)
{
	return PM_FindTextureType(name);
}

static bool CL_InitClient()
{
	if (!Steam_LoadSteamAPI())
	{
		return false;
	}

	EV_HookEvents();
	CL_LoadParticleMan();

	if (!FileSystem_LoadFileSystem())
	{
		return false;
	}

	if (UTIL_IsValveGameDirectory())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error",
			"This mod has detected that it is being run from a Valve game directory which is not supported\n"
			"Run this mod from its intended location\n\nThe game will now shut down", nullptr);
		return false;
	}

	// get tracker interface, if any
	return true;
}


static bool InitTriAPI(triangleapi_t* triAPI)
{
	/* Toodles TODO: Don't fail to load client; Just disable triangle API calls. */
	if (triAPI->version != TRI_API_VERSION)
	{
		client::Con_DPrintf("Error initializing tri API\n");
		return false;
	}

	client::tri::RenderMode			= triAPI->RenderMode;
	client::tri::Begin				= triAPI->Begin;
	client::tri::End				= triAPI->End;
	client::tri::Color4f			= triAPI->Color4f;
	client::tri::Color4ub			= triAPI->Color4ub;
	client::tri::TexCoord2f			= triAPI->TexCoord2f;
	client::tri::Vertex3fv			= triAPI->Vertex3fv;
	client::tri::Vertex3f			= triAPI->Vertex3f;
	client::tri::Brightness			= triAPI->Brightness;
	client::tri::CullFace			= triAPI->CullFace;
	client::tri::SpriteTexture		= triAPI->SpriteTexture;
	client::tri::WorldToScreen		= triAPI->WorldToScreen;
	client::tri::Fog				= triAPI->Fog;
	client::tri::ScreenToWorld		= triAPI->ScreenToWorld;
	client::tri::GetMatrix			= triAPI->GetMatrix;
	client::tri::BoxInPVS			= triAPI->BoxInPVS;
	client::tri::LightAtPoint		= triAPI->LightAtPoint;
	client::tri::Color4fRendermode	= triAPI->Color4fRendermode;
	client::tri::FogParams			= triAPI->FogParams;

	return true;
}


static bool InitEfxAPI(efx_api_t* efxAPI)
{
	client::efx::AllocParticle			 = efxAPI->R_AllocParticle;
	client::efx::BlobExplosion			 = efxAPI->R_BlobExplosion;
	client::efx::Blood					 = efxAPI->R_Blood;
	client::efx::BloodSprite			 = efxAPI->R_BloodSprite;
	client::efx::BloodStream			 = efxAPI->R_BloodStream;
	client::efx::BreakModel				 = efxAPI->R_BreakModel;
	client::efx::Bubbles				 = efxAPI->R_Bubbles;
	client::efx::BubbleTrail			 = efxAPI->R_BubbleTrail;
	client::efx::BulletImpactParticles	 = efxAPI->R_BulletImpactParticles;
	client::efx::EntityParticles		 = efxAPI->R_EntityParticles;
	client::efx::Explosion				 = efxAPI->R_Explosion;
	client::efx::FizzEffect				 = efxAPI->R_FizzEffect;
	client::efx::FireField				 = efxAPI->R_FireField;
	client::efx::FlickerParticles		 = efxAPI->R_FlickerParticles;
	client::efx::FunnelSprite			 = efxAPI->R_FunnelSprite;
	client::efx::Implosion				 = efxAPI->R_Implosion;
	client::efx::LargeFunnel			 = efxAPI->R_LargeFunnel;
	client::efx::LavaSplash				 = efxAPI->R_LavaSplash;
	client::efx::MultiGunshot			 = efxAPI->R_MultiGunshot;
	client::efx::MuzzleFlash			 = efxAPI->R_MuzzleFlash;
	client::efx::ParticleBox			 = efxAPI->R_ParticleBox;
	client::efx::ParticleBurst			 = efxAPI->R_ParticleBurst;
	client::efx::ParticleExplosion		 = efxAPI->R_ParticleExplosion;
	client::efx::ParticleExplosion2		 = efxAPI->R_ParticleExplosion2;
	client::efx::ParticleLine			 = efxAPI->R_ParticleLine;
	client::efx::PlayerSprites			 = efxAPI->R_PlayerSprites;
	client::efx::Projectile				 = efxAPI->R_Projectile;
	client::efx::RicochetSound			 = efxAPI->R_RicochetSound;
	client::efx::RicochetSprite			 = efxAPI->R_RicochetSprite;
	client::efx::RocketFlare			 = efxAPI->R_RocketFlare;
	client::efx::RocketTrail			 = efxAPI->R_RocketTrail;
	client::efx::RunParticleEffect		 = efxAPI->R_RunParticleEffect;
	client::efx::ShowLine				 = efxAPI->R_ShowLine;
	client::efx::SparkEffect			 = efxAPI->R_SparkEffect;
	client::efx::SparkShower			 = efxAPI->R_SparkShower;
	client::efx::SparkStreaks			 = efxAPI->R_SparkStreaks;
	client::efx::Spray					 = efxAPI->R_Spray;
	client::efx::Sprite_Explode			 = efxAPI->R_Sprite_Explode;
	client::efx::Sprite_Smoke			 = efxAPI->R_Sprite_Smoke;
	client::efx::Sprite_Spray			 = efxAPI->R_Sprite_Spray;
	client::efx::Sprite_Trail			 = efxAPI->R_Sprite_Trail;
	client::efx::Sprite_WallPuff		 = efxAPI->R_Sprite_WallPuff;
	client::efx::StreakSplash			 = efxAPI->R_StreakSplash;
	client::efx::TracerEffect			 = efxAPI->R_TracerEffect;
	client::efx::UserTracerParticle		 = efxAPI->R_UserTracerParticle;
	client::efx::TracerParticles		 = efxAPI->R_TracerParticles;
	client::efx::TeleportSplash			 = efxAPI->R_TeleportSplash;
	client::efx::TempSphereModel		 = efxAPI->R_TempSphereModel;
	client::efx::TempModel				 = efxAPI->R_TempModel;
	client::efx::DefaultSprite			 = efxAPI->R_DefaultSprite;
	client::efx::TempSprite				 = efxAPI->R_TempSprite;
	client::efx::Draw_DecalIndex		 = efxAPI->Draw_DecalIndex;
	client::efx::Draw_DecalIndexFromName = efxAPI->Draw_DecalIndexFromName;
	client::efx::DecalShoot				 = efxAPI->R_DecalShoot;
	client::efx::AttachTentToPlayer		 = efxAPI->R_AttachTentToPlayer;
	client::efx::KillAttachedTents		 = efxAPI->R_KillAttachedTents;
	client::efx::BeamCirclePoints		 = efxAPI->R_BeamCirclePoints;
	client::efx::BeamEntPoint			 = efxAPI->R_BeamEntPoint;
	client::efx::BeamEnts				 = efxAPI->R_BeamEnts;
	client::efx::BeamFollow				 = efxAPI->R_BeamFollow;
	client::efx::BeamKill				 = efxAPI->R_BeamKill;
	client::efx::BeamLightning			 = efxAPI->R_BeamLightning;
	client::efx::BeamPoints				 = efxAPI->R_BeamPoints;
	client::efx::BeamRing				 = efxAPI->R_BeamRing;
	client::efx::AllocDlight			 = efxAPI->CL_AllocDlight;
	client::efx::AllocElight			 = efxAPI->CL_AllocElight;
	client::efx::TempEntAlloc			 = efxAPI->CL_TempEntAlloc;
	client::efx::TempEntAllocNoModel	 = efxAPI->CL_TempEntAllocNoModel;
	client::efx::TempEntAllocHigh		 = efxAPI->CL_TempEntAllocHigh;
	client::efx::TentEntAllocCustom		 = efxAPI->CL_TentEntAllocCustom;
	client::efx::GetPackedColor			 = efxAPI->R_GetPackedColor;
	client::efx::LookupColor			 = efxAPI->R_LookupColor;
	client::efx::DecalRemoveAll			 = efxAPI->R_DecalRemoveAll;
	client::efx::FireCustomDecal		 = efxAPI->R_FireCustomDecal;

	return true;
}


static bool InitEventAPI(event_api_t* eventAPI)
{
	if (eventAPI->version != EVENT_API_VERSION)
	{
		client::Con_DPrintf("Error initializing event API\n");
		return false;
	}

	client::event::PlaySound				= eventAPI->EV_PlaySound;
	client::event::StopSound				= eventAPI->EV_StopSound;
	client::event::FindModelIndex			= eventAPI->EV_FindModelIndex;
	client::event::IsLocal					= eventAPI->EV_IsLocal;
	client::event::LocalPlayerDucking		= eventAPI->EV_LocalPlayerDucking;
	client::event::LocalPlayerViewheight	= eventAPI->EV_LocalPlayerViewheight;
	client::event::LocalPlayerBounds		= eventAPI->EV_LocalPlayerBounds;
	client::event::IndexFromTrace			= eventAPI->EV_IndexFromTrace;
	client::event::GetPhysent				= eventAPI->EV_GetPhysent;
	client::event::SetUpPlayerPrediction	= eventAPI->EV_SetUpPlayerPrediction;
	client::event::PushPMStates				= eventAPI->EV_PushPMStates;
	client::event::PopPMStates				= eventAPI->EV_PopPMStates;
	client::event::SetSolidPlayers			= eventAPI->EV_SetSolidPlayers;
	client::event::SetTraceHull				= eventAPI->EV_SetTraceHull;
	client::event::PlayerTrace				= eventAPI->EV_PlayerTrace;
	client::event::WeaponAnimation			= eventAPI->EV_WeaponAnimation;
	client::event::PrecacheEvent			= eventAPI->EV_PrecacheEvent;
	client::event::PlaybackEvent			= eventAPI->EV_PlaybackEvent;
	client::event::TraceTexture				= eventAPI->EV_TraceTexture;
	client::event::StopAllSounds			= eventAPI->EV_StopAllSounds;
	client::event::KillEvents				= eventAPI->EV_KillEvents;

	return true;
}


static bool InitDemoAPI(demo_api_t* demoAPI)
{
	client::demo::IsRecording	= demoAPI->IsRecording;
	client::demo::IsPlayingback	= demoAPI->IsPlayingback;
	client::demo::IsTimeDemo	= demoAPI->IsTimeDemo;
	client::demo::WriteBuffer	= demoAPI->WriteBuffer;

	return true;
}


int Initialize(cl_enginefunc_t* pEnginefuncs, int iVersion)
{
	if (iVersion != CLDLL_INTERFACE_VERSION)
	{
		return 0;
	}

	client::SPR_Load							= pEnginefuncs->pfnSPR_Load;
	client::SPR_Frames							= pEnginefuncs->pfnSPR_Frames;
	client::SPR_Height							= pEnginefuncs->pfnSPR_Height;
	client::SPR_Width							= pEnginefuncs->pfnSPR_Width;
	client::SPR_Set								= pEnginefuncs->pfnSPR_Set;
	client::SPR_Draw							= pEnginefuncs->pfnSPR_Draw;
	client::SPR_DrawHoles						= pEnginefuncs->pfnSPR_DrawHoles;
	client::SPR_DrawAdditive					= pEnginefuncs->pfnSPR_DrawAdditive;
	client::SPR_EnableScissor					= pEnginefuncs->pfnSPR_EnableScissor;
	client::SPR_DisableScissor					= pEnginefuncs->pfnSPR_DisableScissor;
	client::SPR_GetList							= pEnginefuncs->pfnSPR_GetList;
	client::FillRGBA							= pEnginefuncs->pfnFillRGBA;
	client::GetScreenInfo						= pEnginefuncs->pfnGetScreenInfo;
	client::SetCrosshair						= pEnginefuncs->pfnSetCrosshair;
	client::RegisterVariable					= pEnginefuncs->pfnRegisterVariable;
	client::GetCvarFloat						= pEnginefuncs->pfnGetCvarFloat;
	client::GetCvarString						= pEnginefuncs->pfnGetCvarString;
	client::AddCommand							= pEnginefuncs->pfnAddCommand;
	client::HookUserMsg							= pEnginefuncs->pfnHookUserMsg;
	client::ServerCmd							= pEnginefuncs->pfnServerCmd;
	client::ClientCmd							= pEnginefuncs->pfnClientCmd;
	client::GetPlayerInfo						= pEnginefuncs->pfnGetPlayerInfo;
	client::PlaySoundByName						= pEnginefuncs->pfnPlaySoundByName;
	client::PlaySoundByIndex					= pEnginefuncs->pfnPlaySoundByIndex;
	client::AngleVectors						= pEnginefuncs->pfnAngleVectors;
	client::TextMessageGet						= pEnginefuncs->pfnTextMessageGet;
	client::DrawCharacter						= pEnginefuncs->pfnDrawCharacter;
	client::DrawConsoleString					= pEnginefuncs->pfnDrawConsoleString;
	client::DrawSetTextColor					= pEnginefuncs->pfnDrawSetTextColor;
	client::DrawConsoleStringLen				= pEnginefuncs->pfnDrawConsoleStringLen;
	client::ConsolePrint						= pEnginefuncs->pfnConsolePrint;
	client::CenterPrint							= pEnginefuncs->pfnCenterPrint;
	client::GetWindowCenterX					= pEnginefuncs->GetWindowCenterX;
	client::GetWindowCenterY					= pEnginefuncs->GetWindowCenterY;
	client::GetViewAngles						= pEnginefuncs->GetViewAngles;
	client::SetViewAngles						= pEnginefuncs->SetViewAngles;
	client::GetMaxClients						= pEnginefuncs->GetMaxClients;
	client::Cvar_SetValue						= pEnginefuncs->Cvar_SetValue;
	client::Cmd_Argc							= pEnginefuncs->Cmd_Argc;
	client::Cmd_Argv							= pEnginefuncs->Cmd_Argv;
	client::Con_Printf							= pEnginefuncs->Con_Printf;
	client::Con_DPrintf							= pEnginefuncs->Con_DPrintf;
	client::Con_NPrintf							= pEnginefuncs->Con_NPrintf;
	client::Con_NXPrintf						= pEnginefuncs->Con_NXPrintf;
	client::PhysInfo_ValueForKey				= pEnginefuncs->PhysInfo_ValueForKey;
	client::ServerInfo_ValueForKey				= pEnginefuncs->ServerInfo_ValueForKey;
	client::GetClientMaxspeed					= pEnginefuncs->GetClientMaxspeed;
	client::CheckParm							= pEnginefuncs->CheckParm;
	client::Key_Event							= pEnginefuncs->Key_Event;
	client::GetMousePosition					= pEnginefuncs->GetMousePosition;
	client::IsNoClipping						= pEnginefuncs->IsNoClipping;
	client::GetLocalPlayer						= pEnginefuncs->GetLocalPlayer;
	client::GetViewModel						= pEnginefuncs->GetViewModel;
	client::GetEntityByIndex					= pEnginefuncs->GetEntityByIndex;
	client::GetClientTime						= pEnginefuncs->GetClientTime;
	client::V_CalcShake							= pEnginefuncs->V_CalcShake;
	client::V_ApplyShake						= pEnginefuncs->V_ApplyShake;
	client::PM_PointContents					= pEnginefuncs->PM_PointContents;
	client::PM_WaterEntity						= pEnginefuncs->PM_WaterEntity;
	client::PM_TraceLine						= pEnginefuncs->PM_TraceLine;
	client::CL_LoadModel						= pEnginefuncs->CL_LoadModel;
	client::CL_CreateVisibleEntity				= pEnginefuncs->CL_CreateVisibleEntity;
	client::GetSpritePointer					= pEnginefuncs->GetSpritePointer;
	client::PlaySoundByNameAtLocation			= pEnginefuncs->pfnPlaySoundByNameAtLocation;
	client::PrecacheEvent						= pEnginefuncs->pfnPrecacheEvent;
	client::PlaybackEvent						= pEnginefuncs->pfnPlaybackEvent;
	client::WeaponAnim							= pEnginefuncs->pfnWeaponAnim;
	client::RandomFloat							= pEnginefuncs->pfnRandomFloat;
	client::RandomLong							= pEnginefuncs->pfnRandomLong;
	client::HookEvent							= pEnginefuncs->pfnHookEvent;
	client::Con_IsVisible						= pEnginefuncs->Con_IsVisible;
	client::GetGameDirectory					= pEnginefuncs->pfnGetGameDirectory;
	client::GetCvarPointer						= pEnginefuncs->pfnGetCvarPointer;
	client::Key_LookupBinding					= pEnginefuncs->Key_LookupBinding;
	client::GetLevelName						= pEnginefuncs->pfnGetLevelName;
	client::GetScreenFade						= pEnginefuncs->pfnGetScreenFade;
	client::SetScreenFade						= pEnginefuncs->pfnSetScreenFade;
	client::VGui_GetPanel						= pEnginefuncs->VGui_GetPanel;
	client::VGui_ViewportPaintBackground		= pEnginefuncs->VGui_ViewportPaintBackground;
	client::COM_LoadFile						= pEnginefuncs->COM_LoadFile;
	client::COM_ParseFile						= pEnginefuncs->COM_ParseFile;
	client::COM_FreeFile						= pEnginefuncs->COM_FreeFile;
	client::IsSpectateOnly						= pEnginefuncs->IsSpectateOnly;
	client::LoadMapSprite						= pEnginefuncs->LoadMapSprite;
	client::COM_AddAppDirectoryToSearchPath		= pEnginefuncs->COM_AddAppDirectoryToSearchPath;
	client::COM_ExpandFilename					= pEnginefuncs->COM_ExpandFilename;
	client::PlayerInfo_ValueForKey				= pEnginefuncs->PlayerInfo_ValueForKey;
	client::PlayerInfo_SetValueForKey			= pEnginefuncs->PlayerInfo_SetValueForKey;
	client::GetPlayerUniqueID					= pEnginefuncs->GetPlayerUniqueID;
	client::GetTrackerIDForPlayer				= pEnginefuncs->GetTrackerIDForPlayer;
	client::GetPlayerForTrackerID				= pEnginefuncs->GetPlayerForTrackerID;
	client::ServerCmdUnreliable					= pEnginefuncs->pfnServerCmdUnreliable;
	client::GetMousePos							= pEnginefuncs->pfnGetMousePos;
	client::SetMousePos							= pEnginefuncs->pfnSetMousePos;
	client::SetMouseEnable						= pEnginefuncs->pfnSetMouseEnable;
	client::GetFirstCvarPtr						= pEnginefuncs->GetFirstCvarPtr;
	client::GetFirstCmdFunctionHandle			= pEnginefuncs->GetFirstCmdFunctionHandle;
	client::GetNextCmdFunctionHandle			= pEnginefuncs->GetNextCmdFunctionHandle;
	client::GetCmdFunctionName					= pEnginefuncs->GetCmdFunctionName;
	client::hudGetClientOldTime					= pEnginefuncs->hudGetClientOldTime;
	client::hudGetServerGravityValue			= pEnginefuncs->hudGetServerGravityValue;
	client::hudGetModelByIndex					= pEnginefuncs->hudGetModelByIndex;
	client::SetFilterMode						= pEnginefuncs->pfnSetFilterMode;
	client::SetFilterColor						= pEnginefuncs->pfnSetFilterColor;
	client::SetFilterBrightness					= pEnginefuncs->pfnSetFilterBrightness;
	client::SequenceGet							= pEnginefuncs->pfnSequenceGet;
	client::SPR_DrawGeneric						= pEnginefuncs->pfnSPR_DrawGeneric;
	client::SequencePickSentence				= pEnginefuncs->pfnSequencePickSentence;
	client::DrawString							= pEnginefuncs->pfnDrawString;
	client::DrawStringReverse					= pEnginefuncs->pfnDrawStringReverse;
	client::LocalPlayerInfo_ValueForKey			= pEnginefuncs->LocalPlayerInfo_ValueForKey;
	client::VGUI2DrawCharacter					= pEnginefuncs->pfnVGUI2DrawCharacter;
	client::VGUI2DrawCharacterAdd				= pEnginefuncs->pfnVGUI2DrawCharacterAdd;
	client::COM_GetApproxWavePlayLength			= pEnginefuncs->COM_GetApproxWavePlayLength;
	client::GetCareerUI							= pEnginefuncs->pfnGetCareerUI;
	client::Cvar_Set							= pEnginefuncs->Cvar_Set;
	client::IsCareerMatch						= pEnginefuncs->pfnIsCareerMatch;
	client::PlaySoundVoiceByName				= pEnginefuncs->pfnPlaySoundVoiceByName;
	client::PrimeMusicStream					= pEnginefuncs->pfnPrimeMusicStream;
	client::GetAbsoluteTime						= pEnginefuncs->GetAbsoluteTime;
	client::ProcessTutorMessageDecayBuffer		= pEnginefuncs->pfnProcessTutorMessageDecayBuffer;
	client::ConstructTutorMessageDecayBuffer	= pEnginefuncs->pfnConstructTutorMessageDecayBuffer;
	client::ResetTutorMessageDecayData			= pEnginefuncs->pfnResetTutorMessageDecayData;
	client::PlaySoundByNameAtPitch				= pEnginefuncs->pfnPlaySoundByNameAtPitch;
	client::FillRGBABlend						= pEnginefuncs->pfnFillRGBABlend;
	client::GetAppID							= pEnginefuncs->pfnGetAppID;
	client::GetAliasList						= pEnginefuncs->pfnGetAliasList;
	client::VguiWrap2_GetMouseDelta				= pEnginefuncs->pfnVguiWrap2_GetMouseDelta;
	client::FilteredClientCmd					= pEnginefuncs->pfnFilteredClientCmd;

	if (!InitTriAPI(pEnginefuncs->pTriAPI)
	 || !InitEfxAPI(pEnginefuncs->pEfxAPI)
	 || !InitEventAPI(pEnginefuncs->pEventAPI)
	 || !InitDemoAPI(pEnginefuncs->pDemoAPI))
	{
		client::Con_DPrintf("Error initializing client\n");
		client::ClientCmd("quit\n");
		return 0;
	}

	const bool result = CL_InitClient();

	if (!result)
	{
		client::Con_DPrintf("Error initializing client\n");
		client::ClientCmd("quit\n");
		return 0;
	}

	return 1;
}


/*
==========================
	HUD_VidInit

Called when the game initializes
and whenever the vid_mode is changed
so the HUD can reinitialize itself.
==========================
*/

int HUD_VidInit()
{
	gHUD.VidInit();

	VGui_Startup();

	return 1;
}

/*
==========================
	HUD_Init

Called whenever the client connects
to a server.  Reinitializes all 
the hud variables.
==========================
*/

void HUD_Init()
{
	InitInput();
	gHUD.Init();
	Scheme_Init();
}


/*
==========================
	HUD_Redraw

called every screen frame to
redraw the HUD.
===========================
*/

int HUD_Redraw(float time, int intermission)
{
	gHUD.Redraw(time, 0 != intermission);

	return 1;
}


/*
==========================
	HUD_UpdateClientData

called every time shared client
dll/engine data gets changed,
and gives the cdll a chance
to modify the data.

returns 1 if anything has been changed, 0 otherwise.
==========================
*/

int HUD_UpdateClientData(client_data_t* pcldata, float flTime)
{
#ifdef HALFLIFE_JOYSTICK
	Joy_Commands();
#endif

	return static_cast<int>(gHUD.UpdateClientData(pcldata, flTime));
}

/*
==========================
	HUD_Reset

Called at start and end of demos to restore to "non"HUD state.
==========================
*/

void HUD_Reset()
{
	gHUD.VidInit();
}

/*
==========================
HUD_Frame

Called by engine every frame that client .dll is loaded
==========================
*/

void HUD_Frame(double time)
{
	Steam_Frame();
	GetClientVoiceMgr()->Frame(time);
}


/*
==========================
HUD_VoiceStatus

Called when a player starts or stops talking.
==========================
*/

void HUD_VoiceStatus(int entindex, qboolean bTalking)
{
	GetClientVoiceMgr()->UpdateSpeakerStatus(entindex, 0 != bTalking);
}

/*
==========================
HUD_DirectorMessage

Called when a director event message was received
==========================
*/

void HUD_DirectorMessage(int iSize, void* pbuf)
{
	gHUD.m_Spectator.DirectorMessage(iSize, pbuf);
}

void CL_UnloadParticleMan()
{
	g_pParticleMan = nullptr;
}

void CL_LoadParticleMan()
{
	//Now implemented in the client library.
	auto particleManFactory = Sys_GetFactoryThis();

	g_pParticleMan = (IParticleMan*)particleManFactory(PARTICLEMAN_INTERFACE, nullptr);

	if (g_pParticleMan)
	{
		g_pParticleMan->SetUp(nullptr);
	}
}

extern "C" void DLLEXPORT F(void* pv)
{
	cldll_func_t* pcldll_func = (cldll_func_t*)pv;

	cldll_func_t cldll_func =
		{
			Initialize,
			HUD_Init,
			HUD_VidInit,
			HUD_Redraw,
			HUD_UpdateClientData,
			HUD_Reset,
			HUD_PlayerMove,
			HUD_PlayerMoveInit,
			HUD_PlayerMoveTexture,
			Mouse_Activate,
			Mouse_Deactivate,
			Mouse_Event,
			Mouse_ClearStates,
			Mouse_Accumulate,
			CL_CreateMove,
			CL_IsThirdPerson,
			CL_CameraOffset,
			KB_Find,
			CAM_Think,
			V_CalcRefdef,
			HUD_AddEntity,
			HUD_CreateEntities,
			HUD_DrawNormalTriangles,
			HUD_DrawTransparentTriangles,
			HUD_StudioEvent,
			HUD_PostRunCmd,
			HUD_Shutdown,
			HUD_TxferLocalOverrides,
			HUD_ProcessPlayerState,
			HUD_TxferPredictionData,
			Demo_ReadBuffer,
			HUD_ConnectionlessPacket,
			HUD_GetHullBounds,
			HUD_Frame,
			HUD_Key_Event,
			HUD_TempEntUpdate,
			HUD_GetUserEntity,
			HUD_VoiceStatus,
			HUD_DirectorMessage,
			HUD_GetStudioModelInterface,
			HUD_ChatInputPosition,
		};

	*pcldll_func = cldll_func;
}

#include "cl_dll/IGameClientExports.h"

//-----------------------------------------------------------------------------
// Purpose: Exports functions that are used by the gameUI for UI dialogs
//-----------------------------------------------------------------------------
class CClientExports : public IGameClientExports
{
public:
	// returns the name of the server the user is connected to, if any
	const char* GetServerHostName() override
	{
		/*if (gViewPortInterface)
		{
			return gViewPortInterface->GetServerName();
		}*/
		return "";
	}

	// ingame voice manipulation
	bool IsPlayerGameVoiceMuted(int playerIndex) override
	{
		if (GetClientVoiceMgr())
			return GetClientVoiceMgr()->IsPlayerBlocked(playerIndex);
		return false;
	}

	void MutePlayerGameVoice(int playerIndex) override
	{
		if (GetClientVoiceMgr())
		{
			GetClientVoiceMgr()->SetPlayerBlockedState(playerIndex, true);
		}
	}

	void UnmutePlayerGameVoice(int playerIndex) override
	{
		if (GetClientVoiceMgr())
		{
			GetClientVoiceMgr()->SetPlayerBlockedState(playerIndex, false);
		}
	}
};

EXPOSE_SINGLE_INTERFACE(CClientExports, IGameClientExports, GAMECLIENTEXPORTS_INTERFACE_VERSION);
