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

===== util.cpp ========================================================

  Utility code.  Really not optional after all.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include <time.h>
#include "shake.h"
#include "decals.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "UserMessages.h"

inline void MessageBegin(int dest, int type, const Vector& origin, CBaseEntity* entity)
{
	g_engfuncs.pfnMessageBegin(
		dest,
		type,
		origin,
		entity ? entity->pev->pContainingEntity : nullptr);
}

inline void MessageBegin(int dest, int type, CBaseEntity* entity)
{
	g_engfuncs.pfnMessageBegin(
		dest,
		type,
		nullptr,
		entity ? entity->pev->pContainingEntity : nullptr);
}

CBaseEntity* util::FindEntityForward(CBaseEntity* pMe)
{
	TraceResult tr;

	util::MakeVectors(pMe->pev->v_angle);
	util::TraceLine(pMe->pev->origin + pMe->pev->view_ofs, pMe->pev->origin + pMe->pev->view_ofs + gpGlobals->v_forward * 8192, dont_ignore_monsters, pMe, &tr);
	if (tr.flFraction != 1.0 && !FNullEnt(tr.pHit))
	{
		CBaseEntity* pHit = CBaseEntity::Instance(tr.pHit);
		return pHit;
	}
	return NULL;
}

static unsigned int glSeed = 0;

unsigned int seed_table[256] =
	{
		28985, 27138, 26457, 9451, 17764, 10909, 28790, 8716, 6361, 4853, 17798, 21977, 19643, 20662, 10834, 20103,
		27067, 28634, 18623, 25849, 8576, 26234, 23887, 18228, 32587, 4836, 3306, 1811, 3035, 24559, 18399, 315,
		26766, 907, 24102, 12370, 9674, 2972, 10472, 16492, 22683, 11529, 27968, 30406, 13213, 2319, 23620, 16823,
		10013, 23772, 21567, 1251, 19579, 20313, 18241, 30130, 8402, 20807, 27354, 7169, 21211, 17293, 5410, 19223,
		10255, 22480, 27388, 9946, 15628, 24389, 17308, 2370, 9530, 31683, 25927, 23567, 11694, 26397, 32602, 15031,
		18255, 17582, 1422, 28835, 23607, 12597, 20602, 10138, 5212, 1252, 10074, 23166, 19823, 31667, 5902, 24630,
		18948, 14330, 14950, 8939, 23540, 21311, 22428, 22391, 3583, 29004, 30498, 18714, 4278, 2437, 22430, 3439,
		28313, 23161, 25396, 13471, 19324, 15287, 2563, 18901, 13103, 16867, 9714, 14322, 15197, 26889, 19372, 26241,
		31925, 14640, 11497, 8941, 10056, 6451, 28656, 10737, 13874, 17356, 8281, 25937, 1661, 4850, 7448, 12744,
		21826, 5477, 10167, 16705, 26897, 8839, 30947, 27978, 27283, 24685, 32298, 3525, 12398, 28726, 9475, 10208,
		617, 13467, 22287, 2376, 6097, 26312, 2974, 9114, 21787, 28010, 4725, 15387, 3274, 10762, 31695, 17320,
		18324, 12441, 16801, 27376, 22464, 7500, 5666, 18144, 15314, 31914, 31627, 6495, 5226, 31203, 2331, 4668,
		12650, 18275, 351, 7268, 31319, 30119, 7600, 2905, 13826, 11343, 13053, 15583, 30055, 31093, 5067, 761,
		9685, 11070, 21369, 27155, 3663, 26542, 20169, 12161, 15411, 30401, 7580, 31784, 8985, 29367, 20989, 14203,
		29694, 21167, 10337, 1706, 28578, 887, 3373, 19477, 14382, 675, 7033, 15111, 26138, 12252, 30996, 21409,
		25678, 18555, 13256, 23316, 22407, 16727, 991, 9236, 5373, 29402, 6117, 15241, 27715, 19291, 19888, 19847};

unsigned int U_Random()
{
	glSeed *= 69069;
	glSeed += seed_table[glSeed & 0xff];

	return (++glSeed & 0x0fffffff);
}

void U_Srand(unsigned int seed)
{
	glSeed = seed_table[seed & 0xff];
}

/*
=====================
util::SharedRandomLong
=====================
*/
int util::SharedRandomLong(unsigned int seed, int low, int high)
{
	unsigned int range;

	U_Srand((int)seed + low + high);

	range = high - low + 1;
	if (0 == (range - 1))
	{
		return low;
	}
	else
	{
		int offset;
		int rnum;

		rnum = U_Random();

		offset = rnum % range;

		return (low + offset);
	}
}

/*
=====================
util::SharedRandomFloat
=====================
*/
float util::SharedRandomFloat(unsigned int seed, float low, float high)
{
	//
	unsigned int range;

	U_Srand((int)seed + *(int*)&low + *(int*)&high);

	U_Random();
	U_Random();

	range = high - low;
	if (0 == range)
	{
		return low;
	}
	else
	{
		int tensixrand;
		float offset;

		tensixrand = U_Random() & 65535;

		offset = (float)tensixrand / 65536.0;

		return (low + offset * range);
	}
}

// Normal overrides
void util::SetGroupTrace(int groupmask, int op)
{
	g_groupmask = groupmask;
	g_groupop = op;

	ENGINE_SETGROUPMASK(g_groupmask, g_groupop);
}

void util::UnsetGroupTrace()
{
	g_groupmask = 0;
	g_groupop = 0;

	ENGINE_SETGROUPMASK(0, 0);
}

// Smart version, it'll clean itself up when it pops off stack
util::GroupTrace::GroupTrace(int groupmask, int op)
{
	m_oldgroupmask = g_groupmask;
	m_oldgroupop = g_groupop;

	g_groupmask = groupmask;
	g_groupop = op;

	ENGINE_SETGROUPMASK(g_groupmask, g_groupop);
}

util::GroupTrace::~GroupTrace()
{
	g_groupmask = m_oldgroupmask;
	g_groupop = m_oldgroupop;

	ENGINE_SETGROUPMASK(g_groupmask, g_groupop);
}

TYPEDESCRIPTION gEntvarsDescription[] =
	{
		DEFINE_ENTITY_FIELD(classname, FIELD_STRING),
		DEFINE_ENTITY_GLOBAL_FIELD(globalname, FIELD_STRING),

		DEFINE_ENTITY_FIELD(origin, FIELD_POSITION_VECTOR),
		DEFINE_ENTITY_FIELD(oldorigin, FIELD_POSITION_VECTOR),
		DEFINE_ENTITY_FIELD(velocity, FIELD_VECTOR),
		DEFINE_ENTITY_FIELD(basevelocity, FIELD_VECTOR),
		DEFINE_ENTITY_FIELD(movedir, FIELD_VECTOR),

		DEFINE_ENTITY_FIELD(angles, FIELD_VECTOR),
		DEFINE_ENTITY_FIELD(avelocity, FIELD_VECTOR),
		DEFINE_ENTITY_FIELD(punchangle, FIELD_VECTOR),
		DEFINE_ENTITY_FIELD(v_angle, FIELD_VECTOR),
		DEFINE_ENTITY_FIELD(fixangle, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(idealpitch, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(pitch_speed, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(ideal_yaw, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(yaw_speed, FIELD_FLOAT),

		DEFINE_ENTITY_FIELD(modelindex, FIELD_INTEGER),
		DEFINE_ENTITY_GLOBAL_FIELD(model, FIELD_MODELNAME),

		DEFINE_ENTITY_FIELD(viewmodel, FIELD_MODELNAME),
		DEFINE_ENTITY_FIELD(weaponmodel, FIELD_MODELNAME),

		DEFINE_ENTITY_FIELD(absmin, FIELD_POSITION_VECTOR),
		DEFINE_ENTITY_FIELD(absmax, FIELD_POSITION_VECTOR),
		DEFINE_ENTITY_GLOBAL_FIELD(mins, FIELD_VECTOR),
		DEFINE_ENTITY_GLOBAL_FIELD(maxs, FIELD_VECTOR),
		DEFINE_ENTITY_GLOBAL_FIELD(size, FIELD_VECTOR),

		DEFINE_ENTITY_FIELD(ltime, FIELD_TIME),
		DEFINE_ENTITY_FIELD(nextthink, FIELD_TIME),

		DEFINE_ENTITY_FIELD(solid, FIELD_INTEGER),
		DEFINE_ENTITY_FIELD(movetype, FIELD_INTEGER),

		DEFINE_ENTITY_FIELD(skin, FIELD_INTEGER),
		DEFINE_ENTITY_FIELD(body, FIELD_INTEGER),
		DEFINE_ENTITY_FIELD(effects, FIELD_INTEGER),

		DEFINE_ENTITY_FIELD(gravity, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(friction, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(light_level, FIELD_FLOAT),

		DEFINE_ENTITY_FIELD(frame, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(scale, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(sequence, FIELD_INTEGER),
		DEFINE_ENTITY_FIELD(animtime, FIELD_TIME),
		DEFINE_ENTITY_FIELD(framerate, FIELD_FLOAT),
		DEFINE_ENTITY_ARRAY(controller, FIELD_CHARACTER, NUM_ENT_CONTROLLERS),
		DEFINE_ENTITY_ARRAY(blending, FIELD_CHARACTER, NUM_ENT_BLENDERS),

		DEFINE_ENTITY_FIELD(rendermode, FIELD_INTEGER),
		DEFINE_ENTITY_FIELD(renderamt, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(rendercolor, FIELD_VECTOR),
		DEFINE_ENTITY_FIELD(renderfx, FIELD_INTEGER),

		DEFINE_ENTITY_FIELD(health, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(frags, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(weapons, FIELD_INTEGER),
		DEFINE_ENTITY_FIELD(takedamage, FIELD_FLOAT),

		DEFINE_ENTITY_FIELD(deadflag, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(view_ofs, FIELD_VECTOR),
		DEFINE_ENTITY_FIELD(button, FIELD_INTEGER),
		DEFINE_ENTITY_FIELD(impulse, FIELD_INTEGER),

		DEFINE_ENTITY_FIELD(chain, FIELD_EDICT),
		DEFINE_ENTITY_FIELD(dmg_inflictor, FIELD_EDICT),
		DEFINE_ENTITY_FIELD(enemy, FIELD_EDICT),
		DEFINE_ENTITY_FIELD(aiment, FIELD_EDICT),
		DEFINE_ENTITY_FIELD(owner, FIELD_EDICT),
		DEFINE_ENTITY_FIELD(groundentity, FIELD_EDICT),

		DEFINE_ENTITY_FIELD(spawnflags, FIELD_INTEGER),
		DEFINE_ENTITY_FIELD(flags, FIELD_FLOAT),

		DEFINE_ENTITY_FIELD(colormap, FIELD_INTEGER),
		DEFINE_ENTITY_FIELD(team, FIELD_INTEGER),

		DEFINE_ENTITY_FIELD(max_health, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(teleport_time, FIELD_TIME),
		DEFINE_ENTITY_FIELD(armortype, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(armorvalue, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(waterlevel, FIELD_INTEGER),
		DEFINE_ENTITY_FIELD(watertype, FIELD_INTEGER),

		// Having these fields be local to the individual levels makes it easier to test those levels individually.
		DEFINE_ENTITY_GLOBAL_FIELD(target, FIELD_STRING),
		DEFINE_ENTITY_GLOBAL_FIELD(targetname, FIELD_STRING),
		DEFINE_ENTITY_FIELD(netname, FIELD_STRING),
		DEFINE_ENTITY_FIELD(message, FIELD_STRING),

		DEFINE_ENTITY_FIELD(dmg_take, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(dmg_save, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(dmg, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(dmgtime, FIELD_TIME),

		DEFINE_ENTITY_FIELD(noise, FIELD_SOUNDNAME),
		DEFINE_ENTITY_FIELD(noise1, FIELD_SOUNDNAME),
		DEFINE_ENTITY_FIELD(noise2, FIELD_SOUNDNAME),
		DEFINE_ENTITY_FIELD(noise3, FIELD_SOUNDNAME),
		DEFINE_ENTITY_FIELD(speed, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(air_finished, FIELD_TIME),
		DEFINE_ENTITY_FIELD(pain_finished, FIELD_TIME),
		DEFINE_ENTITY_FIELD(radsuit_finished, FIELD_TIME),
};

#define ENTVARS_COUNT (sizeof(gEntvarsDescription) / sizeof(gEntvarsDescription[0]))

edict_t* util::GetEntityList()
{
	return g_engfuncs.pfnPEntityOfEntOffset(0);
}

CBaseEntity* util::GetLocalPlayer()
{
	return util::PlayerByIndex(1);
}

// ripped this out of the engine
float util::AngleMod(float a)
{
	if (a < 0)
	{
		a = a + 360 * ((int)(a / 360) + 1);
	}
	else if (a >= 360)
	{
		a = a - 360 * ((int)(a / 360));
	}
	// a = (360.0/65536) * ((int)(a*(65536/360.0)) & 65535);
	return a;
}

float util::AngleDiff(float destAngle, float srcAngle)
{
	float delta;

	delta = destAngle - srcAngle;
	if (destAngle > srcAngle)
	{
		if (delta >= 180)
			delta -= 360;
	}
	else
	{
		if (delta <= -180)
			delta += 360;
	}
	return delta;
}

Vector util::VecToAngles(const Vector& vec)
{
	float rgflVecOut[3];
	VEC_TO_ANGLES(vec, rgflVecOut);
	return Vector(rgflVecOut);
}


int util::EntitiesInBox(CBaseEntity** pList, int listMax, const Vector& mins, const Vector& maxs, int flagMask, bool checkSolid)
{
	edict_t* pEdict = util::GetEntityList();
	CBaseEntity* pEntity;
	int count;

	count = 0;

	if (!pEdict)
		return count;

	// Ignore world.
	++pEdict;

	for (int i = 1; i < gpGlobals->maxEntities; i++, pEdict++)
	{
		if (0 != pEdict->free) // Not in use
			continue;

		if (0 != flagMask && (pEdict->v.flags & flagMask) == 0) // Does it meet the criteria?
			continue;

		if (0 != checkSolid && (pEdict->v.solid == SOLID_NOT || pEdict->v.mins.x == pEdict->v.maxs.x))
			continue;

		if (mins.x > pEdict->v.absmax.x ||
			mins.y > pEdict->v.absmax.y ||
			mins.z > pEdict->v.absmax.z ||
			maxs.x < pEdict->v.absmin.x ||
			maxs.y < pEdict->v.absmin.y ||
			maxs.z < pEdict->v.absmin.z)
			continue;

		pEntity = CBaseEntity::Instance(pEdict);
		if (!pEntity)
			continue;

		pList[count] = pEntity;
		count++;

		if (count >= listMax)
			return count;
	}

	return count;
}


int util::MonstersInSphere(CBaseEntity** pList, int listMax, const Vector& center, float radius)
{
	edict_t* pEdict = util::GetEntityList();
	CBaseEntity* pEntity;
	int count;
	float distance, delta;

	count = 0;
	float radiusSquared = radius * radius;

	if (!pEdict)
		return count;

	// Ignore world.
	++pEdict;

	for (int i = 1; i < gpGlobals->maxEntities; i++, pEdict++)
	{
		if (0 != pEdict->free) // Not in use
			continue;

		if ((pEdict->v.flags & (FL_CLIENT | FL_MONSTER)) == 0) // Not a client/monster ?
			continue;

		// Use origin for X & Y since they are centered for all monsters
		// Now X
		delta = center.x - pEdict->v.origin.x; //(pEdict->v.absmin.x + pEdict->v.absmax.x)*0.5;
		delta *= delta;

		if (delta > radiusSquared)
			continue;
		distance = delta;

		// Now Y
		delta = center.y - pEdict->v.origin.y; //(pEdict->v.absmin.y + pEdict->v.absmax.y)*0.5;
		delta *= delta;

		distance += delta;
		if (distance > radiusSquared)
			continue;

		// Now Z
		delta = center.z - (pEdict->v.absmin.z + pEdict->v.absmax.z) * 0.5;
		delta *= delta;

		distance += delta;
		if (distance > radiusSquared)
			continue;

		pEntity = CBaseEntity::Instance(pEdict);
		if (!pEntity)
			continue;

		pList[count] = pEntity;
		count++;

		if (count >= listMax)
			return count;
	}


	return count;
}


CBaseEntity* util::FindEntityInSphere(CBaseEntity* pStartEntity, const Vector& vecCenter, float flRadius)
{
	edict_t* pentEntity;

	if (pStartEntity)
		pentEntity = pStartEntity->edict();
	else
		pentEntity = NULL;

	pentEntity = FIND_ENTITY_IN_SPHERE(pentEntity, vecCenter, flRadius);

	if (!FNullEnt(pentEntity))
		return CBaseEntity::Instance(pentEntity);
	return NULL;
}


CBaseEntity* util::FindEntityByString(CBaseEntity* pStartEntity, const char* szKeyword, const char* szValue)
{
	edict_t* pentEntity;

	if (pStartEntity)
		pentEntity = pStartEntity->edict();
	else
		pentEntity = NULL;

	pentEntity = FIND_ENTITY_BY_STRING(pentEntity, szKeyword, szValue);

	if (!FNullEnt(pentEntity))
		return CBaseEntity::Instance(pentEntity);
	return NULL;
}

CBaseEntity* util::FindEntityByClassname(CBaseEntity* pStartEntity, const char* szName)
{
	return util::FindEntityByString(pStartEntity, "classname", szName);
}

CBaseEntity* util::FindEntityByTargetname(CBaseEntity* pStartEntity, const char* szName)
{
	return util::FindEntityByString(pStartEntity, "targetname", szName);
}


CBaseEntity* util::FindEntityGeneric(const char* szWhatever, Vector& vecSrc, float flRadius)
{
	CBaseEntity* pEntity = NULL;

	pEntity = util::FindEntityByTargetname(NULL, szWhatever);
	if (pEntity)
		return pEntity;

	CBaseEntity* pSearch = NULL;
	float flMaxDist2 = flRadius * flRadius;
	while ((pSearch = util::FindEntityByClassname(pSearch, szWhatever)) != NULL)
	{
		float flDist2 = (pSearch->pev->origin - vecSrc).Length();
		flDist2 = flDist2 * flDist2;
		if (flMaxDist2 > flDist2)
		{
			pEntity = pSearch;
			flMaxDist2 = flDist2;
		}
	}
	return pEntity;
}


// returns a CBaseEntity pointer to a player by index.  Only returns if the player is spawned and connected
// otherwise returns NULL
// Index is 1 based
CBaseEntity* util::PlayerByIndex(int playerIndex)
{
	CBaseEntity* pPlayer = NULL;

	if (playerIndex > 0 && playerIndex <= gpGlobals->maxClients)
	{
		edict_t* pPlayerEdict = INDEXENT(playerIndex);
		if (pPlayerEdict && 0 == pPlayerEdict->free)
		{
			pPlayer = CBaseEntity::Instance(pPlayerEdict);
		}
	}

	return pPlayer;
}


void util::MakeVectors(const Vector& vecAngles)
{
	MAKE_VECTORS(vecAngles);
}


void util::MakeAimVectors(const Vector& vecAngles)
{
	float rgflVec[3];
	vecAngles.CopyToArray(rgflVec);
	rgflVec[0] = -rgflVec[0];
	MAKE_VECTORS(rgflVec);
}

void util::MakeInvVectors(const Vector& vec, globalvars_t* pgv)
{
	MAKE_VECTORS(vec);

	pgv->v_right = pgv->v_right * -1;

	std::swap(pgv->v_forward.y, pgv->v_right.x);
	std::swap(pgv->v_forward.z, pgv->v_up.x);
	std::swap(pgv->v_right.z, pgv->v_up.y);
}

static unsigned short FixedUnsigned16(float value, float scale)
{
	int output;

	output = value * scale;
	if (output < 0)
		output = 0;
	if (output > 0xFFFF)
		output = 0xFFFF;

	return (unsigned short)output;
}

static short FixedSigned16(float value, float scale)
{
	int output;

	output = value * scale;

	if (output > 32767)
		output = 32767;

	if (output < -32768)
		output = -32768;

	return (short)output;
}

// Shake the screen of all clients within radius
// radius == 0, shake all clients
void util::ScreenShake(const Vector& center, float amplitude, float frequency, float duration, float radius)
{
	int i;
	float localAmplitude;
	::ScreenShake shake;

	shake.duration = FixedUnsigned16(duration, 1 << 12);  // 4.12 fixed
	shake.frequency = FixedUnsigned16(frequency, 1 << 8); // 8.8 fixed

	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pPlayer = util::PlayerByIndex(i);

		if (!pPlayer || (pPlayer->pev->flags & FL_ONGROUND) == 0) // Don't shake if not onground
			continue;

		localAmplitude = 0;

		if (radius <= 0)
			localAmplitude = amplitude;
		else
		{
			Vector delta = center - pPlayer->pev->origin;
			float distance = delta.Length();

			// Had to get rid of this falloff - it didn't work well
			if (distance < radius)
				localAmplitude = amplitude; //radius - distance;
		}
		if (0 != localAmplitude)
		{
			shake.amplitude = FixedUnsigned16(localAmplitude, 1 << 12); // 4.12 fixed

			MessageBegin(MSG_ONE, gmsgShake, pPlayer);

			WriteShort(shake.amplitude); // shake amount
			WriteShort(shake.duration);  // shake lasts this long
			WriteShort(shake.frequency); // shake noise frequency

			MessageEnd();
		}
	}
}



void util::ScreenShakeAll(const Vector& center, float amplitude, float frequency, float duration)
{
	util::ScreenShake(center, amplitude, frequency, duration, 0);
}


static void ScreenFadeBuild(ScreenFade& fade, const Vector& color, float fadeTime, float fadeHold, int alpha, int flags)
{
	fade.duration = FixedUnsigned16(fadeTime, 1 << 12); // 4.12 fixed
	fade.holdTime = FixedUnsigned16(fadeHold, 1 << 12); // 4.12 fixed
	fade.r = (int)color.x;
	fade.g = (int)color.y;
	fade.b = (int)color.z;
	fade.a = alpha;
	fade.fadeFlags = flags;
}


static void ScreenFadeWrite(const ScreenFade& fade, CBaseEntity* pEntity)
{
	if (!pEntity || !pEntity->IsNetClient())
		return;

	MessageBegin(MSG_ONE, gmsgFade, pEntity);

	WriteShort(fade.duration);	 // fade lasts this long
	WriteShort(fade.holdTime);	 // fade lasts this long
	WriteShort(fade.fadeFlags); // fade type (in / out)
	WriteByte(fade.r);			 // fade red
	WriteByte(fade.g);			 // fade green
	WriteByte(fade.b);			 // fade blue
	WriteByte(fade.a);			 // fade blue

	MessageEnd();
}


void util::ScreenFadeAll(const Vector& color, float fadeTime, float fadeHold, int alpha, int flags)
{
	int i;
	::ScreenFade fade;


	ScreenFadeBuild(fade, color, fadeTime, fadeHold, alpha, flags);

	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pPlayer = util::PlayerByIndex(i);

		ScreenFadeWrite(fade, pPlayer);
	}
}


void util::ScreenFade(CBaseEntity* pEntity, const Vector& color, float fadeTime, float fadeHold, int alpha, int flags)
{
	::ScreenFade fade;

	ScreenFadeBuild(fade, color, fadeTime, fadeHold, alpha, flags);
	ScreenFadeWrite(fade, pEntity);
}


void util::HudMessage(CBaseEntity* pEntity, const hudtextparms_t& textparms, const char* pMessage)
{
	if (!pEntity || !pEntity->IsNetClient())
		return;

	MessageBegin(MSG_ONE, SVC_TEMPENTITY, pEntity);
	WriteByte(TE_TEXTMESSAGE);
	WriteByte(textparms.channel & 0xFF);

	WriteShort(FixedSigned16(textparms.x, 1 << 13));
	WriteShort(FixedSigned16(textparms.y, 1 << 13));
	WriteByte(textparms.effect);

	WriteByte(textparms.r1);
	WriteByte(textparms.g1);
	WriteByte(textparms.b1);
	WriteByte(textparms.a1);

	WriteByte(textparms.r2);
	WriteByte(textparms.g2);
	WriteByte(textparms.b2);
	WriteByte(textparms.a2);

	WriteShort(FixedUnsigned16(textparms.fadeinTime, 1 << 8));
	WriteShort(FixedUnsigned16(textparms.fadeoutTime, 1 << 8));
	WriteShort(FixedUnsigned16(textparms.holdTime, 1 << 8));

	if (textparms.effect == 2)
		WriteShort(FixedUnsigned16(textparms.fxTime, 1 << 8));

	if (strlen(pMessage) < 512)
	{
		WriteString(pMessage);
	}
	else
	{
		char tmp[512];
		strncpy(tmp, pMessage, 511);
		tmp[511] = 0;
		WriteString(tmp);
	}
	MessageEnd();
}

void util::HudMessageAll(const hudtextparms_t& textparms, const char* pMessage)
{
	int i;

	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pPlayer = util::PlayerByIndex(i);
		if (pPlayer)
			util::HudMessage(pPlayer, textparms, pMessage);
	}
}

void util::ClientPrintAll(int msg_dest, const char* msg_name, const char* param1, const char* param2, const char* param3, const char* param4)
{
	MessageBegin(MSG_ALL, gmsgTextMsg);
	WriteByte(msg_dest);
	WriteString(msg_name);

	if (param1)
		WriteString(param1);
	if (param2)
		WriteString(param2);
	if (param3)
		WriteString(param3);
	if (param4)
		WriteString(param4);

	MessageEnd();
}

void util::ClientPrint(CBaseEntity* entity, int msg_dest, const char* msg_name, const char* param1, const char* param2, const char* param3, const char* param4)
{
	if (!entity->IsNetClient())
		return;

	MessageBegin(MSG_ONE, gmsgTextMsg, entity);
	WriteByte(msg_dest);
	WriteString(msg_name);

	if (param1)
		WriteString(param1);
	if (param2)
		WriteString(param2);
	if (param3)
		WriteString(param3);
	if (param4)
		WriteString(param4);

	MessageEnd();
}

void util::SayText(const char* pText, CBaseEntity* pEntity)
{
	if (!pEntity->IsNetClient())
		return;

	MessageBegin(MSG_ONE, gmsgSayText, pEntity);
	WriteByte(pEntity->entindex());
	WriteString(pText);
	MessageEnd();
}

void util::SayTextAll(const char* pText, CBaseEntity* pEntity)
{
	MessageBegin(MSG_ALL, gmsgSayText);
	WriteByte(pEntity->entindex());
	WriteString(pText);
	MessageEnd();
}


char* util::dtos1(int d)
{
	static char buf[8];
	sprintf(buf, "%d", d);
	return buf;
}

char* util::dtos2(int d)
{
	static char buf[8];
	sprintf(buf, "%d", d);
	return buf;
}

char* util::dtos3(int d)
{
	static char buf[8];
	sprintf(buf, "%d", d);
	return buf;
}

char* util::dtos4(int d)
{
	static char buf[8];
	sprintf(buf, "%d", d);
	return buf;
}

void util::ShowMessage(const char* pString, CBaseEntity* pEntity)
{
	if (!pEntity || !pEntity->IsNetClient())
		return;

	MessageBegin(MSG_ONE, gmsgHudText, pEntity);
	WriteString(pString);
	MessageEnd();
}


void util::ShowMessageAll(const char* pString)
{
	int i;

	// loop through all players

	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pPlayer = util::PlayerByIndex(i);
		if (pPlayer)
			util::ShowMessage(pString, pPlayer);
	}
}

// Overloaded to add IGNORE_GLASS
void util::TraceLine(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, CBaseEntity* ignore, TraceResult* ptr)
{
	//TODO: define constants
	TRACE_LINE(vecStart, vecEnd, (igmon == ignore_monsters ? 1 : 0) | (ignore_glass == ignoreGlass ? 0x100 : 0), ignore ? ignore->edict() : nullptr, ptr);
}


void util::TraceLine(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, CBaseEntity* ignore, TraceResult* ptr)
{
	TRACE_LINE(vecStart, vecEnd, (igmon == ignore_monsters ? 1 : 0), ignore ? ignore->edict() : nullptr, ptr);
}


bool util::TraceLine(const Vector& start, const Vector& end, TraceResult* tr, CBaseEntity* ignore, int flags, int hull)
{
	int traceFlags = 0;

	if ((flags & kTraceIgnoreMonsters) != 0)
	{
		traceFlags |= 1;
	}

	if ((flags & kTraceIgnoreGlass) != 0)
	{
		traceFlags |= 256;
	}

	if ((flags & kTraceBox) != 0)
	{
		gpGlobals->trace_flags |= FTRACE_SIMPLEBOX;
	}

	edict_t* ignoreEnt = ignore ? ignore->pev->pContainingEntity : nullptr;

	if (hull == point_hull)
	{	
		g_engfuncs.pfnTraceLine(start, end, traceFlags, ignoreEnt, tr);
	}
	else
	{
		g_engfuncs.pfnTraceHull(start, end, traceFlags, hull, ignoreEnt, tr);
	}

	if ((flags & kTraceBoxModel) != 0 && tr->flFraction != 1.0F)
	{
		TraceResult tr2;
		if (hull == point_hull)
		{
			g_engfuncs.pfnTraceLine(tr->vecEndPos, end, traceFlags, ignoreEnt, &tr2);
		}
		else
		{
			g_engfuncs.pfnTraceHull(start, end, traceFlags, hull, ignoreEnt, &tr2);
		}
		
		if (tr2.flFraction != 1.0F && tr2.pHit == tr->pHit)
		{
			*tr = tr2;
		}
	}

	return tr->flFraction != 1.0F;
}


void util::TraceHull(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t* pentIgnore, TraceResult* ptr)
{
	TRACE_HULL(vecStart, vecEnd, (igmon == ignore_monsters ? 1 : 0), hullNumber, pentIgnore, ptr);
}

void util::TraceModel(const Vector& vecStart, const Vector& vecEnd, int hullNumber, edict_t* pentModel, TraceResult* ptr)
{
	g_engfuncs.pfnTraceModel(vecStart, vecEnd, hullNumber, pentModel, ptr);
}


TraceResult util::GetGlobalTrace()
{
	TraceResult tr;

	tr.fAllSolid = gpGlobals->trace_allsolid;
	tr.fStartSolid = gpGlobals->trace_startsolid;
	tr.fInOpen = gpGlobals->trace_inopen;
	tr.fInWater = gpGlobals->trace_inwater;
	tr.flFraction = gpGlobals->trace_fraction;
	tr.flPlaneDist = gpGlobals->trace_plane_dist;
	tr.pHit = gpGlobals->trace_ent;
	tr.vecEndPos = gpGlobals->trace_endpos;
	tr.vecPlaneNormal = gpGlobals->trace_plane_normal;
	tr.iHitgroup = gpGlobals->trace_hitgroup;
	return tr;
}


float util::VecToYaw(const Vector& vec)
{
	return VEC_TO_YAW(vec);
}


void util::ParticleEffect(const Vector& vecOrigin, const Vector& vecDirection, unsigned int ulColor, unsigned int ulCount)
{
	PARTICLE_EFFECT(vecOrigin, vecDirection, (float)ulColor, (float)ulCount);
}


float util::Approach(float target, float value, float speed)
{
	float delta = target - value;

	if (delta > speed)
		value += speed;
	else if (delta < -speed)
		value -= speed;
	else
		value = target;

	return value;
}


float util::ApproachAngle(float target, float value, float speed)
{
	target = util::AngleMod(target);
	value = util::AngleMod(target);

	float delta = target - value;

	// Speed is assumed to be positive
	if (speed < 0)
		speed = -speed;

	if (delta < -180)
		delta += 360;
	else if (delta > 180)
		delta -= 360;

	if (delta > speed)
		value += speed;
	else if (delta < -speed)
		value -= speed;
	else
		value = target;

	return value;
}


float util::AngleDistance(float next, float cur)
{
	float delta = next - cur;

	if (delta < -180)
		delta += 360;
	else if (delta > 180)
		delta -= 360;

	return delta;
}


float util::SplineFraction(float value, float scale)
{
	value = scale * value;
	float valueSquared = value * value;

	// Nice little ease-in, ease-out spline-like curve
	return 3 * valueSquared - 2 * valueSquared * value;
}


char* util::VarArgs(const char* format, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, format);
	vsprintf(string, format, argptr);
	va_end(argptr);

	return string;
}

Vector util::GetAimVector(edict_t* pent, float flSpeed)
{
	Vector tmp;
	GET_AIM_VECTOR(pent, flSpeed, tmp);
	return tmp;
}

bool util::IsMasterTriggered(string_t sMaster, CBaseEntity* pActivator)
{
	if (!FStringNull(sMaster))
	{
		edict_t* pentTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(sMaster));

		if (!FNullEnt(pentTarget))
		{
			CBaseEntity* pMaster = CBaseEntity::Instance(pentTarget);
			if (pMaster && (pMaster->ObjectCaps() & FCAP_MASTER) != 0)
				return pMaster->IsTriggered(pActivator);
		}

		ALERT(at_console, "Master was null or not a master!\n");
	}

	// if this isn't a master entity, just say yes.
	return true;
}

int util::PointContents(const Vector& vec)
{
	return POINT_CONTENTS(vec);
}

void util::BloodStream(const Vector& origin, const Vector& direction, int color, int amount)
{
	if (color == DONT_BLEED || amount == 0)
	{
		return;
	}

	MessageBegin(MSG_PVS, SVC_TEMPENTITY, origin);
	WriteByte(TE_BLOODSTREAM);
	WriteCoord(origin.x);
	WriteCoord(origin.y);
	WriteCoord(origin.z);
	WriteCoord(direction.x);
	WriteCoord(direction.y);
	WriteCoord(direction.z);
	WriteByte(color);
	WriteByte(std::min(amount, 255));
	MessageEnd();
}

void util::BloodDrips(const Vector& origin, const Vector& direction, int color, int amount)
{
	if (color == DONT_BLEED || amount == 0)
	{
		return;
	}

	if (util::IsDeathmatch())
	{
		// scale up blood effect in multiplayer for better visibility
		amount *= 2;
	}

	amount = std::min(amount, 255);

	MessageBegin(MSG_PVS, SVC_TEMPENTITY, origin);
	WriteByte(TE_BLOODSPRITE);
	WriteCoord(origin.x); // pos
	WriteCoord(origin.y);
	WriteCoord(origin.z);
	WriteShort(g_sModelIndexBloodSpray);		  // initial sprite model
	WriteShort(g_sModelIndexBloodDrop);		  // droplet sprite models
	WriteByte(color);							  // color index into host_basepal
	WriteByte(std::min(std::max(3, amount / 10), 16)); // size
	MessageEnd();
}

Vector util::RandomBloodVector()
{
	Vector direction;

	direction.x = RANDOM_FLOAT(-1, 1);
	direction.y = RANDOM_FLOAT(-1, 1);
	direction.z = RANDOM_FLOAT(0, 1);

	return direction;
}


void util::BloodDecalTrace(TraceResult* pTrace, int bloodColor)
{
	if (bloodColor == DONT_BLEED)
	{
		return;
	}
	if (bloodColor == BLOOD_COLOR_RED)
	{
		util::DecalTrace(pTrace, DECAL_BLOOD1 + RANDOM_LONG(0, 5));
	}
	else
	{
		util::DecalTrace(pTrace, DECAL_YBLOOD1 + RANDOM_LONG(0, 5));
	}
}


void util::DecalTrace(TraceResult* pTrace, int decalNumber)
{
	short entityIndex;
	int index;
	int message;

	if (decalNumber < 0)
		return;

	index = gDecals[decalNumber].index;

	if (index < 0)
		return;

	if (pTrace->flFraction == 1.0)
		return;

	// Only decal BSP models
	if (pTrace->pHit)
	{
		CBaseEntity* pEntity = CBaseEntity::Instance(pTrace->pHit);
		if (pEntity && !pEntity->IsBSPModel())
			return;
		entityIndex = ENTINDEX(pTrace->pHit);
	}
	else
		entityIndex = 0;

	message = TE_DECAL;
	if (entityIndex != 0)
	{
		if (index > 255)
		{
			message = TE_DECALHIGH;
			index -= 256;
		}
	}
	else
	{
		message = TE_WORLDDECAL;
		if (index > 255)
		{
			message = TE_WORLDDECALHIGH;
			index -= 256;
		}
	}

	MessageBegin(MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte(message);
	WriteCoord(pTrace->vecEndPos.x);
	WriteCoord(pTrace->vecEndPos.y);
	WriteCoord(pTrace->vecEndPos.z);
	WriteByte(index);
	if (0 != entityIndex)
		WriteShort(entityIndex);
	MessageEnd();
}

/*
==============
util::PlayerDecalTrace

A player is trying to apply his custom decal for the spray can.
Tell connected clients to display it, or use the default spray can decal
if the custom can't be loaded.
==============
*/
void util::PlayerDecalTrace(TraceResult* pTrace, int playernum, int decalNumber, bool bIsCustom)
{
	int index;

	if (!bIsCustom)
	{
		if (decalNumber < 0)
			return;

		index = gDecals[decalNumber].index;
		if (index < 0)
			return;
	}
	else
		index = decalNumber;

	if (pTrace->flFraction == 1.0)
		return;

	MessageBegin(MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte(TE_PLAYERDECAL);
	WriteByte(playernum);
	WriteCoord(pTrace->vecEndPos.x);
	WriteCoord(pTrace->vecEndPos.y);
	WriteCoord(pTrace->vecEndPos.z);
	WriteShort((short)ENTINDEX(pTrace->pHit));
	WriteByte(index);
	MessageEnd();
}

void util::GunshotDecalTrace(TraceResult* pTrace, int decalNumber)
{
	if (decalNumber < 0)
		return;

	int index = gDecals[decalNumber].index;
	if (index < 0)
		return;

	if (pTrace->flFraction == 1.0)
		return;

	MessageBegin(MSG_PAS, SVC_TEMPENTITY, pTrace->vecEndPos);
	WriteByte(TE_GUNSHOTDECAL);
	WriteCoord(pTrace->vecEndPos.x);
	WriteCoord(pTrace->vecEndPos.y);
	WriteCoord(pTrace->vecEndPos.z);
	WriteShort((short)ENTINDEX(pTrace->pHit));
	WriteByte(index);
	MessageEnd();
}


void util::Sparks(const Vector& position)
{
	MessageBegin(MSG_PVS, SVC_TEMPENTITY, position);
	WriteByte(TE_SPARKS);
	WriteCoord(position.x);
	WriteCoord(position.y);
	WriteCoord(position.z);
	MessageEnd();
}


void util::Ricochet(const Vector& position, float scale)
{
	MessageBegin(MSG_PVS, SVC_TEMPENTITY, position);
	WriteByte(TE_ARMOR_RICOCHET);
	WriteCoord(position.x);
	WriteCoord(position.y);
	WriteCoord(position.z);
	WriteByte((int)(scale * 10));
	MessageEnd();
}


bool util::TeamsMatch(const char* pTeamName1, const char* pTeamName2)
{
	// Everyone matches unless it's teamplay
	if (!g_pGameRules->IsTeamplay())
		return true;

	// Both on a team?
	if (*pTeamName1 != 0 && *pTeamName2 != 0)
	{
		if (!stricmp(pTeamName1, pTeamName2)) // Same Team?
			return true;
	}

	return false;
}


void util::StringToVector(float* pVector, const char* pString)
{
	char *pstr, *pfront, tempString[128];
	int j;

	strcpy(tempString, pString);
	pstr = pfront = tempString;

	for (j = 0; j < 3; j++) // lifted from pr_edict.c
	{
		pVector[j] = atof(pfront);

		while ('\0' != *pstr && *pstr != ' ')
			pstr++;
		if ('\0' == *pstr)
			break;
		pstr++;
		pfront = pstr;
	}
	if (j < 2)
	{
		/*
		ALERT( at_error, "Bad field in entity!! %s:%s == \"%s\"\n",
			pkvd->szClassName, pkvd->szKeyName, pkvd->szValue );
		*/
		for (j = j + 1; j < 3; j++)
			pVector[j] = 0;
	}
}


void util::StringToIntArray(int* pVector, int count, const char* pString)
{
	char *pstr, *pfront, tempString[128];
	int j;

	strcpy(tempString, pString);
	pstr = pfront = tempString;

	for (j = 0; j < count; j++) // lifted from pr_edict.c
	{
		pVector[j] = atoi(pfront);

		while ('\0' != *pstr && *pstr != ' ')
			pstr++;
		if ('\0' == *pstr)
			break;
		pstr++;
		pfront = pstr;
	}

	for (j++; j < count; j++)
	{
		pVector[j] = 0;
	}
}

Vector util::ClampVectorToBox(const Vector& input, const Vector& clampSize)
{
	Vector sourceVector = input;

	if (sourceVector.x > clampSize.x)
		sourceVector.x -= clampSize.x;
	else if (sourceVector.x < -clampSize.x)
		sourceVector.x += clampSize.x;
	else
		sourceVector.x = 0;

	if (sourceVector.y > clampSize.y)
		sourceVector.y -= clampSize.y;
	else if (sourceVector.y < -clampSize.y)
		sourceVector.y += clampSize.y;
	else
		sourceVector.y = 0;

	if (sourceVector.z > clampSize.z)
		sourceVector.z -= clampSize.z;
	else if (sourceVector.z < -clampSize.z)
		sourceVector.z += clampSize.z;
	else
		sourceVector.z = 0;

	return sourceVector.Normalize();
}


float util::WaterLevel(const Vector& position, float minz, float maxz)
{
	Vector midUp = position;
	midUp.z = minz;

	if (util::PointContents(midUp) != CONTENTS_WATER)
		return minz;

	midUp.z = maxz;
	if (util::PointContents(midUp) == CONTENTS_WATER)
		return maxz;

	float diff = maxz - minz;
	while (diff > 1.0)
	{
		midUp.z = minz + diff / 2.0;
		if (util::PointContents(midUp) == CONTENTS_WATER)
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

void util::Bubbles(Vector mins, Vector maxs, int count)
{
	Vector mid = (mins + maxs) * 0.5;

	float flHeight = util::WaterLevel(mid, mid.z, mid.z + 1024);
	flHeight = flHeight - mins.z;

	MessageBegin(MSG_PAS, SVC_TEMPENTITY, mid);
	WriteByte(TE_BUBBLES);
	WriteCoord(mins.x); // mins
	WriteCoord(mins.y);
	WriteCoord(mins.z);
	WriteCoord(maxs.x); // maxz
	WriteCoord(maxs.y);
	WriteCoord(maxs.z);
	WriteCoord(flHeight); // height
	WriteShort(g_sModelIndexBubbles);
	WriteByte(count); // count
	WriteCoord(8);	   // speed
	MessageEnd();
}

void util::BubbleTrail(Vector from, Vector to, int count)
{
	float flHeight = util::WaterLevel(from, from.z, from.z + 256);
	flHeight = flHeight - from.z;

	if (flHeight < 8)
	{
		flHeight = util::WaterLevel(to, to.z, to.z + 256);
		flHeight = flHeight - to.z;
		if (flHeight < 8)
			return;

		flHeight = flHeight + to.z - from.z;
	}

	if (count > 255)
		count = 255;

	MessageBegin(MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte(TE_BUBBLETRAIL);
	WriteCoord(from.x); // mins
	WriteCoord(from.y);
	WriteCoord(from.z);
	WriteCoord(to.x); // maxz
	WriteCoord(to.y);
	WriteCoord(to.z);
	WriteCoord(flHeight); // height
	WriteShort(g_sModelIndexBubbles);
	WriteByte(count); // count
	WriteCoord(8);	   // speed
	MessageEnd();
}


bool util::IsValidEntity(edict_t* pent)
{
	if (!pent || 0 != pent->free || (pent->v.flags & FL_KILLME) != 0)
		return false;
	return true;
}


void util::PrecacheOther(const char* szClassname)
{
	edict_t* pent;

	pent = CREATE_NAMED_ENTITY(MAKE_STRING(szClassname));
	if (FNullEnt(pent))
	{
		ALERT(at_console, "NULL Ent in util::PrecacheOther\n");
		return;
	}

	CBaseEntity* pEntity = CBaseEntity::Instance(VARS(pent));
	if (pEntity)
		pEntity->Precache();
	g_engfuncs.pfnRemoveEntity(pent);
}

//=========================================================
// util::LogPrintf - Prints a logged message to console.
// Preceded by LOG: ( timestamp ) < message >
//=========================================================
void util::LogPrintf(const char* fmt, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, fmt);
	vsprintf(string, fmt, argptr);
	va_end(argptr);

	// Print to server console
	ALERT(at_logged, "%s", string);
}

//=========================================================
// util::DotPoints - returns the dot product of a line from
// src to check and vecdir.
//=========================================================
float util::DotPoints(const Vector& vecSrc, const Vector& vecCheck, const Vector& vecDir)
{
	Vector2D vec2LOS;

	vec2LOS = (vecCheck - vecSrc).Make2D();
	vec2LOS = vec2LOS.Normalize();

	return DotProduct(vec2LOS, (vecDir.Make2D()));
}


//=========================================================
// util::StripToken - for redundant keynames
//=========================================================
void util::StripToken(const char* pKey, char* pDest)
{
	int i = 0;

	while ('\0' != pKey[i] && pKey[i] != '#')
	{
		pDest[i] = pKey[i];
		i++;
	}
	pDest[i] = 0;
}

gamemode_e util::GetGameMode()
{
	return g_pGameRules->GetGameMode();
}

bool util::IsMultiplayer()
{
	return util::GetGameMode() > kGamemodeSingleplayer;
}

bool util::IsDeathmatch()
{
	return util::GetGameMode() > kGamemodeCooperative;
}

// --------------------------------------------------------------
//
// CSave
//
// --------------------------------------------------------------
static int gSizes[FIELD_TYPECOUNT] =
	{
		sizeof(float),	   // FIELD_FLOAT
		sizeof(int),	   // FIELD_STRING
		sizeof(int),	   // FIELD_ENTITY
		sizeof(int),	   // FIELD_CLASSPTR
		sizeof(EHANDLE),   // FIELD_EHANDLE
		sizeof(int),	   // FIELD_entvars_t
		sizeof(int),	   // FIELD_EDICT
		sizeof(float) * 3, // FIELD_VECTOR
		sizeof(float) * 3, // FIELD_POSITION_VECTOR
		sizeof(int*),	   // FIELD_POINTER
		sizeof(int),	   // FIELD_INTEGER
#ifdef GNUC
		sizeof(int*) * 2, // FIELD_FUNCTION
#else
		sizeof(int*), // FIELD_FUNCTION
#endif
		sizeof(byte),		   // FIELD_BOOLEAN
		sizeof(short),		   // FIELD_SHORT
		sizeof(char),		   // FIELD_CHARACTER
		sizeof(float),		   // FIELD_TIME
		sizeof(int),		   // FIELD_MODELNAME
		sizeof(int),		   // FIELD_SOUNDNAME
		sizeof(std::uint64_t), //FIELD_INT64
};


// Base class includes common SAVERESTOREDATA pointer, and manages the entity table
CSaveRestoreBuffer::CSaveRestoreBuffer(SAVERESTOREDATA& data)
	: m_data(data)
{
}


CSaveRestoreBuffer::~CSaveRestoreBuffer() = default;

int CSaveRestoreBuffer::EntityIndex(CBaseEntity* pEntity)
{
	if (pEntity == NULL)
		return -1;
	return EntityIndex(pEntity->pev);
}


int CSaveRestoreBuffer::EntityIndex(entvars_t* pevLookup)
{
	if (pevLookup == NULL)
		return -1;
	return EntityIndex(ENT(pevLookup));
}

int CSaveRestoreBuffer::EntityIndex(EOFFSET eoLookup)
{
	return EntityIndex(ENT(eoLookup));
}


int CSaveRestoreBuffer::EntityIndex(edict_t* pentLookup)
{
	if (pentLookup == NULL)
		return -1;

	int i;
	ENTITYTABLE* pTable;

	for (i = 0; i < m_data.tableCount; i++)
	{
		pTable = m_data.pTable + i;
		if (pTable->pent == pentLookup)
			return i;
	}
	return -1;
}


edict_t* CSaveRestoreBuffer::EntityFromIndex(int entityIndex)
{
	if (entityIndex < 0)
		return NULL;

	int i;
	ENTITYTABLE* pTable;

	for (i = 0; i < m_data.tableCount; i++)
	{
		pTable = m_data.pTable + i;
		if (pTable->id == entityIndex)
			return pTable->pent;
	}
	return NULL;
}


int CSaveRestoreBuffer::EntityFlagsSet(int entityIndex, int flags)
{
	if (entityIndex < 0)
		return 0;
	if (entityIndex > m_data.tableCount)
		return 0;

	m_data.pTable[entityIndex].flags |= flags;

	return m_data.pTable[entityIndex].flags;
}


void CSaveRestoreBuffer::BufferRewind(int size)
{
	if (m_data.size < size)
		size = m_data.size;

	m_data.pCurrentData -= size;
	m_data.size -= size;
}

#ifndef WIN32
extern "C" {
unsigned _rotr(unsigned val, int shift)
{
	unsigned lobit;	 /* non-zero means lo bit set */
	unsigned num = val; /* number to rotate */

	shift &= 0x1f; /* modulo 32 -- this will also make
										   negative shifts work */

	while (shift--)
	{
		lobit = num & 1; /* get high bit */
		num >>= 1;		 /* shift right one bit */
		if (lobit)
			num |= 0x80000000; /* set hi bit if lo bit was set */
	}

	return num;
}
}
#endif

unsigned int CSaveRestoreBuffer::HashString(const char* pszToken)
{
	unsigned int hash = 0;

	while ('\0' != *pszToken)
		hash = _rotr(hash, 4) ^ *pszToken++;

	return hash;
}

unsigned short CSaveRestoreBuffer::TokenHash(const char* pszToken)
{
#if _DEBUG
	static int tokensparsed = 0;
	tokensparsed++;
#endif
	if (0 == m_data.tokenCount || nullptr == m_data.pTokens)
	{
		//if we're here it means trigger_changelevel is trying to actually save something when it's not supposed to.
		ALERT(at_error, "No token table array in TokenHash()!\n");
		return 0;
	}

	const unsigned short hash = (unsigned short)(HashString(pszToken) % (unsigned)m_data.tokenCount);

	for (int i = 0; i < m_data.tokenCount; i++)
	{
#if _DEBUG
		static bool beentheredonethat = false;
		if (i > 50 && !beentheredonethat)
		{
			beentheredonethat = true;
			ALERT(at_error, "CSaveRestoreBuffer :: TokenHash() is getting too full!\n");
		}
#endif

		int index = hash + i;
		if (index >= m_data.tokenCount)
			index -= m_data.tokenCount;

		if (!m_data.pTokens[index] || strcmp(pszToken, m_data.pTokens[index]) == 0)
		{
			m_data.pTokens[index] = (char*)pszToken;
			return index;
		}
	}

	// Token hash table full!!!
	// [Consider doing overflow table(s) after the main table & limiting linear hash table search]
	ALERT(at_error, "CSaveRestoreBuffer :: TokenHash() is COMPLETELY FULL!\n");
	return 0;
}

void CSave::WriteData(const char* pname, int size, const char* pdata)
{
	BufferField(pname, size, pdata);
}


void CSave::WriteShort(const char* pname, const short* data, int count)
{
	BufferField(pname, sizeof(short) * count, (const char*)data);
}


void CSave::WriteInt(const char* pname, const int* data, int count)
{
	BufferField(pname, sizeof(int) * count, (const char*)data);
}


void CSave::WriteFloat(const char* pname, const float* data, int count)
{
	BufferField(pname, sizeof(float) * count, (const char*)data);
}


void CSave::WriteTime(const char* pname, const float* data, int count)
{
	int i;
	Vector tmp, input;

	BufferHeader(pname, sizeof(float) * count);
	for (i = 0; i < count; i++)
	{
		float tmp = data[0];

		// Always encode time as a delta from the current time so it can be re-based if loaded in a new level
		// Times of 0 are never written to the file, so they will be restored as 0, not a relative time
		tmp -= m_data.time;

		BufferData((const char*)&tmp, sizeof(float));
		data++;
	}
}


void CSave::WriteString(const char* pname, const char* pdata)
{
#ifdef TOKENIZE
	short token = (short)TokenHash(pdata);
	WriteShort(pname, &token, 1);
#else
	BufferField(pname, strlen(pdata) + 1, pdata);
#endif
}


void CSave::WriteString(const char* pname, const int* stringId, int count)
{
	int i, size;

#ifdef TOKENIZE
	short token = (short)TokenHash(STRING(*stringId));
	WriteShort(pname, &token, 1);
#else
#if 0
	if ( count != 1 )
		ALERT( at_error, "No string arrays!\n" );
	WriteString( pname, (char *)STRING(*stringId) );
#endif

	size = 0;
	for (i = 0; i < count; i++)
		size += strlen(STRING(stringId[i])) + 1;

	BufferHeader(pname, size);
	for (i = 0; i < count; i++)
	{
		const char* pString = STRING(stringId[i]);
		BufferData(pString, strlen(pString) + 1);
	}
#endif
}


void CSave::WriteVector(const char* pname, const Vector& value)
{
	WriteVector(pname, &value.x, 1);
}


void CSave::WriteVector(const char* pname, const float* value, int count)
{
	BufferHeader(pname, sizeof(float) * 3 * count);
	BufferData((const char*)value, sizeof(float) * 3 * count);
}



void CSave::WritePositionVector(const char* pname, const Vector& value)
{
	if (0 != m_data.fUseLandmark)
	{
		Vector tmp = value - m_data.vecLandmarkOffset;
		WriteVector(pname, tmp);
	}

	WriteVector(pname, value);
}


void CSave::WritePositionVector(const char* pname, const float* value, int count)
{
	int i;
	Vector tmp, input;

	BufferHeader(pname, sizeof(float) * 3 * count);
	for (i = 0; i < count; i++)
	{
		Vector tmp(value[0], value[1], value[2]);

		if (0 != m_data.fUseLandmark)
			tmp = tmp - m_data.vecLandmarkOffset;

		BufferData((const char*)&tmp.x, sizeof(float) * 3);
		value += 3;
	}
}


void CSave::WriteFunction(const char* pname, void** data, int count)
{
	const char* functionName;

	functionName = NAME_FOR_FUNCTION((uint32)*data);
	if (functionName)
		BufferField(pname, strlen(functionName) + 1, functionName);
	else
		ALERT(at_error, "Invalid function pointer in entity!\n");
}


void EntvarsKeyvalue(entvars_t* pev, KeyValueData* pkvd)
{
	int i;
	TYPEDESCRIPTION* pField;

	for (i = 0; i < ENTVARS_COUNT; i++)
	{
		pField = &gEntvarsDescription[i];

		if (!stricmp(pField->fieldName, pkvd->szKeyName))
		{
			switch (pField->fieldType)
			{
			case FIELD_MODELNAME:
			case FIELD_SOUNDNAME:
			case FIELD_STRING:
				(*(int*)((char*)pev + pField->fieldOffset)) = ALLOC_STRING(pkvd->szValue);
				break;

			case FIELD_TIME:
			case FIELD_FLOAT:
				(*(float*)((char*)pev + pField->fieldOffset)) = atof(pkvd->szValue);
				break;

			case FIELD_INTEGER:
				(*(int*)((char*)pev + pField->fieldOffset)) = atoi(pkvd->szValue);
				break;

			case FIELD_POSITION_VECTOR:
			case FIELD_VECTOR:
				util::StringToVector((float*)((char*)pev + pField->fieldOffset), pkvd->szValue);
				break;

			default:
			case FIELD_EVARS:
			case FIELD_CLASSPTR:
			case FIELD_EDICT:
			case FIELD_ENTITY:
			case FIELD_POINTER:
				ALERT(at_error, "Bad field in entity!!\n");
				break;
			}
			pkvd->fHandled = 1;
			return;
		}
	}
}



bool CSave::WriteEntVars(const char* pname, entvars_t* pev)
{
	return WriteFields(pname, pev, gEntvarsDescription, ENTVARS_COUNT);
}



bool CSave::WriteFields(const char* pname, void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount)
{
	int i, j, actualCount, emptyCount;
	TYPEDESCRIPTION* pTest;
	int entityArray[MAX_ENTITYARRAY];
	byte boolArray[MAX_ENTITYARRAY];

	// Precalculate the number of empty fields
	emptyCount = 0;
	for (i = 0; i < fieldCount; i++)
	{
		void* pOutputData;
		pOutputData = ((char*)pBaseData + pFields[i].fieldOffset);
		if (DataEmpty((const char*)pOutputData, pFields[i].fieldSize * gSizes[pFields[i].fieldType]))
			emptyCount++;
	}

	// Empty fields will not be written, write out the actual number of fields to be written
	actualCount = fieldCount - emptyCount;
	WriteInt(pname, &actualCount, 1);

	for (i = 0; i < fieldCount; i++)
	{
		void* pOutputData;
		pTest = &pFields[i];
		pOutputData = ((char*)pBaseData + pTest->fieldOffset);

		// UNDONE: Must we do this twice?
		if (DataEmpty((const char*)pOutputData, pTest->fieldSize * gSizes[pTest->fieldType]))
			continue;

		switch (pTest->fieldType)
		{
		case FIELD_FLOAT:
			WriteFloat(pTest->fieldName, (float*)pOutputData, pTest->fieldSize);
			break;
		case FIELD_TIME:
			WriteTime(pTest->fieldName, (float*)pOutputData, pTest->fieldSize);
			break;
		case FIELD_MODELNAME:
		case FIELD_SOUNDNAME:
		case FIELD_STRING:
			WriteString(pTest->fieldName, (int*)pOutputData, pTest->fieldSize);
			break;
		case FIELD_CLASSPTR:
		case FIELD_EVARS:
		case FIELD_EDICT:
		case FIELD_ENTITY:
		case FIELD_EHANDLE:
			if (pTest->fieldSize > MAX_ENTITYARRAY)
				ALERT(at_error, "Can't save more than %d entities in an array!!!\n", MAX_ENTITYARRAY);
			for (j = 0; j < pTest->fieldSize; j++)
			{
				switch (pTest->fieldType)
				{
				case FIELD_EVARS:
					entityArray[j] = EntityIndex(((entvars_t**)pOutputData)[j]);
					break;
				case FIELD_CLASSPTR:
					entityArray[j] = EntityIndex(((CBaseEntity**)pOutputData)[j]);
					break;
				case FIELD_EDICT:
					entityArray[j] = EntityIndex(((edict_t**)pOutputData)[j]);
					break;
				case FIELD_ENTITY:
					entityArray[j] = EntityIndex(((EOFFSET*)pOutputData)[j]);
					break;
				case FIELD_EHANDLE:
					entityArray[j] = EntityIndex((CBaseEntity*)(((EHANDLE*)pOutputData)[j]));
					break;
				}
			}
			WriteInt(pTest->fieldName, entityArray, pTest->fieldSize);
			break;
		case FIELD_POSITION_VECTOR:
			WritePositionVector(pTest->fieldName, (float*)pOutputData, pTest->fieldSize);
			break;
		case FIELD_VECTOR:
			WriteVector(pTest->fieldName, (float*)pOutputData, pTest->fieldSize);
			break;

		case FIELD_BOOLEAN:
		{
			//TODO: need to refactor save game stuff to make this cleaner and reusable
			//Convert booleans to bytes
			for (j = 0; j < pTest->fieldSize; j++)
			{
				boolArray[j] = ((bool*)pOutputData)[j] ? 1 : 0;
			}

			WriteData(pTest->fieldName, pTest->fieldSize, (char*)boolArray);
		}
		break;

		case FIELD_INTEGER:
			WriteInt(pTest->fieldName, (int*)pOutputData, pTest->fieldSize);
			break;

		case FIELD_INT64:
			WriteData(pTest->fieldName, sizeof(std::uint64_t) * pTest->fieldSize, ((char*)pOutputData));
			break;

		case FIELD_SHORT:
			WriteData(pTest->fieldName, 2 * pTest->fieldSize, ((char*)pOutputData));
			break;

		case FIELD_CHARACTER:
			WriteData(pTest->fieldName, pTest->fieldSize, ((char*)pOutputData));
			break;

		// For now, just write the address out, we're not going to change memory while doing this yet!
		case FIELD_POINTER:
			WriteInt(pTest->fieldName, (int*)(char*)pOutputData, pTest->fieldSize);
			break;

		case FIELD_FUNCTION:
			WriteFunction(pTest->fieldName, (void**)pOutputData, pTest->fieldSize);
			break;
		default:
			ALERT(at_error, "Bad field type\n");
		}
	}

	return true;
}


void CSave::BufferString(char* pdata, int len)
{
	char c = 0;

	BufferData(pdata, len); // Write the string
	BufferData(&c, 1);		// Write a null terminator
}


bool CSave::DataEmpty(const char* pdata, int size)
{
	for (int i = 0; i < size; i++)
	{
		if (0 != pdata[i])
			return false;
	}
	return true;
}


void CSave::BufferField(const char* pname, int size, const char* pdata)
{
	BufferHeader(pname, size);
	BufferData(pdata, size);
}


void CSave::BufferHeader(const char* pname, int size)
{
	short hashvalue = TokenHash(pname);
	if (size > 1 << (sizeof(short) * 8))
		ALERT(at_error, "CSave :: BufferHeader() size parameter exceeds 'short'!\n");
	BufferData((const char*)&size, sizeof(short));
	BufferData((const char*)&hashvalue, sizeof(short));
}


void CSave::BufferData(const char* pdata, int size)
{
	if (m_data.size + size > m_data.bufferSize)
	{
		ALERT(at_error, "Save/Restore overflow!\n");
		m_data.size = m_data.bufferSize;
		return;
	}

	memcpy(m_data.pCurrentData, pdata, size);
	m_data.pCurrentData += size;
	m_data.size += size;
}



// --------------------------------------------------------------
//
// CRestore
//
// --------------------------------------------------------------

int CRestore::ReadField(void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount, int startField, int size, char* pName, void* pData)
{
	int i, j, stringCount, fieldNumber, entityIndex;
	TYPEDESCRIPTION* pTest;
	float timeData;
	Vector position;
	edict_t* pent;
	char* pString;

	position = Vector(0, 0, 0);

	if (0 != m_data.fUseLandmark)
		position = m_data.vecLandmarkOffset;

	for (i = 0; i < fieldCount; i++)
	{
		fieldNumber = (i + startField) % fieldCount;
		pTest = &pFields[fieldNumber];
		if (!stricmp(pTest->fieldName, pName))
		{
			if (!m_global || (pTest->flags & FTYPEDESC_GLOBAL) == 0)
			{
				for (j = 0; j < pTest->fieldSize; j++)
				{
					void* pOutputData = ((char*)pBaseData + pTest->fieldOffset + (j * gSizes[pTest->fieldType]));
					void* pInputData = (char*)pData + j * gSizes[pTest->fieldType];

					switch (pTest->fieldType)
					{
					case FIELD_TIME:
						timeData = *(float*)pInputData;
						// Re-base time variables
						timeData += m_data.time;
						*((float*)pOutputData) = timeData;
						break;
					case FIELD_FLOAT:
						*((float*)pOutputData) = *(float*)pInputData;
						break;
					case FIELD_MODELNAME:
					case FIELD_SOUNDNAME:
					case FIELD_STRING:
						// Skip over j strings
						pString = (char*)pData;
						for (stringCount = 0; stringCount < j; stringCount++)
						{
							while ('\0' != *pString)
								pString++;
							pString++;
						}
						pInputData = pString;
						if (strlen((char*)pInputData) == 0)
							*((int*)pOutputData) = 0;
						else
						{
							int string;

							string = ALLOC_STRING((char*)pInputData);

							*((int*)pOutputData) = string;

							if (!FStringNull(string) && m_precache)
							{
								if (pTest->fieldType == FIELD_MODELNAME)
									PRECACHE_MODEL((char*)STRING(string));
								else if (pTest->fieldType == FIELD_SOUNDNAME)
									PRECACHE_SOUND((char*)STRING(string));
							}
						}
						break;
					case FIELD_EVARS:
						entityIndex = *(int*)pInputData;
						pent = EntityFromIndex(entityIndex);
						if (pent)
							*((entvars_t**)pOutputData) = VARS(pent);
						else
							*((entvars_t**)pOutputData) = NULL;
						break;
					case FIELD_CLASSPTR:
						entityIndex = *(int*)pInputData;
						pent = EntityFromIndex(entityIndex);
						if (pent)
							*((CBaseEntity**)pOutputData) = CBaseEntity::Instance(pent);
						else
							*((CBaseEntity**)pOutputData) = NULL;
						break;
					case FIELD_EDICT:
						entityIndex = *(int*)pInputData;
						pent = EntityFromIndex(entityIndex);
						*((edict_t**)pOutputData) = pent;
						break;
					case FIELD_EHANDLE:
						// Input and Output sizes are different!
						pInputData = (char*)pData + j * sizeof(int);
						entityIndex = *(int*)pInputData;
						pent = EntityFromIndex(entityIndex);
						if (pent)
							*((EHANDLE*)pOutputData) = CBaseEntity::Instance(pent);
						else
							*((EHANDLE*)pOutputData) = NULL;
						break;
					case FIELD_ENTITY:
						entityIndex = *(int*)pInputData;
						pent = EntityFromIndex(entityIndex);
						if (pent)
							*((EOFFSET*)pOutputData) = OFFSET(pent);
						else
							*((EOFFSET*)pOutputData) = 0;
						break;
					case FIELD_VECTOR:
						((float*)pOutputData)[0] = ((float*)pInputData)[0];
						((float*)pOutputData)[1] = ((float*)pInputData)[1];
						((float*)pOutputData)[2] = ((float*)pInputData)[2];
						break;
					case FIELD_POSITION_VECTOR:
						((float*)pOutputData)[0] = ((float*)pInputData)[0] + position.x;
						((float*)pOutputData)[1] = ((float*)pInputData)[1] + position.y;
						((float*)pOutputData)[2] = ((float*)pInputData)[2] + position.z;
						break;

					case FIELD_BOOLEAN:
					{
						// Input and Output sizes are different!
						pOutputData = (char*)pOutputData + j * (sizeof(bool) - gSizes[pTest->fieldType]);
						const bool value = *((byte*)pInputData) != 0;

						*((bool*)pOutputData) = value;
					}
					break;

					case FIELD_INTEGER:
						*((int*)pOutputData) = *(int*)pInputData;
						break;

					case FIELD_INT64:
						*((std::uint64_t*)pOutputData) = *(std::uint64_t*)pInputData;
						break;

					case FIELD_SHORT:
						*((short*)pOutputData) = *(short*)pInputData;
						break;

					case FIELD_CHARACTER:
						*((char*)pOutputData) = *(char*)pInputData;
						break;

					case FIELD_POINTER:
						*((int*)pOutputData) = *(int*)pInputData;
						break;
					case FIELD_FUNCTION:
						if (strlen((char*)pInputData) == 0)
							*((int*)pOutputData) = 0;
						else
							*((int*)pOutputData) = FUNCTION_FROM_NAME((char*)pInputData);
						break;

					default:
						ALERT(at_error, "Bad field type\n");
					}
				}
			}
#if 0
			else
			{
				ALERT( at_console, "Skipping global field %s\n", pName );
			}
#endif
			return fieldNumber;
		}
	}

	return -1;
}


bool CRestore::ReadEntVars(const char* pname, entvars_t* pev)
{
	return ReadFields(pname, pev, gEntvarsDescription, ENTVARS_COUNT);
}


bool CRestore::ReadFields(const char* pname, void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount)
{
	unsigned short i, token;
	int lastField, fileCount;
	HEADER header;

	i = ReadShort();

	token = ReadShort();

	// Check the struct name
	if (token != TokenHash(pname)) // Field Set marker
	{
		//		ALERT( at_error, "Expected %s found %s!\n", pname, BufferPointer() );
		BufferRewind(2 * sizeof(short));
		return false;
	}

	// Skip over the struct name
	fileCount = ReadInt(); // Read field count

	lastField = 0; // Make searches faster, most data is read/written in the same order

	// Clear out base data
	for (i = 0; i < fieldCount; i++)
	{
		// Don't clear global fields
		if (!m_global || (pFields[i].flags & FTYPEDESC_GLOBAL) == 0)
			memset(((char*)pBaseData + pFields[i].fieldOffset), 0, pFields[i].fieldSize * gSizes[pFields[i].fieldType]);
	}

	for (i = 0; i < fileCount; i++)
	{
		BufferReadHeader(&header);
		lastField = ReadField(pBaseData, pFields, fieldCount, lastField, header.size, m_data.pTokens[header.token], header.pData);
		lastField++;
	}

	return true;
}


void CRestore::BufferReadHeader(HEADER* pheader)
{
	pheader->size = ReadShort();	  // Read field size
	pheader->token = ReadShort();	  // Read field name token
	pheader->pData = BufferPointer(); // Field Data is next
	BufferSkipBytes(pheader->size);	  // Advance to next field
}


short CRestore::ReadShort()
{
	short tmp = 0;

	BufferReadBytes((char*)&tmp, sizeof(short));

	return tmp;
}

int CRestore::ReadInt()
{
	int tmp = 0;

	BufferReadBytes((char*)&tmp, sizeof(int));

	return tmp;
}

int CRestore::ReadNamedInt(const char* pName)
{
	HEADER header;

	BufferReadHeader(&header);
	return ((int*)header.pData)[0];
}

char* CRestore::ReadNamedString(const char* pName)
{
	HEADER header;

	BufferReadHeader(&header);
#ifdef TOKENIZE
	return (char*)(m_pdata->pTokens[*(short*)header.pData]);
#else
	return (char*)header.pData;
#endif
}


char* CRestore::BufferPointer()
{
	return m_data.pCurrentData;
}

void CRestore::BufferReadBytes(char* pOutput, int size)
{
	if (Empty())
		return;

	if ((m_data.size + size) > m_data.bufferSize)
	{
		ALERT(at_error, "Restore overflow!\n");
		m_data.size = m_data.bufferSize;
		return;
	}

	if (pOutput)
		memcpy(pOutput, m_data.pCurrentData, size);
	m_data.pCurrentData += size;
	m_data.size += size;
}


void CRestore::BufferSkipBytes(int bytes)
{
	BufferReadBytes(NULL, bytes);
}

int CRestore::BufferSkipZString()
{
	const int maxLen = m_data.bufferSize - m_data.size;

	int len = 0;
	char* pszSearch = m_data.pCurrentData;
	while ('\0' != *pszSearch++ && len < maxLen)
		len++;

	len++;

	BufferSkipBytes(len);

	return len;
}

bool CRestore::BufferCheckZString(const char* string)
{
	const int maxLen = m_data.bufferSize - m_data.size;
	const int len = strlen(string);
	if (len <= maxLen)
	{
		if (0 == strncmp(string, m_data.pCurrentData, len))
			return true;
	}
	return false;
}
