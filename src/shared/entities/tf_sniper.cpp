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
	i.iAmmo1 = AMMO_SHELLS;
	i.iAmmo2 = -1;
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
	i.iProjectileRange = 4096;

	i.pszEvent = "events/wpn/tf_sniper.sc";
	i.pszAttackSound = "ambience/rifle1.wav";
	i.pszAlternateSound = nullptr;
	i.pszReloadSound = nullptr;
	i.flPunchAngle = -4.0F;
	i.iSibling = WEAPON_AUTO_RIFLE;
	i.bShouldIdle = false;
}


void CSniperRifle::Precache()
{
	CTFWeapon::Precache();
	m_usLaserDotOn = g_engfuncs.pfnPrecacheEvent(1, "events/laser_on.sc");
	m_usLaserDotOff = g_engfuncs.pfnPrecacheEvent(1, "events/laser_off.sc");
}


void CSniperRifle::PrimaryAttack()
{
	const auto info = GetInfo();

	m_pPlayer->SetAction(CBasePlayer::Action::Attack);

	float damageScale = std::clamp(1.0F - (float)m_iNextPrimaryAttack / info.iReloadTime, 0.0F, 1.0F);
	float damageMod = (info.iProjectileChargeDamage - info.iProjectileDamage) * damageScale;
	int damage = std::roundf(info.iProjectileDamage + damageMod);

	int damageType = DMG_BULLET;

	/*
		Charge damage, headshots & legshots are only checked if the
		sniper rifle has been charging for at least a third of a second.
	*/
	if (damageScale > 0.0F)
	{
		damageType |= DMG_CALTROP;

		/* Headshot players if the sniper rifle has been scoped in for at least half of a second. */
		if (m_pPlayer->m_iFOV != 0 && m_iScopeTime <= 0)
		{
			damageType |= DMG_AIMED;
		}
	}

	int shots = std::ceil(damage / (float)info.iProjectileDamage);

	/* Charge would use more ammo than what the player has. */
	if (shots * info.iShots > m_pPlayer->m_rgAmmo[info.iAmmo1])
	{
		/* Cap it. */
		shots = std::floor(m_pPlayer->m_rgAmmo[info.iAmmo1] / (float)info.iShots);
		damage = std::min(damage, info.iProjectileDamage * shots);
		damageScale = damage / (float)(info.iProjectileChargeDamage - info.iProjectileDamage);
	}

	m_pPlayer->m_rgAmmo[info.iAmmo1] -= shots;

#ifndef NDEBUG
	ALERT(at_console, "SNIPER RIFLE: %i (%i%%)\n", damage, (int)(damageScale * 100));
#endif

	m_pPlayer->PlaybackEvent(m_usPrimaryAttack, (float)GetID(), m_pPlayer->pev->view_ofs.z, m_pPlayer->m_randomSeed, 1);

#ifdef GAME_DLL
	const auto gun = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs;
	const auto aim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

	Vector dir;
	AngleVectors(aim, &dir, nullptr, nullptr);

	TraceResult tr;
	util::TraceLine(gun, gun + dir * info.iProjectileRange, &tr, m_pPlayer, util::kTraceBox | util::kTraceBoxModel);

	if (tr.flFraction == 1.0F)
	{
		return;
	}

	const auto hit = CBaseEntity::Instance(tr.pHit);

	if (hit == nullptr || hit->pev->takedamage == DAMAGE_NO)
	{
		return;
	}

	if (hit->IsClient())
	{
		float distance = (hit->pev->origin - m_pPlayer->pev->origin).Length();

		/*
			Don't headshot or legshot players who are very far away.
			Unless they're a sniper, to keep those epic sniper duels.
		*/
		if (distance > 1536.0F && dynamic_cast<CBasePlayer*>(hit)->PCNumber() != PC_SNIPER)
		{
#ifndef NDEBUG
			if ((damageType & (DMG_AIMED | DMG_CALTROP)) != 0)
			{
				ALERT(at_console, "SNIPER TOO FAR\n");
			}
#endif
			damageType &= ~(DMG_AIMED | DMG_CALTROP);
		}
		/* Don't headshot players who are very close. */
		else if (distance < 512.0F)
		{
#ifndef NDEBUG
			if ((damageType & DMG_AIMED) != 0)
			{
				ALERT(at_console, "SNIPER TOO CLOSE\n");
			}
#endif
			damageType &= ~DMG_AIMED;
		}
	}

	hit->TraceAttack(
		m_pPlayer,
		damage,
		dir,
		tr.iHitgroup,
		damageType);
	
	hit->ApplyMultiDamage(m_pPlayer, m_pPlayer);
	
	if (hit->IsClient())
	{
		auto player = dynamic_cast<CBasePlayer*>(hit);

		if (g_pGameRules->FPlayerCanTakeDamage(player, m_pPlayer))
		{
			MessageBegin(MSG_PVS, gmsgBlood, tr.vecEndPos);
			WriteFloat(dir.x);
			WriteFloat(dir.y);
			WriteFloat(dir.z);
			WriteByte(0);
			WriteByte(player->m_LastHitGroup == HITGROUP_HEAD ? 1 : 0);
			WriteCoord(tr.vecEndPos.x);
			WriteCoord(tr.vecEndPos.y);
			WriteCoord(tr.vecEndPos.z);
			MessageEnd();
		}
	}
#endif
}


void CSniperRifle::WeaponPostFrame()
{
	const auto info = GetInfo();

	if ((m_pPlayer->pev->button & IN_ATTACK2) != 0 && m_iScopeTime <= 300)
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

		m_iScopeTime = 500;
	}

	if ((m_pPlayer->pev->button & IN_ATTACK) != 0)
	{
		if ((m_pPlayer->m_TFState & kTFStateAiming) == 0 && m_iNextPrimaryAttack <= 0 && m_pPlayer->m_rgAmmo[info.iAmmo1] >= info.iShots)
		{
			SendWeaponAnim(info.iAnims[kWeaponAnimReload]);
			m_pPlayer->m_TFState |= kTFStateAiming;
			m_iNextPrimaryAttack = 333 + info.iReloadTime;
			CreateLaserEffect();
		}
	}
	else if ((m_pPlayer->m_TFState & kTFStateAiming) != 0)
	{
		PrimaryAttack();
		m_pPlayer->m_TFState &= ~kTFStateAiming;
		m_iNextPrimaryAttack = info.iAttackTime;
		DestroyLaserEffect();
	}

#ifdef GAME_DLL
	UpdateLaserEffect();
#endif
}


void CSniperRifle::Holster()
{
	CTFWeapon::Holster();
	DestroyLaserEffect();
}


void CSniperRifle::GetWeaponData(weapon_data_t& data)
{
	CTFWeapon::GetWeaponData(data);

    *reinterpret_cast<int*>(&data.m_flNextSecondaryAttack) = m_iScopeTime;
}


void CSniperRifle::SetWeaponData(const weapon_data_t& data)
{
	CTFWeapon::SetWeaponData(data);

    m_iScopeTime = *reinterpret_cast<const int*>(&data.m_flNextSecondaryAttack);
}


void CSniperRifle::DecrementTimers(const int msec)
{
	CTFWeapon::DecrementTimers(msec);

	m_iScopeTime = std::max(m_iScopeTime - msec, -1100);
}


void CSniperRifle::CreateLaserEffect()
{
#ifdef GAME_DLL
	DestroyLaserEffect(false);

	m_pLaserDot = CSprite::SpriteCreate("sprites/laserdot.spr", pev->origin, false);
	m_pLaserDot->pev->scale = 1.0;
	m_pLaserDot->pev->spawnflags |= SF_SPRITE_TEMPORARY;
	m_pLaserDot->pev->flags |= FL_SKIPLOCALHOST;
	m_pLaserDot->pev->owner = m_pPlayer->edict();

	m_pLaserBeam = CBeam::BeamCreate("sprites/laserbeam.spr", 16);
	m_pLaserBeam->PointEntInit(pev->origin, m_pPlayer->entindex());
	m_pLaserBeam->SetFlags(BEAM_FSHADEOUT);
	m_pLaserBeam->SetEndAttachment(1);
	m_pLaserBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
	m_pLaserBeam->pev->flags |= FL_SKIPLOCALHOST;
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

	MessageBegin(MSG_ONE, gmsgLaserDot, m_pPlayer);
	WriteByte(1);
	MessageEnd();
#else
	m_pPlayer->PlaybackEvent(m_usLaserDotOn);
#endif
}


#ifdef GAME_DLL

void CSniperRifle::UpdateLaserEffect()
{
	if (m_pLaserDot != nullptr || m_pLaserBeam != nullptr)
	{
		const auto info = GetInfo();
		const auto gun = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs;
		const auto aim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		Vector dir;
		AngleVectors(aim, &dir, nullptr, nullptr);

		TraceResult tr;
		util::TraceLine(gun, gun + dir * info.iProjectileRange, &tr, m_pPlayer, util::kTraceBox);

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

#endif


void CSniperRifle::DestroyLaserEffect(const bool sendMessage)
{
#ifdef GAME_DLL
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

	if (sendMessage)
	{
		MessageBegin(MSG_ONE, gmsgLaserDot, m_pPlayer);
		WriteByte(0);
		MessageEnd();
	}
#else
	m_pPlayer->PlaybackEvent(m_usLaserDotOff);
#endif
}


LINK_ENTITY_TO_CLASS(tf_weapon_autorifle, CAutoRifle);

void CAutoRifle::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_autorifle";
	i.iAmmo1 = AMMO_SHELLS;
	i.iAmmo2 = -1;
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
	i.iProjectileRange = 2048;

	i.pszEvent = "events/wpn/tf_ar.sc";
	i.pszAttackSound = "weapons/sniper.wav";
	i.pszAlternateSound = nullptr;
	i.pszReloadSound = nullptr;
	i.flPunchAngle = -2.0F;
	i.iSibling = WEAPON_SNIPER_RIFLE;
	i.bShouldIdle = true;
}

