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

#include "weaponinfo.h"
#ifdef CLIENT_DLL
#include "event_args.h"
#endif

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
	kWeaponAnimAttack,
	kWeaponAnimReload,
	kWeaponAnimStartReload,
	kWeaponAnimEndReload,

	kWeaponAnimLast
};

enum
{
	kProjBullet = 0,
	kProjRocket,
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

	int iShots;
	int iAttackTime;
	int iReloadTime;
	int iProjectileType;
	int iProjectileDamage;
	Vector2D vecProjectileSpread;
	int iProjectileCount;
	const char* pszEvent;
	const char* pszAttackSound;
	float flPunchAngle;
} WeaponInfo;

#ifdef GAME_DLL
constexpr const char* g_szWeaponNames[] =
{
	nullptr,
	nullptr,
	nullptr,
	"tf_weapon_medikit",
	"tf_weapon_spanner",
	"tf_weapon_axe",
	"tf_weapon_sniperrifle",
	"tf_weapon_autorifle",
	"tf_weapon_shotgun",
	"tf_weapon_supershotgun",
	"tf_weapon_ng",
	"tf_weapon_superng",
	"tf_weapon_gl",
	"tf_weapon_flamethrower",
	"tf_weapon_rpg",
	"tf_weapon_ic",
	"tf_weapon_ac",
	nullptr,
	nullptr,
	"tf_weapon_tranq",
	"tf_weapon_railgun",
	"tf_weapon_pl",
	"tf_weapon_knife",
};
#endif

// inventory items that commit war crimes
class CBasePlayerWeapon : public CBaseAnimating
{
public:
	DECLARE_SAVERESTORE()

	void SetObjectCollisionBox() override;

	virtual bool Spawn() override;
	virtual void Precache() override;

	// generic weapon versions of CBasePlayerWeapon calls
	virtual bool AddToPlayer(CBasePlayer* pPlayer);
	virtual void RemoveFromPlayer();
	virtual bool AddDuplicate(CBasePlayerWeapon* pOriginal) { return false; }
#ifndef CLIENT_DLL
	virtual void UpdateOnRemove() override;
#endif
	void EXPORT DefaultTouch(CBaseEntity* pOther);
	void EXPORT Materialize();
	void EXPORT AttemptToMaterialize();
	CBaseEntity* Respawn() override;
	void CheckRespawn();

	virtual void SendWeaponAnim(int iAnim);

	virtual bool CanDeploy() { return true; }
	virtual void Deploy();

	virtual void WeaponPostFrame() = 0;

	virtual bool CanHolster() { return true; }
	virtual void Holster();

	virtual void GetWeaponData(weapon_data_t& data);
	virtual void SetWeaponData(const weapon_data_t& data);
	virtual void DecrementTimers(const int msec);

	static inline WeaponInfo WeaponInfoArray[WEAPON_LAST];

	CBasePlayer* m_pPlayer;

	virtual int GetID() const = 0;
	virtual void GetWeaponInfo(WeaponInfo& i) = 0;

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

	enum
	{
		kWpnStateReloading = 1,
	};

	int m_iWeaponState;
};

inline short g_sModelIndexPlayer;
inline short g_sModelIndexGibs;
inline short g_sModelIndexShell;
inline short g_sModelIndexLaser;
inline short g_sModelIndexLaserDot;
inline short g_sModelIndexFireball;
inline short g_sModelIndexSmoke;
inline short g_sModelIndexWExplosion;
inline short g_sModelIndexBubbles;
inline short g_sModelIndexBloodDrop;
inline short g_sModelIndexBloodSpray;

class CTFWeapon : public CBasePlayerWeapon
{
public:
	void Precache() override;

	virtual void PrimaryAttack();
	void WeaponPostFrame() override;

#ifdef CLIENT_DLL
	static void EV_PrimaryAttack(event_args_t* args);
#endif

private:
	unsigned short m_usPrimaryAttack;
};

class CShotgun : public CTFWeapon
{
public:
	int GetID() const override { return WEAPON_TF_SHOTGUN; }
	void GetWeaponInfo(WeaponInfo& i) override;
};

class CSuperShotgun : public CTFWeapon
{
public:
	int GetID() const override { return WEAPON_SUPER_SHOTGUN; }
	void GetWeaponInfo(WeaponInfo& i) override;
};

class CRocketLauncher : public CTFWeapon
{
public:
	int GetID() const override { return WEAPON_ROCKET_LAUNCHER; }
	void GetWeaponInfo(WeaponInfo& i) override;
};

#ifdef GAME_DLL

// Contact Grenade / Timed grenade / Satchel Charge
class CGrenade : public CBaseAnimating
{
public:
	bool Spawn() override;

	static CGrenade* ShootContact(CBaseEntity* owner, Vector vecStart, Vector vecVelocity);

	void Explode(Vector vecSrc, Vector vecAim);
	void Explode(TraceResult* pTrace, int bitsDamageType);

	void EXPORT BounceTouch(CBaseEntity* pOther);
	void EXPORT ExplodeTouch(CBaseEntity* pOther);
	void EXPORT Detonate();
	void EXPORT TumbleThink();

	virtual void BounceSound();
};

/* Team Fortress style timed grenade */
class CPrimeGrenade : public CGrenade
{
public:
	bool Spawn() override;
	void EXPORT PrimedThink();

	static CPrimeGrenade* PrimeGrenade(CBaseEntity* owner);

protected:
	typedef enum
	{
		kThrow,
		kDrop,
		kOvercook,
	} throw_e;

	void Throw(throw_e mode);
};

class CRocket : public CBaseEntity
{
public:
	bool Spawn() override;

	static CRocket* CreateRocket(const Vector& origin, const Vector& dir, const float damage, CBaseEntity* owner);
	void EXPORT RocketTouch(CBaseEntity *pOther);
};

void RadiusDamage(
	const Vector& origin,
	CBaseEntity* inflictor,
	CBaseEntity* attacker,
	const float damage,
	const float radius,
	const int damageType);

#endif
