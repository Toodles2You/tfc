//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
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


class CDetpack : public CBaseEntity
{
public:
	bool Spawn() override;
	void Set();
	void EXPORT FireInTheHole();
	void EXPORT Detonate();

	static CDetpack* CreateDetpack(const Vector& origin, const float angle, CBaseEntity* activator, const float timer);
};


bool CDetpack::Spawn()
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_TOSS;
#if 0
	pev->effects |= EF_NODRAW;
#else
	EmitSound("weapons/mine_deploy.wav", CHAN_BODY, VOL_NORM, ATTN_IDLE);
#endif

	SetModel("models/detpack.mdl");
	return true;
}


void CDetpack::Set()
{
#if 0
	if ((pev->effects & EF_NODRAW) != 0)
#endif
	{
		pev->effects &= ~EF_NODRAW;
		EmitSound("weapons/mine_activate.wav", CHAN_WEAPON);
	}

	if (pev->pain_finished <= 6.0F)
	{
		FireInTheHole();
        pev->nextthink = gpGlobals->time + pev->pain_finished;
	}
	else
	{
		SetThink(&CDetpack::FireInTheHole);
		pev->nextthink = gpGlobals->time + pev->pain_finished - 6.0F;
	}
}


void CDetpack::FireInTheHole()
{
	EmitSound("weapons/mine_charge.wav", CHAN_WEAPON);

	SetThink(&CDetpack::Detonate);
	pev->nextthink = gpGlobals->time + 6.0F;
}


void CDetpack::Detonate()
{
	const auto sploj = pev->origin + Vector(0.0F, 0.0F, 16.0F);

	tent::Explosion(
		sploj,
		g_vecZero,
		tent::ExplosionType::Detpack,
		180.0F);

	CBaseEntity* activator = CWorld::World;

	if (pev->owner != nullptr)
	{
		activator = CBaseEntity::Instance(pev->owner);
	}

	util::GoalDetpackUse(sploj, activator, this);

	RadiusDamage(
		sploj,
		this,
		activator,
		1400.0F,
		700.0F,
		WEAP_DETPACK_SIZE,
		DMG_BLAST | DMG_ALWAYSGIB);

	Remove();
}


CDetpack* CDetpack::CreateDetpack(const Vector& origin, const float angle, CBaseEntity* activator, const float timer)
{
	const auto end = origin + Vector(0.0F, 0.0F, -16.0F);

	TraceResult trace;
	util::TraceLine(origin, end, util::ignore_monsters, activator, &trace);

	if (trace.flFraction == 1.0F
	 || trace.vecPlaneNormal.z < kGroundPlaneMinZ
	 || trace.fStartSolid != 0
	 || trace.fAllSolid != 0)
	{
		return nullptr;
	}

	CDetpack* detpack = GetClassPtr((CDetpack*)nullptr);
	detpack->pev->angles.y = angle;
	detpack->pev->owner = (activator != nullptr) ? activator->pev->pContainingEntity : nullptr;
	detpack->pev->pain_finished = timer;
	detpack->SetOrigin(trace.vecEndPos + Vector(0.0F, 0.0F, 0.6F));
	detpack->Spawn();
	return detpack;
}


void CBasePlayer::StartDetpack(const float timer)
{
	if (!IsAlive() || !IsPlayer())
	{
		return;
	}

	if ((m_TFState & kTFStateBuilding) != 0
	 || m_flBuildingFinished > gpGlobals->time)
	{
		return;
	}

	if (!m_bDetpackReady)
	{
		return;
	}

	CBaseEntity* detpack =
		CDetpack::CreateDetpack(
			pev->origin + Vector(0.0F, 0.0F, pev->mins.z + 2.0F),
			pev->angles.y,
			this,
			timer);

	if (detpack != nullptr)
	{
		pev->velocity.x = pev->velocity.y = 0.0F; /* Toodles FIXME: !!! */

        if (m_pActiveWeapon != nullptr)
        {
            m_pActiveWeapon->m_ForceSendAnimations = true;
            m_pActiveWeapon->Holster();
            m_pActiveWeapon->m_ForceSendAnimations = false;
        }

		m_hDetpack = detpack;
		m_bDetpackReady = false;
		m_TFState |= kTFStateBuilding;
		m_flBuildingFinished = gpGlobals->time + 5.0F;
	}
	else
	{
	}
}


void CBasePlayer::SetDetpack()
{
	CBaseEntity* detpack = m_hDetpack;

	if (detpack != nullptr)
	{
		dynamic_cast<CDetpack*>(detpack)->Set();
	}

    if (m_pActiveWeapon != nullptr)
    {
        m_pActiveWeapon->m_ForceSendAnimations = true;
        m_pActiveWeapon->Deploy();
        m_pActiveWeapon->m_ForceSendAnimations = false;
    }

	m_hDetpack = nullptr;
	m_TFState &= ~kTFStateBuilding;
	m_flBuildingFinished = gpGlobals->time + 2.0F;
}


void CBasePlayer::CancelDetpack()
{
	if ((m_TFState & kTFStateBuilding) == 0)
	{
		return;
	}

	CBaseEntity* detpack = m_hDetpack;

	if (detpack != nullptr)
	{
		detpack->Remove();
	}

    if (m_pActiveWeapon != nullptr)
    {
        m_pActiveWeapon->m_ForceSendAnimations = true;
        m_pActiveWeapon->Deploy();
        m_pActiveWeapon->m_ForceSendAnimations = false;
    }

	m_hDetpack = nullptr;
	m_bDetpackReady = true;
	m_TFState &= ~kTFStateBuilding;
	m_flBuildingFinished = gpGlobals->time + 2.0F;
}

