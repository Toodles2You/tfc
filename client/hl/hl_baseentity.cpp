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
#include "nodes.h"
#include "skill.h"

void EMIT_SOUND_DYN(edict_t* entity, int channel, const char* sample, float volume, float attenuation, int flags, int pitch) {}

// CBaseEntity Stubs
bool CBaseEntity::TakeHealth(float flHealth, int bitsDamageType) { return true; }
bool CBaseEntity::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) { return true; }
CBaseEntity* CBaseEntity::GetNextTarget() { return NULL; }
bool CBaseEntity::Save(CSave& save) { return true; }
bool CBaseEntity::Restore(CRestore& restore) { return true; }
void CBaseEntity::SetObjectCollisionBox() {}
bool CBaseEntity::Intersects(CBaseEntity* pOther) { return false; }
void CBaseEntity::MakeDormant() {}
bool CBaseEntity::IsDormant() { return false; }
bool CBaseEntity::IsInWorld() { return true; }
bool CBaseEntity::ShouldToggle(USE_TYPE useType, bool currentState) { return false; }
int CBaseEntity::DamageDecal(int bitsDamageType) { return -1; }
CBaseEntity* CBaseEntity::Create(const char* szName, const Vector& vecOrigin, const Vector& vecAngles, edict_t* pentOwner) { return NULL; }
void CBaseEntity::SUB_Remove() {}

// CBaseDelay Stubs
bool CBaseDelay::KeyValue(struct KeyValueData_s*) { return false; }
bool CBaseDelay::Restore(class CRestore&) { return true; }
bool CBaseDelay::Save(class CSave&) { return true; }

// CBaseAnimating Stubs
bool CBaseAnimating::Restore(class CRestore&) { return true; }
bool CBaseAnimating::Save(class CSave&) { return true; }

// DEBUG Stubs
edict_t* DBG_EntOfVars(const entvars_t* pev) { return NULL; }
void DBG_AssertFunction(bool fExpr, const char* szExpr, const char* szFile, int szLine, const char* szMessage) {}

// UTIL_* Stubs
void UTIL_PrecacheOther(const char* szClassname) {}
void UTIL_BloodDrips(const Vector& origin, const Vector& direction, int color, int amount) {}
void UTIL_DecalTrace(TraceResult* pTrace, int decalNumber) {}
void UTIL_GunshotDecalTrace(TraceResult* pTrace, int decalNumber) {}
void UTIL_MakeVectors(const Vector& vecAngles) {}
bool UTIL_IsValidEntity(edict_t* pent) { return true; }
void UTIL_SetOrigin(entvars_t*, const Vector& org) {}
void UTIL_LogPrintf(char*, ...) {}
void UTIL_ClientPrintAll(int, char const*, char const*, char const*, char const*, char const*) {}
void ClientPrint(entvars_t* client, int msg_dest, const char* msg_name, const char* param1, const char* param2, const char* param3, const char* param4) {}

// CBaseToggle Stubs
bool CBaseToggle::Restore(class CRestore&) { return true; }
bool CBaseToggle::Save(class CSave&) { return true; }
bool CBaseToggle::KeyValue(struct KeyValueData_s*) { return false; }

// CGrenade Stubs
void CGrenade::BounceSound() {}
void CGrenade::Explode(Vector, Vector) {}
void CGrenade::Explode(TraceResult*, int) {}
void CGrenade::Killed(entvars_t*, entvars_t*, int) {}
bool CGrenade::Spawn() { return false; }
CGrenade* CGrenade::ShootTimed(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, float time) { return 0; }
CGrenade* CGrenade::ShootContact(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity) { return 0; }
void CGrenade::DetonateUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) {}

void UTIL_Remove(CBaseEntity* pEntity) {}
void UTIL_SetSize(entvars_t* pev, const Vector& vecMin, const Vector& vecMax) {}
CBaseEntity* UTIL_FindEntityInSphere(CBaseEntity* pStartEntity, const Vector& vecCenter, float flRadius) { return 0; }

Vector UTIL_VecToAngles(const Vector& vec) { return 0; }
CSprite* CSprite::SpriteCreate(const char* pSpriteName, const Vector& origin, bool animate) { return 0; }
void CBeam::PointEntInit(const Vector& start, int endIndex) {}
CBeam* CBeam::BeamCreate(const char* pSpriteName, int width) { return NULL; }
void CSprite::Expand(float scaleSpeed, float fadeSpeed) {}


float CBaseAnimating::StudioFrameAdvance(float flInterval) { return 0.0; }
bool CBaseEntity::FInViewCone(CBaseEntity* pEntity) { return false; }
bool CBaseEntity::FInViewCone(const Vector& vecOrigin) { return false; }
bool CBaseEntity::FVisible(CBaseEntity* pEntity) { return false; }
bool CBaseEntity::FVisible(const Vector& vecOrigin) { return false; }
void CBaseEntity::Look(int iDistance) {}
CBaseEntity* CBaseEntity::BestVisibleEnemy() { return nullptr; }
int CBaseAnimating::LookupActivity(int activity) { return 0; }
int CBaseAnimating::LookupActivityHeaviest(int activity) { return 0; }
int CBaseAnimating::LookupSequence(const char* label) { return 0; }
void CBaseAnimating::ResetSequenceInfo() {}
int CBaseAnimating::GetSequenceFlags() { return 0; }
void CBaseAnimating::DispatchAnimEvents(float flInterval) {}
float CBaseAnimating::SetBoneController(int iController, float flValue) { return 0.0; }
void CBaseAnimating::InitBoneControllers() {}
float CBaseAnimating::SetBlending(int iBlender, float flValue) { return 0; }
void CBaseAnimating::GetBonePosition(int iBone, Vector& origin, Vector& angles) {}
void CBaseAnimating::GetAttachment(int iAttachment, Vector& origin, Vector& angles) {}
int CBaseAnimating::FindTransition(int iEndingSequence, int iGoalSequence, int* piDir) { return -1; }
void CBaseAnimating::GetAutomovement(Vector& origin, Vector& angles, float flInterval) {}
void CBaseAnimating::SetBodygroup(int iGroup, int iValue) {}
int CBaseAnimating::GetBodygroup(int iGroup) { return 0; }
void CBaseEntity::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) {}
void CBaseEntity::TraceBleed(float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) {}

int TrainSpeed(int iSpeed, int iMax) { return 0; }
void CBasePlayer::DeathSound() {}
bool CBasePlayer::TakeHealth(float flHealth, int bitsDamageType) { return false; }
void CBasePlayer::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) {}
bool CBasePlayer::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) { return false; }
void CBasePlayer::PackDeadPlayerItems() {}
void CBasePlayer::RemoveAllItems(bool removeSuit) {}
void CBasePlayer::SetAnimation(PLAYER_ANIM playerAnim) {}
void CBasePlayer::WaterMove() {}
bool CBasePlayer::IsOnLadder() { return false; }
void CBasePlayer::PlayerDeathFrame() {}
void CBasePlayer::StartDeathCam() {}
void CBasePlayer::StartObserver(Vector vecPosition, Vector vecViewAngle) {}
void CBasePlayer::PlayerUse() {}
void CBasePlayer::Jump() {}
void CBasePlayer::Duck() {}
int CBasePlayer::Classify() { return 0; }
void CBasePlayer::PreThink() {}
void CBasePlayer::CheckTimeBasedDamage() {}
void CBasePlayer::UpdateGeigerCounter() {}
void CBasePlayer::CheckSuitUpdate() {}
void CBasePlayer::SetSuitUpdate(const char* name, bool fgroup, int iNoRepeatTime) {}
void CBasePlayer::CheckAmmoLevel(CBasePlayerItem* pItem, bool bPrimary) {}
void CBasePlayer::PostThink() {}
void CBasePlayer::Precache() {}
bool CBasePlayer::Save(CSave& save) { return false; }
bool CBasePlayer::Restore(CRestore& restore) { return false; }
bool CBasePlayer::HasWeapons() { return false; }
void CBasePlayer::SelectItem(const char* pstr) {}
void CBasePlayer::SelectItem(int iId) {}
void CBasePlayer::ForceClientDllUpdate() {}
void CBasePlayer::ImpulseCommands() {}
void CBasePlayer::CheatImpulseCommands(int iImpulse) {}
bool CBasePlayer::AddPlayerItem(CBasePlayerItem* pItem) { return false; }
bool CBasePlayer::RemovePlayerItem(CBasePlayerItem* pItem) { return false; }
void CBasePlayer::ItemPreFrame() {}
void CBasePlayer::ItemPostFrame() {}
int CBasePlayer::AmmoInventory(int iAmmoIndex) { return -1; }
void CBasePlayer::UpdateClientData() {}
void CBasePlayer::EnableControl(bool fControl) {}
Vector CBasePlayer::GetAimVector() { return g_vecZero; }
void CBasePlayer::SetCustomDecalFrames(int nFrames) {}
int CBasePlayer::GetCustomDecalFrames() { return -1; }
void CBasePlayer::DropPlayerItem(char* pszItemName) {}
bool CBasePlayer::HasPlayerItem(CBasePlayerItem* pCheckItem) { return false; }
bool CBasePlayer::SwitchWeapon(CBasePlayerItem* pWeapon) { return false; }
const char* CBasePlayer::TeamID() { return ""; }
int CBasePlayer::TeamNumber() { return 0; }
int CBasePlayer::GiveAmmo(int iCount, int iType, int iMax) { return 0; }
void CBasePlayer::AddPoints(int score, bool bAllowNegativeScore) {}
void CBasePlayer::AddPointsToTeam(int score, bool bAllowNegativeScore) {}

void ClearMultiDamage() {}
void ApplyMultiDamage(entvars_t* pevInflictor, entvars_t* pevAttacker) {}
void AddMultiDamage(entvars_t* pevInflictor, CBaseEntity* pEntity, float flDamage, int bitsDamageType) {}
void SpawnBlood(Vector vecSpot, int bloodColor, float flDamage) {}
int DamageDecal(CBaseEntity* pEntity, int bitsDamageType) { return 0; }
void DecalGunshot(TraceResult* pTrace, int iBulletType) {}
void EjectBrass(const Vector& vecOrigin, const Vector& vecVelocity, float rotation, int model, int soundtype) {}
bool CBasePlayerItem::Restore(class CRestore&) { return true; }
bool CBasePlayerItem::Save(class CSave&) { return true; }
bool CBasePlayerWeapon::Restore(class CRestore&) { return true; }
bool CBasePlayerWeapon::Save(class CSave&) { return true; }
void CBasePlayerItem::SetObjectCollisionBox() {}
void CBasePlayerItem::FallInit() {}
void CBasePlayerItem::FallThink() {}
void CBasePlayerItem::Materialize() {}
void CBasePlayerItem::AttemptToMaterialize() {}
void CBasePlayerItem::CheckRespawn() {}
CBaseEntity* CBasePlayerItem::Respawn() { return NULL; }
void CBasePlayerItem::DefaultTouch(CBaseEntity* pOther) {}
void CBasePlayerItem::DestroyItem() {}
void CBasePlayerItem::AddToPlayer(CBasePlayer* pPlayer) {}
void CBasePlayerItem::Drop() {}
void CBasePlayerItem::Kill() {}
bool CBasePlayerItem::Holster() { return true; }
void CBasePlayerItem::AttachToPlayer(CBasePlayer* pPlayer) {}
bool CBasePlayerWeapon::AddDuplicate(CBasePlayerItem* pOriginal) { return false; }
void CBasePlayerWeapon::AddToPlayer(CBasePlayer* pPlayer) {}
bool CBasePlayerWeapon::IsUseable() { return true; }
bool CBasePlayerAmmo::Spawn() { return false; }
CBaseEntity* CBasePlayerAmmo::Respawn() { return this; }
void CBasePlayerAmmo::Materialize() {}
void CBasePlayerAmmo::DefaultTouch(CBaseEntity* pOther) {}
bool CBasePlayerWeapon::ExtractAmmo(CBasePlayerWeapon* pWeapon) { return false; }
bool CBasePlayerWeapon::ExtractClipAmmo(CBasePlayerWeapon* pWeapon) { return false; }
void CBasePlayerWeapon::RetireWeapon() {}
void CBasePlayerWeapon::DoRetireWeapon() {}
void RadiusDamage(Vector vecSrc, entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType) {}
