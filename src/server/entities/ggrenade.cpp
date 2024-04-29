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
/*

===== generic grenade.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "UserMessages.h"
#include "gamerules.h"


unsigned short g_usGetNailedIdiot;


LINK_ENTITY_TO_CLASS(grenade, CGrenade);

//
// Grenade Explode
//
void CGrenade::Explode(Vector vecSrc, Vector vecAim)
{
	if (!CanDetonate())
	{
		Remove();
		return;
	}

	TraceResult tr;
	util::TraceLine(pev->origin, pev->origin + Vector(0, 0, -32), util::ignore_monsters, this, &tr);

	Explode(&tr, DMG_BLAST);
}

void CGrenade::Explode(TraceResult* pTrace, int bitsDamageType)
{
	// Pull out of the wall a bit
	if (pTrace->flFraction != 1.0)
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * 0.6);
	}

	tent::Explosion(pev->origin, -pTrace->vecPlaneNormal, tent::ExplosionType::Normal, pev->dmg);

	CBaseEntity* owner;
	if (pev->owner)
		owner = CBaseEntity::Instance(pev->owner);
	else
		owner = this;

	pev->owner = nullptr; // can't traceline attack owner if this is set

	RadiusDamage(pev->origin, this, owner, pev->dmg, pev->dmg_save, pev->dmg_take, DMG_RESIST_SELF | bitsDamageType);

	Remove();
}


void CGrenade::Detonate()
{
	if (!CanDetonate())
	{
		Remove();
		return;
	}

	TraceResult tr;
	Vector vecSpot; // trace starts here!

	vecSpot = pev->origin + Vector(0, 0, 8);
	util::TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), util::ignore_monsters, this, &tr);

	Explode(&tr, DMG_BLAST);
}


//
// Contact grenade, explode when it touches something
//
void CGrenade::ExplodeTouch(CBaseEntity* pOther)
{
	TraceResult tr;
	Vector vecSpot; // trace starts here!

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	util::TraceLine(vecSpot, vecSpot + pev->velocity.Normalize() * 64, util::ignore_monsters, this, &tr);

	Explode(&tr, DMG_BLAST);
}


void CGrenade::BounceTouch(CBaseEntity* pOther)
{
	// don't hit the guy that launched this grenade
	if (pOther->edict() == pev->owner)
		return;

	if ((pev->flags & FL_ONGROUND) != 0)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.8;

		pev->sequence = RANDOM_LONG(1, 1);
		ResetSequenceInfo();
	}
	else
	{
		if (pev->velocity.z > 0
		 && util::GetGlobalTrace().vecPlaneNormal.z >= kGroundPlaneMinZ)
		{
			pev->velocity = pev->velocity * 0.8;
		}

		// play bounce sound
		BounceSound();
	}
	pev->framerate = pev->velocity.Length() / 200.0;
	if (pev->framerate > 1.0)
		pev->framerate = 1;
	else if (pev->framerate < 0.5)
		pev->framerate = 0;
}


void CGrenade::BounceSound()
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0: EmitSound("weapons/grenade_hit1.wav", CHAN_VOICE, 0.25F); break;
	case 1: EmitSound("weapons/grenade_hit2.wav", CHAN_VOICE, 0.25F); break;
	case 2: EmitSound("weapons/grenade_hit3.wav", CHAN_VOICE, 0.25F); break;
	}
}

void CGrenade::TumbleThink()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->dmgtime <= gpGlobals->time)
	{
		SetThink(&CGrenade::Detonate);
	}
	if (pev->waterlevel > kWaterLevelNone)
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2;
	}
	else
	{
		pev->framerate = 1.0;
	}
}


bool CGrenade::Spawn()
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->classname = MAKE_STRING("grenade");

	pev->solid = SOLID_BBOX;

	SetModel("models/grenade.mdl");
	SetSize(g_vecZero, g_vecZero);

	pev->dmg = 100;
	pev->dmg_save = 0;
	pev->dmg_take = 250;

	return true;
}


CGrenade* CGrenade::ShootContact(CBaseEntity* owner, Vector vecStart, Vector vecVelocity)
{
	CGrenade* pGrenade = GetClassPtr((CGrenade*)NULL);
	pGrenade->Spawn();
	// contact grenades arc lower
	pGrenade->pev->gravity = 0.5; // lower gravity since grenade is aerodynamic and engine doesn't know it.
	pGrenade->SetOrigin(vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = util::VecToAngles(pGrenade->pev->velocity);
	pGrenade->pev->owner = owner->edict();

	// Tumble in air
	pGrenade->pev->avelocity.x = RANDOM_FLOAT(-100, -500);

	// Explode on contact
	pGrenade->SetTouch(&CGrenade::ExplodeTouch);

	pGrenade->pev->dmg = 100;
	pGrenade->pev->dmg_save = 0;
	pGrenade->pev->dmg_take = 250;

	return pGrenade;
}


#ifdef HALFLIFE_GRENADES

bool CPrimeGrenade::Spawn()
{
	pev->classname = MAKE_STRING("grenade");
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	SetSize(g_vecZero, g_vecZero);

	SetOrigin(pev->owner->v.origin);

	SetThink(&CPrimeGrenade::PrimedThink);
	pev->nextthink = gpGlobals->time + 0.8;
	pev->dmgtime = pev->nextthink + 3.0;

	pev->dmg = 180;
	pev->dmg_save = 0;
	pev->dmg_take = 360;

	return true;
}


void CPrimeGrenade::PrimedThink()
{
	auto owner = dynamic_cast<CBasePlayer*>(CBaseEntity::Instance(pev->owner));

	if (owner == nullptr)
	{
		Remove();
		return;
	}

	SetOrigin(owner->pev->origin);

	if (!owner->IsAlive())
	{
		Throw(kDrop);
		return;
	}

	if (!owner->IsPlayer())
	{
		Remove();
		return;
	}

	if ((owner->m_TFState & kTFStateGrenadeThrowing) == 0)
	{
		if (pev->dmgtime <= gpGlobals->time)
		{
			Throw(kOvercook);
			return;
		}
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	Throw(kThrow);
}


/* Toodles TODO: Point contents doesn't catch both "no grenades" and "no build". */
bool CPrimeGrenade::CanDetonate()
{
	const auto contents = g_engfuncs.pfnPointContents(pev->origin);

	if (contents != CONTENTS_SOLID
	 && contents != CONTENTS_SKY
	 && contents != CONTENTS_NO_GRENADES)
	{
		return true;
	}

	if (contents == CONTENTS_NO_GRENADES && !FStringNull(pev->model))
	{
		Vector origin = pev->origin + Vector(0.0F, 0.0F, 4.0F);

		MessageBegin(MSG_PVS, SVC_TEMPENTITY, origin);
		WriteByte(TE_SPRITE);
		WriteCoord(origin.x);
		WriteCoord(origin.y);
		WriteCoord(origin.z);
		WriteShort(g_sModelIndexFlare);
		WriteByte(3);
		WriteByte(255);
		MessageEnd();
	}

	return false;
}


void CPrimeGrenade::Explode(TraceResult* pTrace, int bitsDamageType)
{
	// Pull out of the wall a bit
	if (pTrace->flFraction != 1.0)
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * 0.6);
	}

	tent::Explosion(pev->origin, -pTrace->vecPlaneNormal, tent::ExplosionType::Normal, pev->dmg);

	CBaseEntity* owner;
	if (pev->owner)
		owner = CBaseEntity::Instance(pev->owner);
	else
		owner = this;

	pev->owner = nullptr; // can't traceline attack owner if this is set

	RadiusDamage(pev->origin, this, owner, pev->dmg, pev->dmg_save, pev->dmg_take, bitsDamageType);

	Remove();
}


void CPrimeGrenade::Throw(throw_e mode)
{
	auto owner = dynamic_cast<CBasePlayer*>(CBaseEntity::Instance(pev->owner));

	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SetOrigin(owner->pev->origin);
	pev->angles = Vector(0, owner->pev->angles.y, 0);

	if (mode == kOvercook)
	{
		pev->velocity = g_vecZero;

		SetThink(&CPrimeGrenade::Detonate);
		pev->nextthink = gpGlobals->time;
	}
	else
	{
		pev->gravity = 0.81;
		pev->friction = 0.6;

		SetModel(GetModelName());
		SetSize(g_vecZero, g_vecZero);

		if (mode == kDrop)
		{
			pev->velocity = Vector(0, 0, 30);
		}
		else
		{
			util::MakeVectors(owner->pev->v_angle);
			pev->velocity = gpGlobals->v_forward * 600 + gpGlobals->v_up * 200;

			EmitSound("weapons/grenade.wav", CHAN_BODY, VOL_NORM, ATTN_IDLE);
		}

		SetTouch(&CPrimeGrenade::BounceTouch);
		SetThink(&CPrimeGrenade::TumbleThink);
		pev->nextthink = gpGlobals->time + 0.1;

		pev->sequence = g_engfuncs.pfnRandomLong(3, 6);
		pev->framerate = 1.0;
		ResetSequenceInfo();
	}

	MessageBegin(MSG_ONE, gmsgStatusIcon, owner);
	WriteByte(0);
	WriteString(GetIconName());
	MessageEnd();
	
	owner->m_TFState &= ~(kTFStateGrenadePrime | kTFStateGrenadeThrowing);
}


CPrimeGrenade* CPrimeGrenade::PrimeGrenade(CBaseEntity* owner)
{
	auto grenade = GetClassPtr((CPrimeGrenade*)nullptr);

	grenade->pev->owner = owner->edict();
	grenade->Spawn();

	return grenade;
}


bool CCaltropCanister::Spawn()
{
	if (CPrimeGrenade::Spawn())
	{
		pev->nextthink = gpGlobals->time + 0.5;
		pev->dmgtime = gpGlobals->time + 0.5;
		return true;
	}
	return false;
}


CCaltropCanister* CCaltropCanister::CaltropCanister(CBaseEntity* owner)
{
	auto grenade = GetClassPtr((CCaltropCanister*)nullptr);

	grenade->pev->owner = owner->edict();
	grenade->Spawn();

	return grenade;
}


void CCaltropCanister::Throw(throw_e mode)
{
	auto owner = dynamic_cast<CBasePlayer*>(CBaseEntity::Instance(pev->owner));

	SetOrigin(owner->pev->origin);
	pev->angles = Vector(0, owner->pev->angles.y, 0);

	if (CanDetonate())
	{
		for (int i = 0; i < kNumCaltrops; i++)
		{
			CCaltrop::Caltrop(
				owner,
				pev->origin,
				pev->angles.y + (360 / static_cast<float>(kNumCaltrops)) * i);
		}
	}

	MessageBegin(MSG_ONE, gmsgStatusIcon, owner);
	WriteByte(0);
	WriteString(GetIconName());
	MessageEnd();
	
	owner->m_TFState &= ~(kTFStateGrenadePrime | kTFStateGrenadeThrowing);

	Remove();
}


bool CCaltrop::Spawn()
{
	pev->classname = MAKE_STRING("caltrop");
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	SetOrigin(pev->origin);
	pev->angles = Vector(0, pev->angles.y, 0);

	pev->gravity = 0.5;
	pev->friction = 0.5;

	SetModel(GetModelName());
	SetSize(g_vecZero, g_vecZero);

	util::MakeVectors(pev->angles);
	pev->velocity = gpGlobals->v_forward * 100 + gpGlobals->v_up * 200;
	pev->avelocity = Vector(400, 400, 400);

	pev->health = 10;
	SetThink(&CCaltrop::CaltropThink);
	pev->nextthink = gpGlobals->time + 0.2;

	pev->dmg = 10;

	return true;
}


void CCaltrop::CaltropThink()
{
	if (pev->velocity == g_vecZero || (pev->flags & FL_ONGROUND) != 0)
	{
		EmitSound("weapons/tink1.wav", CHAN_AUTO, VOL_NORM, ATTN_IDLE);

		pev->angles.x = pev->angles.z = 0;
		pev->avelocity = g_vecZero;

		SetTouch(&CCaltrop::CaltropTouch);
		SetThink(&CBaseEntity::Remove);
		pev->nextthink = gpGlobals->time + 15;

		SetOrigin(pev->origin);
	}
	else
	{
		pev->health--;

		if (pev->health < 0)
		{
			Remove();
			return;
		}

		pev->nextthink = gpGlobals->time + 0.2;
	}
}

void CCaltrop::CaltropTouch(CBaseEntity* other)
{
	if (!other->IsPlayer() || !other->IsAlive())
	{
		return;
	}

	if (other->TakeDamage(this, CBaseEntity::Instance(pev->owner), pev->dmg, DMG_CALTROP))
	{
		CBasePlayer* player = dynamic_cast<CBasePlayer*>(other);

		player->m_nLegDamage = std::min(player->m_nLegDamage + 1, 6);
		MessageBegin(MSG_ONE, gmsgStatusIcon, player);
		WriteByte(2);
		WriteString("dmg_caltrop");
		MessageEnd();
	}

	Remove();
}


CCaltrop* CCaltrop::Caltrop(CBaseEntity* owner, const Vector& origin, const float yaw)
{
	auto grenade = GetClassPtr((CCaltrop*)nullptr);

	grenade->pev->owner = owner->edict();
	grenade->pev->origin = origin;
	grenade->pev->angles = Vector(0, yaw, 0);
	grenade->Spawn();

	return grenade;
}


void CConcussionGrenade::Explode(TraceResult* pTrace, int bitsDamageType)
{
	if (pTrace->flFraction != 1.0)
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * 0.6);
	}

	tent::Explosion(pev->origin, -pTrace->vecPlaneNormal, tent::ExplosionType::Concussion, pev->dmg);

	CBaseEntity* owner = this;
	if (pev->owner)
	{
		owner = CBaseEntity::Instance(pev->owner);
	}

	pev->owner = nullptr;

	CBaseEntity* entity = nullptr;
	TraceResult tr;
	Vector difference;
	float length;
	float ajdusted;

	while ((entity = util::FindEntityInSphere(entity, pev->origin, 280)) != nullptr)
	{
		if (!entity->IsPlayer())
		{
			continue;
		}

		util::TraceLine(pev->origin, entity->EyePosition(), &tr, owner, util::kTraceBox);

		if (tr.flFraction != 1.0F && tr.pHit != entity->edict())
		{
			continue;
		}

		difference = entity->pev->origin - pev->origin;

		length = difference.Length();

		if (length < 16.0F)
		{
			difference = entity->pev->velocity * 0.33F;
		}

		entity->pev->velocity = difference;

		ajdusted = (240 - length * 0.5F) * 0.03F;

		entity->pev->velocity.x *= ajdusted;
		entity->pev->velocity.y *= ajdusted;
		entity->pev->velocity.z *= ajdusted * 1.5F;

		if ((entity == owner && (!FStringNull(pev->model) || (owner->pev->flags & FL_ONGROUND) != 0))
		 || (g_pGameRules->PlayerRelationship(entity, owner) < GR_ALLY))
		{
			dynamic_cast<CBasePlayer*>(entity)->BecomeConcussed(owner);
		}
	}

	Remove();
}


CConcussionGrenade* CConcussionGrenade::ConcussionGrenade(CBaseEntity* owner)
{
	auto grenade = GetClassPtr((CConcussionGrenade*)nullptr);

	grenade->pev->owner = owner->edict();
	grenade->Spawn();

	return grenade;
}


void CFlare::StickyTouch(CBaseEntity* pOther)
{
	BounceTouch(pOther);

	if (util::GetGlobalTrace().vecPlaneNormal.z >= kGroundPlaneMinZ)
	{
		pev->velocity = g_vecZero;
		pev->movetype = MOVETYPE_TOSS;
		SetTouch(nullptr);
	}
}


void CFlare::Explode(TraceResult* pTrace, int bitsDamageType)
{
	pev->effects |= EF_LIGHT;
	SetThink(&CFlare::Remove);
	pev->nextthink = gpGlobals->time + 40.0F;
}


bool CFlare::Spawn()
{
	if (CPrimeGrenade::Spawn())
	{
		pev->nextthink = gpGlobals->time + 0.5;
		pev->dmgtime = gpGlobals->time + 0.5;
		return true;
	}
	return false;
}


CFlare* CFlare::Flare(CBaseEntity* owner)
{
	auto grenade = GetClassPtr((CFlare*)nullptr);

	grenade->pev->owner = owner->edict();
	grenade->Spawn();

	return grenade;
}


void CFlare::Throw(throw_e mode)
{
	if (mode == kOvercook)
	{
		mode = kThrow;
	}

	CPrimeGrenade::Throw(mode);
	
	SetTouch(&CFlare::StickyTouch);
}


bool CNailGrenade::Spawn()
{
	pev->health = kNumBursts;
	return CPrimeGrenade::Spawn();
}


void CNailGrenade::Explode(TraceResult* pTrace, int bitsDamageType)
{
	if (pev->health <= 0)
	{
		CPrimeGrenade::Explode(pTrace, bitsDamageType);
	}
	else
	{
		SetModel(GetModelName());
		SetSize(g_vecZero, g_vecZero);

		pev->oldorigin = pev->origin;
		pev->velocity = Vector(0, 0, 272);
		pev->avelocity = Vector(0, 500, 0);

		if ((pev->flags & FL_ONGROUND) != 0)
		{
			BounceSound();
		}

		SetThink(&CNailGrenade::GetReadyToNail);
		pev->nextthink = gpGlobals->time + 0.7F;
	}
}


void CNailGrenade::GetReadyToNail()
{
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;

	SetOrigin(pev->oldorigin + Vector(0, 0, 32));
	pev->velocity = g_vecZero;

	SetThink(&CNailGrenade::GetNailedIdiot);
	pev->nextthink = gpGlobals->time + 0.1F;
}


void CNailGrenade::GetNailedIdiot()
{
	auto owner = CBaseEntity::Instance(pev->owner);

	for (int i = 0; i < kNumNails; i++)
	{
		Vector angles =
			Vector(
				0,
				(360 / static_cast<float>(kNumBursts)) * pev->health
					+ (360 / static_cast<float>(kNumNails)) * i,
				0);

		Vector dir;
		util::MakeVectorsPrivate(angles, dir, nullptr, nullptr);

		CNail::CreateNailGrenadeNail(
			pev->origin + dir * 12,
			dir,
			18,
			owner
		);
	}

	g_engfuncs.pfnPlaybackEvent(
		FEV_GLOBAL | FEV_RELIABLE,
		owner->edict(),
		g_usGetNailedIdiot,
		0.0F,
		pev->origin,
		pev->angles,
		0.0F,
		0.0F,
		pev->health,
		0,
		false,
		false
	);

	pev->health--;

	if (pev->health <= 0)
	{
		SetThink(&CNailGrenade::Detonate);
	}

	pev->nextthink = gpGlobals->time + 0.1F;
}


CNailGrenade* CNailGrenade::NailGrenade(CBaseEntity* owner)
{
	auto grenade = GetClassPtr((CNailGrenade*)nullptr);

	grenade->pev->owner = owner->edict();
	grenade->Spawn();

	grenade->pev->classname = MAKE_STRING("nailgrenade");

	return grenade;
}


void CMirv::Explode(TraceResult* pTrace, int bitsDamageType)
{
	auto owner = CBaseEntity::Instance(pev->owner);

	CPrimeGrenade::Explode(pTrace, bitsDamageType);

	for (int i = 0; i < kNumBomblets; i++)
	{
		CBomblet::Bomblet(
			owner,
			pev->origin,
			pev->angles.y + (360 / static_cast<float>(kNumBomblets)) * i);
	}
}


CMirv* CMirv::Mirv(CBaseEntity* owner)
{
	auto grenade = GetClassPtr((CMirv*)nullptr);

	grenade->pev->owner = owner->edict();
	grenade->Spawn();

	grenade->pev->classname = MAKE_STRING("mirvgrenade");

	return grenade;
}


bool CBomblet::Spawn()
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_TRIGGER;

	pev->angles = Vector(0, pev->angles.y, 0);

	util::MakeVectors(pev->angles);
	SetOrigin(pev->origin + gpGlobals->v_forward * 2 + Vector(0.0F, 0.0F, 2.0F));

	pev->gravity = 0.5;
	pev->friction = 0.8;

	SetModel(GetModelName());
	SetSize(g_vecZero, g_vecZero);

	pev->velocity = gpGlobals->v_forward * 75 + gpGlobals->v_up * 200;

	SetTouch(&CPrimeGrenade::BounceTouch);
	SetThink(&CPrimeGrenade::TumbleThink);
	pev->nextthink = gpGlobals->time + 0.1;
	pev->dmgtime = pev->nextthink + 2.5;

	pev->sequence = g_engfuncs.pfnRandomLong(3, 6);
	pev->framerate = 1.0;
	ResetSequenceInfo();

	pev->dmg = 180;
	pev->dmg_save = 0;
	pev->dmg_take = 360;

	return true;
}


CBomblet* CBomblet::Bomblet(CBaseEntity* owner, const Vector& origin, const float yaw)
{
	auto grenade = GetClassPtr((CBomblet*)nullptr);

	grenade->pev->owner = owner->edict();
	grenade->pev->origin = origin;
	grenade->pev->angles = Vector(0, yaw, 0);
	grenade->Spawn();

	grenade->pev->classname = MAKE_STRING("mirvgrenade");

	return grenade;
}

#endif
