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
//  cl_dll.h
//

#pragma once

// 4-23-98  JOHN

//
//  This DLL is linked by the client when they first initialize.
// This DLL is responsible for the following tasks:
//		- Loading the HUD graphics upon initialization
//		- Drawing the HUD graphics every frame
//		- Handling the custum HUD-update packets
//

#include "Platform.h"

typedef int (*pfnUserMsgHook)(const char* pszName, int iSize, void* pbuf);

#include "mathlib.h"

#include "../engine/cdll_int.h"
#include "../server/cdll_dll.h"

namespace client
{
inline pfnEngSrc_pfnSPR_Load_t SPR_Load;
inline pfnEngSrc_pfnSPR_Frames_t SPR_Frames;
inline pfnEngSrc_pfnSPR_Height_t SPR_Height;
inline pfnEngSrc_pfnSPR_Width_t SPR_Width;
inline pfnEngSrc_pfnSPR_Set_t SPR_Set;
inline pfnEngSrc_pfnSPR_Draw_t SPR_Draw;
inline pfnEngSrc_pfnSPR_DrawHoles_t SPR_DrawHoles;
inline pfnEngSrc_pfnSPR_DrawAdditive_t SPR_DrawAdditive;
inline pfnEngSrc_pfnSPR_EnableScissor_t SPR_EnableScissor;
inline pfnEngSrc_pfnSPR_DisableScissor_t SPR_DisableScissor;
inline pfnEngSrc_pfnSPR_GetList_t SPR_GetList;
inline pfnEngSrc_pfnFillRGBA_t FillRGBA;
inline pfnEngSrc_pfnGetScreenInfo_t GetScreenInfo;
inline pfnEngSrc_pfnSetCrosshair_t SetCrosshair;
inline pfnEngSrc_pfnRegisterVariable_t RegisterVariable;
inline pfnEngSrc_pfnGetCvarFloat_t GetCvarFloat;
inline pfnEngSrc_pfnGetCvarString_t GetCvarString;
inline pfnEngSrc_pfnAddCommand_t AddCommand;
inline pfnEngSrc_pfnHookUserMsg_t HookUserMsg;
inline pfnEngSrc_pfnServerCmd_t ServerCmd;
inline pfnEngSrc_pfnClientCmd_t ClientCmd;
inline pfnEngSrc_pfnGetPlayerInfo_t GetPlayerInfo;
inline pfnEngSrc_pfnPlaySoundByName_t PlaySoundByName;
inline pfnEngSrc_pfnPlaySoundByIndex_t PlaySoundByIndex;
inline pfnEngSrc_pfnAngleVectors_t AngleVectors;
inline pfnEngSrc_pfnTextMessageGet_t TextMessageGet;
inline pfnEngSrc_pfnDrawCharacter_t DrawCharacter;
inline pfnEngSrc_pfnDrawConsoleString_t DrawConsoleString;
inline pfnEngSrc_pfnDrawSetTextColor_t DrawSetTextColor;
inline pfnEngSrc_pfnDrawConsoleStringLen_t DrawConsoleStringLen;
inline pfnEngSrc_pfnConsolePrint_t ConsolePrint;
inline pfnEngSrc_pfnCenterPrint_t CenterPrint;
inline pfnEngSrc_GetWindowCenterX_t GetWindowCenterX;
inline pfnEngSrc_GetWindowCenterY_t GetWindowCenterY;
inline pfnEngSrc_GetViewAngles_t GetViewAngles;
inline pfnEngSrc_SetViewAngles_t SetViewAngles;
inline pfnEngSrc_GetMaxClients_t GetMaxClients;
inline pfnEngSrc_Cvar_SetValue_t Cvar_SetValue;
inline pfnEngSrc_Cmd_Argc_t Cmd_Argc;
inline pfnEngSrc_Cmd_Argv_t Cmd_Argv;
inline pfnEngSrc_Con_Printf_t Con_Printf;
inline pfnEngSrc_Con_DPrintf_t Con_DPrintf;
inline pfnEngSrc_Con_NPrintf_t Con_NPrintf;
inline pfnEngSrc_Con_NXPrintf_t Con_NXPrintf;
inline pfnEngSrc_PhysInfo_ValueForKey_t PhysInfo_ValueForKey;
inline pfnEngSrc_ServerInfo_ValueForKey_t ServerInfo_ValueForKey;
inline pfnEngSrc_GetClientMaxspeed_t GetClientMaxspeed;
inline pfnEngSrc_CheckParm_t CheckParm;
inline pfnEngSrc_Key_Event_t Key_Event;
inline pfnEngSrc_GetMousePosition_t GetMousePosition;
inline pfnEngSrc_IsNoClipping_t IsNoClipping;
inline pfnEngSrc_GetLocalPlayer_t GetLocalPlayer;
inline pfnEngSrc_GetViewModel_t GetViewModel;
inline pfnEngSrc_GetEntityByIndex_t GetEntityByIndex;
inline pfnEngSrc_GetClientTime_t GetClientTime;
inline pfnEngSrc_V_CalcShake_t V_CalcShake;
inline pfnEngSrc_V_ApplyShake_t V_ApplyShake;
inline pfnEngSrc_PM_PointContents_t PM_PointContents;
inline pfnEngSrc_PM_WaterEntity_t PM_WaterEntity;
inline pfnEngSrc_PM_TraceLine_t PM_TraceLine;
inline pfnEngSrc_CL_LoadModel_t CL_LoadModel;
inline pfnEngSrc_CL_CreateVisibleEntity_t CL_CreateVisibleEntity;
inline pfnEngSrc_GetSpritePointer_t GetSpritePointer;
inline pfnEngSrc_pfnPlaySoundByNameAtLocation_t PlaySoundByNameAtLocation;
inline pfnEngSrc_pfnPrecacheEvent_t PrecacheEvent;
inline pfnEngSrc_pfnPlaybackEvent_t PlaybackEvent;
inline pfnEngSrc_pfnWeaponAnim_t WeaponAnim;
inline pfnEngSrc_pfnRandomFloat_t RandomFloat;
inline pfnEngSrc_pfnRandomLong_t RandomLong;
inline pfnEngSrc_pfnHookEvent_t HookEvent;
inline pfnEngSrc_Con_IsVisible_t Con_IsVisible;
inline pfnEngSrc_pfnGetGameDirectory_t GetGameDirectory;
inline pfnEngSrc_pfnGetCvarPointer_t GetCvarPointer;
inline pfnEngSrc_Key_LookupBinding_t Key_LookupBinding;
inline pfnEngSrc_pfnGetLevelName_t GetLevelName;
inline pfnEngSrc_pfnGetScreenFade_t GetScreenFade;
inline pfnEngSrc_pfnSetScreenFade_t SetScreenFade;
inline pfnEngSrc_VGui_GetPanel_t VGui_GetPanel;
inline pfnEngSrc_VGui_ViewportPaintBackground_t VGui_ViewportPaintBackground;
inline pfnEngSrc_COM_LoadFile_t COM_LoadFile;
inline pfnEngSrc_COM_ParseFile_t COM_ParseFile;
inline pfnEngSrc_COM_FreeFile_t COM_FreeFile;
inline pfnEngSrc_IsSpectateOnly_t IsSpectateOnly;
inline pfnEngSrc_LoadMapSprite_t LoadMapSprite;
inline pfnEngSrc_COM_AddAppDirectoryToSearchPath_t COM_AddAppDirectoryToSearchPath;
inline pfnEngSrc_COM_ExpandFilename_t COM_ExpandFilename;
inline pfnEngSrc_PlayerInfo_ValueForKey_t PlayerInfo_ValueForKey;
inline pfnEngSrc_PlayerInfo_SetValueForKey_t PlayerInfo_SetValueForKey;
inline pfnEngSrc_GetPlayerUniqueID_t GetPlayerUniqueID;
inline pfnEngSrc_GetTrackerIDForPlayer_t GetTrackerIDForPlayer;
inline pfnEngSrc_GetPlayerForTrackerID_t GetPlayerForTrackerID;
inline pfnEngSrc_pfnServerCmdUnreliable_t ServerCmdUnreliable;
inline pfnEngSrc_GetMousePos_t GetMousePos;
inline pfnEngSrc_SetMousePos_t SetMousePos;
inline pfnEngSrc_SetMouseEnable_t SetMouseEnable;
inline pfnEngSrc_GetFirstCVarPtr_t GetFirstCvarPtr;
inline pfnEngSrc_GetFirstCmdFunctionHandle_t GetFirstCmdFunctionHandle;
inline pfnEngSrc_GetNextCmdFunctionHandle_t GetNextCmdFunctionHandle;
inline pfnEngSrc_GetCmdFunctionName_t GetCmdFunctionName;
inline pfnEngSrc_GetClientOldTime_t hudGetClientOldTime;
inline pfnEngSrc_GetServerGravityValue_t hudGetServerGravityValue;
inline pfnEngSrc_GetModelByIndex_t hudGetModelByIndex;
inline pfnEngSrc_pfnSetFilterMode_t SetFilterMode;
inline pfnEngSrc_pfnSetFilterColor_t SetFilterColor;
inline pfnEngSrc_pfnSetFilterBrightness_t SetFilterBrightness;
inline pfnEngSrc_pfnSequenceGet_t SequenceGet;
inline pfnEngSrc_pfnSPR_DrawGeneric_t SPR_DrawGeneric;
inline pfnEngSrc_pfnSequencePickSentence_t SequencePickSentence;
inline pfnEngSrc_pfnDrawString_t DrawString;
inline pfnEngSrc_pfnDrawStringReverse_t DrawStringReverse;
inline pfnEngSrc_LocalPlayerInfo_ValueForKey_t LocalPlayerInfo_ValueForKey;
inline pfnEngSrc_pfnVGUI2DrawCharacter_t VGUI2DrawCharacter;
inline pfnEngSrc_pfnVGUI2DrawCharacterAdd_t VGUI2DrawCharacterAdd;
inline pfnEngSrc_COM_GetApproxWavePlayLength COM_GetApproxWavePlayLength;
inline pfnEngSrc_pfnGetCareerUI_t GetCareerUI;
inline pfnEngSrc_Cvar_Set_t Cvar_Set;
inline pfnEngSrc_pfnIsPlayingCareerMatch_t IsCareerMatch;
inline pfnEngSrc_pfnPlaySoundVoiceByName_t PlaySoundVoiceByName;
inline pfnEngSrc_pfnPrimeMusicStream_t PrimeMusicStream;
inline pfnEngSrc_GetAbsoluteTime_t GetAbsoluteTime;
inline pfnEngSrc_pfnProcessTutorMessageDecayBuffer_t ProcessTutorMessageDecayBuffer;
inline pfnEngSrc_pfnConstructTutorMessageDecayBuffer_t ConstructTutorMessageDecayBuffer;
inline pfnEngSrc_pfnResetTutorMessageDecayData_t ResetTutorMessageDecayData;
inline pfnEngSrc_pfnPlaySoundByNameAtPitch_t PlaySoundByNameAtPitch;
inline pfnEngSrc_pfnFillRGBABlend_t FillRGBABlend;
inline pfnEngSrc_pfnGetAppID_t GetAppID;
inline pfnEngSrc_pfnGetAliases_t GetAliasList;
inline pfnEngSrc_pfnVguiWrap2_GetMouseDelta_t VguiWrap2_GetMouseDelta;
inline pfnEngSrc_pfnFilteredClientCmd_t FilteredClientCmd;
} /* namespace client */
