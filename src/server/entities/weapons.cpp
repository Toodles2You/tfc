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

===== weapons.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "UserMessages.h"

// Precaches the weapon and queues the weapon info for sending to clients
void util::PrecacheWeapon(const char* szClassname)
{
	Entity* pent;

	pent = engine::CreateNamedEntity(MAKE_STRING(szClassname));
	if (pent == nullptr)
	{
		engine::AlertMessage(at_console, "NULL Ent in UTIL_PrecacheOtherWeapon\n");
		return;
	}

	CBaseEntity* pEntity = pent->Get<CBaseEntity>();

	if (pEntity)
	{
		const auto weapon = dynamic_cast<CBasePlayerWeapon*>(pEntity);

		WeaponInfo info;
		memset(&info, 0, sizeof(info));

		weapon->Precache();
		weapon->GetWeaponInfo(info);

		CBasePlayerWeapon::WeaponInfoArray[weapon->GetID()] = info;
	}

	engine::RemoveEntity(pent);
}

// called by worldspawn
void W_Precache()
{
	memset(CBasePlayerWeapon::WeaponInfoArray, 0, sizeof(CBasePlayerWeapon::WeaponInfoArray));

	// weapons
	util::PrecacheWeapon("weapon_crowbar");
	util::PrecacheWeapon("weapon_9mmAR");

	// common world objects
	util::PrecacheOther("item_battery");
	util::PrecacheOther("item_healthkit");

	if (util::IsMultiplayer())
	{
		util::PrecacheOther("weaponbox");
	}

	g_sModelIndexShell = engine::PrecacheModel("models/shell.mdl");

	g_sModelIndexLaser = engine::PrecacheModel("sprites/laserbeam.spr");
	g_sModelIndexLaserDot = engine::PrecacheModel("sprites/laserdot.spr");
	g_sModelIndexFireball = engine::PrecacheModel("sprites/zerogxplode.spr");
	g_sModelIndexWExplosion = engine::PrecacheModel("sprites/WXplo1.spr");
	g_sModelIndexSmoke = engine::PrecacheModel("sprites/steam1.spr");
	g_sModelIndexBubbles = engine::PrecacheModel("sprites/bubble.spr");
	g_sModelIndexBloodSpray = engine::PrecacheModel("sprites/bloodspray.spr");
	g_sModelIndexBloodDrop = engine::PrecacheModel("sprites/blood.spr");

	engine::PrecacheModel("models/grenade.mdl");
	engine::PrecacheModel("models/w_grenade.mdl");
	engine::PrecacheModel("sprites/explode1.spr");

	engine::PrecacheModel("models/shell.mdl");
	engine::PrecacheModel("models/shotgunshell.mdl");

	engine::PrecacheSound("weapons/debris1.wav");
	engine::PrecacheSound("weapons/debris2.wav");
	engine::PrecacheSound("weapons/debris3.wav");

	engine::PrecacheSound("weapons/grenade_hit1.wav");
	engine::PrecacheSound("weapons/grenade_hit2.wav");
	engine::PrecacheSound("weapons/grenade_hit3.wav");

	engine::PrecacheSound("weapons/bullet_hit1.wav");
	engine::PrecacheSound("weapons/bullet_hit2.wav");

	engine::PrecacheSound("items/weapondrop1.wav");

	engine::PrecacheSound("items/9mmclip1.wav");
	engine::PrecacheSound("items/9mmclip2.wav");

	engine::PrecacheSound("weapons/desert_eagle_sight.wav");
	engine::PrecacheSound("weapons/desert_eagle_sight2.wav");

	engine::PrecacheSound("items/itembk2.wav");
	engine::PrecacheSound("items/gunpickup2.wav");

	engine::PrecacheSound("weapons/ric1.wav");
	engine::PrecacheSound("weapons/ric2.wav");
	engine::PrecacheSound("weapons/ric3.wav");
	engine::PrecacheSound("weapons/ric4.wav");
	engine::PrecacheSound("weapons/ric5.wav");

#ifdef HALFLIFE_GRENADES
	engine::PrecacheSound("weapons/ax1.wav");
	engine::PrecacheSound("weapons/grenade.wav");
	engine::PrecacheSound("weapons/timer.wav");
#endif
}


#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CBasePlayerWeapon)
	DEFINE_FIELD(CBasePlayerWeapon, m_pPlayer, FIELD_CLASSPTR),
	DEFINE_FIELD(CBasePlayerWeapon, m_iClip, FIELD_INTEGER),
END_SAVERESTORE(CBasePlayerWeapon, CBaseAnimating)
#endif


bool CBasePlayerWeapon::Spawn()
{
	const auto info = GetInfo();

	Precache();

	SetModel(info.pszWorld);

	v.movetype = MOVETYPE_TOSS;
	v.solid = SOLID_TRIGGER;

	engine::DropToFloor(&v);

	m_iClip = iMaxClip();

	Materialize();

	return true;
}


void CBasePlayerWeapon::Precache()
{
	WeaponInfo info;
	memset(&info, 0, sizeof(info));

	GetWeaponInfo(info);

	if (info.pszView != nullptr)
	{
		engine::PrecacheModel(info.pszView);
	}

	if (info.pszWorld != nullptr)
	{
		engine::PrecacheModel(info.pszWorld);
	}

	if (info.pszPlayer != nullptr)
	{
		engine::PrecacheModel(info.pszPlayer);
	}
}


void CBasePlayerWeapon::SetObjectCollisionBox()
{
	v.absmin = v.origin + Vector(-24, -24, 0);
	v.absmax = v.origin + Vector(24, 24, 16);
}


//=========================================================
// Materialize - make a CBasePlayerWeapon visible and tangible
//=========================================================
void CBasePlayerWeapon::Materialize()
{
	if ((v.effects & EF_NODRAW) != 0)
	{
		EmitSound("items/itembk2.wav", CHAN_WEAPON);
		v.effects &= ~EF_NODRAW;
	}

	v.solid = SOLID_TRIGGER;

	SetOrigin(v.origin);
	SetTouch(&CBasePlayerWeapon::DefaultTouch);
	ClearThink();
}


//=========================================================
// AttemptToMaterialize - the weapon is trying to rematerialize,
// should it do so now or wait longer?
//=========================================================
void CBasePlayerWeapon::AttemptToMaterialize()
{
	float time = g_pGameRules->FlWeaponTryRespawn(this);

	if (time == 0)
	{
		Materialize();
		return;
	}

	v.nextthink = time;
}


//=========================================================
// CheckRespawn - a player is taking this weapon, should
// it respawn?
//=========================================================
void CBasePlayerWeapon::CheckRespawn()
{
	switch (g_pGameRules->WeaponShouldRespawn(this))
	{
	case GR_WEAPON_RESPAWN_YES:
		Respawn();
		break;
	case GR_WEAPON_RESPAWN_NO:
		break;
	}
}


//=========================================================
// Respawn- this weapon is already in the world, but it is
// invisible and intangible. Make it visible and tangible.
//=========================================================
CBaseEntity* CBasePlayerWeapon::Respawn()
{
	// make a copy of this weapon that is invisible and inaccessible to players (no touch function). The weapon spawn/respawn code
	// will decide when to make the weapon visible and touchable.
	CBaseEntity* pNewWeapon =
		CBaseEntity::Create(
			(char*)STRING(v.classname),
			g_pGameRules->VecWeaponRespawnSpot(this),
			v.angles,
			*v.owner);

	if (pNewWeapon != nullptr)
	{
		pNewWeapon->v.effects |= EF_NODRAW;
		pNewWeapon->ClearTouch();
		pNewWeapon->SetThink(&CBasePlayerWeapon::AttemptToMaterialize);

		engine::DropToFloor(&v);

		pNewWeapon->v.nextthink = g_pGameRules->FlWeaponRespawnTime(this);
	}

	return pNewWeapon;
}


void CBasePlayerWeapon::DefaultTouch(CBaseEntity* pOther)
{
	if (!pOther->IsPlayer() || !pOther->IsAlive())
	{
		return;
	}

	CBasePlayer* pPlayer = (CBasePlayer*)pOther;

	if (AddToPlayer(pPlayer))
	{
		if (!pPlayer->m_bIsSpawning)
		{
			pPlayer->EmitSound("items/gunpickup2.wav", CHAN_ITEM);
		}

		UseTargets(pOther, USE_TOGGLE, 0);
	}
}


void CBasePlayerWeapon::UpdateOnRemove()
{
	RemoveFromPlayer();
	CBaseAnimating::UpdateOnRemove();
}


bool CBasePlayerWeapon::AddToPlayer(CBasePlayer* pPlayer)
{
	if (!g_pGameRules->CanHavePlayerWeapon(pPlayer, this))
	{
		return false;
	}

	if (pPlayer->HasPlayerWeapon(GetID()))
	{
		return AddDuplicate(pPlayer->m_rgpPlayerWeapons[GetID()]);
	}

	m_pPlayer = pPlayer;

	v.owner = v.aiment = &pPlayer->v;

	v.movetype = MOVETYPE_FOLLOW;
	v.solid = SOLID_NOT;
	v.effects = EF_NODRAW;

	ClearTouch();
	ClearThink();

	pPlayer->AddPlayerWeapon(this);

	CheckRespawn();

	if (gEvilImpulse101)
	{
		Remove();
	}

	return true;
}


void CBasePlayerWeapon::RemoveFromPlayer(bool forceSendAnimations)
{
	if (m_pPlayer == nullptr)
	{
		return;
	}

	m_ForceSendAnimations = forceSendAnimations;
	m_pPlayer->RemovePlayerWeapon(this);
	m_ForceSendAnimations = false;

	v.owner = v.aiment = nullptr;
	m_pPlayer = nullptr;
}


void CBasePlayerWeapon::SendWeaponAnim(int iAnim)
{
	const bool skiplocal = !m_ForceSendAnimations;

	m_pPlayer->v.weaponanim = iAnim;

	if (skiplocal && engine::CanSkipPlayer(&m_pPlayer->v))
		return;

	MessageBegin(MSG_ONE, SVC_WEAPONANIM, m_pPlayer);
	WriteByte(iAnim);
	WriteByte(v.body);
	MessageEnd();
}


void CBasePlayerWeapon::Deploy()
{
	const auto info = GetInfo();

	m_pPlayer->v.viewmodel = engine::ModelIndex(info.pszView);
	m_pPlayer->v.weaponmodel = engine::ModelIndex(info.pszPlayer);
	strcpy(m_pPlayer->m_szAnimExtention, info.pszAnimExt);

	SendWeaponAnim(info.iAnims[kWeaponAnimDeploy]);

	m_pPlayer->SetAction(CBasePlayer::Action::Arm);

	m_iNextPrimaryAttack = std::max(m_iNextPrimaryAttack, 500);
}

void CBasePlayerWeapon::Holster()
{
	const auto info = GetInfo();

	SendWeaponAnim(info.iAnims[kWeaponAnimHolster]);

	m_iNextPrimaryAttack = std::max(m_iNextPrimaryAttack, 500);
	m_fInReload = false;
	m_pPlayer->m_iFOV = 0;
}
