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

===== world.cpp ========================================================

  precaches and defs for entities and other data that must always be available.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "nodes.h"
#include "client.h"
#include "decals.h"
#include "skill.h"
#include "effects.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "bot/hl_bot.h"
#include "bot/hl_bot_manager.h"

CGlobalState gGlobalState;

extern void W_Precache();

//
// This must match the list in util.h
//
DLL_DECALLIST gDecals[] = {
	{"{shot1", 0},	   // DECAL_GUNSHOT1
	{"{shot2", 0},	   // DECAL_GUNSHOT2
	{"{shot3", 0},	   // DECAL_GUNSHOT3
	{"{shot4", 0},	   // DECAL_GUNSHOT4
	{"{shot5", 0},	   // DECAL_GUNSHOT5
	{"{lambda01", 0},  // DECAL_LAMBDA1
	{"{lambda02", 0},  // DECAL_LAMBDA2
	{"{lambda03", 0},  // DECAL_LAMBDA3
	{"{lambda04", 0},  // DECAL_LAMBDA4
	{"{lambda05", 0},  // DECAL_LAMBDA5
	{"{lambda06", 0},  // DECAL_LAMBDA6
	{"{scorch1", 0},   // DECAL_SCORCH1
	{"{scorch2", 0},   // DECAL_SCORCH2
	{"{blood1", 0},	   // DECAL_BLOOD1
	{"{blood2", 0},	   // DECAL_BLOOD2
	{"{blood3", 0},	   // DECAL_BLOOD3
	{"{blood4", 0},	   // DECAL_BLOOD4
	{"{blood5", 0},	   // DECAL_BLOOD5
	{"{blood6", 0},	   // DECAL_BLOOD6
	{"{yblood1", 0},   // DECAL_YBLOOD1
	{"{yblood2", 0},   // DECAL_YBLOOD2
	{"{yblood3", 0},   // DECAL_YBLOOD3
	{"{yblood4", 0},   // DECAL_YBLOOD4
	{"{yblood5", 0},   // DECAL_YBLOOD5
	{"{yblood6", 0},   // DECAL_YBLOOD6
	{"{break1", 0},	   // DECAL_GLASSBREAK1
	{"{break2", 0},	   // DECAL_GLASSBREAK2
	{"{break3", 0},	   // DECAL_GLASSBREAK3
	{"{bigshot1", 0},  // DECAL_BIGSHOT1
	{"{bigshot2", 0},  // DECAL_BIGSHOT2
	{"{bigshot3", 0},  // DECAL_BIGSHOT3
	{"{bigshot4", 0},  // DECAL_BIGSHOT4
	{"{bigshot5", 0},  // DECAL_BIGSHOT5
	{"{spit1", 0},	   // DECAL_SPIT1
	{"{spit2", 0},	   // DECAL_SPIT2
	{"{bproof1", 0},   // DECAL_BPROOF1
	{"{gargstomp", 0}, // DECAL_GARGSTOMP1,	// Gargantua stomp crack
	{"{smscorch1", 0}, // DECAL_SMALLSCORCH1,	// Small scorch mark
	{"{smscorch2", 0}, // DECAL_SMALLSCORCH2,	// Small scorch mark
	{"{smscorch3", 0}, // DECAL_SMALLSCORCH3,	// Small scorch mark
	{"{mommablob", 0}, // DECAL_MOMMABIRTH		// BM Birth spray
	{"{mommablob", 0}, // DECAL_MOMMASPLAT		// BM Mortar spray?? need decal
};

#define SF_DECAL_NOTINDEATHMATCH 2048

class CDecal : public CBaseEntity
{
public:
	bool Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT StaticDecal();
	void EXPORT TriggerDecal(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(infodecal, CDecal);

// UNDONE:  These won't get sent to joining players in multi-player
bool CDecal::Spawn()
{
	if (pev->skin < 0 || (g_pGameRules->IsDeathmatch() && FBitSet(pev->spawnflags, SF_DECAL_NOTINDEATHMATCH)))
	{
		return false;
	}

	if (FStringNull(pev->targetname))
	{
		SetThink(&CDecal::StaticDecal);
		// if there's no targetname, the decal will spray itself on as soon as the world is done spawning.
		pev->nextthink = gpGlobals->time;
	}
	else
	{
		// if there IS a targetname, the decal sprays itself on when it is triggered.
		SetThink(nullptr);
		SetUse(&CDecal::TriggerDecal);
	}

	return true;
}

void CDecal::TriggerDecal(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	// this is set up as a USE function for infodecals that have targetnames, so that the
	// decal doesn't get applied until it is fired. (usually by a scripted sequence)
	TraceResult trace;
	int entityIndex;

	util::TraceLine(pev->origin - Vector(5, 5, 5), pev->origin + Vector(5, 5, 5), util::ignore_monsters, this, &trace);

	MessageBegin(MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte(TE_BSPDECAL);
	WriteCoord(pev->origin.x);
	WriteCoord(pev->origin.y);
	WriteCoord(pev->origin.z);
	WriteShort((int)pev->skin);
	entityIndex = (short)ENTINDEX(trace.pHit);
	WriteShort(entityIndex);
	if (0 != entityIndex)
		WriteShort((int)VARS(trace.pHit)->modelindex);
	MessageEnd();

	Remove();
}


void CDecal::StaticDecal()
{
	TraceResult trace;
	int entityIndex, modelIndex;

	util::TraceLine(pev->origin - Vector(5, 5, 5), pev->origin + Vector(5, 5, 5), util::ignore_monsters, this, &trace);

	entityIndex = (short)ENTINDEX(trace.pHit);
	if (0 != entityIndex)
		modelIndex = (int)VARS(trace.pHit)->modelindex;
	else
		modelIndex = 0;

	g_engfuncs.pfnStaticDecal(pev->origin, (int)pev->skin, entityIndex, modelIndex);

	Remove();
}


bool CDecal::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "texture"))
	{
		pev->skin = DECAL_INDEX(pkvd->szValue);

		if (pev->skin < 0)
		{
			ALERT(at_console, "Can't find decal %s\n", pkvd->szValue);
		}

		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}


CGlobalState::CGlobalState()
{
	Reset();
}

void CGlobalState::Reset()
{
	m_pList = NULL;
	m_listCount = 0;
}

globalentity_t* CGlobalState::Find(string_t globalname)
{
	if (FStringNull(globalname))
		return NULL;

	globalentity_t* pTest;
	const char* pEntityName = STRING(globalname);


	pTest = m_pList;
	while (pTest)
	{
		if (FStrEq(pEntityName, pTest->name))
			break;

		pTest = pTest->pNext;
	}

	return pTest;
}


// This is available all the time now on impulse 104, remove later
//#ifndef NDEBUG
void CGlobalState::DumpGlobals()
{
	static const char* estates[] = {"Off", "On", "Dead"};
	globalentity_t* pTest;

	ALERT(at_console, "-- Globals --\n");
	pTest = m_pList;
	while (pTest)
	{
		ALERT(at_console, "%s: %s (%s)\n", pTest->name, pTest->levelName, estates[pTest->state]);
		pTest = pTest->pNext;
	}
}
//#endif


void CGlobalState::EntityAdd(string_t globalname, string_t mapName, GLOBALESTATE state)
{
	globalentity_t* pNewEntity = (globalentity_t*)calloc(sizeof(globalentity_t), 1);
	pNewEntity->pNext = m_pList;
	m_pList = pNewEntity;
	strcpy(pNewEntity->name, STRING(globalname));
	strcpy(pNewEntity->levelName, STRING(mapName));
	pNewEntity->state = state;
	m_listCount++;
}


void CGlobalState::EntitySetState(string_t globalname, GLOBALESTATE state)
{
	globalentity_t* pEnt = Find(globalname);

	if (pEnt)
		pEnt->state = state;
}


const globalentity_t* CGlobalState::EntityFromTable(string_t globalname)
{
	globalentity_t* pEnt = Find(globalname);

	return pEnt;
}


GLOBALESTATE CGlobalState::EntityGetState(string_t globalname)
{
	globalentity_t* pEnt = Find(globalname);
	if (pEnt)
		return pEnt->state;

	return GLOBAL_OFF;
}


// Global Savedata for Delay
TYPEDESCRIPTION CGlobalState::m_SaveData[] =
	{
		DEFINE_FIELD(CGlobalState, m_listCount, FIELD_INTEGER),
};

// Global Savedata for Delay
TYPEDESCRIPTION gGlobalEntitySaveData[] =
	{
		DEFINE_ARRAY(globalentity_t, name, FIELD_CHARACTER, 64),
		DEFINE_ARRAY(globalentity_t, levelName, FIELD_CHARACTER, 32),
		DEFINE_FIELD(globalentity_t, state, FIELD_INTEGER),
};


bool CGlobalState::Save(CSave& save)
{
	int i;
	globalentity_t* pEntity;

	if (!save.WriteFields("GLOBAL", this, m_SaveData, ARRAYSIZE(m_SaveData)))
		return false;

	pEntity = m_pList;
	for (i = 0; i < m_listCount && pEntity; i++)
	{
		if (!save.WriteFields("GENT", pEntity, gGlobalEntitySaveData, ARRAYSIZE(gGlobalEntitySaveData)))
			return false;

		pEntity = pEntity->pNext;
	}

	return true;
}

bool CGlobalState::Restore(CRestore& restore)
{
	int i, listCount;
	globalentity_t tmpEntity;


	ClearStates();
	if (!restore.ReadFields("GLOBAL", this, m_SaveData, ARRAYSIZE(m_SaveData)))
		return false;

	listCount = m_listCount; // Get new list count
	m_listCount = 0;		 // Clear loaded data

	for (i = 0; i < listCount; i++)
	{
		if (!restore.ReadFields("GENT", &tmpEntity, gGlobalEntitySaveData, ARRAYSIZE(gGlobalEntitySaveData)))
			return false;
		EntityAdd(MAKE_STRING(tmpEntity.name), MAKE_STRING(tmpEntity.levelName), tmpEntity.state);
	}
	return true;
}

void CGlobalState::EntityUpdate(string_t globalname, string_t mapname)
{
	globalentity_t* pEnt = Find(globalname);

	if (pEnt)
		strcpy(pEnt->levelName, STRING(mapname));
}


void CGlobalState::ClearStates()
{
	globalentity_t* pFree = m_pList;
	while (pFree)
	{
		globalentity_t* pNext = pFree->pNext;
		free(pFree);
		pFree = pNext;
	}
	Reset();
}


void SaveGlobalState(SAVERESTOREDATA* pSaveData)
{
	if (!CSaveRestoreBuffer::IsValidSaveRestoreData(pSaveData))
	{
		return;
	}

	CSave saveHelper(*pSaveData);
	gGlobalState.Save(saveHelper);
}


void RestoreGlobalState(SAVERESTOREDATA* pSaveData)
{
	if (!CSaveRestoreBuffer::IsValidSaveRestoreData(pSaveData))
	{
		return;
	}

	CRestore restoreHelper(*pSaveData);
	gGlobalState.Restore(restoreHelper);
}


void ResetGlobalState()
{
	gGlobalState.ClearStates();
	gInitHUD = true; // Init the HUD on a new game / load game
}

// moved CWorld class definition to cbase.h
//=======================
// CWorld
//
// This spawns first when each level begins.
//=======================

LINK_ENTITY_TO_CLASS(worldspawn, CWorld);

#define SF_WORLD_DARK 0x0001	  // Fade from black at startup
#define SF_WORLD_TITLE 0x0002	  // Display game title at startup
#define SF_WORLD_FORCETEAM 0x0004 // Force teams

CWorld::CWorld()
{
	if (World)
	{
		ALERT(at_error, "Do not create multiple instances of worldspawn\n");
		return;
	}

	World = this;
}

CWorld::~CWorld()
{
	if (World != this)
	{
		return;
	}

	World = nullptr;
}

bool CWorld::Spawn()
{
	g_fGameOver = false;
	Precache();
	return true;
}

void CWorld::Precache()
{
	// Flag this entity for removal if it's not the actual world entity.
	if (World != this)
	{
		Remove();
		return;
	}

	// Set up game rules
	delete g_pGameRules;

	g_pGameRules = InstallGameRules();

	delete g_pBotMan;

	g_pBotMan = new CGameBotManager();

	// init sentence group playback stuff from sentences.txt.
	// ok to call this multiple times, calls after first are ignored.
	SENTENCEG_Init();

	// player precaches
	W_Precache();
	ClientPrecache();
	BotPrecache();

	// sounds used from C physics code
	PRECACHE_SOUND("common/null.wav"); // clears sound channels

	PRECACHE_SOUND("items/itembk2.wav");
	PRECACHE_SOUND("items/gunpickup2.wav");

	PRECACHE_SOUND("common/bodydrop3.wav");
	PRECACHE_SOUND("common/bodydrop4.wav");

	PRECACHE_SOUND("weapons/ric1.wav");
	PRECACHE_SOUND("weapons/ric2.wav");
	PRECACHE_SOUND("weapons/ric3.wav");
	PRECACHE_SOUND("weapons/ric4.wav");
	PRECACHE_SOUND("weapons/ric5.wav");

	LIGHT_STYLE(0, "m");
	LIGHT_STYLE(1, "mmnmmommommnonmmonqnmmo");
	LIGHT_STYLE(2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");
	LIGHT_STYLE(3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");
	LIGHT_STYLE(4, "mamamamamama");
	LIGHT_STYLE(5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj");
	LIGHT_STYLE(6, "nmonqnmomnmomomno");
	LIGHT_STYLE(7, "mmmaaaabcdefgmmmmaaaammmaamm");
	LIGHT_STYLE(8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");
	LIGHT_STYLE(9, "aaaaaaaazzzzzzzz");
	LIGHT_STYLE(10, "mmamammmmammamamaaamammma");
	LIGHT_STYLE(11, "abcdefghijklmnopqrrqponmlkjihgfedcba");
	LIGHT_STYLE(12, "mmnnmmnnnmmnn");
	LIGHT_STYLE(63, "a");

	for (int i = 0; i < ARRAYSIZE(gDecals); i++)
	{
		gDecals[i].index = DECAL_INDEX(gDecals[i].name);
	}

	// init the WorldGraph.
	WorldGraph.InitGraph();

	if (g_pGameRules->FAllowMonsters())
	{
		// make sure the .NOD file is newer than the .BSP file.
		if (!WorldGraph.CheckNODFile((char*)STRING(gpGlobals->mapname)))
		{ // NOD file is not present, or is older than the BSP file.
			WorldGraph.AllocNodes();
		}
		else
		{ // Load the node graph for this level
			if (!WorldGraph.FLoadGraph((char*)STRING(gpGlobals->mapname)))
			{ // couldn't load, so alloc and prepare to build a graph.
				ALERT(at_console, "*Error opening .NOD file\n");
				WorldGraph.AllocNodes();
			}
			else
			{
				ALERT(at_console, "\n*Graph Loaded!\n");
			}
		}

		SetThink(&CWorld::PostSpawn);
		pev->nextthink = gpGlobals->time + 0.5f;
	}

	CVAR_SET_FLOAT("sv_zmax", (pev->speed > 0) ? pev->speed : 4096);

	CVAR_SET_FLOAT("mp_defaultteam", ((pev->spawnflags & SF_WORLD_FORCETEAM) != 0) ? 1 : 0);
}


bool CWorld::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "skyname"))
	{
		// Sent over net now.
		CVAR_SET_STRING("sv_skyname", pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "sounds"))
	{
		gpGlobals->cdAudioTrack = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "WaveHeight"))
	{
		// Sent over net now.
		pev->scale = atof(pkvd->szValue) * (1.0 / 8.0);
		CVAR_SET_FLOAT("sv_wateramp", pev->scale);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "MaxRange"))
	{
		pev->speed = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "chaptertitle"))
	{
		pev->netname = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "startdark"))
	{
		if (0 != atoi(pkvd->szValue))
		{	
			pev->spawnflags |= SF_WORLD_DARK;
		}
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "newunit"))
	{
		// Single player only.  Clear save directory if set
		if (0 != atoi(pkvd->szValue))
		{
			CVAR_SET_FLOAT("sv_newunit", 1);
		}
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gametitle"))
	{
		if (0 != atoi(pkvd->szValue))
		{
			pev->spawnflags |= SF_WORLD_TITLE;
		}
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "mapteams"))
	{
		pev->team = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "defaultteam"))
	{
		if (0 != atoi(pkvd->szValue))
		{
			pev->spawnflags |= SF_WORLD_FORCETEAM;
		}
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}


void CWorld::PostSpawn()
{
	if (0 != WorldGraph.m_fGraphPresent)
	{
		if (0 == WorldGraph.m_fGraphPointersSet)
		{
			if (!WorldGraph.FSetGraphPointers())
			{
				ALERT(at_console, "**Graph pointers were not set!\n");
				return;
			}
			else
			{
				ALERT(at_console, "**Graph Pointers Set!\n");
			}
		}
	}
	SetThink(nullptr);
}
