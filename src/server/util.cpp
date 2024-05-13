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
#include <time.h>
#include "shake.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "UserMessages.h"

inline void MessageBegin(int dest, int type, const Vector& origin, CBaseEntity* entity)
{
	engine::MessageBegin(
		dest,
		type,
		origin,
		entity ? &entity->v : nullptr);
}

inline void MessageBegin(int dest, int type, CBaseEntity* entity)
{
	engine::MessageBegin(
		dest,
		type,
		nullptr,
		entity ? &entity->v : nullptr);
}

CBaseEntity* util::FindEntityForward(CBaseEntity* pMe)
{
	TraceResult tr;

	util::MakeVectors(pMe->v.v_angle);
	util::TraceLine(pMe->v.origin + pMe->v.view_ofs, pMe->v.origin + pMe->v.view_ofs + gpGlobals->v_forward * 8192, dont_ignore_monsters, pMe, &tr);
	if (tr.flFraction != 1.0 && tr.pHit != nullptr)
	{
		CBaseEntity* pHit = tr.pHit->Get<CBaseEntity>();
		return pHit;
	}
	return nullptr;
}

static unsigned int glSeed = 0;
static unsigned int seed_table[256] =
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
	25678, 18555, 13256, 23316, 22407, 16727, 991, 9236, 5373, 29402, 6117, 15241, 27715, 19291, 19888, 19847
};

static unsigned int U_Random()
{
	glSeed *= 69069;
	glSeed += seed_table[glSeed & 0xff];

	return (++glSeed & 0x0fffffff);
}

static void U_Srand(unsigned int seed)
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

	engine::SetGroupMask(g_groupmask, g_groupop);
}

void util::UnsetGroupTrace()
{
	g_groupmask = 0;
	g_groupop = 0;

	engine::SetGroupMask(0, 0);
}

// Smart version, it'll clean itself up when it pops off stack
util::GroupTrace::GroupTrace(int groupmask, int op)
{
	m_oldgroupmask = g_groupmask;
	m_oldgroupop = g_groupop;

	g_groupmask = groupmask;
	g_groupop = op;

	engine::SetGroupMask(g_groupmask, g_groupop);
}

util::GroupTrace::~GroupTrace()
{
	g_groupmask = m_oldgroupmask;
	g_groupop = m_oldgroupop;

	engine::SetGroupMask(g_groupmask, g_groupop);
}

Entity* util::GetEntityList()
{
	return engine::PEntityOfEntOffset(0);
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
	engine::VecToAngles(vec, rgflVecOut);
	return Vector(rgflVecOut);
}


int util::EntitiesInBox(CBaseEntity** pList, int listMax, const Vector& mins, const Vector& maxs, int flagMask, bool checkSolid)
{
	Entity* pEdict = util::GetEntityList();
	CBaseEntity* pEntity;
	int count;

	count = 0;

	if (!pEdict)
		return count;

	// Ignore world.
	++pEdict;

	for (int i = 1; i < gpGlobals->maxEntities; i++, pEdict++)
	{
		if (pEdict->IsFree()) // Not in use
			continue;

		if (0 != flagMask && (pEdict->flags & flagMask) == 0) // Does it meet the criteria?
			continue;

		if (0 != checkSolid && (pEdict->solid == SOLID_NOT || pEdict->mins.x == pEdict->maxs.x))
			continue;

		if (mins.x > pEdict->absmax.x ||
			mins.y > pEdict->absmax.y ||
			mins.z > pEdict->absmax.z ||
			maxs.x < pEdict->absmin.x ||
			maxs.y < pEdict->absmin.y ||
			maxs.z < pEdict->absmin.z)
			continue;

		pEntity = pEdict->Get<CBaseEntity>();
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
	Entity* pEdict = util::GetEntityList();
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
		if (pEdict->IsFree()) // Not in use
			continue;

		if ((pEdict->flags & (FL_CLIENT | FL_MONSTER)) == 0) // Not a client/monster ?
			continue;

		// Use origin for X & Y since they are centered for all monsters
		// Now X
		delta = center.x - pEdict->origin.x;
		delta *= delta;

		if (delta > radiusSquared)
			continue;
		distance = delta;

		// Now Y
		delta = center.y - pEdict->origin.y;
		delta *= delta;

		distance += delta;
		if (distance > radiusSquared)
			continue;

		// Now Z
		delta = center.z - (pEdict->absmin.z + pEdict->absmax.z) * 0.5;
		delta *= delta;

		distance += delta;
		if (distance > radiusSquared)
			continue;

		pEntity = pEdict->Get<CBaseEntity>();
		if (!pEntity)
			continue;

		pList[count] = pEntity;
		count++;

		if (count >= listMax)
			return count;
	}


	return count;
}


util::EntityIterator::EntityIterator(const char* key, const char* value, CBaseEntity* start)
{
	_key = key;
	_value = value;
	_start = (start != nullptr) ? &start->v : &CWorld::World->v;
	_current = engine::FindEntityByString(_start, _key, _value);
}

void util::EntityIterator::operator++()
{
	_current = engine::FindEntityByString(_current, _key, _value);
}

util::EntityIterator::operator bool()
{
	return _current != &CWorld::World->v && _current != _start;
}


CBaseEntity* util::FindEntityInSphere(CBaseEntity* pStartEntity, const Vector& vecCenter, float flRadius)
{
	Entity* pentEntity;

	if (pStartEntity)
		pentEntity = pStartEntity->edict();
	else
		pentEntity = nullptr;

	pentEntity = engine::FindEntityInSphere(pentEntity, vecCenter, flRadius);

	if (pentEntity != nullptr && OFFSET(pentEntity) != 0)
		return pentEntity->Get<CBaseEntity>();
	return nullptr;
}


CBaseEntity* util::FindEntityByString(CBaseEntity* pStartEntity, const char* szKeyword, const char* szValue)
{
	Entity* entity = nullptr;

	if (pStartEntity != nullptr)
	{
		entity = &pStartEntity->v;
	}

	entity = engine::FindEntityByString(entity, szKeyword, szValue);

	if (entity != nullptr && engine::EntOffsetOfPEntity(entity) != 0)
	{
		return entity->Get<CBaseEntity>();
	}

	return nullptr;
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
	CBaseEntity* pEntity = nullptr;

	pEntity = util::FindEntityByTargetname(nullptr, szWhatever);
	if (pEntity)
		return pEntity;

	CBaseEntity* pSearch = nullptr;
	float flMaxDist2 = flRadius * flRadius;
	while ((pSearch = util::FindEntityByClassname(pSearch, szWhatever)) != nullptr)
	{
		float flDist2 = (pSearch->v.origin - vecSrc).Length();
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
// otherwise returns nullptr
// Index is 1 based
CBaseEntity* util::PlayerByIndex(int playerIndex)
{
	CBaseEntity* player = nullptr;

	if (playerIndex > 0 && playerIndex <= gpGlobals->maxClients)
	{
		auto entity = engine::PEntityOfEntIndex(playerIndex);

		if (entity != nullptr && !entity->IsFree())
		{
			player = entity->Get<CBaseEntity>();
		}
	}

	return player;
}


void util::MakeVectors(const Vector& vecAngles)
{
	engine::MakeVectors(vecAngles);
}


void util::MakeAimVectors(const Vector& vecAngles)
{
	float rgflVec[3];
	vecAngles.CopyToArray(rgflVec);
	rgflVec[0] = -rgflVec[0];
	engine::MakeVectors(rgflVec);
}

void util::MakeInvVectors(const Vector& vec, globalvars_t* pgv)
{
	engine::MakeVectors(vec);

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

		if (!pPlayer || (pPlayer->v.flags & FL_ONGROUND) == 0) // Don't shake if not onground
			continue;

		localAmplitude = 0;

		if (radius <= 0)
			localAmplitude = amplitude;
		else
		{
			Vector delta = center - pPlayer->v.origin;
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

void util::ClientHearVox(CBaseEntity* client, const char* sentence)
{
	MessageBegin(MSG_ONE, SVC_STUFFTEXT, client);
	if (sentence[0] == '#')
		WriteString(util::VarArgs("spk %s\n", sentence));
	else
		WriteString(util::VarArgs("spk \"%s\"\n", sentence));
	MessageEnd();
}

void util::ClientHearVoxAll(const char* sentence)
{
	MessageBegin(MSG_ALL, SVC_STUFFTEXT);
	if (sentence[0] == '#')
		WriteString(util::VarArgs("spk %s\n", sentence));
	else
		WriteString(util::VarArgs("spk \"%s\"\n", sentence));
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
	engine::TraceLine(vecStart, vecEnd, (igmon == ignore_monsters ? 1 : 0) | (ignore_glass == ignoreGlass ? 0x100 : 0), ignore ? ignore->edict() : nullptr, ptr);
}


void util::TraceLine(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, CBaseEntity* ignore, TraceResult* ptr)
{
	engine::TraceLine(vecStart, vecEnd, (igmon == ignore_monsters ? 1 : 0), ignore ? ignore->edict() : nullptr, ptr);
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

	Entity* ignoreEnt = ignore ? &ignore->v : nullptr;

	if (hull == point_hull)
	{	
		engine::TraceLine(start, end, traceFlags, ignoreEnt, tr);
	}
	else
	{
		engine::TraceHull(start, end, traceFlags, hull, ignoreEnt, tr);
	}

	if ((flags & kTraceBoxModel) != 0 && tr->flFraction != 1.0F)
	{
		TraceResult tr2;
		if (hull == point_hull)
		{
			engine::TraceLine(tr->vecEndPos, end, traceFlags, ignoreEnt, &tr2);
		}
		else
		{
			engine::TraceHull(start, end, traceFlags, hull, ignoreEnt, &tr2);
		}
		
		if (tr2.flFraction != 1.0F && tr2.pHit == tr->pHit)
		{
			*tr = tr2;
		}
	}

	return tr->flFraction != 1.0F;
}


void util::TraceHull(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, int hullNumber, Entity* pentIgnore, TraceResult* ptr)
{
	engine::TraceHull(vecStart, vecEnd, (igmon == ignore_monsters ? 1 : 0), hullNumber, pentIgnore, ptr);
}

void util::TraceModel(const Vector& vecStart, const Vector& vecEnd, int hullNumber, Entity* pentModel, TraceResult* ptr)
{
	engine::TraceModel(vecStart, vecEnd, hullNumber, pentModel, ptr);
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
	return engine::VecToYaw(vec);
}


void util::ParticleEffect(const Vector& vecOrigin, const Vector& vecDirection, unsigned int ulColor, unsigned int ulCount)
{
	engine::ParticleEffect(vecOrigin, vecDirection, (float)ulColor, (float)ulCount);
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

Vector util::GetAimVector(Entity* pent, float flSpeed)
{
	Vector tmp;
	engine::GetAimVector(pent, flSpeed, tmp);
	return tmp;
}

bool util::IsMasterTriggered(string_t sMaster, CBaseEntity* pActivator)
{
	if (!FStringNull(sMaster))
	{
		CBaseEntity* pMaster = util::FindEntityByTargetname(nullptr, STRING(sMaster));

		if (pMaster != nullptr && (pMaster->ObjectCaps() & FCAP_MASTER) != 0)
		{
			return pMaster->IsTriggered(pActivator);
		}

		engine::AlertMessage(at_aiconsole, "Master was null or not a master!\n");
	}

	// if this isn't a master entity, just say yes.
	return true;
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


void util::StringToVector(Vector& pVector, const char* pString)
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
		engine::AlertMessage( at_error, "Bad field in entity!! %s:%s == \"%s\"\n",
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


void util::PrecacheOther(const char* szClassname)
{
	Entity* pent;

	pent = engine::CreateNamedEntity(MAKE_STRING(szClassname));
	if (pent == nullptr)
	{
		engine::AlertMessage(at_console, "NULL Ent in util::PrecacheOther\n");
		return;
	}

	CBaseEntity* pEntity = pent->Get<CBaseEntity>();
	if (pEntity)
		pEntity->Precache();
	engine::RemoveEntity(pent);
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
	engine::AlertMessage(at_logged, "%s", string);
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
