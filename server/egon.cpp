/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "effects.h"
#include "customentity.h"
#include "gamerules.h"
#include "UserMessages.h"

#ifdef CLIENT_DLL
#include "hud.h"
#include "com_weapons.h"
#endif

#define EGON_SWITCH_NARROW_TIME 0.75 // Time it takes to switch fire modes
#define EGON_SWITCH_WIDE_TIME 1.5

LINK_ENTITY_TO_CLASS(weapon_egon, CEgon);

bool CEgon::Spawn()
{
	Precache();
	m_iId = WEAPON_EGON;
	SetModel("models/w_egon.mdl");

	m_iDefaultAmmo = EGON_DEFAULT_GIVE;

	FallInit(); // get ready to fall down.

	return true;
}


void CEgon::Precache()
{
	PRECACHE_MODEL("models/w_egon.mdl");
	PRECACHE_MODEL("models/v_egon.mdl");
	PRECACHE_MODEL("models/p_egon.mdl");

	PRECACHE_SOUND(EGON_SOUND_OFF);
	PRECACHE_SOUND(EGON_SOUND_RUN);
	PRECACHE_SOUND(EGON_SOUND_STARTUP);

	PRECACHE_MODEL(EGON_BEAM_SPRITE);
	PRECACHE_MODEL(EGON_FLARE_SPRITE);

	m_usEgonFire = PRECACHE_EVENT(1, "events/egon_fire.sc");
	m_usEgonStop = PRECACHE_EVENT(1, "events/egon_stop.sc");
}


bool CEgon::Deploy()
{
	m_deployed = false;
	m_fireState = FIRE_OFF;
	return DefaultDeploy("models/v_egon.mdl", "models/p_egon.mdl", EGON_DRAW, "egon");
}

bool CEgon::CanHolster()
{
	if (m_fireState != FIRE_OFF)
		return false;

	return CBasePlayerWeapon::CanHolster();
}

bool CEgon::Holster()
{
	if (DefaultHolster(EGON_HOLSTER))
	{
		EndAttack(false);
		return true;
	}
	return false;
}

bool CEgon::GetWeaponInfo(WeaponInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->iAmmo1 = AMMO_URANIUM;
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
	p->iAmmo2 = AMMO_NONE;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_EGON;
	p->iFlags = 0;
	p->iWeight = EGON_WEIGHT;

	return true;
}

#define EGON_PULSE_INTERVAL 0.1
#define EGON_DISCHARGE_INTERVAL 0.1

float CEgon::GetPulseInterval()
{
	return EGON_PULSE_INTERVAL;
}

float CEgon::GetDischargeInterval()
{
	return EGON_DISCHARGE_INTERVAL;
}

bool CEgon::HasAmmo()
{
	return m_pPlayer->m_rgAmmo[iAmmo1()] > 0;
}

void CEgon::UseAmmo(int count)
{
	if (m_pPlayer->m_rgAmmo[iAmmo1()] >= count)
		m_pPlayer->m_rgAmmo[iAmmo1()] -= count;
	else
		m_pPlayer->m_rgAmmo[iAmmo1()] = 0;
}

void CEgon::Attack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{

		if (m_fireState != FIRE_OFF || m_pBeam)
		{
			EndAttack();
		}
		else
		{
			PlayEmptySound();
		}
		return;
	}

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition();

	switch (m_fireState)
	{
	case FIRE_OFF:
	{
		if (!HasAmmo())
		{
			m_iNextPrimaryAttack = m_iNextSecondaryAttack = 250;
			PlayEmptySound();
			return;
		}

		m_flAmmoUseTime = gpGlobals->time; // start using ammo ASAP.

		m_pPlayer->PlaybackEvent(m_usEgonFire, 0.0, 0.0, 0, m_fireMode, true, false);

		m_shakeTime = 0;

		m_iTimeWeaponIdle = 100;
		pev->fuser1 = UTIL_WeaponTimeBase() + 2;

		pev->dmgtime = gpGlobals->time + GetPulseInterval();
		m_fireState = FIRE_CHARGE;
	}
	break;

	case FIRE_CHARGE:
	{
		Fire(vecSrc, vecAiming);

		if (pev->fuser1 <= UTIL_WeaponTimeBase())
		{
			m_pPlayer->PlaybackEvent(m_usEgonFire, 0.0, 0.0, 0, m_fireMode, false, false);
			pev->fuser1 = 1000;
		}

		if (!HasAmmo())
		{
			EndAttack();
			m_iNextPrimaryAttack = m_iNextSecondaryAttack = 1000;
		}
	}
	break;
	}
}

void CEgon::PrimaryAttack()
{
	m_fireMode = FIRE_WIDE;
	Attack();
}

void CEgon::Fire(const Vector& vecOrigSrc, const Vector& vecDir)
{
	Vector vecDest = vecOrigSrc + vecDir * 2048;
	TraceResult tr;

	Vector tmpSrc = vecOrigSrc + gpGlobals->v_up * -8 + gpGlobals->v_right * 3;

	UTIL_TraceLine(vecOrigSrc, vecDest, dont_ignore_monsters, m_pPlayer, &tr);

	if (0 != tr.fAllSolid)
		return;

#ifndef CLIENT_DLL
	CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

	if (pEntity == NULL)
		return;

	if (UTIL_IsDeathmatch())
	{
		if (m_pSprite && 0 != pEntity->pev->takedamage)
		{
			m_pSprite->pev->effects &= ~EF_NODRAW;
		}
		else if (m_pSprite)
		{
			m_pSprite->pev->effects |= EF_NODRAW;
		}
	}


#endif

	float timedist;

	switch (m_fireMode)
	{
	default:
	case FIRE_NARROW:
#ifndef CLIENT_DLL
		if (pev->dmgtime < gpGlobals->time)
		{
			// Narrow mode only does damage to the entity it hits
			ClearMultiDamage();
			if (0 != pEntity->pev->takedamage)
			{
				pEntity->TraceAttack(m_pPlayer, gSkillData.plrDmgEgonNarrow, vecDir, &tr, DMG_ENERGYBEAM);
			}
			ApplyMultiDamage(m_pPlayer, m_pPlayer);

			if (UTIL_IsDeathmatch())
			{
				// multiplayer uses 1 ammo every 1/10th second
				if (gpGlobals->time >= m_flAmmoUseTime)
				{
					UseAmmo(1);
					m_flAmmoUseTime = gpGlobals->time + 0.1;
				}
			}
			else
			{
				// single player, use 3 ammo/second
				if (gpGlobals->time >= m_flAmmoUseTime)
				{
					UseAmmo(1);
					m_flAmmoUseTime = gpGlobals->time + 0.166;
				}
			}

			pev->dmgtime = gpGlobals->time + GetPulseInterval();
		}
#endif
		timedist = (pev->dmgtime - gpGlobals->time) / GetPulseInterval();
		break;

	case FIRE_WIDE:
#ifndef CLIENT_DLL
		if (pev->dmgtime < gpGlobals->time)
		{
			// wide mode does damage to the ent, and radius damage
			ClearMultiDamage();
			if (0 != pEntity->pev->takedamage)
			{
				pEntity->TraceAttack(m_pPlayer, gSkillData.plrDmgEgonWide, vecDir, &tr, DMG_ENERGYBEAM | DMG_ALWAYSGIB);
			}
			ApplyMultiDamage(m_pPlayer, m_pPlayer);

			if (UTIL_IsDeathmatch())
			{
				// radius damage a little more potent in multiplayer.
				RadiusDamage(tr.vecEndPos, this, m_pPlayer, gSkillData.plrDmgEgonWide / 4, 128, CLASS_NONE, DMG_ENERGYBEAM | DMG_BLAST | DMG_ALWAYSGIB);
			}

			if (!m_pPlayer->IsAlive())
				return;

			if (UTIL_IsDeathmatch())
			{
				//multiplayer uses 5 ammo/second
				if (gpGlobals->time >= m_flAmmoUseTime)
				{
					UseAmmo(1);
					m_flAmmoUseTime = gpGlobals->time + 0.2;
				}
			}
			else
			{
				// Wide mode uses 10 charges per second in single player
				if (gpGlobals->time >= m_flAmmoUseTime)
				{
					UseAmmo(1);
					m_flAmmoUseTime = gpGlobals->time + 0.1;
				}
			}

			pev->dmgtime = gpGlobals->time + GetDischargeInterval();
			if (m_shakeTime < gpGlobals->time)
			{
				UTIL_ScreenShake(tr.vecEndPos, 5.0, 150.0, 0.75, 250.0);
				m_shakeTime = gpGlobals->time + 1.5;
			}
		}
#endif
		timedist = (pev->dmgtime - gpGlobals->time) / GetDischargeInterval();
		break;
	}

	if (timedist < 0)
		timedist = 0;
	else if (timedist > 1)
		timedist = 1;
	timedist = 1 - timedist;

	UpdateEffect(tmpSrc, tr.vecEndPos, timedist);
}


void CEgon::UpdateEffect(const Vector& startPoint, const Vector& endPoint, float timeBlend)
{
#ifndef CLIENT_DLL
	if (!m_pBeam)
	{
		CreateEffect();
	}

	m_pBeam->SetStartPos(endPoint);
	m_pBeam->SetBrightness(255 - (timeBlend * 180));
	m_pBeam->SetWidth(40 - (timeBlend * 20));

	if (m_fireMode == FIRE_WIDE)
		m_pBeam->SetColor(30 + (25 * timeBlend), 30 + (30 * timeBlend), 64 + 80 * fabs(sin(gpGlobals->time * 10)));
	else
		m_pBeam->SetColor(60 + (25 * timeBlend), 120 + (30 * timeBlend), 64 + 80 * fabs(sin(gpGlobals->time * 10)));


	m_pSprite->SetOrigin(endPoint);
	m_pSprite->pev->frame += 8 * gpGlobals->frametime;
	if (m_pSprite->pev->frame > m_pSprite->Frames())
		m_pSprite->pev->frame = 0;

	m_pNoise->SetStartPos(endPoint);

#endif
}

void CEgon::CreateEffect()
{

#ifndef CLIENT_DLL
	DestroyEffect();

	m_pBeam = CBeam::BeamCreate(EGON_BEAM_SPRITE, 40);
	m_pBeam->PointEntInit(pev->origin, m_pPlayer->entindex());
	m_pBeam->SetFlags(BEAM_FSINE);
	m_pBeam->SetEndAttachment(1);
	m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY; // Flag these to be destroyed on save/restore or level transition
	m_pBeam->pev->flags |= FL_SKIPLOCALHOST;
	m_pBeam->pev->owner = m_pPlayer->edict();

	m_pNoise = CBeam::BeamCreate(EGON_BEAM_SPRITE, 55);
	m_pNoise->PointEntInit(pev->origin, m_pPlayer->entindex());
	m_pNoise->SetScrollRate(25);
	m_pNoise->SetBrightness(100);
	m_pNoise->SetEndAttachment(1);
	m_pNoise->pev->spawnflags |= SF_BEAM_TEMPORARY;
	m_pNoise->pev->flags |= FL_SKIPLOCALHOST;
	m_pNoise->pev->owner = m_pPlayer->edict();

	m_pSprite = CSprite::SpriteCreate(EGON_FLARE_SPRITE, pev->origin, false);
	m_pSprite->pev->scale = 1.0;
	m_pSprite->SetTransparency(kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation);
	m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;
	m_pSprite->pev->flags |= FL_SKIPLOCALHOST;
	m_pSprite->pev->owner = m_pPlayer->edict();

	if (m_fireMode == FIRE_WIDE)
	{
		m_pBeam->SetScrollRate(50);
		m_pBeam->SetNoise(20);
		m_pNoise->SetColor(50, 50, 255);
		m_pNoise->SetNoise(8);
	}
	else
	{
		m_pBeam->SetScrollRate(110);
		m_pBeam->SetNoise(5);
		m_pNoise->SetColor(80, 120, 255);
		m_pNoise->SetNoise(2);
	}
#endif
}


void CEgon::DestroyEffect()
{

#ifndef CLIENT_DLL
	if (m_pBeam)
	{
		m_pBeam->Remove();
		m_pBeam = NULL;
	}
	if (m_pNoise)
	{
		m_pNoise->Remove();
		m_pNoise = NULL;
	}
	if (m_pSprite)
	{
		if (m_fireMode == FIRE_WIDE)
			m_pSprite->Expand(10, 500);
		else
			m_pSprite->Remove();
		m_pSprite = NULL;
	}
#endif
}



void CEgon::WeaponIdle()
{
	if ((m_pPlayer->m_afButtonPressed & IN_ATTACK2) == 0 && (m_pPlayer->pev->button & IN_ATTACK) != 0)
	{
		return;
	}

	if (m_iTimeWeaponIdle > 0)
		return;

	if (m_fireState != FIRE_OFF)
		EndAttack();

	int iAnim;

	float flRand = RANDOM_FLOAT(0, 1);

	if (flRand <= 0.5)
	{
		iAnim = EGON_IDLE1;
		m_iTimeWeaponIdle = UTIL_SharedRandomLong(m_pPlayer->random_seed, 10000, 15000);
	}
	else
	{
		iAnim = EGON_FIDGET1;
		m_iTimeWeaponIdle = 3000;
	}

	SendWeaponAnim(iAnim);
	m_deployed = true;
}



void CEgon::EndAttack(bool bSendAnim)
{
	bool bMakeNoise = false;

	if (m_fireState != FIRE_OFF) //Checking the button just in case!.
		bMakeNoise = true;

	m_pPlayer->PlaybackEvent(m_usEgonStop, 0.0, 0.0, bMakeNoise, 0, bSendAnim, false, FEV_GLOBAL | FEV_RELIABLE);

	if (m_fireState != FIRE_OFF)
	{
		m_iTimeWeaponIdle = 2000;

		m_iNextPrimaryAttack = m_iNextSecondaryAttack = 500;
	}

	m_fireState = FIRE_OFF;

	DestroyEffect();
}


IMPLEMENT_AMMO_CLASS(ammo_egonclip, CEgonAmmo, "models/w_chainammo.mdl", AMMO_URANIUMBOX_GIVE, AMMO_URANIUM, URANIUM_MAX_CARRY);
