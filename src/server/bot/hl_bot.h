//========= Copyright ï¿½ 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef HL_BOT_H
#define HL_BOT_H

#include "bot.h"

class CHLBot : public CBot
{
public:
	CHLBot(Entity* containingEntity);

	bool Initialize(const BotProfile* profile) override;
	void SpawnBot() override;
	void Upkeep() override;
	void Update() override;

	bool IsVisible(const Vector* pos, bool testFOV = false) override;
	bool IsVisible(CBasePlayer* player, bool testFOV = false, unsigned char* visParts = NULL) override;

	bool IsEnemyPartVisible(VisiblePartType part) override;

protected:
	CBaseEntity *m_pEnemy;
};

#endif // HL_BOT_H
