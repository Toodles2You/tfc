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

#include "effects.h"
#include "weaponinfo.h"

class CBasePlayer;
class CBasePlayerWeapon;

// Hit Group standards
enum {
	HITGROUP_GENERIC,
	HITGROUP_HEAD,
	HITGROUP_CHEST,
	HITGROUP_STOMACH,
	HITGROUP_LEFTARM,
	HITGROUP_RIGHTARM,
	HITGROUP_LEFTLEG,
	HITGROUP_RIGHTLEG,
	HITGROUP_COUNT,
};

void DeactivateSatchels(CBasePlayer* pOwner);

// Contact Grenade / Timed grenade / Satchel Charge
class CGrenade : public CBaseAnimating
{
public:
	bool Spawn() override;

	static CGrenade* ShootTimed(CBaseEntity* owner, Vector vecStart, Vector vecVelocity, float time);
	static CGrenade* ShootContact(CBaseEntity* owner, Vector vecStart, Vector vecVelocity);

	void Explode(Vector vecSrc, Vector vecAim);
	void Explode(TraceResult* pTrace, int bitsDamageType);
	void EXPORT Smoke();

	void EXPORT BounceTouch(CBaseEntity* pOther);
	void EXPORT SlideTouch(CBaseEntity* pOther);
	void EXPORT ExplodeTouch(CBaseEntity* pOther);
	void EXPORT Detonate();
	void EXPORT DetonateUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT TumbleThink();

	virtual void BounceSound();
	int BloodColor() override { return DONT_BLEED; }
	void Killed(CBaseEntity* inflictor, CBaseEntity* attacker, int bitsDamageType) override;

	float m_flNextAttack;
};


// constant items
#define ITEM_HEALTHKIT 1
#define ITEM_ANTIDOTE 2
#define ITEM_SECURITY 3
#define ITEM_BATTERY 4

#define MAX_NORMAL_BATTERY 100.0F


// weapon weight factors (for auto-switching)   (-1 = noswitch)
#define CROWBAR_WEIGHT 0
#define GLOCK_WEIGHT 10
#define PYTHON_WEIGHT 15
#define MP5_WEIGHT 15
#define SHOTGUN_WEIGHT 15
#define CROSSBOW_WEIGHT 10
#define RPG_WEIGHT 20
#define GAUSS_WEIGHT 20
#define EGON_WEIGHT 20
#define HORNETGUN_WEIGHT 15
#define HANDGRENADE_WEIGHT 5
#define SNARK_WEIGHT 5
#define SATCHEL_WEIGHT -10
#define TRIPMINE_WEIGHT -10


// weapon clip/carry ammo capacities
#define URANIUM_MAX_CARRY 100
#define _9MM_MAX_CARRY 250
#define _357_MAX_CARRY 36
#define BUCKSHOT_MAX_CARRY 125
#define BOLT_MAX_CARRY 50
#define ROCKET_MAX_CARRY 5
#define HANDGRENADE_MAX_CARRY 10
#define SATCHEL_MAX_CARRY 5
#define TRIPMINE_MAX_CARRY 5
#define SNARK_MAX_CARRY 15
#define HORNET_MAX_CARRY 8
#define M203_GRENADE_MAX_CARRY 10

// the maximum amount of ammo each weapon's clip can hold
#define WEAPON_NOCLIP -1

//#define CROWBAR_MAX_CLIP		WEAPON_NOCLIP
#define GLOCK_MAX_CLIP 17
#define PYTHON_MAX_CLIP 6
#define MP5_MAX_CLIP 50
#define SHOTGUN_MAX_CLIP 8
#define CROSSBOW_MAX_CLIP 5
#define RPG_MAX_CLIP 1
#define GAUSS_MAX_CLIP WEAPON_NOCLIP
#define EGON_MAX_CLIP WEAPON_NOCLIP
#define HORNETGUN_MAX_CLIP WEAPON_NOCLIP
#define HANDGRENADE_MAX_CLIP WEAPON_NOCLIP
#define SATCHEL_MAX_CLIP WEAPON_NOCLIP
#define TRIPMINE_MAX_CLIP WEAPON_NOCLIP
#define SNARK_MAX_CLIP WEAPON_NOCLIP


// the default amount of ammo that comes with each gun when it spawns
#define GLOCK_DEFAULT_GIVE 17
#define PYTHON_DEFAULT_GIVE 6
#define MP5_DEFAULT_GIVE 50
#define MP5_M203_DEFAULT_GIVE 0
#define SHOTGUN_DEFAULT_GIVE 12
#define CROSSBOW_DEFAULT_GIVE 5
#define RPG_DEFAULT_GIVE 1
#define GAUSS_DEFAULT_GIVE 20
#define EGON_DEFAULT_GIVE 20
#define HANDGRENADE_DEFAULT_GIVE 5
#define SATCHEL_DEFAULT_GIVE 1
#define TRIPMINE_DEFAULT_GIVE 1
#define SNARK_DEFAULT_GIVE 5
#define HIVEHAND_DEFAULT_GIVE 8

// The amount of ammo given to a player by an ammo item.
#define AMMO_URANIUMBOX_GIVE 20
#define AMMO_GLOCKCLIP_GIVE GLOCK_MAX_CLIP
#define AMMO_357BOX_GIVE PYTHON_MAX_CLIP
#define AMMO_MP5CLIP_GIVE MP5_MAX_CLIP
#define AMMO_CHAINBOX_GIVE 200
#define AMMO_M203BOX_GIVE 2
#define AMMO_BUCKSHOTBOX_GIVE 12
#define AMMO_CROSSBOWCLIP_GIVE CROSSBOW_MAX_CLIP
#define AMMO_RPGCLIP_GIVE RPG_MAX_CLIP
#define AMMO_URANIUMBOX_GIVE 20
#define AMMO_SNARKBOX_GIVE 5

// bullet types
typedef enum
{
	BULLET_NONE = 0,
	BULLET_PLAYER_9MM,		// glock
	BULLET_PLAYER_MP5,		// mp5
	BULLET_PLAYER_357,		// python
	BULLET_PLAYER_BUCKSHOT, // shotgun
	BULLET_PLAYER_CROWBAR,	// crowbar swipe

	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM,
} Bullet;

typedef struct
{
	int iSlot;
	int iPosition;
	int iAmmo1;			  // ammo 1 type
	int iMaxAmmo1;		  // max ammo 1
	int iAmmo2;			  // ammo 2 type
	int iMaxAmmo2;		  // max ammo 2
	const char* pszName;
	int iMaxClip;
	int iId;
	int iFlags;
	int iWeight; // this value used to determine this weapon's importance in autoselection.
} WeaponInfo;

struct AmmoInfo
{
	const char* pszName;
	int iId;

	/**
	*	@brief For exhaustible weapons. If provided, and the player does not have this weapon in their inventory yet it will be given to them.
	*/
	const char* WeaponName = nullptr;
};


// inventory items that commit war crimes
class CBasePlayerWeapon : public CBaseAnimating
{
public:
	void SetObjectCollisionBox() override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	virtual bool CanAddToPlayer(CBasePlayer* player) { return true; } // return true if the weapon you want the weapon added to the player inventory

	// generic weapon versions of CBasePlayerWeapon calls
	virtual void AddToPlayer(CBasePlayer* pPlayer);
	virtual bool AddDuplicate(CBasePlayerWeapon* pOriginal);
	void EXPORT DestroyWeapon();
	void EXPORT DefaultTouch(CBaseEntity* pOther); // default weapon touch
	void EXPORT FallThink(); // when a weapon is first spawned, this think is run to determine when the object has hit the ground.
	void EXPORT Materialize(); // make a weapon visible and tangible
	void EXPORT AttemptToMaterialize(); // the weapon desires to become visible and tangible, if the game rules allow for it
	CBaseEntity* Respawn() override; // copy a weapon
	void FallInit();
	void CheckRespawn();

	virtual bool ExtractAmmo(CBasePlayerWeapon* pWeapon);	  //{ return true; }			// Return true if you can add ammo to yourself when picked up
	virtual bool ExtractClipAmmo(CBasePlayerWeapon* pWeapon); // { return true; }			// Return true if you can add ammo to yourself when picked up

	// generic "shared" ammo handlers
	bool AddPrimaryAmmo(CBasePlayerWeapon* origin, int iCount, int iType, int iMaxClip, int iMaxCarry);
	bool AddSecondaryAmmo(int iCount, int iType, int iMaxCarry);

	virtual void PlayEmptySound();

	virtual void SendWeaponAnim(int iAnim, int body = 0);
	virtual void PlayWeaponSound(int iChannel, const char* szSound, float flVolume = VOL_NORM, float flAttn = ATTN_IDLE, int iFlags = 0, float flPitch = PITCH_NORM);

	virtual bool GetWeaponInfo(WeaponInfo* p) { return false; } // returns false if struct not filled out
	virtual bool CanDeploy();
	virtual bool Deploy() { return true; } // returns is deploy was successful
	virtual bool IsUseable();
	bool DefaultDeploy(const char* szViewModel, const char* szWeaponModel, int iAnim, const char* szAnimExt, int body = 0);
	bool DefaultHolster(int iAnim, int body = 0);
	bool DefaultReload(int iClipSize, int iAnim, int fDelay, int body = 0);

	virtual void WeaponPreFrame() {} // called each frame by the player PreThink
	virtual void WeaponPostFrame(); // called each frame by the player PostThink
	// called by CBasePlayerWeapons WeaponPostFrame()
	virtual void PrimaryAttack() {}						  // do "+ATTACK"
	virtual void SecondaryAttack() {}					  // do "+ATTACK2"
	virtual void Reload() {}							  // do "+RELOAD"
	virtual void WeaponIdle() {}						  // called when no buttons pressed
	void RetireWeapon();

	// Can't use virtual functions as think functions so this wrapper is needed.
	void EXPORT CallDoRetireWeapon()
	{
		DoRetireWeapon();
	}

	virtual void DoRetireWeapon();
	virtual bool ShouldWeaponIdle() { return false; }
	virtual bool CanHolster() { return true; } // can this weapon be put away right now?
	virtual bool Holster();

	void PrintState();

	virtual void Drop();
	virtual void Kill();
	virtual void AttachToPlayer(CBasePlayer* pPlayer);

	virtual void GetWeaponData(weapon_data_t& data) {}

	virtual void SetWeaponData(const weapon_data_t& data) {}

	virtual void DecrementTimers(const int msec) {}

	virtual bool IsReloading() { return m_fInReload; }

	static inline WeaponInfo WeaponInfoArray[MAX_WEAPONS];

	CBasePlayer* m_pPlayer;
	int m_iId;

	int iItemPosition() { return WeaponInfoArray[m_iId].iPosition; }
	int iAmmo1() { return WeaponInfoArray[m_iId].iAmmo1; }
	int iMaxAmmo1() { return WeaponInfoArray[m_iId].iMaxAmmo1; }
	int iAmmo2() { return WeaponInfoArray[m_iId].iAmmo2; }
	int iMaxAmmo2() { return WeaponInfoArray[m_iId].iMaxAmmo2; }
	const char* pszName() { return WeaponInfoArray[m_iId].pszName; }
	int iMaxClip() { return WeaponInfoArray[m_iId].iMaxClip; }
	int iWeight() { return WeaponInfoArray[m_iId].iWeight; }
	int iFlags() { return WeaponInfoArray[m_iId].iFlags; }

	//Hack so deploy animations work when weapon prediction is enabled.
	bool m_ForceSendAnimations = false;

	float m_flPumpTime;
	int m_fInSpecialReload;		   // Are we in the middle of a reload for the shotguns
	int m_iNextPrimaryAttack;   // soonest time WeaponPostFrame will call PrimaryAttack
	int m_iNextSecondaryAttack; // soonest time WeaponPostFrame will call SecondaryAttack
	int m_iTimeWeaponIdle;	   // soonest time WeaponPostFrame will call WeaponIdle
	int m_iClip;				   // number of shots left in the primary weapon clip, -1 it not used
	int m_iClientClip;			   // the last version of m_iClip sent to hud dll
	int m_iClientWeaponState;	   // the last version of the weapon state sent to hud dll (is current weapon, is on target)
	bool m_fInReload;			   // Are we in the middle of a reload;

	int m_iDefaultAmmo; // how much ammo you get when you pick up this weapon as placed by a level designer.

	bool m_bPlayEmptySound;
};


class CBasePlayerAmmo : public CBaseEntity
{
public:
	bool Spawn() override;
	void EXPORT DefaultTouch(CBaseEntity* pOther); // default weapon touch
	virtual bool AddAmmo(CBaseEntity* pOther) { return true; }

	CBaseEntity* Respawn() override;
	void EXPORT Materialize();
};

#define IMPLEMENT_AMMO_CLASS(mapClassName, DLLClassName, modelName, give, ammoType, maxCarry) \
class DLLClassName : public CBasePlayerAmmo \
{ \
	bool Spawn() override \
	{ \
		Precache(); \
		SetModel(modelName); \
		return CBasePlayerAmmo::Spawn(); \
	} \
	void Precache() override \
	{ \
		PRECACHE_MODEL(modelName); \
	} \
	bool AddAmmo(CBaseEntity* pOther) override \
	{ \
		return pOther->GiveAmmo(give, ammoType, maxCarry) != -1; \
	} \
}; \
LINK_ENTITY_TO_CLASS(mapClassName, DLLClassName) \

inline short g_sModelIndexLaser; // holds the index for the laser beam
constexpr const char* g_pModelNameLaser = "sprites/laserbeam.spr";

inline short g_sModelIndexLaserDot;	 // holds the index for the laser beam dot
inline short g_sModelIndexFireball;	 // holds the index for the fireball
inline short g_sModelIndexSmoke;		 // holds the index for the smoke cloud
inline short g_sModelIndexWExplosion; // holds the index for the underwater explosion
inline short g_sModelIndexBubbles;	 // holds the index for the bubbles model
inline short g_sModelIndexBloodDrop;	 // holds the sprite index for blood drops
inline short g_sModelIndexBloodSpray; // holds the sprite index for blood spray (bigger)

extern void ClearMultiDamage();
extern void ApplyMultiDamage(CBaseEntity* inflictor, CBaseEntity* attacker);
extern void AddMultiDamage(CBaseEntity* inflictor, CBaseEntity* attacker, CBaseEntity* pEntity, float flDamage, int bitsDamageType);

extern void DecalGunshot(TraceResult* pTrace, int iBulletType);
extern void SpawnBlood(Vector vecSpot, int bloodColor, float flDamage);
extern int DamageDecal(CBaseEntity* pEntity, int bitsDamageType);
extern void RadiusDamage(Vector vecSrc, CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType);

typedef struct
{
	CBaseEntity* pEntity;
	float amount;
	int type;
} MULTIDAMAGE;

inline MULTIDAMAGE gMultiDamage;


#define LOUD_GUN_VOLUME 1000
#define NORMAL_GUN_VOLUME 600
#define QUIET_GUN_VOLUME 200

#define BRIGHT_GUN_FLASH 512
#define NORMAL_GUN_FLASH 256
#define DIM_GUN_FLASH 128

#define BIG_EXPLOSION_VOLUME 2048
#define NORMAL_EXPLOSION_VOLUME 1024
#define SMALL_EXPLOSION_VOLUME 512

#define WEAPON_ACTIVITY_VOLUME 64

#define VECTOR_CONE_1DEGREES Vector(0.00873, 0.00873, 0.00873)
#define VECTOR_CONE_2DEGREES Vector(0.01745, 0.01745, 0.01745)
#define VECTOR_CONE_3DEGREES Vector(0.02618, 0.02618, 0.02618)
#define VECTOR_CONE_4DEGREES Vector(0.03490, 0.03490, 0.03490)
#define VECTOR_CONE_5DEGREES Vector(0.04362, 0.04362, 0.04362)
#define VECTOR_CONE_6DEGREES Vector(0.05234, 0.05234, 0.05234)
#define VECTOR_CONE_7DEGREES Vector(0.06105, 0.06105, 0.06105)
#define VECTOR_CONE_8DEGREES Vector(0.06976, 0.06976, 0.06976)
#define VECTOR_CONE_9DEGREES Vector(0.07846, 0.07846, 0.07846)
#define VECTOR_CONE_10DEGREES Vector(0.08716, 0.08716, 0.08716)
#define VECTOR_CONE_15DEGREES Vector(0.13053, 0.13053, 0.13053)
#define VECTOR_CONE_20DEGREES Vector(0.17365, 0.17365, 0.17365)

//=========================================================
// CWeaponBox - a single entity that can store weapons
// and ammo.
//=========================================================
class CWeaponBox : public CBaseEntity
{
	void Precache() override;
	bool Spawn() override;
	void Touch(CBaseEntity* pOther) override;
	bool KeyValue(KeyValueData* pkvd) override;
	bool IsEmpty();
	int GiveAmmo(int iCount, int iType, int iMax, int* pIndex = NULL);
	void SetObjectCollisionBox() override;

public:
	void EXPORT Kill();
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	bool HasWeapon(CBasePlayerWeapon* pCheckWeapon);
	bool PackWeapon(CBasePlayerWeapon* pWeapon);
	bool PackAmmo(int iType, int iCount);

	CBasePlayerWeapon* m_rgpPlayerWeapons[MAX_WEAPONS]; // one slot for each

	int m_rgAmmo[MAX_AMMO_SLOTS];	 // ammo quantities

	int m_cAmmoTypes; // how many ammo types packed into this box (if packed by a level designer)
};

#ifdef CLIENT_DLL
void LoadVModel(const char* szViewModel, CBasePlayer* m_pPlayer);
#endif

enum glock_e
{
	GLOCK_IDLE1 = 0,
	GLOCK_IDLE2,
	GLOCK_IDLE3,
	GLOCK_SHOOT,
	GLOCK_SHOOT_EMPTY,
	GLOCK_RELOAD,
	GLOCK_RELOAD_NOT_EMPTY,
	GLOCK_DRAW,
	GLOCK_HOLSTER,
	GLOCK_ADD_SILENCER
};

class CGlock : public CBasePlayerWeapon
{
public:
	bool Spawn() override;
	void Precache() override;
	bool GetWeaponInfo(WeaponInfo* p) override;

	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void GlockFire(float flSpread, int fCycleTime, bool fUseAutoAim);
	bool Deploy() override;
	bool Holster() override;
	void Reload() override;
	void WeaponIdle() override;

private:
	unsigned short m_usFireGlock;
};

enum crowbar_e
{
	CROWBAR_IDLE = 0,
	CROWBAR_DRAW,
	CROWBAR_HOLSTER,
	CROWBAR_ATTACK1HIT,
	CROWBAR_ATTACK1MISS,
	CROWBAR_ATTACK2MISS,
	CROWBAR_ATTACK2HIT,
	CROWBAR_ATTACK3MISS,
	CROWBAR_ATTACK3HIT
};

class CCrowbar : public CBasePlayerWeapon
{
public:
	bool Spawn() override;
	void Precache() override;
	void EXPORT SwingAgain();
	void EXPORT Smack();
	bool GetWeaponInfo(WeaponInfo* p) override;

	void PrimaryAttack() override;
	bool Swing(bool fFirst);
	bool Deploy() override;
	bool Holster() override;
	int m_iSwing;
	TraceResult m_trHit;

private:
	unsigned short m_usCrowbar;
};

enum python_e
{
	PYTHON_IDLE1 = 0,
	PYTHON_FIDGET,
	PYTHON_FIRE1,
	PYTHON_RELOAD,
	PYTHON_HOLSTER,
	PYTHON_DRAW,
	PYTHON_IDLE2,
	PYTHON_IDLE3
};

class CPython : public CBasePlayerWeapon
{
public:
	bool Spawn() override;
	void Precache() override;
	bool GetWeaponInfo(WeaponInfo* p) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	bool Deploy() override;
	bool Holster() override;
	void Reload() override;
	void WeaponIdle() override;

private:
	unsigned short m_usFirePython;
};

enum mp5_e
{
	MP5_LONGIDLE = 0,
	MP5_IDLE1,
	MP5_LAUNCH,
	MP5_RELOAD,
	MP5_DEPLOY,
	MP5_FIRE1,
	MP5_FIRE2,
	MP5_FIRE3,
	MP5_HOLSTER,
};

class CMP5 : public CBasePlayerWeapon
{
public:
	bool Spawn() override;
	void Precache() override;
	bool GetWeaponInfo(WeaponInfo* p) override;
	bool AddDuplicate(CBasePlayerWeapon* pOriginal) override;

	void PrimaryAttack() override;
	void SecondaryAttack() override;
	bool Deploy() override;
	bool Holster() override;
	void Reload() override;
	void WeaponIdle() override;

private:
	unsigned short m_usMP5;
	unsigned short m_usMP52;
};

enum crossbow_e
{
	CROSSBOW_IDLE1 = 0, // full
	CROSSBOW_IDLE2,		// empty
	CROSSBOW_FIDGET1,	// full
	CROSSBOW_FIDGET2,	// empty
	CROSSBOW_FIRE1,		// full
	CROSSBOW_FIRE2,		// reload
	CROSSBOW_FIRE3,		// empty
	CROSSBOW_RELOAD,	// from empty
	CROSSBOW_DRAW1,		// full
	CROSSBOW_DRAW2,		// empty
	CROSSBOW_HOLSTER1,	// full
	CROSSBOW_HOLSTER2,	// empty
};

class CCrossbow : public CBasePlayerWeapon
{
public:
	bool Spawn() override;
	void Precache() override;
	bool GetWeaponInfo(WeaponInfo* p) override;

	void FireBolt();
	void FireSniperBolt();
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	bool Deploy() override;
	bool Holster() override;
	void Reload() override;
	void WeaponIdle() override;

private:
	unsigned short m_usCrossbow;
	unsigned short m_usCrossbow2;
};

enum shotgun_e
{
	SHOTGUN_IDLE = 0,
	SHOTGUN_FIRE,
	SHOTGUN_FIRE2,
	SHOTGUN_RELOAD,
	SHOTGUN_PUMP,
	SHOTGUN_START_RELOAD,
	SHOTGUN_DRAW,
	SHOTGUN_HOLSTER,
	SHOTGUN_IDLE4,
	SHOTGUN_IDLE_DEEP
};

class CShotgun : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];
#endif


	bool Spawn() override;
	void Precache() override;
	bool GetWeaponInfo(WeaponInfo* p) override;

	void PrimaryAttack() override;
	void SecondaryAttack() override;
	bool Deploy() override;
	bool Holster() override;
	void Reload() override;
	void WeaponIdle() override;
	void WeaponPostFrame() override;

	void GetWeaponData(weapon_data_t& data) override;
	void SetWeaponData(const weapon_data_t& data) override;
	void DecrementTimers(const int msec) override;
	bool IsReloading() override { return m_fInSpecialReload != 0; }

private:
	unsigned short m_usDoubleFire;
	unsigned short m_usSingleFire;
};

class CLaserSpot : public CBaseEntity
{
	bool Spawn() override;
	void Precache() override;

	int ObjectCaps() override { return FCAP_DONT_SAVE; }

public:
	void Suspend(float flSuspendTime);
	void EXPORT Revive();

	static CLaserSpot* CreateSpot(CBaseEntity* pOwner);
};

enum rpg_e
{
	RPG_IDLE = 0,
	RPG_FIDGET,
	RPG_RELOAD,	   // to reload
	RPG_FIRE2,	   // to empty
	RPG_HOLSTER1,  // loaded
	RPG_DRAW1,	   // loaded
	RPG_HOLSTER2,  // unloaded
	RPG_DRAW_UL,   // unloaded
	RPG_IDLE_UL,   // unloaded idle
	RPG_FIDGET_UL, // unloaded fidget
};

class CRpg : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];
#endif

	bool Spawn() override;
	void Precache() override;
	void Reload() override;
	bool GetWeaponInfo(WeaponInfo* p) override;

	bool Deploy() override;
	bool CanHolster() override;
	bool Holster() override;

	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void WeaponIdle() override;

	void UpdateSpot();
	bool ShouldWeaponIdle() override { return true; }

	CLaserSpot* m_pSpot;
	bool m_fSpotActive;
	int m_cActiveRockets; // how many missiles in flight from this launcher right now?

private:
	void ToggleLaserDot(bool bOn, bool bSound = true);
	void SuspendLaserDot(float flSuspendTime);

	unsigned short m_usRpg;
	unsigned short m_usLaserDotOn;
	unsigned short m_usLaserDotOff;
};

class CRpgRocket : public CGrenade
{
public:
	~CRpgRocket() override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];
	bool Spawn() override;
	void Precache() override;
	void EXPORT FollowThink();
	void EXPORT IgniteThink();
	void EXPORT RocketTouch(CBaseEntity* pOther);
	static CRpgRocket* CreateRpgRocket(Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner, CRpg* pLauncher);

	int m_iTrail;
	float m_flIgniteTime;
	EHANDLE m_pLauncher; // handle back to the launcher that fired me.
};

#define GAUSS_PRIMARY_CHARGE_VOLUME 256 // how loud gauss is while charging
#define GAUSS_PRIMARY_FIRE_VOLUME 450	// how loud gauss is when discharged

enum gauss_e
{
	GAUSS_IDLE = 0,
	GAUSS_IDLE2,
	GAUSS_FIDGET,
	GAUSS_SPINUP,
	GAUSS_SPIN,
	GAUSS_FIRE,
	GAUSS_FIRE2,
	GAUSS_HOLSTER,
	GAUSS_DRAW
};

class CGauss : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];
#endif

	bool Spawn() override;
	void Precache() override;
	bool GetWeaponInfo(WeaponInfo* p) override;

	bool Deploy() override;
	bool Holster() override;

	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void WeaponIdle() override;

	void StartFire();
	void Fire(Vector vecOrigSrc, Vector vecDirShooting, float flDamage);
	float GetFullChargeTime();
	int m_iBalls;
	int m_iGlow;
	int m_iBeam;
	int m_iSoundState; // don't save this

	// was this weapon just fired primary or secondary?
	// we need to know so we can pick the right set of effects.
	bool m_fPrimaryFire;

private:
	void SendStopEvent(bool sendToHost);

private:
	unsigned short m_usGaussFire;
	unsigned short m_usGaussSpin;
};

enum egon_e
{
	EGON_IDLE1 = 0,
	EGON_FIDGET1,
	EGON_ALTFIREON,
	EGON_ALTFIRECYCLE,
	EGON_ALTFIREOFF,
	EGON_FIRE1,
	EGON_FIRE2,
	EGON_FIRE3,
	EGON_FIRE4,
	EGON_DRAW,
	EGON_HOLSTER
};

enum EGON_FIRESTATE
{
	FIRE_OFF,
	FIRE_CHARGE
};

enum EGON_FIREMODE
{
	FIRE_NARROW,
	FIRE_WIDE
};

#define EGON_PRIMARY_VOLUME 450
#define EGON_BEAM_SPRITE "sprites/xbeam1.spr"
#define EGON_FLARE_SPRITE "sprites/XSpark1.spr"
#define EGON_SOUND_OFF "weapons/egon_off1.wav"
#define EGON_SOUND_RUN "weapons/egon_run3.wav"
#define EGON_SOUND_STARTUP "weapons/egon_windup2.wav"

class CEgon : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];
#endif

	bool Spawn() override;
	void Precache() override;
	bool GetWeaponInfo(WeaponInfo* p) override;

	bool Deploy() override;
	bool CanHolster() override;
	bool Holster() override;

	void UpdateEffect(const Vector& startPoint, const Vector& endPoint, float timeBlend);

	void CreateEffect();
	void DestroyEffect();

	void EndAttack(bool bSendAnim = true);
	void Attack();
	void PrimaryAttack() override;
	bool ShouldWeaponIdle() override { return true; }
	void WeaponIdle() override;

	float m_flAmmoUseTime; // since we use < 1 point of ammo per update, we subtract ammo on a timer.

	float GetPulseInterval();
	float GetDischargeInterval();

	void Fire(const Vector& vecOrigSrc, const Vector& vecDir);

	bool HasAmmo();

	void UseAmmo(int count);

	CBeam* m_pBeam;
	CBeam* m_pNoise;
	CSprite* m_pSprite;

	unsigned short m_usEgonStop;

private:
	float m_shootTime;
	EGON_FIREMODE m_fireMode;
	float m_shakeTime;
	bool m_deployed;

	unsigned short m_usEgonFire;
};

enum hgun_e
{
	HGUN_IDLE1 = 0,
	HGUN_FIDGETSWAY,
	HGUN_FIDGETSHAKE,
	HGUN_DOWN,
	HGUN_UP,
	HGUN_SHOOT
};

class CHgun : public CBasePlayerWeapon
{
public:
	bool Spawn() override;
	void Precache() override;
	bool GetWeaponInfo(WeaponInfo* p) override;
	void AddToPlayer(CBasePlayer* pPlayer) override;

	void PrimaryAttack() override;
	void SecondaryAttack() override;
	bool Deploy() override;
	bool IsUseable() override;
	bool Holster() override;
	void Reload() override;
	void WeaponIdle() override;
	void WeaponPostFrame() override;

	int m_iFirePhase; // don't save me.

private:
	int GetChargeInterval();

	unsigned short m_usHornetFire;
};

enum handgrenade_e
{
	HANDGRENADE_IDLE = 0,
	HANDGRENADE_FIDGET,
	HANDGRENADE_PINPULL,
	HANDGRENADE_THROW1, // toss
	HANDGRENADE_THROW2, // medium
	HANDGRENADE_THROW3, // hard
	HANDGRENADE_HOLSTER,
	HANDGRENADE_DRAW
};

class CHandGrenade : public CBasePlayerWeapon
{
public:
	bool Spawn() override;
	void Precache() override;
	bool GetWeaponInfo(WeaponInfo* p) override;

	void PrimaryAttack() override;
	bool Deploy() override;
	bool CanHolster() override;
	bool Holster() override;
	void WeaponIdle() override;
};

enum satchel_e
{
	SATCHEL_IDLE1 = 0,
	SATCHEL_FIDGET1,
	SATCHEL_DRAW,
	SATCHEL_DROP
};

enum satchel_radio_e
{
	SATCHEL_RADIO_IDLE1 = 0,
	SATCHEL_RADIO_FIDGET1,
	SATCHEL_RADIO_DRAW,
	SATCHEL_RADIO_FIRE,
	SATCHEL_RADIO_HOLSTER
};

class CSatchel : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];
#endif

	bool Spawn() override;
	void Precache() override;
	bool GetWeaponInfo(WeaponInfo* p) override;
	void AddToPlayer(CBasePlayer* pPlayer) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	bool AddDuplicate(CBasePlayerWeapon* pOriginal) override;
	bool CanDeploy() override;
	bool Deploy() override;
	bool IsUseable() override;

	bool Holster() override;
	void WeaponIdle() override;
	void Throw();
};

enum tripmine_e
{
	TRIPMINE_IDLE1 = 0,
	TRIPMINE_IDLE2,
	TRIPMINE_ARM1,
	TRIPMINE_ARM2,
	TRIPMINE_FIDGET,
	TRIPMINE_HOLSTER,
	TRIPMINE_DRAW,
	TRIPMINE_WORLD,
	TRIPMINE_GROUND,
};

class CTripmine : public CBasePlayerWeapon
{
public:
	bool Spawn() override;
	void Precache() override;
	bool GetWeaponInfo(WeaponInfo* p) override;
	void SetObjectCollisionBox() override
	{
		pev->absmin = pev->origin + Vector(-16, -16, -5);
		pev->absmax = pev->origin + Vector(16, 16, 28);
	}

	void PrimaryAttack() override;
	bool Deploy() override;
	bool Holster() override;
	void WeaponIdle() override;

private:
	unsigned short m_usTripFire;
};

enum squeak_e
{
	SQUEAK_IDLE1 = 0,
	SQUEAK_FIDGETFIT,
	SQUEAK_FIDGETNIP,
	SQUEAK_DOWN,
	SQUEAK_UP,
	SQUEAK_THROW
};

class CSqueak : public CBasePlayerWeapon
{
public:
	bool Spawn() override;
	void Precache() override;
	bool GetWeaponInfo(WeaponInfo* p) override;

	void PrimaryAttack() override;
	void SecondaryAttack() override;
	bool Deploy() override;
	bool Holster() override;
	void WeaponIdle() override;
	bool m_fJustThrown;

private:
	unsigned short m_usSnarkFire;
};
