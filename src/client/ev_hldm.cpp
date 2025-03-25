/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
#include "hud.h"
#include "cl_util.h"

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"

#include "com_weapons.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "entity_types.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_materials.h"

#include "eventscripts.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"
#include "view.h"
#include "parsemsg.h"
#include "com_model.h"

#include <string.h>

static int tracerCount[MAX_PLAYERS];

#include "pm_shared.h"

extern cvar_t* r_decals;
extern cvar_t* violence_hblood;
extern cvar_t* violence_hgibs;

extern int gTempEntCount;

static byte debrisFrameCount;

static float EV_WaterLevel(const Vector& position, float minz, float maxz)
{
	Vector midUp = position;
	midUp.z = minz;

	if (client::PM_PointContents(midUp, nullptr) != CONTENTS_WATER)
	{
		return minz;
	}

	midUp.z = maxz;
	if (client::PM_PointContents(midUp, nullptr) == CONTENTS_WATER)
	{
		return maxz;
	}

	float diff = maxz - minz;
	while (diff > 1.0)
	{
		midUp.z = minz + diff / 2.0;
		if (client::PM_PointContents(midUp, nullptr) == CONTENTS_WATER)
		{
			minz = midUp.z;
		}
		else
		{
			maxz = midUp.z;
		}
		diff = maxz - minz;
	}

	return midUp.z;
}

static void EV_BubbleTrail(Vector from, Vector to, int count)
{
	float flHeight = EV_WaterLevel(from, from.z, from.z + 256);
	flHeight = flHeight - from.z;

	if (flHeight < 8)
	{
		flHeight = EV_WaterLevel(to, to.z, to.z + 256);
		flHeight = flHeight - to.z;
		if (flHeight < 8)
		{
			return;
		}

		flHeight = flHeight + to.z - from.z;
	}

	client::efx::BubbleTrail(
		from,
		to,
		flHeight,
		g_sModelIndexBubbles,
		std::min(count, 255),
		8
	);
}

static void EV_Bubbles(const Vector& origin, float radius)
{
	auto mins = origin + Vector(-radius, -radius, -radius);
	auto maxs = origin + Vector(radius, radius, radius);
	auto mid = (mins + maxs) * 0.5;

	auto height = EV_WaterLevel(mid, mid.z, mid.z + 1024);
	height = height - mins.z;
	
	client::efx::Bubbles(mins, maxs, height, g_sModelIndexBubbles, 100, 8);
}

// play a strike sound based on the texture that was hit by the attack traceline.  VecSrc/VecEnd are the
// original traceline endpoints used by the attacker.
// returns volume of strike instrument (crowbar) to play
static float EV_PlayTextureSound(int idx, pmtrace_t* ptr, const Vector& vecSrc, const Vector& vecEnd)
{
	// hit the world, try to play sound based on texture material type
	char chTextureType = CHAR_TEX_CONCRETE;
	float fvol;
	float fvolbar;
	const char* rgsz[4];
	int cnt;
	float fattn = ATTN_NORM;
	int entity;
	char* pTextureName;
	char texname[64];
	char szbuffer[64];

	entity = client::event::IndexFromTrace(ptr);

	chTextureType = 0;

	if (entity == 0)
	{
		// get texture from entity or world (world is ent(0))
		pTextureName =
			(char*)client::event::TraceTexture(
				ptr->ent,
				const_cast<Vector&>(vecSrc),
				const_cast<Vector&>(vecEnd));

		if (pTextureName)
		{
			strcpy(texname, pTextureName);
			pTextureName = texname;

			// strip leading '-0' or '+0~' or '{' or '!'
			if (*pTextureName == '-' || *pTextureName == '+')
			{
				pTextureName += 2;
			}

			if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
			{
				pTextureName++;
			}

			// '}}'
			strcpy(szbuffer, pTextureName);
			szbuffer[CBTEXTURENAMEMAX - 1] = 0;

			// get texture type
			chTextureType = PM_FindTextureType(szbuffer);
		}
	}

	switch (chTextureType)
	{
	default:
	case CHAR_TEX_CONCRETE:
		fvol = 0.9;
		fvolbar = 0.6;
		rgsz[0] = "player/pl_step1.wav";
		rgsz[1] = "player/pl_step2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_METAL:
		fvol = 0.9;
		fvolbar = 0.3;
		rgsz[0] = "player/pl_metal1.wav";
		rgsz[1] = "player/pl_metal2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_DIRT:
		fvol = 0.9;
		fvolbar = 0.1;
		rgsz[0] = "player/pl_dirt1.wav";
		rgsz[1] = "player/pl_dirt2.wav";
		rgsz[2] = "player/pl_dirt3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_VENT:
		fvol = 0.5;
		fvolbar = 0.3;
		rgsz[0] = "player/pl_duct1.wav";
		rgsz[1] = "player/pl_duct1.wav";
		cnt = 2;
		break;
	case CHAR_TEX_GRATE:
		fvol = 0.9;
		fvolbar = 0.5;
		rgsz[0] = "player/pl_grate1.wav";
		rgsz[1] = "player/pl_grate4.wav";
		cnt = 2;
		break;
	case CHAR_TEX_TILE:
		fvol = 0.8;
		fvolbar = 0.2;
		rgsz[0] = "player/pl_tile1.wav";
		rgsz[1] = "player/pl_tile3.wav";
		rgsz[2] = "player/pl_tile2.wav";
		rgsz[3] = "player/pl_tile4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_SLOSH:
		fvol = 0.9;
		fvolbar = 0.0;
		rgsz[0] = "player/pl_slosh1.wav";
		rgsz[1] = "player/pl_slosh3.wav";
		rgsz[2] = "player/pl_slosh2.wav";
		rgsz[3] = "player/pl_slosh4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_WOOD:
		fvol = 0.9;
		fvolbar = 0.2;
		rgsz[0] = "debris/wood1.wav";
		rgsz[1] = "debris/wood2.wav";
		rgsz[2] = "debris/wood3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_GLASS:
	case CHAR_TEX_COMPUTER:
		fvol = 0.8;
		fvolbar = 0.2;
		rgsz[0] = "debris/glass1.wav";
		rgsz[1] = "debris/glass2.wav";
		rgsz[2] = "debris/glass3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_FLESH:
		fvol = 1.0;
		fvolbar = 0.2;
		rgsz[0] = "weapons/bullet_hit1.wav";
		rgsz[1] = "weapons/bullet_hit2.wav";
		fattn = 1.0;
		cnt = 2;
		break;
	}

	// play material hit sound
	client::event::PlaySound(0, ptr->endpos, CHAN_STATIC, rgsz[client::RandomLong(0, cnt - 1)], fvol, fattn, 0, 96 + client::RandomLong(0, 0xf));
	return fvolbar;
}

static void EV_DecalTrace(pmtrace_t *tr, char *decal)
{
	if (r_decals->value <= 0.0f)
	{
		return;
	}

	physent_t *pe = client::event::GetPhysent(tr->ent);

	if (decal && decal[0] && pe && (pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP))
	{
		client::efx::DecalShoot(
			client::efx::Draw_DecalIndex(client::efx::Draw_DecalIndexFromName(decal)), 
			client::event::IndexFromTrace(tr),
			0,
			tr->endpos,
			0
		);
	}
}

static inline char* EV_DecalName(char* format, int count)
{
	static char sz[32];
	sprintf(sz, format, client::RandomLong(1, count));
	return sz;
}

static inline char* EV_DamageDecal(physent_t* pe)
{
	if (pe->classnumber == 1)
	{
		return EV_DecalName("{break%i", 3);
	}
	if (pe->rendermode != kRenderNormal)
	{
		return "{bproof1";
	}
	return EV_DecalName("{shot%i", 5);
}

static void EV_BulletImpact(const Vector& origin, const Vector& dir)
{
	const auto underwater =
		client::PM_PointContents(const_cast<Vector&>(origin), nullptr) == CONTENTS_WATER;

	/* Splash of bright streaks. */
	auto splashOrigin =
		origin + Vector(
			client::RandomFloat(-2.0F, 2.0F),
			client::RandomFloat(-2.0F, 2.0F),
			client::RandomFloat(-2.0F, 2.0F));

	auto splashDir = Vector(dir.x, dir.y, 1.0F);

	auto streakCount = underwater ? 4 : 2;

	client::efx::StreakSplash(splashOrigin, splashDir, 5, streakCount, 1, -100, 100);

	/* Spray of debris. */
	for (auto i = 0; i < 4; i++)
	{
		if (gTempEntCount >= 450)
		{
			break;
		}

		auto debrisDir = dir * 40.0F
			+ Vector(
				client::RandomFloat(-40.0F, 40.0F),
				client::RandomFloat(-40.0F, 40.0F),
				client::RandomFloat(-40.0F, 40.0F));

		auto debris = client::efx::TempSprite(
			const_cast<Vector&>(origin),
			debrisDir,
			0.5F,
			g_sModelIndexDebris,
			kRenderTransAlpha,
			kRenderFxNone,
			1.0F,
			1.0F,
			FTENT_GRAVITY | FTENT_COLLIDEWORLD | FTENT_FADEOUT | FTENT_COLLIDEKILL);

		if (debris != nullptr)
		{
			debris->entity.baseline.renderamt = 255;
			debris->entity.curstate.frame = client::RandomLong(0, debrisFrameCount);

			gTempEntCount++;
		}
	}

	/* Don't spawn dust puffs underwater. */
	if (underwater || gTempEntCount >= 450)
	{
		return;
	}

	/* Puff of dust that drifts downwards. */
	auto puff = client::efx::TempSprite(
		const_cast<Vector&>(origin),
		dir * 24.0F,
		1.0F,
		g_sModelIndexWallPuff,
		kRenderTransAdd,
		kRenderFxNone,
		0.1F,
		3.0F,
		FTENT_SPRANIMATE);

	if (puff != nullptr)
	{
		puff->entity.curstate.framerate = 30.0F;
		puff->entity.baseline.gravity = 80.0F;

		gTempEntCount++;
	}
}

static void EV_GunshotDecalTrace(pmtrace_t* pTrace, char* decalName)
{
	EV_BulletImpact(pTrace->endpos, pTrace->plane.normal);

	int iRand = client::RandomLong(0, 0x7FFF);
	if (iRand < (0x7fff / 2)) // not every bullet makes a sound.
	{
		switch (iRand % 5)
		{
		case 0: client::event::PlaySound(-1, pTrace->endpos, 0, "weapons/ric1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
		case 1: client::event::PlaySound(-1, pTrace->endpos, 0, "weapons/ric2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
		case 2: client::event::PlaySound(-1, pTrace->endpos, 0, "weapons/ric3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
		case 3: client::event::PlaySound(-1, pTrace->endpos, 0, "weapons/ric4.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
		case 4: client::event::PlaySound(-1, pTrace->endpos, 0, "weapons/ric5.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
		}
	}

	EV_DecalTrace(pTrace, decalName);
}

static void EV_BloodTrace(Vector pos, Vector dir, int damage)
{
	float noise = damage / 100.0f;
	int count = std::max(floorf(noise * 10), 1.0F);
	Vector traceDir;
	pmtrace_t tr;

	for (int i = 0; i < count; i++)
	{
		traceDir = dir * -1;
		traceDir.x += client::RandomFloat(-noise, noise);
		traceDir.y += client::RandomFloat(-noise, noise);
		traceDir.z += client::RandomFloat(-noise, noise);

		EV_TraceLine(pos, pos + traceDir * -172, PM_STUDIO_IGNORE, -1, tr);

		if (tr.fraction != 1.0f)
		{
			EV_DecalTrace(&tr, EV_DecalName("{blood%i", 6));
		}
	}
}

static void EV_DecalGunshot(pmtrace_t* pTrace, Vector vecDir)
{
	physent_t* pe = client::event::GetPhysent(pTrace->ent);

	if (!pe)
	{
		return;
	}

	if (pe->solid == SOLID_BSP
	 && client::PM_PointContents(pTrace->endpos, nullptr) != CONTENTS_SKY)
	{
		EV_GunshotDecalTrace(pTrace, EV_DamageDecal(pe));
	}
}

static void EV_CheckTracer(
	int entindex,
	Vector& start,
	Vector& end,
	const Vector& forward,
	const Vector& right,
	float distance,
	int frequency)
{
	Vector muzzle;
	int rightOffset = 2;

	if (g_PlayerExtraInfo[entindex].lefthanded)
	{
		rightOffset = -2;
	}

#if 0
	if (EV_IsLocal(entindex) && CL_IsThirdPerson() == 0)
	{
		muzzle = client::GetViewModel()->attachment[0];
	}
	else
#endif
	{
		muzzle = start + Vector(0, 0, -4) + right * rightOffset + forward * 16;
	}

	EV_BubbleTrail(muzzle, end, distance / 64.0F);

	if (frequency != 0 && ((tracerCount[entindex - 1])++ % frequency) == 0)
	{
		EV_CreateTracer(muzzle, end);
	}
}


/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.
================
*/
static void EV_FireBullets(
	event_args_t* args,
	const int random_seed,
	const Vector2D& spread,
	const unsigned int count = 1,
	const float distance = 8192,
	const bool playTextureSounds = true,
	const int tracerFrequency = 0)
{
	Vector gun;
	Vector aim = args->angles;

	EV_GetGunPosition(args, gun, args->origin);
	
	EV_TracePush(args->entindex);

	for (auto i = 0; i < count; i++)
	{
		const Vector2D spreadScale
		{
			util::SharedRandomFloat(random_seed + i * 4, -0.5, 0.5)
				+ util::SharedRandomFloat(random_seed + 1 + i * 4, -0.5, 0.5),
			util::SharedRandomFloat(random_seed + 2 + i * 4, -0.5, 0.5)
				+ util::SharedRandomFloat(random_seed + 3 + i * 4, -0.5, 0.5)
		};

		const Vector angles
		{
			aim.x + spread.y * spreadScale.x,
			aim.y + spread.x * spreadScale.y,
			aim.z,
		};

		Vector forward, right, up;
		AngleVectors(angles, &forward, &right, &up);

		pmtrace_t tr;
		EV_TraceLine(gun, gun + forward * distance, PM_NORMAL, -1, tr);

		EV_CheckTracer(
			args->entindex,
			gun,
			tr.endpos,
			forward,
			right,
			distance * tr.fraction,
			tracerFrequency);
		
		if (tr.fraction != 1.0F)
		{
			if (playTextureSounds)
			{
				EV_PlayTextureSound(args->entindex, &tr, gun, tr.endpos);
			}
			EV_DecalGunshot(&tr, forward);
		}
	}

	EV_TracePop();
}

void CMP5::EV_PrimaryAttack(event_args_t* args)
{
	Vector up, right, forward;
	AngleVectors(args->angles, &forward, &right, &up);

	if (EV_IsLocal(args->entindex))
	{
		EV_MuzzleFlash();

		client::event::WeaponAnimation(
			kAnimFire1 + client::RandomLong(0, 2), 0);

		V_PunchAxis(0, client::RandomFloat(-2, 2));
	}

	Vector shellOrigin, shellVelocity;
	EV_GetDefaultShellInfo(args, args->origin, args->velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4);
	EV_EjectBrass(shellOrigin, shellVelocity, args->angles[YAW], g_sModelIndexShell, TE_BOUNCE_SHELL);

	const char* sample;
	switch (client::RandomLong(0, 1))
	{
	case 0: sample = "weapons/hks1.wav"; break;
	case 1: sample = "weapons/hks2.wav"; break;
	}

	client::event::PlaySound(
		args->entindex,
		args->origin,
		CHAN_WEAPON,
		sample,
		VOL_NORM,
		ATTN_NORM,
		0,
		client::RandomLong(94, 109));

	EV_FireBullets(args, args->iparam1, Vector2D(6, 6), args->iparam2, 8192, false, 2);
}

void CMP5::EV_SecondaryAttack(event_args_t* args)
{
	if (EV_IsLocal(args->entindex))
	{
		client::event::WeaponAnimation(kAnimLaunch, 0);

		V_PunchAxis(0, -10);
	}

	const char* sample;
	switch (client::RandomLong(0, 1))
	{
	case 0: sample = "weapons/glauncher.wav"; break;
	case 1: sample = "weapons/glauncher2.wav"; break;
	}

	client::event::PlaySound(
		args->entindex,
		args->origin,
		CHAN_WEAPON,
		sample,
		VOL_NORM,
		ATTN_NORM,
		0,
		client::RandomLong(94, 109));
}

static int g_iSwing;

void CCrowbar::EV_PrimaryAttack(event_args_t* args)
{
	Vector gun;
	EV_GetGunPosition(args, gun, args->origin);

	const auto reliable = args->bparam1;
	auto hit = args->iparam1;

	pmtrace_t tr;

	if (!reliable)
	{
		EV_TracePush(args->entindex);

		Vector forward, right, up;
		AngleVectors(args->angles, &forward, &right, &up);

		EV_TraceLine(gun, gun + forward * 64, PM_NORMAL, -1, tr);

		if (EV_IsLocal(args->entindex))
		{
			g_iSwing++;
		}

		if (tr.fraction != 1.0F)
		{
			auto ent = client::event::GetPhysent(tr.ent);

			if (ent->solid != SOLID_BSP && ent->movetype != MOVETYPE_PUSHSTEP)
			{
				hit = kCrowbarHitPlayer;
			}
			else
			{
				hit = kCrowbarHitWorld;
			}
		}
		else
		{
			hit = kCrowbarMiss;
		}
	}

	if (hit == kCrowbarMiss)
	{
		if (EV_IsLocal(args->entindex))
		{
			switch (g_iSwing % 3)
			{
			case 0: client::event::WeaponAnimation(kAnimAttack1Miss, 0); break;
			case 1: client::event::WeaponAnimation(kAnimAttack2Miss, 0); break;
			case 2: client::event::WeaponAnimation(kAnimAttack3Miss, 0); break;
			}
		}

		//Play Swing sound
		client::event::PlaySound(
			args->entindex,
			args->origin,
			CHAN_WEAPON,
			"weapons/cbar_miss1.wav",
			VOL_NORM,
			ATTN_NORM,
			0,
			PITCH_NORM);
	}
	else
	{
		if (EV_IsLocal(args->entindex))
		{
			switch (g_iSwing % 3)
			{
			case 0: client::event::WeaponAnimation(kAnimAttack1Hit, 0); break;
			case 1: client::event::WeaponAnimation(kAnimAttack2Hit, 0); break;
			case 2: client::event::WeaponAnimation(kAnimAttack3Hit, 0); break;
			}
		}

		const char* sample;

		if (hit == kCrowbarHitWorld)
		{
			switch (client::RandomLong(0, 1))
			{
				case 0: sample = "weapons/cbar_hit1.wav"; break;
				case 1: sample = "weapons/cbar_hit2.wav"; break;
			}
		}
		else
		{
			switch (client::RandomLong(0, 2))
			{
				case 0: sample = "weapons/cbar_hitbod1.wav"; break;
				case 1: sample = "weapons/cbar_hitbod2.wav"; break;
				case 2: sample = "weapons/cbar_hitbod3.wav"; break;
			}
		}

		client::event::PlaySound(
			args->entindex,
			args->origin,
			CHAN_ITEM,
			sample,
			VOL_NORM,
			ATTN_NORM,
			0,
			PITCH_NORM);
	}
	
	if (!reliable)
	{
		EV_TracePop();
	}
}

TEMPENTITY* pLaserDot;

void EV_LaserDotOn(event_args_t* args)
{
	#if 0
	const auto bMakeNoise = args->iparam1 != 0;
	
	if (!EV_IsLocal(args->entindex))
		return;

	if (g_CurrentWeaponId != WEAPON_RPG)
	{
		if (pLaserDot != nullptr)
		{
			pLaserDot->die = client::GetClientTime();
			pLaserDot = nullptr;
		}
		return;
	}

	if (pLaserDot == nullptr)
	{
		if (bMakeNoise)
		{
			client::event::PlaySound(
				args->entindex,
				args->origin,
				CHAN_WEAPON,
				"weapons/desert_eagle_sight.wav",
				0.9,
				ATTN_IDLE,
				0,
				PITCH_NORM);
		}
		
		pLaserDot = client::efx::TempSprite(
			args->origin,
			g_vecZero,
			1.0,
			g_sModelIndexLaserDot,
			kRenderGlow,
			kRenderFxNoDissipation,
			1.0,
			99999,
			FTENT_SPRCYCLE | FTENT_PERSIST);
	}
	
	const auto flSuspendTime = args->fparam1;

	if (flSuspendTime > 0 && pLaserDot != nullptr)
	{
		pLaserDot->entity.baseline.fuser4 = client::GetClientTime() + flSuspendTime;
		pLaserDot->flags |= FTENT_NOMODEL;
	}
	#endif
}

void EV_LaserDotOff(event_args_t* args)
{
	#if 0
	const auto bMakeNoise = args->iparam1 != 0;

	if (!EV_IsLocal(args->entindex))
		return;
	
	if (pLaserDot != nullptr)
	{
		if (bMakeNoise)
		{
			client::event::PlaySound(
				args->entindex,
				args->origin,
				CHAN_WEAPON,
				"weapons/desert_eagle_sight2.wav",
				0.9,
				ATTN_IDLE,
				0,
				PITCH_NORM);
		}
		
		pLaserDot->die = client::GetClientTime();
		pLaserDot = nullptr;
	}
	#endif
}

void EV_TrainPitchAdjust(event_args_t* args)
{
	int idx;
	Vector origin;

	unsigned short us_params;
	int noise;
	float m_flVolume;
	int pitch;
	bool stop;

	char sz[256];

	idx = args->entindex;

	origin = args->origin;

	us_params = (unsigned short)args->iparam1;
	stop = 0 != args->bparam1;

	m_flVolume = (float)(us_params & 0x003f) / 40.0;
	noise = (int)(((us_params) >> 12) & 0x0007);
	pitch = (int)(10.0 * (float)((us_params >> 6) & 0x003f));

	switch (noise)
	{
	case 1:
		strcpy(sz, "plats/ttrain1.wav");
		break;
	case 2:
		strcpy(sz, "plats/ttrain2.wav");
		break;
	case 3:
		strcpy(sz, "plats/ttrain3.wav");
		break;
	case 4:
		strcpy(sz, "plats/ttrain4.wav");
		break;
	case 5:
		strcpy(sz, "plats/ttrain6.wav");
		break;
	case 6:
		strcpy(sz, "plats/ttrain7.wav");
		break;
	default:
		// no sound
		strcpy(sz, "");
		return;
	}

	if (stop)
	{
		client::event::StopSound(idx, CHAN_STATIC, sz);
	}
	else
	{
		client::event::PlaySound(idx, origin, CHAN_STATIC, sz, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch);
	}
}

static void EV_GibTouch(struct tempent_s* ent, struct pmtrace_s* ptr)
{
	EV_DecalTrace(ptr, EV_DecalName("{blood%i", 6));

	switch (client::RandomLong(0, 25))
	{
	case 0: client::event::PlaySound(-1, ptr->endpos, CHAN_STATIC, "debris/flesh1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
	case 1: client::event::PlaySound(-1, ptr->endpos, CHAN_STATIC, "debris/flesh2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
	case 2: client::event::PlaySound(-1, ptr->endpos, CHAN_STATIC, "debris/flesh3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
	case 3: client::event::PlaySound(-1, ptr->endpos, CHAN_STATIC, "debris/flesh5.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
	case 4: client::event::PlaySound(-1, ptr->endpos, CHAN_STATIC, "debris/flesh6.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
	case 5: client::event::PlaySound(-1, ptr->endpos, CHAN_STATIC, "debris/flesh7.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
	default: break;
	}
}

static void EV_SpawnGibs(event_args_t* args, int count)
{
	auto rotate = Vector(
		client::RandomLong(-100, 100),
		client::RandomLong(-100, 100),
		client::RandomLong(-100, 100)
	);
	TEMPENTITY *gib;
	Vector velocity, dir;
	auto body = 0;
	Vector forward;
	AngleVectors(args->angles, &forward, nullptr, nullptr);
	forward = forward * -1.0f;
	while (--count >= 0)
	{
		dir = forward;
		dir[0] += client::RandomFloat(-0.30, 0.30);
		dir[1] += client::RandomFloat(-0.30, 0.30);
		dir[2] += client::RandomFloat(-0.30, 0.30);

		velocity = dir * client::RandomFloat(500, 1200);
		velocity[2] += 600;

		gib = client::efx::TempModel(args->origin, velocity, rotate, 15.0f, g_sModelIndexGibs, TE_BOUNCE_NULL);

		if (gib != nullptr)
		{
			gib->flags |= (FTENT_COLLIDEWORLD | FTENT_ROTATE | FTENT_FADEOUT | FTENT_CLIENTCUSTOM | FTENT_SMOKETRAIL);
			gib->entity.curstate.body = body;
			gib->hitcallback = EV_GibTouch;
		}

		body = client::RandomLong(1, 5);
	}
}

static void EV_SpawnCorpse(event_args_t* args)
{
	auto player = client::GetEntityByIndex(args->entindex);
	if (player == nullptr)
	{
		return;
	}

	auto model = client::hudGetModelByIndex(g_sModelIndexPlayer);
	if (model == nullptr)
	{
		return;
	}
	
	auto gib = client::efx::TempEntAllocHigh(args->origin, model);
	if (gib == nullptr)
	{
		return;
	}

	gib->flags |= (FTENT_COLLIDEWORLD | FTENT_PERSIST | FTENT_GRAVITY | FTENT_FADEOUT);
	gib->entity.origin = player->origin + Vector(0.0F, 0.0F, 2.0F);
	gib->entity.angles = player->angles;
	gib->entity.curstate.sequence = args->iparam1;
	gib->entity.curstate.frame = 0.0F;
	gib->entity.curstate.framerate = 1.0F;
	gib->entity.curstate.renderfx = kRenderFxDeadPlayer;
	gib->entity.curstate.iuser4 = args->entindex;
	gib->entity.prevstate = gib->entity.curstate;
	gib->die = client::GetClientTime() + 15.0F;
}

void EV_Gibbed(event_args_t* args)
{
	if (violence_hgibs->value != 0.0f
	 && (args->iparam2 == GIB_ALWAYS
	 || (args->iparam2 == GIB_NORMAL && args->fparam2 < -40.0f)))
	{
		client::event::PlaySound(
			-1,
			args->origin,
			CHAN_STATIC,
			"common/bodysplat.wav",
			VOL_NORM,
			ATTN_NORM,
			0,
			PITCH_NORM);

		EV_SpawnGibs(args, 5);
		return;
	}
	EV_SpawnCorpse(args);
}

void EV_Teleport(event_args_t* args)
{
	client::event::PlaySound(
		-1,
		args->origin,
		CHAN_STATIC,
		"misc/r_tele1.wav",
		VOL_NORM,
		ATTN_NORM,
		0,
		PITCH_NORM);

	client::efx::TeleportSplash(args->origin);
}

static void EV_SmokeCallback(TEMPENTITY* ent, float frametime, float currenttime)
{
	if (ent->entity.curstate.fuser1 > currenttime)
	{
		return;
	}

	ent->callback = nullptr;
	ent->flags &= ~FTENT_CLIENTCUSTOM;
	ent->die = currenttime;

	if (ent->entity.curstate.iuser2)
	{
		EV_Bubbles(ent->entity.origin, 64);
		return;
	}

	auto smonk = client::efx::TempSprite(
		ent->entity.origin,
		ent->entity.angles,
		1.0F,
		g_sModelIndexSmoke,
		kRenderTransAlpha,
		kRenderFxNone,
		1.0F,
		1.0F,
		FTENT_SPRANIMATE);

	if (smonk)
	{
		client::efx::Sprite_Smoke(
			smonk,
			std::max((ent->entity.curstate.iuser1 - 50) * 0.08F, 1.0F));
	}
}

void EV_Explosion(event_args_t* args)
{
	const auto underwater =
		client::PM_PointContents(args->origin, nullptr) == CONTENTS_WATER;

	EV_TracePush(-1);

	Vector dir = args->angles;
	if (dir == g_vecZero)
	{
		dir = Vector(0, 0, -1);
	}
	else
	{
		dir = dir.Normalize();
	}
	Vector origin = args->origin;

	pmtrace_t tr;
	EV_TraceLine(origin, origin + dir * 64, PM_STUDIO_BOX, -1, tr);
	
	const auto scale = std::max((args->iparam1 - 50) * 0.06F, 1.0F);

	if (tr.fraction != 1.0F)
	{
		origin = tr.endpos + tr.plane.normal * 16 * scale;
	}

	auto sprite = g_sModelIndexFireball;
	if (underwater)
	{
		sprite = g_sModelIndexWExplosion;
	}

	client::efx::Explosion(
		origin,
		sprite,
		scale,
		15,
		TE_EXPLFLAG_NONE);

	EV_DecalTrace(&tr, EV_DecalName("{scorch%i", 3));

	EV_TracePop();

	const char* sample;

	switch (client::RandomLong(0, 2))
	{
	case 0: sample = "weapons/debris1.wav"; break;
	case 1: sample = "weapons/debris2.wav"; break;
	case 2: sample = "weapons/debris3.wav"; break;
	}

	client::event::PlaySound(
		-1,
		tr.endpos,
		CHAN_STATIC,
		sample,
		0.55F,
		ATTN_NORM,
		0,
		PITCH_NORM);

	if (args->bparam1)
	{
		auto smonk = client::efx::TempEntAllocNoModel(origin);
		if (smonk)
		{
			smonk->flags |= FTENT_CLIENTCUSTOM;
			smonk->callback = EV_SmokeCallback;
			smonk->die = client::GetClientTime() + 3.0F;
			smonk->entity.curstate.fuser1 = client::GetClientTime() + 0.3F;
			smonk->entity.curstate.iuser1 = args->iparam1;
			smonk->entity.curstate.iuser2 = underwater ? 1 : 0;
		}
	}

	if (args->bparam2 && !underwater)
	{
		/* Toodles: Why don't these work right, Gabe? */
		for (auto i = client::RandomLong(0, 3); i > 0; i--)
		{
			client::efx::SparkShower(origin);
		}
	}
}

int MSG_Blood(const char* name, int size, void* buf)
{
	BEGIN_READ(buf, size);

	EV_TracePush(-1);

	Vector traceDir;
	traceDir.x = READ_FLOAT();
	traceDir.y = READ_FLOAT();
	traceDir.z = READ_FLOAT();

	const auto traceHits = READ_BYTE() + 1;
	const auto traceFlags = READ_BYTE();

	for (auto i = 0; i < traceHits; i++)
	{
		Vector traceEndPos;
		traceEndPos.x = READ_COORD();
		traceEndPos.y = READ_COORD();
		traceEndPos.z = READ_COORD();

		if (violence_hblood->value <= 0.0f)
		{
			continue;
		}

		if (gTempEntCount >= 400)
		{
			continue;
		}

		if ((traceFlags & (1 << i)) != 0)
		{
			client::efx::BloodStream(
				traceEndPos,
				-traceDir,
				70,
				100);
		}
		else
		{
			client::efx::BloodSprite(
				traceEndPos,
				BLOOD_COLOR_RED,
				g_sModelIndexBloodSpray,
				g_sModelIndexBloodDrop,
				4);

			gTempEntCount += 15;
		}

		EV_BloodTrace(traceEndPos, traceDir, 4);
	}

	EV_TracePop();

	return true;
}

int MSG_PredictedSound(const char* name, int size, void* buf)
{
	BEGIN_READ(buf, size);

	int entindex = READ_BYTE();
	int channel = READ_BYTE();
	int volume = READ_BYTE();
	int attenuation = READ_BYTE();
	int pitch = READ_BYTE();
	const char* sample = READ_STRING();

	cl_entity_t* entity;

	if (EV_IsLocal(entindex))
	{
		entity = client::GetLocalPlayer();
	}
	else
	{
		entity = client::GetEntityByIndex(entindex);
	}

	if (entity != nullptr)
	{
		client::event::PlaySound(
			entindex,
			entity->origin,
			channel,
			sample,
			volume / 255.0F,
			attenuation / 127.0F,
			0,
			pitch);
	}

	return true;
}

int MSG_Shooter(const char* name, int size, void* buf)
{
	BEGIN_READ(buf, size);

	Vector origin;
	origin.x = READ_COORD();
	origin.y = READ_COORD();
	origin.z = READ_COORD();

	Vector movedir;
	movedir.x = READ_FLOAT();
	movedir.y = READ_FLOAT();
	movedir.z = READ_FLOAT();

	const auto gibVelocity = READ_FLOAT();
	const auto gibVariance = READ_FLOAT();
	const auto gibLife = READ_FLOAT();
	const auto modelIndex = READ_SHORT();

	if (modelIndex == 0 && violence_hgibs->value == 0.0F)
	{
		return true;
	}

	Vector angles;
	VectorAngles(movedir, angles);

	Vector forward, right, up;
	AngleVectors(angles, &forward, &right, &up);

	auto velocity = movedir
		+ right * client::RandomFloat(-1, 1) * gibVariance
		+ forward * client::RandomFloat(-1, 1) * gibVariance
		+ up * client::RandomFloat(-1, 1) * gibVariance;

	velocity = velocity.Normalize() * gibVelocity;

	auto gib = client::efx::TempModel(
		origin,
		velocity,
		angles,
		gibLife * client::RandomFloat(0.95F, 1.05F),
		modelIndex != 0 ? modelIndex : g_sModelIndexGibs,
		TE_BOUNCE_NULL);

	if (gib == nullptr)
	{
		return true;
	}

	gib->flags |= FTENT_COLLIDEWORLD | FTENT_ROTATE | FTENT_FADEOUT;

	gib->entity.baseline.angles = Vector(
		client::RandomFloat(100, 200),
		client::RandomFloat(100, 300),
		0
	);

	/* Client physics are slightly different, so this is some stupid compensation. */
	gib->bounceFactor = 1.375F;

	if (modelIndex != 0)
	{
		/* Randomize the body. */
		if (gib->entity.model->type == mod_studio
		 && gib->entity.model->numsubmodels > 1)
		{
			gib->entity.curstate.body =
				client::RandomLong(0, gib->entity.model->numsubmodels - 1);
		}

		/* Toodles TODO: Material doesn't actually get used anywhere. */
		gib->entity.curstate.iuser1 = READ_BYTE();
		gib->entity.curstate.rendermode = READ_BYTE();
		gib->entity.curstate.renderamt = READ_BYTE();
		gib->entity.curstate.rendercolor.r = READ_BYTE();
		gib->entity.curstate.rendercolor.g = READ_BYTE();
		gib->entity.curstate.rendercolor.b = READ_BYTE();
		gib->entity.curstate.renderfx = READ_BYTE();
		gib->entity.curstate.scale = READ_BYTE() / 255.0F;
		gib->entity.curstate.skin = READ_BYTE();
	}
	else
	{
		gib->flags |= FTENT_SMOKETRAIL;
		gib->entity.curstate.body = client::RandomLong(1, 5);
		gib->hitcallback = EV_GibTouch;
	}

	return true;
}

/*
======================
EV_HookEvents

Associate script file name with callback functions.
======================
*/
void EV_HookEvents()
{
	client::HookEvent("events/mp5.sc", CMP5::EV_PrimaryAttack);
	client::HookEvent("events/mp52.sc", CMP5::EV_SecondaryAttack);
	client::HookEvent("events/crowbar.sc", CCrowbar::EV_PrimaryAttack);
	client::HookEvent("events/laser_on.sc", EV_LaserDotOn);
	client::HookEvent("events/laser_off.sc", EV_LaserDotOff);
	client::HookEvent("events/gibs.sc", EV_Gibbed);
	client::HookEvent("events/teleport.sc", EV_Teleport);
	client::HookEvent("events/explosion.sc", EV_Explosion);
	client::HookEvent("events/train.sc", EV_TrainPitchAdjust);

	client::HookUserMsg("Blood", MSG_Blood);
	client::HookUserMsg("PredSound", MSG_PredictedSound);
	client::HookUserMsg("Shooter", MSG_Shooter);
}

void EV_Init()
{
	g_sModelIndexPlayer = client::event::FindModelIndex("models/player.mdl");
	g_sModelIndexGibs = client::event::FindModelIndex("models/hgibs.mdl");
	g_sModelIndexShell = client::event::FindModelIndex("models/shell.mdl");

	g_sModelIndexLaser = client::event::FindModelIndex("sprites/laserbeam.spr");
	g_sModelIndexLaserDot = client::event::FindModelIndex("sprites/laserdot.spr");
	g_sModelIndexFireball = client::event::FindModelIndex("sprites/zerogxplode.spr");
	g_sModelIndexWExplosion = client::event::FindModelIndex("sprites/WXplo1.spr");
	g_sModelIndexSmoke = client::event::FindModelIndex("sprites/steam1.spr");
	g_sModelIndexBubbles = client::event::FindModelIndex("sprites/bubble.spr");
	g_sModelIndexBloodSpray = client::event::FindModelIndex("sprites/bloodspray.spr");
	g_sModelIndexBloodDrop = client::event::FindModelIndex("sprites/blood.spr");
	g_sModelIndexWallPuff = client::event::FindModelIndex("sprites/wall_puff1.spr");
	g_sModelIndexDebris = client::event::FindModelIndex("sprites/debris.spr");

	auto model = client::hudGetModelByIndex(g_sModelIndexDebris);
	if (model != nullptr)
	{
		debrisFrameCount = model->numframes - 1;
	}

	if (pLaserDot != nullptr)
	{
		pLaserDot->die = -1000;
		pLaserDot = nullptr;
	}
}
