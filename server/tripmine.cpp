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
#include "effects.h"
#include "gamerules.h"

#define TRIPMINE_PRIMARY_VOLUME 450

#ifndef CLIENT_DLL

#define SF_TRIPMINE_INSTANT_ON 1

class CTripmineGrenade : public CGrenade
{
	bool Spawn() override;
	void Precache() override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;

	void EXPORT WarningThink();
	void EXPORT PowerupThink();
	void EXPORT BeamBreakThink();
	void EXPORT DelayDeathThink();
	void Killed(CBaseEntity* inflictor, CBaseEntity* attacker, int bitsDamageType) override;

	void MakeBeam();
	void KillBeam();

	float m_flPowerUp;
	Vector m_vecDir;
	Vector m_vecEnd;
	float m_flBeamLength;

	EHANDLE m_hOwner;
	CBeam* m_pBeam;
	Vector m_posOwner;
	Vector m_angleOwner;
	edict_t* m_pRealOwner; // tracelines don't hit PEV->OWNER, which means a player couldn't detonate his own trip mine, so we store the owner here.
};

LINK_ENTITY_TO_CLASS(monster_tripmine, CTripmineGrenade);

TYPEDESCRIPTION CTripmineGrenade::m_SaveData[] =
	{
		DEFINE_FIELD(CTripmineGrenade, m_flPowerUp, FIELD_TIME),
		DEFINE_FIELD(CTripmineGrenade, m_vecDir, FIELD_VECTOR),
		DEFINE_FIELD(CTripmineGrenade, m_vecEnd, FIELD_POSITION_VECTOR),
		DEFINE_FIELD(CTripmineGrenade, m_flBeamLength, FIELD_FLOAT),
		DEFINE_FIELD(CTripmineGrenade, m_hOwner, FIELD_EHANDLE),
		//Don't save, recreate.
		//DEFINE_FIELD(CTripmineGrenade, m_pBeam, FIELD_CLASSPTR),
		DEFINE_FIELD(CTripmineGrenade, m_posOwner, FIELD_POSITION_VECTOR),
		DEFINE_FIELD(CTripmineGrenade, m_angleOwner, FIELD_VECTOR),
		DEFINE_FIELD(CTripmineGrenade, m_pRealOwner, FIELD_EDICT),
};

IMPLEMENT_SAVERESTORE(CTripmineGrenade, CGrenade);


bool CTripmineGrenade::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;

	SetModel("models/v_tripmine.mdl");
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = TRIPMINE_WORLD;
	ResetSequenceInfo();
	pev->framerate = 0;

	SetSize(Vector(-8, -8, -8), Vector(8, 8, 8));
	SetOrigin(pev->origin);

	if ((pev->spawnflags & SF_TRIPMINE_INSTANT_ON) != 0)
	{
		// power up quickly
		m_flPowerUp = gpGlobals->time + 1.0;
	}
	else
	{
		// power up in 2.5 seconds
		m_flPowerUp = gpGlobals->time + 2.5;
	}

	SetThink(&CTripmineGrenade::PowerupThink);
	pev->nextthink = gpGlobals->time + 0.2;

	pev->takedamage = DAMAGE_YES;
	pev->dmg = gSkillData.plrDmgTripmine;
	pev->health = 1; // don't let die normally

	if (pev->owner != NULL)
	{
		// play deploy sound
		EmitSound("weapons/mine_deploy.wav", CHAN_VOICE);
		EmitSound("weapons/mine_charge.wav", CHAN_BODY, 0.2F);

		m_pRealOwner = pev->owner; // see CTripmineGrenade for why.
	}

	UTIL_MakeAimVectors(pev->angles);

	m_vecDir = gpGlobals->v_forward;
	m_vecEnd = pev->origin + m_vecDir * 2048;

	return true;
}


void CTripmineGrenade::Precache()
{
	PRECACHE_MODEL("models/v_tripmine.mdl");
	PRECACHE_SOUND("weapons/mine_deploy.wav");
	PRECACHE_SOUND("weapons/mine_activate.wav");
	PRECACHE_SOUND("weapons/mine_charge.wav");
}


void CTripmineGrenade::WarningThink()
{
	// set to power up
	SetThink(&CTripmineGrenade::PowerupThink);
	pev->nextthink = gpGlobals->time + 1.0;
}


void CTripmineGrenade::PowerupThink()
{
	TraceResult tr;

	if (m_hOwner == NULL)
	{
		// find an owner
		edict_t* oldowner = pev->owner;
		pev->owner = NULL;
		UTIL_TraceLine(pev->origin + m_vecDir * 8, pev->origin - m_vecDir * 32, dont_ignore_monsters, this, &tr);
		if (0 != tr.fStartSolid || (oldowner && tr.pHit == oldowner))
		{
			pev->owner = oldowner;
			m_flPowerUp += 0.1;
			pev->nextthink = gpGlobals->time + 0.1;
			return;
		}
		if (tr.flFraction < 1.0)
		{
			pev->owner = tr.pHit;
			m_hOwner = CBaseEntity::Instance(pev->owner);
			m_posOwner = m_hOwner->pev->origin;
			m_angleOwner = m_hOwner->pev->angles;
		}
		else
		{
			StopSound("weapons/mine_deploy.wav", CHAN_VOICE);
			StopSound("weapons/mine_charge.wav", CHAN_BODY);
			ALERT(at_console, "WARNING: Tripmine at %.0f, %.0f, %.0f removed\n", pev->origin.x, pev->origin.y, pev->origin.z);
			KillBeam();
			Remove();
			return;
		}
	}
	else if (m_posOwner != m_hOwner->pev->origin || m_angleOwner != m_hOwner->pev->angles)
	{
		// disable
		StopSound("weapons/mine_deploy.wav", CHAN_VOICE);
		StopSound("weapons/mine_charge.wav", CHAN_BODY);
		CBaseEntity* pMine = Create("weapon_tripmine", pev->origin + m_vecDir * 24, pev->angles);
		pMine->pev->spawnflags |= SF_NORESPAWN;

		KillBeam();
		Remove();
		return;
	}

	if (gpGlobals->time > m_flPowerUp)
	{
		// make solid
		pev->solid = SOLID_BBOX;
		SetOrigin(pev->origin);

		MakeBeam();

		// play enabled sound
		EmitSound("weapons/mine_activate.wav", CHAN_VOICE, 0.5F, ATTN_NORM, 75);
	}
	pev->nextthink = gpGlobals->time + 0.1;
}


void CTripmineGrenade::KillBeam()
{
	if (m_pBeam)
	{
		m_pBeam->Remove();
		m_pBeam = NULL;
	}
}


void CTripmineGrenade::MakeBeam()
{
	TraceResult tr;

	// ALERT( at_console, "serverflags %f\n", gpGlobals->serverflags );

	UTIL_TraceLine(pev->origin, m_vecEnd, dont_ignore_monsters, this, &tr);

	m_flBeamLength = tr.flFraction;

	// set to follow laser spot
	SetThink(&CTripmineGrenade::BeamBreakThink);
	pev->nextthink = gpGlobals->time + 0.1;

	Vector vecTmpEnd = pev->origin + m_vecDir * 2048 * m_flBeamLength;

	m_pBeam = CBeam::BeamCreate(g_pModelNameLaser, 10);
	//Mark as temporary so the beam will be recreated on save game load and level transitions.
	m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
	//PointEntInit causes clients to use the position of whatever the previous entity to use this edict had until the server updates them.
	//m_pBeam->PointEntInit(vecTmpEnd, entindex());
	m_pBeam->PointsInit(pev->origin, vecTmpEnd);
	m_pBeam->SetColor(0, 214, 198);
	m_pBeam->SetScrollRate(255);
	m_pBeam->SetBrightness(64);
}


void CTripmineGrenade::BeamBreakThink()
{
	bool bBlowup = false;

	TraceResult tr;

	// HACKHACK Set simple box using this really nice global!
	gpGlobals->trace_flags = FTRACE_SIMPLEBOX;
	UTIL_TraceLine(pev->origin, m_vecEnd, dont_ignore_monsters, this, &tr);

	// ALERT( at_console, "%f : %f\n", tr.flFraction, m_flBeamLength );

	// respawn detect.
	if (!m_pBeam)
	{
		// Use the same trace parameters as the original trace above so the right entity is hit.
		TraceResult tr2;
		// Clear out old owner so it can be hit by traces.
		pev->owner = nullptr;
		UTIL_TraceLine(pev->origin + m_vecDir * 8, pev->origin - m_vecDir * 32, dont_ignore_monsters, this, &tr2);
		MakeBeam();
		if (tr2.pHit)
		{
			// reset owner too
			pev->owner = tr2.pHit;
			m_hOwner = CBaseEntity::Instance(tr2.pHit);
		}
	}

	if (fabs(m_flBeamLength - tr.flFraction) > 0.001)
	{
		bBlowup = true;
	}
	else
	{
		if (m_hOwner == NULL)
			bBlowup = true;
		else if (m_posOwner != m_hOwner->pev->origin)
			bBlowup = true;
		else if (m_angleOwner != m_hOwner->pev->angles)
			bBlowup = true;
	}

	if (bBlowup)
	{
		// a bit of a hack, but all CGrenade code passes pev->owner along to make sure the proper player gets credit for the kill
		// so we have to restore pev->owner from pRealOwner, because an entity's tracelines don't strike it's pev->owner which meant
		// that a player couldn't trigger his own tripmine. Now that the mine is exploding, it's safe the restore the owner so the
		// CGrenade code knows who the explosive really belongs to.
		pev->owner = m_pRealOwner;
		pev->health = 0;
		CBaseEntity* owner;
		if (pev->owner)
			owner = CBaseEntity::Instance(pev->owner);
		else
			owner = this;
		Killed(owner, owner, DMG_GENERIC);
		return;
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

bool CTripmineGrenade::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	if (gpGlobals->time < m_flPowerUp && flDamage < pev->health)
	{
		// disable
		KillBeam();
		Remove();
		return false;
	}
	return CGrenade::TakeDamage(inflictor, attacker, flDamage, bitsDamageType);
}

void CTripmineGrenade::Killed(CBaseEntity* inflictor, CBaseEntity* attacker, int bitsDamageType)
{
	pev->takedamage = DAMAGE_NO;

	if (attacker && (attacker->pev->flags & FL_CLIENT) != 0)
	{
		// some client has destroyed this mine, they'll get credit for any kills
		pev->owner = attacker->edict();
	}

	SetThink(&CTripmineGrenade::DelayDeathThink);
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT(0.1, 0.3);

	EmitSound("common/null.wav", CHAN_BODY); // shut off chargeup
}


void CTripmineGrenade::DelayDeathThink()
{
	KillBeam();
	TraceResult tr;
	UTIL_TraceLine(pev->origin + m_vecDir * 8, pev->origin - m_vecDir * 64, dont_ignore_monsters, this, &tr);

	Explode(&tr, DMG_BLAST);
}
#endif

LINK_ENTITY_TO_CLASS(weapon_tripmine, CTripmine);

bool CTripmine::Spawn()
{
	Precache();
	m_iId = WEAPON_TRIPMINE;
	SetModel("models/v_tripmine.mdl");
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = TRIPMINE_GROUND;
	// ResetSequenceInfo( );
	pev->framerate = 0;

	FallInit(); // get ready to fall down

	m_iDefaultAmmo = TRIPMINE_DEFAULT_GIVE;

	//HACK: force the body to the first person view by default so it doesn't show up as a huge tripmine for a second.
#ifdef CLIENT_DLL
	pev->body = 0;
#endif

	if (!UTIL_IsDeathmatch())
	{
		SetSize(Vector(-16, -16, 0), Vector(16, 16, 28));
	}

	return true;
}

void CTripmine::Precache()
{
	PRECACHE_MODEL("models/v_tripmine.mdl");
	PRECACHE_MODEL("models/p_tripmine.mdl");
	UTIL_PrecacheOther("monster_tripmine");

	m_usTripFire = PRECACHE_EVENT(1, "events/tripfire.sc");
}

bool CTripmine::GetWeaponInfo(WeaponInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->iAmmo1 = AMMO_TRIPMINES;
	p->iMaxAmmo1 = TRIPMINE_MAX_CARRY;
	p->iAmmo2 = AMMO_NONE;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_TRIPMINE;
	p->iWeight = TRIPMINE_WEIGHT;
	p->iFlags = WEAPON_FLAG_LIMITINWORLD | WEAPON_FLAG_EXHAUSTIBLE;

	return true;
}

bool CTripmine::Deploy()
{
	pev->body = 0;
	return DefaultDeploy("models/v_tripmine.mdl", "models/p_tripmine.mdl", TRIPMINE_DRAW, "trip");
}


bool CTripmine::Holster()
{
	const auto bHasAmmo = m_pPlayer->m_rgAmmo[iAmmo1()] != 0;
	
	if (DefaultHolster(bHasAmmo ? TRIPMINE_HOLSTER : -1))
	{
		if (!bHasAmmo)
		{
			m_pPlayer->ClearWeaponBit(m_iId);
			SetThink(&CTripmine::DestroyWeapon);
			pev->nextthink = gpGlobals->time + 0.1;
		}
		return true;
	}

	return false;
}

void CTripmine::PrimaryAttack()
{
	if (m_pPlayer->m_rgAmmo[iAmmo1()] <= 0)
		return;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;

	TraceResult tr;

	UTIL_TraceLine(vecSrc, vecSrc + vecAiming * 128, dont_ignore_monsters, m_pPlayer, &tr);

	m_pPlayer->PlaybackEvent(m_usTripFire);

	if (tr.flFraction < 1.0)
	{
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);
		if (pEntity && (pEntity->pev->flags & FL_CONVEYOR) == 0)
		{
			Vector angles = UTIL_VecToAngles(tr.vecPlaneNormal);

			CBaseEntity* pEnt = CBaseEntity::Create("monster_tripmine", tr.vecEndPos + tr.vecPlaneNormal * 8, angles, m_pPlayer->edict());

			m_pPlayer->m_rgAmmo[iAmmo1()]--;

			// player "shoot" animation
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);

			if (m_pPlayer->m_rgAmmo[iAmmo1()] <= 0)
			{
				// no more mines!
				RetireWeapon();
				return;
			}
		}
		else
		{
			// ALERT( at_console, "no deploy\n" );
		}
	}
	else
	{
	}

	m_iNextPrimaryAttack = 300;
	m_iTimeWeaponIdle = UTIL_SharedRandomLong(m_pPlayer->random_seed, 10000, 15000);
}

void CTripmine::WeaponIdle()
{
	//If we're here then we're in a player's inventory, and need to use this body
	pev->body = 0;

	if (m_iTimeWeaponIdle > 0)
		return;

	if (m_pPlayer->m_rgAmmo[iAmmo1()] > 0)
	{
		SendWeaponAnim(TRIPMINE_DRAW);
	}
	else
	{
		RetireWeapon();
		return;
	}

	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.25)
	{
		iAnim = TRIPMINE_IDLE1;
		m_iTimeWeaponIdle = 3000;
	}
	else if (flRand <= 0.75)
	{
		iAnim = TRIPMINE_IDLE2;
		m_iTimeWeaponIdle = 2000;
	}
	else
	{
		iAnim = TRIPMINE_FIDGET;
		m_iTimeWeaponIdle = 3333;
	}

	SendWeaponAnim(iAnim);
}
