//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Team Fortress map entities
//
// $NoKeywords: $
//=============================================================================

#pragma once

#include "extdll.h"

enum
{
    TFGA_TOUCH         = 1,    // Activated when touched
    TFGA_TOUCH_DETPACK = 2,    // Activated when touched by a detpack explosion
    TFGA_REVERSE_AP    = 4,    // Activated when AP details are _not_ met
    TFGA_SPANNER       = 8,    // Activated when hit by an engineer's spanner
    TFGA_DROPTOGROUND  = 2048, // Drop to Ground when spawning
};

// Defines for Goal Effects types
enum
{
    TFGE_AP               = 1,  // AP is affected. Default.
    TFGE_AP_TEAM          = 2,  // All of the AP's team.
    TFGE_NOT_AP_TEAM      = 4,  // All except AP's team.
    TFGE_NOT_AP           = 8,  // All except AP.
    TFGE_WALL             = 16, // If set, walls stop the Radius effects
    TFGE_SAME_ENVIRONMENT = 32, // If set, players in a different environment to the Goal are not affected
    TFGE_TIMER_CHECK_AP   = 64, // If set, Timer Goals check their critera for all players fitting their effects
};

// Defines for Goal Result types
enum
{
    TFGR_SINGLE             = 1,  // Goal can only be activated once
    TFGR_ADD_BONUSES        = 2,  // Any Goals activated by this one give their bonuses
    TFGR_ENDGAME            = 4,  // Goal fires Intermission, displays scores, and ends level
    TFGR_NO_ITEM_RESULTS    = 8,  // GoalItems given by this Goal don't do results
    TFGR_REMOVE_DISGUISE    = 16, // Prevent/Remove undercover from any Spy
    TFGR_FORCE_RESPAWN      = 32, // Forces the player to teleport to a respawn point
    TFGR_DESTROY_BUILDINGS  = 64, // Destroys this player's buildings, if any
};

// Defines for Goal Group Result types
// None!

// Defines for Goal Item types
enum
{
    TFGI_GLOW          = 1,    // Players carrying this GoalItem will glow
    TFGI_SLOW          = 2,    // Players carrying this GoalItem will move at half-speed
    TFGI_DROP          = 4,    // Players dying with this item will drop it
    TFGI_RETURN_DROP   = 8,    // Return if a player with it dies
    TFGI_RETURN_GOAL   = 16,   // Return if a player with it has it removed by a goal's activation
    TFGI_RETURN_REMOVE = 32,   // Return if it is removed by TFGI_REMOVE
    TFGI_REVERSE_AP    = 64,   // Only pickup if the player _doesn't_ match AP Details
    TFGI_REMOVE        = 128,  // Remove if left untouched for 2 minutes after being dropped
    TFGI_KEEP          = 256,  // Players keep this item even when they die
    TFGI_ITEMGLOWS     = 512,  // Item glows when on the ground
    TFGI_DONTREMOVERES = 1024, // Don't remove results when the item is removed
    TFGI_DROPTOGROUND  = 2048, // Drop To Ground when spawning
    TFGI_CANBEDROPPED  = 4096, // Can be voluntarily dropped by players
    TFGI_SOLID         = 8192, // Is solid... blocks bullets, etc
};

// Defines for methods of GoalItem returning
enum
{
    GI_RET_DROP_DEAD = 0, // Dropped by a dead player
    GI_RET_DROP_LIVING,   // Dropped by a living player
    GI_RET_GOAL,          // Returned by a Goal
    GI_RET_TIME,          // Returned due to timeout
};

// Defines for TeamSpawnpoints
enum
{
    TFSP_MULTIPLEITEMS = 1, // Give out the GoalItem multiple times
    TFSP_MULTIPLEMSGS  = 2, // Display the message multiple times
};

// Defines for TeamSpawnpoints
enum
{
    TFSP_REMOVESELF = 1, // Remove itself after being spawned on
};

// Defines for Goal States
enum TFGoalState
{
	TFGS_ACTIVE = 1,
	TFGS_INACTIVE,
	TFGS_REMOVED,
	TFGS_DELAYED,
	/* Toodles: Added for state set function. Goal classes don't actually use this. State will be set to TFGS_INACTIVE. */
	TFGS_RESTORED,
};

// Defines for GoalItem Removing from Player Methods
enum
{
    GI_DROP_PLAYERDEATH = 0, // Dropped by a dying player
    GI_DROP_REMOVEGOAL,      // Removed by a Goal
    GI_DROP_PLAYERDROP,      // Dropped by a player
};

// Legal Playerclass Handling
enum
{
    TF_ILL_SCOUT    = 1,
    TF_ILL_SNIPER   = 2,
    TF_ILL_SOLDIER  = 4,
    TF_ILL_DEMOMAN  = 8,
    TF_ILL_MEDIC    = 16,
    TF_ILL_HVYWEP   = 32,
    TF_ILL_PYRO     = 64,
    TF_ILL_RANDOMPC = 128,
    TF_ILL_SPY      = 256,
    TF_ILL_ENGINEER = 512,
};

// Addition classes
enum TFGoalClass
{
    CLASS_TFGOAL = 128,
    CLASS_TFGOAL_TIMER,
    CLASS_TFGOAL_ITEM,
    CLASS_TFSPAWN,
};

enum
{
	TFGI_ITEM_BLUE   = 131072,
	TFGI_ITEM_RED    = 262144,
	TFGI_ITEM_YELLOW = 16777216,
	TFGI_ITEM_GREEN  = 33554432,
};

class CBaseEntity;
class CTFVars;
class CTFGoal;
class CTFGoalItem;
class CTFSpawn;
class CTFTeamCheck;

namespace util
{
CTFGoal* FindGoal(int goal_no);
CTFGoalItem* FindItem(int item_no);
CTFSpawn* FindTeamSpawn(int spawn_no);
bool GoalInState(int goal_no, TFGoalState state);
bool GroupInState(int group_no, TFGoalState state, bool check_items = false);
void SetGoalState(int goal_no, TFGoalState state, CBaseEntity* player = nullptr, CTFVars* activating_goal = nullptr);
void SetGroupState(int group_no, TFGoalState state, CBaseEntity* player = nullptr, CTFVars* activating_goal = nullptr);
void SetSpawnGroupActive(int group_no, bool active);
} /* namespace util */

/* Toodles: I made these into their own class because multi-inheritence sucks. */
class CTFVars
{
public:
    CTFVars();

    int GetTeam();
    int GetOwningTeam();
    bool KeyValue(KeyValueData* pkvd);
    bool PlayerMeetsCriteria(CBaseEntity* player);
    bool Activated(CBaseEntity* player, bool item = false);
    void DoGoalWork(CBaseEntity* player);
    void DoGroupWork(CBaseEntity* player);

protected:
    CTFTeamCheck* m_pTeamCheck;
    CTFTeamCheck* m_pOwnerTeamCheck;

public:
    /* Item info */
    int goal_no;
    int group_no;

    /* Info */
    int goal_activation;
    int goal_effects;
    int goal_result;
    TFGoalState goal_state;

    string_t mdl;

    /* Criteria */
    int team_no;
    string_t teamcheck;
    int playerclass;

    int items_allowed;
    int has_item_from_group;
    int hasnt_item_from_group;

    int if_item_has_moved;
    int if_item_hasnt_moved;

    int if_goal_is_active;
    int if_goal_is_inactive;
    int if_goal_is_removed;

    int if_group_is_active;
    int if_group_is_inactive;
    int if_group_is_removed;

    /* Messages */
    int owned_by;
    string_t owned_by_teamcheck;

    string_t deathtype;

    string_t broadcast;
    string_t team_broadcast;
    string_t non_team_broadcast;
    string_t owners_team_broadcast;
    string_t non_owners_team_broadcast;

    string_t netname_broadcast; /* arg 1 = activator's name */
    string_t netname_team_broadcast;
    string_t netname_non_team_broadcast;
    string_t netname_owners_team_broadcast;

    string_t speak;
    string_t ap_speak;
    string_t team_speak;
    string_t non_team_speak;
    string_t owners_team_speak;
    string_t non_owners_team_speak;

    /* Item status */
    int display_item_status1;
    int display_item_status2;
    int display_item_status3;
    int display_item_status4;

    string_t team_str_home;
    string_t team_str_moved;
    string_t team_str_carried;

    string_t non_team_str_home;
    string_t non_team_str_moved;
    string_t non_team_str_carried;

    /* Operations */
    float wait;
    float delay_time;

    int maxammo_shells;
    int maxammo_nails;
    int t_length;

    /* Effects */
    int else_goal;
    
    int all_active;
    int last_impulse;

    int activate_goal_no;
    int inactivate_goal_no;
    int remove_goal_no;
    int restore_goal_no;

    int activate_group_no;
    int inactivate_group_no;
    int remove_group_no;
    int restore_group_no;

    int remove_spawnpoint;
    int restore_spawnpoint;
    int remove_spawngroup;
    int restore_spawngroup;

    int items;

    int axhitme;
    int return_item_no;
    int remove_item_group;

    int lives;
    int armorclass;

    int ammo_shells;
    int ammo_nails;
    int ammo_rockets;
    int ammo_cells;
    int ammo_detpack;
#if 0
    int ammo_medikit;
#endif
    int no_grenades_1;
    int no_grenades_2;

    float invincible_finished;
    float invisible_finished;
    float super_damage_finished;

    int count;
    int increase_team;
    int increase_team1;
    int increase_team2;
    int increase_team3;
    int increase_team4;

    /* End round */
    float endround_time;
    string_t endround_owned_by;
    string_t endround_non_owned_by;
    string_t endround_team1;
    string_t endround_team2;
    string_t endround_team3;
    string_t endround_team4;

private:
    static TYPEDESCRIPTION m_PairData[];
    static int m_PairCount;
};
