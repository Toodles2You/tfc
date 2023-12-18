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
//=========================================================
// GameRules
//=========================================================

#pragma once

#include <forward_list>
#include <vector>
#include <string>

class CBasePlayerWeapon;
class CBasePlayer;
class CItem;
class CBasePlayerAmmo;

// weapon respawning return codes
enum
{
	GR_NONE = 0,

	GR_WEAPON_RESPAWN_YES,
	GR_WEAPON_RESPAWN_NO,

	GR_AMMO_RESPAWN_YES,
	GR_AMMO_RESPAWN_NO,

	GR_ITEM_RESPAWN_YES,
	GR_ITEM_RESPAWN_NO,

	GR_PLR_DROP_GUN_ALL,
	GR_PLR_DROP_GUN_ACTIVE,
	GR_PLR_DROP_GUN_NO,

	GR_PLR_DROP_AMMO_ALL,
	GR_PLR_DROP_AMMO_ACTIVE,
	GR_PLR_DROP_AMMO_NO,
};

// Player relationship return codes
enum
{
	GR_ENEMY		= -2,
	GR_NEUTRAL		= -1,
	GR_NOTTEAMMATE	= 0,
	GR_ALLY			= 1,
	GR_TEAMMATE		= 2,
};

typedef enum
{
	// initialize the game, create teams
	GR_STATE_INIT = 0,

	// Before players have joined the game. Periodically checks to see if enough players are ready
	// to start a game. Also reverts to this when there are no active players
	GR_STATE_PREGAME,

	// The game is about to start, wait a bit and spawn everyone
	GR_STATE_STARTGAME,

	// All players are respawned, frozen in place
	GR_STATE_PREROUND,

	// Round is on, playing normally
	GR_STATE_RND_RUNNING,

	// Someone has won the round
	GR_STATE_TEAM_WIN,

	// Noone has won, manually restart the game, reset scores
	GR_STATE_RESTART,

	// Noone has won, restart the game
	GR_STATE_STALEMATE,

	// Game is over, showing the scoreboard etc
	GR_STATE_GAME_OVER,

	// Game is in a bonus state, transitioned to after a round ends
	GR_STATE_BONUS,

	// Game is awaiting the next wave/round of a multi round experience
	GR_STATE_BETWEEN_RNDS,

	GR_NUM_ROUND_STATES
} gamerules_state_e;


class CSpawnPoint
{
public:
	CSpawnPoint();
	CSpawnPoint(CBaseEntity *pEntity);

	virtual bool IsValid(CBasePlayer *pPlayer, int attempt);

	Vector m_origin;
	Vector m_angles;
    string_t m_target;
    string_t m_master;
	float m_lastSpawnTime;
};

class CGameRules
{
public:
	virtual void Think() = 0;								 // GR_Think - runs every server frame, should handle any timer tasks, periodic events, etc.
	virtual bool IsAllowedToSpawn(CBaseEntity* pEntity) = 0; // Can this item spawn (eg monsters don't spawn in deathmatch).

	virtual bool FShouldSwitchWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon) = 0;							  // should the player switch to this weapon?

	// Functions to verify the single/multiplayer status of a game
	virtual bool IsMultiplayer() = 0;								 // is this a multiplayer game? (either coop or deathmatch)
	virtual bool IsDeathmatch() = 0;								 //is this a deathmatch game?
	virtual bool IsTeamplay() { return false; }						 // is this deathmatch game being played with team rules?
	virtual bool IsCoOp() = 0;										 // is this a coop game?
	virtual const char* GetGameDescription() { return "Half-Life"; } // this is the game name that gets seen in the server browser

	// Client connection/disconnection
	virtual bool ClientConnected(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128]) = 0; // a client just connected to the server (player hasn't spawned yet)
	virtual void ClientPutInServer(CBasePlayer* pPlayer) = 0;
	virtual void InitHUD(CBasePlayer* pl);																				   // the client dll is ready for updating
	virtual void ClientDisconnected(edict_t* pClient) = 0;																	   // a client just disconnected from the server
	virtual void UpdateGameMode(CBasePlayer* pPlayer);																	   // the client needs to be informed of the current game mode
	virtual gamemode_e GetGameMode() { return kGamemodeSingleplayer; }

	// Client damage rules
	virtual float FlPlayerFallDamage(CBasePlayer* pPlayer) = 0;										 // this client just hit the ground after a fall. How much damage?
	virtual bool FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker) { return true; } // can this player take damage from this attacker?

	// Client spawn/respawn control
	virtual void PlayerSpawn(CBasePlayer* pPlayer) = 0;		   // called by CBasePlayer::Spawn just before releasing player into the game
	virtual void PlayerThink(CBasePlayer* pPlayer) = 0;		   // called by CBasePlayer::PreThink every frame, before physics are run and after keys are accepted
	virtual bool FPlayerCanRespawn(CBasePlayer* pPlayer) = 0;  // is this player allowed to respawn now?
	virtual float FlPlayerSpawnTime(CBasePlayer* pPlayer) = 0; // When in the future will this player be able to spawn?
	virtual bool IsSpawnSpotValid(CSpawnPoint *pSpawn, CBasePlayer *pPlayer, int attempt) { return true; }
	virtual CSpawnPoint *GetPlayerSpawnSpot(CBasePlayer* pPlayer); // Place this player on their spawnspot and face them the proper direction.
	virtual void AddPlayerSpawnSpot(CBaseEntity *pEntity);
	virtual bool FPlayerCanSuicide(CBasePlayer *pPlayer) = 0;  // Prevent players from suiciding too often.

	virtual bool ClientCommand(CBasePlayer* pPlayer, const char* pcmd) { return false; } // handles the user commands;  returns true if command handled properly
	virtual void ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer) {}		 // the player has changed userinfo;  can change it now

	// Client kills/scoring
	virtual float GetPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled, bool assist = false) = 0;					// how many points do I award whoever kills this player?
	virtual void PlayerKilled(CBasePlayer* pVictim, CBaseEntity* killer, CBaseEntity* inflictor, CBaseEntity* accomplice, int bitsDamageType) = 0; // Called each time a player dies
	virtual void DeathNotice(CBasePlayer* pVictim, CBaseEntity* killer, CBaseEntity* inflictor, CBaseEntity* accomplice, int bitsDamageType) = 0;	// Call this from within a GameRules class to report an obituary.
																									// Weapon retrieval
	virtual bool CanHavePlayerWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon) { return true; }	// The player is touching an CBasePlayerWeapon, do I give it to him?
	virtual void PlayerGotWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon) = 0;				// Called each time a player picks up a weapon from the ground

	// Weapon spawn/respawn control
	virtual int WeaponShouldRespawn(CBasePlayerWeapon* pWeapon) = 0;	   // should this weapon respawn?
	virtual float FlWeaponRespawnTime(CBasePlayerWeapon* pWeapon) = 0;   // when may this weapon respawn?
	virtual float FlWeaponTryRespawn(CBasePlayerWeapon* pWeapon) = 0;	   // can i respawn now,  and if not, when should i try again?
	virtual Vector VecWeaponRespawnSpot(CBasePlayerWeapon* pWeapon) = 0; // where in the world should this weapon respawn?

	// Item retrieval
	virtual bool CanHaveItem(CBasePlayer* pPlayer, CItem* pItem) { return true; }	// is this player allowed to take this item?
	virtual void PlayerGotItem(CBasePlayer* pPlayer, CItem* pItem) {} // call each time a player picks up an item (battery, healthkit, longjump)

	// Item spawn/respawn control
	virtual int ItemShouldRespawn(CItem* pItem) = 0;	 // Should this item respawn?
	virtual float FlItemRespawnTime(CItem* pItem) = 0;	 // when may this item respawn?
	virtual Vector VecItemRespawnSpot(CItem* pItem) = 0; // where in the world should this item respawn?

	// Ammo retrieval
	virtual bool CanHaveAmmo(CBasePlayer* pPlayer, int iAmmoType, int iMaxCarry); // can this player take more of this ammo?
	virtual void PlayerGotAmmo(CBasePlayer* pPlayer, char* szName, int iCount) {}	// called each time a player picks up some ammo in the world

	// Ammo spawn/respawn control
	virtual int AmmoShouldRespawn(CBasePlayerAmmo* pAmmo) = 0;	   // should this ammo item respawn?
	virtual float FlAmmoRespawnTime(CBasePlayerAmmo* pAmmo) = 0;   // when should this ammo item respawn?
	virtual Vector VecAmmoRespawnSpot(CBasePlayerAmmo* pAmmo) = 0; // where in the world should this ammo item respawn?
																   // by default, everything spawns

	virtual int DeadPlayerWeapons(CBasePlayer* pPlayer) = 0;
	virtual int DeadPlayerAmmo(CBasePlayer* pPlayer) = 0;

	// Teamplay stuff
	virtual int PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget) = 0; // What is the player's relationship with this entity?
	virtual int GetTeamIndex(const char* pTeamName) { return -1; }
	virtual const char* GetIndexedTeamName(int teamIndex) { return ""; }
	virtual bool IsValidTeam(const char* pTeamName) { return true; }
	virtual bool ChangePlayerTeam(CBasePlayer* pPlayer, int teamIndex, bool bKill, bool bGib, bool bAutoTeam) {}
	virtual bool ChangePlayerTeam(CBasePlayer* pPlayer, const char* pTeamName, bool bKill, bool bGib, bool bAutoTeam) {}
	virtual int GetDefaultPlayerTeam(CBasePlayer* pPlayer) {}

	// Monsters
	virtual bool FAllowMonsters() = 0; //are monsters allowed

	gamerules_state_e GetState() { return m_state; }
	virtual void EnterState(gamerules_state_e state) { m_state = state; }

	// Immediately end a multiplayer game
	virtual void EndMultiplayerGame() {}

	virtual bool IsPlayerPrivileged(CBasePlayer* pPlayer);

protected:
	gamerules_state_e m_state = GR_STATE_INIT;
	CSpawnPoint m_startPoint;
};

extern CGameRules* InstallGameRules();


//=========================================================
// CHalfLifeRules - rules for the single player Half-Life
// game.
//=========================================================
class CHalfLifeRules : public CGameRules
{
public:
	CHalfLifeRules();

	// GR_Think
	void Think() override;
	bool IsAllowedToSpawn(CBaseEntity* pEntity) override;

	bool FShouldSwitchWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon) override;

	// Functions to verify the single/multiplayer status of a game
	bool IsMultiplayer() override { return false; }
	bool IsDeathmatch() override { return false; }
	bool IsCoOp() override { return false; }

	// Client connection/disconnection
	bool ClientConnected(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128]) override;
	void ClientPutInServer(CBasePlayer* pPlayer) override;
	void ClientDisconnected(edict_t* pClient) override;

	// Client damage rules
	float FlPlayerFallDamage(CBasePlayer* pPlayer) override;

	// Client spawn/respawn control
	void PlayerSpawn(CBasePlayer* pPlayer) override;
	void PlayerThink(CBasePlayer* pPlayer) override;
	bool FPlayerCanRespawn(CBasePlayer* pPlayer) override;
	float FlPlayerSpawnTime(CBasePlayer* pPlayer) override;
	bool FPlayerCanSuicide(CBasePlayer *pPlayer) override;

	// Client kills/scoring
	float GetPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled, bool assist = false) override;
	void PlayerKilled(CBasePlayer* pVictim, CBaseEntity* killer, CBaseEntity* inflictor, CBaseEntity* accomplice, int bitsDamageType) override;
	void DeathNotice(CBasePlayer* pVictim, CBaseEntity* killer, CBaseEntity* inflictor, CBaseEntity* accomplice, int bitsDamageType) override;

	// Weapon retrieval
	void PlayerGotWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon) override;

	// Weapon spawn/respawn control
	int WeaponShouldRespawn(CBasePlayerWeapon* pWeapon) override;
	float FlWeaponRespawnTime(CBasePlayerWeapon* pWeapon) override;
	float FlWeaponTryRespawn(CBasePlayerWeapon* pWeapon) override;
	Vector VecWeaponRespawnSpot(CBasePlayerWeapon* pWeapon) override;

	// Item spawn/respawn control
	int ItemShouldRespawn(CItem* pItem) override;
	float FlItemRespawnTime(CItem* pItem) override;
	Vector VecItemRespawnSpot(CItem* pItem) override;

	// Ammo spawn/respawn control
	int AmmoShouldRespawn(CBasePlayerAmmo* pAmmo) override;
	float FlAmmoRespawnTime(CBasePlayerAmmo* pAmmo) override;
	Vector VecAmmoRespawnSpot(CBasePlayerAmmo* pAmmo) override;

	int DeadPlayerWeapons(CBasePlayer* pPlayer) override { return GR_PLR_DROP_GUN_NO; }
	int DeadPlayerAmmo(CBasePlayer* pPlayer) override { return GR_PLR_DROP_AMMO_NO; }

	// Monsters
	bool FAllowMonsters() override { return true; }

	// Teamplay stuff
	int PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget) override;
};

class CTeam
{
public:
	CTeam();
	CTeam(short index, std::string name);

	void AddPlayer(CBasePlayer *player);
	void RemovePlayer(CBasePlayer *player);

	void AddPoints(float score = 1);

public:
	short m_index;
	std::string m_name;
	float m_score;
	int m_numPlayers;
	std::vector<CBasePlayer *> m_players;
};

class CPoll
{
public:
	enum
	{
		kPollDuration = 20,
	};

	typedef void (CGameRules::*callback_t)(int, int, byte*, void*);

private:
	callback_t m_Callback;
	void* m_User;
	const unsigned int m_NumOptions;
	const float m_EndTime;

	unsigned int m_IgnorePlayers;
	byte m_Tally[12];

public:
	CPoll(
		callback_t callback,
		int options,
		const char* text,
		void* user = nullptr,
		int duration = kPollDuration);

	~CPoll();

	void CastVote(int playerIndex, int option);
	bool CheckVotes();

private:
	bool CanPlayerVote(CBasePlayer* player);
};

//=========================================================
// CHalfLifeMultiplay - rules for the basic half life multiplayer
// competition
//=========================================================
class CHalfLifeMultiplay : public CGameRules
{
public:
	CHalfLifeMultiplay();

	// GR_Think
	void Think() override;
	bool IsAllowedToSpawn(CBaseEntity* pEntity) override;

	bool FShouldSwitchWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon) override;

	// Functions to verify the single/multiplayer status of a game
	bool IsMultiplayer() override { return true; }
	bool IsDeathmatch() override { return m_deathmatch; }
	bool IsCoOp() override { return m_coop; }

	// Client connection/disconnection
	// If ClientConnected returns false, the connection is rejected and the user is provided the reason specified in
	//  svRejectReason
	// Only the client's name and remote address are provided to the dll for verification.
	bool ClientConnected(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128]) override;
	void ClientPutInServer(CBasePlayer* pPlayer) override;
	void InitHUD(CBasePlayer* pl) override; // the client dll is ready for updating
	void ClientDisconnected(edict_t* pClient) override;
	gamemode_e GetGameMode() override { return IsCoOp() ? kGamemodeCooperative : kGamemodeDeathmatch; }

	// Client damage rules
	float FlPlayerFallDamage(CBasePlayer* pPlayer) override;

	// Client spawn/respawn control
	void PlayerSpawn(CBasePlayer* pPlayer) override;
	void PlayerThink(CBasePlayer* pPlayer) override;
	bool FPlayerCanRespawn(CBasePlayer* pPlayer) override;
	float FlPlayerSpawnTime(CBasePlayer* pPlayer) override;
	bool IsSpawnSpotValid(CSpawnPoint *pSpawn, CBasePlayer *pPlayer, int attempt) override;
	CSpawnPoint *GetPlayerSpawnSpot(CBasePlayer* pPlayer) override;
	void AddPlayerSpawnSpot(CBaseEntity *pEntity) override;
	bool FPlayerCanSuicide(CBasePlayer *pPlayer) override;

	bool ClientCommand(CBasePlayer* pPlayer, const char* pcmd) override;
	void ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer) override;

	// Client kills/scoring
	float GetPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled, bool assist = false) override;
	void PlayerKilled(CBasePlayer* pVictim, CBaseEntity* killer, CBaseEntity* inflictor, CBaseEntity* accomplice, int bitsDamageType) override;
	void DeathNotice(CBasePlayer* pVictim, CBaseEntity* killer, CBaseEntity* inflictor, CBaseEntity* accomplice, int bitsDamageType) override;

	// Weapon retrieval
	void PlayerGotWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon) override;
	bool CanHavePlayerWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon) override; // The player is touching an CBasePlayerWeapon, do I give it to him?

	// Weapon spawn/respawn control
	int WeaponShouldRespawn(CBasePlayerWeapon* pWeapon) override;
	float FlWeaponRespawnTime(CBasePlayerWeapon* pWeapon) override;
	float FlWeaponTryRespawn(CBasePlayerWeapon* pWeapon) override;
	Vector VecWeaponRespawnSpot(CBasePlayerWeapon* pWeapon) override;

	// Item spawn/respawn control
	int ItemShouldRespawn(CItem* pItem) override;
	float FlItemRespawnTime(CItem* pItem) override;
	Vector VecItemRespawnSpot(CItem* pItem) override;

	// Ammo spawn/respawn control
	int AmmoShouldRespawn(CBasePlayerAmmo* pAmmo) override;
	float FlAmmoRespawnTime(CBasePlayerAmmo* pAmmo) override;
	Vector VecAmmoRespawnSpot(CBasePlayerAmmo* pAmmo) override;

	int DeadPlayerWeapons(CBasePlayer* pPlayer) override { return GR_PLR_DROP_GUN_ACTIVE; }
	int DeadPlayerAmmo(CBasePlayer* pPlayer) override { return GR_PLR_DROP_AMMO_ALL; }

	// Teamplay stuff
	int PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget) override;
	int GetTeamIndex(const char* pTeamName) override;
	const char* GetIndexedTeamName(int teamIndex) override;
	bool IsValidTeam(const char* pTeamName) override;
	int GetDefaultPlayerTeam(CBasePlayer* pPlayer) override;
	bool ChangePlayerTeam(CBasePlayer* pPlayer, int teamIndex, bool bKill, bool bGib, bool bAutoTeam) override;
	bool ChangePlayerTeam(CBasePlayer* pPlayer, const char* pTeamName, bool bKill, bool bGib, bool bAutoTeam) override;

	// Monsters
	bool FAllowMonsters() override { return m_allowMonsters; }

	// Immediately end a multiplayer game
	void EndMultiplayerGame() override { EnterState(GR_STATE_GAME_OVER); }

	virtual void EnterState(gamerules_state_e state) override;

protected:
	void CheckTimeLimit();
	void CheckCurrentPoll();

	virtual void Enter_RND_RUNNING();
	virtual void Think_RND_RUNNING();

	virtual void Enter_GAME_OVER();
	virtual void Think_GAME_OVER();

protected:
	float m_stateChangeTime;
    std::vector<CSpawnPoint> m_spawnPoints;
	std::vector<CSpawnPoint*> m_validSpawnPoints;
	std::size_t m_numSpawnPoints = 0;
	bool m_deathmatch = false;
	bool m_coop = false;
	bool m_allowMonsters = false;
	bool m_allowSpectators = false;
	int m_numTeams;
	std::vector<CTeam> m_teams;
	CTeam m_spectators;
	
	float m_NextPollCheck;
	CPoll* m_CurrentPoll;

	bool m_NextMapVoteCalled;
	void MapVoteBegin();
	void MapVoteEnd(int winner, int numOptions, byte* tally, void* user);

	bool AllowSpectators() { return m_allowSpectators; }

	virtual void ChangeLevel();
	void SendMOTDToClient(CBasePlayer* player);
	virtual void SendMenusToClient(CBasePlayer* player);

	virtual bool PrivilegedCommand(CBasePlayer* pPlayer, const char* pcmd);
};

inline CGameRules* g_pGameRules = nullptr;
