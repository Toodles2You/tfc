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

#include "pm_materials.h"
#include "entity_state.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_shared.h"
#include "pm_materials.h"
#include "pm_movevars.h"
#include "pm_debug.h"
#include "gamemovement.h"

#include <forward_list>


#define PLAYER_FATAL_FALL_SPEED 1024															  // approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED 580															  // approx 20 feet
#define DAMAGE_FOR_FALL_SPEED (float)100 / (PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED) // damage per unit per second.
#define PLAYER_FALL_PUNCH_THRESHHOLD (float)350 // won't punch player's screen/make scrape noise unless player falling at least this fast.

typedef enum
{
	PLAYER_IDLE,
	PLAYER_WALK,
	PLAYER_JUMP,
	PLAYER_SUPERJUMP,
	PLAYER_DIE,
	PLAYER_ATTACK1,
} PLAYER_ANIM;

#define CHAT_INTERVAL 1.0f

class CTeam;

class CBasePlayer : public CBaseAnimating
{
public:
	enum class State
	{
	#ifdef HALFLIFE_GRENADES
		GrenadePrime    = 1,
		GrenadeThrowing = 2,
		Grenade         = 3,
	#endif
		Holstered       = 4,
	};

public:
	// Spectator camera
	void Observer_FindNextPlayer(bool bReverse);
	void Observer_HandleButtons();
	void Observer_SetMode(int iMode);
	void Observer_CheckTarget();
	EHANDLE m_hObserverTarget;
	float m_flNextObserverInput;
	int m_iObserverLastMode; // last used observer mode
	bool IsObserver() { return pev->iuser1 != OBS_NONE; }
	bool IsSpectator() { return TeamNumber() == TEAM_SPECTATORS; }

	unsigned int m_randomSeed; // See that is shared between client & server for shared weapons code

	int m_afButtonLast;
	int m_afButtonPressed;
	int m_afButtonReleased;

	EHANDLE m_SndLast;	   // last sound entity to modify player room type
	int m_SndRoomtype = 0; // last roomtype set by sound entity. Defaults to 0 on new maps to disable it by default.
	int m_ClientSndRoomtype;
	float m_flSndRange; // dist from player to sound entity

	float m_flFallVelocity;

	float m_fNextSuicideTime;	   // the time after which the player can next use the suicide command


	// these are time-sensitive things that we keep track of

	int m_bitsHUDDamage; // Damage bits for the current fame. These get sent to
						 // the hude via the DAMAGE message

#ifdef GAME_DLL
	enum class ResetHUD
	{
		No = 0,
		Initialize,
		Reset,
	} m_ResetHUD;
#endif

#ifdef HALFLIFE_TRAINCONTROL
	int m_iTrain;	// Train control position
#endif

#ifdef HALFLIFE_TANKCONTROL
	EHANDLE m_pTank;		 // the tank which the player is currently controlling,  NULL if no tank
#endif

	float m_fDeadTime;		 // the time at which the player died  (used in PlayerDeathFrame())

	int m_iFOV;		  // field of view
	// usable player weapons
	CBasePlayerWeapon* m_rgpPlayerWeapons[WEAPON_TYPES];
	std::forward_list<CBasePlayerWeapon *> m_lpPlayerWeapons;
	CBasePlayerWeapon* m_pActiveWeapon;

	std::uint64_t m_WeaponBits;

	//Not saved, used to update client.
	std::uint64_t m_ClientWeaponBits;

	/* Player state flags synchronized between the client & server. */
protected:
	std::uint64_t m_StateBits;
public:
	inline bool InState(const State state) { return (m_StateBits & static_cast<std::uint64_t>(state)) != 0; }
	inline void EnterState(const State state) { m_StateBits |= static_cast<std::uint64_t>(state); }
	inline void LeaveState(const State state) { m_StateBits &= ~static_cast<std::uint64_t>(state); }

	// shared ammo slots
	byte m_rgAmmo[AMMO_TYPES];

	int m_iDeaths;

	int m_nCustomSprayFrames = -1; // Custom clan logo frames for this player
	float m_flNextDecalTime;  // next time this player can spray a decal

	CTeam *m_team = nullptr;

	bool Spawn() override;
	void Pain();

	virtual void PreThink();
	virtual void PostThink();
	inline Vector GetGunPosition() { return pev->origin + pev->view_ofs; }
	bool GiveHealth(float flHealth, int bitsDamageType) override;
	void TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, int hitgroup, int bitsDamageType) override;
	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;
	void Killed(CBaseEntity* inflictor, CBaseEntity* attacker, int bitsDamageType) override;
	Vector BodyTarget() override { return Center() + pev->view_ofs * 0.8; } // position to shoot at
	bool IsAlive() override { return (pev->deadflag == DEAD_NO) && pev->health > 0; }

	bool IsClient() override { return true; }

	// Spectators should return false for this, they aren't "players" as far as game logic is concerned
	bool IsPlayer() override { return (pev->effects & EF_NODRAW) == 0 && !IsSpectator() && !IsObserver(); }

	bool IsNetClient() override { return true; } // Bots should return false for this, they can't receive NET messages
												 // Spectators should return true for this
	const char* TeamID() override;

	void PackDeadPlayerWeapons();
	void RemoveAllWeapons();

	void SetWeaponBit(int id);
	void ClearWeaponBit(int id);

	// JOHN:  sends custom messages if player HUD data has changed  (eg health, ammo)
	virtual void UpdateClientData();

#ifdef HALFLIFE_SAVERESTORE
	static TYPEDESCRIPTION m_playerSaveData[];

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
#endif

	// Player is moved across the transition by other means
	int ObjectCaps() override { return (CBaseAnimating::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_NET_ALWAYS_SEND; }
	void Precache() override;

	void DeathSound();

	virtual void HandleSequenceFinished() override;

	enum class Action
	{
		Idle,
		Walk,
		Jump,
		Die,
		Attack,
		Reload,
		Arm,
	};

	Action m_Action;
	char m_szAnimExtention[32];

	virtual void SetAction(const Action action, const bool force = false);

protected:
	virtual bool CanBeginAction(const Action action);

	virtual int GetDeathSequence();
	virtual int GetSmallFlinchSequence();
	virtual int GetActionSequence(const Action action, bool& restart);

	virtual int GetGaitSequence();

	virtual void UpdateMovementAction();

public:
	// custom player functions
	virtual void ImpulseCommands();
	void CheatImpulseCommands(int iImpulse);

	void StartObserver();

	void AddPlayerWeapon(CBasePlayerWeapon* weapon);
	void RemovePlayerWeapon(CBasePlayerWeapon* weapon);
	void DropPlayerWeapon(char* pszWeaponName);
	bool HasPlayerWeapon(int iId) { return (m_WeaponBits & (1ULL << iId)) != 0; }
	bool HasWeapons() { return m_WeaponBits != 0; }
	void SelectWeapon(int id);
	void WeaponPostFrame();
	void GiveNamedItem(const char* szName);
	void EnableControl(bool fControl);
	bool SetWeaponHolstered(const bool holstered, const bool forceSendAnimations = true);

	bool GiveAmmo(int iAmount, int iType) override;

	void PlaybackEvent(
		unsigned short event,
		float fParam1 = 0.0F,
		float fParam2 = 0.0F,
		int iParam1 = 0,
		int iParam2 = 0,
		qboolean bParam1 = false,
		qboolean bParam2 = false,
		unsigned int flags = FEV_NOTHOST);

public:
	void PlayerDeathFrame();
	void PlayerUse();
#ifdef GAME_DLL
	EHANDLE m_hUseObject;
	bool CanUseObject(CBaseEntity* object, const float maxDot = 0.0F, const float maxDistance = 0.0F);
	void SetUseObject(CBaseEntity* object);
#endif

	Vector GetAimVector();

	void ForceClientDllUpdate(); // Forces all client .dll specific data to be resent to client.

	void SetCustomDecalFrames(int nFrames);
	int GetCustomDecalFrames();

	float m_flNextChatTime;

	void SetPrefsFromUserinfo(char* infobuffer);
#ifdef GAME_DLL
	void SendExtraInfo(CBaseEntity* toWhom = nullptr);
#endif

	int m_iAutoWepSwitch;
	bool m_bGrenadeToggle;
	bool m_bLeftHanded;

	//True if the player is currently spawning.
	bool m_bIsSpawning = false;

	int m_bitsDamageType; // what types of damage has monster (player) taken

	int m_LastHitGroup; // the last body region that took damage

	void FireBullets(const float damage, const Vector2D& spread, const unsigned int count = 1, const float distance = 8192);

	virtual void GetClientData(clientdata_t& data, bool sendWeapons);
	virtual void SetClientData(const clientdata_t& data);
	virtual void DecrementTimers(const int msec);
	virtual void CmdStart(const usercmd_t& cmd, unsigned int randomSeed);

#ifdef CLIENT_DLL
	virtual void UpdateHudData();
#endif

	virtual void GetEntityState(entity_state_t& state) override;
	virtual void SetEntityState(const entity_state_t& state) override;

#ifdef GAME_DLL
	void EmitSoundHUD(
		const char* sample,
		int channel = CHAN_AUTO,
		float volume = VOL_NORM,
		float attenuation = ATTN_NORM,
		int pitch = PITCH_NORM,
		int flags = 0);
#endif

	void InstallGameMovement(CGameMovement* gameMovement)
	{
		if (m_gameMovement != nullptr)
		{
			delete m_gameMovement;
		}
		m_gameMovement = gameMovement;
	}

	CGameMovement* GetGameMovement() { return m_gameMovement; }

	CBasePlayerWeapon* GetNextBestWeapon(CBasePlayerWeapon* current);

protected:
	CGameMovement* m_gameMovement = nullptr;
	EHANDLE m_hLastAttacker[2];

#ifdef HALFLIFE_GRENADES
	void PrimeGrenade();
	void ThrowGrenade();
#endif
};

inline void CBasePlayer::SetWeaponBit(int id)
{
	m_WeaponBits |= 1ULL << id;
}

inline void CBasePlayer::ClearWeaponBit(int id)
{
	m_WeaponBits &= ~(1ULL << id);
}

inline bool gEvilImpulse101 = false;
