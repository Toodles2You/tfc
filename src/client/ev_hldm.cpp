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

#include <string.h>

static int tracerCount[MAX_PLAYERS];

#include "pm_shared.h"

extern cvar_t* r_decals;
extern cvar_t* violence_hblood;
extern cvar_t* violence_hgibs;

static float EV_WaterLevel(const Vector& position, float minz, float maxz)
{
	Vector midUp = position;
	midUp.z = minz;

	if (gEngfuncs.PM_PointContents(midUp, nullptr) != CONTENTS_WATER)
	{
		return minz;
	}

	midUp.z = maxz;
	if (gEngfuncs.PM_PointContents(midUp, nullptr) == CONTENTS_WATER)
	{
		return maxz;
	}

	float diff = maxz - minz;
	while (diff > 1.0)
	{
		midUp.z = minz + diff / 2.0;
		if (gEngfuncs.PM_PointContents(midUp, nullptr) == CONTENTS_WATER)
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

	gEngfuncs.pEfxAPI->R_BubbleTrail(
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
	
	gEngfuncs.pEfxAPI->R_Bubbles(mins, maxs, height, g_sModelIndexBubbles, 100, 8);
}

// play a strike sound based on the texture that was hit by the attack traceline.  VecSrc/VecEnd are the
// original traceline endpoints used by the attacker.
// returns volume of strike instrument (crowbar) to play
static float EV_PlayTextureSound(int idx, pmtrace_t* ptr, float* vecSrc, float* vecEnd)
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

	entity = gEngfuncs.pEventAPI->EV_IndexFromTrace(ptr);

	chTextureType = 0;

	if (entity == 0)
	{
		// get texture from entity or world (world is ent(0))
		pTextureName = (char*)gEngfuncs.pEventAPI->EV_TraceTexture(ptr->ent, vecSrc, vecEnd);

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
	gEngfuncs.pEventAPI->EV_PlaySound(0, ptr->endpos, CHAN_STATIC, rgsz[gEngfuncs.pfnRandomLong(0, cnt - 1)], fvol, fattn, 0, 96 + gEngfuncs.pfnRandomLong(0, 0xf));
	return fvolbar;
}

static void EV_DecalTrace(pmtrace_t *tr, char *decal)
{
	if (r_decals->value <= 0.0f)
	{
		return;
	}

	physent_t *pe = gEngfuncs.pEventAPI->EV_GetPhysent(tr->ent);

	if (decal && decal[0] && pe && (pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP))
	{
		gEngfuncs.pEfxAPI->R_DecalShoot(
			gEngfuncs.pEfxAPI->Draw_DecalIndex(gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(decal)), 
			gEngfuncs.pEventAPI->EV_IndexFromTrace(tr),
			0,
			tr->endpos,
			0
		);
	}
}

static inline char* EV_DecalName(char* format, int count)
{
	static char sz[32];
	sprintf(sz, format, gEngfuncs.pfnRandomLong(1, count));
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

static void EV_GunshotDecalTrace(pmtrace_t* pTrace, char* decalName)
{
	gEngfuncs.pEfxAPI->R_BulletImpactParticles(pTrace->endpos);

	int iRand = gEngfuncs.pfnRandomLong(0, 0x7FFF);
	if (iRand < (0x7fff / 2)) // not every bullet makes a sound.
	{
		switch (iRand % 5)
		{
		case 0: gEngfuncs.pEventAPI->EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
		case 1: gEngfuncs.pEventAPI->EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
		case 2: gEngfuncs.pEventAPI->EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
		case 3: gEngfuncs.pEventAPI->EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric4.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
		case 4: gEngfuncs.pEventAPI->EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric5.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
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
		traceDir.x += gEngfuncs.pfnRandomFloat(-noise, noise);
		traceDir.y += gEngfuncs.pfnRandomFloat(-noise, noise);
		traceDir.z += gEngfuncs.pfnRandomFloat(-noise, noise);

		gEngfuncs.pEventAPI->EV_PlayerTrace(pos, pos + traceDir * -172, PM_WORLD_ONLY, -1, &tr);

		if (tr.fraction != 1.0f)
		{
			EV_DecalTrace(&tr, EV_DecalName("{blood%i", 6));
		}
	}
}

static void EV_DecalGunshot(pmtrace_t* pTrace, Vector vecDir)
{
	physent_t* pe = gEngfuncs.pEventAPI->EV_GetPhysent(pTrace->ent);

	if (!pe)
	{
		return;
	}

	if (pe->solid == SOLID_BSP)
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

#if 0
	if (EV_IsLocal(entindex) && CL_IsThirdPerson() == 0)
	{
		muzzle = gEngfuncs.GetViewModel()->attachment[0];
	}
	else
#endif
	{
		muzzle = start + Vector(0, 0, -4) + right * 2 + forward * 16;
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
	
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(true, true);
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(args->entindex - 1);
	gEngfuncs.pEventAPI->EV_SetTraceHull(kHullPoint);

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
		AngleVectors(angles, forward, right, up);

		pmtrace_t tr;
		gEngfuncs.pEventAPI->EV_PlayerTrace(gun, gun + forward * distance, PM_NORMAL, -1, &tr);

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

	gEngfuncs.pEventAPI->EV_PopPMStates();
}

void CTFWeapon::EV_PrimaryAttack(event_args_t* args)
{
	const auto& info = CBasePlayerWeapon::WeaponInfoArray[(int)args->fparam1];

	Vector up, right, forward;
	AngleVectors(args->angles, forward, right, up);

	if (EV_IsLocal(args->entindex))
	{
		EV_MuzzleFlash();

		gEngfuncs.pEventAPI->EV_WeaponAnimation(info.iAnims[kWeaponAnimAttack], 0);

		V_PunchAxis(0, info.flPunchAngle);
	}

	Vector shellOrigin, shellVelocity;
	EV_GetDefaultShellInfo(args, args->origin, args->velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4);
	EV_EjectBrass(shellOrigin, shellVelocity, args->angles[YAW], g_sModelIndexShell, TE_BOUNCE_SHOTSHELL);

	gEngfuncs.pEventAPI->EV_PlaySound(
		args->entindex,
		args->origin,
		CHAN_WEAPON,
		info.pszAttackSound,
		VOL_NORM,
		ATTN_NORM,
		0,
		gEngfuncs.pfnRandomLong(94, 109));

	EV_FireBullets(args, args->iparam1, info.vecProjectileSpread, info.iProjectileCount * args->iparam2, 2048, false, 0);
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
			pLaserDot->die = gEngfuncs.GetClientTime();
			pLaserDot = nullptr;
		}
		return;
	}

	if (pLaserDot == nullptr)
	{
		if (bMakeNoise)
		{
			gEngfuncs.pEventAPI->EV_PlaySound(
				args->entindex,
				args->origin,
				CHAN_WEAPON,
				"weapons/desert_eagle_sight.wav",
				0.9,
				ATTN_IDLE,
				0,
				PITCH_NORM);
		}
		
		pLaserDot = gEngfuncs.pEfxAPI->R_TempSprite(
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
		pLaserDot->entity.baseline.fuser4 = gEngfuncs.GetClientTime() + flSuspendTime;
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
			gEngfuncs.pEventAPI->EV_PlaySound(
				args->entindex,
				args->origin,
				CHAN_WEAPON,
				"weapons/desert_eagle_sight2.wav",
				0.9,
				ATTN_IDLE,
				0,
				PITCH_NORM);
		}
		
		pLaserDot->die = gEngfuncs.GetClientTime();
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

	VectorCopy(args->origin, origin);

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
		gEngfuncs.pEventAPI->EV_StopSound(idx, CHAN_STATIC, sz);
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_STATIC, sz, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch);
	}
}

static void EV_GibTouch(struct tempent_s* ent, struct pmtrace_s* ptr)
{
	EV_DecalTrace(ptr, EV_DecalName("{blood%i", 6));

	switch (gEngfuncs.pfnRandomLong(0, 25))
	{
	case 0: gEngfuncs.pEventAPI->EV_PlaySound(0, ptr->endpos, CHAN_STATIC, "debris/flesh1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
	case 1: gEngfuncs.pEventAPI->EV_PlaySound(0, ptr->endpos, CHAN_STATIC, "debris/flesh2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
	case 2: gEngfuncs.pEventAPI->EV_PlaySound(0, ptr->endpos, CHAN_STATIC, "debris/flesh3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
	case 3: gEngfuncs.pEventAPI->EV_PlaySound(0, ptr->endpos, CHAN_STATIC, "debris/flesh5.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
	case 4: gEngfuncs.pEventAPI->EV_PlaySound(0, ptr->endpos, CHAN_STATIC, "debris/flesh6.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
	case 5: gEngfuncs.pEventAPI->EV_PlaySound(0, ptr->endpos, CHAN_STATIC, "debris/flesh7.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
	default: break;
	}
}

static void EV_SpawnGibs(event_args_t* args, int count)
{
	auto rotate = Vector(
		gEngfuncs.pfnRandomLong(-100, 100),
		gEngfuncs.pfnRandomLong(-100, 100),
		gEngfuncs.pfnRandomLong(-100, 100)
	);
	TEMPENTITY *gib;
	Vector velocity, dir;
	auto body = 0;
	Vector forward;
	AngleVectors(args->angles, forward, nullptr, nullptr);
	forward = forward * -1.0f;
	while (--count >= 0)
	{
		dir = forward;
		dir[0] += gEngfuncs.pfnRandomFloat(-0.30, 0.30);
		dir[1] += gEngfuncs.pfnRandomFloat(-0.30, 0.30);
		dir[2] += gEngfuncs.pfnRandomFloat(-0.30, 0.30);

		velocity = dir * gEngfuncs.pfnRandomFloat(500, 1200);
		velocity[2] += 600;

		gib = gEngfuncs.pEfxAPI->R_TempModel(args->origin, velocity, rotate, 15.0f, g_sModelIndexGibs, TE_BOUNCE_NULL);

		if (gib != nullptr)
		{
			gib->flags |= (FTENT_COLLIDEWORLD | FTENT_ROTATE | FTENT_FADEOUT | FTENT_CLIENTCUSTOM | FTENT_SMOKETRAIL);
			gib->entity.curstate.body = body;
			gib->hitcallback = EV_GibTouch;
		}

		body = gEngfuncs.pfnRandomLong(1, 5);
	}
}

static void EV_SpawnCorpse(event_args_t* args)
{
	auto player = gEngfuncs.GetEntityByIndex(args->entindex);
	if (player == nullptr)
	{
		return;
	}

	auto model = gEngfuncs.hudGetModelByIndex(g_sModelIndexPlayer);
	if (model == nullptr)
	{
		return;
	}
	
	auto gib = gEngfuncs.pEfxAPI->CL_TempEntAllocHigh(args->origin, model);
	if (gib == nullptr)
	{
		return;
	}

	/*! FIXME: Colormap doesn't get copied. */
	gib->flags |= (FTENT_COLLIDEWORLD | FTENT_PERSIST | FTENT_GRAVITY | FTENT_FADEOUT);
	gib->entity.origin = player->origin;
	gib->entity.angles = player->angles;
	gib->entity.curstate.sequence = args->iparam1;
	gib->entity.curstate.frame = 0.0f;
	gib->entity.curstate.framerate = 1.0f;
	gib->entity.curstate.renderfx = kRenderFxDeadPlayer;
	gib->entity.curstate.iuser4 = args->entindex;
	gib->entity.baseline = gib->entity.curstate;
	gib->die = gEngfuncs.GetClientTime() + 15.0f;
}

void EV_Gibbed(event_args_t* args)
{
	if (violence_hgibs->value != 0.0f
	 && (args->iparam2 == GIB_ALWAYS
	 || (args->iparam2 == GIB_NORMAL && args->fparam2 < -40.0f)))
	{
		gEngfuncs.pEventAPI->EV_PlaySound(
			0,
			args->origin,
			CHAN_STATIC,
			"common/bodysplat.wav",
			1.0,
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
	gEngfuncs.pEventAPI->EV_PlaySound(
		0,
		args->origin,
		CHAN_STATIC,
		"misc/r_tele1.wav",
		1.0,
		ATTN_NORM,
		0,
		PITCH_NORM);

	gEngfuncs.pEfxAPI->R_TeleportSplash(args->origin);
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

	auto smonk = gEngfuncs.pEfxAPI->R_TempSprite(
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
		gEngfuncs.pEfxAPI->R_Sprite_Smoke(
			smonk,
			(ent->entity.curstate.iuser1 - 50) * 0.08);
	}
}

void EV_Explosion(event_args_t* args)
{
	const auto underwater =
		gEngfuncs.PM_PointContents(args->origin, nullptr) == CONTENTS_WATER;

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);
	gEngfuncs.pEventAPI->EV_SetTraceHull(kHullPoint);

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
	gEngfuncs.pEventAPI->EV_PlayerTrace(
		origin,
		origin + dir * 64,
		PM_WORLD_ONLY,
		-1,
		&tr);
	
	const auto scale = (args->iparam1 - 50) * 0.06F;

	if (tr.fraction != 1.0F)
	{
		origin = tr.endpos + tr.plane.normal * 16 * scale;
	}

	auto sprite = g_sModelIndexFireball;
	if (underwater)
	{
		sprite = g_sModelIndexWExplosion;
	}

	gEngfuncs.pEfxAPI->R_Explosion(
		origin,
		sprite,
		scale,
		15,
		TE_EXPLFLAG_NONE);

	EV_DecalTrace(&tr, EV_DecalName("{scorch%i", 3));

	gEngfuncs.pEventAPI->EV_PopPMStates();

	const char* sample;

	switch (gEngfuncs.pfnRandomLong(0, 2))
	{
	case 0: sample = "weapons/debris1.wav"; break;
	case 1: sample = "weapons/debris2.wav"; break;
	case 2: sample = "weapons/debris3.wav"; break;
	}

	gEngfuncs.pEventAPI->EV_PlaySound(
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
		auto smonk = gEngfuncs.pEfxAPI->CL_TempEntAllocNoModel(origin);
		if (smonk)
		{
			smonk->flags |= FTENT_CLIENTCUSTOM;
			smonk->callback = EV_SmokeCallback;
			smonk->die = gEngfuncs.GetClientTime() + 3.0F;
			smonk->entity.curstate.fuser1 = gEngfuncs.GetClientTime() + 0.3F;
			smonk->entity.curstate.iuser1 = args->iparam1;
			smonk->entity.curstate.iuser2 = underwater ? 1 : 0;
		}
	}

	if (args->bparam2 && !underwater)
	{
		/* Toodles: Why don't these work right, Gabe? */
		for (auto i = gEngfuncs.pfnRandomLong(0, 3); i > 0; i--)
		{
			gEngfuncs.pEfxAPI->R_SparkShower(origin);
		}
	}
}

int MSG_Blood(const char* name, int size, void* buf)
{
	BEGIN_READ(buf, size);

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);
	gEngfuncs.pEventAPI->EV_SetTraceHull(kHullPoint);

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

		if ((traceFlags & (1 << i)) != 0)
		{
			gEngfuncs.pEfxAPI->R_BloodStream(
				traceEndPos,
				-traceDir,
				70,
				100);
		}
		else
		{
			gEngfuncs.pEfxAPI->R_BloodSprite(
				traceEndPos,
				BLOOD_COLOR_RED,
				g_sModelIndexBloodSpray,
				g_sModelIndexBloodDrop,
				4);
		}

		EV_BloodTrace(traceEndPos, traceDir, 4);
	}

	gEngfuncs.pEventAPI->EV_PopPMStates();

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
	gEngfuncs.pfnHookEvent("events/laser_on.sc", EV_LaserDotOn);
	gEngfuncs.pfnHookEvent("events/laser_off.sc", EV_LaserDotOff);
	gEngfuncs.pfnHookEvent("events/gibs.sc", EV_Gibbed);
	gEngfuncs.pfnHookEvent("events/teleport.sc", EV_Teleport);
	gEngfuncs.pfnHookEvent("events/explosion.sc", EV_Explosion);
	gEngfuncs.pfnHookEvent("events/train.sc", EV_TrainPitchAdjust);

	gEngfuncs.pfnHookUserMsg("blood", MSG_Blood);
}

void EV_Init()
{
	g_sModelIndexPlayer = gEngfuncs.pEventAPI->EV_FindModelIndex("models/player.mdl");
	g_sModelIndexGibs = gEngfuncs.pEventAPI->EV_FindModelIndex("models/hgibs.mdl");
	g_sModelIndexShell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shotgunshell.mdl");

	g_sModelIndexLaser = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/laserbeam.spr");
	g_sModelIndexLaserDot = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/laserdot.spr");
	g_sModelIndexFireball = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/zerogxplode.spr");
	g_sModelIndexWExplosion = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/WXplo1.spr");
	g_sModelIndexSmoke = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/steam1.spr");
	g_sModelIndexBubbles = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/bubble.spr");
	g_sModelIndexBloodSpray = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/bloodspray.spr");
	g_sModelIndexBloodDrop = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/blood.spr");

	if (pLaserDot != nullptr)
	{
		pLaserDot->die = -1000;
		pLaserDot = nullptr;
	}
}
