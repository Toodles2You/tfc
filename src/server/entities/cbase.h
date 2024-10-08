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

// These are caps bits to indicate what an object's capabilities (currently used for save/restore and level transitions)

enum
{
	FCAP_ACROSS_TRANSITION = 0x00000002, // should transfer between transitions

#ifdef HALFLIFE_SAVERESTORE
	FCAP_MUST_SPAWN = 0x00000004,		  // Spawn after restore
#endif
	FCAP_DONT_SAVE = 0x80000000,		  // Don't save this & remove upon forced player respawn

	FCAP_IMPULSE_USE = 0x00000008,		  // can be used by the player
	FCAP_CONTINUOUS_USE = 0x00000010,	  // can be used by the player
	FCAP_DIRECTIONAL_USE = 0x00000040,	  // Player sends +/- 1 when using (currently only tracktrains)
	FCAP_MASTER = 0x00000080,			  // Can be used to "master" other entities (like multisource)

	FCAP_FORCE_TRANSITION = 0x00000080, // ALWAYS goes across transitions

	FCAP_NET_ALWAYS_SEND = 0x00000100, // Don't perform a PVS check in AddToFullPack
};

#include "Platform.h"
#include "saverestore.h"
#include "monsterevent.h"
#include "entity_state.h"

#ifdef HALFLIFE_SAVERESTORE
#define EXPORT DLLEXPORT
#else
#define EXPORT
#endif

/**
*	@brief HACKHACK -- this is a hack to keep the node graph entity from "touching" things (like triggers)
*	while it builds the graph
*/
#ifdef HALFLIFE_NODEGRAPH
inline bool gTouchDisabled = false;
#endif

inline Vector g_vecAttackDir;

// C functions for external declarations that call the appropriate C++ methods
#ifdef GAME_DLL

extern "C" DLLEXPORT int GetEntityAPI(DLL_FUNCTIONS* pFunctionTable, int interfaceVersion);
extern "C" DLLEXPORT int GetEntityAPI2(DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion);
extern "C" DLLEXPORT int GetNewDLLFunctions(NEW_DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion);

int DispatchSpawn(Entity* pent);
void DispatchKeyValue(Entity* pentKeyvalue, KeyValueData* pkvd);
void DispatchTouch(Entity* pentTouched, Entity* pentOther);
void DispatchUse(Entity* pentUsed, Entity* pentOther);
void DispatchThink(Entity* pent);
void DispatchBlocked(Entity* pentBlocked, Entity* pentOther);
void DispatchSave(Entity* pent, SAVERESTOREDATA* pSaveData);
int DispatchRestore(Entity* pent, SAVERESTOREDATA* pSaveData, int globalEntity);
void DispatchObjectCollsionBox(Entity* pent);
void SaveWriteFields(SAVERESTOREDATA* pSaveData, const char* pname, void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount);
void SaveReadFields(SAVERESTOREDATA* pSaveData, const char* pname, void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount);
void SaveGlobalState(SAVERESTOREDATA* pSaveData);
void RestoreGlobalState(SAVERESTOREDATA* pSaveData);
void ResetGlobalState();

#endif /* GAME_DLL */

// monster to monster relationship types
enum
{
	R_AL = -2,	// (ALLY) pals. Good alternative to R_NO when applicable.
	R_FR = -1,	// (FEAR)will run
	R_NO = 0,	// (NO RELATIONSHIP) disregard
	R_DL = 1,	// (DISLIKE) will attack
	R_HT = 2,	// (HATE)will attack this character instead of any visible DISLIKEd characters
	R_NM = 3,	// (NEMESIS)  A monster Will ALWAYS attack its nemsis, no matter what
};

class CBaseEntity;
class CBasePlayer;
class CBasePlayerWeapon;


#define SF_NORESPAWN (1 << 30) // !!!set this bit on guns and stuff that should never respawn.

//
// EHANDLE. Safe way to point to CBaseEntities who may die between frames
//
class EHANDLE
{
private:
	Entity* m_pent;
	int m_serialnumber;

public:
	EHANDLE()
	{
		m_pent = nullptr;
		m_serialnumber = 0;
	}

	Entity* Get();
	Entity* Set(Entity* pent);

	operator CBaseEntity*();

	CBaseEntity* operator=(CBaseEntity* pEntity);
	CBaseEntity* operator->();
};

#ifdef GAME_DLL
typedef struct
{
	float amount;
	int type;
} MULTIDAMAGE;
#endif

//
// Base Entity.  All entity types derive from this
//
class CBaseEntity
{
public:
	enum class Type
	{
		World,
		Point,
		Door,
		RotatingDoor,
		Breakable,
		Pushable,
		Button,
		RotatingButton,
		MomentaryButton,
		MultiSource,
		Laser,
		TriggerVolume,
		ChooChooTrain,
		Track,
		Tank,
	};

public:
	Entity& v; // Don't need to save/restore this pointer, the engine resets it

	// Constructor.  Set engine to use C/C++ callback functions
	// pointers to engine data
	CBaseEntity(Entity* containingEntity);
	virtual ~CBaseEntity();

	virtual bool Is(const Type type) { return false; }

	// path corners
	CBaseEntity* m_pGoalEnt; // path corner we are heading towards
	CBaseEntity* m_pLink;	 // used for temporary link-list operations.

	/**
	*	@brief Entity flags sent to the client in ::AddToFullPack
	*/
	byte m_EFlags = 0;

	// initialization functions
	virtual bool Spawn() { return false; }
	virtual void Precache() {}
	virtual bool EntvarsKeyvalue(KeyValueData *pkvd);
	virtual bool KeyValue(KeyValueData* pkvd);

	enum { kEntvarsCount = 86 };
	static TYPEDESCRIPTION m_EntvarsDescription[kEntvarsCount];

#ifdef HALFLIFE_SAVERESTORE
	static TYPEDESCRIPTION m_SaveData[];

	virtual bool Save(CSave& save);
	virtual bool Restore(CRestore& restore);
#endif

	virtual int ObjectCaps() { return FCAP_ACROSS_TRANSITION; }
	virtual void Activate() {}

	// Setup the object->object collision box (v.mins / v.maxs is the object->world collision box)
	virtual void SetObjectCollisionBox();

	void SetOrigin(const Vector& org);
	void SetSize(const Vector& mins, const Vector& maxs);
	void SetModel(const char* name);
	void SetModel(string_t name) { SetModel(STRING(name)); }

	virtual void DeathNotice(CBaseEntity* child) {} // monster maker children use this to tell the monster maker that they have died.

	virtual void TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, int hitgroup, int bitsDamageType);
	virtual bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType);
	virtual bool GiveHealth(float flHealth, int bitsDamageType, bool bClearEffects = true);
	virtual void Killed(CBaseEntity* inflictor, CBaseEntity* attacker, int bitsDamageType);
	virtual bool IsTriggered(CBaseEntity* pActivator) { return true; }
	virtual int GetToggleState() { return TS_AT_TOP; }
	virtual int GiveAmmo(int iAmount, int iType) { return 0; }
	virtual float GetDelay() { return 0; }
	virtual bool IsMoving() { return v.velocity != g_vecZero; }
	virtual void OverrideReset() {}
	// This is ONLY used by the node graph to test movement through a door
	virtual void SetToggleState(int state) {}
	virtual bool OnControls(CBaseEntity* other) { return false; }
	virtual bool IsAlive() { return (v.deadflag == DEAD_NO) && v.health > 0; }
	virtual bool IsBSPModel() { return v.solid == SOLID_BSP || v.movetype == MOVETYPE_PUSHSTEP; }
	virtual bool HasTarget(string_t targetname) { return streq(targetname, v.target); }
	virtual bool IsClient() { return false; }
	virtual bool IsPlayer() { return false; }
	virtual bool IsBot() { return false; }
	virtual bool IsNetClient() { return false; }
	virtual const char* TeamID() { return ""; }
	virtual int PCNumber() { return v.playerclass; }
	virtual int TeamNumber() { return v.team; }

	virtual CBaseEntity* GetNextTarget();

	// fundamental callbacks

	using ThinkCallback = void (CBaseEntity::*)();
	using TouchCallback = void (CBaseEntity::*)(CBaseEntity*);
	using UseCallback = void (CBaseEntity::*)(CBaseEntity*, CBaseEntity*, USE_TYPE, float);

	ThinkCallback m_pfnThink;
	TouchCallback m_pfnTouch;
	UseCallback m_pfnUse;

	template <class T>
	void SetThink(void (T::*callback)())
	{
		m_pfnThink = static_cast<ThinkCallback>(callback);
	}

	template <class T>
	void SetTouch(void (T::*callback)(CBaseEntity*))
	{
		m_pfnTouch = static_cast<TouchCallback>(callback);
	}

	template <class T>
	void SetUse(void (T::*callback)(CBaseEntity*, CBaseEntity*, USE_TYPE, float))
	{
		m_pfnUse = static_cast<UseCallback>(callback);
	}

	void ClearThink() { m_pfnThink = nullptr; }
	void ClearTouch() { m_pfnTouch = nullptr; }
	void ClearUse() { m_pfnUse = nullptr; }

	virtual void Think()
	{
		if (m_pfnThink != nullptr)
		{
			(this->*m_pfnThink)();
		}
	}
	virtual void Touch(CBaseEntity* other)
	{
		if (m_pfnTouch != nullptr)
		{
			(this->*m_pfnTouch)(other);
		}
	}
	virtual void Use(CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value)
	{
		if (m_pfnUse != nullptr)
		{
			(this->*m_pfnUse)(activator, caller, useType, value);
		}
	}
	virtual void Blocked(CBaseEntity* other)
	{
	}
	virtual bool ShouldCollide(CBaseEntity* other)
	{
		return true;
	}
	virtual bool ShouldBlockTrace()
	{
		return true;
	}
	virtual bool SpannerHit(CBaseEntity* other)
	{
		return false;
	}
	virtual bool ElectromagneticPulse(CBaseEntity* attacker, CBaseEntity* inflictor)
	{
		return false;
	}

	void* operator new(size_t stAllocateBlock)
	{
		//Allocate zero-initialized memory.
		auto memory = ::operator new(stAllocateBlock);
		std::memset(memory, 0, stAllocateBlock);
		return memory;
	}

	//Don't call delete on entities directly, tell the engine to delete it instead.
	void operator delete(void* pMem)
	{
		::operator delete(pMem);
	}

#ifndef CLIENT_DLL
	virtual void UpdateOnRemove();
#endif

	// common member functions
	void EXPORT Remove();
	void EXPORT SUB_CallUseToggle() { this->Use(this, this, USE_TOGGLE, 0); }
	bool ShouldToggle(USE_TYPE useType, bool currentState);

	virtual CBaseEntity* Respawn() { return nullptr; }

	virtual void UseTargets(CBaseEntity* pActivator, USE_TYPE useType, float value);
	// Do the bounding boxes of these two intersect?
	bool Intersects(CBaseEntity* pOther);

#ifdef HALFLIFE_SAVERESTORE
	void MakeDormant();
	bool IsDormant();
#endif

	bool IsLockedByMaster() { return false; }

	// used by monsters that are created by the MonsterMaker
	virtual void UpdateOwner() {}

	static CBaseEntity* Create(const char* szName, const Vector& vecOrigin, const Vector& vecAngles, Entity& owner);

	virtual Vector Center() { return (v.absmax + v.absmin) * 0.5; } // center point of entity
	virtual Vector EyePosition() { return v.origin + v.view_ofs; }  // position of eyes
	virtual Vector EarPosition() { return v.origin + v.view_ofs; }  // position of ears
	virtual Vector BodyTarget() { return Center(); }  // position to shoot at

	virtual int IRelationship(CBaseEntity* pTarget) { return R_NO; }
	bool FInViewCone(CBaseEntity* pEntity);
	bool FInViewCone(const Vector& vecOrigin);
	virtual bool FVisible(CBaseEntity* pEntity);
	virtual bool FVisible(const Vector& vecOrigin);

	virtual void Look(int iDistance); // basic sight function for monsters
	virtual CBaseEntity* BestVisibleEnemy();		// finds best visible enemy for attack

	float m_flFieldOfView;

	void EmitSound(
		const char* sample,
		int channel = CHAN_AUTO,
		float volume = VOL_NORM,
		float attenuation = ATTN_NORM,
		int pitch = PITCH_NORM,
		int flags = 0);

	/**
	*	@brief Just like @see EmitSound, but will skip the current host player if they have cl_lw turned on.
	*	@details entity must be the current host entity for this to work, and must be called only inside a player's PostThink method.
	*/
	void EmitSoundPredicted(
		const char* sample,
		int channel = CHAN_AUTO,
		float volume = VOL_NORM,
		float attenuation = ATTN_NORM,
		int pitch = PITCH_NORM,
		int flags = 0);

	void StopSound(const char* sample, int channel = CHAN_AUTO);

	virtual void GetEntityState(entity_state_t& state, CBasePlayer* player = nullptr);
	virtual void SetEntityState(const entity_state_t& state);

#ifdef GAME_DLL
	bool ApplyMultiDamage(CBaseEntity* inflictor, CBaseEntity* attacker);
	void AddMultiDamage(float damage, int damageType);
#endif

	float m_flDelay;
	int m_iszKillTarget;

	void EXPORT DelayThink();

protected:
#ifdef GAME_DLL
	MULTIDAMAGE m_multiDamage = {0.0F, DMG_GENERIC};
#endif
};

class CBaseAnimating : public CBaseEntity
{
public:
	CBaseAnimating(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	DECLARE_SAVERESTORE()

	// Basic Monster Animation functions
	float StudioFrameAdvance(float flInterval = 0.0); // accumulate animation frame time from last time called until now
	int GetSequenceFlags();
	int LookupActivity(int activity);
	int LookupActivityHeaviest(int activity);
	int LookupSequence(const char* label);
	void ResetSequenceInfo();
	void DispatchAnimEvents(float flFutureInterval = 0.1); // Handle events that have happend since last time called up until X seconds into the future
	virtual void HandleAnimEvent(MonsterEvent_t* pEvent) {}
	float SetBoneController(int iController, float flValue);
	void InitBoneControllers();
	float SetBlending(int iBlender, float flValue);
	void GetBonePosition(int iBone, Vector& origin, Vector& angles);
	int FindTransition(int iEndingSequence, int iGoalSequence, int* piDir);
	void GetAttachment(int iAttachment, Vector& origin, Vector& angles);
	void SetBodygroup(int iGroup, int iValue);
	int GetBodygroup(int iGroup);
	bool ExtractBbox(int sequence, Vector& mins, Vector& maxs);
	void SetSequenceBox();
	virtual void HandleSequenceFinished() {}

	// animation needs
	float m_flFrameRate;	  // computed FPS for current sequence
	float m_flGroundSpeed;	  // computed linear movement rate for current sequence
	float m_flLastEventCheck; // last time the event list was checked
	bool m_fSequenceFinished; // flag set when StudioAdvanceFrame moves across a frame boundry
	bool m_fSequenceLoops;	  // true if the sequence loops
};

#ifdef GAME_DLL

class CPointEntity : public CBaseEntity
{
public:
	CPointEntity(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	bool Is(const Type type) override { return type == Type::Point; }

	bool Spawn() override;
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

private:
};


typedef struct locksounds // sounds that doors and buttons make when locked/unlocked
{
	string_t sLockedSound;		// sound a door makes when it's locked
	string_t sLockedSentence;	// sentence group played when door is locked
	string_t sUnlockedSound;	// sound a door makes when it's unlocked
	string_t sUnlockedSentence; // sentence group played when door is unlocked

	int iLockedSentence;   // which sentence in sentence group to play next
	int iUnlockedSentence; // which sentence in sentence group to play next

	float flwaitSound;	  // time delay between playing consecutive 'locked/unlocked' sounds
	float flwaitSentence; // time delay between playing consecutive sentences
	byte bEOFLocked;	  // true if hit end of list of locked sentences
	byte bEOFUnlocked;	  // true if hit end of list of unlocked sentences
} locksound_t;

void PlayLockSounds(CBaseEntity* entity, locksound_t* pls, bool flocked, bool fbutton);

//
// MultiSouce
//

#define MAX_MULTI_TARGETS 16 // maximum number of targets a single multi_manager entity may be assigned.
#define MS_MAX_TARGETS 32

class CMultiSource : public CPointEntity
{
public:
	CMultiSource(Entity* containingEntity) : CPointEntity(containingEntity) {}

	bool Is(const Type type) override
	{
		return type == Type::MultiSource || CPointEntity::Is(type);
	}

	DECLARE_SAVERESTORE()

	bool Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	int ObjectCaps() override { return (CPointEntity::ObjectCaps() | FCAP_MASTER); }
	bool IsTriggered(CBaseEntity* pActivator) override;
	void EXPORT Register();

	EHANDLE m_rgEntities[MS_MAX_TARGETS];
	int m_rgTriggered[MS_MAX_TARGETS];

	int m_iTotal;
	string_t m_globalstate;
};

#include "tf_map.h"

//
// generic Toggle entity.
//

class CBaseToggle : public CBaseEntity
{
public:
	CBaseToggle(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	DECLARE_SAVERESTORE()

	CTFVars tfv;

	bool EntvarsKeyvalue(KeyValueData *pkvd) override;
	bool KeyValue(KeyValueData* pkvd) override;

	void UseTargets(CBaseEntity* pActivator, USE_TYPE useType, float value) override;

	bool AttemptToActivate(CBaseEntity* activator);

	TOGGLE_STATE m_toggle_state;
	float m_flActivateFinished; //like attack_finished, but for doors
	float m_flMoveDistance;		// how far a door should slide or rotate
	float m_flWait;
	float m_flLip;
	float m_flTWidth;  // for plats
	float m_flTLength; // for plats

	Vector m_vecPosition1;
	Vector m_vecPosition2;
	Vector m_vecAngle1;
	Vector m_vecAngle2;

	int m_cTriggersLeft; // trigger_counter only, # of activations remaining
	float m_flHeight;
	EHANDLE m_hActivator;
	void (CBaseToggle::*m_pfnCallWhenMoveDone)();
	Vector m_vecFinalDest;
	Vector m_vecFinalAngle;

	int m_bitsDamageInflict; // DMG_ damage type that the door or tigger does

	int GetToggleState() override { return m_toggle_state; }
	float GetDelay() override { return m_flWait; }

	// common member functions
	void LinearMove(Vector vecDest, float flSpeed);
	void EXPORT LinearMoveDone();
	void AngularMove(Vector vecDestAngle, float flSpeed);
	void EXPORT AngularMoveDone();
	bool IsLockedByMaster();

	static float AxisValue(int flags, const Vector& angles);
	static void AxisDir(CBaseEntity* entity); /* OOP with extra steps. */
	static float AxisDelta(int flags, const Vector& angle1, const Vector& angle2);

	string_t m_sMaster; // If this button has a master switch, this is the targetname.
						// A master switch must be of the multisource type. If all
						// of the switches in the multisource have been triggered, then
						// the button will be allowed to operate. Otherwise, it will be
						// deactivated.
};
#define SetMoveDone(a) m_pfnCallWhenMoveDone = static_cast<void (CBaseToggle::*)()>(a)

#define bits_CAP_DUCK (1 << 0)		 // crouch
#define bits_CAP_JUMP (1 << 1)		 // jump/leap
#define bits_CAP_STRAFE (1 << 2)	 // strafe ( walk/run sideways)
#define bits_CAP_SQUAD (1 << 3)		 // can form squads
#define bits_CAP_SWIM (1 << 4)		 // proficiently navigate in water
#define bits_CAP_CLIMB (1 << 5)		 // climb ladders/ropes
#define bits_CAP_USE (1 << 6)		 // open doors/push buttons/pull levers
#define bits_CAP_HEAR (1 << 7)		 // can hear forced sounds
#define bits_CAP_AUTO_DOORS (1 << 8) // can trigger auto doors
#define bits_CAP_OPEN_DOORS (1 << 9) // can open manual doors
#define bits_CAP_TURN_HEAD (1 << 10) // can turn head, always bone controller 0

#define bits_CAP_RANGE_ATTACK1 (1 << 11) // can do a range attack 1
#define bits_CAP_RANGE_ATTACK2 (1 << 12) // can do a range attack 2
#define bits_CAP_MELEE_ATTACK1 (1 << 13) // can do a melee attack 1
#define bits_CAP_MELEE_ATTACK2 (1 << 14) // can do a melee attack 2

#define bits_CAP_FLY (1 << 15) // can fly, move all around

#define bits_CAP_DOORS_GROUP (bits_CAP_USE | bits_CAP_AUTO_DOORS | bits_CAP_OPEN_DOORS)

const char* ButtonSound(int sound);

//
// Generic Button
//
class CBaseButton : public CBaseToggle
{
public:
	CBaseButton(Entity* containingEntity) : CBaseToggle(containingEntity) {}

	bool Is(const Type type) override { return type == Type::Button; }

	DECLARE_SAVERESTORE()

	bool Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;

	void ButtonActivate();

	void EXPORT ButtonTouch(CBaseEntity* pOther);
	void EXPORT ButtonSpark();
	void EXPORT TriggerAndWait();
	void EXPORT ButtonReturn();
	void EXPORT ButtonBackHome();
	void EXPORT ButtonUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;

	enum BUTTON_CODE
	{
		BUTTON_NOTHING,
		BUTTON_ACTIVATE,
		BUTTON_RETURN
	};
	BUTTON_CODE ButtonResponseToTouch();

	// Buttons that don't take damage can be IMPULSE used
	int ObjectCaps() override { return (CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | (v.takedamage ? 0 : FCAP_IMPULSE_USE); }

	bool m_fStayPushed; // button stays pushed in until touched again?
	bool m_fRotating;	// a rotating button?  default is a sliding button.

	string_t m_strChangeTarget; // if this field is not null, this is an index into the engine string array.
								// when this button is touched, it's target entity's TARGET field will be set
								// to the button's ChangeTarget. This allows you to make a func_train switch paths, etc.

	locksound_t m_ls; // door lock sounds

	byte m_bLockedSound; // ordinals from entity selection
	byte m_bLockedSentence;
	byte m_bUnlockedSound;
	byte m_bUnlockedSentence;
	int m_sounds;
};

#endif /* GAME_DLL */


// this moved here from world.cpp, to allow classes to be derived from it
//=======================
// CWorld
//
// This spawns first when each level begins.
//=======================
class CWorld : public CBaseEntity
{
public:
	CWorld(Entity* containingEntity);
	~CWorld();

	bool Is(const Type type) override { return type == Type::World; }

	bool Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;

	static inline CWorld* World = nullptr;

#ifdef HALFLIFE_NODEGRAPH
protected:
	EXPORT void PostSpawn();
#endif
};
