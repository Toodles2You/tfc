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

#include "Platform.h"

//
// Misc utility code
//
#include "activity.h"
#include "enginecallback.h"

class CBaseEntity;

inline globalvars_t* gpGlobals = nullptr;

// Use this instead of ALLOC_STRING on constant strings
#define STRING(offset) ((const char*)(gpGlobals->pStringBase + (unsigned int)(offset)))
#define MAKE_STRING(str) ((uint64)(str) - (uint64)(STRING(0)))

inline edict_t* FIND_ENTITY_BY_CLASSNAME(edict_t* entStart, const char* pszName)
{
	return FIND_ENTITY_BY_STRING(entStart, "classname", pszName);
}

inline edict_t* FIND_ENTITY_BY_TARGETNAME(edict_t* entStart, const char* pszName)
{
	return FIND_ENTITY_BY_STRING(entStart, "targetname", pszName);
}

// for doing a reverse lookup. Say you have a door, and want to find its button.
inline edict_t* FIND_ENTITY_BY_TARGET(edict_t* entStart, const char* pszName)
{
	return FIND_ENTITY_BY_STRING(entStart, "target", pszName);
}

// Keeps clutter down a bit, when using a float as a bit-vector
#define SetBits(flBitVector, bits) ((flBitVector) = (int)(flBitVector) | (bits))
#define ClearBits(flBitVector, bits) ((flBitVector) = (int)(flBitVector) & ~(bits))
#define FBitSet(flBitVector, bit) (((int)(flBitVector) & (bit)) != 0)

// More explicit than "int"
typedef int EOFFSET;

// In case this ever changes
#define M_PI 3.14159265358979323846

// This is the glue that hooks .MAP entity class names to our CPP classes
// The _declspec forces them to be exported by name so we can do a lookup with GetProcAddress()
// The function is used to intialize / allocate the object for the entity
#define LINK_ENTITY_TO_CLASS(mapClassName, DLLClassName)    \
	extern "C" DLLEXPORT void mapClassName(entvars_t* pev); \
	void mapClassName(entvars_t* pev) { GetClassPtr((DLLClassName*)pev); }

//
// Conversion among the three types of "entity", including identity-conversions.
//
inline edict_t* ENT(const entvars_t* pev)
{
	return pev->pContainingEntity;
}
inline edict_t* ENT(edict_t* pent)
{
	return pent;
}
inline edict_t* ENT(EOFFSET eoffset) { return (*g_engfuncs.pfnPEntityOfEntOffset)(eoffset); }
inline EOFFSET OFFSET(const edict_t* pent)
{
#if _DEBUG
	if (!pent)
		ALERT(at_error, "Bad ent in OFFSET()\n");
#endif
	return (*g_engfuncs.pfnEntOffsetOfPEntity)(pent);
}
inline EOFFSET OFFSET(entvars_t* pev)
{
#if _DEBUG
	if (!pev)
		ALERT(at_error, "Bad pev in OFFSET()\n");
#endif
	return OFFSET(ENT(pev));
}

inline entvars_t* VARS(edict_t* pent)
{
	if (!pent)
		return NULL;

	return &pent->v;
}

inline int ENTINDEX(edict_t* pEdict) { return (*g_engfuncs.pfnIndexOfEdict)(pEdict); }
inline edict_t* INDEXENT(int iEdictNum) { return (*g_engfuncs.pfnPEntityOfEntIndex)(iEdictNum); }

inline void MessageBegin(int dest, int type, const Vector& origin, CBaseEntity* entity);
inline void MessageBegin(int dest, int type, CBaseEntity* entity);

inline void MessageBegin(int dest, int type, const Vector& origin)
{
	g_engfuncs.pfnMessageBegin(
		dest,
		type,
		origin,
		nullptr);
}

inline void MessageBegin(int dest, int type)
{
	g_engfuncs.pfnMessageBegin(dest, type, nullptr, nullptr);
}

inline void MessageEnd()
{
	g_engfuncs.pfnMessageEnd();
}

inline void WriteByte(int value)
{
	g_engfuncs.pfnWriteByte(value);
}

inline void WriteChar(int value)
{
	g_engfuncs.pfnWriteChar(value);
}

inline void WriteShort(int value)
{
	g_engfuncs.pfnWriteShort(value);
}

inline void WriteLong(int value)
{
	g_engfuncs.pfnWriteLong(value);
}

inline void WriteAngle(float value)
{
	g_engfuncs.pfnWriteAngle(value);
}

inline void WriteCoord(float value)
{
	g_engfuncs.pfnWriteCoord(value);
}

inline void WriteString(const char* value)
{
	g_engfuncs.pfnWriteString(value);
}

inline void WriteEntity(int value)
{
	g_engfuncs.pfnWriteEntity(value);
}

inline void WriteFloat(float value)
{
	WriteLong(*reinterpret_cast<int*>(&value));
}

// Testing the three types of "entity" for nullity
inline bool FNullEnt(EOFFSET eoffset)
{
	return eoffset == 0;
}
inline bool FNullEnt(const edict_t* pent) { return pent == NULL || FNullEnt(OFFSET(pent)); }
inline bool FNullEnt(entvars_t* pev) { return pev == NULL || FNullEnt(OFFSET(pev)); }

// Testing strings for nullity
#define iStringNull 0
inline bool FStringNull(int iString)
{
	return iString == iStringNull;
}

// Dot products for view cone checking
#define VIEW_FIELD_FULL (float)-1.0		   // +-180 degrees
#define VIEW_FIELD_WIDE (float)-0.7		   // +-135 degrees 0.1 // +-85 degrees, used for full FOV checks
#define VIEW_FIELD_NARROW (float)0.7	   // +-45 degrees, more narrow check used to set up ranged attacks
#define VIEW_FIELD_ULTRA_NARROW (float)0.9 // +-25 degrees, more narrow check used to set up ranged attacks

// All monsters need this data
#define DONT_BLEED -1
#define BLOOD_COLOR_RED (byte)247
#define BLOOD_COLOR_YELLOW (byte)195
#define BLOOD_COLOR_GREEN BLOOD_COLOR_YELLOW

typedef enum
{

	MONSTERSTATE_NONE = 0,
	MONSTERSTATE_IDLE,
	MONSTERSTATE_COMBAT,
	MONSTERSTATE_ALERT,
	MONSTERSTATE_HUNT,
	MONSTERSTATE_PRONE,
	MONSTERSTATE_SCRIPT,
	MONSTERSTATE_PLAYDEAD,
	MONSTERSTATE_DEAD

} MONSTERSTATE;



// Things that toggle (buttons/triggers/doors) need this
typedef enum
{
	TS_AT_TOP,
	TS_AT_BOTTOM,
	TS_GOING_UP,
	TS_GOING_DOWN
} TOGGLE_STATE;

// Misc useful
inline bool FStrEq(const char* sz1, const char* sz2)
{
	return (strcmp(sz1, sz2) == 0);
}
inline bool FClassnameIs(edict_t* pent, const char* szClassname)
{
	return FStrEq(STRING(VARS(pent)->classname), szClassname);
}
inline bool FClassnameIs(entvars_t* pev, const char* szClassname)
{
	return FStrEq(STRING(pev->classname), szClassname);
}

//
// Constants that were used only by QC (maybe not used at all now)
//
// Un-comment only as needed
//

#define SND_SPAWNING (1 << 8)	  // duplicated in protocol.h we're spawing, used in some cases for ambients
#define SND_STOP (1 << 5)		  // duplicated in protocol.h stop sound
#define SND_CHANGE_VOL (1 << 6)	  // duplicated in protocol.h change sound vol
#define SND_CHANGE_PITCH (1 << 7) // duplicated in protocol.h change sound pitch

// func_rotating
#define SF_BRUSH_ROTATE_Y_AXIS 0
#define SF_BRUSH_ROTATE_INSTANT 1
#define SF_BRUSH_ROTATE_BACKWARDS 2
#define SF_BRUSH_ROTATE_Z_AXIS 4
#define SF_BRUSH_ROTATE_X_AXIS 8
#define SF_PENDULUM_AUTO_RETURN 16
#define SF_PENDULUM_PASSABLE 32

#define SVC_STUFFTEXT 9
#define SVC_TEMPENTITY 23
#define SVC_INTERMISSION 30
#define SVC_CDTRACK 32
#define SVC_WEAPONANIM 35
#define SVC_ROOMTYPE 37
#define SVC_DIRECTOR 51


// Sound Utilities

// sentence groups
#define CBSENTENCENAME_MAX 16
#define CVOXFILESENTENCEMAX 1536 // max number of sentences in game. NOTE: this must match \
								 // CVOXFILESENTENCEMAX in engine\sound.h!!!

extern char gszallsentencenames[CVOXFILESENTENCEMAX][CBSENTENCENAME_MAX];
extern int gcallsentences;

int USENTENCEG_Pick(int isentenceg, char* szfound);
int USENTENCEG_PickSequential(int isentenceg, char* szfound, int ipick, bool freset);
void USENTENCEG_InitLRU(unsigned char* plru, int count);

void SENTENCEG_Init();
void SENTENCEG_Stop(CBaseEntity* entity, int isentenceg, int ipick);
int SENTENCEG_PlayRndI(CBaseEntity* entity, int isentenceg, float volume, float attenuation, int flags, int pitch);
int SENTENCEG_PlayRndSz(CBaseEntity* entity, const char* szrootname, float volume, float attenuation, int flags, int pitch);
int SENTENCEG_PlaySequentialSz(CBaseEntity* entity, const char* szrootname, float volume, float attenuation, int flags, int pitch, int ipick, bool freset);
int SENTENCEG_GetIndex(const char* szrootname);
int SENTENCEG_Lookup(const char* sample, char* sentencenum);

#define PLAYBACK_EVENT(flags, who, index) PLAYBACK_EVENT_FULL(flags, who, index, 0, g_vecZero, g_vecZero, 0.0, 0.0, 0, 0, 0, 0);
#define PLAYBACK_EVENT_DELAY(flags, who, index, delay) PLAYBACK_EVENT_FULL(flags, who, index, delay, g_vecZero, g_vecZero, 0.0, 0.0, 0, 0, 0, 0);

/**
*	@brief Helper type to run a function when the helper is destroyed.
*	Useful for running cleanup on scope exit and function return.
*/
template <typename Func>
struct CallOnDestroy
{
	const Func Function;

	explicit CallOnDestroy(Func&& function)
		: Function(function)
	{
	}

	~CallOnDestroy()
	{
		Function();
	}
};

namespace util
{
/**
*	@brief Gets the list of entities.
*	Will return @c nullptr if there is no map loaded.
*/
edict_t* GetEntityList();

/**
*	@brief Gets the local player in singleplayer, or @c nullptr in multiplayer.
*/
CBaseEntity* GetLocalPlayer();

// Misc functions
Vector SetMovedir(Vector& angles);
void FireTargets(const char* targetName, CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
int BuildChangeList(LEVELLIST* pLevelList, int maxList);

float VecToYaw(const Vector& vec);
Vector VecToAngles(const Vector& vec);
float AngleMod(float a);
float AngleDiff(float destAngle, float srcAngle);

CBaseEntity* FindEntityInSphere(CBaseEntity* pStartEntity, const Vector& vecCenter, float flRadius);
CBaseEntity* FindEntityByString(CBaseEntity* pStartEntity, const char* szKeyword, const char* szValue);
CBaseEntity* FindEntityByClassname(CBaseEntity* pStartEntity, const char* szName);
CBaseEntity* FindEntityByTargetname(CBaseEntity* pStartEntity, const char* szName);
CBaseEntity* FindEntityGeneric(const char* szName, Vector& vecSrc, float flRadius);

// returns a CBaseEntity pointer to a player by index.  Only returns if the player is spawned and connected
// otherwise returns NULL
// Index is 1 based
CBaseEntity* PlayerByIndex(int playerIndex);

inline edict_t* EntitiesInPVS(edict_t* pent)
{
	return g_engfuncs.pfnEntitiesInPVS(pent);
}

void MakeVectors(const Vector& vecAngles);

// Pass in an array of pointers and an array size, it fills the array and returns the number inserted
int MonstersInSphere(CBaseEntity** pList, int listMax, const Vector& center, float radius);
int EntitiesInBox(CBaseEntity** pList, int listMax, const Vector& mins, const Vector& maxs, int flagMask, bool checkSolid = false);

inline void MakeVectorsPrivate(const Vector& vecAngles, float* p_vForward, float* p_vRight, float* p_vUp)
{
	g_engfuncs.pfnAngleVectors(vecAngles, p_vForward, p_vRight, p_vUp);
}

void MakeAimVectors(const Vector& vecAngles); // like MakeVectors, but assumes pitch isn't inverted
void MakeInvVectors(const Vector& vec, globalvars_t* pgv);

void ParticleEffect(const Vector& vecOrigin, const Vector& vecDirection, unsigned int ulColor, unsigned int ulCount);
void ScreenShake(const Vector& center, float amplitude, float frequency, float duration, float radius);
void ScreenShakeAll(const Vector& center, float amplitude, float frequency, float duration);
void ShowMessage(const char* pString, CBaseEntity* pPlayer);
void ShowMessageAll(const char* pString);
void ScreenFadeAll(const Vector& color, float fadeTime, float holdTime, int alpha, int flags);
void ScreenFade(CBaseEntity* pEntity, const Vector& color, float fadeTime, float fadeHold, int alpha, int flags);

enum
{
	kTraceIgnoreMonsters = 1,
	kTraceIgnoreGlass = 2,
	kTraceBox = 4, /* Trace against bounding boxes, rather than models */
	kTraceBoxModel = 8, /* Perform a second trace after the box trace to check for a body part hit */
};

typedef enum
{
	ignore_monsters = 1,
	dont_ignore_monsters = 0,
	missile = 2
} IGNORE_MONSTERS;
typedef enum
{
	ignore_glass = 1,
	dont_ignore_glass = 0
} IGNORE_GLASS;
void TraceLine(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, CBaseEntity* ignore, TraceResult* ptr);
void TraceLine(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, CBaseEntity* ignore, TraceResult* ptr);

bool TraceLine(const Vector& start, const Vector& end, TraceResult* tr, CBaseEntity* ignore = nullptr, int flags = 0, int hull = 0);

enum
{
	point_hull = 0,
	human_hull = 1,
	large_hull = 2,
	head_hull = 3
};
void TraceHull(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t* pentIgnore, TraceResult* ptr);
TraceResult GetGlobalTrace();
void TraceModel(const Vector& vecStart, const Vector& vecEnd, int hullNumber, edict_t* pentModel, TraceResult* ptr);
Vector GetAimVector(edict_t* pent, float flSpeed);

bool IsMasterTriggered(string_t sMaster, CBaseEntity* pActivator);
void StringToVector(float* pVector, const char* pString);
void StringToIntArray(int* pVector, int count, const char* pString);
Vector ClampVectorToBox(const Vector& input, const Vector& clampSize);
float Approach(float target, float value, float speed);
float ApproachAngle(float target, float value, float speed);
float AngleDistance(float next, float cur);

char* VarArgs(const char* format, ...);
bool TeamsMatch(const char* pTeamName1, const char* pTeamName2);

// Use for ease-in, ease-out style interpolation (accel/decel)
float SplineFraction(float value, float scale);

// allows precacheing of other entities
void PrecacheOther(const char* szClassname);
void PrecacheWeapon(const char* szClassname);

// prints a message to each client
void ClientPrintAll(int msg_dest, const char* msg_name, const char* param1 = NULL, const char* param2 = NULL, const char* param3 = NULL, const char* param4 = NULL);
inline void CenterPrintAll(const char* msg_name, const char* param1 = NULL, const char* param2 = NULL, const char* param3 = NULL, const char* param4 = NULL)
{
	ClientPrintAll(HUD_PRINTCENTER, msg_name, param1, param2, param3, param4);
}

// prints messages through the HUD
void ClientPrint(CBaseEntity* entity, int msg_dest, const char* msg_name, const char* param1 = NULL, const char* param2 = NULL, const char* param3 = NULL, const char* param4 = NULL);

// prints a message to the HUD say (chat)
void SayText(const char* pText, CBaseEntity* pEntity);
void SayTextAll(const char* pText, CBaseEntity* pEntity);


typedef struct hudtextparms_s
{
	float x;
	float y;
	int effect;
	byte r1, g1, b1, a1;
	byte r2, g2, b2, a2;
	float fadeinTime;
	float fadeoutTime;
	float holdTime;
	float fxTime;
	int channel;
} hudtextparms_t;

// prints as transparent 'title' to the HUD
void HudMessageAll(const hudtextparms_t& textparms, const char* pMessage);
void HudMessage(CBaseEntity* pEntity, const hudtextparms_t& textparms, const char* pMessage);

// for handy use with ClientPrint params
char* dtos1(int d);
char* dtos2(int d);
char* dtos3(int d);
char* dtos4(int d);

// Writes message to console with timestamp and FragLog header.
void LogPrintf(const char* fmt, ...);

// Sorta like FInViewCone, but for nonmonsters.
float DotPoints(const Vector& vecSrc, const Vector& vecCheck, const Vector& vecDir);

void StripToken(const char* pKey, char* pDest); // for redundant keynames

enum
{
	GROUP_OP_AND,
	GROUP_OP_NAND,
};

inline int g_groupmask = 0;
inline int g_groupop = 0;

class GroupTrace
{
public:
	GroupTrace(int groupmask, int op);
	~GroupTrace();

private:
	int m_oldgroupmask, m_oldgroupop;
};

void SetGroupTrace(int groupmask, int op);
void UnsetGroupTrace();

int SharedRandomLong(unsigned int seed, int low, int high);
float SharedRandomFloat(unsigned int seed, float low, float high);

CBaseEntity* FindEntityForward(CBaseEntity* pMe);

constexpr bool IsServer()
{
#ifdef CLIENT_DLL
	return false;
#else
	return true;
#endif
}

gamemode_e GetGameMode();
bool IsMultiplayer();
bool IsDeathmatch();
} /* namespace util */

namespace tent
{
void Sparks(const Vector& position);
void Ricochet(const Vector& position, float scale);
void TeleportSplash(CBaseEntity* entity);
void Explosion(const Vector& origin, const Vector& velocity, float damage = 100.0F, bool smoke = true, bool sparks = true);

void BloodStream(const Vector& origin, const Vector& direction, int color, int amount);
void BloodDrips(const Vector& origin, const Vector& direction, int color, int amount);
void BloodDecalTrace(TraceResult* pTrace, int bloodColor);

void DecalTrace(TraceResult* pTrace, int decalNumber);
void PlayerDecalTrace(TraceResult* pTrace, int playernum, int decalNumber, bool bIsCustom);
} // namespace tent
