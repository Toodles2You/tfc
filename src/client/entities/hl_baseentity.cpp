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

/*
==========================
This file contains "stubs" of class member implementations so that we can predict certain
 weapons client side.  From time to time you might find that you need to implement part of the
 these functions.  If so, cut it from here, paste it in hl_weapons.cpp or somewhere else and
 add in the functionality you need.
==========================
*/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

bool CBaseEntity::EntvarsKeyvalue(struct KeyValueData_s*) { return false; }
bool CBaseEntity::KeyValue(KeyValueData* pkvd) { return false; }
bool CBaseEntity::GiveHealth(float flHealth, int bitsDamageType) { return true; }
bool CBaseEntity::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) { return true; }
CBaseEntity* CBaseEntity::GetNextTarget() { return NULL; }
#ifdef HALFLIFE_SAVERESTORE
bool CBaseEntity::Save(CSave& save) { return true; }
bool CBaseEntity::Restore(CRestore& restore) { return true; }
#endif
void CBaseEntity::SetObjectCollisionBox() {}
void CBaseEntity::SetOrigin(const Vector& org) {}
void CBaseEntity::SetModel(const char* name) {}
void CBaseEntity::SetSize(const Vector& mins, const Vector& maxs) {}
bool CBaseEntity::Intersects(CBaseEntity* pOther) { return false; }
#ifdef HALFLIFE_SAVERESTORE
void CBaseEntity::MakeDormant() {}
bool CBaseEntity::IsDormant() { return false; }
#endif
bool CBaseEntity::ShouldToggle(USE_TYPE useType, bool currentState) { return false; }
CBaseEntity* CBaseEntity::Create(const char* szName, const Vector& vecOrigin, const Vector& vecAngles, edict_t* pentOwner) { return NULL; }
void CBaseEntity::UseTargets(CBaseEntity* pActivator, USE_TYPE useType, float value) {}
void CBaseEntity::Remove() {}
bool CBaseEntity::FInViewCone(CBaseEntity* pEntity) { return false; }
bool CBaseEntity::FInViewCone(const Vector& vecOrigin) { return false; }
bool CBaseEntity::FVisible(CBaseEntity* pEntity) { return false; }
bool CBaseEntity::FVisible(const Vector& vecOrigin) { return false; }
void CBaseEntity::Look(int iDistance) {}
CBaseEntity* CBaseEntity::BestVisibleEnemy() { return nullptr; }
void CBaseEntity::TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, int hitgroup, int bitsDamageType) {}
void CBaseEntity::EmitSound(const char* sample, int channel, float volume, float attenuation, int pitch, int flags) {}
void CBaseEntity::StopSound(const char* sample, int channel) {}
void CBaseEntity::DelayThink() {}

void CBasePlayer::DeathSound() {}
bool CBasePlayer::GiveHealth(float flHealth, int bitsDamageType) { return false; }
void CBasePlayer::TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, int hitgroup, int bitsDamageType) {}
bool CBasePlayer::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) { return false; }
void CBasePlayer::PackDeadPlayerWeapons() {}
void CBasePlayer::RemoveAllWeapons() {}
void CBasePlayer::PlayerDeathFrame() {}
void CBasePlayer::StartObserver() {}
void CBasePlayer::PlayerUse() {}
void CBasePlayer::Precache() {}
#ifdef HALFLIFE_SAVERESTORE
bool CBasePlayer::Save(CSave& save) { return false; }
bool CBasePlayer::Restore(CRestore& restore) { return false; }
#endif
void CBasePlayer::ForceClientDllUpdate() {}
void CBasePlayer::ImpulseCommands() {}
void CBasePlayer::CheatImpulseCommands(int iImpulse) {}
void CBasePlayer::AddPlayerWeapon(CBasePlayerWeapon* weapon) {}
void CBasePlayer::RemovePlayerWeapon(CBasePlayerWeapon* weapon) {}
void CBasePlayer::UpdateClientData() {}
void CBasePlayer::EnableControl(bool fControl) {}
Vector CBasePlayer::GetAimVector() { return g_vecZero; }
void CBasePlayer::SetCustomDecalFrames(int nFrames) {}
int CBasePlayer::GetCustomDecalFrames() { return -1; }
void CBasePlayer::DropPlayerWeapon(char* pszWeaponName) {}
const char* CBasePlayer::TeamID() { return ""; }
bool CBasePlayer::GiveAmmo(int iCount, int iType) { return false; }

#ifdef HALFLIFE_SAVERESTORE
bool CBasePlayerWeapon::Restore(class CRestore&) { return true; }
bool CBasePlayerWeapon::Save(class CSave&) { return true; }
#endif
bool CBasePlayerWeapon::Spawn() { return false; }
void CBasePlayerWeapon::Precache() {}
void CBasePlayerWeapon::SetObjectCollisionBox() {}
void CBasePlayerWeapon::Materialize() {}
void CBasePlayerWeapon::AttemptToMaterialize() {}
void CBasePlayerWeapon::CheckRespawn() {}
CBaseEntity* CBasePlayerWeapon::Respawn() { return nullptr; }
void CBasePlayerWeapon::DefaultTouch(CBaseEntity* pOther) {}
bool CBasePlayerWeapon::AddToPlayer(CBasePlayer* pPlayer) { return false; }
void CBasePlayerWeapon::RemoveFromPlayer() {}
