//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Whatever
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"


void
CBasePlayer::PlaybackEvent(
	unsigned short event,
	float fParam1,
	float fParam2,
	int iParam1,
	int iParam2,
	qboolean bParam1,
	qboolean bParam2,
	unsigned int flags)
{
	g_engfuncs.pfnPlaybackEvent(
		flags,
		edict(),
		event,
		0.0f,
		pev->origin,
		pev->angles + pev->punchangle,
		fParam1,
		fParam2,
		iParam1,
		iParam2,
		bParam1,
		bParam2);
}


void CBasePlayer::WeaponPostFrame()
{
	ImpulseCommands();

	// check if the player is using a tank
#ifdef GAME_DLL
	if (m_pTank != nullptr)
	{
		return;
	}
#endif

	if (m_iNextAttack > 0)
	{
		return;
	}

	if (m_pActiveWeapon == nullptr)
	{
		return;
	}

	m_pActiveWeapon->WeaponPostFrame();
}


#ifdef CLIENT_DLL
void CBasePlayer::PreThink()
{
	const auto buttonsChanged = m_afButtonLast ^ pev->button;

	m_afButtonPressed = buttonsChanged & pev->button;
	m_afButtonReleased = buttonsChanged & ~pev->button;
}
#endif


void CBasePlayer::PostThink()
{
	// if (g_fGameOver)
	// 	goto pt_end; // intermission or finale

	if (!IsAlive())
	{
		goto pt_end;
	}

#ifdef GAME_DLL
	if (m_pTank != NULL)
	{
		if (m_pTank->OnControls(pev) && 0 == pev->weaponmodel)
		{
			m_pTank->Use(this, this, USE_SET, 2); // try fire the gun
		}
		else
		{ // they've moved off the platform
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = NULL;
		}
	}
#endif

	WeaponPostFrame();

#ifdef GAME_DLL
	if ((FBitSet(pev->flags, FL_ONGROUND)) && (pev->health > 0) && m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD)
	{
		if (pev->watertype == CONTENT_WATER)
		{
			// Did he hit the world or a non-moving entity?
			// BUG - this happens all the time in water, especially when
			// BUG - water has current force
			// if ( !pev->groundentity || VARS(pev->groundentity)->velocity.z == 0 )
			// EmitSound("player/pl_wade1.wav", CHAN_VOICE);
		}
		else if (m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED)
		{ // after this point, we start doing damage

			float flFallDamage = g_pGameRules->FlPlayerFallDamage(this);

			if (flFallDamage > 0)
			{
				TakeDamage(CWorld::World, CWorld::World, flFallDamage, DMG_FALL);

				if (pev->health <= 0)
				{
					EmitSound("common/bodysplat.wav", CHAN_VOICE);
				}
			}
		}
	}

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		m_flFallVelocity = 0;
	}
#endif

pt_end:
	StudioFrameAdvance();

	m_afButtonLast = pev->button;
}


void CBasePlayer::GetClientData(clientdata_t& data, bool sendWeapons)
{
#ifdef GAME_DLL
	data.flags = pev->flags;
    data.deadflag = pev->deadflag;

	if (IsSpectator() || IsAlive())
	{
		data.health = std::max(pev->health, 1.0F);
	}
	else
	{
		data.health = 0.0F;
	}
	
	data.vuser4.z = pev->armorvalue;

	data.waterlevel = pev->waterlevel;
	data.watertype = pev->watertype;

	data.origin = pev->origin;
	data.velocity = pev->velocity;
	data.view_ofs = pev->view_ofs;

	data.bInDuck = pev->bInDuck;
	data.flTimeStepSound = pev->flTimeStepSound;
	data.flDuckTime = pev->flDuckTime;
	data.flSwimTime = pev->flSwimTime;
	data.waterjumptime = pev->teleport_time;

	data.iuser1 = pev->iuser1;
	data.iuser2 = pev->iuser2;
	data.iuser3 = pev->iuser3;
#endif

	data.m_iId = (m_pActiveWeapon != nullptr) ? m_pActiveWeapon->m_iId : WEAPON_NONE;

	byte* ammo = reinterpret_cast<byte*>(&data.ammo_shells);
	ammo[0] = m_rgAmmo[AMMO_9MM];
	ammo[1] = m_rgAmmo[AMMO_ARGRENADES];

	data.viewmodel = pev->viewmodel;
    data.maxspeed = pev->maxspeed;
	data.weaponanim = pev->weaponanim;
	data.punchangle = pev->punchangle;

	data.fov = m_iFOV;
    *reinterpret_cast<int*>(&data.m_flNextAttack) = m_iNextAttack;
}


void CBasePlayer::SetClientData(const clientdata_t& data)
{
	pev->health = data.health;
    pev->deadflag = data.deadflag;

	pev->armorvalue = data.vuser4.z;

	pev->waterlevel = data.waterlevel;
	pev->watertype = data.watertype;

	pev->origin = data.origin;
	pev->velocity = data.velocity;
	pev->view_ofs = data.view_ofs;

	pev->bInDuck = data.bInDuck;
	pev->flTimeStepSound = data.flTimeStepSound;
	pev->flDuckTime = data.flDuckTime;
	pev->flSwimTime = data.flSwimTime;
	pev->teleport_time = data.waterjumptime;

	pev->iuser1 = data.iuser1;
	pev->iuser2 = data.iuser2;
	pev->iuser3 = data.iuser3;

	if (m_pActiveWeapon == nullptr)
	{
		if (data.m_iId != WEAPON_NONE)
		{
			m_pActiveWeapon = m_rgpPlayerWeapons[data.m_iId];
		}
	}
	else if (data.m_iId != m_pActiveWeapon->m_iId)
	{
		m_pActiveWeapon = m_rgpPlayerWeapons[data.m_iId];
	}

	const byte* ammo = reinterpret_cast<const byte*>(&data.ammo_shells);
	m_rgAmmo[AMMO_9MM] = ammo[0];
	m_rgAmmo[AMMO_ARGRENADES] = ammo[1];

	pev->viewmodel = data.viewmodel;
	pev->maxspeed = data.maxspeed;
	pev->weaponanim = data.weaponanim;
	pev->punchangle = data.punchangle;

	m_iFOV = data.fov;
    m_iNextAttack = *reinterpret_cast<const int*>(&data.m_flNextAttack);
}


void CBasePlayer::DecrementTimers(const int msec)
{
	float len = VectorNormalize(pev->punchangle);
	if (len != 0.0F)
	{
		len -= (10.0F + len * 0.5F) * (msec / 1000.0F);
		len = std::max(len, 0.0F);
		pev->punchangle = pev->punchangle * len;
	}

	m_iNextAttack = std::max(m_iNextAttack - msec, -1);
}


void CBasePlayer::SelectWeapon(int id)
{
	if (id <= WEAPON_NONE || id >= WEAPON_LAST)
	{
		return;
	}

	auto weapon = m_rgpPlayerWeapons[id];

	if (weapon == nullptr || weapon == m_pActiveWeapon)
	{
		return;
	}

	if (m_pActiveWeapon != nullptr)
	{
		m_pActiveWeapon->Holster();
	}
	m_pActiveWeapon = weapon;
	m_pActiveWeapon->Deploy();
}


void CBasePlayer::CmdStart(const usercmd_t& cmd, unsigned int randomSeed)
{
	if (cmd.weaponselect != WEAPON_NONE)
	{
        SelectWeapon(cmd.weaponselect);
		((usercmd_t*)&cmd)->weaponselect = WEAPON_NONE;
	}

	m_randomSeed = randomSeed;
}

