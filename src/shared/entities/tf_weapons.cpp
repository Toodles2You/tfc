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


void CTFWeapon::Precache()
{
	WeaponInfo info;
	memset(&info, 0, sizeof(info));

	GetWeaponInfo(info);

	CBasePlayerWeapon::Precache();

#ifdef GAME_DLL
	if (info.pszAttackSound != nullptr)
	{
		engine::PrecacheSound(info.pszAttackSound);
	}
	if (info.pszAlternateSound != nullptr)
	{
		engine::PrecacheSound(info.pszAlternateSound);
	}
	if (info.pszReloadSound != nullptr)
	{
		engine::PrecacheSound(info.pszReloadSound);
	}
#endif

	if (info.pszEvent != nullptr && info.pszEvent[0] != '\0')
	{
		m_usPrimaryAttack = engine::PrecacheEvent(1, info.pszEvent);
	}
}


CTFWeapon* CTFWeapon::GetSibling()
{
	const auto info = GetInfo();

	if (info.iSibling != -1)
	{
		return dynamic_cast<CTFWeapon*>(m_pPlayer->m_rgpPlayerWeapons[info.iSibling]);
	}

	return nullptr;
}


void CTFWeapon::UpdateSiblingInfo(const bool holster)
{
	CTFWeapon* sibling = GetSibling();

	if (sibling != nullptr)
	{
		CTFWeapon* from,* to;

		if (holster)
		{
			from = this;
			to = sibling;
		}
		else
		{
			from = sibling;
			to = this;
		}

		to->m_iClip = from->m_iClip;
	}
}


void CTFWeapon::Deploy()
{
	CBasePlayerWeapon::Deploy();
	UpdateSiblingInfo(false);
	m_iWeaponState &= ~(kWpnStateEmptySound | kWpnStateIdle);
}


void CTFWeapon::Holster()
{
	if (m_fInReload || m_pPlayer->InState(CBasePlayer::State::Aiming))
	{
		m_iNextPrimaryAttack = 0;
		m_fInReload = false;
		m_pPlayer->LeaveState(CBasePlayer::State::Aiming);
	}

	CBasePlayerWeapon::Holster();
	UpdateSiblingInfo(true);
}


void CTFWeapon::PrimaryAttack()
{
	const auto info = GetInfo();
	int rounds = 0;

	while (m_iNextPrimaryAttack <= 0)
	{
		m_iNextPrimaryAttack += info.iAttackTime;
		rounds++;
	}

	if (info.iMaxClip > 0)
	{
		if (info.iShots * rounds > m_iClip)
		{
			rounds = m_iClip / info.iShots;
		}

		m_iClip -= info.iShots * rounds;
	}
	else if (info.iAmmo1 != -1)
	{
		if (info.iShots * rounds > m_pPlayer->m_rgAmmo[info.iAmmo1])
		{
			rounds = m_pPlayer->m_rgAmmo[info.iAmmo1] / info.iShots;
		}

		m_pPlayer->m_rgAmmo[info.iAmmo1] -= info.iShots * rounds;
	}

	m_pPlayer->Undisguise();

	SendWeaponAnim(info.iAnims[kWeaponAnimAttack]);
	m_pPlayer->SetAction(CBasePlayer::Action::Attack);

	m_pPlayer->PlaybackEvent(m_usPrimaryAttack, (float)GetID(), m_pPlayer->v.view_ofs.z, m_pPlayer->m_randomSeed, rounds);

#ifdef GAME_DLL
	CBaseEntity* projectile = nullptr;

	const auto ping = m_pPlayer->m_netPing;

	switch (info.iProjectileType)
	{
		case kProjRocket:
		{
			const auto aim = m_pPlayer->v.v_angle + m_pPlayer->v.punchangle;
			util::MakeVectors(aim);

			int rightOffset = 8;

			if (m_pPlayer->m_bLeftHanded)
			{
				rightOffset = -8;
			}

			const auto gun =
				m_pPlayer->v.origin
					+ m_pPlayer->v.view_ofs
					+ gpGlobals->v_forward * 16
					+ gpGlobals->v_right * rightOffset
					+ gpGlobals->v_up * -8;

			projectile =
			CRocket::CreateRocket(
				gun,
				gpGlobals->v_forward,
				info.iProjectileDamage,
				info.iProjectileDamageMin,
				info.iProjectileRadius,
				m_pPlayer);
			break;
		}
		case kProjNail:
		case kProjTranquilizerDart:
		case kProjLaser:
		{
			const auto aim = m_pPlayer->v.v_angle + m_pPlayer->v.punchangle;
			util::MakeVectors(aim);

			int rightOffset = 4;

			if (m_pPlayer->m_bLeftHanded)
			{
				rightOffset = -4;
			}

			const auto gun =
				m_pPlayer->v.origin
					+ m_pPlayer->v.view_ofs
					+ gpGlobals->v_right * rightOffset
					+ gpGlobals->v_up * -4;

			if (info.iProjectileType == kProjTranquilizerDart)
			{
				projectile =
				CNail::CreateTranquilizerDart(gun, gpGlobals->v_forward, info.iProjectileDamage, m_pPlayer);
			}
			else if (info.iProjectileType == kProjLaser)
			{
				projectile =
				CNail::CreateLaser(gun, gpGlobals->v_forward, info.iProjectileDamage, m_pPlayer);
			}
			else
			{
				projectile =
				CNail::CreateNail(gun, gpGlobals->v_forward, info.iProjectileDamage, m_pPlayer);
			}
			break;
		}
		case kProjPipeBomb:
		case kProjPipeBombRemote:
		{
			const auto aim = m_pPlayer->v.v_angle + m_pPlayer->v.punchangle;
			util::MakeVectors(aim);

			int rightOffset = 8;

			if (m_pPlayer->m_bLeftHanded)
			{
				rightOffset = -8;
			}

			const auto gun =
				m_pPlayer->v.origin
					+ m_pPlayer->v.view_ofs
					+ gpGlobals->v_right * rightOffset
					+ gpGlobals->v_up * -16;

			CPipeBombLauncher* launcher = nullptr;

			if (info.iProjectileType == kProjPipeBombRemote)
			{
				launcher = dynamic_cast<CPipeBombLauncher*>(this);
			}

			projectile =
			CPipeBomb::CreatePipeBomb(
				gun,
				gpGlobals->v_forward * 0.75F + gpGlobals->v_up * 0.25F,
				info.iProjectileDamage,
				info.iProjectileDamageMin,
				info.iProjectileRadius,
				m_pPlayer,
				launcher);
			break;
		}
		case kProjFlame:
		{
			const auto aim = m_pPlayer->v.v_angle + m_pPlayer->v.punchangle;
			util::MakeVectors(aim);

			int rightOffset = 8;

			if (m_pPlayer->m_bLeftHanded)
			{
				rightOffset = -8;
			}

			const auto gun =
				m_pPlayer->v.origin
					+ m_pPlayer->v.view_ofs
					+ gpGlobals->v_forward * 16
					+ gpGlobals->v_right * rightOffset
					+ gpGlobals->v_up * -8;

			projectile =
			CFlame::CreateFlame(gun, gpGlobals->v_forward, info.iProjectileDamage, m_pPlayer);
			break;
		}
		case kProjIncendiaryRocket:
		{
			const auto aim = m_pPlayer->v.v_angle + m_pPlayer->v.punchangle;
			util::MakeVectors(aim);

			int rightOffset = 8;

			if (m_pPlayer->m_bLeftHanded)
			{
				rightOffset = -8;
			}

			const auto gun =
				m_pPlayer->v.origin
					+ m_pPlayer->v.view_ofs
					+ gpGlobals->v_forward * 16
					+ gpGlobals->v_right * rightOffset
					+ gpGlobals->v_up * -8;

			projectile =
			CRocket::CreateIncendiaryRocket(
				gun,
				gpGlobals->v_forward,
				info.iProjectileDamage,
				info.iProjectileDamageMin,
				info.iProjectileRadius,
				m_pPlayer);
			break;
		}
		default:
		{
			m_pPlayer->FireBullets(
				info.iProjectileDamage,
				info.iProjectileDamageMin,
				info.vecProjectileSpread,
				info.iProjectileCount * rounds,
				info.iProjectileRange);
			break;
		}
	}

	if (projectile != nullptr)
	{
		util::LagCompensation(projectile, ping);
	}
#endif

	m_iWeaponState &= ~kWpnStateIdle;
}


void CTFWeapon::WeaponPostFrame()
{
	const auto info = GetInfo();

	if (m_fInReload)
	{
		if (m_iNextPrimaryAttack <= 0)
		{
			if ((m_iWeaponState & kWpnStateReloading) == 0)
			{
				m_iWeaponState |= kWpnStateReloading;
				m_iWeaponState &= ~kWpnStateIdle;
			}
			else
			{
				int iShots = info.iShots;

				if (info.iAmmo1 != -1)
				{
					iShots = std::min(iShots, static_cast<int>(m_pPlayer->m_rgAmmo[info.iAmmo1]));

					m_pPlayer->m_rgAmmo[info.iAmmo1] -= iShots;
				}

				m_iClip = std::min(m_iClip + iShots, info.iMaxClip);
			}
		}
		if ((m_pPlayer->v.button & IN_ATTACK) != 0 && m_iClip >= info.iShots)
		{
			m_fInReload = false;
			m_iNextPrimaryAttack = 0;
			PrimaryAttack();
		}
		else if (m_iNextPrimaryAttack <= 0)
		{
			if (m_iClip < info.iMaxClip
			 && (info.iAmmo1 == -1 || m_pPlayer->m_rgAmmo[info.iAmmo1] >= info.iShots))
			{
				m_pPlayer->SetAction(CBasePlayer::Action::Reload);

				SendWeaponAnim(info.iAnims[kWeaponAnimReload]);
#ifdef CLIENT_DLL
				if (info.pszReloadSound != nullptr)
				{
					m_pPlayer->EmitSoundPredicted(info.pszReloadSound, CHAN_ITEM);
				}
#endif

				m_iNextPrimaryAttack = info.iReloadTime / (info.iMaxClip / info.iShots);
			}
			else
			{
				SendWeaponAnim(info.iAnims[kWeaponAnimEndReload]);

				m_fInReload = false;
			}
		}
	}
	else if (m_iNextPrimaryAttack <= 0)
	{
		if ((m_pPlayer->v.button & IN_ATTACK) != 0)
		{
			if (info.iMaxClip > 0)
			{
				if (m_iClip >= info.iShots)
				{
					PrimaryAttack();
					return;
				}
			}
			else if (info.iAmmo1 != -1)
			{
				if (m_pPlayer->m_rgAmmo[info.iAmmo1] >= info.iShots)
				{
					PrimaryAttack();
					return;
				}
			}
			else
			{
				PrimaryAttack();
				return;
			}
			if ((m_iWeaponState & kWpnStateEmptySound) == 0)
			{
				m_pPlayer->EmitSoundPredicted("weapons/357_cock1.wav", CHAN_ITEM, VOL_NORM, ATTN_IDLE);
				m_iWeaponState |= kWpnStateEmptySound;
			}
		}

		if (m_iClip < info.iMaxClip
		 && (info.iAmmo1 == -1 || m_pPlayer->m_rgAmmo[info.iAmmo1] >= info.iShots))
		{
			SendWeaponAnim(info.iAnims[kWeaponAnimStartReload]);

			m_iWeaponState &= ~(kWpnStateReloading | kWpnStateEmptySound);
			m_fInReload = true;
			/*
				Toodles: Slow the initial reload interval so that firing
				the last shot effectively halves the weapon's fire-rate.
			*/
			m_iNextPrimaryAttack = std::max(info.iAttackTime - info.iReloadTime / (info.iMaxClip / info.iShots), 100);
		}
		else
		{
			if (info.bShouldIdle && (m_iWeaponState & kWpnStateIdle) == 0)
			{
				SendWeaponAnim(info.iAnims[kWeaponAnimIdle]);
				m_iWeaponState |= kWpnStateIdle;
			}

			m_iNextPrimaryAttack = std::max(m_iNextPrimaryAttack, 0);
		}
	}
}


LINK_ENTITY_TO_CLASS(tf_weapon_shotgun, CShotgun);

void CShotgun::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_shotgun";
	i.iAmmo1 = AMMO_SHELLS;
	i.iAmmo2 = -1;
	i.iMaxClip = 8;
	i.iSlot = 1;
	i.iPosition = 0;
	i.iFlags = 0;
	i.iWeight = 15;

	i.pszWorld = "models/w_shotgun.mdl";
	i.pszView = "models/v_tfc_12gauge.mdl";
	i.pszPlayer = "models/p_smallshotgun.mdl";
	i.pszAnimExt = "shotgun";

	i.iAnims[kWeaponAnimIdle] = 0;
	i.iAnims[kWeaponAnimDeploy] = 6;
	i.iAnims[kWeaponAnimHolster] = 7;
	i.iAnims[kWeaponAnimAttack] = 1;
	i.iAnims[kWeaponAnimReload] = 3;
	i.iAnims[kWeaponAnimStartReload] = 5;
	i.iAnims[kWeaponAnimEndReload] = 4;

	i.iShots = 1;

	i.iAttackTime = 500;
	i.iReloadTime = 2000;

	i.iProjectileType = kProjBullet;
	/* Toodles TODO: This probably isn't a good method for faking extra shotgun pellets. */
#ifdef GAME_DLL
	i.iProjectileDamage = 24;
	i.vecProjectileSpread = Vector2D(0.0F, 0.0F);
	i.iProjectileCount = 1;
	i.iProjectileDamageMin = 12;
#else
	i.iProjectileDamage = 4;
	i.vecProjectileSpread = Vector2D(2.3F, 2.3F);
	i.iProjectileCount = 6;
	i.iProjectileDamageMin = 4;
#endif
	i.iProjectileRange = 2432;

	i.pszEvent = "events/wpn/tf_sg.sc";
	i.pszAttackSound = "weapons/sbarrel1.wav";
	i.pszAlternateSound = nullptr;
	i.pszReloadSound = "weapons/reload1.wav";
	i.flPunchAngle = -2.0F;
	i.iSibling = -1;
	i.bShouldIdle = false;
}


LINK_ENTITY_TO_CLASS(tf_weapon_supershotgun, CSuperShotgun);

void CSuperShotgun::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_supershotgun";
	i.iAmmo1 = AMMO_SHELLS;
	i.iAmmo2 = -1;
	i.iMaxClip = 16;
	i.iSlot = 2;
	i.iPosition = 1;
	i.iFlags = 0;
	i.iWeight = 15;

	i.pszWorld = "models/w_shotgun.mdl";
	i.pszView = "models/v_tfc_shotgun.mdl";
	i.pszPlayer = "models/p_shotgun.mdl";
	i.pszAnimExt = "shotgun";

	i.iAnims[kWeaponAnimIdle] = 0;
	i.iAnims[kWeaponAnimDeploy] = 6;
	i.iAnims[kWeaponAnimHolster] = 7;
	i.iAnims[kWeaponAnimAttack] = 1;
	i.iAnims[kWeaponAnimReload] = 3;
	i.iAnims[kWeaponAnimStartReload] = 5;
	i.iAnims[kWeaponAnimEndReload] = 4;

	i.iShots = 2;

	i.iAttackTime = 700;
	i.iReloadTime = 3000;

	i.iProjectileType = kProjBullet;
	i.iProjectileDamage = 4;
	i.vecProjectileSpread = Vector2D(8.0F, 4.6F);
	i.iProjectileCount = 14;
	i.iProjectileDamageMin = 4;
	i.iProjectileRange = 1536;

	i.pszEvent = "events/wpn/tf_ssg.sc";
	i.pszAttackSound = "weapons/shotgn2.wav";
	i.pszAlternateSound = nullptr;
	i.pszReloadSound = "weapons/reload1.wav";
	i.flPunchAngle = -4.0F;
	i.iSibling = -1;
	i.bShouldIdle = false;
}


LINK_ENTITY_TO_CLASS(tf_weapon_ng, CNailgun);

void CNailgun::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_ng";
	i.iAmmo1 = AMMO_NAILS;
	i.iAmmo2 = -1;
	i.iMaxClip = -1;
	i.iSlot = 3;
	i.iPosition = 0;
	i.iFlags = 0;
	i.iWeight = 15;

	i.pszWorld = "models/w_9mmAR.mdl";
	i.pszView = "models/v_tfc_nailgun.mdl";
	i.pszPlayer = "models/p_nailgun.mdl";
	i.pszAnimExt = "mp5";

	i.iAnims[kWeaponAnimIdle] = 0;
	i.iAnims[kWeaponAnimDeploy] = 4;
	i.iAnims[kWeaponAnimHolster] = 8;
	i.iAnims[kWeaponAnimAttack] = 5;
	i.iAnims[kWeaponAnimReload] = 3;
	i.iAnims[kWeaponAnimStartReload] = 3;
	i.iAnims[kWeaponAnimEndReload] = 3;

	i.iShots = 1;

	i.iAttackTime = 100;
	i.iReloadTime = 1500;

	i.iProjectileType = kProjNail;
	i.iProjectileDamage = 9;
	i.vecProjectileSpread = Vector2D(0.0F, 0.0F);
	i.iProjectileCount = 1;

	i.pszEvent = "events/wpn/tf_nail.sc";
	i.pszAttackSound = "weapons/airgun_1.wav";
	i.pszAlternateSound = nullptr;
	i.pszReloadSound = nullptr;
	i.flPunchAngle = 0.0F;
	i.iSibling = -1;
	i.bShouldIdle = false;
}


LINK_ENTITY_TO_CLASS(tf_weapon_superng, CSuperNailgun);

void CSuperNailgun::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_superng";
	i.iAmmo1 = AMMO_NAILS;
	i.iAmmo2 = -1;
	i.iMaxClip = -1;
	i.iSlot = 3;
	i.iPosition = 2;
	i.iFlags = 0;
	i.iWeight = 15;

	i.pszWorld = "models/w_9mmAR.mdl";
	i.pszView = "models/v_tfc_supernailgun.mdl";
	i.pszPlayer = "models/p_snailgun.mdl";
	i.pszAnimExt = "mp5";

	i.iAnims[kWeaponAnimIdle] = 0;
	i.iAnims[kWeaponAnimDeploy] = 4;
	i.iAnims[kWeaponAnimHolster] = 8;
	i.iAnims[kWeaponAnimAttack] = 5;
	i.iAnims[kWeaponAnimReload] = 3;
	i.iAnims[kWeaponAnimStartReload] = 3;
	i.iAnims[kWeaponAnimEndReload] = 3;

	i.iShots = 2;

	i.iAttackTime = 100;
	i.iReloadTime = 1500;

	i.iProjectileType = kProjNail;
	i.iProjectileDamage = 13;
	i.vecProjectileSpread = Vector2D(0.0F, 0.0F);
	i.iProjectileCount = 1;

	i.pszEvent = "events/wpn/tf_snail.sc";
	i.pszAttackSound = "weapons/spike2.wav";
	i.pszAlternateSound = nullptr;
	i.pszReloadSound = nullptr;
	i.flPunchAngle = 0.0F;
	i.iSibling = -1;
	i.bShouldIdle = false;
}


LINK_ENTITY_TO_CLASS(tf_weapon_flamethrower, CFlamethrower);

void CFlamethrower::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_flamethrower";
	i.iAmmo1 = AMMO_CELLS;
	i.iAmmo2 = -1;
	i.iMaxClip = -1;
	i.iSlot = 3;
	i.iPosition = 3;
	i.iFlags = 0;
	i.iWeight = 15;

	i.pszWorld = "models/w_egon.mdl";
	i.pszView = "models/v_flame.mdl";
	i.pszPlayer = "models/p_egon.mdl";
	i.pszAnimExt = "egon";

	i.iAnims[kWeaponAnimIdle] = 0;
	i.iAnims[kWeaponAnimDeploy] = 9;
	i.iAnims[kWeaponAnimHolster] = 10;
	i.iAnims[kWeaponAnimAttack] = 5;
	i.iAnims[kWeaponAnimReload] = 6;
	i.iAnims[kWeaponAnimStartReload] = 7;
	i.iAnims[kWeaponAnimEndReload] = 8;

	i.iShots = 1;

	i.iAttackTime = 150;
	i.iReloadTime = 1500;

	i.iProjectileType = kProjFlame;
	/*
		Toodles: Originally the flamethrower dealt 15 damage upon touch
		& then 6 from ignition. This results in the same 20 damage.
	*/
	i.iProjectileDamage = 17;
	i.vecProjectileSpread = Vector2D(0.0F, 0.0F);
	i.iProjectileCount = 1;

	i.pszEvent = "events/wpn/tf_flame.sc";
	i.pszAttackSound = "weapons/flmfire2.wav";
	i.pszAlternateSound = nullptr;
	i.pszReloadSound = nullptr;
	i.flPunchAngle = 0.0F;
	i.iSibling = -1;
	i.bShouldIdle = true;
}


LINK_ENTITY_TO_CLASS(tf_weapon_rpg, CRocketLauncher);

void CRocketLauncher::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_rpg";
	i.iAmmo1 = AMMO_ROCKETS;
	i.iAmmo2 = -1;
	i.iMaxClip = 4;
	i.iSlot = 4;
	i.iPosition = 0;
	i.iFlags = 0;
	i.iWeight = 15;

	i.pszWorld = "models/w_rpg.mdl";
	i.pszView = "models/v_tfc_rpg.mdl";
	i.pszPlayer = "models/p_srpg.mdl";
	i.pszAnimExt = "rpg";

	i.iAnims[kWeaponAnimIdle] = 0;
	i.iAnims[kWeaponAnimDeploy] = 4;
	i.iAnims[kWeaponAnimHolster] = 3;
	i.iAnims[kWeaponAnimAttack] = 2;
	i.iAnims[kWeaponAnimReload] = 8;
	i.iAnims[kWeaponAnimStartReload] = 7;
	i.iAnims[kWeaponAnimEndReload] = 9;

	i.iShots = 1;

	i.iAttackTime = 800;
	i.iReloadTime = 5000;

	i.iProjectileType = kProjRocket;
	i.iProjectileDamage = 100;
	i.iProjectileDamageMin = 50;
	i.iProjectileRadius = 100;
	i.vecProjectileSpread = Vector2D(0.0F, 0.0F);
	i.iProjectileCount = 1;

	i.pszEvent = "events/wpn/tf_rpg.sc";
	i.pszAttackSound = "weapons/rocketfire1.wav";
	i.pszAlternateSound = nullptr;
	i.pszReloadSound = nullptr;
	i.flPunchAngle = -4.0F;
	i.iSibling = -1;
	i.bShouldIdle = false;
}


LINK_ENTITY_TO_CLASS(tf_weapon_ic, CIncendiaryCannon);

void CIncendiaryCannon::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_ic";
	i.iAmmo1 = AMMO_ROCKETS;
	i.iAmmo2 = -1;
	i.iMaxClip = -1;
	i.iSlot = 4;
	i.iPosition = 3;
	i.iFlags = 0;
	i.iWeight = 15;

	i.pszWorld = "models/w_rpg.mdl";
	i.pszView = "models/v_rpg.mdl";
	i.pszPlayer = "models/p_rpg.mdl";
	i.pszAnimExt = "rpg";

	i.iAnims[kWeaponAnimIdle] = 0;
	i.iAnims[kWeaponAnimDeploy] = 5;
	i.iAnims[kWeaponAnimHolster] = 3;
	i.iAnims[kWeaponAnimAttack] = 3;
	i.iAnims[kWeaponAnimReload] = 2;
	i.iAnims[kWeaponAnimStartReload] = -1;
	i.iAnims[kWeaponAnimEndReload] = -1;

	i.iShots = 1;

	i.iAttackTime = 1200;
	i.iReloadTime = 5000;

	i.iProjectileType = kProjIncendiaryRocket;
	i.iProjectileDamage = 55;
	i.iProjectileDamageMin = 16;
	i.iProjectileRadius = 180;
	i.vecProjectileSpread = Vector2D(0.0F, 0.0F);
	i.iProjectileCount = 1;

	i.pszEvent = "events/wpn/tf_ic.sc";
	i.pszAttackSound = "weapons/sgun1.wav";
	i.pszAlternateSound = nullptr;
	i.pszReloadSound = nullptr;
	i.flPunchAngle = -3.0F;
	i.iSibling = -1;
	i.bShouldIdle = false;
}


LINK_ENTITY_TO_CLASS(tf_weapon_tranq, CTranquilizerGun);

void CTranquilizerGun::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_tranq";
	i.iAmmo1 = AMMO_SHELLS;
	i.iAmmo2 = -1;
	i.iMaxClip = -1;
	i.iSlot = 1;
	i.iPosition = 2;
	i.iFlags = 0;
	i.iWeight = 15;

	i.pszWorld = "models/w_9mmhandgun.mdl";
	i.pszView = "models/v_tfc_pistol.mdl";
	i.pszPlayer = "models/p_spygun.mdl";
	i.pszAnimExt = "onehanded";

	i.iAnims[kWeaponAnimIdle] = 0;
	i.iAnims[kWeaponAnimDeploy] = 7;
	i.iAnims[kWeaponAnimHolster] = 8;
	i.iAnims[kWeaponAnimAttack] = 3;
	i.iAnims[kWeaponAnimReload] = -1;
	i.iAnims[kWeaponAnimStartReload] = -1;
	i.iAnims[kWeaponAnimEndReload] = -1;

	i.iShots = 1;

	i.iAttackTime = 1500;
	i.iReloadTime = 1500;

	i.iProjectileType = kProjTranquilizerDart;
	i.iProjectileDamage = 20;
	i.vecProjectileSpread = Vector2D(0.0F, 0.0F);
	i.iProjectileCount = 1;

	i.pszEvent = "events/wpn/tf_tranq.sc";
	i.pszAttackSound = "weapons/dartgun.wav";
	i.pszAlternateSound = nullptr;
	i.pszReloadSound = nullptr;
	i.flPunchAngle = -2.0F;
	i.iSibling = -1;
	i.bShouldIdle = true;
}


LINK_ENTITY_TO_CLASS(tf_weapon_railgun, CRailgun);

void CRailgun::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_railgun";
	i.iAmmo1 = AMMO_NAILS;
	i.iAmmo2 = -1;
	i.iMaxClip = -1;
	i.iSlot = 1;
	i.iPosition = 3;
	i.iFlags = 0;
	i.iWeight = 15;

	i.pszWorld = "models/w_9mmhandgun.mdl";
	i.pszView = "models/v_tfc_railgun.mdl";
	i.pszPlayer = "models/p_9mmhandgun.mdl";
	i.pszAnimExt = "onehanded";

	i.iAnims[kWeaponAnimIdle] = 0;
	i.iAnims[kWeaponAnimDeploy] = 2;
	i.iAnims[kWeaponAnimHolster] = 3;
	i.iAnims[kWeaponAnimAttack] = 1;
	i.iAnims[kWeaponAnimReload] = -1;
	i.iAnims[kWeaponAnimStartReload] = -1;
	i.iAnims[kWeaponAnimEndReload] = -1;

	i.iShots = 1;

	i.iAttackTime = 400;
	i.iReloadTime = 1500;

	i.iProjectileType = kProjLaser;
	i.iProjectileDamage = 25;
	i.vecProjectileSpread = Vector2D(0.0F, 0.0F);
	i.iProjectileCount = 1;

	i.pszEvent = "events/wpn/tf_rail.sc";
	i.pszAttackSound = "weapons/railgun.wav";
	i.pszAlternateSound = nullptr;
	i.pszReloadSound = nullptr;
	i.flPunchAngle = -2.0F;
	i.iSibling = -1;
	i.bShouldIdle = true;
}

