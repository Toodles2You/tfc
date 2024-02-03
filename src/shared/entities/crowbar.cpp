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
#include "trace.h"
#ifdef GAME_DLL
#include "gamerules.h"
#endif


LINK_ENTITY_TO_CLASS(weapon_crowbar, CCrowbar);


void CCrowbar::Precache()
{
	CBasePlayerWeapon::Precache();

#ifdef GAME_DLL
	g_engfuncs.pfnPrecacheSound("weapons/cbar_hit1.wav");
	g_engfuncs.pfnPrecacheSound("weapons/cbar_hit2.wav");

	g_engfuncs.pfnPrecacheSound("weapons/cbar_hitbod1.wav");
	g_engfuncs.pfnPrecacheSound("weapons/cbar_hitbod2.wav");
	g_engfuncs.pfnPrecacheSound("weapons/cbar_hitbod3.wav");

	g_engfuncs.pfnPrecacheSound("weapons/cbar_miss1.wav");
#endif

	m_usPrimaryAttack = g_engfuncs.pfnPrecacheEvent(1, "events/crowbar.sc");
}


void CCrowbar::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "weapon_crowbar";
	i.iAmmo1 = -1;
	i.iAmmo2 = -1;
	i.iMaxClip = -1;
	i.iSlot = 0;
	i.iPosition = 0;
	i.iWeight = 0;

	i.pszWorld = "models/w_crowbar.mdl";
	i.pszView = "models/v_crowbar.mdl";
	i.pszPlayer = "models/p_crowbar.mdl";
	i.pszAnimExt = "crowbar";

	i.iAnims[kWeaponAnimIdle] = kAnimIdle;
	i.iAnims[kWeaponAnimDeploy] = kAnimDeploy;
	i.iAnims[kWeaponAnimHolster] = kAnimHolster;
}


void CCrowbar::PrimaryAttack()
{
	m_pPlayer->SetAction(CBasePlayer::Action::Attack);

	Vector forward;
	AngleVectors(m_pPlayer->pev->v_angle, &forward, nullptr, nullptr);

	Vector gun = m_pPlayer->GetGunPosition();
	Vector end = gun + forward * 64;

	Trace trace{gun, end, m_pPlayer->entindex(), Trace::kBox};

	auto hit = kCrowbarMiss;
	if (trace.fraction != 1.0F)
	{
		hit = kCrowbarHitWorld;

#ifdef GAME_DLL
		auto other = g_engfuncs.pfnPEntityOfEntIndex(trace.entity);

		if (other->v.solid != SOLID_BSP && other->v.movetype != MOVETYPE_PUSHSTEP)
		{
			hit = kCrowbarHitPlayer;
		}
#endif
	}

#ifdef GAME_DLL
	m_pPlayer->PlaybackEvent(
		m_usPrimaryAttack,
		0.0F,
		0.0F,
		hit,
		0,
		true,
		false,
		FEV_RELIABLE | FEV_NOTHOST);
#else
	m_pPlayer->PlaybackEvent(m_usPrimaryAttack);
#endif

	if (trace.fraction == 1.0F)
	{
		m_iNextPrimaryAttack = 500;
		return;
	}

#ifdef GAME_DLL
	auto other = CBaseEntity::Instance(g_engfuncs.pfnPEntityOfEntIndex(trace.entity));

	other->TraceAttack(
		m_pPlayer,
		25,
		gpGlobals->v_forward,
		trace.hitgroup,
		DMG_CLUB);

	other->ApplyMultiDamage(m_pPlayer, m_pPlayer);
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

