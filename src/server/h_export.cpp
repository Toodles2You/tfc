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
/*

===== h_export.cpp ========================================================

  Entity classes exported by Halflife.

*/

#include "extdll.h"
#include "util.h"

#include "cbase.h"

#undef DLLEXPORT
#ifdef WIN32
#define DLLEXPORT __stdcall
#else
#define DLLEXPORT __attribute__((visibility("default")))
#endif

extern "C" void DLLEXPORT GiveFnptrsToDll(enginefuncs_t* pengfuncsFromEngine, globalvars_t* pGlobals)
{
	engine::PrecacheModel					 = pengfuncsFromEngine->pfnPrecacheModel;
	engine::PrecacheSound					 = pengfuncsFromEngine->pfnPrecacheSound;
	engine::SetModel						 = pengfuncsFromEngine->pfnSetModel;
	engine::ModelIndex						 = pengfuncsFromEngine->pfnModelIndex;
	engine::ModelFrames						 = pengfuncsFromEngine->pfnModelFrames;
	engine::SetSize							 = pengfuncsFromEngine->pfnSetSize;
	engine::ChangeLevel						 = pengfuncsFromEngine->pfnChangeLevel;
	engine::GetSpawnParms					 = pengfuncsFromEngine->pfnGetSpawnParms;
	engine::SaveSpawnParms					 = pengfuncsFromEngine->pfnSaveSpawnParms;
	engine::VecToYaw						 = pengfuncsFromEngine->pfnVecToYaw;
	engine::VecToAngles						 = pengfuncsFromEngine->pfnVecToAngles;
	engine::MoveToOrigin					 = pengfuncsFromEngine->pfnMoveToOrigin;
	engine::ChangeYaw						 = pengfuncsFromEngine->pfnChangeYaw;
	engine::ChangePitch						 = pengfuncsFromEngine->pfnChangePitch;
	engine::FindEntityByString				 = pengfuncsFromEngine->pfnFindEntityByString;
	engine::GetEntityIllum					 = pengfuncsFromEngine->pfnGetEntityIllum;
	engine::FindEntityInSphere				 = pengfuncsFromEngine->pfnFindEntityInSphere;
	engine::FindClientInPVS					 = pengfuncsFromEngine->pfnFindClientInPVS;
	engine::EntitiesInPVS					 = pengfuncsFromEngine->pfnEntitiesInPVS;
	engine::MakeVectors						 = pengfuncsFromEngine->pfnMakeVectors;
	engine::AngleVectors					 = pengfuncsFromEngine->pfnAngleVectors;
	engine::CreateEntity					 = pengfuncsFromEngine->pfnCreateEntity;
	engine::RemoveEntity					 = pengfuncsFromEngine->pfnRemoveEntity;
	engine::CreateNamedEntity				 = pengfuncsFromEngine->pfnCreateNamedEntity;
	engine::MakeStatic						 = pengfuncsFromEngine->pfnMakeStatic;
	engine::EntIsOnFloor					 = pengfuncsFromEngine->pfnEntIsOnFloor;
	engine::DropToFloor						 = pengfuncsFromEngine->pfnDropToFloor;
	engine::WalkMove						 = pengfuncsFromEngine->pfnWalkMove;
	engine::SetOrigin						 = pengfuncsFromEngine->pfnSetOrigin;
	engine::EmitSound						 = pengfuncsFromEngine->pfnEmitSound;
	engine::EmitAmbientSound				 = pengfuncsFromEngine->pfnEmitAmbientSound;
	engine::TraceLine						 = pengfuncsFromEngine->pfnTraceLine;
	engine::TraceToss						 = pengfuncsFromEngine->pfnTraceToss;
	engine::TraceMonsterHull				 = pengfuncsFromEngine->pfnTraceMonsterHull;
	engine::TraceHull						 = pengfuncsFromEngine->pfnTraceHull;
	engine::TraceModel						 = pengfuncsFromEngine->pfnTraceModel;
	engine::TraceTexture					 = pengfuncsFromEngine->pfnTraceTexture;
	engine::TraceSphere						 = pengfuncsFromEngine->pfnTraceSphere;
	engine::GetAimVector					 = pengfuncsFromEngine->pfnGetAimVector;
	engine::ServerCommand					 = pengfuncsFromEngine->pfnServerCommand;
	engine::ServerExecute					 = pengfuncsFromEngine->pfnServerExecute;
	engine::ClientCommand					 = pengfuncsFromEngine->pfnClientCommand;
	engine::ParticleEffect					 = pengfuncsFromEngine->pfnParticleEffect;
	engine::LightStyle						 = pengfuncsFromEngine->pfnLightStyle;
	engine::DecalIndex						 = pengfuncsFromEngine->pfnDecalIndex;
	engine::PointContents					 = pengfuncsFromEngine->pfnPointContents;
	engine::MessageBegin		    		 = pengfuncsFromEngine->pfnMessageBegin;
	engine::MessageEnd		    			 = pengfuncsFromEngine->pfnMessageEnd;
	engine::WriteByte						 = pengfuncsFromEngine->pfnWriteByte;
	engine::WriteChar						 = pengfuncsFromEngine->pfnWriteChar;
	engine::WriteShort	   					 = pengfuncsFromEngine->pfnWriteShort;
	engine::WriteLong						 = pengfuncsFromEngine->pfnWriteLong;
	engine::WriteAngle			    		 = pengfuncsFromEngine->pfnWriteAngle;
	engine::WriteCoord				    	 = pengfuncsFromEngine->pfnWriteCoord;
	engine::WriteString				    	 = pengfuncsFromEngine->pfnWriteString;
	engine::WriteEntity					     = pengfuncsFromEngine->pfnWriteEntity;
	engine::CVarRegister					 = pengfuncsFromEngine->pfnCVarRegister;
	engine::CVarGetFloat					 = pengfuncsFromEngine->pfnCVarGetFloat;
	engine::CVarGetString					 = pengfuncsFromEngine->pfnCVarGetString;
	engine::CVarSetFloat					 = pengfuncsFromEngine->pfnCVarSetFloat;
	engine::CVarSetString					 = pengfuncsFromEngine->pfnCVarSetString;
	engine::AlertMessage					 = pengfuncsFromEngine->pfnAlertMessage;
	engine::EngineFprintf					 = pengfuncsFromEngine->pfnEngineFprintf;
	engine::PvAllocEntPrivateData			 = pengfuncsFromEngine->pfnPvAllocEntPrivateData;
	engine::PvEntPrivateData				 = pengfuncsFromEngine->pfnPvEntPrivateData;
	engine::FreeEntPrivateData				 = pengfuncsFromEngine->pfnFreeEntPrivateData;
	engine::SzFromIndex						 = pengfuncsFromEngine->pfnSzFromIndex;
	engine::AllocString						 = pengfuncsFromEngine->pfnAllocString;
	engine::GetVarsOfEnt					 = pengfuncsFromEngine->pfnGetVarsOfEnt;
	engine::PEntityOfEntOffset				 = pengfuncsFromEngine->pfnPEntityOfEntOffset;
	engine::EntOffsetOfPEntity				 = pengfuncsFromEngine->pfnEntOffsetOfPEntity;
	engine::IndexOfEdict					 = pengfuncsFromEngine->pfnIndexOfEdict;
	engine::PEntityOfEntIndex				 = pengfuncsFromEngine->pfnPEntityOfEntIndex;
	engine::FindEntityByVars				 = pengfuncsFromEngine->pfnFindEntityByVars;
	engine::GetModelPtr						 = pengfuncsFromEngine->pfnGetModelPtr;
	engine::RegUserMsg						 = pengfuncsFromEngine->pfnRegUserMsg;
	engine::AnimationAutomove				 = pengfuncsFromEngine->pfnAnimationAutomove;
	engine::GetBonePosition					 = pengfuncsFromEngine->pfnGetBonePosition;
	engine::FunctionFromName				 = pengfuncsFromEngine->pfnFunctionFromName;
	engine::NameForFunction					 = pengfuncsFromEngine->pfnNameForFunction;
	engine::ClientPrintf					 = pengfuncsFromEngine->pfnClientPrintf;
	engine::ServerPrint						 = pengfuncsFromEngine->pfnServerPrint;
	engine::Cmd_Args						 = pengfuncsFromEngine->pfnCmd_Args;
	engine::Cmd_Argv						 = pengfuncsFromEngine->pfnCmd_Argv;
	engine::Cmd_Argc						 = pengfuncsFromEngine->pfnCmd_Argc;
	engine::GetAttachment					 = pengfuncsFromEngine->pfnGetAttachment;
	engine::CRC32_Init						 = pengfuncsFromEngine->pfnCRC32_Init;
	engine::CRC32_ProcessBuffer				 = pengfuncsFromEngine->pfnCRC32_ProcessBuffer;
	engine::CRC32_ProcessByte				 = pengfuncsFromEngine->pfnCRC32_ProcessByte;
	engine::CRC32_Final						 = pengfuncsFromEngine->pfnCRC32_Final;
	engine::RandomLong						 = pengfuncsFromEngine->pfnRandomLong;
	engine::RandomFloat						 = pengfuncsFromEngine->pfnRandomFloat;
	engine::SetView							 = pengfuncsFromEngine->pfnSetView;
	engine::Time							 = pengfuncsFromEngine->pfnTime;
	engine::CrosshairAngle					 = pengfuncsFromEngine->pfnCrosshairAngle;
	engine::LoadFileForMe					 = pengfuncsFromEngine->pfnLoadFileForMe;
	engine::FreeFile						 = pengfuncsFromEngine->pfnFreeFile;
	engine::EndSection						 = pengfuncsFromEngine->pfnEndSection;
	engine::CompareFileTime					 = pengfuncsFromEngine->pfnCompareFileTime;
	engine::GetGameDir						 = pengfuncsFromEngine->pfnGetGameDir;
	engine::Cvar_RegisterVariable			 = pengfuncsFromEngine->pfnCvar_RegisterVariable;
	engine::FadeClientVolume				 = pengfuncsFromEngine->pfnFadeClientVolume;
	engine::SetClientMaxspeed				 = pengfuncsFromEngine->pfnSetClientMaxspeed;
	engine::CreateFakeClient				 = pengfuncsFromEngine->pfnCreateFakeClient;
	engine::RunPlayerMove					 = pengfuncsFromEngine->pfnRunPlayerMove;
	engine::NumberOfEntities				 = pengfuncsFromEngine->pfnNumberOfEntities;
	engine::GetInfoKeyBuffer				 = pengfuncsFromEngine->pfnGetInfoKeyBuffer;
	engine::InfoKeyValue					 = pengfuncsFromEngine->pfnInfoKeyValue;
	engine::SetKeyValue						 = pengfuncsFromEngine->pfnSetKeyValue;
	engine::SetClientKeyValue				 = pengfuncsFromEngine->pfnSetClientKeyValue;
	engine::IsMapValid						 = pengfuncsFromEngine->pfnIsMapValid;
	engine::StaticDecal						 = pengfuncsFromEngine->pfnStaticDecal;
	engine::PrecacheGeneric					 = pengfuncsFromEngine->pfnPrecacheGeneric;
	engine::GetPlayerUserId					 = pengfuncsFromEngine->pfnGetPlayerUserId;
	engine::BuildSoundMsg					 = pengfuncsFromEngine->pfnBuildSoundMsg;
	engine::IsDedicatedServer				 = pengfuncsFromEngine->pfnIsDedicatedServer;
	engine::CVarGetPointer					 = pengfuncsFromEngine->pfnCVarGetPointer;
	engine::GetPlayerWONId					 = pengfuncsFromEngine->pfnGetPlayerWONId;
	engine::Info_RemoveKey					 = pengfuncsFromEngine->pfnInfo_RemoveKey;
	engine::GetPhysicsKeyValue				 = pengfuncsFromEngine->pfnGetPhysicsKeyValue;
	engine::SetPhysicsKeyValue				 = pengfuncsFromEngine->pfnSetPhysicsKeyValue;
	engine::GetPhysicsInfoString			 = pengfuncsFromEngine->pfnGetPhysicsInfoString;
	engine::PrecacheEvent					 = pengfuncsFromEngine->pfnPrecacheEvent;
	engine::PlaybackEvent					 = pengfuncsFromEngine->pfnPlaybackEvent;
	engine::SetFatPVS						 = pengfuncsFromEngine->pfnSetFatPVS;
	engine::SetFatPAS						 = pengfuncsFromEngine->pfnSetFatPAS;
	engine::CheckVisibility					 = pengfuncsFromEngine->pfnCheckVisibility;
	engine::DeltaSetField					 = pengfuncsFromEngine->pfnDeltaSetField;
	engine::DeltaUnsetField					 = pengfuncsFromEngine->pfnDeltaUnsetField;
	engine::DeltaAddEncoder					 = pengfuncsFromEngine->pfnDeltaAddEncoder;
	engine::GetCurrentPlayer				 = pengfuncsFromEngine->pfnGetCurrentPlayer;
	engine::CanSkipPlayer					 = pengfuncsFromEngine->pfnCanSkipPlayer;
	engine::DeltaFindField					 = pengfuncsFromEngine->pfnDeltaFindField;
	engine::DeltaSetFieldByIndex			 = pengfuncsFromEngine->pfnDeltaSetFieldByIndex;
	engine::DeltaUnsetFieldByIndex			 = pengfuncsFromEngine->pfnDeltaUnsetFieldByIndex;
	engine::SetGroupMask					 = pengfuncsFromEngine->pfnSetGroupMask;
	engine::CreateInstancedBaseline			 = pengfuncsFromEngine->pfnCreateInstancedBaseline;
	engine::Cvar_DirectSet					 = pengfuncsFromEngine->pfnCvar_DirectSet;
	engine::ForceUnmodified					 = pengfuncsFromEngine->pfnForceUnmodified;
	engine::GetPlayerStats					 = pengfuncsFromEngine->pfnGetPlayerStats;
	engine::AddServerCommand				 = pengfuncsFromEngine->pfnAddServerCommand;
	engine::Voice_GetClientListening		 = pengfuncsFromEngine->pfnVoice_GetClientListening;
	engine::Voice_SetClientListening		 = pengfuncsFromEngine->pfnVoice_SetClientListening;
	engine::GetPlayerAuthId					 = pengfuncsFromEngine->pfnGetPlayerAuthId;
	engine::SequenceGet						 = pengfuncsFromEngine->pfnSequenceGet;
	engine::SequencePickSentence			 = pengfuncsFromEngine->pfnSequencePickSentence;
	engine::GetFileSize						 = pengfuncsFromEngine->pfnGetFileSize;
	engine::GetApproxWavePlayLen			 = pengfuncsFromEngine->pfnGetApproxWavePlayLen;
	engine::IsCareerMatch					 = pengfuncsFromEngine->pfnIsCareerMatch;
	engine::GetLocalizedStringLength		 = pengfuncsFromEngine->pfnGetLocalizedStringLength;
	engine::RegisterTutorMessageShown		 = pengfuncsFromEngine->pfnRegisterTutorMessageShown;
	engine::GetTimesTutorMessageShown		 = pengfuncsFromEngine->pfnGetTimesTutorMessageShown;
	engine::ProcessTutorMessageDecayBuffer	 = pengfuncsFromEngine->ProcessTutorMessageDecayBuffer;
	engine::ConstructTutorMessageDecayBuffer = pengfuncsFromEngine->ConstructTutorMessageDecayBuffer;
	engine::ResetTutorMessageDecayData		 = pengfuncsFromEngine->ResetTutorMessageDecayData;
	engine::QueryClientCvarValue			 = pengfuncsFromEngine->pfnQueryClientCvarValue;
	engine::QueryClientCvarValue2			 = pengfuncsFromEngine->pfnQueryClientCvarValue2;
	engine::CheckParm						 = pengfuncsFromEngine->pfnCheckParm;
	engine::PEntityOfEntIndexAllEntities	 = pengfuncsFromEngine->pfnPEntityOfEntIndexAllEntities;

	gpGlobals = pGlobals;
}
