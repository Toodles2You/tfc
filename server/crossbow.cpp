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
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "UserMessages.h"

#ifndef CLIENT_DLL
#define BOLT_AIR_VELOCITY 2000
#define BOLT_WATER_VELOCITY 1000

class CCrossbowBolt : public CBaseEntity
{
	bool Spawn() override;
	void Precache() override;
	int Classify() override;
	void EXPORT BubbleThink();
	void EXPORT BoltTouch(CBaseEntity* pOther);
	void EXPORT ExplodeThink();

	int m_iTrail;

public:
	static CCrossbowBolt* BoltCreate(
		const Vector origin,
		const Vector dir,
		const float speed,
		CBaseEntity *owner);
};
LINK_ENTITY_TO_CLASS(crossbow_bolt, CCrossbowBolt);

CCrossbowBolt* CCrossbowBolt::BoltCreate(
	const Vector origin,
	const Vector dir,
	const float speed,
	CBaseEntity *owner)
{
	// Create a new entity with CCrossbowBolt private data
	CCrossbowBolt* pBolt = GetClassPtr((CCrossbowBolt*)NULL);
	pBolt->pev->classname = MAKE_STRING("bolt");
	pBolt->pev->origin = origin;
	pBolt->pev->angles = UTIL_VecToAngles(dir);
	pBolt->pev->speed = speed;
	pBolt->pev->velocity = dir * pBolt->pev->speed;
	pBolt->pev->avelocity.z = 10;
	pBolt->pev->owner = owner->edict();
	pBolt->Spawn();

	return pBolt;
}

bool CCrossbowBolt::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5;

	SetModel("models/crossbow_bolt.mdl");

	SetSize(g_vecZero, g_vecZero);
	SetOrigin(pev->origin);

	SetTouch(&CCrossbowBolt::BoltTouch);
	SetThink(&CCrossbowBolt::BubbleThink);
	pev->nextthink = gpGlobals->time + 0.2;

	return true;
}


void CCrossbowBolt::Precache()
{
	PRECACHE_MODEL("models/crossbow_bolt.mdl");
	PRECACHE_SOUND("weapons/xbow_hitbod1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/xbow_fly1.wav");
	PRECACHE_SOUND("weapons/xbow_hit1.wav");
	PRECACHE_SOUND("fvox/beep.wav");
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
}


int CCrossbowBolt::Classify()
{
	return CLASS_NONE;
}

void CCrossbowBolt::BoltTouch(CBaseEntity* pOther)
{
	SetTouch(NULL);
	SetThink(NULL);

	Vector vecDir = pev->velocity.Normalize();
	
	if (pOther->IsBSPModel())
	{
		SetOrigin(pev->origin - vecDir * 12);
	}

	pev->angles = UTIL_VecToAngles(vecDir);
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_FLY;
	pev->velocity = Vector(0, 0, 0);

	if (0 != pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		CBaseEntity* owner = CBaseEntity::Instance(pev->owner);

		ClearMultiDamage();

		if (pOther->IsPlayer())
		{
			pOther->TraceAttack(owner, gSkillData.plrDmgCrossbowClient, pev->velocity.Normalize(), &tr, DMG_AIMED | DMG_NEVERGIB);
		}
		else
		{
			pOther->TraceAttack(owner, gSkillData.plrDmgCrossbowMonster, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_AIMED | DMG_NEVERGIB);
		}

		ApplyMultiDamage(this, owner);

		pev->velocity = Vector(0, 0, 0);
		// play body "thwack" sound
		switch (RANDOM_LONG(0, 1))
		{
		case 0: EmitSound("weapons/xbow_hitbod1.wav", CHAN_BODY); break;
		case 1: EmitSound("weapons/xbow_hitbod2.wav", CHAN_BODY); break;
		}
	}
	else
	{
		EmitSound(
			"weapons/xbow_hit1.wav",
			CHAN_BODY,
			RANDOM_FLOAT(0.95, 1.0),
			ATTN_NORM,
			98 + RANDOM_LONG(0, 7));

		if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			UTIL_Sparks(pev->origin);
		}
	}

	if (UTIL_IsDeathmatch())
	{
		SetThink(&CCrossbowBolt::ExplodeThink);
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	Remove();
}

void CCrossbowBolt::BubbleThink()
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->waterlevel == 0)
		return;

	UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 1);
}

void CCrossbowBolt::ExplodeThink()
{
	int iContents = UTIL_PointContents(pev->origin);
	int iScale;

	pev->dmg = 40;
	iScale = 10;

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	if (iContents != CONTENTS_WATER)
	{
		WRITE_SHORT(g_sModelIndexFireball);
	}
	else
	{
		WRITE_SHORT(g_sModelIndexWExplosion);
	}
	WRITE_BYTE(iScale); // scale * 10
	WRITE_BYTE(15);		// framerate
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();

	CBaseEntity* owner;

	if (pev->owner)
		owner = CBaseEntity::Instance(pev->owner);
	else
		owner = this;

	pev->owner = nullptr; // can't traceline attack owner if this is set

	RadiusDamage(pev->origin, this, owner, 128, 128 * 2.5, CLASS_NONE, DMG_BLAST | DMG_ALWAYSGIB);

	Remove();
}
#endif

LINK_ENTITY_TO_CLASS(weapon_crossbow, CCrossbow);

bool CCrossbow::Spawn()
{
	Precache();
	m_iId = WEAPON_CROSSBOW;
	SetModel("models/w_crossbow.mdl");

	m_iDefaultAmmo = CROSSBOW_DEFAULT_GIVE;

	FallInit(); // get ready to fall down.

	return true;
}

void CCrossbow::Precache()
{
	PRECACHE_MODEL("models/w_crossbow.mdl");
	PRECACHE_MODEL("models/v_crossbow.mdl");
	PRECACHE_MODEL("models/p_crossbow.mdl");

	PRECACHE_SOUND("weapons/xbow_fire1.wav");
	PRECACHE_SOUND("weapons/xbow_reload1.wav");

	UTIL_PrecacheOther("crossbow_bolt");

	m_usCrossbow = PRECACHE_EVENT(1, "events/crossbow1.sc");
	m_usCrossbow2 = PRECACHE_EVENT(1, "events/crossbow2.sc");
}


bool CCrossbow::GetWeaponInfo(WeaponInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->iAmmo1 = AMMO_BOLTS;
	p->iMaxAmmo1 = BOLT_MAX_CARRY;
	p->iAmmo2 = AMMO_NONE;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = CROSSBOW_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 2;
	p->iId = WEAPON_CROSSBOW;
	p->iFlags = 0;
	p->iWeight = CROSSBOW_WEIGHT;
	return true;
}


bool CCrossbow::Deploy()
{
	if (0 != m_iClip)
		return DefaultDeploy("models/v_crossbow.mdl", "models/p_crossbow.mdl", CROSSBOW_DRAW1, "bow");
	return DefaultDeploy("models/v_crossbow.mdl", "models/p_crossbow.mdl", CROSSBOW_DRAW2, "bow");
}

bool CCrossbow::Holster()
{
	return DefaultHolster(0 != m_iClip ? CROSSBOW_HOLSTER1 : CROSSBOW_HOLSTER2);
}

void CCrossbow::PrimaryAttack()
{
	if (m_pPlayer->m_iFOV != 0 && UTIL_IsDeathmatch())
	{
		FireSniperBolt();
		return;
	}

	FireBolt();
}

// this function only gets called in multiplayer
void CCrossbow::FireSniperBolt()
{
	m_iNextPrimaryAttack = 750;

	if (m_iClip == 0)
	{
		PlayEmptySound();
		return;
	}

	TraceResult tr;

	m_iClip--;

	m_pPlayer->PlaybackEvent(m_usCrossbow2, 0.0, 0.0, m_iClip);

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(anglesAim);
	Vector vecSrc = m_pPlayer->GetGunPosition() - gpGlobals->v_up * 2;
	Vector vecDir = gpGlobals->v_forward;

	UTIL_TraceLine(vecSrc, vecSrc + vecDir * 8192, dont_ignore_monsters, m_pPlayer, &tr);

#ifndef CLIENT_DLL
	if (0 != tr.pHit->v.takedamage)
	{
		ClearMultiDamage();
		CBaseEntity::Instance(tr.pHit)->TraceAttack(m_pPlayer, 120, vecDir, &tr, DMG_BULLET | DMG_AIMED | DMG_NEVERGIB);
		ApplyMultiDamage(this, m_pPlayer);
	}
#endif
}

void CCrossbow::FireBolt()
{
	TraceResult tr;

	if (m_iClip == 0)
	{
		PlayEmptySound();
		return;
	}

	m_iClip--;

	m_pPlayer->PlaybackEvent(m_usCrossbow, 0.0, 0.0, m_iClip);

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(anglesAim);
	Vector vecSrc = m_pPlayer->GetGunPosition() - gpGlobals->v_up * 2;
	Vector vecDir = gpGlobals->v_forward;

#ifndef CLIENT_DLL
	if (m_pPlayer->pev->waterlevel == 3)
	{
		CCrossbowBolt::BoltCreate(vecSrc, vecDir, BOLT_WATER_VELOCITY, m_pPlayer);
	}
	else
	{
		CCrossbowBolt::BoltCreate(vecSrc, vecDir, BOLT_AIR_VELOCITY, m_pPlayer);
	}
#endif

	m_pPlayer->CheckAmmoLevel(this);

	m_iNextPrimaryAttack = m_iNextSecondaryAttack = 750;

	if (m_iClip != 0)
		m_iTimeWeaponIdle = 5000;
	else
		m_iTimeWeaponIdle = 750;
}


void CCrossbow::SecondaryAttack()
{
	if (m_pPlayer->m_iFOV != 0)
	{
		m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
	}
	else if (m_pPlayer->m_iFOV != 20)
	{
		m_pPlayer->m_iFOV = 20;
	}

	m_iNextSecondaryAttack = 200;
}


void CCrossbow::Reload()
{
	if (DefaultReload(CROSSBOW_MAX_CLIP, CROSSBOW_RELOAD, 4500))
	{
		PlayWeaponSound(CHAN_ITEM, "weapons/xbow_reload1.wav");
		m_pPlayer->m_iFOV = 0;
	}
}


void CCrossbow::WeaponIdle()
{
	if (m_iTimeWeaponIdle <= 0)
	{
		float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
		if (flRand <= 0.75)
		{
			if (0 != m_iClip)
			{
				SendWeaponAnim(CROSSBOW_IDLE1);
			}
			else
			{
				SendWeaponAnim(CROSSBOW_IDLE2);
			}
			m_iTimeWeaponIdle = UTIL_SharedRandomLong(m_pPlayer->random_seed, 10000, 15000);
		}
		else
		{
			if (0 != m_iClip)
			{
				SendWeaponAnim(CROSSBOW_FIDGET1);
				m_iTimeWeaponIdle = 3000;
			}
			else
			{
				SendWeaponAnim(CROSSBOW_FIDGET2);
				m_iTimeWeaponIdle = 2667;
			}
		}
	}
}


IMPLEMENT_AMMO_CLASS(ammo_crossbow, CCrossbowAmmo, "models/w_crossbow_clip.mdl", AMMO_CROSSBOWCLIP_GIVE, AMMO_BOLTS, BOLT_MAX_CARRY);
