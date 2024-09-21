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

#define attachment euser1


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
};


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

