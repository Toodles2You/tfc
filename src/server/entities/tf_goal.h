//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Team Fortress map entities
//
// $NoKeywords: $
//=============================================================================

#pragma once

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "tf_map.h"

class CTFGoal : public CBaseToggle
{
public:
    friend class CBasePlayer;

    CTFGoal();

    virtual bool KeyValue(KeyValueData* pkvd) override;
    virtual void Precache() override;
    virtual bool Spawn() override;
    virtual TFGoalClass Classify() { return CLASS_TFGOAL; }

    virtual void StartGoal();
    virtual bool AttemptToActivate(CBaseEntity* player, CTFVars* activating_goal);
    virtual void InactivateGoal();
    virtual void RestoreGoal();
    virtual void RemoveGoal();
    virtual void DoResults(CBaseEntity* player, bool add_bonuses);
    virtual void RemoveResults(CBaseEntity* player);
    virtual void SetupRespawn();

    void EXPORT GoalUse(CBaseEntity* activator, CBaseEntity* caller, USE_TYPE use_type, float value);
    void EXPORT PlaceGoal();
    void EXPORT GoalTouch(CBaseEntity* other);
    void EXPORT DelayedResults();
    void EXPORT DoRespawn();

#if 0
    virtual bool OnSpannerHit(CBaseEntity* player) override;
#endif

    void SetVisible(bool visible);

private:
    /* Render information saved in debug mode */
    int prev_rendermode;
    int prev_renderamt;
    Vector prev_rendercolor;
    int prev_renderfx;

protected:
    bool IsPlayerAffected(CBaseEntity* player, CBaseEntity* activating_player);
    void ApplyResults(CBaseEntity* player, CBaseEntity* activating_player, bool add_bonuses);
    void DoTriggerWork(CBaseEntity* player);

public:
    inline bool InGoalState(TFGoalState state) { return tfv.goal_state == state; }
    inline void SetGoalState(TFGoalState state) { tfv.goal_state = state; }
    inline int GetNumber() { return tfv.goal_no; }
    inline int GetGroup() { return tfv.group_no; }
    inline bool HasGoalEffects(int effects) { return (tfv.goal_effects & effects); }
    inline bool IsGoalActivatedBy(int activation) { return (tfv.goal_activation & activation); }
    inline bool HasGoalResults(int results) { return (tfv.goal_result & results); }

protected:
    Vector goal_min;
    Vector goal_max;
    
    float search_time;
    float pausetime;
    string_t noise4;
    string_t team_drop;
    string_t netname_team_drop;
    string_t non_team_drop;
    string_t netname_non_team_drop;
    float distance;
    float attack_finished;
    float speed_reduction;

    CBaseEntity* m_pPlayer;
    bool m_bAddBonuses;
};

class CTFGoalTimer : public CTFGoal
{
public:
    CTFGoalTimer();

    virtual bool Spawn() override;
    virtual TFGoalClass Classify() override { return CLASS_TFGOAL_TIMER; }

    virtual void StartGoal() override;
    virtual void SetupRespawn() override;

    void EXPORT PlaceTimer();
    void EXPORT TimerTick();
};

class CTFGoalItem : public CTFGoal
{
public:
    CTFGoalItem();

    virtual bool Spawn() override;
    virtual TFGoalClass Classify() override { return CLASS_TFGOAL_ITEM; }

    virtual void StartGoal() override;
    void GiveToPlayer(CBaseEntity* player, CTFGoal* activating_goal);
    void RemoveFromPlayer(CBaseEntity* activating_player, int method);
    void DropItem(CBaseEntity* activating_player, bool alive);
    void StartReturnItem(float delay = 0.5f);

    void EXPORT PlaceItem();
    void EXPORT ItemTouch(CBaseEntity* other);
    void EXPORT ReturnItem();
    void EXPORT RemoveItem();
    void EXPORT SetDropTouch();
    void EXPORT DropThink();

protected:
    void DoItemGroupWork(CBaseEntity* player);
    void SetGlow(CBaseEntity* entity, bool active);
};

class CTFSpawn : public CTFGoal
{
public:
    CTFSpawn();

    virtual bool Spawn() override;
    virtual TFGoalClass Classify() override { return CLASS_TFSPAWN; }

    void DebugState();
};

class CTFDetect : public CBaseEntity
{
public:
    CTFDetect();

    virtual bool KeyValue(KeyValueData* pkvd) override;
    virtual bool Spawn() override;

protected:
    int number_of_teams;
};

class CTFTeamCheck : public CBaseToggle
{
public:
    CTFTeamCheck();

    bool Spawn() override;
};

class CTFTeamSet : public CBaseToggle
{
public:
    CTFTeamSet();

    bool Spawn() override;
    void EXPORT TeamSetUse(CBaseEntity* activator, CBaseEntity* caller, USE_TYPE use_type, float value);
};
