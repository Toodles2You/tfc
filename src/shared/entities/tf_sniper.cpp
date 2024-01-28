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
#include "customentity.h"
#endif

#include <algorithm>


LINK_ENTITY_TO_CLASS(tf_weapon_sniperrifle, CSniperRifle);

void CSniperRifle::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_sniperrifle";
	i.iAmmo1 = AMMO_NONE;
	i.iMaxAmmo1 = -1;
	i.iAmmo2 = AMMO_NONE;
	i.iMaxAmmo2 = -1;
	i.iMaxClip = -1;
	i.iSlot = 1;
	i.iPosition = 1;
	i.iFlags = 0;
	i.iWeight = 15;

	i.pszWorld = "models/w_crossbow.mdl";
	i.pszView = "models/v_tfc_sniper.mdl";
	i.pszPlayer = "models/p_sniper.mdl";
	i.pszAnimExt = "sniper";

	i.iAnims[kWeaponAnimIdle] = 0;
	i.iAnims[kWeaponAnimDeploy] = 3;
	i.iAnims[kWeaponAnimHolster] = 4;
	i.iAnims[kWeaponAnimAttack] = 2;
	i.iAnims[kWeaponAnimReload] = 1;
	i.iAnims[kWeaponAnimStartReload] = -1;
	i.iAnims[kWeaponAnimEndReload] = -1;

	i.iShots = 1;

	i.iAttackTime = 1500;
	i.iReloadTime = 13000;

	i.iProjectileType = kProjBullet;
	i.iProjectileDamage = 50;
	i.vecProjectileSpread = Vector2D(0.0F, 0.0F);
	i.iProjectileCount = 1;
	i.iProjectileChargeDamage = 400;

	i.pszEvent = "events/wpn/tf_sniper.sc";
	i.pszAttackSound = "ambience/rifle1.wav";
	i.pszAlternateSound = nullptr;
	i.pszReloadSound = nullptr;
	i.flPunchAngle = -4.0F;
	i.iSibling = WEAPON_AUTO_RIFLE;
}


void CSniperRifle::PrimaryAttack()
{
	const auto info = GetInfo();

	m_pPlayer->SetAction(CBasePlayer::Action::Attack);

	float damageScale = std::clamp(1.0F - (float)m_iNextPrimaryAttack / info.iReloadTime, 0.0F, 1.0F);
	float damageMod = (info.iProjectileChargeDamage - info.iProjectileDamage) * damageScale;
	int damage = std::roundf(info.iProjectileDamage + damageMod);

	int damageType = DMG_BULLET;

	if (damageScale > 0.0F)
	{
		damageType |= DMG_AIMED | DMG_CALTROP;
	}

#ifndef NDEBUG
	ALERT(at_console, "SNIPER RIFLE: %i (%i%%)\n", damage, (int)(damageScale * 100));
#endif

#ifdef GAME_DLL
	const auto gun = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs;
	const auto aim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

	Vector dir;
	AngleVectors(aim, &dir, nullptr, nullptr);

	TraceResult tr;
	util::TraceLine(gun, gun + dir * 4096.0F, &tr, m_pPlayer, util::kTraceBox | util::kTraceBoxModel);

	if (tr.flFraction != 1.0F)
	{
		const auto hit = CBaseEntity::Instance(tr.pHit);

		if (hit->IsClient())
		{
			float distance = (hit->pev->origin - m_pPlayer->pev->origin).Length();

			if (distance > 1536.0F && dynamic_cast<CBasePlayer*>(hit)->PCNumber() != PC_SNIPER)
			{
				damage *= 0.5F;
				damageType &= ~DMG_AIMED | DMG_CALTROP;
#ifndef NDEBUG
				ALERT(at_console, "SNIPER TOO FAR\n");
#endif
			}
			else if (distance < 512.0F)
			{
				damage *= 0.8F;
				damageType &= ~DMG_AIMED;
#ifndef NDEBUG
				ALERT(at_console, "SNIPER TOO CLOSE\n");
#endif
			}
		}

		hit->TraceAttack(
			m_pPlayer,
			damage,
			dir,
			tr.iHitgroup,
			damageType);
		
		hit->ApplyMultiDamage(m_pPlayer, m_pPlayer);
		
		if (hit->IsClient()
		 && g_pGameRules->FPlayerCanTakeDamage(dynamic_cast<CBasePlayer*>(hit), m_pPlayer))
		{
			MessageBegin(MSG_PVS, gmsgBlood, tr.vecEndPos);
			WriteFloat(dir.x);
			WriteFloat(dir.y);
			WriteFloat(dir.z);
			WriteByte(0);

			if (dynamic_cast<CBasePlayer*>(hit)->m_LastHitGroup == HITGROUP_HEAD)
			{
				WriteByte(1);
			}
			else
			{
				WriteByte(0);
			}

			WriteCoord(tr.vecEndPos.x);
			WriteCoord(tr.vecEndPos.y);
			WriteCoord(tr.vecEndPos.z);
			MessageEnd();
		}
	}
#endif

	m_pPlayer->PlaybackEvent(m_usPrimaryAttack, (float)GetID(), 0.0F, m_pPlayer->m_randomSeed, 1);
}


void CSniperRifle::WeaponPostFrame()
{
	const auto info = GetInfo();

	if ((m_pPlayer->m_afButtonPressed & IN_ATTACK2) != 0)
	{
		if (m_pPlayer->m_iFOV == 0)
		{
			m_pPlayer->m_iFOV = 20;
		}
		else
		{
			m_pPlayer->m_iFOV = 0;
		}

#ifdef CLIENT_DLL
		m_pPlayer->EmitSoundPredicted("weapons/zoom.wav", CHAN_ITEM);
#endif
	}

	if ((m_pPlayer->pev->button & IN_ATTACK) != 0)
	{
		if ((m_pPlayer->m_TFState & kTFStateAiming) == 0 && m_iNextPrimaryAttack <= 0)
		{
			SendWeaponAnim(info.iAnims[kWeaponAnimReload]);
			m_pPlayer->m_TFState |= kTFStateAiming;
			m_iNextPrimaryAttack = 333 + info.iReloadTime;
#ifdef GAME_DLL
			CreateLaserEffect();
#endif
		}
	}
	else if ((m_pPlayer->m_TFState & kTFStateAiming) != 0)
	{
		PrimaryAttack();
		m_pPlayer->m_TFState &= ~kTFStateAiming;
		m_iNextPrimaryAttack = info.iAttackTime;
#ifdef GAME_DLL
		DestroyLaserEffect();
#endif
	}

#ifdef GAME_DLL
	UpdateLaserEffect();
#endif
}


void CSniperRifle::Holster()
{
	CTFWeapon::Holster();
#ifdef GAME_DLL
	DestroyLaserEffect();
#endif
}

#ifdef GAME_DLL
void CSniperRifle::CreateLaserEffect()
{
	DestroyLaserEffect();

	m_pLaserDot = CSprite::SpriteCreate("sprites/laserdot.spr", pev->origin, false);
	m_pLaserDot->pev->scale = 1.0;
	m_pLaserDot->pev->spawnflags |= SF_SPRITE_TEMPORARY;
#ifdef NDEBUG
	m_pLaserDot->pev->flags |= FL_SKIPLOCALHOST;
#endif
	m_pLaserDot->pev->owner = m_pPlayer->edict();

	m_pLaserBeam = CBeam::BeamCreate("sprites/laserbeam.spr", 16);
	m_pLaserBeam->PointEntInit(pev->origin, m_pPlayer->entindex());
	m_pLaserBeam->SetFlags(BEAM_FSHADEOUT);
	m_pLaserBeam->SetEndAttachment(1);
	m_pLaserBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
#ifdef NDEBUG
	m_pLaserBeam->pev->flags |= FL_SKIPLOCALHOST;
#endif
	m_pLaserBeam->pev->owner = m_pPlayer->edict();

	if (m_pPlayer->TeamNumber() == TEAM_BLUE)
	{
		m_pLaserDot->SetTransparency(kRenderGlow, 51, 51, 255, 223, kRenderFxNoDissipation);
		m_pLaserBeam->SetColor(51, 51, 255);
		m_pLaserBeam->SetBrightness(159);
	}
	else
	{
		m_pLaserDot->SetTransparency(kRenderGlow, 255, 0, 0, 159, kRenderFxNoDissipation);
		m_pLaserBeam->SetColor(255, 0, 0);
		m_pLaserBeam->SetBrightness(63);
	}
}


void CSniperRifle::UpdateLaserEffect()
{
	if (m_pLaserDot != nullptr || m_pLaserBeam != nullptr)
	{
		const auto gun = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs;
		const auto aim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		Vector dir;
		AngleVectors(aim, &dir, nullptr, nullptr);

		TraceResult tr;
		util::TraceLine(gun, gun + dir * 4096.0F, &tr, m_pPlayer, util::kTraceBox);

		if (m_pLaserDot != nullptr)
		{
			m_pLaserDot->pev->effects |= EF_NOINTERP;
			m_pLaserDot->SetOrigin(tr.vecEndPos);
		}

		if (m_pLaserBeam != nullptr)
		{
			m_pLaserBeam->SetStartPos(tr.vecEndPos);
		}
	}
}


void CSniperRifle::DestroyLaserEffect()
{
	if (m_pLaserDot != nullptr)
	{
		m_pLaserDot->Remove();
		m_pLaserDot = nullptr;
	}

	if (m_pLaserBeam != nullptr)
	{
		m_pLaserBeam->Remove();
		m_pLaserBeam = nullptr;
	}
}
#endif


LINK_ENTITY_TO_CLASS(tf_weapon_autorifle, CAutoRifle);

void CAutoRifle::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_autorifle";
	i.iAmmo1 = AMMO_NONE;
	i.iMaxAmmo1 = -1;
	i.iAmmo2 = AMMO_NONE;
	i.iMaxAmmo2 = -1;
	i.iMaxClip = -1;
	i.iSlot = 2;
	i.iPosition = 0;
	i.iFlags = 0;
	i.iWeight = 15;

	i.pszWorld = "models/w_crossbow.mdl";
	i.pszView = "models/v_tfc_sniper.mdl";
	i.pszPlayer = "models/p_sniper.mdl";
	i.pszAnimExt = "autosniper";

	i.iAnims[kWeaponAnimIdle] = 5;
	i.iAnims[kWeaponAnimDeploy] = 7;
	i.iAnims[kWeaponAnimHolster] = 8;
	i.iAnims[kWeaponAnimAttack] = 6;
	i.iAnims[kWeaponAnimReload] = -1;
	i.iAnims[kWeaponAnimStartReload] = -1;
	i.iAnims[kWeaponAnimEndReload] = -1;

	i.iShots = 1;

	i.iAttackTime = 100;
	i.iReloadTime = 1500;

	i.iProjectileType = kProjBullet;
	i.iProjectileDamage = 8;
	i.vecProjectileSpread = Vector2D(2.3F, 2.3F);
	i.iProjectileCount = 1;
	i.iProjectileChargeDamage = 0;

	i.pszEvent = "events/wpn/tf_ar.sc";
	i.pszAttackSound = "weapons/sniper.wav";
	i.pszAlternateSound = nullptr;
	i.pszReloadSound = nullptr;
	i.flPunchAngle = -2.0F;
	i.iSibling = WEAPON_SNIPER_RIFLE;
}

