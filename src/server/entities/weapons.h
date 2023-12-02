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

enum WeaponAnim
{
	kWeaponAnimIdle = 0,
	kWeaponAnimDeploy,
	kWeaponAnimHolster,

	kWeaponAnimLast
};

typedef struct
{
	int iSlot;
	int iPosition;
	int iAmmo1;
	int iMaxAmmo1;
	int iAmmo2;
	int iMaxAmmo2;
	const char* pszName;
	int iMaxClip;
	int iFlags;
	int iWeight;
	const char* pszWorld;
	const char* pszView;
	const char* pszPlayer;
	const char* pszAnimExt;
	int iAnims[kWeaponAnimLast];
} WeaponInfo;

// inventory items that commit war crimes
class CBasePlayerWeapon : public CBaseAnimating
{
public:
	void SetObjectCollisionBox() override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	virtual bool Spawn() override;

	virtual bool CanAddToPlayer(CBasePlayer* player) { return true; } // return true if the weapon you want the weapon added to the player inventory

	// generic weapon versions of CBasePlayerWeapon calls
	virtual void AddToPlayer(CBasePlayer* pPlayer);
	virtual void RemoveFromPlayer();
	virtual bool AddDuplicate(CBasePlayerWeapon* pOriginal) { return false; }
#ifndef CLIENT_DLL
	virtual void UpdateOnRemove() override;
#endif
	void EXPORT DefaultTouch(CBaseEntity* pOther); // default weapon touch
	void EXPORT Materialize(); // make a weapon visible and tangible
	void EXPORT AttemptToMaterialize(); // the weapon desires to become visible and tangible, if the game rules allow for it
	CBaseEntity* Respawn() override; // copy a weapon
	void CheckRespawn();

	virtual void SendWeaponAnim(int iAnim);
	virtual void PlayWeaponSound(int iChannel, const char* szSound, float flVolume = VOL_NORM, float flAttn = ATTN_IDLE, int iFlags = 0, float flPitch = PITCH_NORM);

	virtual bool CanDeploy() { return true; }
	virtual void Deploy();

	virtual void WeaponPostFrame() = 0;

	virtual bool CanHolster() { return true; } // can this weapon be put away right now?
	virtual void Holster();

	virtual void GetWeaponData(weapon_data_t& data);
	virtual void SetWeaponData(const weapon_data_t& data);
	virtual void DecrementTimers(const int msec);

	static inline WeaponInfo WeaponInfoArray[WEAPON_LAST];

	CBasePlayer* m_pPlayer;

	virtual int GetID() const = 0;
	virtual bool GetWeaponInfo(WeaponInfo* p) = 0;

	WeaponInfo& GetInfo() { return WeaponInfoArray[GetID()]; }
	int iAmmo1() { return GetInfo().iAmmo1; }
	int iMaxAmmo1() { return GetInfo().iMaxAmmo1; }
	int iAmmo2() { return GetInfo().iAmmo2; }
	int iMaxAmmo2() { return GetInfo().iMaxAmmo2; }
	const char* pszName() { return GetInfo().pszName; }
	int iMaxClip() { return GetInfo().iMaxClip; }
	int iWeight() { return GetInfo().iWeight; }
	int iFlags() { return GetInfo().iFlags; }

	//Hack so deploy animations work when weapon prediction is enabled.
	bool m_ForceSendAnimations = false;

	int m_iNextPrimaryAttack;
	int m_iClip;
	bool m_fInReload;
};

inline short g_sModelIndexPlayer;
inline short g_sModelIndexGibs;
inline short g_sModelIndexShell;
inline short g_sModelIndexLaser; // holds the index for the laser beam
inline short g_sModelIndexLaserDot;	 // holds the index for the laser beam dot
inline short g_sModelIndexFireball;	 // holds the index for the fireball
inline short g_sModelIndexSmoke;		 // holds the index for the smoke cloud
inline short g_sModelIndexWExplosion; // holds the index for the underwater explosion
inline short g_sModelIndexBubbles;	 // holds the index for the bubbles model
inline short g_sModelIndexBloodDrop;	 // holds the sprite index for blood drops
inline short g_sModelIndexBloodSpray; // holds the sprite index for blood spray (bigger)

class CCrowbar : public CBasePlayerWeapon
{
public:
	enum
	{
		kAnimIdle = 0,
		kAnimDeploy,
		kAnimHolster,
		kAnimAttack1Hit,
		kAnimAttack1Miss,
		kAnimAttack2Miss,
		kAnimAttack2Hit,
		kAnimAttack3Miss,
		kAnimAttack3Hit
	};

	enum
	{
		kCrowbarMiss = 0,
		kCrowbarHitWorld,
		kCrowbarHitPlayer,
	};

	int GetID() const override { return WEAPON_CROWBAR; }
	bool GetWeaponInfo(WeaponInfo* i) override;

	void Precache() override;

	void PrimaryAttack();
	void WeaponPostFrame() override;

private:
	unsigned short m_usCrowbar;
};

class CMP5 : public CBasePlayerWeapon
{
public:
	enum
	{
		kAnimLongidle = 0,
		kAnimIdle1,
		kAnimLaunch,
		kAnimReload,
		kAnimDeploy,
		kAnimFire1,
		kAnimFire2,
		kAnimFire3,
		kAnimHolster,
	};

	int GetID() const override { return WEAPON_MP5; }
	bool GetWeaponInfo(WeaponInfo* i) override;

	void Precache() override;

	void PrimaryAttack();
	void SecondaryAttack();
	void WeaponPostFrame() override;

private:
	unsigned short m_usMP5;
	unsigned short m_usMP52;
};

#ifndef CLIENT_DLL

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

class CBasePlayerAmmo : public CBaseEntity
{
public:
	bool Spawn() override;
	void EXPORT DefaultTouch(CBaseEntity* pOther); // default weapon touch
	virtual bool AddAmmo(CBaseEntity* pOther) { return true; }

	CBaseEntity* Respawn() override;
	void EXPORT Materialize();
};

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

	CBasePlayerWeapon* m_rgpPlayerWeapons[WEAPON_LAST]; // one slot for each

	int m_rgAmmo[AMMO_LAST];	 // ammo quantities

	int m_cAmmoTypes; // how many ammo types packed into this box (if packed by a level designer)
};

#endif
