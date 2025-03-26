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
	engine::PrecacheSound("weapons/cbar_hit1.wav");
	engine::PrecacheSound("weapons/cbar_hit2.wav");

	engine::PrecacheSound("weapons/cbar_hitbod1.wav");
	engine::PrecacheSound("weapons/cbar_hitbod2.wav");
	engine::PrecacheSound("weapons/cbar_hitbod3.wav");

	engine::PrecacheSound("weapons/cbar_miss1.wav");
#endif

	m_usPrimaryAttack = engine::PrecacheEvent(1, "events/crowbar.sc");
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
	AngleVectors(m_pPlayer->v.v_angle, &forward, nullptr, nullptr);

	Vector gun = m_pPlayer->GetGunPosition();
	Vector end = gun + forward * 64;
#ifdef GAME_DLL
	int ignore = m_pPlayer->v.GetIndex();
#else
	int ignore = -1; 
#endif

	Trace trace{gun, end, ignore, Trace::kBox};

	auto hit = kCrowbarMiss;
	if (trace.fraction != 1.0F)
	{
		hit = kCrowbarHitWorld;

#ifdef GAME_DLL
		auto other = Entity::FromIndex(trace.entity);

		if (other->solid != SOLID_BSP && other->movetype != MOVETYPE_PUSHSTEP)
		{
			hit = kCrowbarHitPlayer;
		}
#endif
	}

	if (hit == kCrowbarMiss)
		SendWeaponAnim(kAnimAttack1Miss);
	else
		SendWeaponAnim(kAnimAttack1Hit);

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
	auto other = Entity::FromIndex(trace.entity)->Get<CBaseEntity>();

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
	if ((m_pPlayer->v.button & IN_ATTACK) != 0 && m_iNextPrimaryAttack <= 0)
	{
		PrimaryAttack();
	}
}

