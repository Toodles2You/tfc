//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Erecting
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


LINK_ENTITY_TO_CLASS(tf_weapon_detpack, CDetpack);

void CDetpack::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_detpack";
	i.iAmmo1 = -1;
	i.iAmmo2 = -1;
	i.iMaxClip = -1;
	i.iSlot = -1;
	i.iPosition = -1;
	i.iFlags = 0;
	i.iWeight = -1;

	i.pszWorld = "models/detpack.mdl";

	i.iAttackTime = 3000;
	i.iReloadTime = 5000;
}


void CDetpack::RemoveFromPlayer(bool forceSendAnimations)
{
	if (m_pPlayer == nullptr)
	{
		return;
	}

	m_pPlayer->SetWeaponHolstered(false, false);
	m_pPlayer->LeaveState(CBasePlayer::State::CannotMove);
	CTFWeapon::RemoveFromPlayer(forceSendAnimations);
}


void CDetpack::Deploy()
{
	const auto info = GetInfo();

	m_pPlayer->SetWeaponHolstered(true, false);
	m_pPlayer->EnterState(CBasePlayer::State::CannotMove);
	m_iNextPrimaryAttack = info.iAttackTime;
}


void CDetpack::WeaponPostFrame()
{
	if ((m_pPlayer->v.button & IN_SPECIAL) != 0)
	{
		if (!m_pPlayer->InState(CBasePlayer::State::Holstered))
		{
			Deploy();
		}
		else if (m_iNextPrimaryAttack <= 0)
		{
			Set();
		}
	}
	else if (m_pPlayer->InState(CBasePlayer::State::Holstered))
	{
		Holster();
	}
}


void CDetpack::Holster()
{
	m_pPlayer->SetWeaponHolstered(false, false);
	m_pPlayer->LeaveState(CBasePlayer::State::CannotMove);
	m_iNextPrimaryAttack = 0;
}


void CDetpack::Set()
{
	const auto info = GetInfo();

#ifdef GAME_DLL
	auto player = m_pPlayer;
#endif

	RemoveFromPlayer(false);

#ifdef GAME_DLL
	v.owner = player->v.pContainingEntity;
	v.team = player->TeamNumber();

	v.solid = SOLID_TRIGGER;
	v.movetype = MOVETYPE_TOSS;

	SetOrigin(player->v.origin);

	SetModel("models/detpack.mdl");
	v.effects &= ~EF_NODRAW;
	v.angles.x = v.angles.z = 0;

	EmitSound("weapons/mine_activate.wav", CHAN_VOICE);

	v.pain_finished = std::max(v.pain_finished, 5.0F);

	if (v.pain_finished <= 6.0F)
	{
		FireInTheHole();
        v.nextthink = gpGlobals->time + v.pain_finished;
	}
	else
	{
		SetThink(&CDetpack::FireInTheHole);
		v.nextthink = gpGlobals->time + v.pain_finished - 6.0F;
	}

	SetUse(&CDetpack::Disarm);
#endif
}


#ifdef GAME_DLL

void CDetpack::FireInTheHole()
{
	EmitSound("weapons/mine_charge.wav", CHAN_WEAPON);

	SetThink(&CDetpack::Detonate);
	v.nextthink = gpGlobals->time + 6.0F;
}


void CDetpack::Detonate()
{
	const auto sploj = v.origin + Vector(0.0F, 0.0F, 16.0F);

	tent::Explosion(
		sploj,
		g_vecZero,
		tent::ExplosionType::Detpack,
		180.0F);

	CBaseEntity* activator = CWorld::World;

	if (v.owner != nullptr)
	{
		activator = v.owner->Get<CBaseEntity>();
	}

	util::GoalDetpackUse(sploj, activator, this);

	RadiusDamage(
		sploj,
		this,
		activator,
		1400.0F,
		700.0F,
		WEAP_DETPACK_SIZE,
		DMG_BLAST | DMG_ALWAYSGIB);

	Remove();
}

void CDetpack::Disarm(CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value)
{
	if (!caller->IsPlayer())
	{
		return;
	}

	auto player = dynamic_cast<CBasePlayer*>(caller);

	if (player->PCNumber() != PC_SCOUT)
	{
		return;
	}

	if (g_pGameRules->PlayerRelationship(player, this) >= GR_ALLY)
	{
		return;
	}

	const auto info = GetInfo();

	switch (useType)
	{
		case USE_CONTINUOUS_BEGIN:
		{
			v.pain_finished = gpGlobals->time + info.iReloadTime / 1000.0F;
			EmitSound("weapons/mine_disarm.wav", CHAN_ITEM);
			player->SetWeaponHolstered(true);
			break;
		}
		case USE_CONTINUOUS:
		{
			if (v.pain_finished <= gpGlobals->time)
			{
	            g_pGameRules->AddPointsToPlayer(player, 1);
				EmitSound("weapons/mine_disarmed.wav", CHAN_ITEM);
				player->SetWeaponHolstered(false);
				Remove();
			}
			break;
		}
		case USE_CONTINUOUS_END:
		{
			player->SetWeaponHolstered(false);
			break;
		}
	}
}


#endif /* GAME_DLL */

