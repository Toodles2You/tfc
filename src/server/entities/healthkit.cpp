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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "items.h"
#include "gamerules.h"
#include "UserMessages.h"

class CHealthKit : public CItem
{
	bool Spawn() override;
	void Precache() override;
	bool MyTouch(CBasePlayer* pPlayer) override;

	/*
	int		Save( CSave &save ) override; 
	int		Restore( CRestore &restore ) override;
	
	static	TYPEDESCRIPTION m_SaveData[];
*/
};


LINK_ENTITY_TO_CLASS(item_healthkit, CHealthKit);

/*
TYPEDESCRIPTION	CHealthKit::m_SaveData[] = 
{

};


IMPLEMENT_SAVERESTORE( CHealthKit, CItem);
*/

bool CHealthKit::Spawn()
{
	Precache();
	SetModel("models/w_medkit.mdl");

	return CItem::Spawn();
}

void CHealthKit::Precache()
{
	PRECACHE_MODEL("models/w_medkit.mdl");
	PRECACHE_SOUND("items/smallmedkit1.wav");
}

bool CHealthKit::MyTouch(CBasePlayer* pPlayer)
{
	if (pPlayer->pev->deadflag != DEAD_NO)
	{
		return false;
	}

	if (pPlayer->TakeHealth(15, DMG_GENERIC))
	{
		MessageBegin(MSG_ONE, gmsgItemPickup, pPlayer);
		WriteString(STRING(pev->classname));
		MessageEnd();

		pPlayer->EmitSound("items/smallmedkit1.wav", CHAN_ITEM);
		return true;
	}

	return false;
}



//-------------------------------------------------------------
// Wall mounted health kit
//-------------------------------------------------------------
class CWallHealth : public CBaseToggle
{
public:
	bool Spawn() override;
	void Precache() override;
	void EXPORT Off();
	void EXPORT Recharge();
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	int ObjectCaps() override { return (CBaseToggle::ObjectCaps() | FCAP_CONTINUOUS_USE) & ~FCAP_ACROSS_TRANSITION; }
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	float m_flNextCharge;
	int m_iReactivate; // DeathMatch Delay until reactvated
	int m_iJuice;
	int m_iOn; // 0 = off, 1 = startup, 2 = going
	float m_flSoundTime;
};

TYPEDESCRIPTION CWallHealth::m_SaveData[] =
	{
		DEFINE_FIELD(CWallHealth, m_flNextCharge, FIELD_TIME),
		DEFINE_FIELD(CWallHealth, m_iReactivate, FIELD_INTEGER),
		DEFINE_FIELD(CWallHealth, m_iJuice, FIELD_INTEGER),
		DEFINE_FIELD(CWallHealth, m_iOn, FIELD_INTEGER),
		DEFINE_FIELD(CWallHealth, m_flSoundTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CWallHealth, CBaseEntity);

LINK_ENTITY_TO_CLASS(func_healthcharger, CWallHealth);


bool CWallHealth::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "style") ||
		FStrEq(pkvd->szKeyName, "height") ||
		FStrEq(pkvd->szKeyName, "value1") ||
		FStrEq(pkvd->szKeyName, "value2") ||
		FStrEq(pkvd->szKeyName, "value3"))
	{
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "dmdelay"))
	{
		m_iReactivate = atoi(pkvd->szValue);
		return true;
	}

	return CBaseToggle::KeyValue(pkvd);
}

bool CWallHealth::Spawn()
{
	Precache();

	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	SetSize(pev->mins, pev->maxs);
	SetOrigin(pev->origin); // set size and link into world
	SetModel(STRING(pev->model));
	m_iJuice = 40;
	pev->frame = 0;

	return true;
}

void CWallHealth::Precache()
{
	PRECACHE_SOUND("items/medshot4.wav");
	PRECACHE_SOUND("items/medshotno1.wav");
	PRECACHE_SOUND("items/medcharge4.wav");
}


void CWallHealth::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	// Make sure that we have a caller
	if (!pActivator)
		return;
	// if it's not a player, ignore
	if (!pActivator->IsPlayer())
		return;

	auto player = static_cast<CBasePlayer*>(pActivator);

	// if there is no juice left, turn it off
	if (m_iJuice <= 0)
	{
		pev->frame = 1;
		Off();
	}

	// if there is no juice left, make the deny noise
	if (m_iJuice <= 0)
	{
		if (m_flSoundTime <= gpGlobals->time)
		{
			m_flSoundTime = gpGlobals->time + 0.62;
			EmitSound("items/medshotno1.wav", CHAN_ITEM);
		}
		return;
	}

	pev->nextthink = pev->ltime + 0.25;
	SetThink(&CWallHealth::Off);

	// Time to recharge yet?

	if (m_flNextCharge >= gpGlobals->time)
		return;

	// Play the on sound or the looping charging sound
	if (0 == m_iOn)
	{
		m_iOn++;
		EmitSound("items/medshot4.wav", CHAN_ITEM);
		m_flSoundTime = 0.56 + gpGlobals->time;
	}
	if ((m_iOn == 1) && (m_flSoundTime <= gpGlobals->time))
	{
		m_iOn++;
		EmitSound("items/medcharge4.wav", CHAN_STATIC);
	}


	// charge the player
	if (player->TakeHealth(1, DMG_GENERIC))
	{
		m_iJuice--;
	}

	// govern the rate of charge
	m_flNextCharge = gpGlobals->time + 0.1;
}

void CWallHealth::Recharge()
{
	EmitSound("items/medshot4.wav", CHAN_ITEM);
	m_iJuice = 40;
	pev->frame = 0;
	SetThink(nullptr);
}

void CWallHealth::Off()
{
	// Stop looping sound.
	if (m_iOn > 1)
		StopSound("items/medcharge4.wav", CHAN_STATIC);

	m_iOn = 0;

	if ((0 == m_iJuice) && ((m_iReactivate = g_pGameRules->FlHealthChargerRechargeTime()) > 0))
	{
		pev->nextthink = pev->ltime + m_iReactivate;
		SetThink(&CWallHealth::Recharge);
	}
	else
		SetThink(nullptr);
}