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


class CSentryGun : public CBuilding
{
public:
	CSentryGun(Entity* containingEntity) : CBuilding(containingEntity) {}

	const char* GetModelName() override { return "models/base.mdl"; }
	const char* GetClassName() override { return "building_sentrygun"; }
};


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
			building = Entity::Create<CSentryGun>();

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

