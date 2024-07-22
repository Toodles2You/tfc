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
	util::TraceLine(v.origin, v.origin + Vector(0, 0, -32), util::ignore_monsters, this, &tr);

	Explode(&tr, DMG_BLAST);
}

void CGrenade::Explode(TraceResult* pTrace, int bitsDamageType)
{
	// Pull out of the wall a bit
	if (pTrace->flFraction != 1.0)
	{
		v.origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * 0.6);
	}

	tent::Explosion(v.origin, -pTrace->vecPlaneNormal, tent::ExplosionType::Normal, v.dmg);

	CBaseEntity* owner;
	if (v.owner)
		owner = v.owner->Get<CBaseEntity>();
	else
		owner = this;

	v.owner = nullptr; // can't traceline attack owner if this is set

	RadiusDamage(v.origin, this, owner, v.dmg, v.dmg_save, v.dmg_take, DMG_RESIST_SELF | bitsDamageType);

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

	vecSpot = v.origin + Vector(0, 0, 8);
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

	v.enemy = &pOther->v;

	vecSpot = v.origin - v.velocity.Normalize() * 32;
	util::TraceLine(vecSpot, vecSpot + v.velocity.Normalize() * 64, util::ignore_monsters, this, &tr);

	Explode(&tr, DMG_BLAST);
}


void CGrenade::BounceTouch(CBaseEntity* pOther)
{
	// don't hit the guy that launched this grenade
	if (&pOther->v == v.owner)
		return;

	if ((v.flags & FL_ONGROUND) != 0)
	{
		// add a bit of static friction
		v.velocity = v.velocity * 0.8;

		v.sequence = engine::RandomLong(1, 1);
		ResetSequenceInfo();
	}
	else
	{
		if (v.velocity.z > 0
		 && util::GetGlobalTrace().vecPlaneNormal.z >= kGroundPlaneMinZ)
		{
			v.velocity = v.velocity * 0.8;
		}

		// play bounce sound
		BounceSound();
	}
	v.framerate = v.velocity.Length() / 200.0;
	if (v.framerate > 1.0)
		v.framerate = 1;
	else if (v.framerate < 0.5)
		v.framerate = 0;
}


void CGrenade::BounceSound()
{
	if (v.radsuit_finished > gpGlobals->time)
	{
		return;
	}

	v.radsuit_finished = gpGlobals->time + (1.0F / 15.0F);

	switch (engine::RandomLong(0, 2))
	{
	case 0: EmitSound("weapons/grenade_hit1.wav", CHAN_VOICE, 0.25F); break;
	case 1: EmitSound("weapons/grenade_hit2.wav", CHAN_VOICE, 0.25F); break;
	case 2: EmitSound("weapons/grenade_hit3.wav", CHAN_VOICE, 0.25F); break;
	}
}

void CGrenade::TumbleThink()
{
	StudioFrameAdvance();
	v.nextthink = gpGlobals->time + 0.1;

	if (v.dmgtime <= gpGlobals->time)
	{
		SetThink(&CGrenade::Detonate);
	}
	if (v.waterlevel > kWaterLevelNone)
	{
		v.velocity = v.velocity * 0.5;
		v.framerate = 0.2;
	}
	else
	{
		v.framerate = 1.0;
	}
}


bool CGrenade::Spawn()
{
	v.movetype = MOVETYPE_BOUNCE;
	v.classname = MAKE_STRING("grenade");

	v.solid = SOLID_BBOX;

	SetModel("models/grenade.mdl");
	SetSize(g_vecZero, g_vecZero);

	v.dmg = 100;
	v.dmg_save = 0;
	v.dmg_take = 250;

	return true;
}


CGrenade* CGrenade::ShootContact(CBaseEntity* owner, Vector vecStart, Vector vecVelocity)
{
	CGrenade* pGrenade = Entity::Create<CGrenade>();
	pGrenade->Spawn();
	// contact grenades arc lower
	pGrenade->v.gravity = 0.5; // lower gravity since grenade is aerodynamic and engine doesn't know it.
	pGrenade->SetOrigin(vecStart);
	pGrenade->v.velocity = vecVelocity;
	pGrenade->v.angles = util::VecToAngles(pGrenade->v.velocity);
	pGrenade->v.owner = &owner->v;
	pGrenade->v.team = owner->TeamNumber();

	// Tumble in air
	pGrenade->v.avelocity.x = engine::RandomFloat(-100, -500);

	// Explode on contact
	pGrenade->SetTouch(&CGrenade::ExplodeTouch);

	pGrenade->v.dmg = 100;
	pGrenade->v.dmg_save = 0;
	pGrenade->v.dmg_take = 250;

	return pGrenade;
}


bool CGrenade::ShouldCollide(CBaseEntity* other)
{
	if (!other->IsPlayer())
	{
		return CBaseAnimating::ShouldCollide(other);
	}

	/* Don't collide with teammates for the first moments of flight. */
	return gpGlobals->time - v.air_finished >= 0.5F
		|| g_pGameRules->FPlayerCanTakeDamage(
			static_cast<CBasePlayer*>(other),
			v.owner->Get<CBasePlayer>(),
			this);
}


#ifdef HALFLIFE_GRENADES

bool CPrimeGrenade::Spawn()
{
	v.classname = MAKE_STRING("grenade");
	v.movetype = MOVETYPE_NONE;
	v.solid = SOLID_NOT;

	SetSize(g_vecZero, g_vecZero);

	SetOrigin(v.owner->origin);

	SetThink(&CPrimeGrenade::PrimedThink);
	v.nextthink = gpGlobals->time + 0.8;
	v.dmgtime = v.nextthink + 3.0;

	v.dmg = 180;
	v.dmg_save = 0;
	v.dmg_take = 360;

	return true;
}


void CPrimeGrenade::PrimedThink()
{
	auto owner = v.owner->Get<CBasePlayer>();

	if (owner == nullptr)
	{
		Remove();
		return;
	}

	SetOrigin(owner->v.origin);

	if (!owner->IsAlive() || owner->InState(CBasePlayer::State::FeigningDeath))
	{
		Throw(kDrop);
		return;
	}

	if (!owner->IsPlayer())
	{
		Remove();
		return;
	}

	if (!owner->InState(CBasePlayer::State::GrenadeThrowing))
	{
		if (v.dmgtime <= gpGlobals->time)
		{
			Throw(kOvercook);
			return;
		}
		v.nextthink = gpGlobals->time + 0.1;
		return;
	}

	Throw(kThrow);
}


/* Toodles TODO: Point contents doesn't catch both "no grenades" and "no build". */
bool CPrimeGrenade::CanDetonate()
{
	const auto contents = engine::PointContents(v.origin);

	if (contents != CONTENTS_SOLID
	 && contents != CONTENTS_SKY
	 && contents != CONTENTS_NO_GRENADES)
	{
		return true;
	}

	if (contents == CONTENTS_NO_GRENADES && !FStringNull(v.model))
	{
		Vector origin = v.origin + Vector(0.0F, 0.0F, 4.0F);

		MessageBegin(MSG_PVS, SVC_TEMPENTITY, origin);
		WriteByte(TE_SPRITE);
		WriteCoord(origin);
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
		v.origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * 0.6);
	}

	tent::Explosion(v.origin, -pTrace->vecPlaneNormal, tent::ExplosionType::Normal, v.dmg);

	CBaseEntity* owner;
	if (v.owner)
		owner = v.owner->Get<CBasePlayer>();
	else
		owner = this;

	v.owner = nullptr; // can't traceline attack owner if this is set

	RadiusDamage(v.origin, this, owner, v.dmg, v.dmg_save, v.dmg_take, bitsDamageType);

	Remove();
}


void CPrimeGrenade::Throw(throw_e mode)
{
	auto owner = v.owner->Get<CBasePlayer>();

	v.movetype = MOVETYPE_BOUNCE;
	v.solid = SOLID_BBOX;

	SetOrigin(owner->v.origin);
	v.angles = Vector(0, owner->v.angles.y, 0);

	if (mode == kOvercook)
	{
		v.velocity = g_vecZero;

		SetThink(&CPrimeGrenade::Detonate);
		v.nextthink = gpGlobals->time;
	}
	else
	{
		v.gravity = 0.81;
		v.friction = 0.6;

		SetModel(GetModelName());
		SetSize(g_vecZero, g_vecZero);

		if (mode == kDrop)
		{
			v.velocity = Vector(0, 0, 30);
		}
		else
		{
			util::MakeVectors(owner->v.v_angle);
			v.velocity = gpGlobals->v_forward * 600 + gpGlobals->v_up * 200;

			EmitSound("weapons/grenade.wav", CHAN_BODY, VOL_NORM, ATTN_IDLE);
		}

		SetTouch(&CPrimeGrenade::BounceTouch);
		SetThink(&CPrimeGrenade::TumbleThink);
		v.nextthink = gpGlobals->time + 0.1;

		v.sequence = engine::RandomLong(3, 6);
		v.framerate = 1.0;
		ResetSequenceInfo();

		util::LagCompensation(this, owner->m_netPing);
	}

	v.air_finished = gpGlobals->time;

	MessageBegin(MSG_ONE, gmsgStatusIcon, owner);
	WriteByte(0);
	WriteString(GetIconName());
	MessageEnd();
	
	owner->LeaveState(CBasePlayer::State::Grenade);
	owner->m_iGrenadeExplodeTime = 0;
	owner->m_hGrenade = nullptr;
}


CPrimeGrenade* CPrimeGrenade::PrimeGrenade(CBaseEntity* owner)
{
	auto grenade = Entity::Create<CPrimeGrenade>();

	grenade->v.owner = &owner->v;
	grenade->v.team = owner->TeamNumber();
	grenade->Spawn();

	return grenade;
}


bool CCaltropCanister::Spawn()
{
	if (CPrimeGrenade::Spawn())
	{
		v.nextthink = gpGlobals->time + 0.5;
		v.dmgtime = gpGlobals->time + 0.5;
		return true;
	}
	return false;
}


CCaltropCanister* CCaltropCanister::CaltropCanister(CBaseEntity* owner)
{
	auto grenade = Entity::Create<CCaltropCanister>();

	grenade->v.owner = &owner->v;
	grenade->v.team = owner->TeamNumber();
	grenade->Spawn();

	return grenade;
}


void CCaltropCanister::Throw(throw_e mode)
{
	auto owner = v.owner->Get<CBasePlayer>();

	SetOrigin(owner->v.origin);
	v.angles = Vector(0, owner->v.angles.y, 0);

	if (CanDetonate())
	{
		for (int i = 0; i < kNumCaltrops; i++)
		{
			CCaltrop::Caltrop(
				owner,
				v.origin,
				v.angles.y + (360 / static_cast<float>(kNumCaltrops)) * i);
		}
	}

	MessageBegin(MSG_ONE, gmsgStatusIcon, owner);
	WriteByte(0);
	WriteString(GetIconName());
	MessageEnd();
	
	owner->LeaveState(CBasePlayer::State::Grenade);
	owner->m_iGrenadeExplodeTime = 0;
	owner->m_hGrenade = nullptr;

	Remove();
}


bool CCaltrop::Spawn()
{
	v.classname = MAKE_STRING("caltrop");
	v.movetype = MOVETYPE_TOSS;
	v.solid = SOLID_TRIGGER;

	SetOrigin(v.origin);
	v.angles = Vector(0, v.angles.y, 0);

	v.gravity = 0.5;
	v.friction = 0.5;

	SetModel(GetModelName());
	SetSize(g_vecZero, g_vecZero);

	util::MakeVectors(v.angles);
	v.velocity = gpGlobals->v_forward * 100 + gpGlobals->v_up * 200;
	v.avelocity = Vector(400, 400, 400);

	v.health = 10;
	SetThink(&CCaltrop::CaltropThink);
	v.nextthink = gpGlobals->time + 0.2;

	v.dmg = 10;

	return true;
}


void CCaltrop::CaltropThink()
{
	if (v.velocity == g_vecZero || (v.flags & FL_ONGROUND) != 0)
	{
		EmitSound("weapons/tink1.wav", CHAN_AUTO, VOL_NORM, ATTN_IDLE);

		v.angles.x = v.angles.z = 0;
		v.avelocity = g_vecZero;

		SetTouch(&CCaltrop::CaltropTouch);
		SetThink(&CBaseEntity::Remove);
		v.nextthink = gpGlobals->time + 15;

		SetOrigin(v.origin);
	}
	else
	{
		v.health--;

		if (v.health < 0)
		{
			Remove();
			return;
		}

		v.nextthink = gpGlobals->time + 0.2;
	}
}

void CCaltrop::CaltropTouch(CBaseEntity* other)
{
	if (!other->IsPlayer() || !other->IsAlive())
	{
		return;
	}

	if (other->TakeDamage(this, v.owner->Get<CBasePlayer>(), v.dmg, DMG_CALTROP))
	{
		auto player = static_cast<CBasePlayer*>(other);

		player->m_nLegDamage = std::min(player->m_nLegDamage + 1, 6);
		MessageBegin(MSG_ONE, gmsgStatusIcon, player);
		WriteByte(2);
		WriteString("dmg_caltrop");
		MessageEnd();
	}

	Remove();
}


bool CCaltrop::ShouldCollide(CBaseEntity* other)
{
	if (!other->IsPlayer())
	{
		return CPrimeGrenade::ShouldCollide(other);
	}

	return g_pGameRules->FPlayerCanTakeDamage(
		dynamic_cast<CBasePlayer*>(other),
		v.owner->Get<CBasePlayer>(),
		this);
}


CCaltrop* CCaltrop::Caltrop(CBaseEntity* owner, const Vector& origin, const float yaw)
{
	auto grenade = Entity::Create<CCaltrop>();

	grenade->v.owner = &owner->v;
	grenade->v.team = owner->TeamNumber();
	grenade->v.origin = origin;
	grenade->v.angles = Vector(0, yaw, 0);
	grenade->Spawn();

	return grenade;
}


void CConcussionGrenade::Explode(TraceResult* pTrace, int bitsDamageType)
{
	if (pTrace->flFraction != 1.0)
	{
		v.origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * 0.6);
	}

	CBaseEntity* owner = this;
	CBaseEntity* predictionOwner = nullptr;
	if (v.owner)
	{
		owner = v.owner->Get<CBasePlayer>();

		if (FStringNull(v.model))
		{
			predictionOwner = owner;
		}
	}

	v.owner = nullptr;

	tent::Explosion(v.origin, -pTrace->vecPlaneNormal, tent::ExplosionType::Concussion, v.dmg, true, true, predictionOwner);

	CBaseEntity* entity = nullptr;
	TraceResult tr;
	Vector difference;
	float length;
	float ajdusted;

	while ((entity = util::FindEntityInSphere(entity, v.origin, 280)) != nullptr)
	{
		if (!entity->IsPlayer()
		 || entity == predictionOwner)
		{
			continue;
		}

		util::TraceLine(v.origin, entity->EyePosition(), &tr, owner, util::kTraceBox);

		if (tr.flFraction != 1.0F && tr.pHit != &entity->v)
		{
			continue;
		}

		difference = entity->v.origin - v.origin;

		length = difference.Length();

		if (length < 16.0F)
		{
			difference = entity->v.velocity * 0.33F;
		}

		entity->v.velocity = difference;

		ajdusted = (240 - length * 0.5F) * 0.03F;

		entity->v.velocity.x *= ajdusted;
		entity->v.velocity.y *= ajdusted;
		entity->v.velocity.z *= ajdusted * 1.5F;

		if (entity == owner || g_pGameRules->PlayerRelationship(entity, this) < GR_ALLY)
		{
			dynamic_cast<CBasePlayer*>(entity)->BecomeConcussed(owner);
		}
	}

	Remove();
}


CConcussionGrenade* CConcussionGrenade::ConcussionGrenade(CBaseEntity* owner)
{
	auto grenade = Entity::Create<CConcussionGrenade>();

	grenade->v.owner = &owner->v;
	grenade->v.team = owner->TeamNumber();
	grenade->Spawn();

	return grenade;
}


void CFlare::StickyTouch(CBaseEntity* pOther)
{
	BounceTouch(pOther);

	if (util::GetGlobalTrace().vecPlaneNormal.z >= kGroundPlaneMinZ)
	{
		v.velocity = g_vecZero;
		v.movetype = MOVETYPE_TOSS;
		ClearTouch();
	}
}


void CFlare::Explode(TraceResult* pTrace, int bitsDamageType)
{
	v.effects |= EF_LIGHT;
	SetThink(&CFlare::Remove);
	v.nextthink = gpGlobals->time + 40.0F;
}


bool CFlare::Spawn()
{
	if (CPrimeGrenade::Spawn())
	{
		v.nextthink = gpGlobals->time + 0.5;
		v.dmgtime = gpGlobals->time + 0.5;
		return true;
	}
	return false;
}


CFlare* CFlare::Flare(CBaseEntity* owner)
{
	auto grenade = Entity::Create<CFlare>();

	grenade->v.owner = &owner->v;
	grenade->v.team = owner->TeamNumber();
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
	v.health = kNumBursts;
	return CPrimeGrenade::Spawn();
}


void CNailGrenade::Explode(TraceResult* pTrace, int bitsDamageType)
{
	if (v.health <= 0)
	{
		CPrimeGrenade::Explode(pTrace, bitsDamageType);
	}
	else
	{
		SetModel(GetModelName());
		SetSize(g_vecZero, g_vecZero);

		v.oldorigin = v.origin;
		v.velocity = Vector(0, 0, 272);
		v.avelocity = Vector(0, 500, 0);

		if ((v.flags & FL_ONGROUND) != 0)
		{
			BounceSound();
		}

		SetThink(&CNailGrenade::GetReadyToNail);
		v.nextthink = gpGlobals->time + 0.7F;
	}
}


void CNailGrenade::GetReadyToNail()
{
	v.movetype = MOVETYPE_FLY;
	v.solid = SOLID_NOT;

	SetOrigin(v.oldorigin + Vector(0, 0, 32));
	v.velocity = g_vecZero;

	SetThink(&CNailGrenade::GetNailedIdiot);
	v.nextthink = gpGlobals->time + 0.1F;
}


void CNailGrenade::GetNailedIdiot()
{
	auto owner = v.owner->Get<CBasePlayer>();

	for (int i = 0; i < kNumNails; i++)
	{
		Vector angles =
			Vector(
				0,
				(360 / static_cast<float>(kNumBursts)) * v.health
					+ (360 / static_cast<float>(kNumNails)) * i,
				0);

		Vector dir;
		util::MakeVectorsPrivate(angles, dir, nullptr, nullptr);

		CNail::CreateNailGrenadeNail(
			v.origin + dir * 12,
			dir,
			18,
			owner
		);
	}

	engine::PlaybackEvent(
		FEV_GLOBAL | FEV_RELIABLE,
		&owner->v,
		g_usGetNailedIdiot,
		0.0F,
		v.origin,
		v.angles,
		0.0F,
		0.0F,
		v.health,
		0,
		false,
		false
	);

	v.health--;

	if (v.health <= 0)
	{
		SetThink(&CNailGrenade::Detonate);
	}

	v.nextthink = gpGlobals->time + 0.1F;
}


CNailGrenade* CNailGrenade::NailGrenade(CBaseEntity* owner)
{
	auto grenade = Entity::Create<CNailGrenade>();

	grenade->v.owner = &owner->v;
	grenade->v.team = owner->TeamNumber();
	grenade->Spawn();

	grenade->v.classname = MAKE_STRING("nailgrenade");

	return grenade;
}


void CMirv::Explode(TraceResult* pTrace, int bitsDamageType)
{
	auto owner = v.owner->Get<CBasePlayer>();

	CPrimeGrenade::Explode(pTrace, bitsDamageType);

	for (int i = 0; i < kNumBomblets; i++)
	{
		CBomblet::Bomblet(
			owner,
			v.origin,
			v.angles.y + (360 / static_cast<float>(kNumBomblets)) * i);
	}
}


CMirv* CMirv::Mirv(CBaseEntity* owner)
{
	auto grenade = Entity::Create<CMirv>();

	grenade->v.owner = &owner->v;
	grenade->v.team = owner->TeamNumber();
	grenade->Spawn();

	grenade->v.classname = MAKE_STRING("mirvgrenade");

	return grenade;
}


bool CBomblet::Spawn()
{
	v.movetype = MOVETYPE_BOUNCE;
	v.solid = SOLID_TRIGGER;

	v.angles = Vector(0, v.angles.y, 0);

	util::MakeVectors(v.angles);
	SetOrigin(v.origin + gpGlobals->v_forward * 2 + Vector(0.0F, 0.0F, 2.0F));

	v.gravity = 0.5;
	v.friction = 0.8;

	SetModel(GetModelName());
	SetSize(g_vecZero, g_vecZero);

	v.velocity = gpGlobals->v_forward * 75 + gpGlobals->v_up * 200;

	SetTouch(&CPrimeGrenade::BounceTouch);
	SetThink(&CPrimeGrenade::TumbleThink);
	v.nextthink = gpGlobals->time + 0.1;
	v.dmgtime = v.nextthink + 2.5;

	v.sequence = engine::RandomLong(3, 6);
	v.framerate = 1.0;
	ResetSequenceInfo();

	v.dmg = 180;
	v.dmg_save = 0;
	v.dmg_take = 360;

	return true;
}


bool CBomblet::ShouldCollide(CBaseEntity* other)
{
	if (!other->IsPlayer())
	{
		return CPrimeGrenade::ShouldCollide(other);
	}

	return g_pGameRules->FPlayerCanTakeDamage(
		static_cast<CBasePlayer*>(other),
		v.owner->Get<CBasePlayer>(),
		this);
}


CBomblet* CBomblet::Bomblet(CBaseEntity* owner, const Vector& origin, const float yaw)
{
	auto grenade = Entity::Create<CBomblet>();

	grenade->v.owner = &owner->v;
	grenade->v.team = owner->TeamNumber();
	grenade->v.origin = origin;
	grenade->v.angles = Vector(0, yaw, 0);
	grenade->Spawn();

	grenade->v.classname = MAKE_STRING("mirvgrenade");

	return grenade;
}


bool CNapalmGrenade::Spawn()
{
	v.health = kNumBursts;

	if (!CPrimeGrenade::Spawn())
	{
		return false;
	}

	v.dmg = 10;
	v.dmg_save = 10;
	v.dmg_take = 180;

	return true;
}


void CNapalmGrenade::Explode(TraceResult* pTrace, int bitsDamageType)
{
	if (v.health <= 0)
	{
		Remove();
	}
	else
	{
		v.gravity = 0.81;
		v.friction = 0.6;

		SetModel(GetModelName());
		SetSize(g_vecZero, g_vecZero);

		EmitSound("weapons/flmgrexp.wav", CHAN_AUTO, VOL_NORM, 0.2F);

		SetThink(&CNapalmGrenade::NapalmThink);
		v.nextthink = gpGlobals->time + 0.1F;
	}
}


void CNapalmGrenade::NapalmThink()
{
	if (!CanDetonate())
	{
		Remove();
		return;
	}

	const auto contents = engine::PointContents(v.origin);

	/* Toodles TODO: There should probably be some sort of effect here. */
	if (contents == CONTENTS_WATER)
	{
		Remove();
		return;
	}

	tent::FireField(v.origin);

	CBaseEntity* entity = nullptr;
	TraceResult tr;

	while ((entity = util::FindEntityInSphere(entity, v.origin, v.dmg_take)) != nullptr)
	{
		if (entity->v.takedamage == DAMAGE_NO)
		{
			continue;
		}

		util::TraceLine(v.origin, entity->EyePosition(), &tr, this, util::kTraceBox);

		if (tr.flFraction != 1.0F && tr.pHit != &entity->v)
		{
			continue;
		}

		entity->TakeDamage(this, v.owner->Get<CBasePlayer>(), v.dmg, DMG_BURN | DMG_IGNITE);
	}

	v.health--;

	if (v.health <= 0)
	{
		Remove();
		return;
	}

	/* Toodles TODO: Make this a looping sound. */
	EmitSound("ambience/fire1.wav", CHAN_WEAPON, VOL_NORM, ATTN_NORM);

	v.nextthink = gpGlobals->time + 0.5F;
}


void CNapalmGrenade::UpdateOnRemove()
{
	StopSound("ambience/fire1.wav", CHAN_WEAPON);
	CPrimeGrenade::UpdateOnRemove();
}


CNapalmGrenade* CNapalmGrenade::NapalmGrenade(CBaseEntity* owner)
{
	auto grenade = Entity::Create<CNapalmGrenade>();

	grenade->v.owner = &owner->v;
	grenade->v.team = owner->TeamNumber();
	grenade->Spawn();

	grenade->v.classname = MAKE_STRING("napalmgrenade");

	return grenade;
}

#endif
