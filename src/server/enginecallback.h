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

#pragma once

#include "event_flags.h"

// Must be provided by user of this code
// Holds engine functionality callbacks
namespace engine
{
inline int (*PrecacheModel)(const char* s);
inline int (*PrecacheSound)(const char* s);
inline void (*SetModel)(Entity* e, const char* m);
inline int (*ModelIndex)(const char* m);
inline int (*ModelFrames)(int modelIndex);
inline void (*SetSize)(Entity* e, const float* rgflMin, const float* rgflMax);
inline void (*ChangeLevel)(const char* s1, const char* s2);
inline void (*GetSpawnParms)(Entity* ent);
inline void (*SaveSpawnParms)(Entity* ent);
inline float (*VecToYaw)(const float* rgflVector);
inline void (*VecToAngles)(const float* rgflVectorIn, float* rgflVectorOut);
inline void (*MoveToOrigin)(Entity* ent, const float* pflGoal, float dist, int iMoveType);
inline void (*ChangeYaw)(Entity* ent);
inline void (*ChangePitch)(Entity* ent);
inline Entity* (*FindEntityByString)(Entity* pEdictStartSearchAfter, const char* pszField, const char* pszValue);
inline int (*GetEntityIllum)(Entity* pEnt);
inline Entity* (*FindEntityInSphere)(Entity* pEdictStartSearchAfter, const float* org, float rad);
inline Entity* (*FindClientInPVS)(Entity* pEdict);
inline Entity* (*EntitiesInPVS)(Entity* pplayer);
inline void (*MakeVectors)(const float* rgflVector);
inline void (*AngleVectors)(const float* rgflVector, float* forward, float* right, float* up);
inline Entity* (*CreateEntity)();
inline void (*RemoveEntity)(Entity* e);
inline Entity* (*CreateNamedEntity)(int className);
inline void (*MakeStatic)(Entity* ent);
inline int (*EntIsOnFloor)(Entity* e);
inline int (*DropToFloor)(Entity* e);
inline int (*WalkMove)(Entity* ent, float yaw, float dist, int iMode);
inline void (*SetOrigin)(Entity* e, const float* rgflOrigin);
inline void (*EmitSound)(Entity* entity, int channel, const char* sample, /*int*/ float volume, float attenuation, int fFlags, int pitch);
inline void (*EmitAmbientSound)(Entity* entity, float* pos, const char* samp, float vol, float attenuation, int fFlags, int pitch);
inline void (*TraceLine)(const float* v1, const float* v2, int fNoMonsters, Entity* pentToSkip, TraceResult* ptr);
inline void (*TraceToss)(Entity* pent, Entity* pentToIgnore, TraceResult* ptr);
inline int (*TraceMonsterHull)(Entity* pEdict, const float* v1, const float* v2, int fNoMonsters, Entity* pentToSkip, TraceResult* ptr);
inline void (*TraceHull)(const float* v1, const float* v2, int fNoMonsters, int hullNumber, Entity* pentToSkip, TraceResult* ptr);
inline void (*TraceModel)(const float* v1, const float* v2, int hullNumber, Entity* pent, TraceResult* ptr);
inline const char* (*TraceTexture)(Entity* pTextureEntity, const float* v1, const float* v2);
inline void (*TraceSphere)(const float* v1, const float* v2, int fNoMonsters, float radius, Entity* pentToSkip, TraceResult* ptr);
inline void (*GetAimVector)(Entity* ent, float speed, float* rgflReturn);
inline void (*ServerCommand)(const char* str);
inline void (*ServerExecute)();
inline void (*ClientCommand)(Entity* pEdict, const char* szFmt, ...);
inline void (*ParticleEffect)(const float* org, const float* dir, float color, float count);
inline void (*LightStyle)(int style, const char* val);
inline int (*DecalIndex)(const char* name);
inline int (*PointContents)(const float* rgflVector);
inline void (*MessageBegin)(int msg_dest, int msg_type, const float* pOrigin, Entity* ed);
inline void (*MessageEnd)();
inline void (*WriteByte)(int iValue);
inline void (*WriteChar)(int iValue);
inline void (*WriteShort)(int iValue);
inline void (*WriteLong)(int iValue);
inline void (*WriteAngle)(float flValue);
inline void (*WriteCoord)(float flValue);
inline void (*WriteString)(const char* sz);
inline void (*WriteEntity)(int iValue);
inline void (*CVarRegister)(cvar_t* pCvar);
inline float (*CVarGetFloat)(const char* szVarName);
inline const char* (*CVarGetString)(const char* szVarName);
inline void (*CVarSetFloat)(const char* szVarName, float flValue);
inline void (*CVarSetString)(const char* szVarName, const char* szValue);
inline void (*AlertMessage)(ALERT_TYPE atype, const char* szFmt, ...);
inline void (*EngineFprintf)(void* pfile, const char* szFmt, ...);
inline void* (*PvAllocEntPrivateData)(Entity* pEdict, int32 cb);
inline void* (*PvEntPrivateData)(Entity* pEdict);
inline void (*FreeEntPrivateData)(Entity* pEdict);
inline const char* (*SzFromIndex)(int iString);
inline int (*AllocString)(const char* szValue);
inline entvars_t* (*GetVarsOfEnt)(Entity* pEdict);
inline Entity* (*PEntityOfEntOffset)(int iEntOffset);
inline int (*EntOffsetOfPEntity)(const Entity* pEdict);
inline int (*IndexOfEdict)(const Entity* pEdict);
inline Entity* (*PEntityOfEntIndex)(int iEntIndex);
inline Entity* (*FindEntityByVars)(entvars_t* pvars);
inline void* (*GetModelPtr)(Entity* pEdict);
inline int (*RegUserMsg)(const char* pszName, int iSize);
inline void (*AnimationAutomove)(const Entity* pEdict, float flTime);
inline void (*GetBonePosition)(const Entity* pEdict, int iBone, float* rgflOrigin, float* rgflAngles);
inline uint32 (*FunctionFromName)(const char* pName);
inline const char* (*NameForFunction)(uint32 function);
inline void (*ClientPrintf)(Entity* pEdict, PRINT_TYPE ptype, const char* szMsg);
inline void (*ServerPrint)(const char* szMsg);
inline const char* (*Cmd_Args)();
inline const char* (*Cmd_Argv)(int argc);
inline int (*Cmd_Argc)();
inline void (*GetAttachment)(const Entity* pEdict, int iAttachment, float* rgflOrigin, float* rgflAngles);
inline void (*CRC32_Init)(CRC32_t* pulCRC);
inline void (*CRC32_ProcessBuffer)(CRC32_t* pulCRC, void* p, int len);
inline void (*CRC32_ProcessByte)(CRC32_t* pulCRC, unsigned char ch);
inline CRC32_t (*CRC32_Final)(CRC32_t pulCRC);
inline int32 (*RandomLong)(int32 lLow, int32 lHigh);
inline float (*RandomFloat)(float flLow, float flHigh);
inline void (*SetView)(const Entity* pClient, const Entity* pViewent);
inline float (*Time)();
inline void (*CrosshairAngle)(const Entity* pClient, float pitch, float yaw);
inline byte* (*LoadFileForMe)(const char* filename, int* pLength);
inline void (*FreeFile)(void* buffer);
inline void (*EndSection)(const char* pszSectionName);
inline int (*CompareFileTime)(const char* filename1, const char* filename2, int* iCompare);
inline void (*GetGameDir)(char* szGetGameDir);
inline void (*Cvar_RegisterVariable)(cvar_t* variable);
inline void (*FadeClientVolume)(const Entity* pEdict, int fadePercent, int fadeOutSeconds, int holdTime, int fadeInSeconds);
inline void (*SetClientMaxspeed)(const Entity* pEdict, float fNewMaxspeed);
inline Entity* (*CreateFakeClient)(const char* netname);
inline void (*RunPlayerMove)(Entity* fakeclient, const float* viewangles, float forwardmove, float sidemove, float upmove, unsigned short buttons, byte impulse, byte msec);
inline int (*NumberOfEntities)();
inline char* (*GetInfoKeyBuffer)(Entity* e);
inline char* (*InfoKeyValue)(char* infobuffer, const char* key);
inline void (*SetKeyValue)(char* infobuffer, const char* key, const char* value);
inline void (*SetClientKeyValue)(int clientIndex, char* infobuffer, const char* key, const char* value);
inline int (*IsMapValid)(const char* filename);
inline void (*StaticDecal)(const float* origin, int decalIndex, int entityIndex, int modelIndex);
inline int (*PrecacheGeneric)(const char* s);
inline int (*GetPlayerUserId)(Entity* e);
inline void (*BuildSoundMsg)(Entity* entity, int channel, const char* sample, /*int*/ float volume, float attenuation, int fFlags, int pitch, int msg_dest, int msg_type, const float* pOrigin, Entity* ed);
inline int (*IsDedicatedServer)();
inline cvar_t* (*CVarGetPointer)(const char* szVarName);
inline unsigned int (*GetPlayerWONId)(Entity* e);
inline void (*Info_RemoveKey)(char* s, const char* key);
inline const char* (*GetPhysicsKeyValue)(const Entity* pClient, const char* key);
inline void (*SetPhysicsKeyValue)(const Entity* pClient, const char* key, const char* value);
inline const char* (*GetPhysicsInfoString)(const Entity* pClient);
inline unsigned short (*PrecacheEvent)(int type, const char* psz);
inline void (*PlaybackEvent)(int flags, const Entity* pInvoker, unsigned short eventindex, float delay, const float* origin, const float* angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2);
inline unsigned char* (*SetFatPVS)(float* org);
inline unsigned char* (*SetFatPAS)(float* org);
inline int (*CheckVisibility)(const Entity* entity, unsigned char* pset);
inline void (*DeltaSetField)(struct delta_s* pFields, const char* fieldname);
inline void (*DeltaUnsetField)(struct delta_s* pFields, const char* fieldname);
inline void (*DeltaAddEncoder)(const char* name, void (*conditionalencode)(struct delta_s* pFields, const unsigned char* from, const unsigned char* to));
inline int (*GetCurrentPlayer)();
inline int (*CanSkipPlayer)(const Entity* player);
inline int (*DeltaFindField)(struct delta_s* pFields, const char* fieldname);
inline void (*DeltaSetFieldByIndex)(struct delta_s* pFields, int fieldNumber);
inline void (*DeltaUnsetFieldByIndex)(struct delta_s* pFields, int fieldNumber);
inline void (*SetGroupMask)(int mask, int op);
inline int (*CreateInstancedBaseline)(int classname, struct entity_state_s* baseline);
inline void (*Cvar_DirectSet)(struct cvar_s* var, const char* value);
inline void (*ForceUnmodified)(FORCE_TYPE type, float* mins, float* maxs, const char* filename);
inline void (*GetPlayerStats)(const Entity* pClient, int* ping, int* packet_loss);
inline void (*AddServerCommand)(const char* cmd_name, void (*function)());
inline qboolean (*Voice_GetClientListening)(int iReceiver, int iSender);
inline qboolean (*Voice_SetClientListening)(int iReceiver, int iSender, qboolean bListen);
inline const char* (*GetPlayerAuthId)(Entity* e);
inline sequenceEntry_s* (*SequenceGet)(const char* fileName, const char* entryName);
inline sentenceEntry_s* (*SequencePickSentence)(const char* groupName, int pickMethod, int* picked);
inline int (*GetFileSize)(const char* filename);
inline unsigned int (*GetApproxWavePlayLen)(const char* filepath);
inline int (*IsCareerMatch)();
inline int (*GetLocalizedStringLength)(const char* label);
inline void (*RegisterTutorMessageShown)(int mid);
inline int (*GetTimesTutorMessageShown)(int mid);
inline void (*ProcessTutorMessageDecayBuffer)(int* buffer, int bufferLength);
inline void (*ConstructTutorMessageDecayBuffer)(int* buffer, int bufferLength);
inline void (*ResetTutorMessageDecayData)();
inline void (*QueryClientCvarValue)(const Entity* player, const char* cvarName);
inline void (*QueryClientCvarValue2)(const Entity* player, const char* cvarName, int requestID);
inline int (*CheckParm)(const char* pchCmdLineToken, const char** ppnext);
inline Entity* (*PEntityOfEntIndexAllEntities)(int iEntIndex);
} /* namespace engine */
