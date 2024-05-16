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

#include "Platform.h"

#include <assert.h>
#include "mathlib.h"
#include "cdll_dll.h"
#include "const.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_shared.h"
#include "pm_materials.h"
#include "pm_movevars.h"
#include "pm_debug.h"
#include <stdio.h>	// nullptr
#include <string.h> // strcpy
#include <stdlib.h> // atoi
#include <ctype.h>	// isspace

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"


#ifdef CLIENT_DLL
// Spectator Mode
bool iJumpSpectator;
Vector vJumpOrigin;
Vector vJumpAngles;
#endif

#pragma warning(disable : 4305)

typedef enum
{
	mod_brush,
	mod_sprite,
	mod_alias,
	mod_studio
} modtype_t;

playermove_t* pmove = nullptr;

typedef struct
{
	int planenum;
	short children[2]; // negative numbers are contents
} dclipnode_t;

typedef struct mplane_s
{
	Vector normal; // surface normal
	float dist;	   // closest appoach to origin
	byte type;	   // for texture axis selection and fast side tests
	byte signbits; // signx + signy<<1 + signz<<1
	byte pad[2];
} mplane_t;

typedef struct hull_s
{
	dclipnode_t* clipnodes;
	mplane_t* planes;
	int firstclipnode;
	int lastclipnode;
	Vector clip_mins;
	Vector clip_maxs;
} hull_t;

#define STEP_CONCRETE 0 // default step sound
#define STEP_METAL 1	// metal floor
#define STEP_DIRT 2		// dirt, sand, rock
#define STEP_VENT 3		// ventillation duct
#define STEP_GRATE 4	// metal grating
#define STEP_TILE 5		// floor tiles
#define STEP_SLOSH 6	// shallow liquid puddle
#define STEP_WADE 7		// wading in liquid
#define STEP_LADDER 8	// climbing ladder

// double to float warning
#pragma warning(disable : 4244)

static Vector rgv3tStuckTable[54];
static int rgStuckLast[MAX_PLAYERS];

// Texture names
static int gcTextures = 0;
static char grgszTextureName[CTEXTURESMAX][CBTEXTURENAMEMAX];
static char grgchTextureType[CTEXTURESMAX];


static void PM_SwapTextures(int i, int j)
{
	char chTemp;
	char szTemp[CBTEXTURENAMEMAX];

	strcpy(szTemp, grgszTextureName[i]);
	chTemp = grgchTextureType[i];

	strcpy(grgszTextureName[i], grgszTextureName[j]);
	grgchTextureType[i] = grgchTextureType[j];

	strcpy(grgszTextureName[j], szTemp);
	grgchTextureType[j] = chTemp;
}


static void PM_SortTextures()
{
	// Bubble sort, yuck, but this only occurs at startup and it's only 512 elements...
	//
	int i, j;

	for (i = 0; i < gcTextures; i++)
	{
		for (j = i + 1; j < gcTextures; j++)
		{
			if (stricmp(grgszTextureName[i], grgszTextureName[j]) > 0)
			{
				// Swap
				//
				PM_SwapTextures(i, j);
			}
		}
	}
}


static void PM_InitTextureTypes()
{
	char buffer[512];
	int i, j;
	byte* pMemFile;
	int fileSize, filePos;
	static bool bTextureTypeInit = false;

	if (bTextureTypeInit)
		return;

	memset(&(grgszTextureName[0][0]), 0, CTEXTURESMAX * CBTEXTURENAMEMAX);
	memset(grgchTextureType, 0, CTEXTURESMAX);

	gcTextures = 0;
	memset(buffer, 0, 512);

	fileSize = pmove->COM_FileSize("sound/materials.txt");
	pMemFile = pmove->COM_LoadFile("sound/materials.txt", 5, nullptr);
	if (!pMemFile)
		return;

	filePos = 0;
	// for each line in the file...
	while (pmove->memfgets(pMemFile, fileSize, &filePos, buffer, 511) != nullptr && (gcTextures < CTEXTURESMAX))
	{
		// skip whitespace
		i = 0;
		while ('\0' != buffer[i] && 0 != isspace(buffer[i]))
			i++;

		if ('\0' == buffer[i])
			continue;

		// skip comment lines
		if (buffer[i] == '/' || 0 == isalpha(buffer[i]))
			continue;

		// get texture type
		grgchTextureType[gcTextures] = toupper(buffer[i++]);

		// skip whitespace
		while ('\0' != buffer[i] && 0 != isspace(buffer[i]))
			i++;

		if ('\0' == buffer[i])
			continue;

		// get sentence name
		j = i;
		while ('\0' != buffer[j] && 0 == isspace(buffer[j]))
			j++;

		if ('\0' == buffer[j])
			continue;

		// null-terminate name and save in sentences array
		j = std::min(j, CBTEXTURENAMEMAX - 1 + i);
		buffer[j] = 0;
		strcpy(&(grgszTextureName[gcTextures++][0]), &(buffer[i]));
	}

	// Must use engine to free since we are in a .dll
	pmove->COM_FreeFile(pMemFile);

	PM_SortTextures();

	bTextureTypeInit = true;
}


char PM_FindTextureType(const char* name)
{
	int left, right, pivot;
	int val;

	left = 0;
	right = gcTextures - 1;

	while (left <= right)
	{
		pivot = (left + right) / 2;

		val = strnicmp(name, grgszTextureName[pivot], CBTEXTURENAMEMAX - 1);
		if (val == 0)
		{
			return grgchTextureType[pivot];
		}
		else if (val > 0)
		{
			left = pivot + 1;
		}
		else if (val < 0)
		{
			right = pivot - 1;
		}
	}

	return CHAR_TEX_CONCRETE;
}


/*
=================
PM_GetRandomStuckOffsets

When a player is stuck, it's costly to try and unstick them
Grab a test offset for the player based on a passed in index
=================
*/
int PM_GetRandomStuckOffsets(int nIndex, Vector& offset)
{
	// Last time we did a full
	int idx;
	idx = rgStuckLast[nIndex]++;

	offset = rgv3tStuckTable[idx % 54];

	return (idx % 54);
}


void PM_ResetStuckOffsets(int nIndex)
{
	rgStuckLast[nIndex] = 0;
}


/*
=================
PM_TryToUnstuck

If pmove->origin is in a solid position,
try nudging slightly on all axis to
allow for the cut precision of the net coordinates
=================
*/
bool PM_TryToUnstuck(Vector base, int (*pfnIgnore)(physent_t *pe))
{
	float x, y, z;
	float xystep = 8.0;
	float zstep = 18.0;
	float xyminmax = xystep;
	float zminmax = 4 * zstep;
	Vector test;

	for (z = 0; z <= zminmax; z += zstep)
	{
		for (x = -xyminmax; x <= xyminmax; x += xystep)
		{
			for (y = -xyminmax; y <= xyminmax; y += xystep)
			{
				test = base;
				test[0] += x;
				test[1] += y;
				test[2] += z;

				if (pmove->PM_TestPlayerPositionEx(test, nullptr, pfnIgnore) == -1)
				{
					pmove->origin = test;
					return false;
				}
			}
		}
	}

	return true;
}


static void PM_CreateStuckTable()
{
	float x, y, z;
	int idx;
	int i;
	Vector zi;

	memset(rgv3tStuckTable, 0, 54 * sizeof(Vector));

	idx = 0;
	// Little Moves.
	x = y = 0;
	// Z moves
	for (z = -0.125; z <= 0.125; z += 0.125)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}
	x = z = 0;
	// Y moves
	for (y = -0.125; y <= 0.125; y += 0.125)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}
	y = z = 0;
	// X moves
	for (x = -0.125; x <= 0.125; x += 0.125)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}

	// Remaining multi axis nudges.
	for (x = -0.125; x <= 0.125; x += 0.250)
	{
		for (y = -0.125; y <= 0.125; y += 0.250)
		{
			for (z = -0.125; z <= 0.125; z += 0.250)
			{
				rgv3tStuckTable[idx][0] = x;
				rgv3tStuckTable[idx][1] = y;
				rgv3tStuckTable[idx][2] = z;
				idx++;
			}
		}
	}

	// Big Moves.
	x = y = 0;
	zi[0] = 0.0f;
	zi[1] = 1.0f;
	zi[2] = 6.0f;

	for (i = 0; i < 3; i++)
	{
		// Z moves
		z = zi[i];
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}

	x = z = 0;

	// Y moves
	for (y = -2.0f; y <= 2.0f; y += 2.0)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}
	y = z = 0;
	// X moves
	for (x = -2.0f; x <= 2.0f; x += 2.0f)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}

	// Remaining multi axis nudges.
	for (i = 0; i < 3; i++)
	{
		z = zi[i];

		for (x = -2.0f; x <= 2.0f; x += 2.0f)
		{
			for (y = -2.0f; y <= 2.0f; y += 2.0)
			{
				rgv3tStuckTable[idx][0] = x;
				rgv3tStuckTable[idx][1] = y;
				rgv3tStuckTable[idx][2] = z;
				idx++;
			}
		}
	}
}


int PM_GetVisEntInfo(int ent)
{
	if (ent >= 0 && ent <= pmove->numvisent)
	{
		return pmove->visents[ent].info;
	}
	return -1;
}


int PM_GetPhysEntInfo(int ent)
{
	if (ent >= 0 && ent <= pmove->numphysent)
	{
		return pmove->physents[ent].info;
	}
	return -1;
}


void PM_Init(struct playermove_s* ppmove)
{
	pmove = ppmove;

	PM_CreateStuckTable();
	PM_InitTextureTypes();

	//The engine copies the hull sizes initialized by PM_GetHullBounds *before* PM_GetHullBounds is actually called, so manually initialize these.
	for (int i = 0; i < kHullCount; ++i)
	{
		if (!PM_GetHullBounds(i, pmove->player_mins[i], pmove->player_maxs[i]))
		{
			//Matches the engine's behavior in ignoring the remaining hull sizes if any hull isn't provided.
			break;
		}
	}
}


bool PM_GetHullBounds(int hullnumber, float* mins, float* maxs)
{
	switch (hullnumber)
	{
	case kHullPlayer:
		VEC_HULL_MIN.CopyToArray(mins);
		VEC_HULL_MAX.CopyToArray(maxs);
		return true;
	case kHullDuck:
		VEC_DUCK_HULL_MIN.CopyToArray(mins);
		VEC_DUCK_HULL_MAX.CopyToArray(maxs);
		return true;
	case kHullPoint:
		g_vecZero.CopyToArray(mins);
		g_vecZero.CopyToArray(maxs);
		return true;
	}

	return false;
}

