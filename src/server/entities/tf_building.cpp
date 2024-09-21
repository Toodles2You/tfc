//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Erecting
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "shake.h"

#define attachment euser1
#define sibling euser2
#define left_yaw vuser1.x
#define right_yaw vuser1.y
#define sentry_state vuser1.z


class CBuilding : public CBaseAnimating
{
public:
	friend class CBuilder;

	CBuilding(Entity* containingEntity) : CBaseAnimating(containingEntity) {}

	bool Spawn() override;
	void UpdateOnRemove() override;

	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;
	void Killed(CBaseEntity* inflictor, CBaseEntity* attacker, int bitsDamageType) override;

	virtual const char* GetModelName() { return "models/dispenser.mdl"; }
	virtual const char* GetClassName() { return "building_dispenser"; }
	virtual float GetHeight() { return 48.0F; }

protected:
    CBasePlayer* m_pPlayer;
};


bool CBuilding::Spawn()
{
	/* Set up the player's building handle. */

	m_pPlayer->m_hBuildings[PCNumber() - 1] = this;

	v.classname = MAKE_STRING(GetClassName());
	v.team = m_pPlayer->TeamNumber();

	v.solid = SOLID_BBOX;
	v.movetype = MOVETYPE_TOSS;

    SetOrigin(v.origin);

	SetModel(GetModelName());

    SetSize(Vector(-16.0F, -16.0F, 0.0F), Vector(16.0F, 16.0F, GetHeight()));

	v.sequence = 0;
	ResetSequenceInfo();

	v.effects |= EF_NOINTERP;

	v.takedamage = DAMAGE_AIM;
	v.health = v.max_health = 150.0F;

	return true;
}


void CBuilding::UpdateOnRemove()
{
	if (m_pPlayer != nullptr)
	{
		/* Clear the player's building handle. */

		m_pPlayer->m_hBuildings[PCNumber() - 1] = nullptr;

		/* Tell the player's builder that we're being removed. */

		if (m_pPlayer->HasPlayerWeapon(WEAPON_BUILDER))
		{
			auto builder = static_cast<CBuilder*>(
				m_pPlayer->m_rgpPlayerWeapons[WEAPON_BUILDER]);

			builder->m_iClip &= ~(1 << PCNumber());
		}
	}

	if (v.attachment != nullptr)
	{
		auto head = v.attachment->Get<CBaseEntity>();

		if (head != nullptr)
		{
			head->Remove();
		}

		v.attachment = nullptr;
	}

	CBaseAnimating::UpdateOnRemove();
}


bool CBuilding::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	if (v.takedamage == DAMAGE_NO)
	{
		return false;
	}

	if (g_pGameRules->PlayerRelationship(attacker, this) >= GR_ALLY)
	{
		return false;
	}

	v.health -= flDamage;

	if (v.health <= 0.0F)
	{
		Killed(inflictor, attacker, bitsDamageType);

		return false;
	}

	return true;
}


void CBuilding::Killed(CBaseEntity* inflictor, CBaseEntity* attacker, int bitsDamageType)
{
	v.takedamage = DAMAGE_NO;
	v.deadflag = DEAD_DEAD;
	v.health = std::min(v.health, 0.0F);

	tent::Explosion(Center(), Vector(0.0F, 0.0f, -1.0F), tent::ExplosionType::Normal);

	if (attacker != m_pPlayer)
	{
		char* str = nullptr;

		switch (PCNumber())
		{
			case BUILD_DISPENSER:
			{
				str = "#Dispenser_destroyed";
				break;
			}

			case BUILD_SENTRYGUN:
			{
				str = "#Sentry_destroyed";
				break;
			}

			case BUILD_ENTRY_TELEPORTER:
			{
				str = "#Teleporter_Entrance_Destroyed";
				break;
			}

			case BUILD_EXIT_TELEPORTER:
			{
				str = "#Teleporter_Exit_Destroyed";
				break;
			}
		}

		if (str != nullptr)
		{
			util::ClientPrint(m_pPlayer, HUD_PRINTCENTER, str);
		}
	}

	Remove();
}


class CDispenser : public CBuilding
{
public:
	CDispenser(Entity* containingEntity) : CBuilding(containingEntity) {}

	bool Spawn() override;
	void Think() override;
	void Touch(CBaseEntity* other) override;

protected:
	static constexpr byte kDispenseInterval = 2;

	static constexpr byte kGenerateInterval = 14;

	static constexpr int kDispenseAmmo[] = {20, 20, 10, 75};
	static constexpr int kDispenseArmor = 20;

	static constexpr int kGenerateAmmo[] = {20, 30, 15, 20};
	static constexpr int kGenerateArmor = 25;

	static constexpr int kMaxAmmo[] = {400, 600, 300, 400};
	static constexpr int kMaxArmor = 500;

	uint16 m_rgAmmo[AMMO_SECONDARY];
	uint16 m_rgArmor;
};


class CDispenserTrigger : public CBaseEntity
{
public:
	CDispenserTrigger(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	bool Spawn() override;
    void Touch(CBaseEntity* other) override;

	int ObjectCaps() override { return (CBaseEntity::ObjectCaps() | FCAP_DONT_SAVE) & ~FCAP_ACROSS_TRANSITION; }
};


bool CDispenser::Spawn()
{
	if (!CBuilding::Spawn())
	{
		return false;
	}

	auto head = Entity::Create<CDispenserTrigger>();

	if (head == nullptr)
	{
		return false;
	}

	v.attachment = &head->v;

	head->v.attachment = &v;
	head->v.origin = v.origin;

	if (!head->Spawn())
	{
		return false;
	}

	v.nextthink = gpGlobals->time + kGenerateInterval + 2.0F;

	return true;
}


void CDispenser::Think()
{
	/* Generate some ammo & armor. */

	for (int i = 0; i < AMMO_SECONDARY; i++)
	{
		m_rgAmmo[i] = std::min(
			m_rgAmmo[i] + kGenerateAmmo[i], kMaxAmmo[i]);
	}

	m_rgArmor = std::min(m_rgArmor + kGenerateArmor, kMaxArmor);

	engine::EmitAmbientSound(&v, Center(), "items/suitchargeok1.wav",
		0.5F, ATTN_IDLE, 0, PITCH_NORM);

	v.nextthink = gpGlobals->time + kGenerateInterval;
}


void CDispenser::Touch(CBaseEntity* other)
{
	if (!other->IsPlayer() || !other->IsAlive())
	{
		return;
	}

	/*
		Toodles: The use of radsuit_finished indicates that a dispenser is
		dispensing this frame. As such, touch functions for everyone should
		be run. It also ensures the sound only plays once.
	*/

	const auto firstDispenseThisFrame =
		v.radsuit_finished != gpGlobals->time;

	if (v.dmgtime > gpGlobals->time && firstDispenseThisFrame)
	{
		return;
	}

	auto player = static_cast<CBasePlayer*>(other);

	auto result = false;

	/* Dispense some ammo. */

#if 1

	/* Toodles: Regular ammo types are infinite. */

	for (int i = 0; i < AMMO_CELLS; i++)
	{
		if (player->GiveAmmo(kDispenseAmmo[i], i) != 0)
		{
			result = true;
		}
	}

	/* Toodles: Cells are limited & are only dispensed to engineers. */

	if (player->PCNumber() == PC_ENGINEER)
	{
		const auto i = AMMO_CELLS;

		const auto ammo = std::min((int)m_rgAmmo[i], kDispenseAmmo[i]);

		const auto dispensed = player->GiveAmmo(ammo, i);

		m_rgAmmo[i] -= dispensed;

		if (dispensed != 0)
		{
			result = true;
		}
	}

#else

	for (int i = 0; i < AMMO_SECONDARY; i++)
	{
		const auto ammo = std::min((int)m_rgAmmo[i], kDispenseAmmo[i]);

		const auto dispensed = player->GiveAmmo(ammo, i);

		m_rgAmmo[i] -= dispensed;

		if (dispensed != 0)
		{
			result = true;
		}
	}

#endif

	/* Dispense some armor. */

	const auto armor = std::min((int)m_rgArmor, kDispenseArmor);

	const auto add = std::min((float)armor,
		player->m_flArmorMax - player->v.armorvalue);

	player->v.armorvalue += add;
	m_rgArmor -= add;

	if (add != 0.0F)
	{
		result = true;
	}

	if (result)
	{
		if (firstDispenseThisFrame)
		{
			/*
				Ambient sound cause the stupid dispenser's
				origin will likely be in the floor.
			*/

			engine::EmitAmbientSound(&v, Center(), "items/ammopickup2.wav",
				VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
		}

		/* Tell the engineer if we're being used by an enemy. */

		if (g_pGameRules->PlayerRelationship(m_pPlayer, other) < GR_ALLY
		 && util::DoDamageResponse(other, m_pPlayer))
		{
			util::ClientPrint(m_pPlayer, HUD_PRINTCENTER, "#Dispenser_used");
		}
	}

	v.dmgtime = gpGlobals->time + kDispenseInterval;
	v.radsuit_finished = gpGlobals->time;
}


bool CDispenserTrigger::Spawn()
{
	v.movetype = MOVETYPE_NONE;
	v.solid = SOLID_TRIGGER;

	SetOrigin(v.origin);

    SetSize(Vector(-64.0F, -64.0F, 0.0F), Vector(64.0F, 64.0F, 64.0F));

	v.effects |= EF_NODRAW;

	v.angles = g_vecZero;

	return true;
}


void CDispenserTrigger::Touch(CBaseEntity* other)
{
	auto dispenser = v.attachment->Get<CDispenser>();

    dispenser->Touch(other);
}


class CSentryBase : public CBuilding
{
public:
	friend class CSentryGun;

	CSentryBase(Entity* containingEntity) : CBuilding(containingEntity) {}

	const char* GetModelName() override { return "models/base.mdl"; }
	const char* GetClassName() override { return "building_sentrygun"; }

	bool Spawn() override;
};


class CSentryGun : public CBaseAnimating
{
protected:
	static constexpr float kSentryYawRange = 50.0F;
	static constexpr float kSentryYawSpeed = 30.0F; /* 10.0F */
	static constexpr float kSentryRange = 1000.0F;
	static constexpr float kSentryDamage = 16.0F;

	enum
	{
		kRotateLeft,
		kRotateRight,
		kHunting,
		kLockedOn,
	};

	enum
	{
		kSequenceOff,
		kSequenceFire,
		kSequenceIdle,
	};

public:
	CSentryGun(Entity* containingEntity) : CBaseAnimating(containingEntity) {}

	bool Spawn() override;
	void Think() override;

protected:
	void FindTarget();
	float HuntTarget(CBaseEntity* other);
	void SetYaw(const float yaw);
	bool UpdateAngles(const float scale = 1.0F);
	void Fire();
	void Rotate();
};


bool CSentryBase::Spawn()
{
	if (!CBuilding::Spawn())
	{
		return false;
	}

	auto head = Entity::Create<CSentryGun>();

	if (head == nullptr)
	{
		return false;
	}

	v.attachment = &head->v;

	head->v.attachment = &v;

	head->v.origin = v.origin + Vector(0.0F, 0.0F, 21.0F);
	head->v.v_angle = v.angles;
	head->v.team = v.team;

	if (!head->Spawn())
	{
		return false;
	}

	return true;
}


bool CSentryGun::Spawn()
{
	v.solid = SOLID_NOT;
	v.movetype = MOVETYPE_STEP;
	v.gravity = 0.0F;

    SetOrigin(v.origin);

	SetModel("models/sentry1.mdl");

	InitBoneControllers();

	SetBoneController(0, v.v_angle.y);
	SetBoneController(1, -v.v_angle.x);

	v.sequence = kSequenceIdle;
	ResetSequenceInfo();

	v.effects |= EF_NOINTERP;

	/* Toodles: This was originally 42 units from the base. */
	v.view_ofs.z = 21.0F;

	v.yaw_speed = kSentryYawSpeed;
	v.pitch_speed = kSentryYawSpeed;

	SetYaw(v.v_angle.y);

	v.nextthink = gpGlobals->time + 0.5F;

	v.radsuit_finished = v.nextthink;

	return true;
}


void CSentryGun::Think()
{
	StudioFrameAdvance();

	FindTarget();

	Rotate();

	auto thinkInterval = 0.1F;

	if (v.enemy != nullptr)
	{
		thinkInterval = 0.05F;
	}

	v.nextthink = gpGlobals->time + thinkInterval;
}


void CSentryGun::FindTarget()
{
	auto bestDistance = kSentryRange;
	CBaseEntity* bestEnemy = nullptr;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		const auto player = static_cast<CBasePlayer*>(util::PlayerByIndex(i));

		if (player == nullptr)
		{
			continue;
		}

		const auto distance = HuntTarget(player);

		if (distance < 0.0F || distance >= bestDistance)
		{
			continue;
		}

		bestDistance = distance;
		bestEnemy = player;
	}

	if (bestEnemy != nullptr)
	{
		/* Check if this is a new enemy. */

		if (v.enemy != &bestEnemy->v)
		{
			v.enemy = &bestEnemy->v;

			v.sentry_state = kHunting;

			v.sequence = kSequenceIdle;
			ResetSequenceInfo();

			EmitSound("weapons/turrspot.wav", CHAN_VOICE);
		}
	}
	else
	{
		/* Check if we've lost sight of any enemies. */

		if (v.enemy != nullptr)
		{
			v.enemy = nullptr;

			v.sentry_state = kRotateLeft;

			v.sequence = kSequenceIdle;
			ResetSequenceInfo();
		}
	}
}


float CSentryGun::HuntTarget(CBaseEntity* other)
{
	if (!other->IsPlayer() || !other->IsAlive())
	{
		return -1.0F;
	}

	if (v.enemy == nullptr || other != v.enemy->Get<CBaseEntity>())
	{
		/* Toodles TODO: Ignore disguised spies. */

		if (g_pGameRules->PlayerRelationship(other, this) >= GR_ALLY)
		{
			return -1.0F;
		}
	}
	
	/*
		Toodles: Removed a check for whether or not the
		enemy was in front of the sentry after 500 units.
	*/

	const auto distance = (other->Center() - EyePosition()).LengthSquared();

	if (distance > kSentryRange * kSentryRange)
	{
		return -1.0F;
	}

	CSentryBase* base = nullptr;

	if (v.attachment != nullptr)
	{
		base = v.attachment->Get<CSentryBase>();
	}

	TraceResult trace;

	util::TraceLine(EyePosition(), other->EyePosition(),
		util::ignore_monsters, base, &trace);

	if (trace.flFraction != 1.0F && trace.pHit != &other->v)
	{
		return -1.0F;
	}

	return sqrtf (distance);
}


void CSentryGun::SetYaw(const float yaw)
{
	v.v_angle.z = util::AngleMod(yaw);

	v.left_yaw = util::AngleMod(v.v_angle.z - kSentryYawRange);
	v.right_yaw = util::AngleMod(v.v_angle.z + kSentryYawRange);

	if (v.left_yaw > v.right_yaw)
	{
		const auto temp = v.left_yaw;

		v.left_yaw = v.right_yaw;

		v.right_yaw = temp;
	}

	v.sentry_state = kRotateLeft;

	v.ideal_yaw = v.left_yaw;
}


bool CSentryGun::UpdateAngles(const float scale)
{
    const auto delta = std::min(gpGlobals->time - v.radsuit_finished, 0.2F);

    v.radsuit_finished = gpGlobals->time;

	/* Update yaw. */

	const auto currentYaw = util::AngleMod(v.v_angle.y);

	const auto done = (currentYaw == v.ideal_yaw);

	if (!done)
	{
		const auto speed = v.yaw_speed * scale * 2.0F * delta;

		auto move = v.ideal_yaw - currentYaw;

		if (v.ideal_yaw > currentYaw)
		{
			if (move >= 180.0F)
			{
				move -= 360.0F;
			}
		}
		else
		{
			if (move <= -180.0F)
			{
				move += 360.0F;
			}
		}

		if (move > 0.0F)
		{
			if (move > speed)
			{	
				move = speed;
			}
		}
		else
		{
			if (move < -speed)
			{
				move = -speed;
			}
		}

		v.v_angle.y = util::AngleMod(currentYaw + move);
	}

	/* Update pitch. */

	const auto currentPitch = util::AngleMod(v.v_angle.x);

	if (currentPitch != v.idealpitch)
    {
		const auto speed = v.pitch_speed * scale * 2.0F * delta;

		auto move = v.idealpitch - currentPitch;

		if (v.idealpitch > currentPitch)
		{
			if (move >= 180.0F)
			{
				move -= 360.0F;
			}
		}
		else
		{
			if (move <= -180.0F)
			{
				move += 360.0F;
			}
		}

		if (move > 0.0F)
		{
			if (move > speed)
			{
				move = speed;
			}
		}
		else
		{
			if (move < -speed)
			{
				move = -speed;
			}
		}

		v.v_angle.x = util::AngleMod(currentPitch + move);
    }

	return done;
}


void CSentryGun::Fire()
{
	v.pain_finished--;

	if (v.pain_finished > 0)
	{
		return;
	}

	v.pain_finished = 4;

	v.sequence = kSequenceFire;
	v.frame = 0.0F;
	ResetSequenceInfo();

	v.effects |= EF_NOINTERP;

	EmitSound("weapons/pl_gun3.wav", CHAN_WEAPON);

	util::MakeAimVectors(v.v_angle);

	CSentryBase* base = nullptr;

	if (v.attachment != nullptr)
	{
		base = v.attachment->Get<CSentryBase>();
	}

	const auto start = EyePosition();
	const auto end = start + gpGlobals->v_forward * 8192.0F;

	TraceResult trace;

	if (util::TraceLine(start, end, &trace, base, util::kTraceBoxModel))
	{
		const auto other = trace.pHit->Get<CBaseEntity>();

		if (other != nullptr)
		{
			other->TakeDamage(base, base->m_pPlayer,
				kSentryDamage, DMG_BULLET);
		}
	}

	/* Toodles TODO: Sentry event. */

	MessageBegin(MSG_PVS, SVC_TEMPENTITY, trace.vecEndPos);
	WriteByte(TE_TRACER);
	WriteCoord(start);
	WriteCoord(trace.vecEndPos);
	MessageEnd();
}


void CSentryGun::Rotate()
{	
	/* Hunting an enemy. */

	if (v.sentry_state == kHunting || v.sentry_state == kLockedOn)
	{
		const auto enemy = v.enemy->Get<CBaseEntity>();

		if (enemy != nullptr)
		{
			const auto dir = (enemy->Center() - EyePosition()).Normalize();
			const auto angles = util::VecToAngles(dir);

			v.ideal_yaw = angles.y;
			v.idealpitch = angles.x;

			UpdateAngles(2.0F);

			SetBoneController(0, v.v_angle.y);
			SetBoneController(1, -v.v_angle.x);

			if (v.sentry_state == kLockedOn
			 || (fabsf(v.v_angle.y - v.ideal_yaw) <= 10.0F
			  && fabsf(v.v_angle.x - v.idealpitch) <= 10.0F))
			{
				v.sentry_state = kLockedOn;

				Fire();
			}

			return;
		}

		v.sentry_state = kRotateLeft;

		v.sequence = kSequenceIdle;
		ResetSequenceInfo();
	}

	/* Rotating. */

	if (!UpdateAngles())
	{
		SetBoneController(0, v.v_angle.y);
		SetBoneController(1, -v.v_angle.x);
		return;
	}

	if (v.sentry_state == kRotateLeft)
	{
		v.sentry_state = kRotateRight;

		v.ideal_yaw = v.right_yaw;

		if (engine::RandomLong(0, 9) == 0)
		{
			EmitSound("weapons/turridle.wav", CHAN_VOICE);
		}
	}
	else
	{
		v.sentry_state = kRotateLeft;

		v.ideal_yaw = v.left_yaw;
	}
}


class CTeleporter : public CBuilding
{
public:
	CTeleporter(Entity* containingEntity) : CBuilding(containingEntity) {}

	const char* GetModelName() override { return "models/teleporter.mdl"; }
	const char* GetClassName() override { return "building_teleporter"; }
	float GetHeight() override { return 12.0F; }

	bool Spawn() override;
	void UpdateOnRemove() override;

protected:
	CTeleporter* GetMatchingTeleporter();
	bool CanTeleport(CBaseEntity* other);

	void SetReady();
	void ClearReady();
	void SelectForTeleport(CBaseEntity* other);
	void Recharge(const float duration = 0.0F);

	void EXPORT EntryTouch(CBaseEntity* other);

	void EXPORT Teleport();
	void EXPORT Ready();

protected:
	static constexpr byte kTeleportInterval = 10;

	EHANDLE m_hTeleportee;
};


bool CTeleporter::Spawn()
{
	if (!CBuilding::Spawn())
	{
		return false;
	}

	const auto entry = PCNumber() == BUILD_ENTRY_TELEPORTER;

	/* Link the player's teleporters together. */

	CBaseEntity* match = m_pPlayer->m_hBuildings[
		(entry ? BUILD_EXIT_TELEPORTER : BUILD_ENTRY_TELEPORTER) - 1];

	if (match != nullptr)
	{
		v.sibling = &match->v;

		match->v.sibling = &v;

		Recharge();
	}

	return true;
}


void CTeleporter::UpdateOnRemove()
{
	ClearReady();

	const auto match = GetMatchingTeleporter();

	if (match != nullptr)
	{
		v.sibling = nullptr;

		match->v.sibling = nullptr;

		match->ClearReady();
	}

	CBaseEntity* const other = m_hTeleportee;

	if (other != nullptr)
	{
		CBaseEntity* const otherTeleporter =
			static_cast<CBasePlayer*>(other)->m_hTeleporter;

		if (otherTeleporter == this)
		{
			static_cast<CBasePlayer*>(other)->m_hTeleporter = nullptr;
		}

		m_hTeleportee = nullptr;
	}

	CBuilding::UpdateOnRemove();
}


CTeleporter* CTeleporter::GetMatchingTeleporter()
{
	if (v.sibling == nullptr)
	{
		return nullptr;
	}

	return v.sibling->Get<CTeleporter>();
}


bool CTeleporter::CanTeleport(CBaseEntity* other)
{
	/* Toodles TODO: Allow disguised spies to use enemy teleporters. */

	if (g_pGameRules->PlayerRelationship(other, this) < GR_ALLY)
	{
		return false;
	}

	/* Already queued for another teleporter. */

	CBaseEntity* const otherTeleporter =
		static_cast<CBasePlayer*>(other)->m_hTeleporter;

	if (otherTeleporter != nullptr)
	{
		return false;
	}

	/* Make sure they're standing still on top of us. */

	if (other->v.groundentity != &v || (other->v.flags & FL_ONGROUND) == 0)
	{
		return false;
	}

	if ((other->v.button & (IN_FORWARD | IN_BACK | IN_MOVERIGHT | IN_MOVELEFT)) != 0)
	{
		return false;
	}

	if (static_cast<int>(other->v.velocity.LengthSquared()) != 0)
	{
		return false;
	}

	return true;
}


void CTeleporter::SetReady()
{
	ClearThink();

	if (PCNumber() == BUILD_ENTRY_TELEPORTER)
	{
		SetTouch(&CTeleporter::EntryTouch);
	}
	else
	{
		ClearTouch();
	}
}


void CTeleporter::ClearReady()
{
	ClearThink();
	ClearTouch();
}


void CTeleporter::SelectForTeleport(CBaseEntity* other)
{
	m_hTeleportee = other;

	static_cast<CBasePlayer*>(other)->m_hTeleporter = this;

	SetThink(&CTeleporter::Teleport);

	v.nextthink = gpGlobals->time + 0.5F;
}


void CTeleporter::Recharge(const float duration)
{
	const auto match = GetMatchingTeleporter();

	if (duration == 0.0F)
	{
		Ready();
		match->Ready();
		return;
	}

	SetThink(&CTeleporter::Ready);
	match->SetThink(&CTeleporter::Ready);

	v.nextthink = match->v.nextthink = gpGlobals->time + duration;
}


void CTeleporter::EntryTouch(CBaseEntity* other)
{
	if (!other->IsPlayer() || !other->IsAlive())
	{
		return;
	}

	if (!CanTeleport(other))
	{
		return;
	}

    engine::EmitAmbientSound(&v, Center(), "misc/teleport_out.wav",
		VOL_NORM, ATTN_NORM, 0, PITCH_NORM);

	util::ScreenFade(other, Vector(0.0F, 255.0F, 0.0F),
		0.5F, 0.1F, 255.0F, FFADE_OUT);

	const auto match = GetMatchingTeleporter();

	ClearReady();
	match->ClearReady();

	match->SelectForTeleport(other);
}


void CTeleporter::Teleport()
{
	CBaseEntity* const other = m_hTeleportee;

	if (other == nullptr)
	{
		Recharge();
		return;
	}

	CBaseEntity* const otherTeleporter =
		static_cast<CBasePlayer*>(other)->m_hTeleporter;

	m_hTeleportee = nullptr;

	static_cast<CBasePlayer*>(other)->m_hTeleporter = nullptr;

	/* Make sure the player never died. */

	if (!other->IsPlayer() || !other->IsAlive() || otherTeleporter != this)
	{
		Recharge();
		return;
	}

	/* Teleport them to us! */

	other->v.flags &= ~FL_ONGROUND;

	auto origin = v.origin;
	origin.z += v.maxs.z - other->v.mins.z + 1.0F;

	other->SetOrigin(origin);

	other->v.angles = other->v.v_angle = v.angles;
	other->v.fixangle = 1;

	other->v.velocity = other->v.basevelocity = g_vecZero;

	engine::EmitAmbientSound(&v, Center(), "misc/teleport_in.wav",
		VOL_NORM, ATTN_NORM, 0, PITCH_NORM);

	util::ScreenFade(other, Vector(0.0F, 255.0F, 0.0F),
		0.5F, 0.0F, 255.0F, FFADE_IN);

	Recharge(kTeleportInterval);
}


void CTeleporter::Ready()
{
	SetReady();

	engine::EmitAmbientSound(&v, Center(), "misc/teleport_ready.wav",
		VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
}


bool CBuilder::SpawnBuilding(const int buildingType)
{
	CBuilding* building = nullptr;

	switch (buildingType)
	{
		default:
		{
			return false;
		}

		case BUILD_DISPENSER:
		{
			building = Entity::Create<CDispenser>();

			break;
		}

		case BUILD_SENTRYGUN:
		{
			building = Entity::Create<CSentryBase>();

			break;
		}

		case BUILD_ENTRY_TELEPORTER:
		case BUILD_EXIT_TELEPORTER:
		{
			building = Entity::Create<CTeleporter>();

			break;
		}
	}

	if (building == nullptr)
	{
		return false;
	}

	building->v.origin = v.origin;
	building->v.angles = v.angles;
	building->v.playerclass = buildingType;
	building->m_pPlayer = m_pPlayer;

	if (!building->Spawn())
	{
		building->Remove();

		return false;
	}

	return true;
}

