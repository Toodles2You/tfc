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
	util::TraceLine(gun, gun + dir * 64.0F, &tr, m_pPlayer, util::kTraceBox);

	edict_t* first = tr.pHit;
	int result = kResultMiss;

	if (tr.flFraction != 1.0F)
	{
		result = HitEntity(CBaseEntity::Instance(tr.pHit), dir, tr);

		if (info.iProjectileCount != -1)
		{
			goto finished;
		}
	}

	{
		CBaseEntity* entity = nullptr;
		CBaseEntity* closest = nullptr;
		float bestDot = 0.4F;

		while ((entity = util::FindEntityInSphere(entity, gun, 64.0F)) != nullptr)
		{
			if (first != entity->edict() && entity->pev->takedamage != DAMAGE_NO)
			{
				Vector los = entity->Center() - gun;

				los = util::ClampVectorToBox(los, entity->pev->size * 0.5);

				float dot = DotProduct(los.Make2D(), dir.Make2D());

				if (dot > bestDot)
				{
					util::TraceLine(gun, entity->pev->origin + los, &tr, m_pPlayer, util::kTraceBox);

					if (tr.pHit == entity->edict())
					{
						closest = entity;
						bestDot = dot;

						int newResult = HitEntity(CBaseEntity::Instance(tr.pHit), dir, tr);

						if (result < newResult)
						{
							result = newResult;

							if (info.iProjectileCount != -1)
							{
								goto finished;
							}
						}

					}
				}
			}
		}
	}

finished:
	m_pPlayer->PlaybackEvent(m_usPrimaryAttack, (float)GetID(), m_pPlayer->pev->view_ofs.z, m_pPlayer->m_randomSeed, result, true, false, FEV_RELIABLE);
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

			m_pPlayer->PlaybackEvent(m_usPrimaryAttack, (float)GetID(), m_pPlayer->pev->view_ofs.z, m_pPlayer->m_randomSeed, 0, false, false);

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


#ifdef GAME_DLL

int CTFMelee::HitEntity(CBaseEntity* hit, const Vector& dir, const TraceResult& tr)
{
	const auto info = GetInfo();
	int result = kResultHitWorld;

	hit->TraceAttack(
		m_pPlayer,
		info.iProjectileDamage,
		dir,
		tr.iHitgroup,
		DMG_CLUB);

	hit->ApplyMultiDamage(m_pPlayer, m_pPlayer);

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

	return result;
}

#endif


LINK_ENTITY_TO_CLASS(tf_weapon_medikit, CMedikit);

void CMedikit::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_medikit";
	i.iAmmo1 = -1;
	i.iAmmo2 = -1;
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
	i.iSibling = -1;
	i.bShouldIdle = false;
}


#ifdef GAME_DLL

int CMedikit::HitEntity(CBaseEntity* hit, const Vector& dir, const TraceResult& tr)
{
	const auto info = GetInfo();
	int result = kResultMiss;

	if (hit->IsClient())
	{
		result = kResultHit;

		if (g_pGameRules->PlayerRelationship(dynamic_cast<CBasePlayer*>(hit), m_pPlayer) >= GR_ALLY)
		{
			if (hit->pev->health >= hit->pev->max_health)
			{
				hit->GiveHealth(5, DMG_IGNORE_MAXHEALTH);
			}
			else
			{
				hit->GiveHealth(hit->pev->max_health - hit->pev->health, DMG_IGNORE_MAXHEALTH);
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
	}

	return result;
}

#endif


LINK_ENTITY_TO_CLASS(tf_weapon_axe, CAxe);

void CAxe::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_axe";
	i.iAmmo1 = -1;
	i.iAmmo2 = -1;
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
	i.iProjectileCount = -1;
	i.iProjectileChargeDamage = 0;

	i.pszEvent = "events/wpn/tf_axe.sc";
	i.pszAttackSound = "weapons/cbar_hitbod1.wav";
	i.pszAlternateSound = "weapons/cbar_hit1.wav";
	i.pszReloadSound = "weapons/cbar_miss1.wav";
	i.flPunchAngle = 0.0F;
	i.iSibling = -1;
	i.bShouldIdle = false;
}

