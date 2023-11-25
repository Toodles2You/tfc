//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Whatever
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"


LINK_ENTITY_TO_CLASS(weapon_crowbar, CCrowbar);


bool CCrowbar::Spawn()
{
	Precache();
	SetModel("models/w_crowbar.mdl");
	FallInit();
	m_iId = WEAPON_CROWBAR;
	m_iClip = WEAPON_NOCLIP;
	return true;
}


void CCrowbar::Precache()
{
	PRECACHE_MODEL("models/v_crowbar.mdl");
	PRECACHE_MODEL("models/w_crowbar.mdl");
	PRECACHE_MODEL("models/p_crowbar.mdl");

	PRECACHE_SOUND("weapons/cbar_hit1.wav");
	PRECACHE_SOUND("weapons/cbar_hit2.wav");

	PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod3.wav");

	PRECACHE_SOUND("weapons/cbar_miss1.wav");

	m_usCrowbar = PRECACHE_EVENT(1, "events/crowbar.sc");
}


bool CCrowbar::GetWeaponInfo(WeaponInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->iAmmo1 = AMMO_NONE;
	p->iMaxAmmo1 = -1;
	p->iAmmo2 = AMMO_NONE;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 0;
	p->iId = WEAPON_CROWBAR;
	p->iWeight = 0;
	return true;
}


bool CCrowbar::Deploy()
{
	return DefaultDeploy("models/v_crowbar.mdl", "models/p_crowbar.mdl", kAnimDraw, "crowbar");
}


bool CCrowbar::Holster()
{
	return DefaultHolster(kAnimHolster);
}


void CCrowbar::PrimaryAttack()
{
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	util::MakeVectors(m_pPlayer->pev->v_angle);

	Vector gun = m_pPlayer->GetGunPosition();

	TraceResult tr;
	util::TraceLine(
		gun,
		gun + gpGlobals->v_forward * 64,
		&tr,
		m_pPlayer,
		util::kTraceBox);

#ifdef CLIENT_DLL
	m_pPlayer->PlaybackEvent(m_usCrowbar);
#else
	auto hit = kCrowbarMiss;
	if (tr.flFraction != 1.0F)
	{
		hit = kCrowbarHitWorld;
		if (tr.pHit->v.solid != SOLID_BSP && tr.pHit->v.movetype != MOVETYPE_PUSHSTEP)
		{
			hit = kCrowbarHitPlayer;
		}
	}

	m_pPlayer->PlaybackEvent(
		m_usCrowbar,
		0.0F,
		0.0F,
		hit,
		0,
		true,
		false,
		FEV_RELIABLE | FEV_NOTHOST);
#endif

	if (tr.flFraction == 1.0F)
	{
		m_iNextPrimaryAttack = 500;
		return;
	}

	auto entity = CBaseEntity::Instance(tr.pHit);

	ClearMultiDamage();

	entity->TraceAttack(
		m_pPlayer,
		25,
		gpGlobals->v_forward,
		&tr,
		DMG_CLUB);

	ApplyMultiDamage(m_pPlayer, m_pPlayer);

	m_iNextPrimaryAttack = 250;
}


void CCrowbar::WeaponPostFrame()
{
	if ((m_pPlayer->pev->button & IN_ATTACK) != 0 && m_iNextPrimaryAttack <= 0)
	{
		PrimaryAttack();
	}
}

