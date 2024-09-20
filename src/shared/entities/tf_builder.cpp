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

void CBuilder::GetWeaponInfo(WeaponInfo& i)
{
	i.pszName = "tf_weapon_builder";
	i.iAmmo1 = -1;
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
	m_iWeaponState = 0;
}


void CBuilder::Deploy()
{
	const auto info = GetInfo();

	m_pPlayer->SetWeaponHolstered(true, false);
	m_pPlayer->EnterState(CBasePlayer::State::CannotMove);
	m_iNextPrimaryAttack = 1000 * kBuildingTime[m_iWeaponState - 1];

#ifdef GAME_DLL
	const auto gun = m_pPlayer->v.origin;
	const auto aim = Vector(0.0F, m_pPlayer->v.v_angle.y, 0.0F);

	Vector dir;
	AngleVectors(aim, &dir, nullptr, nullptr);

	v.aiment = nullptr;
	v.team = m_pPlayer->TeamNumber();

	v.solid = SOLID_BBOX;
	v.movetype = MOVETYPE_TOSS;

    SetOrigin(gun + dir * 64.0F);

	v.flags &= ~FL_ONGROUND;

	SetModel("models/tool_box.mdl");

    SetSize(Vector(-16, -16, 0), Vector(16, 16, 48));

	v.effects &= ~EF_NODRAW;
	v.angles.x = v.angles.z = 0;
#endif

	m_pPlayer->EmitSoundPredicted("weapons/building.wav", CHAN_WEAPON, VOL_NORM, ATTN_NORM, PITCH_HIGH);
}


void CBuilder::WeaponPostFrame()
{
	if (m_iWeaponState == 0)
	{
		return;
	}

	if (m_pPlayer->m_rgAmmo[AMMO_CELLS] < kBuildingCost[m_iWeaponState - 1])
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

	v.aiment = &m_pPlayer->v;

	v.movetype = MOVETYPE_FOLLOW;
	v.solid = SOLID_NOT;
	v.effects = EF_NODRAW;
}


void CBuilder::StartBuilding(const int buildingType)
{
	m_iWeaponState = buildingType;

	if (m_pPlayer->m_rgAmmo[AMMO_CELLS] < kBuildingCost[m_iWeaponState - 1])
	{
		StopBuilding();
		return;
	}

	Deploy();
}


void CBuilder::StopBuilding()
{
	if (m_iWeaponState != 0)
	{
		Holster();

		m_iWeaponState = 0;
	}
}


void CBuilder::FinishBuilding()
{
	Holster();

	m_pPlayer->m_rgAmmo[AMMO_CELLS] -= kBuildingCost[m_iWeaponState - 1];

	m_iWeaponState = 0;

	m_pPlayer->EmitSoundPredicted("weapons/turrset.wav", CHAN_WEAPON);
}


int CBuilder::GetBuildState()
{
	auto buildState = 0;

	if (m_pPlayer->m_rgAmmo[AMMO_CELLS] >= kBuildingCost[0])
	{
		buildState |= BS_CAN_DISPENSER;
	}

	if (m_pPlayer->m_rgAmmo[AMMO_CELLS] >= kBuildingCost[1])
	{
		buildState |= BS_CAN_SENTRYGUN;
	}

	if (m_pPlayer->m_rgAmmo[AMMO_CELLS] >= kBuildingCost[2])
	{
		buildState |= BS_CAN_ENTRY_TELEPORTER;
	}

	if (m_pPlayer->m_rgAmmo[AMMO_CELLS] >= kBuildingCost[3])
	{
		buildState |= BS_CAN_EXIT_TELEPORTER;
	}

	if (m_iWeaponState != 0)
	{
		buildState |= BS_BUILDING;
	}

	return buildState;
}

