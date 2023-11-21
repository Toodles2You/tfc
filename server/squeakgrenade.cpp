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

enum w_squeak_e
{
	WSQUEAK_IDLE1 = 0,
	WSQUEAK_FIDGET,
	WSQUEAK_JUMP,
	WSQUEAK_RUN,
};

#ifndef CLIENT_DLL

class CSqueakGrenade : public CGrenade
{
	bool Spawn() override;
	void Precache() override;
	int IRelationship(CBaseEntity* pTarget) override;
	int Classify() override;
	void EXPORT SuperBounceTouch(CBaseEntity* pOther);
	void EXPORT HuntThink();
	int BloodColor() override { return BLOOD_COLOR_YELLOW; }
	void Killed(CBaseEntity* inflictor, CBaseEntity* attacker, int bitsDamageType) override;
	void GibMonster();

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	static float m_flNextBounceSoundTime;

	// CBaseEntity *m_pTarget;
	float m_flDie;
	EHANDLE m_hEnemy;
	Vector m_vecTarget;
	float m_flNextHunt;
	float m_flNextHit;
	Vector m_posPrev;
	EHANDLE m_hOwner;
};

float CSqueakGrenade::m_flNextBounceSoundTime = 0;

LINK_ENTITY_TO_CLASS(monster_snark, CSqueakGrenade);
TYPEDESCRIPTION CSqueakGrenade::m_SaveData[] =
	{
		DEFINE_FIELD(CSqueakGrenade, m_flDie, FIELD_TIME),
		DEFINE_FIELD(CSqueakGrenade, m_vecTarget, FIELD_VECTOR),
		DEFINE_FIELD(CSqueakGrenade, m_flNextHunt, FIELD_TIME),
		DEFINE_FIELD(CSqueakGrenade, m_flNextHit, FIELD_TIME),
		DEFINE_FIELD(CSqueakGrenade, m_posPrev, FIELD_POSITION_VECTOR),
		DEFINE_FIELD(CSqueakGrenade, m_hOwner, FIELD_EHANDLE),
};

IMPLEMENT_SAVERESTORE(CSqueakGrenade, CGrenade);

#define SQUEEK_DETONATE_DELAY 15.0

int CSqueakGrenade::IRelationship(CBaseEntity* pTarget)
{
	if (pTarget->IsPlayer())
	{
		if (!pev->owner || pev->owner == pTarget->edict())
		{
			return R_DL;
		}

		auto owner = CBaseEntity::Instance(pev->owner);
		auto relationship = g_pGameRules->PlayerRelationship(owner, pTarget);
		
		if (relationship != GR_TEAMMATE && relationship != GR_ALLY)
		{
			return R_DL;
		}
	}

	return R_NO;
}

int CSqueakGrenade::Classify()
{
	if (pev->owner && (pev->owner->v.flags & FL_CLIENT) != 0)
	{
		return CLASS_PLAYER_BIOWEAPON;
	}

	return CLASS_ALIEN_BIOWEAPON;
}

bool CSqueakGrenade::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/w_squeak.mdl");
	SetSize(Vector(-4, -4, 0), Vector(4, 4, 8));
	SetOrigin(pev->origin);

	SetTouch(&CSqueakGrenade::SuperBounceTouch);
	SetThink(&CSqueakGrenade::HuntThink);
	pev->nextthink = gpGlobals->time + 0.1;
	m_flNextHunt = gpGlobals->time + 1E6;

	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_AIM;
	pev->health = gSkillData.snarkHealth;
	pev->gravity = 0.5;
	pev->friction = 0.5;

	pev->dmg = gSkillData.snarkDmgPop;

	m_flDie = gpGlobals->time + SQUEEK_DETONATE_DELAY;

	m_flFieldOfView = 0; // 180 degrees

	if (pev->owner)
		m_hOwner = Instance(pev->owner);

	m_flNextBounceSoundTime = gpGlobals->time; // reset each time a snark is spawned.

	pev->sequence = WSQUEAK_RUN;
	ResetSequenceInfo();

	return true;
}

void CSqueakGrenade::Precache()
{
	PRECACHE_MODEL("models/w_squeak.mdl");
	PRECACHE_SOUND("squeek/sqk_blast1.wav");
	PRECACHE_SOUND("common/bodysplat.wav");
	PRECACHE_SOUND("squeek/sqk_die1.wav");
	PRECACHE_SOUND("squeek/sqk_hunt1.wav");
	PRECACHE_SOUND("squeek/sqk_hunt2.wav");
	PRECACHE_SOUND("squeek/sqk_hunt3.wav");
	PRECACHE_SOUND("squeek/sqk_deploy1.wav");
}


void CSqueakGrenade::Killed(CBaseEntity* inflictor, CBaseEntity* attacker, int bitsDamageType)
{
	pev->model = iStringNull; // make invisible
	SetTouch(NULL);

	// since squeak grenades never leave a body behind, clear out their takedamage now.
	// Squeaks do a bit of radius damage when they pop, and that radius damage will
	// continue to call this function unless we acknowledge the Squeak's death now. (sjb)
	pev->takedamage = DAMAGE_NO;

	// play squeek blast
	EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, "squeek/sqk_blast1.wav", 1, 0.5, 0, PITCH_NORM);

	UTIL_BloodDrips(pev->origin, g_vecZero, BloodColor(), 80);

	if (m_hOwner != NULL)
		RadiusDamage(pev->origin, this, m_hOwner, pev->dmg, pev->dmg * 2.5, CLASS_NONE, DMG_BLAST);
	else
		RadiusDamage(pev->origin, this, this, pev->dmg, pev->dmg * 2.5, CLASS_NONE, DMG_BLAST);

	// reset owner so death message happens
	if (m_hOwner != NULL)
		pev->owner = m_hOwner->edict();

	CBaseEntity::Killed(inflictor, attacker, bitsDamageType | DMG_ALWAYSGIB);
	
	if (pev->owner && (pev->owner->v.flags & FL_CLIENT) == 0)
		CBaseEntity::Instance(pev->owner)->DeathNotice(pev);

	Remove();
}

void CSqueakGrenade::GibMonster()
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "common/bodysplat.wav", 0.75, ATTN_NORM, 0, 200);
}



void CSqueakGrenade::HuntThink()
{
	// ALERT( at_console, "think\n" );

	if (!IsInWorld())
	{
		SetTouch(NULL);
		Remove();
		return;
	}

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	// explode when ready
	if (gpGlobals->time >= m_flDie)
	{
		g_vecAttackDir = pev->velocity.Normalize();
		pev->health = -1;
		Killed(this, this, DMG_ALWAYSGIB);
		return;
	}

	// float
	if (pev->waterlevel != 0)
	{
		if (pev->movetype == MOVETYPE_BOUNCE)
		{
			pev->movetype = MOVETYPE_FLY;
		}
		pev->velocity = pev->velocity * 0.9;
		pev->velocity.z += 8.0;
	}
	else if (pev->movetype == MOVETYPE_FLY)
	{
		pev->movetype = MOVETYPE_BOUNCE;
	}

	// return if not time to hunt
	if (m_flNextHunt > gpGlobals->time)
		return;

	m_flNextHunt = gpGlobals->time + 2.0;

	CBaseEntity* pOther = NULL;
	Vector vecDir;
	TraceResult tr;

	Vector vecFlat = pev->velocity;
	vecFlat.z = 0;
	vecFlat = vecFlat.Normalize();

	UTIL_MakeVectors(pev->angles);

	if (m_hEnemy == NULL || !m_hEnemy->IsAlive())
	{
		// find target, bounce a bit towards it.
		Look(512);
		m_hEnemy = BestVisibleEnemy();
	}

	// squeek if it's about time blow up
	if ((m_flDie - gpGlobals->time <= 0.5) && (m_flDie - gpGlobals->time >= 0.3))
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "squeek/sqk_die1.wav", 1, ATTN_NORM, 0, 100 + RANDOM_LONG(0, 0x3F));
	}

	// higher pitch as squeeker gets closer to detonation time
	float flpitch = 155.0 - 60.0 * ((m_flDie - gpGlobals->time) / SQUEEK_DETONATE_DELAY);
	if (flpitch < 80)
		flpitch = 80;

	if (m_hEnemy != NULL)
	{
		if (FVisible(m_hEnemy))
		{
			vecDir = m_hEnemy->EyePosition() - pev->origin;
			m_vecTarget = vecDir.Normalize();
		}

		float flVel = pev->velocity.Length();
		float flAdj = 50.0 / (flVel + 10.0);

		if (flAdj > 1.2)
			flAdj = 1.2;

		// ALERT( at_console, "think : enemy\n");

		// ALERT( at_console, "%.0f %.2f %.2f %.2f\n", flVel, m_vecTarget.x, m_vecTarget.y, m_vecTarget.z );

		pev->velocity = pev->velocity * flAdj + m_vecTarget * 300;
	}

	if ((pev->flags & FL_ONGROUND) != 0)
	{
		pev->avelocity = Vector(0, 0, 0);
	}
	else
	{
		if (pev->avelocity == Vector(0, 0, 0))
		{
			pev->avelocity.x = RANDOM_FLOAT(-100, 100);
			pev->avelocity.z = RANDOM_FLOAT(-100, 100);
		}
	}

	if ((pev->origin - m_posPrev).Length() < 1.0)
	{
		pev->velocity.x = RANDOM_FLOAT(-100, 100);
		pev->velocity.y = RANDOM_FLOAT(-100, 100);
	}
	m_posPrev = pev->origin;

	pev->angles = UTIL_VecToAngles(pev->velocity);
	pev->angles.z = 0;
	pev->angles.x = 0;
}


void CSqueakGrenade::SuperBounceTouch(CBaseEntity* pOther)
{
	float flpitch;

	TraceResult tr = UTIL_GetGlobalTrace();

	// don't hit the guy that launched this grenade
	if (pev->owner && pOther->edict() == pev->owner)
		return;

	// at least until we've bounced once
	pev->owner = NULL;

	pev->angles.x = 0;
	pev->angles.z = 0;

	// avoid bouncing too much
	if (m_flNextHit > gpGlobals->time)
		return;

	// higher pitch as squeeker gets closer to detonation time
	flpitch = 155.0 - 60.0 * ((m_flDie - gpGlobals->time) / SQUEEK_DETONATE_DELAY);

	if (0 != pOther->pev->takedamage && m_flNextAttack < gpGlobals->time)
	{
		// attack!

		// make sure it's me who has touched them
		if (tr.pHit == pOther->edict())
		{
			// and it's not another squeakgrenade
			if (tr.pHit->v.modelindex != pev->modelindex)
			{
				// ALERT( at_console, "hit enemy\n");
				ClearMultiDamage();
				pOther->TraceAttack(this, gSkillData.snarkDmgBite, gpGlobals->v_forward, &tr, DMG_SLASH);
				if (m_hOwner != NULL)
					ApplyMultiDamage(this, m_hOwner);
				else
					ApplyMultiDamage(this, this);

				pev->dmg += gSkillData.snarkDmgPop; // add more explosion damage
				// m_flDie += 2.0; // add more life

				// make bite sound
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "squeek/sqk_deploy1.wav", 1.0, ATTN_NORM, 0, (int)flpitch);
				m_flNextAttack = gpGlobals->time + 0.5;
			}
		}
		else
		{
			// ALERT( at_console, "been hit\n");
		}
	}

	m_flNextHit = gpGlobals->time + 0.1;
	m_flNextHunt = gpGlobals->time;

	if (UTIL_IsMultiplayer())
	{
		// in multiplayer, we limit how often snarks can make their bounce sounds to prevent overflows.
		if (gpGlobals->time < m_flNextBounceSoundTime)
		{
			// too soon!
			return;
		}
	}

	if ((pev->flags & FL_ONGROUND) == 0)
	{
		// play bounce sound
		float flRndSound = RANDOM_FLOAT(0, 1);

		if (flRndSound <= 0.33)
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "squeek/sqk_hunt1.wav", 1, ATTN_NORM, 0, (int)flpitch);
		else if (flRndSound <= 0.66)
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "squeek/sqk_hunt2.wav", 1, ATTN_NORM, 0, (int)flpitch);
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "squeek/sqk_hunt3.wav", 1, ATTN_NORM, 0, (int)flpitch);
	}

	m_flNextBounceSoundTime = gpGlobals->time + 0.5; // half second.
}

#endif

LINK_ENTITY_TO_CLASS(weapon_snark, CSqueak);


bool CSqueak::Spawn()
{
	Precache();
	m_iId = WEAPON_SNARK;
	SET_MODEL(ENT(pev), "models/w_sqknest.mdl");

	FallInit(); //get ready to fall down.

	m_iDefaultAmmo = SNARK_DEFAULT_GIVE;

	pev->sequence = 1;
	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0;
	
	return true;
}


void CSqueak::Precache()
{
	PRECACHE_MODEL("models/w_sqknest.mdl");
	PRECACHE_MODEL("models/v_squeak.mdl");
	PRECACHE_MODEL("models/p_squeak.mdl");
	PRECACHE_SOUND("squeek/sqk_hunt2.wav");
	PRECACHE_SOUND("squeek/sqk_hunt3.wav");
	UTIL_PrecacheOther("monster_snark");

	m_usSnarkFire = PRECACHE_EVENT(1, "events/snarkfire.sc");
}


bool CSqueak::GetWeaponInfo(WeaponInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->iAmmo1 = AMMO_SNARKS;
	p->iMaxAmmo1 = SNARK_MAX_CARRY;
	p->iAmmo2 = AMMO_NONE;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_SNARK;
	p->iWeight = SNARK_WEIGHT;
	p->iFlags = WEAPON_FLAG_LIMITINWORLD | WEAPON_FLAG_EXHAUSTIBLE;

	return true;
}



bool CSqueak::Deploy()
{
	// play hunt sound
	float flRndSound = RANDOM_FLOAT(0, 1);

	if (flRndSound <= 0.5)
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "squeek/sqk_hunt2.wav", 1, ATTN_NORM, 0, 100);
	else
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "squeek/sqk_hunt3.wav", 1, ATTN_NORM, 0, 100);

	const bool result = DefaultDeploy("models/v_squeak.mdl", "models/p_squeak.mdl", SQUEAK_UP, "squeak");

	if (result)
	{
		m_iTimeWeaponIdle = 1700;
	}

	return result;
}


bool CSqueak::Holster()
{
	const auto bHasAmmo = m_pPlayer->m_rgAmmo[iAmmo1()] != 0;
	
	if (DefaultHolster(bHasAmmo ? SQUEAK_DOWN : -1))
	{
		m_fJustThrown = false;
		if (!bHasAmmo)
		{
			m_pPlayer->ClearWeaponBit(m_iId);
			SetThink(&CSqueak::DestroyWeapon);
			pev->nextthink = gpGlobals->time + 0.1;
		}
		return true;
	}

	return false;
}


void CSqueak::PrimaryAttack()
{
	if (0 != m_pPlayer->m_rgAmmo[iAmmo1()])
	{
		UTIL_MakeVectors(m_pPlayer->pev->v_angle);
		TraceResult tr;
		Vector trace_origin;

		// HACK HACK:  Ugly hacks to handle change in origin based on new physics code for players
		// Move origin up if crouched and start trace a bit outside of body ( 20 units instead of 16 )
		trace_origin = m_pPlayer->pev->origin;
		if ((m_pPlayer->pev->flags & FL_DUCKING) != 0)
		{
			trace_origin = trace_origin - (VEC_HULL_MIN - VEC_DUCK_HULL_MIN);
		}

		// find place to toss monster
		UTIL_TraceLine(trace_origin + gpGlobals->v_forward * 20, trace_origin + gpGlobals->v_forward * 64, dont_ignore_monsters, nullptr, &tr);

		m_pPlayer->PlaybackEvent(m_usSnarkFire);

		if (tr.fAllSolid == 0 && tr.fStartSolid == 0 && tr.flFraction > 0.25)
		{
			// player "shoot" animation
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL
			CBaseEntity* pSqueak = CBaseEntity::Create("monster_snark", tr.vecEndPos, m_pPlayer->pev->v_angle, m_pPlayer->edict());
			pSqueak->pev->velocity = gpGlobals->v_forward * 200 + m_pPlayer->pev->velocity;
#endif

			// play hunt sound
			float flRndSound = RANDOM_FLOAT(0, 1);

			if (flRndSound <= 0.5)
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "squeek/sqk_hunt2.wav", 1, ATTN_NORM, 0, 105);
			else
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "squeek/sqk_hunt3.wav", 1, ATTN_NORM, 0, 105);

			m_pPlayer->m_rgAmmo[iAmmo1()]--;

			m_fJustThrown = true;

			m_iNextPrimaryAttack = 300;
			m_iTimeWeaponIdle = 1000;
		}
	}
}


void CSqueak::SecondaryAttack()
{
}


void CSqueak::WeaponIdle()
{
	if (m_iTimeWeaponIdle > 0)
		return;

	if (m_fJustThrown)
	{
		m_fJustThrown = false;

		if (0 == m_pPlayer->m_rgAmmo[iAmmo1()])
		{
			RetireWeapon();
			return;
		}

		SendWeaponAnim(SQUEAK_UP);
		m_iTimeWeaponIdle = UTIL_SharedRandomLong(m_pPlayer->random_seed, 10000, 15000);
		return;
	}

	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.75)
	{
		iAnim = SQUEAK_IDLE1;
		m_iTimeWeaponIdle = 3750;
	}
	else if (flRand <= 0.875)
	{
		iAnim = SQUEAK_FIDGETFIT;
		m_iTimeWeaponIdle = 4375;
	}
	else
	{
		iAnim = SQUEAK_FIDGETNIP;
		m_iTimeWeaponIdle = 5000;
	}
	SendWeaponAnim(iAnim);
}
