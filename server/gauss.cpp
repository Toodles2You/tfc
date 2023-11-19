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
#include "shake.h"
#include "gamerules.h"
#include "UserMessages.h"

LINK_ENTITY_TO_CLASS(weapon_gauss, CGauss);

float CGauss::GetFullChargeTime()
{
	if (UTIL_IsDeathmatch())
	{
		return 1.5;
	}

	return 4;
}

#ifdef CLIENT_DLL
extern bool g_irunninggausspred;
#endif

void CGauss::Spawn()
{
	Precache();
	m_iId = WEAPON_GAUSS;
	SET_MODEL(ENT(pev), "models/w_gauss.mdl");

	m_iDefaultAmmo = GAUSS_DEFAULT_GIVE;

	FallInit(); // get ready to fall down.
}


void CGauss::Precache()
{
	PRECACHE_MODEL("models/w_gauss.mdl");
	PRECACHE_MODEL("models/v_gauss.mdl");
	PRECACHE_MODEL("models/p_gauss.mdl");

	PRECACHE_SOUND("weapons/gauss2.wav");
	PRECACHE_SOUND("weapons/electro4.wav");
	PRECACHE_SOUND("weapons/electro5.wav");
	PRECACHE_SOUND("weapons/electro6.wav");
	PRECACHE_SOUND("ambience/pulsemachine.wav");

	m_iGlow = PRECACHE_MODEL("sprites/hotglow.spr");
	m_iBalls = PRECACHE_MODEL("sprites/hotglow.spr");
	m_iBeam = PRECACHE_MODEL("sprites/smoke.spr");

	m_usGaussFire = PRECACHE_EVENT(1, "events/gauss.sc");
	m_usGaussSpin = PRECACHE_EVENT(1, "events/gaussspin.sc");
}

bool CGauss::GetWeaponInfo(WeaponInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->iAmmo1 = AMMO_URANIUM;
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
	p->iAmmo2 = AMMO_NONE;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_GAUSS;
	p->iFlags = 0;
	p->iWeight = GAUSS_WEIGHT;

	return true;
}

bool CGauss::Deploy()
{
	return DefaultDeploy("models/v_gauss.mdl", "models/p_gauss.mdl", GAUSS_DRAW, "gauss");
}

bool CGauss::Holster()
{
	if (DefaultHolster(GAUSS_HOLSTER))
	{
		SendStopEvent(true);
		m_fInAttack = 0;
		return true;
	}
	return false;
}


void CGauss::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_iNextSecondaryAttack = m_iNextPrimaryAttack = 150;
		return;
	}

	if (m_pPlayer->m_rgAmmo[iAmmo1()] < 2)
	{
		PlayEmptySound();
		m_pPlayer->m_iNextAttack = 500;
		return;
	}

	m_fPrimaryFire = true;

	m_pPlayer->m_rgAmmo[iAmmo1()] -= 2;

	StartFire();
	m_fInAttack = 0;
	m_iTimeWeaponIdle = 1000;
	m_pPlayer->m_iNextAttack = 200;
}

void CGauss::SecondaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		if (m_fInAttack != 0)
		{
			PlayWeaponSound(CHAN_ITEM, "weapons/electro4.wav", 1.0, ATTN_NORM, 0, 80 + UTIL_SharedRandomLong(m_pPlayer->random_seed, 0, 0x3f));
			//Have to send to the host as well because the client will predict the frame with m_fInAttack == 0
			SendStopEvent(true);
			SendWeaponAnim(GAUSS_IDLE);
			m_fInAttack = 0;
		}
		else
		{
			PlayEmptySound();
		}

		m_iNextSecondaryAttack = m_iNextPrimaryAttack = 500;
		return;
	}

	if (m_fInAttack == 0)
	{
		if (m_pPlayer->m_rgAmmo[iAmmo1()] <= 0)
		{
			PlayEmptySound();
			m_pPlayer->m_iNextAttack = 500;
			return;
		}

		m_fPrimaryFire = false;

		m_pPlayer->m_rgAmmo[iAmmo1()]--; // take one ammo just to start the spin
		m_pPlayer->m_flNextAmmoBurn = UTIL_WeaponTimeBase();

		// spin up

		SendWeaponAnim(GAUSS_SPINUP);
		m_fInAttack = 1;
		m_iTimeWeaponIdle = 500;
		m_pPlayer->m_flStartCharge = gpGlobals->time;
		m_pPlayer->m_flAmmoStartCharge = UTIL_WeaponTimeBase() + GetFullChargeTime();

		m_pPlayer->PlaybackEvent(m_usGaussSpin, 0.0, 0.0, 110);

		m_iSoundState = SND_CHANGE_PITCH;
	}
	else if (m_fInAttack == 1)
	{
		if (m_iTimeWeaponIdle <= 0)
		{
			SendWeaponAnim(GAUSS_SPIN);
			m_fInAttack = 2;
		}
	}
	else
	{
		if (m_pPlayer->m_rgAmmo[iAmmo1()] > 0)
		{
			// during the charging process, eat one bit of ammo every once in a while
			if (UTIL_WeaponTimeBase() >= m_pPlayer->m_flNextAmmoBurn && m_pPlayer->m_flNextAmmoBurn != 1000)
			{
				if (UTIL_IsDeathmatch())
				{
					m_pPlayer->m_rgAmmo[iAmmo1()]--;
					m_pPlayer->m_flNextAmmoBurn = UTIL_WeaponTimeBase() + 0.1;
				}
				else
				{
					m_pPlayer->m_rgAmmo[iAmmo1()]--;
					m_pPlayer->m_flNextAmmoBurn = UTIL_WeaponTimeBase() + 0.3;
				}
			}
		}

		if (m_pPlayer->m_rgAmmo[iAmmo1()] <= 0)
		{
			// out of ammo! force the gun to fire
			StartFire();
			m_fInAttack = 0;
			m_iTimeWeaponIdle = 1000;
			m_pPlayer->m_iNextAttack = 1000;
			return;
		}

		if (UTIL_WeaponTimeBase() >= m_pPlayer->m_flAmmoStartCharge)
		{
			// don't eat any more ammo after gun is fully charged.
			m_pPlayer->m_flNextAmmoBurn = 1000;
		}

		int pitch = (gpGlobals->time - m_pPlayer->m_flStartCharge) * (150 / GetFullChargeTime()) + 100;
		if (pitch > 250)
			pitch = 250;

		// ALERT( at_console, "%d %d %d\n", m_fInAttack, m_iSoundState, pitch );

		if (m_iSoundState == 0)
			ALERT(at_console, "sound state %d\n", m_iSoundState);

		m_pPlayer->PlaybackEvent(m_usGaussSpin, 0.0, 0.0, pitch, 0, m_iSoundState == SND_CHANGE_PITCH);

		m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions

		// m_iTimeWeaponIdle = 100;
		if (m_pPlayer->m_flStartCharge < gpGlobals->time - 10)
		{
			// Player charged up too long. Zap him.
			PlayWeaponSound(CHAN_WEAPON, "weapons/electro4.wav", 1.0, ATTN_NORM, 0, 80 + UTIL_SharedRandomLong(m_pPlayer->random_seed, 0, 0x3f));
			PlayWeaponSound(CHAN_ITEM, "weapons/electro6.wav", 1.0, ATTN_NORM, 0, 75 + UTIL_SharedRandomLong(m_pPlayer->random_seed, 0, 0x3f));

			m_fInAttack = 0;
			m_iTimeWeaponIdle = 1000;
			m_pPlayer->m_iNextAttack = 1000;

			SendStopEvent(false);

#ifndef CLIENT_DLL
			m_pPlayer->TakeDamage(CWorld::World->pev, CWorld::World->pev, 50, DMG_SHOCK);
			UTIL_ScreenFade(m_pPlayer, Vector(255, 128, 0), 2, 0.5, 128, FFADE_IN);
#endif
			SendWeaponAnim(GAUSS_IDLE);

			// Player may have been killed and this weapon dropped, don't execute any more code after this!
			return;
		}
	}
}

//=========================================================
// StartFire- since all of this code has to run and then
// call Fire(), it was easier at this point to rip it out
// of weaponidle() and make its own function then to try to
// merge this into Fire(), which has some identical variable names
//=========================================================
void CGauss::StartFire()
{
	float flDamage;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition(); // + gpGlobals->v_up * -8 + gpGlobals->v_right * 8;

	if (gpGlobals->time - m_pPlayer->m_flStartCharge > GetFullChargeTime())
	{
		flDamage = 200;
	}
	else
	{
		flDamage = 200 * ((gpGlobals->time - m_pPlayer->m_flStartCharge) / GetFullChargeTime());
	}

	if (m_fPrimaryFire)
	{
		// fixed damage on primary attack
#ifdef CLIENT_DLL
		flDamage = 20;
#else
		flDamage = gSkillData.plrDmgGauss;
#endif
	}

	if (m_fInAttack != 3)
	{
		//ALERT ( at_console, "Time:%f Damage:%f\n", gpGlobals->time - m_pPlayer->m_flStartCharge, flDamage );

#ifndef CLIENT_DLL
		float flZVel = m_pPlayer->pev->velocity.z;

		if (!m_fPrimaryFire)
		{
			m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * flDamage * 5;
		}

		if (!UTIL_IsDeathmatch())
		{
			// in deathmatch, gauss can pop you up into the air. Not in single play.
			m_pPlayer->pev->velocity.z = flZVel;
		}
#endif
		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	}

	Fire(vecSrc, vecAiming, flDamage);
}

void CGauss::Fire(Vector vecOrigSrc, Vector vecDir, float flDamage)
{
	Vector vecSrc = vecOrigSrc;
	Vector vecDest = vecSrc + vecDir * 8192;
	edict_t* pentIgnore;
	TraceResult tr, beam_tr;
	float flMaxFrac = 1.0;
	int nTotal = 0;
	bool fHasPunched = false;
	bool fFirstBeam = true;
	int nMaxHits = 10;

	pentIgnore = ENT(m_pPlayer->pev);

#ifdef CLIENT_DLL
	if (m_fPrimaryFire == false)
		g_irunninggausspred = true;
#endif

	// The main firing event is sent unreliably so it won't be delayed.
	m_pPlayer->PlaybackEvent(m_usGaussFire, flDamage, 0.0, 0, 0, m_fPrimaryFire);

	SendStopEvent(false);


	/*ALERT( at_console, "%f %f %f\n%f %f %f\n", 
		vecSrc.x, vecSrc.y, vecSrc.z, 
		vecDest.x, vecDest.y, vecDest.z );*/


	//	ALERT( at_console, "%f %f\n", tr.flFraction, flMaxFrac );

#ifndef CLIENT_DLL
	while (flDamage > 10 && nMaxHits > 0)
	{
		nMaxHits--;

		// ALERT( at_console, "." );
		UTIL_TraceLine(vecSrc, vecDest, dont_ignore_monsters, pentIgnore, &tr);

		if (0 != tr.fAllSolid)
			break;

		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

		if (pEntity == NULL)
			break;

		if (fFirstBeam)
		{
			fFirstBeam = false;

			nTotal += 26;
		}

		if (0 != pEntity->pev->takedamage)
		{
			ClearMultiDamage();
			pEntity->TraceAttack(m_pPlayer->pev, flDamage, vecDir, &tr, DMG_BULLET);
			ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

			MESSAGE_BEGIN(MSG_PVS, gmsgBlood, pEntity->pev->origin);
			WRITE_FLOAT(vecDir.x);
			WRITE_FLOAT(vecDir.y);
			WRITE_FLOAT(vecDir.z);
			WRITE_BYTE(0);
			WRITE_BYTE(0);
			WRITE_COORD(tr.vecEndPos.x);
			WRITE_COORD(tr.vecEndPos.y);
			WRITE_COORD(tr.vecEndPos.z);
			MESSAGE_END();
		}

		if (pEntity->ReflectGauss())
		{
			float n;

			pentIgnore = NULL;

			n = -DotProduct(tr.vecPlaneNormal, vecDir);

			if (n < 0.5) // 60 degrees
			{
				// ALERT( at_console, "reflect %f\n", n );
				// reflect
				Vector r;

				r = 2.0 * tr.vecPlaneNormal * n + vecDir;
				flMaxFrac = flMaxFrac - tr.flFraction;
				vecDir = r;
				vecSrc = tr.vecEndPos + vecDir * 8;
				vecDest = vecSrc + vecDir * 8192;

				// explode a bit
				RadiusDamage(tr.vecEndPos, pev, m_pPlayer->pev, flDamage * n, flDamage * n * 2.5, CLASS_NONE, DMG_BLAST);

				nTotal += 34;

				// lose energy
				if (n == 0)
					n = 0.1;
				flDamage = flDamage * (1 - n);
			}
			else
			{
				nTotal += 13;

				// limit it to one hole punch
				if (fHasPunched)
					break;
				fHasPunched = true;

				// try punching through wall if secondary attack (primary is incapable of breaking through)
				if (!m_fPrimaryFire)
				{
					UTIL_TraceLine(tr.vecEndPos + vecDir * 8, vecDest, dont_ignore_monsters, pentIgnore, &beam_tr);
					if (0 == beam_tr.fAllSolid)
					{
						// trace backwards to find exit point
						UTIL_TraceLine(beam_tr.vecEndPos, tr.vecEndPos, dont_ignore_monsters, pentIgnore, &beam_tr);

						float n = (beam_tr.vecEndPos - tr.vecEndPos).Length();

						if (n < flDamage)
						{
							if (n == 0)
								n = 1;
							flDamage -= n;

							// ALERT( at_console, "punch %f\n", n );
							nTotal += 21;

							// exit blast damage
							//m_pPlayer->RadiusDamage( beam_tr.vecEndPos + vecDir * 8, pev, m_pPlayer->pev, flDamage, CLASS_NONE, DMG_BLAST );
							float damage_radius;


							if (UTIL_IsDeathmatch())
							{
								damage_radius = flDamage * 1.75; // Old code == 2.5
							}
							else
							{
								damage_radius = flDamage * 2.5;
							}

							::RadiusDamage(beam_tr.vecEndPos + vecDir * 8, pev, m_pPlayer->pev, flDamage, damage_radius, CLASS_NONE, DMG_BLAST);

							nTotal += 53;

							vecSrc = beam_tr.vecEndPos + vecDir;
						}
					}
					else
					{
						//ALERT( at_console, "blocked %f\n", n );
						flDamage = 0;
					}
				}
				else
				{
					//ALERT( at_console, "blocked solid\n" );

					flDamage = 0;
				}
			}
		}
		else
		{
			vecSrc = tr.vecEndPos + vecDir;
			pentIgnore = ENT(pEntity->pev);
		}
	}
#endif
	// ALERT( at_console, "%d bytes\n", nTotal );
}




void CGauss::WeaponIdle()
{
	if (m_iTimeWeaponIdle > 0)
		return;

	if (m_fInAttack != 0)
	{
		StartFire();
		m_fInAttack = 0;
		m_iTimeWeaponIdle = 2000;
	}
	else
	{
		int iAnim;
		float flRand = RANDOM_FLOAT(0, 1);
		if (flRand <= 0.5)
		{
			iAnim = GAUSS_IDLE;
			m_iTimeWeaponIdle = UTIL_SharedRandomLong(m_pPlayer->random_seed, 10000, 15000);
		}
		else if (flRand <= 0.75)
		{
			iAnim = GAUSS_IDLE2;
			m_iTimeWeaponIdle = UTIL_SharedRandomLong(m_pPlayer->random_seed, 10000, 15000);
		}
		else
		{
			iAnim = GAUSS_FIDGET;
			m_iTimeWeaponIdle = 3000;
		}

		return;
		SendWeaponAnim(iAnim);
	}
}

void CGauss::SendStopEvent(bool sendToHost)
{
	// This reliable event is used to stop the spinning sound
	// It's delayed by a fraction of second to make sure it is delayed by 1 frame on the client
	// It's sent reliably anyway, which could lead to other delays

	unsigned int flags = FEV_RELIABLE | FEV_GLOBAL;

	if (!sendToHost)
	{
		flags |= FEV_NOTHOST;
	}

	m_pPlayer->PlaybackEvent(m_usGaussFire, 0.0, 0.0, 0, 0, false, true, flags);
}


IMPLEMENT_AMMO_CLASS(ammo_gaussclip, CGaussAmmo, "models/w_gaussammo.mdl", AMMO_URANIUMBOX_GIVE, AMMO_URANIUM, URANIUM_MAX_CARRY);
