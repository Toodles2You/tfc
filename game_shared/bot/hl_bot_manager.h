//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef HL_BOT_MANAGER_H
#define HL_BOT_MANAGER_H

class CHLBotManager;
using CGameBotManager = CHLBotManager;

class BotProfile;

#include "nav.h"
#include "bot_manager.h"

class CHLBotManager : public CBotManager
{
public:
	CHLBotManager();

	void ClientDisconnect(CBasePlayer *pPlayer) override;
	bool ClientCommand(CBasePlayer *pPlayer, const char *pcmd) override;

	void ServerActivate() override;
	void ServerDeactivate() override;
	void ServerCommand(const char *pcmd) override;
	void AddServerCommand(const char *cmd) override;
	void AddServerCommands() override;

	unsigned int GetPlayerPriority(CBasePlayer *player) const override;

	Place GetNavPlace() { return m_NavPlace; }
	void SetNavPlace(Place place) { m_NavPlace = place; }

protected:
	void AddBot(const char *profile = nullptr);

	Place m_NavPlace;
};

inline DLL_GLOBAL CBotManager *g_pBotMan = nullptr;


#endif // HL_BOT_MANAGER_H
