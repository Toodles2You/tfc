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

// Com_Weapons.cpp
// Shared weapons common/shared functions
#include <stdarg.h>
#include "hud.h"
#include "cl_util.h"
#include "com_weapons.h"

#include "const.h"
#include "entity_state.h"
#include "r_efx.h"

// During our weapon prediction processing, we'll need to reference some data that is part of
//  the final state passed into the postthink functionality.  We'll set this pointer and then
//  reset it to nullptr as appropriate
struct local_state_s g_finalstate;

int g_CurrentWeaponId = WEAPON_NONE;
int g_LastWeaponId = WEAPON_NONE;

/*
====================
COM_Log

Log debug messages to file ( appends )
====================
*/
void COM_Log(const char* pszFile, const char* fmt, ...)
{
	va_list argptr;
	char string[1024];
	FILE* fp;
	const char* pfilename;

	if (!pszFile)
	{
		pfilename = "c:\\hllog.txt";
	}
	else
	{
		pfilename = pszFile;
	}

	va_start(argptr, fmt);
	vsprintf(string, fmt, argptr);
	va_end(argptr);

	fp = fopen(pfilename, "a+t");
	if (fp)
	{
		fprintf(fp, "%s", string);
		fclose(fp);
	}
}

/*
=====================
HUD_PlaySound

Play a sound, if we are seeing this command for the first time
=====================
*/
void HUD_PlaySound(const char* sound, float volume)
{
	if (!HUD_FirstTimePredicting())
	{
		return;
	}

	client::PlaySoundByNameAtLocation(sound, volume, g_finalstate.playerstate.origin);
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

gamemode_e util::GetGameMode()
{
	return gHUD.m_gameMode;
}

bool util::IsMultiplayer()
{
	return util::GetGameMode() > kGamemodeSingleplayer;
}

bool util::IsDeathmatch()
{
	return util::GetGameMode() > kGamemodeCooperative;
}
