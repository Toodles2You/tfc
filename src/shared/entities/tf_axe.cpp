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
#include "UserMessages.h"
#include "gamerules.h"
#endif

#include <algorithm>


void CTFMelee::PrimaryAttack()
{
#ifdef GAME_DLL
	const auto info = GetInfo();

	const auto gun = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs;
	const auto aim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

	Vector dir;
	AngleVectors(aim, &dir, nullptr, nullptr);

	TraceResult tr;
	util::TraceLine(gun, gun + dir * 72.0F, &tr, m_pPlayer, util::kTraceBox);

	int result = kResultMiss;

	if (tr.flFraction != 1.0F)
	{
		const auto hit = CBaseEntity::Instance(tr.pHit);

		if (info.iProjectileType == kProjAdrenaline)
		{
			if (hit->IsClient())
			{
				if (g_pGameRules->PlayerRelationship(dynamic_cast<CBasePlayer*>(hit), m_pPlayer) >= GR_ALLY)
				{
					if (hit->pev->health >= hit->pev->max_health)
					{
						hit->TakeHealth(5, DMG_IGNORE_MAXHEALTH);
					}
					else
					{
						hit->TakeHealth(hit->pev->max_health - hit->pev->health, DMG_IGNORE_MAXHEALTH);
					}
				}
				else
				{
					hit->TraceAttack(
						m_pPlayer,
						info.iProjectileDamage,
						dir,
						tr.iHitgroup,
						DMG_GENERIC);

					if (hit->ApplyMultiDamage(m_pPlayer, m_pPlayer))
					{
						dynamic_cast<CBasePlayer*>(hit)->BecomeInfected(m_pPlayer);
					}
				}

				result = kResultHit;
			}
		}
		else
		{
			hit->TraceAttack(
				m_pPlayer,
				info.iProjectileDamage,
				dir,
				tr.iHitgroup,
				DMG_CLUB);

			hit->ApplyMultiDamage(m_pPlayer, m_pPlayer);

			result = kResultHitWorld;

			if (hit->IsClient())
			{
				if (g_pGameRules->FPlayerCanTakeDamage(dynamic_cast<CBasePlayer*>(hit), m_pPlayer))
				{
					MessageBegin(MSG_PVS, gmsgBlood, tr.vecEndPos);
					WriteFloat(dir.x);
					WriteFloat(dir.y);
					WriteFloat(dir.z);
					WriteByte(0);
					WriteByte(0);
					WriteCoord(tr.vecEndPos.x);
					WriteCoord(tr.vecEndPos.y);
					WriteCoord(tr.vecEndPos.z);
					MessageEnd();
				}

				result = kResultHit;
			}
		}
	}

	m_pPlayer->PlaybackEvent(m_usPrimaryAttack, (float)GetID(), 0.0F, m_pPlayer->m_randomSeed, result, true, false, FEV_RELIABLE);
#endif
}


void CTFMelee::WeaponPostFrame()
{
	const auto info = GetInfo();

	if (m_iNextPrimaryAttack <= 0)
	{
		if ((m_iWeaponState & kWpnStateReloading) != 0)
		{
			PrimaryAttack();
			m_iWeaponState &= ~kWpnStateReloading;
			m_iNextPrimaryAttack += info.iAttackTime - info.iReloadTime;
		}
		else if ((m_pPlayer->pev->button & IN_ATTACK) != 0)
		{
			m_pPlayer->SetAction(CBasePlayer::Action::Attack);

			m_pPlayer->PlaybackEvent(m_usPrimaryAttack, (float)GetID(), 0.0F, m_pPlayer->m_randomSeed, 0, false, false);

			if (info.iReloadTime > 0)
			{
				m_iWeaponState |= kWpnStateReloading;
				m_iNextPrimaryAttack += info.iReloadTime;
			}
			else
			{
				PrimaryAttack();
				m_iNextPrimaryAttack += info.iAttackTime;
			}
		}
		else
		{
			m_iNextPrimaryAttack = std::max(m_iNextPrimaryAttack, 0);
		}
	}
}


LINK_ENTITY_TO_CLASS(tf_weapon_medikit, CMedikit);

void CMedikit::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_medikit";
	i.iAmmo1 = AMMO_NONE;
	i.iMaxAmmo1 = -1;
	i.iAmmo2 = AMMO_NONE;
	i.iMaxAmmo2 = -1;
	i.iMaxClip = -1;
	i.iSlot = 0;
	i.iPosition = 1;
	i.iFlags = 0;
	i.iWeight = 15;

	i.pszWorld = "models/w_medkit.mdl";
	i.pszView = "models/v_tfc_medkit.mdl";
	i.pszPlayer = "models/p_medkit.mdl";
	i.pszAnimExt = "medkit";

	i.iAnims[kWeaponAnimIdle] = 0;
	i.iAnims[kWeaponAnimDeploy] = 5;
	i.iAnims[kWeaponAnimHolster] = 4;
	i.iAnims[kWeaponAnimAttack] = 3;
	i.iAnims[kWeaponAnimReload] = 2;
	i.iAnims[kWeaponAnimStartReload] = -1;
	i.iAnims[kWeaponAnimEndReload] = -1;

	i.iShots = 1;

	i.iAttackTime = 400;
	i.iReloadTime = 0;

	i.iProjectileType = kProjAdrenaline;
	i.iProjectileDamage = 10;
	i.vecProjectileSpread = Vector2D(0.0F, 0.0F);
	i.iProjectileCount = 1;
	i.iProjectileChargeDamage = 0;

	i.pszEvent = "events/wpn/tf_mednormal.sc";
	i.pszAttackSound = "items/medshot5.wav";
	i.pszAlternateSound = nullptr;
	i.pszReloadSound = "items/medshotno1.wav";
	i.flPunchAngle = 0.0F;
	i.iSibling = WEAPON_NONE;
}


LINK_ENTITY_TO_CLASS(tf_weapon_axe, CAxe);

void CAxe::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_axe";
	i.iAmmo1 = AMMO_NONE;
	i.iMaxAmmo1 = -1;
	i.iAmmo2 = AMMO_NONE;
	i.iMaxAmmo2 = -1;
	i.iMaxClip = -1;
	i.iSlot = 0;
	i.iPosition = 0;
	i.iFlags = 0;
	i.iWeight = 15;

	i.pszWorld = "models/w_crowbar.mdl";
	i.pszView = "models/v_tfc_crowbar.mdl";
	i.pszPlayer = "models/p_crowbar.mdl";
	i.pszAnimExt = "crowbar";

	i.iAnims[kWeaponAnimIdle] = 0;
	i.iAnims[kWeaponAnimDeploy] = 1;
	i.iAnims[kWeaponAnimHolster] = 2;
	i.iAnims[kWeaponAnimAttack] = -1;
	i.iAnims[kWeaponAnimReload] = 4;
	i.iAnims[kWeaponAnimStartReload] = -1;
	i.iAnims[kWeaponAnimEndReload] = -1;

	i.iShots = 1;

	i.iAttackTime = 400;
	i.iReloadTime = 0;

	i.iProjectileType = kProjKinetic;
	i.iProjectileDamage = 20;
	i.vecProjectileSpread = Vector2D(0.0F, 0.0F);
	i.iProjectileCount = 1;
	i.iProjectileChargeDamage = 0;

	i.pszEvent = "events/wpn/tf_axe.sc";
	i.pszAttackSound = "weapons/cbar_hitbod1.wav";
	i.pszAlternateSound = "weapons/cbar_hit1.wav";
	i.pszReloadSound = "weapons/cbar_miss1.wav";
	i.flPunchAngle = 0.0F;
	i.iSibling = WEAPON_NONE;
}

