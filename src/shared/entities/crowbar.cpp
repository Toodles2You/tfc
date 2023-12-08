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
#ifdef GAME_DLL
#include "gamerules.h"
#endif


LINK_ENTITY_TO_CLASS(weapon_crowbar, CCrowbar);


void CCrowbar::Precache()
{
#ifdef GAME_DLL
	PRECACHE_MODEL("models/v_crowbar.mdl");
	PRECACHE_MODEL("models/w_crowbar.mdl");
	PRECACHE_MODEL("models/p_crowbar.mdl");

	PRECACHE_SOUND("weapons/cbar_hit1.wav");
	PRECACHE_SOUND("weapons/cbar_hit2.wav");

	PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod3.wav");

	PRECACHE_SOUND("weapons/cbar_miss1.wav");
#endif

	m_usPrimaryAttack = g_engfuncs.pfnPrecacheEvent(1, "events/crowbar.sc");
}


bool CCrowbar::GetWeaponInfo(WeaponInfo* i)
{
	i->pszName = "weapon_crowbar";
	i->iAmmo1 = AMMO_NONE;
	i->iMaxAmmo1 = -1;
	i->iAmmo2 = AMMO_NONE;
	i->iMaxAmmo2 = -1;
	i->iMaxClip = -1;
	i->iSlot = 0;
	i->iPosition = 0;
	i->iWeight = 0;

	i->pszWorld = "models/w_crowbar.mdl";
	i->pszView = "models/v_crowbar.mdl";
	i->pszPlayer = "models/p_crowbar.mdl";
	i->pszAnimExt = "crowbar";

	i->iAnims[kWeaponAnimIdle] = kAnimIdle;
	i->iAnims[kWeaponAnimDeploy] = kAnimDeploy;
	i->iAnims[kWeaponAnimHolster] = kAnimHolster;

	return true;
}


void CCrowbar::PrimaryAttack()
{
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifdef CLIENT_DLL
	m_pPlayer->PlaybackEvent(m_usPrimaryAttack);
#else
	util::MakeVectors(m_pPlayer->pev->v_angle);


	Vector gun = m_pPlayer->GetGunPosition();

	TraceResult tr;
	util::TraceLine(
		gun,
		gun + gpGlobals->v_forward * 64,
		&tr,
		m_pPlayer,
		util::kTraceBox);

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
		m_usPrimaryAttack,
		0.0F,
		0.0F,
		hit,
		0,
		true,
		false,
		FEV_RELIABLE | FEV_NOTHOST);

	if (tr.flFraction == 1.0F)
	{
		m_iNextPrimaryAttack = 500;
		return;
	}

	auto entity = CBaseEntity::Instance(tr.pHit);

	entity->TraceAttack(
		m_pPlayer,
		25,
		gpGlobals->v_forward,
		&tr,
		DMG_CLUB);

	entity->ApplyMultiDamage(m_pPlayer, m_pPlayer);
#endif

	m_iNextPrimaryAttack = 250;
}


void CCrowbar::WeaponPostFrame()
{
	if ((m_pPlayer->pev->button & IN_ATTACK) != 0 && m_iNextPrimaryAttack <= 0)
	{
		PrimaryAttack();
	}
}

