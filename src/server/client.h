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

#pragma once

extern qboolean ClientConnect(Entity* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128]);
extern void ClientDisconnect(Entity* pEntity);
extern void ClientKill(Entity* pEntity);
extern void ClientPutInServer(Entity* pEntity);
extern void ClientCommand(Entity* pEntity);
extern void ClientUserInfoChanged(Entity* pEntity, char* infobuffer);
extern void ServerActivate(Entity* pEdictList, int edictCount, int clientMax);
extern void ServerDeactivate();
#ifdef HALFLIFE_NODEGRAPH
void InitMapLoadingUtils();
#endif
extern void StartFrame();
extern void PlayerPostThink(Entity* pEntity);
extern void PlayerPreThink(Entity* pEntity);
extern void ParmsNewLevel();
extern void ParmsChangeLevel();

extern void ClientPrecache();

extern const char* GetGameDescription();
extern void PlayerCustomization(Entity* pEntity, customization_t* pCust);

extern void SpectatorConnect(Entity* pEntity);
extern void SpectatorDisconnect(Entity* pEntity);
extern void SpectatorThink(Entity* pEntity);

extern void Sys_Error(const char* error_string);

extern void SetupVisibility(Entity* pViewEntity, Entity* pClient, unsigned char** pvs, unsigned char** pas);
extern void UpdateClientData(const Entity* ent, int sendweapons, struct clientdata_s* cd);
extern int AddToFullPack(struct entity_state_s* state, int e, Entity* ent, Entity* host, int hostflags, int player, unsigned char* pSet);
extern void CreateBaseline(int player, int eindex, struct entity_state_s* baseline, Entity* entity, int playermodelindex, Vector* player_mins, Vector* player_maxs);
extern void RegisterEncoders();

extern int GetWeaponData(Entity* player, struct weapon_data_s* info);

extern void CmdStart(const Entity* player, const struct usercmd_s* cmd, unsigned int random_seed);
extern void CmdEnd(const Entity* player);

extern int ConnectionlessPacket(const struct netadr_s* net_from, const char* args, char* response_buffer, int* response_buffer_size);

extern int GetHullBounds(int hullnumber, float* mins, float* maxs);

extern void CreateInstancedBaselines();

extern int InconsistentFile(const Entity* player, const char* filename, char* disconnect_message);

extern int AllowLagCompensation();
