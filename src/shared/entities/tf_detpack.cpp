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


LINK_ENTITY_TO_CLASS(tf_weapon_detpack, CDetpack);

void CDetpack::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_detpack";
	i.iAmmo1 = -1;
	i.iAmmo2 = -1;
	i.iMaxClip = -1;
	i.iSlot = 2;
	i.iPosition = 2;
	i.iFlags = 0;
	i.iWeight = -1;

	i.pszWorld = "models/detpack.mdl";
	i.pszView = "models/v_tripmine.mdl";
	i.pszPlayer = "models/p_tripmine.mdl";
	i.pszAnimExt = "knife";

	i.iAnims[kWeaponAnimIdle] = 0;
	i.iAnims[kWeaponAnimDeploy] = 6;
	i.iAnims[kWeaponAnimHolster] = 5;
	i.iAnims[kWeaponAnimAttack] = 2;
	i.iAnims[kWeaponAnimReload] = -1;
	i.iAnims[kWeaponAnimStartReload] = -1;
	i.iAnims[kWeaponAnimEndReload] = -1;

	i.iShots = 0;

	i.iAttackTime = 5000;
	i.iReloadTime = 5000;

	i.iProjectileType = kProjKinetic;
	i.iProjectileDamage = 0;
	i.vecProjectileSpread = Vector2D(0.0F, 0.0F);
	i.iProjectileCount = 0;

	i.pszEvent = nullptr;
	i.pszAttackSound = nullptr;
	i.pszAlternateSound = nullptr;
	i.pszReloadSound = nullptr;
	i.flPunchAngle = 0.0F;
	i.iSibling = -1;
	i.bShouldIdle = false;
}


void CDetpack::RemoveFromPlayer(bool forceSendAnimations)
{
	if (m_pPlayer == nullptr)
	{
		return;
	}

	m_pPlayer->m_TFState &= ~kTFStateBuilding;
	CTFWeapon::RemoveFromPlayer(forceSendAnimations);
}


void CDetpack::PrimaryAttack()
{
	const auto info = GetInfo();

	m_pPlayer->EmitSoundPredicted("weapons/mine_deploy.wav", CHAN_BODY, VOL_NORM, ATTN_IDLE);

	SendWeaponAnim(info.iAnims[kWeaponAnimAttack]);

	m_pPlayer->m_TFState |= kTFStateBuilding;
	m_iNextPrimaryAttack = info.iAttackTime;
}


void CDetpack::WeaponPostFrame()
{
	const auto info = GetInfo();

	if ((m_pPlayer->pev->button & IN_ATTACK) != 0)
	{
		if (m_iNextPrimaryAttack <= 0)
		{
			if ((m_pPlayer->m_TFState & kTFStateBuilding) != 0)
			{
				Set();
			}
			else
			{
				PrimaryAttack();
			}
		}
	}
	else if ((m_pPlayer->m_TFState & kTFStateBuilding) != 0)
	{
		SendWeaponAnim(info.iAnims[kWeaponAnimDeploy]);
		m_pPlayer->m_TFState &= ~kTFStateBuilding;
		m_iNextPrimaryAttack = 0;
	}
}


void CDetpack::Holster()
{
	m_pPlayer->m_TFState &= ~kTFStateBuilding;
	m_iNextPrimaryAttack = 0;
	CTFWeapon::Holster();
}


void CDetpack::Set()
{
	const auto info = GetInfo();

#ifdef GAME_DLL
	SetOrigin(m_pPlayer->pev->origin);
#endif

	RemoveFromPlayer(false);

#ifdef GAME_DLL
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_TOSS;

	SetModel("models/detpack.mdl");
	pev->effects &= ~EF_NODRAW;
	pev->angles.x = pev->angles.z = 0;

	EmitSound("weapons/mine_activate.wav", CHAN_VOICE);

	pev->pain_finished = std::max(pev->pain_finished, 5.0F);

	if (pev->pain_finished <= 6.0F)
	{
		FireInTheHole();
        pev->nextthink = gpGlobals->time + pev->pain_finished;
	}
	else
	{
		SetThink(&CDetpack::FireInTheHole);
		pev->nextthink = gpGlobals->time + pev->pain_finished - 6.0F;
	}
#endif
}


#ifdef GAME_DLL

void CDetpack::FireInTheHole()
{
	EmitSound("weapons/mine_charge.wav", CHAN_WEAPON);

	SetThink(&CDetpack::Detonate);
	pev->nextthink = gpGlobals->time + 6.0F;
}


void CDetpack::Detonate()
{
	const auto sploj = pev->origin + Vector(0.0F, 0.0F, 16.0F);

	tent::Explosion(
		sploj,
		g_vecZero,
		tent::ExplosionType::Detpack,
		180.0F);

	CBaseEntity* activator = CWorld::World;

	if (pev->owner != nullptr)
	{
		activator = CBaseEntity::Instance(pev->owner);
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

#endif /* GAME_DLL */

