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

#include "bitvec.h"

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
	kProjFlame,
	kProjIncendiaryRocket,
	kProjTranquilizerDart,
	kProjLaser,
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
	union {
		int iProjectileRange;
		int iProjectileRadius;
	};
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
	// nullptr,
	"tf_weapon_detpack",
	"tf_weapon_tranq",
	"tf_weapon_railgun",
	"tf_weapon_pl",
	"tf_weapon_knife",
	"tf_weapon_builder",
};
#endif

// inventory items that commit war crimes
class CBasePlayerWeapon : public CBaseAnimating
{
public:
	CBasePlayerWeapon(Entity* containingEntity) : CBaseAnimating(containingEntity) {}

	DECLARE_SAVERESTORE()

	void SetObjectCollisionBox() override;

	virtual bool Spawn() override;
	virtual void Precache() override;

	// generic weapon versions of CBasePlayerWeapon calls
	virtual bool AddToPlayer(CBasePlayer* pPlayer);
	virtual void RemoveFromPlayer(bool forceSendAnimations = true);
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
	union {
		bool m_fInReload;
		bool m_fInAttack;
	};

	enum
	{
		kWpnStateReloading = 1,
		kWpnStateFiring = kWpnStateReloading,
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
inline short g_sModelIndexWallPuff;
inline short g_sModelIndexDebris;
inline short g_sModelIndexSmokeTrail;
inline short g_sModelIndexNail;
inline short g_sModelIndexSaveMe;
inline short g_sModelIndexFlare;
inline short g_sModelIndexShockWave;
inline short g_sModelIndexFlame;
inline short g_sModelIndexFire;
inline short g_sModelIndexFireLoop;
inline short g_sModelIndexDart;
inline short g_sModelIndexComputerGibs;

inline unsigned short g_usConcBlast;
inline unsigned short g_usEMP;
inline unsigned short g_usFlash;

class CTFWeapon : public CBasePlayerWeapon
{
public:
	CTFWeapon(Entity* containingEntity) : CBasePlayerWeapon(containingEntity) {}

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
	CTFMelee(Entity* containingEntity) : CTFWeapon(containingEntity) {}

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
	CMedikit(Entity* containingEntity) : CTFMelee(containingEntity) {}

	int GetID() const override { return WEAPON_MEDIKIT; }
	void GetWeaponInfo(WeaponInfo& i) override;

#ifdef GAME_DLL
protected:
	int HitEntity(CBaseEntity* hit, const Vector& dir, const TraceResult& tr) override;
#endif
};

class CSpanner : public CTFMelee
{
public:
	CSpanner(Entity* containingEntity) : CTFMelee(containingEntity) {}

	int GetID() const override { return WEAPON_SPANNER; }
	void GetWeaponInfo(WeaponInfo& i) override;

#ifdef GAME_DLL
protected:
	int HitEntity(CBaseEntity* hit, const Vector& dir, const TraceResult& tr) override;
#endif
};

class CAxe : public CTFMelee
{
public:
	CAxe(Entity* containingEntity) : CTFMelee(containingEntity) {}

	int GetID() const override { return WEAPON_AXE; }
	void GetWeaponInfo(WeaponInfo& i) override;
};

class CSniperRifle : public CTFWeapon
{
public:
	CSniperRifle(Entity* containingEntity) : CTFWeapon(containingEntity) {}

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
	CAutoRifle(Entity* containingEntity) : CTFWeapon(containingEntity) {}

	int GetID() const override { return WEAPON_AUTO_RIFLE; }
	void GetWeaponInfo(WeaponInfo& i) override;
};

class CShotgun : public CTFWeapon
{
public:
	CShotgun(Entity* containingEntity) : CTFWeapon(containingEntity) {}

	int GetID() const override { return WEAPON_TF_SHOTGUN; }
	void GetWeaponInfo(WeaponInfo& i) override;
};

class CSuperShotgun : public CTFWeapon
{
public:
	CSuperShotgun(Entity* containingEntity) : CTFWeapon(containingEntity) {}

	int GetID() const override { return WEAPON_SUPER_SHOTGUN; }
	void GetWeaponInfo(WeaponInfo& i) override;
};

class CNailgun : public CTFWeapon
{
public:
	CNailgun(Entity* containingEntity) : CTFWeapon(containingEntity) {}

	int GetID() const override { return WEAPON_NAILGUN; }
	void GetWeaponInfo(WeaponInfo& i) override;
};

class CSuperNailgun : public CTFWeapon
{
public:
	CSuperNailgun(Entity* containingEntity) : CTFWeapon(containingEntity) {}

	int GetID() const override { return WEAPON_SUPER_NAILGUN; }
	void GetWeaponInfo(WeaponInfo& i) override;
};

class CGrenadeLauncher : public CTFWeapon
{
public:
	CGrenadeLauncher(Entity* containingEntity) : CTFWeapon(containingEntity) {}

	int GetID() const override { return WEAPON_GRENADE_LAUNCHER; }
	void GetWeaponInfo(WeaponInfo& i) override;
};

class CFlamethrower : public CTFWeapon
{
public:
	CFlamethrower(Entity* containingEntity) : CTFWeapon(containingEntity) {}

	int GetID() const override { return WEAPON_FLAMETHROWER; }
	void GetWeaponInfo(WeaponInfo& i) override;
};

class CRocketLauncher : public CTFWeapon
{
public:
	CRocketLauncher(Entity* containingEntity) : CTFWeapon(containingEntity) {}

	int GetID() const override { return WEAPON_ROCKET_LAUNCHER; }
	void GetWeaponInfo(WeaponInfo& i) override;
};

class CIncendiaryCannon : public CTFWeapon
{
public:
	CIncendiaryCannon(Entity* containingEntity) : CTFWeapon(containingEntity) {}

	int GetID() const override { return WEAPON_INCENDIARY; }
	void GetWeaponInfo(WeaponInfo& i) override;
};

class CAssaultCannon : public CTFWeapon
{
public:
	CAssaultCannon(Entity* containingEntity) : CTFWeapon(containingEntity) {}

	int GetID() const override { return WEAPON_ASSAULT_CANNON; }
	void GetWeaponInfo(WeaponInfo& i) override;

	bool CanHolster() override;

	void WeaponPostFrame() override;

	void Holster() override;
};

class CDetpack : public CTFWeapon
{
public:
	CDetpack(Entity* containingEntity) : CTFWeapon(containingEntity) {}

	int ObjectCaps() override
	{
		if (m_pPlayer != nullptr)
		{
			return CTFWeapon::ObjectCaps();
		}
		return CTFWeapon::ObjectCaps() | FCAP_DONT_SAVE | FCAP_CONTINUOUS_USE;
	}

	int GetID() const override { return WEAPON_DETPACK; }
	void GetWeaponInfo(WeaponInfo& i) override;

	bool AddToPlayer(CBasePlayer* pPlayer) override;
	void RemoveFromPlayer(bool forceSendAnimations = true) override;

	bool CanDeploy() override { return false; }
	void Deploy() override;

	void WeaponPostFrame() override;

	void Holster() override;

	void Set();
#ifdef GAME_DLL
	void EXPORT FireInTheHole();
	void EXPORT Detonate();
	void EXPORT Disarm(CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value);

	bool ElectromagneticPulse(CBaseEntity* attacker, CBaseEntity* inflictor) override;
#endif
};

class CTranquilizerGun : public CTFWeapon
{
public:
	CTranquilizerGun(Entity* containingEntity) : CTFWeapon(containingEntity) {}

	int GetID() const override { return WEAPON_TRANQ; }
	void GetWeaponInfo(WeaponInfo& i) override;
};

class CRailgun : public CTFWeapon
{
public:
	CRailgun(Entity* containingEntity) : CTFWeapon(containingEntity) {}

	int GetID() const override { return WEAPON_LASER; }
	void GetWeaponInfo(WeaponInfo& i) override;
};

class CPipeBombLauncher : public CTFWeapon
{
public:
	CPipeBombLauncher(Entity* containingEntity) : CTFWeapon(containingEntity) {}

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

class CKnife : public CTFMelee
{
public:
	CKnife(Entity* containingEntity) : CTFMelee(containingEntity) {}

	int GetID() const override { return WEAPON_KNIFE; }
	void GetWeaponInfo(WeaponInfo& i) override;

#ifdef GAME_DLL
protected:
	int HitEntity(CBaseEntity* hit, const Vector& dir, const TraceResult& tr) override;
#endif
};

class CBuilder : public CTFWeapon
{
public:
	CBuilder(Entity* containingEntity) : CTFWeapon(containingEntity) {}

	virtual bool Spawn() override;

	bool ShouldCollide(CBaseEntity* other) override;
	bool ShouldBlockTrace() override { return false; }

	int ObjectCaps() override
	{
		if (m_pPlayer != nullptr)
		{
			return CTFWeapon::ObjectCaps();
		}
		return CTFWeapon::ObjectCaps() | FCAP_DONT_SAVE | FCAP_CONTINUOUS_USE;
	}

	int GetID() const override { return WEAPON_BUILDER; }
	void GetWeaponInfo(WeaponInfo& i) override;

	bool AddToPlayer(CBasePlayer* pPlayer) override;
	void RemoveFromPlayer(bool forceSendAnimations = true) override;

	bool CanDeploy() override { return false; }
	void Deploy() override;

	void WeaponPostFrame() override;

	void Holster() override;

protected:
	static constexpr byte kBuildCost[] = {0, 100, 130, 255, 125, 125};
	static constexpr byte kBuildTime[] = {0, 2, 5, 5, 4, 4};

	bool CheckArea(const Vector& origin);

public:
	void StartBuilding(const int buildingType);
	void StopBuilding();
	bool SpawnBuilding(const int buildingType);
	void FinishBuilding();
	void DestroyBuilding(const int buildingType, const bool detonate);
	int GetBuildState();
	void RotateBuilding(const int buildingType, const float angle);
};

#ifdef GAME_DLL

// Contact Grenade / Timed grenade / Satchel Charge
class CGrenade : public CBaseAnimating
{
public:
	CGrenade(Entity* containingEntity) : CBaseAnimating(containingEntity) {}

	bool Spawn() override;

	int ObjectCaps() override { return CBaseAnimating::ObjectCaps() | FCAP_DONT_SAVE; }

	static CGrenade* ShootContact(CBaseEntity* owner, Vector vecStart, Vector vecVelocity);

	void Explode(Vector vecSrc, Vector vecAim);
	virtual void Explode(TraceResult* pTrace, int bitsDamageType);

	virtual bool CanDetonate() { return true; }
	void EXPORT BounceTouch(CBaseEntity* pOther);
	void EXPORT ExplodeTouch(CBaseEntity* pOther);
	void EXPORT Detonate();
	void EXPORT TumbleThink();

	virtual void BounceSound();

	bool ShouldCollide(CBaseEntity* other) override;
};

#ifdef HALFLIFE_GRENADES

/* Team Fortress style timed grenade */
class CPrimeGrenade : public CGrenade
{
public:
	CPrimeGrenade(Entity* containingEntity) : CGrenade(containingEntity) {}

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
	CCaltropCanister(Entity* containingEntity) : CPrimeGrenade(containingEntity) {}

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
	CCaltrop(Entity* containingEntity) : CPrimeGrenade(containingEntity) {}

	bool Spawn() override;
	void EXPORT CaltropThink();
	void EXPORT CaltropTouch(CBaseEntity* other);

	bool ShouldCollide(CBaseEntity* other) override;

	const char* GetModelName() override { return "models/caltrop.mdl"; }
	const char* GetIconName() override { return "d_caltrop"; }

	static CCaltrop* Caltrop(CBaseEntity* owner, const Vector& origin, const float yaw);
};

class CConcussionGrenade : public CPrimeGrenade
{
public:
	CConcussionGrenade(Entity* containingEntity) : CPrimeGrenade(containingEntity) {}

	void Explode(TraceResult* pTrace, int bitsDamageType) override;

	const char* GetModelName() override { return "models/conc_grenade.mdl"; }
	const char* GetIconName() override { return "d_concussiongrenade"; }

	static CConcussionGrenade* ConcussionGrenade(CBaseEntity* owner);
};

class CFlare : public CPrimeGrenade
{
public:
	CFlare(Entity* containingEntity) : CPrimeGrenade(containingEntity) {}

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
	CNailGrenade(Entity* containingEntity) : CPrimeGrenade(containingEntity) {}

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
	CMirv(Entity* containingEntity) : CPrimeGrenade(containingEntity) {}

	void Explode(TraceResult* pTrace, int bitsDamageType) override;

	const char* GetModelName() override { return "models/mirv_grenade.mdl"; }
	const char* GetIconName() override { return "d_mirvgrenade"; }

	static CMirv* Mirv(CBaseEntity* owner);

private:
	static constexpr int kNumBomblets = 4;
};

class CBomblet : public CPrimeGrenade
{
public:
	CBomblet(Entity* containingEntity) : CPrimeGrenade(containingEntity) {}

	bool Spawn() override;

	bool ShouldCollide(CBaseEntity* other) override;

	const char* GetModelName() override { return "models/bomblet.mdl"; }
	const char* GetIconName() override { return "d_mirvgrenade"; }

	static CBomblet* Bomblet(CBaseEntity* owner, const Vector& origin, const float yaw);
};

class CNapalmGrenade : public CPrimeGrenade
{
public:
	CNapalmGrenade(Entity* containingEntity) : CPrimeGrenade(containingEntity) {}

	bool Spawn() override;
	void Explode(TraceResult* pTrace, int bitsDamageType) override;
	void EXPORT NapalmThink();

	void UpdateOnRemove() override;

	const char* GetModelName() override { return "models/napalm.mdl"; }
	const char* GetIconName() override { return "d_napalmgrenade"; }

	static CNapalmGrenade* NapalmGrenade(CBaseEntity* owner);

private:
	static constexpr int kNumBursts = 16;
};

class CEMP : public CPrimeGrenade
{
public:
	CEMP(Entity* containingEntity) : CPrimeGrenade(containingEntity) {}

	void Explode(TraceResult* pTrace, int bitsDamageType) override;

	const char* GetModelName() override { return "models/emp_grenade.mdl"; }
	const char* GetIconName() override { return "d_empgrenade"; }

	static CEMP* EMP(CBaseEntity* owner);
};

class CFlashGrenade : public CPrimeGrenade
{
public:
	CFlashGrenade(Entity* containingEntity) : CPrimeGrenade(containingEntity) {}

	bool Spawn() override;
	void Explode(TraceResult* pTrace, int bitsDamageType) override;

	const char* GetModelName() override { return "models/spy_grenade.mdl"; }
	const char* GetIconName() override { return "d_gasgrenade"; }

	static CFlashGrenade* FlashGrenade(CBaseEntity* owner);
};

class CRocket : public CGrenade
{
public:
	CRocket(Entity* containingEntity) : CGrenade(containingEntity) {}

	bool Spawn() override;

	int ObjectCaps() override { return CBaseEntity::ObjectCaps() | FCAP_DONT_SAVE; }

	static CRocket* CreateRocket(
		const Vector& origin,
		const Vector& dir,
		const float damageMax,
		const float damageMin,
		const float radius,
		CBaseEntity* owner);
	static CRocket* CreateIncendiaryRocket(
		const Vector& origin,
		const Vector& dir,
		const float damageMax,
		const float damageMin,
		const float radius,
		CBaseEntity* owner);
	static CRocket* CreateSentryRocket(
		const Vector& origin,
		const Vector& dir,
		const float damageMax,
		const float damageMin,
		const float radius,
		CBaseEntity* owner);
	void EXPORT RocketTouch(CBaseEntity *pOther);
	void EXPORT PleaseGoInTheRightDirection();
};

class CIncendiaryRocket : public CRocket
{
public:
	CIncendiaryRocket(Entity* containingEntity) : CRocket(containingEntity) {}

	void Explode(TraceResult* pTrace, int bitsDamageType) override;
};

class CNail : public CBaseEntity
{
public:
	CNail(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	bool Spawn() override;

	int ObjectCaps() override { return CBaseEntity::ObjectCaps() | FCAP_DONT_SAVE; }

	static CNail* CreateNail(const Vector& origin, const Vector& dir, const float damage, CBaseEntity* owner);
	static CNail* CreateNailGrenadeNail(const Vector& origin, const Vector& dir, const float damage, CBaseEntity* owner);
	static CNail* CreateTranquilizerDart(const Vector& origin, const Vector& dir, const float damage, CBaseEntity* owner);
	static CNail* CreateLaser(const Vector& origin, const Vector& dir, const float damage, CBaseEntity* owner);
	void EXPORT NailTouch(CBaseEntity *pOther);
	void EXPORT PleaseGoInTheRightDirection();

	bool ShouldCollide(CBaseEntity* other) override;

protected:
	CBitVec<MAX_PLAYERS> m_touchedPlayers;
};

class CFlame : public CBaseEntity
{
public:
	CFlame(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	bool Spawn() override;

	int ObjectCaps() override { return CBaseEntity::ObjectCaps() | FCAP_DONT_SAVE; }

	static CFlame* CreateFlame(const Vector& origin, const Vector& dir, const float damage, CBaseEntity* owner);
	void EXPORT FlameTouch(CBaseEntity *pOther);
	void EXPORT PleaseDoNotGoInTheWater();

	bool ShouldCollide(CBaseEntity* other) override;

protected:
	CBitVec<MAX_PLAYERS> m_touchedPlayers;
};

class CPipeBomb : public CGrenade
{
public:
	CPipeBomb(Entity* containingEntity) : CGrenade(containingEntity) {}

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

	bool ShouldCollide(CBaseEntity* other) override;

	bool ElectromagneticPulse(CBaseEntity* attacker, CBaseEntity* inflictor) override;
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
