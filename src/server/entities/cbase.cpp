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
#include "saverestore.h"
#include "client.h"
#include "gamerules.h"
#include "game.h"
#include "pm_shared.h"

void OnFreeEntPrivateData(Entity* pEdict);
int ShouldCollide(Entity* pentTouched, Entity* pentOther);

static DLL_FUNCTIONS gFunctionTable =
	{
		GameDLLInit,			   //pfnGameInit
		DispatchSpawn,			   //pfnSpawn
		DispatchThink,			   //pfnThink
		DispatchUse,			   //pfnUse
		DispatchTouch,			   //pfnTouch
		DispatchBlocked,		   //pfnBlocked
		DispatchKeyValue,		   //pfnKeyValue
		DispatchSave,			   //pfnSave
		DispatchRestore,		   //pfnRestore
		DispatchObjectCollsionBox, //pfnAbsBox

		SaveWriteFields, //pfnSaveWriteFields
		SaveReadFields,	 //pfnSaveReadFields

		SaveGlobalState,	//pfnSaveGlobalState
		RestoreGlobalState, //pfnRestoreGlobalState
		ResetGlobalState,	//pfnResetGlobalState

		ClientConnect,		   //pfnClientConnect
		ClientDisconnect,	   //pfnClientDisconnect
		ClientKill,			   //pfnClientKill
		ClientPutInServer,	   //pfnClientPutInServer
		ClientCommand,		   //pfnClientCommand
		ClientUserInfoChanged, //pfnClientUserInfoChanged
		ServerActivate,		   //pfnServerActivate
		ServerDeactivate,	   //pfnServerDeactivate

		PlayerPreThink,	 //pfnPlayerPreThink
		PlayerPostThink, //pfnPlayerPostThink

		StartFrame,		  //pfnStartFrame
		ParmsNewLevel,	  //pfnParmsNewLevel
		ParmsChangeLevel, //pfnParmsChangeLevel

		GetGameDescription,	 //pfnGetGameDescription    Returns string describing current .dll game.
		PlayerCustomization, //pfnPlayerCustomization   Notifies .dll of new customization for player.

		SpectatorConnect,	 //pfnSpectatorConnect      Called when spectator joins server
		SpectatorDisconnect, //pfnSpectatorDisconnect   Called when spectator leaves the server
		SpectatorThink,		 //pfnSpectatorThink        Called when spectator sends a command packet (usercmd_t)

		Sys_Error, //pfnSys_Error				Called when engine has encountered an error

		PM_Move,			//pfnPM_Move
		PM_Init,			//pfnPM_Init				Server version of player movement initialization
		PM_FindTextureType, //pfnPM_FindTextureType

		SetupVisibility,		  //pfnSetupVisibility        Set up PVS and PAS for networking for this client
		UpdateClientData,		  //pfnUpdateClientData       Set up data sent only to specific client
		AddToFullPack,			  //pfnAddToFullPack
		CreateBaseline,			  //pfnCreateBaseline			Tweak entity baseline for network encoding, allows setup of player baselines, too.
		RegisterEncoders,		  //pfnRegisterEncoders		Callbacks for network encoding
		GetWeaponData,			  //pfnGetWeaponData
		CmdStart,				  //pfnCmdStart
		CmdEnd,					  //pfnCmdEnd
		ConnectionlessPacket,	  //pfnConnectionlessPacket
		GetHullBounds,			  //pfnGetHullBounds
		CreateInstancedBaselines, //pfnCreateInstancedBaselines
		InconsistentFile,		  //pfnInconsistentFile
		AllowLagCompensation,	  //pfnAllowLagCompensation
};

NEW_DLL_FUNCTIONS gNewDLLFunctions =
	{
		OnFreeEntPrivateData, //pfnOnFreeEntPrivateData
		GameDLLShutdown,
		ShouldCollide,
};

static void SetObjectCollisionBox(Entity* entity);

extern "C" {

int GetEntityAPI(DLL_FUNCTIONS* pFunctionTable, int interfaceVersion)
{
	if (!pFunctionTable || interfaceVersion != INTERFACE_VERSION)
	{
		return 0;
	}

	memcpy(pFunctionTable, &gFunctionTable, sizeof(DLL_FUNCTIONS));
	return 1;
}

int GetEntityAPI2(DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion)
{
	if (!pFunctionTable || *interfaceVersion != INTERFACE_VERSION)
	{
		// Tell engine what version we had, so it can figure out who is out of date.
		*interfaceVersion = INTERFACE_VERSION;
		return 0;
	}

	memcpy(pFunctionTable, &gFunctionTable, sizeof(DLL_FUNCTIONS));
	return 1;
}

int GetNewDLLFunctions(NEW_DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion)
{
	if (!pFunctionTable || *interfaceVersion != NEW_DLL_FUNCTIONS_VERSION)
	{
		*interfaceVersion = NEW_DLL_FUNCTIONS_VERSION;
		return 0;
	}

	memcpy(pFunctionTable, &gNewDLLFunctions, sizeof(gNewDLLFunctions));
	return 1;
}
}


int DispatchSpawn(Entity* pent)
{
	CBaseEntity* pEntity = pent->Get<CBaseEntity>();

	if (pEntity)
	{
		// Initialize these or entities who don't link to the world won't have anything in here
		pEntity->v.absmin = pEntity->v.origin - Vector(1, 1, 1);
		pEntity->v.absmax = pEntity->v.origin + Vector(1, 1, 1);

		if (!pEntity->Spawn())
		{
			return -1;
		}

		// Try to get the pointer again, in case the spawn function deleted the entity.
		pEntity = pent->Get<CBaseEntity>();

		if (pEntity)
		{
			if (g_pGameRules && !g_pGameRules->IsAllowedToSpawn(pEntity))
				return -1; // return that this entity should be deleted
			if ((pEntity->v.flags & FL_KILLME) != 0)
				return -1;
		}


		// Handle global stuff here
		if (pEntity && !FStringNull(pEntity->v.globalname))
		{
			const globalentity_t* pGlobal = gGlobalState.EntityFromTable(pEntity->v.globalname);
			if (pGlobal)
			{
				// Already dead? delete
				if (pGlobal->state == GLOBAL_DEAD)
					return -1;
#ifdef HALFLIFE_SAVERESTORE
				else if (!FStrEq(STRING(gpGlobals->mapname), pGlobal->levelName))
					pEntity->MakeDormant(); // Hasn't been moved to this level yet, wait but stay alive
											// In this level & not dead, continue on as normal
#endif
			}
			else
			{
				// Spawned entities default to 'On'
				gGlobalState.EntityAdd(pEntity->v.globalname, gpGlobals->mapname, GLOBAL_ON);
			}
		}
	}

	return 0;
}


TYPEDESCRIPTION CBaseEntity::m_EntvarsDescription[kEntvarsCount] =
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


static void EntvarsKeyvalue(Entity* entity, KeyValueData* pkvd)
{
	int i;
	TYPEDESCRIPTION* pField;

	for (i = 0; i < ARRAYSIZE(CBaseEntity::m_EntvarsDescription); i++)
	{
		pField = &CBaseEntity::m_EntvarsDescription[i];

		if (stricmp(pField->fieldName, pkvd->szKeyName) != 0)
		{
			continue;
		}

		switch (pField->fieldType)
		{
		case FIELD_MODELNAME:
		case FIELD_SOUNDNAME:
		case FIELD_STRING:
			(*(int*)((char*)entity + pField->fieldOffset)) = ALLOC_STRING(pkvd->szValue);
			break;

		case FIELD_TIME:
		case FIELD_FLOAT:
			(*(float*)((char*)entity + pField->fieldOffset)) = atof(pkvd->szValue);
			break;

		case FIELD_INTEGER:
			(*(int*)((char*)entity + pField->fieldOffset)) = atoi(pkvd->szValue);
			break;

		case FIELD_POSITION_VECTOR:
		case FIELD_VECTOR:
			util::StringToVector((float*)((char*)entity + pField->fieldOffset), pkvd->szValue);
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


/*
The only time EntvarsKeyvalue should get called prior to entity creation is for the classname.
*/
void DispatchKeyValue(Entity* pentKeyvalue, KeyValueData* pkvd)
{
	if (pkvd == nullptr || pentKeyvalue == nullptr)
	{
		return;
	}

	CBaseEntity* pEntity = pentKeyvalue->Get<CBaseEntity>();

	if (pkvd->szClassName == nullptr || pEntity == nullptr)
	{
		EntvarsKeyvalue(pentKeyvalue, pkvd);
		return;
	}

	if (pEntity->EntvarsKeyvalue(pkvd))
	{
		return;
	}

	pkvd->fHandled = static_cast<int32>(pEntity->KeyValue(pkvd));
}

void DispatchTouch(Entity* pentTouched, Entity* pentOther)
{
#ifdef HALFLIFE_NODEGRAPH
	if (gTouchDisabled)
		return;
#endif

	CBaseEntity* pEntity = pentTouched->Get<CBaseEntity>();
	CBaseEntity* pOther = pentOther->Get<CBaseEntity>();

	if (pEntity && pOther && ((pEntity->v.flags | pOther->v.flags) & FL_KILLME) == 0)
	{
		/*
			Toodles: Trigger entities (such as items) don't call "ShouldCollide" since 
			they technically don't "collide" with entities, so it must be checked here.
		*/
		if (pEntity->v.solid != SOLID_TRIGGER || pEntity->ShouldCollide(pOther))
		{
			pEntity->Touch(pOther);
		}
	}
}


void DispatchUse(Entity* pentUsed, Entity* pentOther)
{
	CBaseEntity* pEntity = pentUsed->Get<CBaseEntity>();
	CBaseEntity* pOther = pentOther->Get<CBaseEntity>();

	if (pEntity && (pEntity->v.flags & FL_KILLME) == 0)
		pEntity->Use(pOther, pOther, USE_TOGGLE, 0);
}

void DispatchThink(Entity* pent)
{
	CBaseEntity* pEntity = pent->Get<CBaseEntity>();
	if (pEntity)
	{
#ifdef HALFLIFE_SAVERESTORE
		if (FBitSet(pEntity->v.flags, FL_DORMANT))
			ALERT(at_error, "Dormant entity %s is thinking!!\n", STRING(pEntity->v.classname));
#endif

		pEntity->Think();
	}
}

void DispatchBlocked(Entity* pentBlocked, Entity* pentOther)
{
	CBaseEntity* pEntity = pentBlocked->Get<CBaseEntity>();
	CBaseEntity* pOther = pentOther->Get<CBaseEntity>();

	if (pEntity)
		pEntity->Blocked(pOther);
}

void DispatchSave(Entity* pent, SAVERESTOREDATA* pSaveData)
{
	gpGlobals->time = pSaveData->time;

#ifdef HALFLIFE_SAVERESTORE
	CBaseEntity* pEntity = pent->Get<CBaseEntity>();

	if (pEntity && CSaveRestoreBuffer::IsValidSaveRestoreData(pSaveData))
	{
		ENTITYTABLE* pTable = &pSaveData->pTable[pSaveData->currentIndex];

		if (pTable->pent != pent)
			ALERT(at_error, "ENTITY TABLE OR INDEX IS WRONG!!!!\n");

		if ((pEntity->ObjectCaps() & FCAP_DONT_SAVE) != 0)
			return;

		// These don't use ltime & nextthink as times really, but we'll fudge around it.
		if (pEntity->v.movetype == MOVETYPE_PUSH)
		{
			float delta = pEntity->v.nextthink - pEntity->v.ltime;
			pEntity->v.ltime = gpGlobals->time;
			pEntity->v.nextthink = pEntity->v.ltime + delta;
		}

		pTable->location = pSaveData->size;			 // Remember entity position for file I/O
		pTable->classname = pEntity->v.classname; // Remember entity class for respawn

		CSave saveHelper(*pSaveData);
		pEntity->Save(saveHelper);

		pTable->size = pSaveData->size - pTable->location; // Size of entity block is data size written to block
	}
#endif
}

void OnFreeEntPrivateData(Entity* pEdict)
{
	if (pEdict != nullptr)
	{
		/* Since the edict doesn't get deleted, fix it so it doesn't interfere. */
		pEdict->model = iStringNull;
		pEdict->modelindex = 0;
		pEdict->effects = EF_NOINTERP | EF_NODRAW;
		pEdict->solid = SOLID_NOT;
		pEdict->movetype = MOVETYPE_NONE;
		pEdict->takedamage = DAMAGE_NO;
		pEdict->origin = g_vecZero;

		// Zero this out so the engine doesn't try to free it again.
		pEdict->Free<CBaseEntity>();
	}
}

int ShouldCollide(Entity* pentTouched, Entity* pentOther)
{
	auto touched = pentTouched->Get<CBaseEntity>();
	auto other = pentOther->Get<CBaseEntity>();

	if (touched != nullptr && other != nullptr && ((touched->v.flags | other->v.flags) & FL_KILLME) == 0)
	{
		/*
			Toodles: Important to note that this will not affect player physics.
			Define CHalfLifePlayerMovement::ShouldCollide in order to prevent 
			players from colliding with objects.
		*/
		return touched->ShouldCollide(other) && other->ShouldCollide(touched);
	}

	return 1;
}

// Find the matching global entity.  Spit out an error if the designer made entities of
// different classes with the same global name
CBaseEntity* FindGlobalEntity(string_t classname, string_t globalname)
{
	CBaseEntity* pReturn = util::FindEntityByString(nullptr, "globalname", STRING(globalname));
	if (pReturn)
	{
		if (!FClassnameIs(&pReturn->v, STRING(classname)))
		{
			ALERT(at_console, "Global entity found %s, wrong class %s\n", STRING(globalname), STRING(pReturn->v.classname));
			pReturn = NULL;
		}
	}

	return pReturn;
}


int DispatchRestore(Entity* pent, SAVERESTOREDATA* pSaveData, int globalEntity)
{
	gpGlobals->time = pSaveData->time;

#ifndef HALFLIFE_SAVERESTORE
	return -1;
#else
	CBaseEntity* pEntity = pent->Get<CBaseEntity>();

	if (pEntity && CSaveRestoreBuffer::IsValidSaveRestoreData(pSaveData))
	{
		Entity tmpVars;
		Vector oldOffset;

		CRestore restoreHelper(*pSaveData);
		if (0 != globalEntity)
		{
			CRestore tmpRestore(*pSaveData);
			tmpRestore.PrecacheMode(false);
			tmpRestore.ReadEntVars("ENTVARS", &tmpVars);

			// HACKHACK - reset the save pointers, we're going to restore for real this time
			pSaveData->size = pSaveData->pTable[pSaveData->currentIndex].location;
			pSaveData->pCurrentData = pSaveData->pBaseData + pSaveData->size;
			// -------------------


			const globalentity_t* pGlobal = gGlobalState.EntityFromTable(tmpVars.globalname);

			// Don't overlay any instance of the global that isn't the latest
			// pSaveData->szCurrentMapName is the level this entity is coming from
			// pGlobla->levelName is the last level the global entity was active in.
			// If they aren't the same, then this global update is out of date.
			if (!FStrEq(pSaveData->szCurrentMapName, pGlobal->levelName))
				return 0;

			// Compute the new global offset
			oldOffset = pSaveData->vecLandmarkOffset;
			CBaseEntity* pNewEntity = FindGlobalEntity(tmpVars.classname, tmpVars.globalname);
			if (pNewEntity)
			{
				//				ALERT( at_console, "Overlay %s with %s\n", STRING(pNewEntity->v.classname), STRING(tmpVars.classname) );
				// Tell the restore code we're overlaying a global entity from another level
				restoreHelper.SetGlobalMode(true); // Don't overwrite global fields
				pSaveData->vecLandmarkOffset = (pSaveData->vecLandmarkOffset - pNewEntity->v.mins) + tmpVars.mins;
				pEntity = pNewEntity; // we're going to restore this data OVER the old entity
				pent = &pEntity->v;
				// Update the global table to say that the global definition of this entity should come from this level
				gGlobalState.EntityUpdate(pEntity->v.globalname, gpGlobals->mapname);
			}
			else
			{
				// This entity will be freed automatically by the engine.  If we don't do a restore on a matching entity (below)
				// or call EntityUpdate() to move it to this level, we haven't changed global state at all.
				return 0;
			}
		}

		if ((pEntity->ObjectCaps() & FCAP_MUST_SPAWN) != 0)
		{
			pEntity->Restore(restoreHelper);
			pEntity->Spawn();
		}
		else
		{
			pEntity->Restore(restoreHelper);
			pEntity->Precache();
		}

		// Again, could be deleted, get the pointer again.
		pEntity = pent->Get<CBaseEntity>();

#if 0
		if ( pEntity && !FStringNull(pEntity->v.globalname) && 0 != globalEntity ) 
		{
			ALERT( at_console, "Global %s is %s\n", STRING(pEntity->v.globalname), STRING(pEntity->v.model) );
		}
#endif

		// Is this an overriding global entity (coming over the transition), or one restoring in a level
		if (0 != globalEntity)
		{
			//			ALERT( at_console, "After: %f %f %f %s\n", pEntity->v.origin.x, pEntity->v.origin.y, pEntity->v.origin.z, STRING(pEntity->v.model) );
			pSaveData->vecLandmarkOffset = oldOffset;
			if (pEntity)
			{
				pEntity->SetOrigin(pEntity->v.origin);
				pEntity->OverrideReset();
			}
		}
		else if (pEntity && !FStringNull(pEntity->v.globalname))
		{
			const globalentity_t* pGlobal = gGlobalState.EntityFromTable(pEntity->v.globalname);
			if (pGlobal)
			{
				// Already dead? delete
				if (pGlobal->state == GLOBAL_DEAD)
					return -1;
				else if (!FStrEq(STRING(gpGlobals->mapname), pGlobal->levelName))
				{
					pEntity->MakeDormant(); // Hasn't been moved to this level yet, wait but stay alive
				}
				// In this level & not dead, continue on as normal
			}
			else
			{
				ALERT(at_error, "Global Entity %s (%s) not in table!!!\n", STRING(pEntity->v.globalname), STRING(pEntity->v.classname));
				// Spawned entities default to 'On'
				gGlobalState.EntityAdd(pEntity->v.globalname, gpGlobals->mapname, GLOBAL_ON);
			}
		}
	}
	return 0;
#endif
}


void DispatchObjectCollsionBox(Entity* pent)
{
	CBaseEntity* pEntity = pent->Get<CBaseEntity>();
	if (pEntity)
	{
		pEntity->SetObjectCollisionBox();
	}
	else
		SetObjectCollisionBox(pent);
}


void SaveWriteFields(SAVERESTOREDATA* pSaveData, const char* pname, void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount)
{
#ifdef HALFLIFE_SAVERESTORE
	if (!CSaveRestoreBuffer::IsValidSaveRestoreData(pSaveData))
	{
		return;
	}

	CSave saveHelper(*pSaveData);
	saveHelper.WriteFields(pname, pBaseData, pFields, fieldCount);
#endif
}


void SaveReadFields(SAVERESTOREDATA* pSaveData, const char* pname, void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount)
{
#ifdef HALFLIFE_SAVERESTORE
	if (!CSaveRestoreBuffer::IsValidSaveRestoreData(pSaveData))
	{
		return;
	}

	// Always check if the player is stuck when loading a save game.
	g_CheckForPlayerStuck = true;

	CRestore restoreHelper(*pSaveData);
	restoreHelper.ReadFields(pname, pBaseData, pFields, fieldCount);
#endif
}


Entity* EHANDLE::Get()
{
	if (m_pent)
	{
		if (m_pent->GetSerialNumber() == m_serialnumber)
			return m_pent;
		else
			return NULL;
	}
	return NULL;
};

Entity* EHANDLE::Set(Entity* pent)
{
	m_pent = pent;
	if (pent)
		m_serialnumber = m_pent->GetSerialNumber();
	return pent;
};


EHANDLE::operator CBaseEntity*()
{
	if (Get() == nullptr)
	{
		return nullptr;
	}
	return Get()->Get<CBaseEntity>();
};


CBaseEntity* EHANDLE::operator=(CBaseEntity* pEntity)
{
	if (pEntity)
	{
		m_pent = &pEntity->v;
		if (m_pent)
			m_serialnumber = m_pent->GetSerialNumber();
	}
	else
	{
		m_pent = NULL;
		m_serialnumber = 0;
	}
	return pEntity;
}

CBaseEntity* EHANDLE::operator->()
{
	if (Get() == nullptr)
	{
		return nullptr;
	}
	return Get()->Get<CBaseEntity>();
}


bool CBaseEntity::EntvarsKeyvalue(KeyValueData *pkvd)
{
	::EntvarsKeyvalue(&v, pkvd);
	return pkvd->fHandled != 0;
}


// give health
bool CBaseEntity::GiveHealth(float flHealth, int bitsDamageType)
{
	if (v.takedamage == DAMAGE_NO)
	{
		return false;
	}

	// heal
	if (v.health >= v.max_health)
		return false;

	v.health += flHealth;

	if (v.health > v.max_health)
		v.health = v.max_health;

	return true;
}

// inflict damage on this entity.  bitsDamageType indicates type of damage inflicted, ie: DMG_CRUSH

bool CBaseEntity::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	Vector vecTemp;

	if (v.takedamage == DAMAGE_NO)
	{
		return false;
	}

	// if Attacker == Inflictor, the attack was a melee or other instant-hit attack.
	// (that is, no actual entity projectile was involved in the attack so use the shooter's origin).
	if (attacker == inflictor)
	{
		vecTemp = inflictor->v.origin - Center();
	}
	else
	// an actual missile was involved.
	{
		vecTemp = inflictor->v.origin - Center();
	}

	// this global is still used for glass and other non-monster killables, along with decals.
	g_vecAttackDir = vecTemp.Normalize();

	// save damage based on the target's armor level

	// figure momentum add (don't let hurt brushes or other triggers move player)
	if (inflictor != nullptr && (v.movetype == MOVETYPE_WALK || v.movetype == MOVETYPE_STEP) && (inflictor->v.solid != SOLID_TRIGGER))
	{
		Vector vecDir = v.origin - (inflictor->v.absmin + inflictor->v.absmax) * 0.5;
		vecDir = vecDir.Normalize();

		float flForce = flDamage * ((32 * 32 * 72.0) / (v.size.x * v.size.y * v.size.z)) * 5;

		if (flForce > 1000.0)
			flForce = 1000.0;
		v.velocity = v.velocity + vecDir * flForce;
	}

	// do the damage
	v.health -= flDamage;
	if (v.health <= 0)
	{
		Killed(inflictor, attacker, bitsDamageType);
		return false;
	}

	return true;
}


void CBaseEntity::Killed(CBaseEntity* inflictor, CBaseEntity* attacker, int bitsDamageType)
{
	v.takedamage = DAMAGE_NO;
	v.deadflag = DEAD_DEAD;
	Remove();
}


CBaseEntity* CBaseEntity::GetNextTarget()
{
	if (FStringNull(v.target))
		return nullptr;

	return util::FindEntityByTargetname(nullptr, STRING(v.target));
}

// Global Savedata for Delay
#ifdef HALFLIFE_SAVERESTORE
TYPEDESCRIPTION CBaseEntity::m_SaveData[] =
{
	DEFINE_FIELD(CBaseEntity, m_pGoalEnt, FIELD_CLASSPTR),
	DEFINE_FIELD(CBaseEntity, m_EFlags, FIELD_CHARACTER),

	DEFINE_FIELD(CBaseEntity, m_pfnThink, FIELD_FUNCTION),
	DEFINE_FIELD(CBaseEntity, m_pfnTouch, FIELD_FUNCTION),
	DEFINE_FIELD(CBaseEntity, m_pfnUse, FIELD_FUNCTION),
	DEFINE_FIELD(CBaseEntity, m_pfnBlocked, FIELD_FUNCTION),

	DEFINE_FIELD(CBaseDelay, m_flDelay, FIELD_FLOAT),
	DEFINE_FIELD(CBaseDelay, m_iszKillTarget, FIELD_STRING),
};


bool CBaseEntity::Save(CSave& save)
{
	if (save.WriteEntVars("ENTVARS", &v))
		return save.WriteFields("BASE", this, m_SaveData, ARRAYSIZE(m_SaveData));

	return false;
}

bool CBaseEntity::Restore(CRestore& restore)
{
	bool status;

	status = restore.ReadEntVars("ENTVARS", &v);
	if (status)
		status = restore.ReadFields("BASE", this, m_SaveData, ARRAYSIZE(m_SaveData));

	if (v.modelindex != 0 && !FStringNull(v.model))
	{
		Vector mins, maxs;
		mins = v.mins; // Set model is about to destroy these
		maxs = v.maxs;


		PRECACHE_MODEL((char*)STRING(v.model));
		SetModel(STRING(v.model));
		SetSize(mins, maxs); // Reset them
	}

	return status;
}
#endif


// Initialize absmin & absmax to the appropriate box
void SetObjectCollisionBox(Entity* entity)
{
	if ((entity->solid == SOLID_BSP) &&
		(entity->angles != g_vecZero))
	{ // expand for rotation
		float max, v;
		int i;

		max = 0;
		for (i = 0; i < 3; i++)
		{
			v = fabs(((float*)entity->mins)[i]);
			if (v > max)
				max = v;
			v = fabs(((float*)entity->maxs)[i]);
			if (v > max)
				max = v;
		}
		for (i = 0; i < 3; i++)
		{
			((float*)entity->absmin)[i] = ((float*)entity->origin)[i] - max;
			((float*)entity->absmax)[i] = ((float*)entity->origin)[i] + max;
		}
	}
	else
	{
		entity->absmin = entity->origin + entity->mins;
		entity->absmax = entity->origin + entity->maxs;
	}

	entity->absmin.x -= 1;
	entity->absmin.y -= 1;
	entity->absmin.z -= 1;
	entity->absmax.x += 1;
	entity->absmax.y += 1;
	entity->absmax.z += 1;
}


void CBaseEntity::SetObjectCollisionBox()
{
	::SetObjectCollisionBox(&v);
}


void CBaseEntity::SetOrigin(const Vector& org)
{
	v.origin = org;
	g_engfuncs.pfnSetOrigin(&v, org);
}


void CBaseEntity::SetSize(const Vector& mins, const Vector& maxs)
{
	g_engfuncs.pfnSetSize(&v, mins, maxs);
}


void CBaseEntity::SetModel(const char* name)
{
	g_engfuncs.pfnSetModel(&v, name);
}


bool CBaseEntity::Intersects(CBaseEntity* pOther)
{
	if (pOther->v.absmin.x > v.absmax.x ||
		pOther->v.absmin.y > v.absmax.y ||
		pOther->v.absmin.z > v.absmax.z ||
		pOther->v.absmax.x < v.absmin.x ||
		pOther->v.absmax.y < v.absmin.y ||
		pOther->v.absmax.z < v.absmin.z)
		return false;
	return true;
}

#ifdef HALFLIFE_SAVERESTORE
void CBaseEntity::MakeDormant()
{
	SetBits(v.flags, FL_DORMANT);

	// Don't touch
	v.solid = SOLID_NOT;
	// Don't move
	v.movetype = MOVETYPE_NONE;
	// Don't draw
	SetBits(v.effects, EF_NODRAW);
	// Don't think
	v.nextthink = 0;
	// Relink
	SetOrigin(v.origin);
}

bool CBaseEntity::IsDormant()
{
	return FBitSet(v.flags, FL_DORMANT);
}
#endif

bool CBaseEntity::ShouldToggle(USE_TYPE useType, bool currentState)
{
	if (useType != USE_TOGGLE && useType != USE_SET)
	{
		if ((currentState && useType == USE_ON) || (!currentState && useType == USE_OFF))
			return false;
	}
	return true;
}



// NOTE: szName must be a pointer to constant memory, e.g. "monster_class" because the entity
// will keep a pointer to it after this call.
CBaseEntity* CBaseEntity::Create(const char* szName, const Vector& vecOrigin, const Vector& vecAngles, Entity& owner)
{
	auto pent = g_engfuncs.pfnCreateNamedEntity(MAKE_STRING(szName));

	if (pent == nullptr)
	{
		ALERT(at_console, "NULL Ent in Create!\n");
		return NULL;
	}

	auto entity = pent->Get<CBaseEntity>();

	entity->v.owner = &owner;
	entity->v.origin = vecOrigin;
	entity->v.angles = vecAngles;

	DispatchSpawn(entity->edict());

	return entity;
}


void CBaseEntity::GetEntityState(entity_state_t& state)
{
	state.entityType = ENTITY_NORMAL;

	// Flag custom entities.
	if ((v.flags & FL_CUSTOMENTITY) != 0)
	{
		state.entityType = ENTITY_BEAM;
	}

	// Round animtime to nearest millisecond
	state.animtime = (int)(1000.0 * v.animtime) / 1000.0;

	state.origin = v.origin;
	state.angles = v.angles;
	state.mins = v.mins;
	state.maxs = v.maxs;

	state.startpos = v.startpos;
	state.endpos = v.endpos;

	state.impacttime = v.impacttime;
	state.starttime = v.starttime;

	if ((v.flags & EF_NODRAW) != 0)
	{
		state.modelindex = 0;
	}
	else
	{
		state.modelindex = v.modelindex;
	}

	state.frame = v.frame;

	state.skin = v.skin;
	state.effects = v.effects;

	if ((v.flags & FL_FLY) != 0)
	{
		state.eflags |= EFLAG_SLERP;
	}
	else
	{
		state.eflags &= ~EFLAG_SLERP;
	}

	state.eflags |= m_EFlags;

	state.scale = v.scale;
	state.solid = v.solid;
	state.colormap = v.colormap;

	state.movetype = v.movetype;
	state.sequence = v.sequence;
	state.framerate = v.framerate;
	state.body = v.body;

	for (int i = 0; i < 4; i++)
	{
		state.controller[i] = v.controller[i];
	}

	for (int i = 0; i < 2; i++)
	{
		state.blending[i] = v.blending[i];
	}

	state.rendermode = v.rendermode;
	state.renderamt = v.renderamt;
	state.renderfx = v.renderfx;
	state.rendercolor.r = v.rendercolor.x;
	state.rendercolor.g = v.rendercolor.y;
	state.rendercolor.b = v.rendercolor.z;

	state.aiment = 0;
	if (v.aiment)
	{
		state.aiment = ENTINDEX(v.aiment);
	}

	state.owner = 0;
	if (v.owner)
	{
		int owner = ENTINDEX(v.owner);

		// Only care if owned by a player
		if (owner >= 1 && owner <= gpGlobals->maxClients)
		{
			state.owner = owner;
		}
	}

	// Class is overridden for non-players to signify a breakable glass object ( sort of a class? )
	state.playerclass = v.playerclass;
}

