//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Whatever
//
// $NoKeywords: $
//=============================================================================

#pragma once

class CHLBot;

class CTFSpawnPoint : public CSpawnPoint
{
public:
	CTFSpawnPoint(int iTeamNumber = TEAM_UNASSIGNED);
	CTFSpawnPoint(CBaseEntity *pEntity, int iTeamNumber = TEAM_UNASSIGNED);

	bool IsValid(CBasePlayer *pPlayer, int attempt) override;

	int m_teamNumber;
};

class CTeamFortress : public CHalfLifeMultiplay
{
public:
	friend class CHLBot;

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
	int DeadPlayerWeapons(CBasePlayer* pPlayer) override { return GR_PLR_DROP_GUN_NO; }
	void AddPlayerSpawnSpot(CBaseEntity *pEntity) override;
	int GetMaxAmmo(CBasePlayer* pPlayer, int iAmmoType) override;

protected:
	bool ChangePlayerClass(CBasePlayer* pPlayer, int classIndex);

protected:
	virtual void Enter_RND_RUNNING() override;
	virtual void Think_RND_RUNNING() override;

	virtual void SendMenusToClient(CBasePlayer* player) override;
};
