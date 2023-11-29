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

// CBaseEntity Stubs
bool CBaseEntity::TakeHealth(float flHealth, int bitsDamageType) { return true; }
bool CBaseEntity::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) { return true; }
CBaseEntity* CBaseEntity::GetNextTarget() { return NULL; }
bool CBaseEntity::Save(CSave& save) { return true; }
bool CBaseEntity::Restore(CRestore& restore) { return true; }
void CBaseEntity::SetObjectCollisionBox() {}
void CBaseEntity::SetOrigin(const Vector& org) {}
void CBaseEntity::SetModel(const char* name) {}
void CBaseEntity::SetSize(const Vector& mins, const Vector& maxs) {}
bool CBaseEntity::Intersects(CBaseEntity* pOther) { return false; }
void CBaseEntity::MakeDormant() {}
bool CBaseEntity::IsDormant() { return false; }
bool CBaseEntity::IsInWorld() { return true; }
bool CBaseEntity::ShouldToggle(USE_TYPE useType, bool currentState) { return false; }
CBaseEntity* CBaseEntity::Create(const char* szName, const Vector& vecOrigin, const Vector& vecAngles, edict_t* pentOwner) { return NULL; }
void CBaseEntity::Remove() {}

// CBaseDelay Stubs
bool CBaseDelay::KeyValue(struct KeyValueData_s*) { return false; }
bool CBaseDelay::Restore(class CRestore&) { return true; }
bool CBaseDelay::Save(class CSave&) { return true; }

// CBaseAnimating Stubs
bool CBaseAnimating::Restore(class CRestore&) { return true; }
bool CBaseAnimating::Save(class CSave&) { return true; }

// util Stubs
void util::PrecacheOther(const char* szClassname) {}
void util::MakeVectors(const Vector& vecAngles) {}
void util::LogPrintf(const char*, ...) {}
void util::ClientPrintAll(int, char const*, char const*, char const*, char const*, char const*) {}
void util::ClientPrint(CBaseEntity* entity, int msg_dest, const char* msg_name, const char* param1, const char* param2, const char* param3, const char* param4) {}

// CBaseToggle Stubs
bool CBaseToggle::Restore(class CRestore&) { return true; }
bool CBaseToggle::Save(class CSave&) { return true; }
bool CBaseToggle::KeyValue(struct KeyValueData_s*) { return false; }

// CGrenade Stubs
void CGrenade::BounceSound() {}
void CGrenade::Explode(Vector, Vector) {}
void CGrenade::Explode(TraceResult*, int) {}
void CGrenade::Killed(CBaseEntity*, CBaseEntity*, int) {}
bool CGrenade::Spawn() { return false; }
CGrenade* CGrenade::ShootTimed(CBaseEntity* owner, Vector vecStart, Vector vecVelocity, float time) { return 0; }
CGrenade* CGrenade::ShootContact(CBaseEntity* owner, Vector vecStart, Vector vecVelocity) { return 0; }
void CGrenade::DetonateUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) {}

CBaseEntity* util::FindEntityInSphere(CBaseEntity* pStartEntity, const Vector& vecCenter, float flRadius) { return 0; }

Vector util::VecToAngles(const Vector& vec) { return 0; }
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
void CBaseEntity::TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) {}

int TrainSpeed(int iSpeed, int iMax) { return 0; }
void CBasePlayer::DeathSound() {}
bool CBasePlayer::TakeHealth(float flHealth, int bitsDamageType) { return false; }
void CBasePlayer::TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) {}
bool CBasePlayer::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) { return false; }
void CBasePlayer::PackDeadPlayerWeapons() {}
void CBasePlayer::RemoveAllWeapons() {}
void CBasePlayer::SetAnimation(PLAYER_ANIM playerAnim) {}
void CBasePlayer::WaterMove() {}
void CBasePlayer::PlayerDeathFrame() {}
void CBasePlayer::StartObserver() {}
void CBasePlayer::PlayerUse() {}
void CBasePlayer::CheckTimeBasedDamage() {}
void CBasePlayer::Precache() {}
bool CBasePlayer::Save(CSave& save) { return false; }
bool CBasePlayer::Restore(CRestore& restore) { return false; }
bool CBasePlayer::HasWeapons() { return false; }
void CBasePlayer::SelectWeapon(const char* pstr) {}
void CBasePlayer::ForceClientDllUpdate() {}
void CBasePlayer::ImpulseCommands() {}
void CBasePlayer::CheatImpulseCommands(int iImpulse) {}
bool CBasePlayer::AddPlayerWeapon(CBasePlayerWeapon* pWeapon) { return false; }
bool CBasePlayer::RemovePlayerWeapon(CBasePlayerWeapon* pWeapon) { return false; }
int CBasePlayer::AmmoInventory(int iAmmoIndex) { return -1; }
void CBasePlayer::UpdateClientData() {}
void CBasePlayer::EnableControl(bool fControl) {}
Vector CBasePlayer::GetAimVector() { return g_vecZero; }
void CBasePlayer::SetCustomDecalFrames(int nFrames) {}
int CBasePlayer::GetCustomDecalFrames() { return -1; }
void CBasePlayer::DropPlayerWeapon(char* pszWeaponName) {}
bool CBasePlayer::HasPlayerWeapon(CBasePlayerWeapon* pCheckWeapon) { return false; }
bool CBasePlayer::SwitchWeapon(CBasePlayerWeapon* pWeapon) { return false; }
const char* CBasePlayer::TeamID() { return ""; }
int CBasePlayer::GiveAmmo(int iCount, int iType, int iMax) { return 0; }
void CBasePlayer::AddPoints(int score, bool bAllowNegativeScore) {}
void CBasePlayer::AddPointsToTeam(int score, bool bAllowNegativeScore) {}

void ClearMultiDamage() {}
void ApplyMultiDamage(CBaseEntity* inflictor, CBaseEntity* attacker) {}
void AddMultiDamage(CBaseEntity* inflictor, CBaseEntity* attacker, CBaseEntity* pEntity, float flDamage, int bitsDamageType) {}
bool CBasePlayerWeapon::Restore(class CRestore&) { return true; }
bool CBasePlayerWeapon::Save(class CSave&) { return true; }
void CBasePlayerWeapon::SetObjectCollisionBox() {}
void CBasePlayerWeapon::FallInit() {}
void CBasePlayerWeapon::FallThink() {}
void CBasePlayerWeapon::Materialize() {}
void CBasePlayerWeapon::AttemptToMaterialize() {}
void CBasePlayerWeapon::CheckRespawn() {}
CBaseEntity* CBasePlayerWeapon::Respawn() { return nullptr; }
void CBasePlayerWeapon::DefaultTouch(CBaseEntity* pOther) {}
void CBasePlayerWeapon::DestroyWeapon() {}
void CBasePlayerWeapon::Drop() {}
void CBasePlayerWeapon::Kill() {}
void CBasePlayerWeapon::AttachToPlayer(CBasePlayer* pPlayer) {}
bool CBasePlayerWeapon::AddDuplicate(CBasePlayerWeapon* pOriginal) { return false; }
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
void RadiusDamage(Vector vecSrc, CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, float flRadius, int bitsDamageType) {}


void CBaseEntity::EmitSound(const char* sample, int channel, float volume, float attenuation, int pitch, int flags) {}
void CBaseEntity::EmitSoundPredicted(const char* sample, int channel, float volume, float attenuation, int pitch, int flags) {}
void CBaseEntity::StopSound(const char* sample, int channel) {}

void CBaseEntity::SetEntityState(entity_state_t& state) {}
void CBasePlayer::SetEntityState(entity_state_t& state) {}
