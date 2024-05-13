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
#include "client.h"
#include "effects.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#ifdef HALFLIFE_NODEGRAPH
#include "nodes.h"
#endif
#ifdef HALFLIFE_BOTS
#include "bot/hl_bot.h"
#include "bot/hl_bot_manager.h"
#endif

CGlobalState gGlobalState;

extern void W_Precache();

#define SF_DECAL_NOTINDEATHMATCH 2048

class CDecal : public CBaseEntity
{
public:
	CDecal(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	bool Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT StaticDecal();
	void EXPORT TriggerDecal(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(infodecal, CDecal);

// UNDONE:  These won't get sent to joining players in multi-player
bool CDecal::Spawn()
{
	if (v.skin < 0 || (g_pGameRules->IsDeathmatch() && FBitSet(v.spawnflags, SF_DECAL_NOTINDEATHMATCH)))
	{
		return false;
	}

	if (FStringNull(v.targetname))
	{
		SetThink(&CDecal::StaticDecal);
		// if there's no targetname, the decal will spray itself on as soon as the world is done spawning.
		v.nextthink = gpGlobals->time;
	}
	else
	{
		// if there IS a targetname, the decal sprays itself on when it is triggered.
		ClearThink();
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

	util::TraceLine(v.origin - Vector(5, 5, 5), v.origin + Vector(5, 5, 5), util::ignore_monsters, this, &trace);

	MessageBegin(MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte(TE_BSPDECAL);
	WriteCoord(v.origin.x);
	WriteCoord(v.origin.y);
	WriteCoord(v.origin.z);
	WriteShort((int)v.skin);
	entityIndex = (short)ENTINDEX(trace.pHit);
	WriteShort(entityIndex);
	if (0 != entityIndex)
		WriteShort((int)trace.pHit->modelindex);
	MessageEnd();

	Remove();
}


void CDecal::StaticDecal()
{
	TraceResult trace;
	int entityIndex, modelIndex;

	util::TraceLine(v.origin - Vector(5, 5, 5), v.origin + Vector(5, 5, 5), util::ignore_monsters, this, &trace);

	entityIndex = (short)ENTINDEX(trace.pHit);
	if (0 != entityIndex)
		modelIndex = (int)trace.pHit->modelindex;
	else
		modelIndex = 0;

	engine::StaticDecal(v.origin, (int)v.skin, entityIndex, modelIndex);

	Remove();
}


bool CDecal::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "texture"))
	{
		v.skin = engine::DecalIndex(pkvd->szValue);

		if (v.skin < 0)
		{
			engine::AlertMessage(at_console, "Can't find decal %s\n", pkvd->szValue);
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
	m_pList = nullptr;
	m_listCount = 0;
}

globalentity_t* CGlobalState::Find(string_t globalname)
{
	if (FStringNull(globalname))
		return nullptr;

	globalentity_t* pTest;
	const char* pEntityName = STRING(globalname);


	pTest = m_pList;
	while (pTest)
	{
		if (streq(pEntityName, pTest->name))
			break;

		pTest = pTest->pNext;
	}

	return pTest;
}


void CGlobalState::DumpGlobals()
{
	static const char* estates[] = {"Off", "On", "Dead"};
	globalentity_t* pTest;

	engine::AlertMessage(at_console, "-- Globals --\n");
	pTest = m_pList;
	while (pTest)
	{
		engine::AlertMessage(at_console, "%s: %s (%s)\n", pTest->name, pTest->levelName, estates[pTest->state]);
		pTest = pTest->pNext;
	}
}


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
#ifdef HALFLIFE_SAVERESTORE
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
#endif

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
#ifdef HALFLIFE_SAVERESTORE
	if (!CSaveRestoreBuffer::IsValidSaveRestoreData(pSaveData))
	{
		return;
	}

	CSave saveHelper(*pSaveData);
	gGlobalState.Save(saveHelper);
#endif
}


void RestoreGlobalState(SAVERESTOREDATA* pSaveData)
{
#ifdef HALFLIFE_SAVERESTORE
	if (!CSaveRestoreBuffer::IsValidSaveRestoreData(pSaveData))
	{
		return;
	}

	CRestore restoreHelper(*pSaveData);
	gGlobalState.Restore(restoreHelper);
#endif
}


void ResetGlobalState()
{
	gGlobalState.ClearStates();
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

CWorld::CWorld(Entity* containingEntity) : CBaseEntity(containingEntity)
{
	if (World)
	{
		engine::AlertMessage(at_error, "Do not create multiple instances of worldspawn\n");
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

#ifdef HALFLIFE_BOTS
	delete g_pBotMan;

	g_pBotMan = new CGameBotManager();
#endif

	// init sentence group playback stuff from sentences.txt.
	// ok to call this multiple times, calls after first are ignored.
	SENTENCEG_Init();

	// player precaches
	W_Precache();
	ClientPrecache();

#ifdef HALFLIFE_BOTS
	BotPrecache();
#endif

	engine::LightStyle(0, "m");
	engine::LightStyle(1, "mmnmmommommnonmmonqnmmo");
	engine::LightStyle(2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");
	engine::LightStyle(3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");
	engine::LightStyle(4, "mamamamamama");
	engine::LightStyle(5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj");
	engine::LightStyle(6, "nmonqnmomnmomomno");
	engine::LightStyle(7, "mmmaaaabcdefgmmmmaaaammmaamm");
	engine::LightStyle(8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");
	engine::LightStyle(9, "aaaaaaaazzzzzzzz");
	engine::LightStyle(10, "mmamammmmammamamaaamammma");
	engine::LightStyle(11, "abcdefghijklmnopqrrqponmlkjihgfedcba");
	engine::LightStyle(12, "mmnnmmnnnmmnn");
	engine::LightStyle(63, "a");

#ifdef HALFLIFE_NODEGRAPH
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
				engine::AlertMessage(at_console, "*Error opening .NOD file\n");
				WorldGraph.AllocNodes();
			}
			else
			{
				engine::AlertMessage(at_console, "\n*Graph Loaded!\n");
			}
		}

		SetThink(&CWorld::PostSpawn);
		v.nextthink = gpGlobals->time + 0.5f;
	}
#endif

	engine::CVarSetFloat("sv_zmax", (v.speed > 0) ? v.speed : 4096);

	engine::CVarSetFloat("mp_defaultteam", ((v.spawnflags & SF_WORLD_FORCETEAM) != 0) ? 1 : 0);
}


bool CWorld::KeyValue(KeyValueData* pkvd)
{
	if (streq(pkvd->szKeyName, "skyname"))
	{
		// Sent over net now.
		engine::CVarSetString("sv_skyname", pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "sounds"))
	{
		gpGlobals->cdAudioTrack = atoi(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "WaveHeight"))
	{
		// Sent over net now.
		v.scale = atof(pkvd->szValue) * (1.0 / 8.0);
		engine::CVarSetFloat("sv_wateramp", v.scale);
		return true;
	}
	else if (streq(pkvd->szKeyName, "MaxRange"))
	{
		v.speed = atof(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "chaptertitle"))
	{
		v.netname = engine::AllocString(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "startdark"))
	{
		if (0 != atoi(pkvd->szValue))
		{	
			v.spawnflags |= SF_WORLD_DARK;
		}
		return true;
	}
#ifdef HALFLIFE_SAVERESTORE
	else if (streq(pkvd->szKeyName, "newunit"))
	{
		// Single player only.  Clear save directory if set
		if (0 != atoi(pkvd->szValue))
		{
			engine::CVarSetFloat("sv_newunit", 1);
		}
		return true;
	}
#endif
	else if (streq(pkvd->szKeyName, "gametitle"))
	{
		if (0 != atoi(pkvd->szValue))
		{
			v.spawnflags |= SF_WORLD_TITLE;
		}
		return true;
	}
	else if (streq(pkvd->szKeyName, "mapteams"))
	{
		v.team = engine::AllocString(pkvd->szValue);
		return true;
	}
	else if (streq(pkvd->szKeyName, "defaultteam"))
	{
		if (0 != atoi(pkvd->szValue))
		{
			v.spawnflags |= SF_WORLD_FORCETEAM;
		}
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}


#ifdef HALFLIFE_NODEGRAPH
void CWorld::PostSpawn()
{
	if (0 != WorldGraph.m_fGraphPresent)
	{
		if (0 == WorldGraph.m_fGraphPointersSet)
		{
			if (!WorldGraph.FSetGraphPointers())
			{
				engine::AlertMessage(at_console, "**Graph pointers were not set!\n");
				return;
			}
			else
			{
				engine::AlertMessage(at_console, "**Graph Pointers Set!\n");
			}
		}
	}
	ClearThink();
}
#endif
