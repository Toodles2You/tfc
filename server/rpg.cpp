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
#include "UserMessages.h"

LINK_ENTITY_TO_CLASS(weapon_rpg, CRpg);

#ifndef CLIENT_DLL

LINK_ENTITY_TO_CLASS(laser_spot, CLaserSpot);

//=========================================================
//=========================================================
CLaserSpot* CLaserSpot::CreateSpot(CBaseEntity* pOwner)
{
	CLaserSpot* pSpot = GetClassPtr((CLaserSpot*)NULL);
	pSpot->Spawn();

	pSpot->pev->classname = MAKE_STRING("laser_spot");
	pSpot->pev->owner = pOwner->edict();
	pSpot->pev->flags |= FL_SKIPLOCALHOST;

	return pSpot;
}

//=========================================================
//=========================================================
bool CLaserSpot::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	pev->rendermode = kRenderGlow;
	pev->renderfx = kRenderFxNoDissipation;
	pev->renderamt = 255;

	SetModel("sprites/laserdot.spr");
	SetOrigin(pev->origin);

	return true;
}

//=========================================================
// Suspend- make the laser sight invisible.
//=========================================================
void CLaserSpot::Suspend(float flSuspendTime)
{
	pev->effects |= EF_NODRAW;

	SetThink(&CLaserSpot::Revive);
	pev->nextthink = gpGlobals->time + flSuspendTime;
}

//=========================================================
// Revive - bring a suspended laser sight back.
//=========================================================
void CLaserSpot::Revive()
{
	pev->effects &= ~EF_NODRAW;

	SetThink(NULL);
}

void CLaserSpot::Precache()
{
	PRECACHE_MODEL("sprites/laserdot.spr");
};

LINK_ENTITY_TO_CLASS(rpg_rocket, CRpgRocket);

CRpgRocket::~CRpgRocket()
{
	if (m_pLauncher)
	{
		// my launcher is still around, tell it I'm dead.
		static_cast<CRpg*>(static_cast<CBaseEntity*>(m_pLauncher))->m_cActiveRockets--;
	}
}

//=========================================================
//=========================================================
CRpgRocket* CRpgRocket::CreateRpgRocket(Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner, CRpg* pLauncher)
{
	CRpgRocket* pRocket = GetClassPtr((CRpgRocket*)NULL);

	pRocket->SetOrigin(vecOrigin);
	pRocket->pev->angles = vecAngles;
	pRocket->Spawn();
	pRocket->SetTouch(&CRpgRocket::RocketTouch);
	pRocket->m_pLauncher = pLauncher; // remember what RPG fired me.
	pLauncher->m_cActiveRockets++;	  // register this missile as active for the launcher
	pRocket->pev->owner = pOwner->edict();

	return pRocket;
}

//=========================================================
//=========================================================
bool CRpgRocket::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SetModel("models/rpgrocket.mdl");
	SetSize(g_vecZero, g_vecZero);
	SetOrigin(pev->origin);

	pev->classname = MAKE_STRING("rpg_rocket");

	SetThink(&CRpgRocket::IgniteThink);
	SetTouch(&CRpgRocket::ExplodeTouch);

	pev->angles.x -= 30;
	UTIL_MakeVectors(pev->angles);
	pev->angles.x = -(pev->angles.x + 30);

	pev->velocity = gpGlobals->v_forward * 250;
	pev->gravity = 0.5;

	pev->nextthink = gpGlobals->time + 0.4;

	pev->dmg = gSkillData.plrDmgRPG;
	
	return true;
}

//=========================================================
//=========================================================
void CRpgRocket::RocketTouch(CBaseEntity* pOther)
{
	StopSound("weapons/rocket1.wav", CHAN_VOICE);
	ExplodeTouch(pOther);
}

//=========================================================
//=========================================================
void CRpgRocket::Precache()
{
	PRECACHE_MODEL("models/rpgrocket.mdl");
	m_iTrail = PRECACHE_MODEL("sprites/smoke.spr");
	PRECACHE_SOUND("weapons/rocket1.wav");
}


void CRpgRocket::IgniteThink()
{
	// pev->movetype = MOVETYPE_TOSS;

	pev->movetype = MOVETYPE_FLY;
	pev->effects |= EF_LIGHT;

	// make rocket sound
	EmitSound("weapons/rocket1.wav", CHAN_VOICE, VOL_NORM, 0.5);

	// rocket trail
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);

	WRITE_BYTE(TE_BEAMFOLLOW);
	WRITE_SHORT(entindex()); // entity
	WRITE_SHORT(m_iTrail);	 // model
	WRITE_BYTE(40);			 // life
	WRITE_BYTE(5);			 // width
	WRITE_BYTE(224);		 // r, g, b
	WRITE_BYTE(224);		 // r, g, b
	WRITE_BYTE(255);		 // r, g, b
	WRITE_BYTE(255);		 // brightness

	MESSAGE_END(); // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	m_flIgniteTime = gpGlobals->time;

	// set to follow laser spot
	SetThink(&CRpgRocket::FollowThink);
	pev->nextthink = gpGlobals->time + 0.1;
}


void CRpgRocket::FollowThink()
{
	CBaseEntity* pOther = NULL;
	Vector vecTarget;
	Vector vecDir;
	float flDist, flMax, flDot;
	TraceResult tr;

	UTIL_MakeAimVectors(pev->angles);

	vecTarget = gpGlobals->v_forward;
	flMax = 4096;

	// Examine all entities within a reasonable radius
	while ((pOther = UTIL_FindEntityByClassname(pOther, "laser_spot")) != NULL)
	{
		UTIL_TraceLine(pev->origin, pOther->pev->origin, dont_ignore_monsters, this, &tr);
		// ALERT( at_console, "%f\n", tr.flFraction );
		if (tr.flFraction >= 0.90)
		{
			vecDir = pOther->pev->origin - pev->origin;
			flDist = vecDir.Length();
			vecDir = vecDir.Normalize();
			flDot = DotProduct(gpGlobals->v_forward, vecDir);
			if ((flDot > 0) && (flDist * (1 - flDot) < flMax))
			{
				flMax = flDist * (1 - flDot);
				vecTarget = vecDir;
			}
		}
	}

	pev->angles = UTIL_VecToAngles(vecTarget);

	// this acceleration and turning math is totally wrong, but it seems to respond well so don't change it.
	float flSpeed = pev->velocity.Length();
	if (gpGlobals->time - m_flIgniteTime < 1.0)
	{
		pev->velocity = pev->velocity * 0.2 + vecTarget * (flSpeed * 0.8 + 400);
		if (pev->waterlevel == 3)
		{
			// go slow underwater
			if (pev->velocity.Length() > 300)
			{
				pev->velocity = pev->velocity.Normalize() * 300;
			}
			UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 4);
		}
		else
		{
			if (pev->velocity.Length() > 2000)
			{
				pev->velocity = pev->velocity.Normalize() * 2000;
			}
		}
	}
	else
	{
		if ((pev->effects & EF_LIGHT) != 0)
		{
			pev->effects = 0;
			StopSound("weapons/rocket1.wav", CHAN_VOICE);
		}
		pev->velocity = pev->velocity * 0.2 + vecTarget * flSpeed * 0.798;
		if (pev->waterlevel == 0 && pev->velocity.Length() < 1500)
		{
			Detonate();
		}
	}
	// ALERT( at_console, "%.0f\n", flSpeed );

	pev->nextthink = gpGlobals->time + 0.1;
}
#endif



void CRpg::Reload()
{
	// because the RPG waits to autoreload when no missiles are active while  the LTD is on, the
	// weapons code is constantly calling into this function, but is often denied because
	// a) missiles are in flight, but the LTD is on
	// or
	// b) player is totally out of ammo and has nothing to switch to, and should be allowed to
	//    shine the designator around
	//
	// Set the next attack time into the future so that WeaponIdle will get called more often
	// than reload, allowing the RPG LTD to be updated

	m_iNextPrimaryAttack = 500;

	if (0 != m_cActiveRockets && m_fSpotActive)
	{
		// no reloading when there are active missiles tracking the designator.
		// ward off future autoreload attempts by setting next attack time into the future for a bit.
		return;
	}

	if (DefaultReload(RPG_MAX_CLIP, RPG_RELOAD, 2000))
	{
		if (m_pSpot && m_fSpotActive)
		{
			SuspendLaserDot(2.1);
			m_iNextSecondaryAttack = 2100;
		}

		m_iTimeWeaponIdle = UTIL_SharedRandomLong(m_pPlayer->random_seed, 10000, 15000);
	}
}

bool CRpg::Spawn()
{
	Precache();
	m_iId = WEAPON_RPG;

	SetModel("models/w_rpg.mdl");
	m_fSpotActive = true;

	if (UTIL_IsDeathmatch())
	{
		// more default ammo in multiplay.
		m_iDefaultAmmo = RPG_DEFAULT_GIVE * 2;
	}
	else
	{
		m_iDefaultAmmo = RPG_DEFAULT_GIVE;
	}

	FallInit(); // get ready to fall down.

	return true;
}


void CRpg::Precache()
{
	PRECACHE_MODEL("models/w_rpg.mdl");
	PRECACHE_MODEL("models/v_rpg.mdl");
	PRECACHE_MODEL("models/p_rpg.mdl");

	UTIL_PrecacheOther("laser_spot");
	UTIL_PrecacheOther("rpg_rocket");

	PRECACHE_SOUND("weapons/rocketfire1.wav");
	PRECACHE_SOUND("weapons/glauncher.wav"); // alternative fire sound

	m_usRpg = PRECACHE_EVENT(1, "events/rpg.sc");
	m_usLaserDotOn = PRECACHE_EVENT(1, "events/laser_on.sc");
	m_usLaserDotOff = PRECACHE_EVENT(1, "events/laser_off.sc");
}


bool CRpg::GetWeaponInfo(WeaponInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->iAmmo1 = AMMO_ROCKETS;
	p->iMaxAmmo1 = ROCKET_MAX_CARRY;
	p->iAmmo2 = AMMO_NONE;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = RPG_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_RPG;
	p->iFlags = 0;
	p->iWeight = RPG_WEIGHT;

	return true;
}

bool CRpg::Deploy()
{
	if (m_iClip == 0)
	{
		return DefaultDeploy("models/v_rpg.mdl", "models/p_rpg.mdl", RPG_DRAW_UL, "rpg");
	}

	return DefaultDeploy("models/v_rpg.mdl", "models/p_rpg.mdl", RPG_DRAW1, "rpg");
}


bool CRpg::CanHolster()
{
	if (m_fSpotActive && 0 != m_cActiveRockets)
	{
		// can't put away while guiding a missile.
		return false;
	}

	return true;
}

bool CRpg::Holster()
{
	if (DefaultHolster(0 != m_iClip ? RPG_HOLSTER1 : RPG_HOLSTER2))
	{
		ToggleLaserDot(false, false);
		return true;
	}
	return false;
}



void CRpg::PrimaryAttack()
{
	if (0 != m_iClip)
	{
#ifndef CLIENT_DLL
		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		UTIL_MakeVectors(m_pPlayer->pev->v_angle);
		Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -8;

		CRpgRocket* pRocket = CRpgRocket::CreateRpgRocket(vecSrc, m_pPlayer->pev->v_angle, m_pPlayer, this);

		UTIL_MakeVectors(m_pPlayer->pev->v_angle); // RpgRocket::Create stomps on globals, so remake.
		pRocket->pev->velocity = pRocket->pev->velocity + gpGlobals->v_forward * DotProduct(m_pPlayer->pev->velocity, gpGlobals->v_forward);
#endif

		m_pPlayer->PlaybackEvent(m_usRpg);

		m_iClip--;

		m_iNextPrimaryAttack = 1500;
		m_iTimeWeaponIdle = 1500;
	}
	else
	{
		PlayEmptySound();
	}
	UpdateSpot();
}


void CRpg::SecondaryAttack()
{
	m_fSpotActive = !m_fSpotActive;

	if (!m_fSpotActive)
	{
		ToggleLaserDot(false);
	}

	m_iNextSecondaryAttack = 200;
}


void CRpg::WeaponIdle()
{
	UpdateSpot();

	if (m_iTimeWeaponIdle > 0)
		return;

	if (0 != m_pPlayer->m_rgAmmo[iAmmo1()])
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
		if (flRand <= 0.75 || m_fSpotActive)
		{
			if (m_iClip == 0)
				iAnim = RPG_IDLE_UL;
			else
				iAnim = RPG_IDLE;

			m_iTimeWeaponIdle = 6000;
		}
		else
		{
			if (m_iClip == 0)
				iAnim = RPG_FIDGET_UL;
			else
				iAnim = RPG_FIDGET;

			m_iTimeWeaponIdle = 6100;
		}

		SendWeaponAnim(iAnim);
	}
	else
	{
		m_iTimeWeaponIdle = 1000;
	}
}



void CRpg::UpdateSpot()
{
	// Don't turn on the laser if we're in the middle of a reload.
	if (m_fInReload)
	{
		return;
	}

	if (m_fSpotActive)
	{
		if (!m_pSpot)
		{
			ToggleLaserDot(true);
		}

#ifndef CLIENT_DLL
		UTIL_MakeVectors(m_pPlayer->pev->v_angle);
		Vector vecSrc = m_pPlayer->GetGunPosition();
		Vector vecAiming = gpGlobals->v_forward;

		TraceResult tr;
		UTIL_TraceLine(vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, m_pPlayer, &tr);

		m_pSpot->SetOrigin(tr.vecEndPos);
#endif
	}
}

void CRpg::ToggleLaserDot(bool bOn, bool bSound)
{
	if (bOn)
	{
		if (m_pSpot == nullptr)
		{
#ifdef CLIENT_DLL
			m_pSpot = (CLaserSpot*)1;
#else
			m_pSpot = CLaserSpot::CreateSpot(m_pPlayer);
#endif
		}
	}
	else
	{
		if (m_pSpot != nullptr)
		{
#ifndef CLIENT_DLL
			m_pSpot->Killed(nullptr, nullptr, DMG_NEVERGIB);
#endif
			m_pSpot = nullptr;
		}
	}

	m_pPlayer->PlaybackEvent(
		bOn ? m_usLaserDotOn : m_usLaserDotOff,
		0.0,
		0.0,
		bSound ? 1 : 0,
		0,
		false,
		false,
		FEV_GLOBAL | FEV_RELIABLE);
}

void CRpg::SuspendLaserDot(float flSuspendTime)
{
	if (m_pSpot == nullptr)
		return;

	m_pPlayer->PlaybackEvent(
		m_usLaserDotOn,
		flSuspendTime,
		0.0,
		0,
		0,
		false,
		false,
		FEV_GLOBAL | FEV_RELIABLE | FEV_UPDATE);
}

IMPLEMENT_AMMO_CLASS(ammo_rpgclip, CRpgAmmo, "models/w_rpgammo.mdl", (UTIL_IsDeathmatch() ? AMMO_RPGCLIP_GIVE * 2 : AMMO_RPGCLIP_GIVE), AMMO_ROCKETS, ROCKET_MAX_CARRY);
