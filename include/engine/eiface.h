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

#pragma once

#include "Platform.h"

#define INTERFACE_VERSION 140

#include "custom.h"
#include "cvardef.h"
#include "Sequence.h"
//
// Defines entity interface between engine and DLLs.
// This header file included by engine files and DLL files.
//
// Before including this header, DLLs must:
//		include progdefs.h
// This is conveniently done for them in extdll.h
//

class Entity;

typedef enum
{
	at_notice,
	at_console,	  // same as at_notice, but forces a ConPrintf, not a message box
	at_aiconsole, // same as at_console, but only shown if developer level is 2!
	at_warning,
	at_error,
	at_logged // Server print to console ( only in multiplayer games ).
} ALERT_TYPE;

// 4-22-98  JOHN: added for use in pfnClientPrintf
typedef enum
{
	print_console,
	print_center,
	print_chat,
} PRINT_TYPE;

// For integrity checking of content on clients
typedef enum
{
	force_exactfile,					// File on client must exactly match server's file
	force_model_samebounds,				// For model files only, the geometry must fit in the same bbox
	force_model_specifybounds,			// For model files only, the geometry must fit in the specified bbox
	force_model_specifybounds_if_avail, // For Steam model files only, the geometry must fit in the specified bbox (if the file is available)
} FORCE_TYPE;

// Returned by TraceLine
typedef struct
{
	int fAllSolid;	 // if true, plane is not valid
	int fStartSolid; // if true, the initial point was in a solid area
	int fInOpen;
	int fInWater;
	float flFraction; // time completed, 1.0 = didn't hit anything
	Vector vecEndPos; // final position
	float flPlaneDist;
	Vector vecPlaneNormal; // surface normal at impact
	Entity* pHit;		   // entity the surface is on
	int iHitgroup;		   // 0 == generic, non zero is specific body part
} TraceResult;

// CD audio status
typedef struct
{
	int fPlaying;	 // is sound playing right now?
	int fWasPlaying; // if not, CD is paused if WasPlaying is true.
	int fInitialized;
	int fEnabled;
	int fPlayLooping;
	float cdvolume;
	//byte 	remap[100];
	int fCDRom;
	int fPlayTrack;
} CDStatus;

#include "../common/crc.h"


// Engine hands this to DLLs for functionality callbacks
typedef struct enginefuncs_s
{
	int (*pfnPrecacheModel)(const char* s);
	int (*pfnPrecacheSound)(const char* s);
	void (*pfnSetModel)(Entity* e, const char* m);
	int (*pfnModelIndex)(const char* m);
	int (*pfnModelFrames)(int modelIndex);
	void (*pfnSetSize)(Entity* e, const float* rgflMin, const float* rgflMax);
	void (*pfnChangeLevel)(const char* s1, const char* s2);
	void (*pfnGetSpawnParms)(Entity* ent);
	void (*pfnSaveSpawnParms)(Entity* ent);
	float (*pfnVecToYaw)(const float* rgflVector);
	void (*pfnVecToAngles)(const float* rgflVectorIn, float* rgflVectorOut);
	void (*pfnMoveToOrigin)(Entity* ent, const float* pflGoal, float dist, int iMoveType);
	void (*pfnChangeYaw)(Entity* ent);
	void (*pfnChangePitch)(Entity* ent);
	Entity* (*pfnFindEntityByString)(Entity* pEdictStartSearchAfter, const char* pszField, const char* pszValue);
	int (*pfnGetEntityIllum)(Entity* pEnt);
	Entity* (*pfnFindEntityInSphere)(Entity* pEdictStartSearchAfter, const float* org, float rad);
	Entity* (*pfnFindClientInPVS)(Entity* pEdict);
	Entity* (*pfnEntitiesInPVS)(Entity* pplayer);
	void (*pfnMakeVectors)(const float* rgflVector);
	void (*pfnAngleVectors)(const float* rgflVector, float* forward, float* right, float* up);
	Entity* (*pfnCreateEntity)();
	void (*pfnRemoveEntity)(Entity* e);
	Entity* (*pfnCreateNamedEntity)(int className);
	void (*pfnMakeStatic)(Entity* ent);
	int (*pfnEntIsOnFloor)(Entity* e);
	int (*pfnDropToFloor)(Entity* e);
	int (*pfnWalkMove)(Entity* ent, float yaw, float dist, int iMode);
	void (*pfnSetOrigin)(Entity* e, const float* rgflOrigin);
	void (*pfnEmitSound)(Entity* entity, int channel, const char* sample, /*int*/ float volume, float attenuation, int fFlags, int pitch);
	void (*pfnEmitAmbientSound)(Entity* entity, float* pos, const char* samp, float vol, float attenuation, int fFlags, int pitch);
	void (*pfnTraceLine)(const float* v1, const float* v2, int fNoMonsters, Entity* pentToSkip, TraceResult* ptr);
	void (*pfnTraceToss)(Entity* pent, Entity* pentToIgnore, TraceResult* ptr);
	int (*pfnTraceMonsterHull)(Entity* pEdict, const float* v1, const float* v2, int fNoMonsters, Entity* pentToSkip, TraceResult* ptr);
	void (*pfnTraceHull)(const float* v1, const float* v2, int fNoMonsters, int hullNumber, Entity* pentToSkip, TraceResult* ptr);
	void (*pfnTraceModel)(const float* v1, const float* v2, int hullNumber, Entity* pent, TraceResult* ptr);
	const char* (*pfnTraceTexture)(Entity* pTextureEntity, const float* v1, const float* v2);
	void (*pfnTraceSphere)(const float* v1, const float* v2, int fNoMonsters, float radius, Entity* pentToSkip, TraceResult* ptr);
	void (*pfnGetAimVector)(Entity* ent, float speed, float* rgflReturn);
	void (*pfnServerCommand)(const char* str);
	void (*pfnServerExecute)();
	void (*pfnClientCommand)(Entity* pEdict, const char* szFmt, ...);
	void (*pfnParticleEffect)(const float* org, const float* dir, float color, float count);
	void (*pfnLightStyle)(int style, const char* val);
	int (*pfnDecalIndex)(const char* name);
	int (*pfnPointContents)(const float* rgflVector);
	void (*pfnMessageBegin)(int msg_dest, int msg_type, const float* pOrigin, Entity* ed);
	void (*pfnMessageEnd)();
	void (*pfnWriteByte)(int iValue);
	void (*pfnWriteChar)(int iValue);
	void (*pfnWriteShort)(int iValue);
	void (*pfnWriteLong)(int iValue);
	void (*pfnWriteAngle)(float flValue);
	void (*pfnWriteCoord)(float flValue);
	void (*pfnWriteString)(const char* sz);
	void (*pfnWriteEntity)(int iValue);
	void (*pfnCVarRegister)(cvar_t* pCvar);
	float (*pfnCVarGetFloat)(const char* szVarName);
	const char* (*pfnCVarGetString)(const char* szVarName);
	void (*pfnCVarSetFloat)(const char* szVarName, float flValue);
	void (*pfnCVarSetString)(const char* szVarName, const char* szValue);
	void (*pfnAlertMessage)(ALERT_TYPE atype, const char* szFmt, ...);
	void (*pfnEngineFprintf)(void* pfile, const char* szFmt, ...);
	void* (*pfnPvAllocEntPrivateData)(Entity* pEdict, int32 cb);
	void* (*pfnPvEntPrivateData)(Entity* pEdict);
	void (*pfnFreeEntPrivateData)(Entity* pEdict);
	const char* (*pfnSzFromIndex)(int iString);
	int (*pfnAllocString)(const char* szValue);
	entvars_t* (*pfnGetVarsOfEnt)(Entity* pEdict);
	Entity* (*pfnPEntityOfEntOffset)(int iEntOffset);
	int (*pfnEntOffsetOfPEntity)(const Entity* pEdict);
	int (*pfnIndexOfEdict)(const Entity* pEdict);
	Entity* (*pfnPEntityOfEntIndex)(int iEntIndex);
	Entity* (*pfnFindEntityByVars)(entvars_t* pvars);
	void* (*pfnGetModelPtr)(Entity* pEdict);
	int (*pfnRegUserMsg)(const char* pszName, int iSize);
	void (*pfnAnimationAutomove)(const Entity* pEdict, float flTime);
	void (*pfnGetBonePosition)(const Entity* pEdict, int iBone, float* rgflOrigin, float* rgflAngles);
	uint32 (*pfnFunctionFromName)(const char* pName);
	const char* (*pfnNameForFunction)(uint32 function);
	void (*pfnClientPrintf)(Entity* pEdict, PRINT_TYPE ptype, const char* szMsg); // JOHN: engine callbacks so game DLL can print messages to individual clients
	void (*pfnServerPrint)(const char* szMsg);
	const char* (*pfnCmd_Args)();		  // these 3 added
	const char* (*pfnCmd_Argv)(int argc); // so game DLL can easily
	int (*pfnCmd_Argc)();				  // access client 'cmd' strings
	void (*pfnGetAttachment)(const Entity* pEdict, int iAttachment, float* rgflOrigin, float* rgflAngles);
	void (*pfnCRC32_Init)(CRC32_t* pulCRC);
	void (*pfnCRC32_ProcessBuffer)(CRC32_t* pulCRC, void* p, int len);
	void (*pfnCRC32_ProcessByte)(CRC32_t* pulCRC, unsigned char ch);
	CRC32_t (*pfnCRC32_Final)(CRC32_t pulCRC);
	int32 (*pfnRandomLong)(int32 lLow, int32 lHigh);
	float (*pfnRandomFloat)(float flLow, float flHigh);
	void (*pfnSetView)(const Entity* pClient, const Entity* pViewent);
	float (*pfnTime)();
	void (*pfnCrosshairAngle)(const Entity* pClient, float pitch, float yaw);
	byte* (*pfnLoadFileForMe)(const char* filename, int* pLength);
	void (*pfnFreeFile)(void* buffer);
	void (*pfnEndSection)(const char* pszSectionName); // trigger_endsection
	int (*pfnCompareFileTime)(const char* filename1, const char* filename2, int* iCompare);
	void (*pfnGetGameDir)(char* szGetGameDir);
	void (*pfnCvar_RegisterVariable)(cvar_t* variable);
	void (*pfnFadeClientVolume)(const Entity* pEdict, int fadePercent, int fadeOutSeconds, int holdTime, int fadeInSeconds);
	void (*pfnSetClientMaxspeed)(const Entity* pEdict, float fNewMaxspeed);
	Entity* (*pfnCreateFakeClient)(const char* netname); // returns nullptr if fake client can't be created
	void (*pfnRunPlayerMove)(Entity* fakeclient, const float* viewangles, float forwardmove, float sidemove, float upmove, unsigned short buttons, byte impulse, byte msec);
	int (*pfnNumberOfEntities)();
	char* (*pfnGetInfoKeyBuffer)(Entity* e); // passing in nullptr gets the serverinfo
	char* (*pfnInfoKeyValue)(char* infobuffer, const char* key);
	void (*pfnSetKeyValue)(char* infobuffer, const char* key, const char* value);
	void (*pfnSetClientKeyValue)(int clientIndex, char* infobuffer, const char* key, const char* value);
	int (*pfnIsMapValid)(const char* filename);
	void (*pfnStaticDecal)(const float* origin, int decalIndex, int entityIndex, int modelIndex);
	int (*pfnPrecacheGeneric)(const char* s);
	int (*pfnGetPlayerUserId)(Entity* e); // returns the server assigned userid for this player.  useful for logging frags, etc.  returns -1 if the edict couldn't be found in the list of clients
	void (*pfnBuildSoundMsg)(Entity* entity, int channel, const char* sample, /*int*/ float volume, float attenuation, int fFlags, int pitch, int msg_dest, int msg_type, const float* pOrigin, Entity* ed);
	int (*pfnIsDedicatedServer)(); // is this a dedicated server?
	cvar_t* (*pfnCVarGetPointer)(const char* szVarName);
	unsigned int (*pfnGetPlayerWONId)(Entity* e); // returns the server assigned WONid for this player.  useful for logging frags, etc.  returns -1 if the edict couldn't be found in the list of clients

	// YWB 8/1/99 TFF Physics additions
	void (*pfnInfo_RemoveKey)(char* s, const char* key);
	const char* (*pfnGetPhysicsKeyValue)(const Entity* pClient, const char* key);
	void (*pfnSetPhysicsKeyValue)(const Entity* pClient, const char* key, const char* value);
	const char* (*pfnGetPhysicsInfoString)(const Entity* pClient);
	unsigned short (*pfnPrecacheEvent)(int type, const char* psz);
	void (*pfnPlaybackEvent)(int flags, const Entity* pInvoker, unsigned short eventindex, float delay, const float* origin, const float* angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2);

	unsigned char* (*pfnSetFatPVS)(float* org);
	unsigned char* (*pfnSetFatPAS)(float* org);

	int (*pfnCheckVisibility)(const Entity* entity, unsigned char* pset);

	void (*pfnDeltaSetField)(struct delta_s* pFields, const char* fieldname);
	void (*pfnDeltaUnsetField)(struct delta_s* pFields, const char* fieldname);
	void (*pfnDeltaAddEncoder)(const char* name, void (*conditionalencode)(struct delta_s* pFields, const unsigned char* from, const unsigned char* to));
	int (*pfnGetCurrentPlayer)();
	int (*pfnCanSkipPlayer)(const Entity* player);
	int (*pfnDeltaFindField)(struct delta_s* pFields, const char* fieldname);
	void (*pfnDeltaSetFieldByIndex)(struct delta_s* pFields, int fieldNumber);
	void (*pfnDeltaUnsetFieldByIndex)(struct delta_s* pFields, int fieldNumber);

	void (*pfnSetGroupMask)(int mask, int op);

	int (*pfnCreateInstancedBaseline)(int classname, struct entity_state_s* baseline);
	void (*pfnCvar_DirectSet)(struct cvar_s* var, const char* value);

	// Forces the client and server to be running with the same version of the specified file
	//  ( e.g., a player model ).
	// Calling this has no effect in single player
	void (*pfnForceUnmodified)(FORCE_TYPE type, float* mins, float* maxs, const char* filename);

	void (*pfnGetPlayerStats)(const Entity* pClient, int* ping, int* packet_loss);

	void (*pfnAddServerCommand)(const char* cmd_name, void (*function)());

	// For voice communications, set which clients hear eachother.
	// NOTE: these functions take player entity indices (starting at 1).
	qboolean (*pfnVoice_GetClientListening)(int iReceiver, int iSender);
	qboolean (*pfnVoice_SetClientListening)(int iReceiver, int iSender, qboolean bListen);

	const char* (*pfnGetPlayerAuthId)(Entity* e);

	// PSV: Added for CZ training map
	//	const char *(*pfnKeyNameForBinding)		( const char* pBinding );

	sequenceEntry_s* (*pfnSequenceGet)(const char* fileName, const char* entryName);
	sentenceEntry_s* (*pfnSequencePickSentence)(const char* groupName, int pickMethod, int* picked);

	// LH: Give access to filesize via filesystem
	int (*pfnGetFileSize)(const char* filename);

	unsigned int (*pfnGetApproxWavePlayLen)(const char* filepath);
	// MDC: Added for CZ career-mode
	int (*pfnIsCareerMatch)();

	// BGC: return the number of characters of the localized string referenced by using "label"
	int (*pfnGetLocalizedStringLength)(const char* label);

	// BGC: added to facilitate persistent storage of tutor message decay values for
	// different career game profiles.  Also needs to persist regardless of mp.dll being
	// destroyed and recreated.
	void (*pfnRegisterTutorMessageShown)(int mid);
	int (*pfnGetTimesTutorMessageShown)(int mid);
	void (*ProcessTutorMessageDecayBuffer)(int* buffer, int bufferLength);
	void (*ConstructTutorMessageDecayBuffer)(int* buffer, int bufferLength);
	void (*ResetTutorMessageDecayData)();

	void (*pfnQueryClientCvarValue)(const Entity* player, const char* cvarName);
	void (*pfnQueryClientCvarValue2)(const Entity* player, const char* cvarName, int requestID);
	int (*pfnCheckParm)(const char* pchCmdLineToken, const char** ppnext);
	Entity* (*pfnPEntityOfEntIndexAllEntities)(int iEntIndex);
} enginefuncs_t;


// ONLY ADD NEW FUNCTIONS TO THE END OF THIS STRUCT.  INTERFACE VERSION IS FROZEN AT 138

// Passed to pfnKeyValue
typedef struct KeyValueData_s
{
	const char* szClassName; // in: entity classname
	const char* szKeyName;	 // in: name of key
	const char* szValue;	 // in: value of key
	int32 fHandled;			 // out: DLL sets to true if key-value pair was understood
} KeyValueData;


typedef struct
{
	char mapName[32];
	char landmarkName[32];
	Entity* pentLandmark;
	Vector vecLandmarkOrigin;
} LEVELLIST;
#define MAX_LEVEL_CONNECTIONS 16 // These are encoded in the lower 16bits of ENTITYTABLE->flags

typedef struct
{
	int id;		   // Ordinal ID of this entity (used for entity <--> pointer conversions)
	Entity* pent; // Pointer to the in-game entity

	int location;		// Offset from the base data of this entity
	int size;			// Byte size of this entity's data
	int flags;			// This could be a short -- bit mask of transitions that this entity is in the PVS of
	string_t classname; // entity class name

} ENTITYTABLE;

#define FENTTABLE_PLAYER 0x80000000
#define FENTTABLE_REMOVED 0x40000000
#define FENTTABLE_MOVEABLE 0x20000000
#define FENTTABLE_GLOBAL 0x10000000

typedef struct saverestore_s
{
	char* pBaseData;							// Start of all entity save data
	char* pCurrentData;							// Current buffer pointer for sequential access
	int size;									// Current data size
	int bufferSize;								// Total space for data
	int tokenSize;								// Size of the linear list of tokens
	int tokenCount;								// Number of elements in the pTokens table
	char** pTokens;								// Hash table of entity strings (sparse)
	int currentIndex;							// Holds a global entity table ID
	int tableCount;								// Number of elements in the entity table
	int connectionCount;						// Number of elements in the levelList[]
	ENTITYTABLE* pTable;						// Array of ENTITYTABLE elements (1 for each entity)
	LEVELLIST levelList[MAX_LEVEL_CONNECTIONS]; // List of connections from this level

	// smooth transition
	int fUseLandmark;
	char szLandmarkName[20];  // landmark we'll spawn near in next level
	Vector vecLandmarkOffset; // for landmark transitions
	float time;
	char szCurrentMapName[32]; // To check global entities

} SAVERESTOREDATA;

typedef enum _fieldtypes
{
	FIELD_FLOAT = 0,	   // Any floating point value
	FIELD_STRING,		   // A string ID (return from ALLOC_STRING)
	FIELD_ENTITY,		   // An entity offset (EntityOffset)
	FIELD_CLASSPTR,		   // CBaseEntity *
	FIELD_EHANDLE,		   // Entity handle
	FIELD_EVARS,		   // EVARS *
	FIELD_EDICT,		   // Entity *, or Entity *  (same thing)
	FIELD_VECTOR,		   // Any vector
	FIELD_POSITION_VECTOR, // A world coordinate (these are fixed up across level transitions automagically)
	FIELD_POINTER,		   // Arbitrary data pointer... to be removed, use an array of FIELD_CHARACTER
	FIELD_INTEGER,		   // Any integer or enum
	FIELD_FUNCTION,		   // A class function pointer (Think, Use, etc)
	FIELD_BOOLEAN,		   // boolean, implemented as an int, I may use this as a hint for compression
	FIELD_SHORT,		   // 2 byte integer
	FIELD_CHARACTER,	   // a byte
	FIELD_TIME,			   // a floating point time (these are fixed up automatically too!)
	FIELD_MODELNAME,	   // Engine string that is a model name (needs precache)
	FIELD_SOUNDNAME,	   // Engine string that is a sound name (needs precache)

	FIELD_ENGINETYPECOUNT, //All preceding types are known to the engine and must remain.

	FIELD_INT64 = FIELD_ENGINETYPECOUNT, //64 bit integer

	FIELD_TYPECOUNT, // MUST BE LAST
} FIELDTYPE;

#define _FIELD(type, name, fieldtype, count, flags)                            \
	{                                                                          \
		fieldtype, #name, static_cast<int>(offsetof(type, name)), count, flags \
	}
#define DEFINE_FIELD(type, name, fieldtype) _FIELD(type, name, fieldtype, 1, 0)
#define DEFINE_ARRAY(type, name, fieldtype, count) _FIELD(type, name, fieldtype, count, 0)
#define DEFINE_ENTITY_FIELD(name, fieldtype) _FIELD(Entity, name, fieldtype, 1, 0)
#define DEFINE_ENTITY_ARRAY(name, fieldtype, count) _FIELD(Entity, name, fieldtype, count, 0)
#define DEFINE_ENTITY_GLOBAL_FIELD(name, fieldtype) _FIELD(Entity, name, fieldtype, 1, FTYPEDESC_GLOBAL)
#define DEFINE_GLOBAL_FIELD(type, name, fieldtype) _FIELD(type, name, fieldtype, 1, FTYPEDESC_GLOBAL)

#define _FIELD_ALIAS(type, name, alias, fieldtype, count, flags)               \
	{                                                                          \
		fieldtype, alias, static_cast<int>(offsetof(type, name)), count, flags \
	}
#define DEFINE_FIELD_ALIAS(type, name, alias, fieldtype) _FIELD_ALIAS(type, name, alias, fieldtype, 1, 0)


#define FTYPEDESC_GLOBAL 0x0001 // This field is masked for global entity save/restore

typedef struct
{
	FIELDTYPE fieldType;
	const char* fieldName;
	int fieldOffset;
	short fieldSize;
	short flags;
} TYPEDESCRIPTION;

typedef struct
{
	// Initialize/shutdown the game (one-time call after loading of game .dll )
	void (*pfnGameInit)();
	int (*pfnSpawn)(Entity* pent);
	void (*pfnThink)(Entity* pent);
	void (*pfnUse)(Entity* pentUsed, Entity* pentOther);
	void (*pfnTouch)(Entity* pentTouched, Entity* pentOther);
	void (*pfnBlocked)(Entity* pentBlocked, Entity* pentOther);
	void (*pfnKeyValue)(Entity* pentKeyvalue, KeyValueData* pkvd);
	void (*pfnSave)(Entity* pent, SAVERESTOREDATA* pSaveData);
	int (*pfnRestore)(Entity* pent, SAVERESTOREDATA* pSaveData, int globalEntity);
	void (*pfnSetAbsBox)(Entity* pent);

	void (*pfnSaveWriteFields)(SAVERESTOREDATA*, const char*, void*, TYPEDESCRIPTION*, int);
	void (*pfnSaveReadFields)(SAVERESTOREDATA*, const char*, void*, TYPEDESCRIPTION*, int);

	void (*pfnSaveGlobalState)(SAVERESTOREDATA*);
	void (*pfnRestoreGlobalState)(SAVERESTOREDATA*);
	void (*pfnResetGlobalState)();

	qboolean (*pfnClientConnect)(Entity* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128]);

	void (*pfnClientDisconnect)(Entity* pEntity);
	void (*pfnClientKill)(Entity* pEntity);
	void (*pfnClientPutInServer)(Entity* pEntity);
	void (*pfnClientCommand)(Entity* pEntity);
	void (*pfnClientUserInfoChanged)(Entity* pEntity, char* infobuffer);

	void (*pfnServerActivate)(Entity* pEdictList, int edictCount, int clientMax);
	void (*pfnServerDeactivate)();

	void (*pfnPlayerPreThink)(Entity* pEntity);
	void (*pfnPlayerPostThink)(Entity* pEntity);

	void (*pfnStartFrame)();
	void (*pfnParmsNewLevel)();
	void (*pfnParmsChangeLevel)();

	// Returns string describing current .dll.  E.g., TeamFotrress 2, Half-Life
	const char* (*pfnGetGameDescription)();

	// Notify dll about a player customization.
	void (*pfnPlayerCustomization)(Entity* pEntity, customization_t* pCustom);

	// Spectator funcs
	void (*pfnSpectatorConnect)(Entity* pEntity);
	void (*pfnSpectatorDisconnect)(Entity* pEntity);
	void (*pfnSpectatorThink)(Entity* pEntity);

	// Notify game .dll that engine is going to shut down.  Allows mod authors to set a breakpoint.
	void (*pfnSys_Error)(const char* error_string);

	void (*pfnPM_Move)(struct playermove_s* ppmove, qboolean server);
	void (*pfnPM_Init)(struct playermove_s* ppmove);
	char (*pfnPM_FindTextureType)(const char* name);
	void (*pfnSetupVisibility)(Entity* pViewEntity, Entity* pClient, unsigned char** pvs, unsigned char** pas);
	void (*pfnUpdateClientData)(const Entity* ent, int sendweapons, struct clientdata_s* cd);
	int (*pfnAddToFullPack)(struct entity_state_s* state, int e, Entity* ent, Entity* host, int hostflags, int player, unsigned char* pSet);
	void (*pfnCreateBaseline)(int player, int eindex, struct entity_state_s* baseline, Entity* entity, int playermodelindex, Vector* player_mins, Vector* player_maxs);
	void (*pfnRegisterEncoders)();
	int (*pfnGetWeaponData)(Entity* player, struct weapon_data_s* info);

	void (*pfnCmdStart)(const Entity* player, const struct usercmd_s* cmd, unsigned int random_seed);
	void (*pfnCmdEnd)(const Entity* player);

	// Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
	//  size of the response_buffer, so you must zero it out if you choose not to respond.
	int (*pfnConnectionlessPacket)(const struct netadr_s* net_from, const char* args, char* response_buffer, int* response_buffer_size);

	// Enumerates player hulls.  Returns 0 if the hull number doesn't exist, 1 otherwise
	int (*pfnGetHullBounds)(int hullnumber, float* mins, float* maxs);

	// Create baselines for certain "unplaced" items.
	void (*pfnCreateInstancedBaselines)();

	// One of the pfnForceUnmodified files failed the consistency check for the specified player
	// Return 0 to allow the client to continue, 1 to force immediate disconnection ( with an optional disconnect message of up to 256 characters )
	int (*pfnInconsistentFile)(const Entity* player, const char* filename, char* disconnect_message);

	// The game .dll should return 1 if lag compensation should be allowed ( could also just set
	//  the sv_unlag cvar.
	// Most games right now should return 0, until client-side weapon prediction code is written
	//  and tested for them.
	int (*pfnAllowLagCompensation)();
} DLL_FUNCTIONS;

extern DLL_FUNCTIONS gEntityInterface;

// Current version.
#define NEW_DLL_FUNCTIONS_VERSION 1

typedef struct
{
	// Called right before the object's memory is freed.
	// Calls its destructor.
	void (*pfnOnFreeEntPrivateData)(Entity* pEnt);
	void (*pfnGameShutdown)();
	int (*pfnShouldCollide)(Entity* pentTouched, Entity* pentOther);
	void (*pfnCvarValue)(const Entity* pEnt, const char* value);
	void (*pfnCvarValue2)(const Entity* pEnt, int requestID, const char* cvarName, const char* value);
} NEW_DLL_FUNCTIONS;
typedef int (*NEW_DLL_FUNCTIONS_FN)(NEW_DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion);

// Pointers will be null if the game DLL doesn't support this API.
extern NEW_DLL_FUNCTIONS gNewDLLFunctions;

typedef int (*APIFUNCTION)(DLL_FUNCTIONS* pFunctionTable, int interfaceVersion);
typedef int (*APIFUNCTION2)(DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion);
