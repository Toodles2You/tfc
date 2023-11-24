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

// Contact Grenade / Timed grenade / Satchel Charge
class CGrenade : public CBaseAnimating
{
public:
	bool Spawn() override;

	static CGrenade* ShootTimed(CBaseEntity* owner, Vector vecStart, Vector vecVelocity, float time);
	static CGrenade* ShootContact(CBaseEntity* owner, Vector vecStart, Vector vecVelocity);

	void Explode(Vector vecSrc, Vector vecAim);
	void Explode(TraceResult* pTrace, int bitsDamageType);

	void EXPORT BounceTouch(CBaseEntity* pOther);
	void EXPORT SlideTouch(CBaseEntity* pOther);
	void EXPORT ExplodeTouch(CBaseEntity* pOther);
	void EXPORT Detonate();
	void EXPORT DetonateUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT TumbleThink();

	virtual void BounceSound();
	void Killed(CBaseEntity* inflictor, CBaseEntity* attacker, int bitsDamageType) override;

	float m_flNextAttack;
};


// constant items
#define ITEM_HEALTHKIT 1
#define ITEM_ANTIDOTE 2
#define ITEM_SECURITY 3
#define ITEM_BATTERY 4

#define MAX_NORMAL_BATTERY 100.0F

#define WEAPON_NOCLIP -1

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

extern void RadiusDamage(Vector vecSrc, CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, float flRadius, int bitsDamageType);

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
