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
//
// teamplay_gamerules.h
//

#pragma once

#define MAX_TEAMNAME_LENGTH 16

#define TEAMPLAY_TEAMLISTLENGTH ((TEAM_SPECTATORS - 1) * MAX_TEAMNAME_LENGTH)

class CHalfLifeTeamplay : public CHalfLifeMultiplay
{
public:
	CHalfLifeTeamplay();

	void ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer) override;
	bool IsTeamplay() override { return true; }
	bool FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker, CBaseEntity* inflictor = nullptr) override;
	int PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget) override;
	float GetPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled, bool assist = false) override;
	void InitHUD(CBasePlayer* pl) override;
	const char* GetGameDescription() override { return "HL Teamplay"; } // this is the game name that gets seen in the server browser
	gamemode_e GetGameMode() override { return kGamemodeTeamplay; }
	int GetDefaultPlayerTeam(CBasePlayer* pPlayer) override;
	bool ChangePlayerTeam(CBasePlayer* pPlayer, int teamIndex, bool bKill, bool bGib, bool bAutoTeam) override;
	void AddPointsToPlayer(CBasePlayer* player, float score = 1, bool allowNegative = false) override;

protected:
	virtual void Enter_RND_RUNNING() override;
	virtual void Think_RND_RUNNING() override;

	virtual void SendMenusToClient(CBasePlayer* player) override;
};
