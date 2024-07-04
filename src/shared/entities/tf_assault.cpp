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


LINK_ENTITY_TO_CLASS(tf_weapon_ac, CAssaultCannon);

void CAssaultCannon::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_ac";
	i.iAmmo1 = AMMO_SHELLS;
	i.iAmmo2 = -1;
	i.iMaxClip = -1;
	i.iSlot = 4;
	i.iPosition = 2;
	i.iFlags = 0;
	i.iWeight = 15;

	i.pszWorld = "models/w_shotgun.mdl";
	i.pszView = "models/v_tfac.mdl";
	i.pszPlayer = "models/p_mini.mdl";
	i.pszAnimExt = "AC";

	i.iAnims[kWeaponAnimIdle] = 0;
	i.iAnims[kWeaponAnimDeploy] = 5;
	i.iAnims[kWeaponAnimHolster] = 6;
	i.iAnims[kWeaponAnimAttack] = 4;
	i.iAnims[kWeaponAnimReload] = 2;
	i.iAnims[kWeaponAnimStartReload] = -1;
	i.iAnims[kWeaponAnimEndReload] = 3;

	i.iShots = 1;

	i.iAttackTime = 100;
	i.iReloadTime = 600;

	i.iProjectileType = kProjBullet;
	i.iProjectileDamage = 7;
	i.vecProjectileSpread = Vector2D(5.7F, 5.7F);
	i.iProjectileCount = 5;
	i.iProjectileDamageMin = 7;
	i.iProjectileRange = 2432;

	i.pszEvent = "events/wpn/tf_acfire.sc";
	i.pszAttackSound = "weapons/asscan2.wav";
	i.pszAlternateSound = "weapons/asscan3.wav";
	i.pszReloadSound = "weapons/asscan1.wav";
	i.flPunchAngle = 0.0F;
	i.iSibling = -1;
	i.bShouldIdle = false;
}


bool CAssaultCannon::CanHolster()
{
	return !m_fInAttack;
}


/* Toodles FIXME: This "state machine" kinda blows. */
void CAssaultCannon::WeaponPostFrame()
{
	const auto info = GetInfo();

	if ((m_pPlayer->v.button & IN_ATTACK) != 0)
	{
		if (m_iNextPrimaryAttack > 0)
		{
			return;
		}

		if (m_pPlayer->m_rgAmmo[info.iAmmo1] >= info.iShots)
		{
			if (!m_pPlayer->InState(CBasePlayer::State::Aiming))
			{
				/* Spin up! */
				SendWeaponAnim(info.iAnims[kWeaponAnimReload]);
				m_pPlayer->EmitSoundPredicted(info.pszReloadSound, CHAN_WEAPON,
					VOL_NORM, ATTN_STATIC, PITCH_HIGH);
				m_pPlayer->EnterState(CBasePlayer::State::Aiming);
				m_iNextPrimaryAttack = info.iReloadTime;
				m_fInAttack = true;
			}
			else
			{
				/* Fire! */
				PrimaryAttack();
				m_iWeaponState |= kWpnStateFiring;
			}

			return;
		}

		if ((m_iWeaponState & kWpnStateEmptySound) == 0)
		{
			m_pPlayer->EmitSoundPredicted("weapons/357_cock1.wav",
				CHAN_ITEM, VOL_NORM, ATTN_IDLE);
			m_iWeaponState |= kWpnStateEmptySound;
		}
	}
	else
	{
		m_iWeaponState &= ~kWpnStateEmptySound;
	}

	/* Player is armed. */
	if (m_pPlayer->InState(CBasePlayer::State::Aiming))
	{
		/* Player is either firing or has finished spinning up. */
		if ((m_iWeaponState & kWpnStateFiring) != 0
		 || m_iNextPrimaryAttack <= 0)
		{
			/* Spin down! */
			SendWeaponAnim(info.iAnims[kWeaponAnimEndReload]);
			m_pPlayer->LeaveState(CBasePlayer::State::Aiming);
			m_pPlayer->EmitSoundPredicted(info.pszAlternateSound,
				CHAN_WEAPON, VOL_NORM, ATTN_STATIC, PITCH_HIGH);
			m_iWeaponState &= ~kWpnStateFiring;
			m_iNextPrimaryAttack = info.iReloadTime;
		}
	}
	else if (m_fInAttack)
	{
		if (m_iNextPrimaryAttack <= 0)
		{
			/* Spun down entirely. We can holster now. */
			m_fInAttack = false;
		}
	}
}


void CAssaultCannon::Holster()
{
	const auto info = GetInfo();

	CTFWeapon::Holster();

	m_pPlayer->LeaveState(CBasePlayer::State::Aiming);
	m_pPlayer->StopSound(info.pszAttackSound, CHAN_WEAPON);
}

