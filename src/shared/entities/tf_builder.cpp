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


LINK_ENTITY_TO_CLASS(tf_weapon_builder, CBuilder);

bool CBuilder::Spawn()
{
	if (!CTFWeapon::Spawn())
	{
		return false;
	}

	m_iClip = 0;

	return true;
}


void CBuilder::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_builder";
	i.iAmmo1 = AMMO_CELLS;
	i.iAmmo2 = -1;
	i.iMaxClip = -1;
	i.iSlot = -1;
	i.iPosition = -1;
	i.iFlags = 0;
	i.iWeight = -1;

	i.pszWorld = "models/tool_box.mdl";
}


bool CBuilder::AddToPlayer(CBasePlayer* pPlayer)
{
	if (!CTFWeapon::AddToPlayer(pPlayer))
	{
		Remove();

		return false;
	}

	m_iClip = 0;

#ifdef GAME_DLL

	for (int i = BUILD_DISPENSER; i <= BUILD_EXIT_TELEPORTER; i++)
	{
		CBaseEntity* building = m_pPlayer->m_hBuildings[i - 1];

		if (building != nullptr)
		{
			m_iClip |= 1 << i;
		}
	}

#endif /* GAME_DLL */

	return true;
}


void CBuilder::RemoveFromPlayer(bool forceSendAnimations)
{
	if (m_pPlayer == nullptr)
	{
		return;
	}

	m_pPlayer->SetWeaponHolstered(false, false);
	m_pPlayer->LeaveState(CBasePlayer::State::CannotMove);
	CTFWeapon::RemoveFromPlayer(forceSendAnimations);
	m_iWeaponState = BUILD_NONE;
}


void CBuilder::Deploy()
{
	/* Build 64 units in front of us. */

	/* Account for hull sizes. */

	auto gun = m_pPlayer->v.origin;
	gun.z += m_pPlayer->v.mins.z - VEC_HULL_MIN.z;

	const auto aim = Vector(0.0F, m_pPlayer->v.v_angle.y, 0.0F);

	Vector dir;
	AngleVectors(aim, &dir, nullptr, nullptr);

	v.origin = gun + dir * 64.0F;

	if (!CheckArea(v.origin, v.startpos))
	{
		StopBuilding();
		return;
	}

	m_pPlayer->SetWeaponHolstered(true, false);
	m_pPlayer->EnterState(CBasePlayer::State::CannotMove);
	m_iNextPrimaryAttack = 1000 * kBuildTime[m_iWeaponState];

#ifdef GAME_DLL
	v.aiment = v.owner = nullptr;
	v.team = m_pPlayer->TeamNumber();
	v.iuser4 = m_pPlayer->v.GetIndex();

	v.solid = SOLID_BBOX;
	v.movetype = MOVETYPE_TOSS;

    SetOrigin(v.origin);

	v.flags &= ~FL_ONGROUND;

	SetModel("models/tool_box.mdl");

    SetSize(Vector(-16.0F, -16.0F, 0.0F), Vector(16.0F, 16.0F, 48.0F));

	v.effects &= ~EF_NODRAW;

	v.angles.y = util::AngleMod(m_pPlayer->v.angles.y + 180.0F);
	v.angles.x = v.angles.z = 0.0F;
#endif

	m_pPlayer->EmitSoundPredicted("weapons/building.wav", CHAN_WEAPON,
		0.98F, ATTN_NORM, 125);
}


void CBuilder::WeaponPostFrame()
{
	if (m_iWeaponState == BUILD_NONE)
	{
		return;
	}

	const auto &info = GetInfo();

	if (m_pPlayer->m_rgAmmo[info.iAmmo1] < kBuildCost[m_iWeaponState])
	{
		StopBuilding();
		return;
	}

	/* See if we've moved too far away. */

	if (static_cast<int>((m_pPlayer->v.origin - v.origin).LengthSquared()) >= 112 * 112)
	{
		StopBuilding();
		return;
	}

	if (m_iNextPrimaryAttack <= 0)
	{
		FinishBuilding();
	}
}


void CBuilder::Holster()
{
	m_pPlayer->SetWeaponHolstered(false, false);
	m_pPlayer->LeaveState(CBasePlayer::State::CannotMove);
	m_iNextPrimaryAttack = 0;

	v.aiment = v.owner = &m_pPlayer->v;
	v.iuser4 = 0;

	v.movetype = MOVETYPE_FOLLOW;
	v.solid = SOLID_NOT;
	v.effects = EF_NODRAW;
}


bool CBuilder::CheckArea(const Vector& origin, Vector& outStartPos)
{
	outStartPos = origin;

#ifdef GAME_DLL

	const auto contents = engine::PointContents(origin);

	/* Must be in the open. */

	/*
		Toodles FIXME: This needs to account for
		the new "no build" & "no grenade" contents.
	*/

	if (contents != CONTENTS_EMPTY && contents != CONTENTS_WATER)
	{
		return false;
	}

	TraceResult trace;

	/* Make sure we can see the build origin. */

	util::TraceLine(m_pPlayer->v.origin, origin,
		util::dont_ignore_monsters, m_pPlayer, &trace);
	
	if (trace.flFraction != 1.0F || trace.fAllSolid != 0)
	{
		return false;
	}

	/* Make sure the build origin is clear. */

	util::TraceHull(origin, origin,
		util::dont_ignore_monsters, 2, &m_pPlayer->v, &trace);
	
	if (trace.flFraction != 1.0F || trace.fAllSolid != 0)
	{
		return false;
	}

	/* Another slightly larger entity check, just to be safe. */

	CBaseEntity *entity = nullptr;

	while ((entity = util::FindEntityInSphere(entity, origin, 48.0F)) != nullptr)
	{
		if (entity == this || entity == m_pPlayer
		 || entity->v.solid == SOLID_NOT || entity->v.solid == SOLID_TRIGGER)
		{
			continue;
		}

		return false;
	}

	/* Find a place to rest on the ground. */

	util::TraceLine(origin, origin + Vector(0.0F, 0.0F, -64.0F),
		util::dont_ignore_monsters, this, &trace);

	if (trace.flFraction == 1.0F)
	{
		return false;
	}

	outStartPos = trace.vecEndPos;

#endif /* GAME_DLL */

	return true;
}


void CBuilder::StartBuilding(const int buildingType)
{
	if (buildingType > 10)
	{
		DestroyBuilding(buildingType - 10, true);
		return;
	}

	if (buildingType > 5)
	{
		DestroyBuilding(buildingType - 5, false);
		return;
	}

	if (buildingType == BUILD_NONE)
	{
		StopBuilding();
		return;
	}

	const auto &info = GetInfo();

	if ((m_iClip & (1 << buildingType)) != 0)
	{
		StopBuilding();
		return;
	}

	if (m_pPlayer->m_rgAmmo[info.iAmmo1] < kBuildCost[buildingType])
	{
		StopBuilding();
		return;
	}

	m_iWeaponState = buildingType;

	Deploy();
}


void CBuilder::StopBuilding()
{
	if (m_iWeaponState != BUILD_NONE)
	{
		Holster();

#ifdef GAME_DLL
		m_pPlayer->EmitSoundHUD("common/null.wav", CHAN_WEAPON);
		m_pPlayer->EmitSoundHUD("common/wpn_denyselect.wav", CHAN_ITEM);
#else
		m_pPlayer->EmitSoundPredicted("common/null.wav", CHAN_WEAPON);
		m_pPlayer->EmitSoundPredicted("common/wpn_denyselect.wav", CHAN_ITEM);
#endif

		m_iWeaponState = BUILD_NONE;
	}
}


void CBuilder::FinishBuilding()
{
	const auto &info = GetInfo();

	const auto buildingType = m_iWeaponState;

	Holster();

	m_iWeaponState = BUILD_NONE;

	if (m_pPlayer->m_rgAmmo[info.iAmmo1] < kBuildCost[buildingType])
	{
		return;
	}

	if (!SpawnBuilding(buildingType))
	{
		return;
	}

	m_pPlayer->m_rgAmmo[info.iAmmo1] -= kBuildCost[buildingType];

	m_iClip |= 1 << buildingType;

	m_pPlayer->EmitSoundPredicted("weapons/turrset.wav", CHAN_WEAPON);
}


#ifdef CLIENT_DLL

bool CBuilder::SpawnBuilding(const int buildingType)
{
	switch (buildingType)
	{
		default:
		{
			return false;
		}

		case BUILD_DISPENSER:
		{
			break;
		}

		case BUILD_SENTRYGUN:
		{
			break;
		}

		case BUILD_ENTRY_TELEPORTER:
		case BUILD_EXIT_TELEPORTER:
		{
			break;
		}
	}

	return true;
}

#endif /* CLIENT_DLL */


int CBuilder::GetBuildState()
{
	const auto &info = GetInfo();

	auto buildState = m_iClip;

	if (m_iWeaponState != BUILD_NONE)
	{
		buildState |= BS_BUILDING;
	}

	for (int i = BUILD_DISPENSER; i <= BUILD_EXIT_TELEPORTER; i++)
	{
		if ((buildState & (1 << i)) == 0
		 && m_pPlayer->m_rgAmmo[info.iAmmo1] >= kBuildCost[i])
		{
			buildState |= 32 << i;
		}
	}

	return buildState;
}


void CBuilder::DestroyBuilding(const int buildingType, const bool detonate)
{
	if (buildingType == BUILD_NONE)
	{
		return;
	}

	if ((m_iClip & (1 << buildingType)) == 0)
	{
		return;
	}

#ifdef GAME_DLL

	CBaseEntity* building = m_pPlayer->m_hBuildings[buildingType - 1];

	if (building != nullptr)
	{
		if (detonate)
		{
			building->Killed(m_pPlayer, m_pPlayer, DMG_GENERIC);
		}
		else
		{
			building->Remove();
		}
	}

#endif /* GAME_DLL */

	m_iClip &= ~(1 << buildingType);
}

