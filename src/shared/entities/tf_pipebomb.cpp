//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
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


LINK_ENTITY_TO_CLASS(tf_weapon_gl, CGrenadeLauncher);

void CGrenadeLauncher::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_gl";
	i.iAmmo1 = AMMO_NONE;
	i.iMaxAmmo1 = -1;
	i.iAmmo2 = AMMO_NONE;
	i.iMaxAmmo2 = -1;
	i.iMaxClip = 6;
	i.iSlot = 3;
	i.iPosition = 1;
	i.iFlags = 0;
	i.iWeight = 15;

	i.pszWorld = "models/w_rpg.mdl";
	i.pszView = "models/v_tfgl.mdl";
	i.pszPlayer = "models/p_glauncher.mdl";
	i.pszAnimExt = "shotgun";

	i.iAnims[kWeaponAnimIdle] = 0;
	i.iAnims[kWeaponAnimDeploy] = 8;
	i.iAnims[kWeaponAnimHolster] = 10;
	i.iAnims[kWeaponAnimAttack] = 2;
	i.iAnims[kWeaponAnimReload] = -1;
	i.iAnims[kWeaponAnimStartReload] = 4;
	i.iAnims[kWeaponAnimEndReload] = 5;

	i.iShots = 1;

	i.iAttackTime = 600;
	i.iReloadTime = 4000;

	i.iProjectileType = kProjPipeBomb;
	i.iProjectileDamage = 100;
	i.vecProjectileSpread = Vector2D(0.0F, 0.0F);
	i.iProjectileCount = 1;
	i.iProjectileChargeDamage = 0;

	i.pszEvent = "events/wpn/tf_gl.sc";
	i.pszAttackSound = "weapons/glauncher.wav";
	i.pszAlternateSound = nullptr;
	i.pszReloadSound = nullptr;
	i.flPunchAngle = -2.0F;
	i.iSibling = WEAPON_PIPEBOMB_LAUNCHER;
}


LINK_ENTITY_TO_CLASS(tf_weapon_pl, CPipeBombLauncher);

void CPipeBombLauncher::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_pl";
	i.iAmmo1 = AMMO_NONE;
	i.iMaxAmmo1 = -1;
	i.iAmmo2 = AMMO_NONE;
	i.iMaxAmmo2 = -1;
	i.iMaxClip = 6;
	i.iSlot = 4;
	i.iPosition = 1;
	i.iFlags = 0;
	i.iWeight = 15;

	i.pszWorld = "models/w_rpg.mdl";
	i.pszView = "models/v_tfgl.mdl";
	i.pszPlayer = "models/p_glauncher.mdl";
	i.pszAnimExt = "shotgun";

	i.iAnims[kWeaponAnimIdle] = 1;
	i.iAnims[kWeaponAnimDeploy] = 9;
	i.iAnims[kWeaponAnimHolster] = 11;
	i.iAnims[kWeaponAnimAttack] = 3;
	i.iAnims[kWeaponAnimReload] = -1;
	i.iAnims[kWeaponAnimStartReload] = 6;
	i.iAnims[kWeaponAnimEndReload] = 7;

	i.iShots = 1;

	i.iAttackTime = 600;
	i.iReloadTime = 4000;

	i.iProjectileType = kProjPipeBombRemote;
	i.iProjectileDamage = 100;
	i.vecProjectileSpread = Vector2D(0.0F, 0.0F);
	i.iProjectileCount = 1;
	i.iProjectileChargeDamage = 0;

	i.pszEvent = "events/wpn/tf_pipel.sc";
	i.pszAttackSound = "weapons/glauncher.wav";
	i.pszAlternateSound = nullptr;
	i.pszReloadSound = nullptr;
	i.flPunchAngle = -2.0F;
	i.iSibling = WEAPON_GRENADE_LAUNCHER;
}


#ifdef GAME_DLL

CPipeBombLauncher::~CPipeBombLauncher()
{
	DetonatePipeBombs(true);
}


void CPipeBombLauncher::DetonatePipeBombs(const bool fizzle)
{
	for (; !m_pPipeBombs.empty(); m_pPipeBombs.pop())
	{
		CPipeBomb* pipebomb = m_pPipeBombs.front();

		if (pipebomb != nullptr)
		{
			if (fizzle)
			{
				pipebomb->Remove();
			}
			else
			{
				if (pipebomb->pev->pain_finished > gpGlobals->time)
				{
					break;
				}

				pipebomb->SetThink(&CPipeBomb::Detonate);
				pipebomb->pev->nextthink = gpGlobals->time;
			}
		}
	}
}


void CPipeBombLauncher::AddPipeBomb(CPipeBomb* pipebomb)
{
	EHANDLE hPipeBomb;
	
	hPipeBomb = (CBaseEntity*)pipebomb;

	m_pPipeBombs.push(hPipeBomb);

	if (m_pPipeBombs.size() > kMaxPipeBombs)
	{
		pipebomb = m_pPipeBombs.front();

		if (pipebomb != nullptr)
		{
			pipebomb->SetThink(&CPipeBomb::Detonate);
			pipebomb->pev->nextthink = gpGlobals->time;

			m_pPipeBombs.pop();
		}
	}
}

#endif