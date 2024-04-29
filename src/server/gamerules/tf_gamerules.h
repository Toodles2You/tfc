//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Whatever
//
// $NoKeywords: $
//=============================================================================

#pragma once

class CHLBot;
class CTFSpawn;
class CTFDetect;

class CTFSpawnPoint : public CSpawnPoint
{
public:
	CTFSpawnPoint();
	CTFSpawnPoint(CBaseEntity *pEntity);

	bool IsValid(CBasePlayer *pPlayer, int attempt) override;

	CTFSpawn* m_TFSpawn;
};

class CTFTeamInfo
{
public:
	CTFTeamInfo()
	{
		m_szTeamName[0] = '\0';

		m_iLives = 0;
		m_iMaxPlayers = 0;

		m_afInvalidClasses = 0;
		m_afAlliedTeams = 0;
	}

	char m_szTeamName[16];

	int m_iLives;
	int m_iMaxPlayers;

	int m_afInvalidClasses;
	int m_afAlliedTeams;
};

class CTeamFortress : public CHalfLifeMultiplay
{
public:
	friend class CHLBot;
	friend class CTFDetect;

	CTeamFortress();

	bool ClientCommand(CBasePlayer* pPlayer, const char* pcmd) override;
	void ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer) override;
	bool IsTeamplay() override { return true; }
	bool FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker) override;
	int PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget) override;
	float GetPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled, bool assist = false) override;
	void InitHUD(CBasePlayer* pl) override;
	gamemode_e GetGameMode() override { return kGamemodeTeamFortress; }
	bool ChangePlayerTeam(CBasePlayer* pPlayer, int teamIndex, bool bKill, bool bGib, bool bAutoTeam) override;
	void PlayerSpawn(CBasePlayer* pPlayer) override;
	bool FPlayerCanRespawn(CBasePlayer* pPlayer) override;
	int DeadPlayerWeapons(CBasePlayer* pPlayer) override { return GR_PLR_DROP_GUN_NO; }
	int DeadPlayerAmmo(CBasePlayer* pPlayer) override { return GR_PLR_DROP_AMMO_ALL; }
	void AddPlayerSpawnSpot(CBaseEntity *pEntity) override;
	int GetMaxAmmo(CBasePlayer* pPlayer, int iAmmoType) override;

protected:
	bool ChangePlayerClass(CBasePlayer* pPlayer, int classIndex);

protected:
	virtual void Enter_RND_RUNNING() override;
	virtual void Think_RND_RUNNING() override;

	virtual void SendMenusToClient(CBasePlayer* player) override;

protected:
	void InitializeTeams();

	int m_afToggleFlags;
	CTFTeamInfo m_TFTeamInfo[TEAM_SPECTATORS - 1];
};
