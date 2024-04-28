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

#ifdef GAME_DLL
#include "effects.h"

#include <queue>

class CPipeBomb;
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
	kProjNail,
	kProjPipeBomb,
	kProjPipeBombRemote,
	kProjKinetic,
	kProjAdrenaline,
};

typedef struct
{
	int iSlot;
	int iPosition;
	int iAmmo1;
	int iAmmo2;
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
	union {
		int iProjectileChargeDamage;
		int iProjectileDamageMin;
	};
	int iProjectileRadius;
	const char* pszEvent;
	const char* pszAttackSound;
	const char* pszAlternateSound;
	const char* pszReloadSound;
	float flPunchAngle;
	int iSibling;
	bool bShouldIdle;
} WeaponInfo;

#ifdef GAME_DLL
constexpr const char* g_szWeaponNames[WEAPON_TYPES] =
{
	// nullptr,
	// nullptr,
	"tf_weapon_medikit",
	// "tf_weapon_spanner",
	"tf_weapon_axe",
	"tf_weapon_sniperrifle",
	"tf_weapon_autorifle",
	"tf_weapon_shotgun",
	"tf_weapon_supershotgun",
	"tf_weapon_ng",
	"tf_weapon_superng",
	"tf_weapon_gl",
	// "tf_weapon_flamethrower",
	"tf_weapon_rpg",
	// "tf_weapon_ic",
	// "tf_weapon_ac",
	// nullptr,
	// nullptr,
	// "tf_weapon_tranq",
	// "tf_weapon_railgun",
	"tf_weapon_pl",
	// "tf_weapon_knife",
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

	static inline WeaponInfo WeaponInfoArray[WEAPON_TYPES];

	CBasePlayer* m_pPlayer;

	virtual int GetID() const = 0;
	virtual void GetWeaponInfo(WeaponInfo& i) = 0;

	WeaponInfo& GetInfo() { return WeaponInfoArray[GetID()]; }
	int iAmmo1() { return GetInfo().iAmmo1; }
	int iAmmo2() { return GetInfo().iAmmo2; }
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
		kWpnStateEmptySound = 2,
		kWpnStateIdle = 4,
	};

	int m_iWeaponState;
};

inline short g_sModelIndexPlayer;
inline short g_sModelIndexGibs;
inline short g_sModelIndexShell;
inline short g_sModelIndexShotgunShell;
inline short g_sModelIndexLaser;
inline short g_sModelIndexLaserDot;
inline short g_sModelIndexFireball;
inline short g_sModelIndexSmoke;
inline short g_sModelIndexWExplosion;
inline short g_sModelIndexBubbles;
inline short g_sModelIndexBloodDrop;
inline short g_sModelIndexBloodSpray;
inline short g_sModelIndexSmokeTrail;
inline short g_sModelIndexNail;
inline short g_sModelIndexSaveMe;
inline short g_sModelIndexFlare;

class CTFWeapon : public CBasePlayerWeapon
{
public:
	void Precache() override;

	void Deploy() override;

	virtual void PrimaryAttack();
	void WeaponPostFrame() override;

	void Holster() override;

#ifdef CLIENT_DLL
	static void EV_PrimaryAttack(event_args_t* args);
#endif

	CTFWeapon* GetSibling();

protected:
	void UpdateSiblingInfo(const bool holster);

protected:
	unsigned short m_usPrimaryAttack;
};

class CTFMelee : public CTFWeapon
{
public:
	enum
	{
		kResultMiss = 0,
		kResultHit,
		kResultHitWorld
	};

	void PrimaryAttack() override;
	void WeaponPostFrame() override;

#ifdef CLIENT_DLL
	static void EV_MeleeAttack(event_args_t* args);
#endif

#ifdef GAME_DLL
protected:
	virtual int HitEntity(CBaseEntity* hit, const Vector& dir, const TraceResult& tr);
#endif
};

class CMedikit : public CTFMelee
{
public:
	int GetID() const override { return WEAPON_MEDIKIT; }
	void GetWeaponInfo(WeaponInfo& i) override;

#ifdef GAME_DLL
protected:
	int HitEntity(CBaseEntity* hit, const Vector& dir, const TraceResult& tr) override;
#endif
};

class CAxe : public CTFMelee
{
public:
	int GetID() const override { return WEAPON_AXE; }
	void GetWeaponInfo(WeaponInfo& i) override;
};

class CSniperRifle : public CTFWeapon
{
public:
	int GetID() const override { return WEAPON_SNIPER_RIFLE; }
	void GetWeaponInfo(WeaponInfo& i) override;

	void Precache() override;

	void PrimaryAttack() override;
	void WeaponPostFrame() override;

	void Holster() override;

	void GetWeaponData(weapon_data_t& data) override;
	void SetWeaponData(const weapon_data_t& data) override;
	void DecrementTimers(const int msec) override;

protected:
	int m_iScopeTime;

	void CreateLaserEffect();
	void DestroyLaserEffect(const bool sendMessage = true);

#ifdef GAME_DLL
	CSprite* m_pLaserDot = nullptr;
	CBeam* m_pLaserBeam = nullptr;

	void UpdateLaserEffect();
#endif

	unsigned short m_usLaserDotOn;
	unsigned short m_usLaserDotOff;
};

class CAutoRifle : public CTFWeapon
{
public:
	int GetID() const override { return WEAPON_AUTO_RIFLE; }
	void GetWeaponInfo(WeaponInfo& i) override;
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

class CNailgun : public CTFWeapon
{
public:
	int GetID() const override { return WEAPON_NAILGUN; }
	void GetWeaponInfo(WeaponInfo& i) override;
};

class CSuperNailgun : public CTFWeapon
{
public:
	int GetID() const override { return WEAPON_SUPER_NAILGUN; }
	void GetWeaponInfo(WeaponInfo& i) override;
};

class CGrenadeLauncher : public CTFWeapon
{
public:
	int GetID() const override { return WEAPON_GRENADE_LAUNCHER; }
	void GetWeaponInfo(WeaponInfo& i) override;
};

class CRocketLauncher : public CTFWeapon
{
public:
	int GetID() const override { return WEAPON_ROCKET_LAUNCHER; }
	void GetWeaponInfo(WeaponInfo& i) override;
};

class CPipeBombLauncher : public CTFWeapon
{
public:
	int GetID() const override { return WEAPON_PIPEBOMB_LAUNCHER; }
	void GetWeaponInfo(WeaponInfo& i) override;

#ifdef GAME_DLL
public:
	~CPipeBombLauncher() override;

	void DetonatePipeBombs(const bool fizzle = false);
	void AddPipeBomb(CPipeBomb* pipebomb);

private:
	static constexpr int kMaxPipeBombs = 8;

	std::queue<EHANDLE> m_pPipeBombs;
#endif
};

#ifdef GAME_DLL

// Contact Grenade / Timed grenade / Satchel Charge
class CGrenade : public CBaseAnimating
{
public:
	bool Spawn() override;

	static CGrenade* ShootContact(CBaseEntity* owner, Vector vecStart, Vector vecVelocity);

	void Explode(Vector vecSrc, Vector vecAim);
	virtual void Explode(TraceResult* pTrace, int bitsDamageType);

	virtual bool CanDetonate() { return true; }
	void EXPORT BounceTouch(CBaseEntity* pOther);
	void EXPORT ExplodeTouch(CBaseEntity* pOther);
	void EXPORT Detonate();
	void EXPORT TumbleThink();

	virtual void BounceSound();
};

#ifdef HALFLIFE_GRENADES

/* Team Fortress style timed grenade */
class CPrimeGrenade : public CGrenade
{
public:
	bool Spawn() override;
	void EXPORT PrimedThink();

	bool CanDetonate() override;
	virtual void Explode(TraceResult* pTrace, int bitsDamageType) override;

	virtual const char* GetModelName() { return "models/w_grenade.mdl"; }
	virtual const char* GetIconName() { return "grenade"; }

	static CPrimeGrenade* PrimeGrenade(CBaseEntity* owner);

protected:
	typedef enum
	{
		kThrow,
		kDrop,
		kOvercook,
	} throw_e;

	virtual void Throw(throw_e mode);
};

class CCaltropCanister : public CPrimeGrenade
{
public:
	bool Spawn() override;

	const char* GetModelName() override { return "models/caltrop.mdl"; }
	const char* GetIconName() override { return "d_caltrop"; }

	static CCaltropCanister* CaltropCanister(CBaseEntity* owner);

protected:
	void Throw(throw_e mode) override;

private:
	static constexpr int kNumCaltrops = 6;
};

class CCaltrop : public CPrimeGrenade
{
public:
	bool Spawn() override;
	void EXPORT CaltropThink();
	void EXPORT CaltropTouch(CBaseEntity* other);

	const char* GetModelName() override { return "models/caltrop.mdl"; }
	const char* GetIconName() override { return "d_caltrop"; }

	static CCaltrop* Caltrop(CBaseEntity* owner, const Vector& origin, const float yaw);
};

class CConcussionGrenade : public CPrimeGrenade
{
public:
	void Explode(TraceResult* pTrace, int bitsDamageType) override;

	const char* GetModelName() override { return "models/conc_grenade.mdl"; }
	const char* GetIconName() override { return "d_concussiongrenade"; }

	static CConcussionGrenade* ConcussionGrenade(CBaseEntity* owner);
};

class CFlare : public CPrimeGrenade
{
public:
	bool Spawn() override;

	void EXPORT StickyTouch(CBaseEntity* pOther);

	void Explode(TraceResult* pTrace, int bitsDamageType) override;

	const char* GetModelName() override { return "models/w_flare.mdl"; }

	static CFlare* Flare(CBaseEntity* owner);

protected:
	void Throw(throw_e mode) override;
};

class CNailGrenade : public CPrimeGrenade
{
public:
	bool Spawn() override;
	void Explode(TraceResult* pTrace, int bitsDamageType) override;
	void EXPORT GetReadyToNail();
	void EXPORT GetNailedIdiot();

	const char* GetModelName() override { return "models/ngrenade.mdl"; }
	const char* GetIconName() override { return "d_nailgrenade"; }

	static CNailGrenade* NailGrenade(CBaseEntity* owner);

private:
	static constexpr int kNumNails = 6;
	static constexpr int kNumBursts = 40;
};

class CMirv : public CPrimeGrenade
{
public:
	void Explode(TraceResult* pTrace, int bitsDamageType) override;

	const char* GetModelName() override { return "models/mirv_grenade.mdl"; }
	const char* GetIconName() override { return "d_mirvgrenade"; }

	static CMirv* Mirv(CBaseEntity* owner);

private:
	static constexpr int kNumBomblets = 4;
};

/* Toodles: This had a bunch of stupid randomness that I got rid of. */
class CBomblet : public CPrimeGrenade
{
public:
	bool Spawn() override;

	const char* GetModelName() override { return "models/bomblet.mdl"; }
	const char* GetIconName() override { return "d_mirvgrenade"; }

	static CBomblet* Bomblet(CBaseEntity* owner, const Vector& origin, const float yaw);
};

class CRocket : public CGrenade
{
public:
	bool Spawn() override;

	static CRocket* CreateRocket(
		const Vector& origin,
		const Vector& dir,
		const float damageMax,
		const float damageMin,
		const float radius,
		CBaseEntity* owner);
	void EXPORT RocketTouch(CBaseEntity *pOther);
	void EXPORT PleaseGoInTheRightDirection();
};

class CNail : public CBaseEntity
{
public:
	bool Spawn() override;

	static CNail* CreateNail(const Vector& origin, const Vector& dir, const float damage, CBaseEntity* owner);
	static CNail* CreateNailGrenadeNail(const Vector& origin, const Vector& dir, const float damage, CBaseEntity* owner);
	void EXPORT NailTouch(CBaseEntity *pOther);
	void EXPORT PleaseGoInTheRightDirection();
};

class CPipeBomb : public CGrenade
{
public:
	bool Spawn() override;

	static CPipeBomb* CreatePipeBomb(
		const Vector& origin,
		const Vector& dir,
		const float damageMax,
		const float damageMin,
		const float radius,
		CBaseEntity* owner,
		CPipeBombLauncher* launcher);
	void EXPORT PipeBombTouch(CBaseEntity *pOther);
};

#endif

void RadiusDamage(
	const Vector& origin,
	CBaseEntity* inflictor,
	CBaseEntity* attacker,
	const float damageMax,
	const float damageMin,
	const float radius,
	const int damageType);

#endif
