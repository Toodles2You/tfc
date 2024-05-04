//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Team Fortress map entities
//
// $NoKeywords: $
//=============================================================================

#include <algorithm>
#include <string>

#include "tf_goal.h"
#include "gamerules.h"
#include "tf_gamerules.h"
#include "game.h"

//==================================================
// CTFVars
//==================================================

LINK_ENTITY_TO_CLASS(info_tf_teamcheck, CTFTeamCheck);
LINK_ENTITY_TO_CLASS(info_tf_teamset, CTFTeamSet);

TYPEDESCRIPTION CTFVars::m_PairData[] = {
    /* Item info */
    DEFINE_FIELD(CTFVars, goal_no, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, group_no, FIELD_INTEGER),

    /* Info */
    DEFINE_FIELD(CTFVars, goal_activation, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, goal_effects, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, goal_result, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, goal_state, FIELD_INTEGER),

    DEFINE_FIELD_ALIAS(CTFVars, goal_activation, "g_a", FIELD_INTEGER),
    DEFINE_FIELD_ALIAS(CTFVars, goal_effects, "g_e", FIELD_INTEGER),

    DEFINE_FIELD(CTFVars, mdl, FIELD_STRING),

    /* Criteria */
    DEFINE_FIELD(CTFVars, team_no, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, teamcheck, FIELD_STRING),
    DEFINE_FIELD(CTFVars, playerclass, FIELD_INTEGER),

    DEFINE_FIELD(CTFVars, items_allowed, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, has_item_from_group, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, hasnt_item_from_group, FIELD_INTEGER),

    DEFINE_FIELD_ALIAS(CTFVars, has_item_from_group, "h_i_g", FIELD_INTEGER),

    DEFINE_FIELD(CTFVars, if_item_has_moved, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, if_item_hasnt_moved, FIELD_INTEGER),

    DEFINE_FIELD(CTFVars, if_goal_is_active, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, if_goal_is_inactive, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, if_goal_is_removed, FIELD_INTEGER),

    DEFINE_FIELD(CTFVars, if_group_is_active, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, if_group_is_inactive, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, if_group_is_removed, FIELD_INTEGER),

    /* Messages */
    DEFINE_FIELD(CTFVars, owned_by, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, owned_by_teamcheck, FIELD_STRING),

    DEFINE_FIELD(CTFVars, deathtype, FIELD_STRING),

    DEFINE_FIELD(CTFVars, broadcast, FIELD_STRING),
    DEFINE_FIELD(CTFVars, team_broadcast, FIELD_STRING),
    DEFINE_FIELD(CTFVars, non_team_broadcast, FIELD_STRING),
    DEFINE_FIELD(CTFVars, owners_team_broadcast, FIELD_STRING),
    DEFINE_FIELD(CTFVars, non_owners_team_broadcast, FIELD_STRING),

    DEFINE_FIELD_ALIAS(CTFVars, broadcast, "b_b", FIELD_STRING),
    DEFINE_FIELD_ALIAS(CTFVars, team_broadcast, "b_t", FIELD_STRING),
    DEFINE_FIELD_ALIAS(CTFVars, non_team_broadcast, "b_n", FIELD_STRING),
    DEFINE_FIELD_ALIAS(CTFVars, owners_team_broadcast, "b_o", FIELD_STRING),

    DEFINE_FIELD(CTFVars, netname_broadcast, FIELD_STRING),
    DEFINE_FIELD(CTFVars, netname_team_broadcast, FIELD_STRING),
    DEFINE_FIELD(CTFVars, netname_non_team_broadcast, FIELD_STRING),
    DEFINE_FIELD(CTFVars, netname_owners_team_broadcast, FIELD_STRING),

    DEFINE_FIELD_ALIAS(CTFVars, netname_broadcast, "n_b", FIELD_STRING),
    DEFINE_FIELD_ALIAS(CTFVars, netname_team_broadcast, "n_t", FIELD_STRING),
    DEFINE_FIELD_ALIAS(CTFVars, netname_non_team_broadcast, "n_n", FIELD_STRING),
    DEFINE_FIELD_ALIAS(CTFVars, netname_owners_team_broadcast, "n_o", FIELD_STRING),

    DEFINE_FIELD(CTFVars, speak, FIELD_STRING),
    DEFINE_FIELD(CTFVars, ap_speak, FIELD_STRING),
    DEFINE_FIELD(CTFVars, team_speak, FIELD_STRING),
    DEFINE_FIELD(CTFVars, non_team_speak, FIELD_STRING),
    DEFINE_FIELD(CTFVars, owners_team_speak, FIELD_STRING),
    DEFINE_FIELD(CTFVars, non_owners_team_speak, FIELD_STRING),

    /* Item status */
    DEFINE_FIELD(CTFVars, display_item_status1, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, display_item_status2, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, display_item_status3, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, display_item_status4, FIELD_INTEGER),

    DEFINE_FIELD(CTFVars, team_str_home, FIELD_STRING),
    DEFINE_FIELD(CTFVars, team_str_moved, FIELD_STRING),
    DEFINE_FIELD(CTFVars, team_str_carried, FIELD_STRING),

    DEFINE_FIELD_ALIAS(CTFVars, team_str_moved, "t_s_m", FIELD_STRING),
    DEFINE_FIELD_ALIAS(CTFVars, team_str_carried, "t_s_c", FIELD_STRING),

    DEFINE_FIELD(CTFVars, non_team_str_home, FIELD_STRING),
    DEFINE_FIELD(CTFVars, non_team_str_moved, FIELD_STRING),
    DEFINE_FIELD(CTFVars, non_team_str_carried, FIELD_STRING),

    DEFINE_FIELD_ALIAS(CTFVars, non_team_str_carried, "n_s_c", FIELD_STRING),

    /* Operations */
    DEFINE_FIELD(CTFVars, wait, FIELD_FLOAT),
    DEFINE_FIELD(CTFVars, delay_time, FIELD_FLOAT),

    DEFINE_FIELD(CTFVars, maxammo_shells, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, maxammo_nails, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, t_length, FIELD_INTEGER),

    /* Effects */
    DEFINE_FIELD(CTFVars, else_goal, FIELD_INTEGER),

    DEFINE_FIELD(CTFVars, all_active, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, last_impulse, FIELD_INTEGER),

    DEFINE_FIELD(CTFVars, activate_goal_no, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, inactivate_goal_no, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, remove_goal_no, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, restore_goal_no, FIELD_INTEGER),

    DEFINE_FIELD_ALIAS(CTFVars, remove_goal_no, "rv_g", FIELD_INTEGER),
    DEFINE_FIELD_ALIAS(CTFVars, restore_goal_no, "rs_g", FIELD_INTEGER),

    DEFINE_FIELD(CTFVars, activate_group_no, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, inactivate_group_no, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, remove_group_no, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, restore_group_no, FIELD_INTEGER),

    DEFINE_FIELD_ALIAS(CTFVars, remove_group_no, "rv_gr", FIELD_INTEGER),
    DEFINE_FIELD_ALIAS(CTFVars, restore_group_no, "rs_gr", FIELD_INTEGER),

    DEFINE_FIELD(CTFVars, remove_spawnpoint, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, restore_spawnpoint, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, remove_spawngroup, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, restore_spawngroup, FIELD_INTEGER),

    DEFINE_FIELD_ALIAS(CTFVars, remove_spawngroup, "rv_s_h", FIELD_INTEGER),
    DEFINE_FIELD_ALIAS(CTFVars, restore_spawngroup, "rs_s_h", FIELD_INTEGER),

    DEFINE_FIELD(CTFVars, items, FIELD_INTEGER),

    DEFINE_FIELD(CTFVars, axhitme, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, return_item_no, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, remove_item_group, FIELD_INTEGER),

    DEFINE_FIELD_ALIAS(CTFVars, remove_item_group, "r_i_g", FIELD_INTEGER),

    DEFINE_FIELD(CTFVars, lives, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, armorclass, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, ammo_shells, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, ammo_nails, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, ammo_rockets, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, ammo_cells, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, ammo_detpack, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, no_grenades_1, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, no_grenades_2, FIELD_INTEGER),

    DEFINE_FIELD_ALIAS(CTFVars, ammo_shells, "a_s", FIELD_INTEGER),
    DEFINE_FIELD_ALIAS(CTFVars, ammo_nails, "a_n", FIELD_INTEGER),
    DEFINE_FIELD_ALIAS(CTFVars, ammo_rockets, "a_r", FIELD_INTEGER),
    DEFINE_FIELD_ALIAS(CTFVars, ammo_cells, "a_c", FIELD_INTEGER),

    DEFINE_FIELD(CTFVars, invincible_finished, FIELD_FLOAT),
    DEFINE_FIELD(CTFVars, invisible_finished, FIELD_FLOAT),
    DEFINE_FIELD(CTFVars, super_damage_finished, FIELD_FLOAT),

    DEFINE_FIELD(CTFVars, count, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, increase_team, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, increase_team1, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, increase_team2, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, increase_team3, FIELD_INTEGER),
    DEFINE_FIELD(CTFVars, increase_team4, FIELD_INTEGER),

    /* End round */
    DEFINE_FIELD(CTFVars, endround_time, FIELD_FLOAT),
    DEFINE_FIELD(CTFVars, endround_owned_by, FIELD_STRING),
    DEFINE_FIELD(CTFVars, endround_non_owned_by, FIELD_STRING),
    DEFINE_FIELD(CTFVars, endround_team2, FIELD_STRING),
    DEFINE_FIELD(CTFVars, endround_team3, FIELD_STRING),
    DEFINE_FIELD(CTFVars, endround_team4, FIELD_STRING),
};

int CTFVars::m_PairCount;

//==================================================
// CTFVars
//==================================================

CTFVars::CTFVars()
{
    CTFVars::m_PairCount = (sizeof(CTFVars::m_PairData) / sizeof(CTFVars::m_PairData[0]));
    std::memset(this, 0, sizeof(this));
    goal_state = TFGS_INACTIVE;
    goal_effects = TFGE_AP;
}

int CTFVars::GetTeam()
{
    bool has_teamcheck = !FStringNull(teamcheck);

    if (!has_teamcheck)
    {
        return team_no;
    }

    if (m_pTeamCheck == nullptr
     && (m_pTeamCheck = (CTFTeamCheck*)util::FindEntityByTargetname(nullptr, STRING(teamcheck))) == nullptr)
    {
        return team_no;
    }

    return m_pTeamCheck->tfv.team_no;
}

int CTFVars::GetOwningTeam()
{
    bool has_teamcheck = !FStringNull(owned_by_teamcheck);

    if (!has_teamcheck)
    {
        return owned_by;
    }

    if (m_pOwnerTeamCheck == nullptr
     && (m_pOwnerTeamCheck = (CTFTeamCheck*)util::FindEntityByTargetname(nullptr, STRING(owned_by_teamcheck))) == nullptr)
    {
        return owned_by;
    }

    return m_pOwnerTeamCheck->tfv.team_no;
}

bool CTFVars::KeyValue(KeyValueData* pkvd)
{
    TYPEDESCRIPTION* field;

    for (int i = 0; i < CTFVars::m_PairCount; i++)
    {
        field = &CTFVars::m_PairData[i];

        if (!stricmp(field->fieldName, pkvd->szKeyName))
        {
            switch (field->fieldType)
            {
            case FIELD_STRING:
                (*(int*)((char*)this + field->fieldOffset)) = ALLOC_STRING(pkvd->szValue);
                break;
            case FIELD_FLOAT:
                (*(float*)((char*)this + field->fieldOffset)) = atof(pkvd->szValue);
                break;
            case FIELD_INTEGER:
                (*(int*)((char*)this + field->fieldOffset)) = atoi(pkvd->szValue);
                break;
            case FIELD_VECTOR:
                util::StringToVector((float*)((char*)this + field->fieldOffset), pkvd->szValue);
                break;
            }
            return true;
        }
    }

#if 0
    ALERT(at_aiconsole, "WARNING! Unhandled TF pair: \'%s\' = \'%s\'\n", pkvd->szKeyName, pkvd->szValue);
#endif
    return false;
}

bool CTFVars::PlayerMeetsCriteria(CBaseEntity* player)
{
    CTFGoal* goal = nullptr;

    if (player && player->IsClient())
    {
        if (GetTeam() != TEAM_UNASSIGNED && GetTeam() != player->TeamNumber())
        {
            return false;
        }

        if (playerclass != PC_UNDEFINED && playerclass != player->PCNumber())
        {
            return false;
        }

        if (items_allowed)
        {
            goal = util::FindItem(items_allowed);

            if (goal != nullptr && goal->pev->owner != player->edict())
            {
                return false;
            }
        }
    }

    if (if_goal_is_active && !util::GoalInState(if_goal_is_active, TFGS_ACTIVE))
    {
        return false;
    }
    if (if_goal_is_inactive && !util::GoalInState(if_goal_is_inactive, TFGS_INACTIVE))
    {
        return false;
    }
    if (if_goal_is_removed && !util::GoalInState(if_goal_is_removed, TFGS_REMOVED))
    {
        return false;
    }

    if (if_group_is_active && !util::GroupInState(if_group_is_active, TFGS_ACTIVE))
    {
        return false;
    }
    if (if_group_is_inactive && !util::GroupInState(if_group_is_inactive, TFGS_INACTIVE))
    {
        return false;
    }
    if (if_group_is_removed && !util::GroupInState(if_group_is_removed, TFGS_REMOVED))
    {
        return false;
    }

    if (if_item_has_moved)
    {
        goal = util::FindItem(if_item_has_moved);

        if (goal != nullptr
         && !goal->InGoalState(TFGS_ACTIVE)
         && goal->pev->origin == goal->pev->oldorigin)
        {
            return false;
        }
    }
    if (if_item_hasnt_moved)
    {
        goal = util::FindItem(if_item_hasnt_moved);

        if (goal != nullptr
         && (goal->InGoalState(TFGS_ACTIVE)
          || goal->pev->origin != goal->pev->oldorigin))
        {
            return false;
        }
    }

    if (player && player->IsPlayer())
    {
        if (has_item_from_group)
        {
            bool got_one = false;

            while ((goal = (CTFGoal*)util::FindEntityByClassname(goal, "item_tfgoal")) && !got_one)
            {
                if (goal->GetGroup() == has_item_from_group && goal->pev->owner == player->edict())
                {
                    got_one = true;
                }
            }

            if (!got_one)
            {
                return false;
            }
        }
        if (hasnt_item_from_group)
        {
            while ((goal = (CTFGoal*)util::FindEntityByClassname(goal, "item_tfgoal")))
            {
                if (goal->GetGroup() == hasnt_item_from_group && goal->pev->owner == player->edict())
                {
                    return false;
                }
            }
        }
    }

    return true;
}

bool CTFVars::Activated(CBaseEntity* player, bool item)
{
    if (goal_state == TFGS_ACTIVE
     || goal_state == TFGS_REMOVED
     || goal_state == TFGS_DELAYED)
    {
        return false;
    }

    bool criteria_met = PlayerMeetsCriteria(player);
    bool reverse_criteria = false;

    if (item)
    {
        reverse_criteria = (goal_activation & TFGI_REVERSE_AP) != 0;
    }
    else
    {
        reverse_criteria = (goal_activation & TFGA_REVERSE_AP) != 0;
    }

    return criteria_met != reverse_criteria;
}

void CTFVars::DoGoalWork(CBaseEntity* player)
{
    if (activate_goal_no)
    {
        util::SetGoalState(activate_goal_no, TFGS_ACTIVE, player, this);
    }
    if (inactivate_goal_no)
    {
        util::SetGoalState(inactivate_goal_no, TFGS_INACTIVE);
    }
    if (restore_goal_no)
    {
        util::SetGoalState(restore_goal_no, TFGS_RESTORED);
    }
    if (remove_goal_no)
    {
        util::SetGoalState(remove_goal_no, TFGS_REMOVED);
    }

    if (return_item_no)
    {
        CTFGoalItem* goal = util::FindItem(return_item_no);

        if (goal != nullptr)
        {
            if (goal->pev->owner != nullptr)
            {
                CBaseEntity* owner = CBaseEntity::Instance(goal->pev->owner);
                goal->RemoveFromPlayer(owner, GI_DROP_REMOVEGOAL);
            }
            goal->StartReturnItem(0.1F);
        }
    }

    if (remove_spawnpoint)
    {
        CTFSpawn* spawn = util::FindTeamSpawn(remove_spawnpoint);

        if (spawn != nullptr)
        {
            spawn->tfv.goal_state = TFGS_ACTIVE; /* Toodles FIXME: Use TFGS_REMOVED? */
            spawn->DebugState();
        }
    }
    if (restore_spawnpoint)
    {
        CTFSpawn* spawn = util::FindTeamSpawn(restore_spawnpoint);

        if (spawn != nullptr)
        {
            spawn->tfv.goal_state = TFGS_INACTIVE;
            spawn->DebugState();
        }
    }
}

void CTFVars::DoGroupWork(CBaseEntity* player)
{
    if (all_active)
    {
        if (last_impulse == 0)
        {
            ALERT(at_console, "Goal %i has an all_active specified but, no last_impulse\n");
        }
        else if (util::GroupInState(all_active, TFGS_ACTIVE))
        {
            CTFGoal* goal = util::FindGoal(last_impulse);

            if (goal != nullptr)
            {
                goal->DoResults(player, (goal_result & TFGR_ADD_BONUSES) != 0);
            }
        }
    }

    if (activate_group_no)
    {
        util::SetGroupState(activate_group_no, TFGS_ACTIVE, player, this); /* Toodles: Originally never applied bonuses. May break some maps. */
    }
    if (inactivate_group_no)
    {
        util::SetGroupState(inactivate_group_no, TFGS_INACTIVE);
    }
    if (remove_group_no)
    {
        util::SetGroupState(remove_group_no, TFGS_REMOVED);
    }
    if (restore_group_no)
    {
        util::SetGroupState(restore_group_no, TFGS_RESTORED);
    }

    if (remove_spawngroup)
    {
        util::SetSpawnGroupActive(remove_spawngroup, false);
    }
    if (restore_spawngroup)
    {
        util::SetSpawnGroupActive(restore_spawngroup, true);
    }
}

void CTFVars::DisplayItemStatus(CBaseEntity* player, const int index)
{
    int goalNo = 0;
    switch (index)
    {
        case 0: goalNo = display_item_status1; break;
        case 1: goalNo = display_item_status2; break;
        case 2: goalNo = display_item_status3; break;
        case 3: goalNo = display_item_status4; break;
        default: return;
    }

    if (goalNo <= 0)
    {
        return;
    }

    auto goal = util::FindItem(goalNo);

    if (goal == nullptr)
    {
        return;
    }

    const auto isGoalOwner = player->TeamNumber() == goal->tfv.GetOwningTeam();
    const char* message = nullptr;
    const char* carrier = nullptr;

    if (goal->InGoalState(TFGS_ACTIVE))
    {
        /* Goal is being carried! */
        if (isGoalOwner)
        {
            if (!FStringNull(team_str_carried))
            {
                message = STRING(team_str_carried);
            }
        }
        else if (!FStringNull(non_team_str_carried))
        {
            message = STRING(non_team_str_carried);
        }

        /* Find the carrier. */
        if (goal->pev->owner != nullptr)
        {
            auto goalCarrier = CBaseEntity::Instance(goal->pev->owner);
            if (goalCarrier != nullptr)
            {
                carrier = STRING(goalCarrier->pev->netname);
            }
        }
    }
    else if (goal->pev->origin != goal->pev->oldorigin)
    {
        /* Goal is dropped! */
        if (isGoalOwner)
        {
            if (!FStringNull(team_str_moved))
            {
                message = STRING(team_str_moved);
            }
        }
        else if (!FStringNull(non_team_str_moved))
        {
            message = STRING(non_team_str_moved);
        }
    }
    else
    {
        /* Goal is at home! */
        if (isGoalOwner)
        {
            if (!FStringNull(team_str_home))
            {
                message = STRING(team_str_home);
            }
        }
        else if (!FStringNull(non_team_str_home))
        {
            message = STRING(non_team_str_home);
        }
    }

    if (message == nullptr)
    {
        return;
    }

    util::ClientPrint(player, HUD_PRINTTALK, message, carrier);
}

//==================================================
// CTFSpawn
//==================================================

LINK_ENTITY_TO_CLASS(info_player_teamspawn, CTFSpawn);
LINK_ENTITY_TO_CLASS(i_p_t, CTFSpawn);

CTFSpawn::CTFSpawn() : CTFGoal()
{
}

bool CTFSpawn::Spawn()
{
    if (!g_bDeveloperMode)
    {
        pev->effects |= EF_NODRAW;
    }
    else
    {
        g_engfuncs.pfnPrecacheModel("models/player/civilian/civilian.mdl");
        SetModel("models/player/civilian/civilian.mdl");

        pev->rendermode = kRenderTransAdd;
        pev->renderamt = 64;

        DebugState();
    }

    pev->solid = SOLID_NOT;
    pev->movetype = MOVETYPE_NONE;
    pev->classname = MAKE_STRING("info_player_teamspawn");
    SetUse(&CTFGoal::GoalUse);

    g_pGameRules->AddPlayerSpawnSpot(this);
    return true;
}

void CTFSpawn::DebugState()
{
    if (!g_bDeveloperMode)
    {
        return;
    }

    if (InGoalState(TFGS_INACTIVE))
    {
        pev->effects &= ~EF_NODRAW;
    }
    else
    {
        pev->effects |= EF_NODRAW;
    }
}

//==================================================
// CTFDetect
//==================================================

LINK_ENTITY_TO_CLASS(info_tfdetect, CTFDetect);

CTFDetect::CTFDetect() : CBaseEntity()
{
    number_of_teams = 2;
}

bool CTFDetect::KeyValue(KeyValueData* pkvd)
{
    CTeamFortress* pTFGameRules = dynamic_cast<CTeamFortress*>(g_pGameRules);

    if (FStrEq(pkvd->szKeyName, "number_of_teams"))
    {
        number_of_teams = std::clamp(atoi(pkvd->szValue), 1, (int)TEAM_SPECTATORS - 1);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "impulse"))
    {
        pTFGameRules->m_afToggleFlags = atoi(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "broadcast"))
    {
        /* Map version */
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "message"))
    {
        /* LocalCmd string (Client command???) */
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "team1_name"))
    {
        strncpy(pTFGameRules->m_TFTeamInfo[0].m_szTeamName, pkvd->szValue, 15);
        pTFGameRules->m_TFTeamInfo[0].m_szTeamName[15] = '\0';
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "team2_name"))
    {
        strncpy(pTFGameRules->m_TFTeamInfo[1].m_szTeamName, pkvd->szValue, 15);
        pTFGameRules->m_TFTeamInfo[1].m_szTeamName[15] = '\0';
        number_of_teams = std::max(number_of_teams, 2);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "team3_name"))
    {
        strncpy(pTFGameRules->m_TFTeamInfo[2].m_szTeamName, pkvd->szValue, 15);
        pTFGameRules->m_TFTeamInfo[2].m_szTeamName[15] = '\0';
        number_of_teams = std::max(number_of_teams, 3);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "team4_name"))
    {
        strncpy(pTFGameRules->m_TFTeamInfo[3].m_szTeamName, pkvd->szValue, 15);
        pTFGameRules->m_TFTeamInfo[3].m_szTeamName[15] = '\0';
        number_of_teams = std::max(number_of_teams, 4);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "team1_allies"))
    {
        pTFGameRules->m_TFTeamInfo[0].m_afAlliedTeams = atoi(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "team2_allies"))
    {
        pTFGameRules->m_TFTeamInfo[1].m_afAlliedTeams = atoi(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "team3_allies"))
    {
        pTFGameRules->m_TFTeamInfo[2].m_afAlliedTeams = atoi(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "team4_allies"))
    {
        pTFGameRules->m_TFTeamInfo[3].m_afAlliedTeams = atoi(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "ammo_shells"))
    {
        pTFGameRules->m_TFTeamInfo[0].m_iLives = std::max(atoi(pkvd->szValue), 0);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "ammo_nails"))
    {
        pTFGameRules->m_TFTeamInfo[1].m_iLives = std::max(atoi(pkvd->szValue), 0);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "ammo_rockets"))
    {
        pTFGameRules->m_TFTeamInfo[2].m_iLives = std::max(atoi(pkvd->szValue), 0);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "ammo_cells"))
    {
        pTFGameRules->m_TFTeamInfo[3].m_iLives = std::max(atoi(pkvd->szValue), 0);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "ammo_medikit"))
    {
        pTFGameRules->m_TFTeamInfo[0].m_iMaxPlayers = std::max(atoi(pkvd->szValue), 0);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "ammo_detpack"))
    {
        pTFGameRules->m_TFTeamInfo[1].m_iMaxPlayers = std::max(atoi(pkvd->szValue), 0);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "maxammo_medikit"))
    {
        pTFGameRules->m_TFTeamInfo[2].m_iMaxPlayers = std::max(atoi(pkvd->szValue), 0);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "maxammo_detpack"))
    {
        pTFGameRules->m_TFTeamInfo[3].m_iMaxPlayers = std::max(atoi(pkvd->szValue), 0);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "maxammo_shells"))
    {
        pTFGameRules->m_TFTeamInfo[0].m_afInvalidClasses = atoi(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "maxammo_nails"))
    {
        pTFGameRules->m_TFTeamInfo[1].m_afInvalidClasses = atoi(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "maxammo_rockets"))
    {
        pTFGameRules->m_TFTeamInfo[2].m_afInvalidClasses = atoi(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "maxammo_cells"))
    {
        pTFGameRules->m_TFTeamInfo[3].m_afInvalidClasses = atoi(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "display_item_status1"))
    {
        pTFGameRules->display_item_status[0] = atoi(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "display_item_status2"))
    {
        pTFGameRules->display_item_status[1] = atoi(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "display_item_status3"))
    {
        pTFGameRules->display_item_status[2] = atoi(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "display_item_status4"))
    {
        pTFGameRules->display_item_status[3] = atoi(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "team_str_home"))
    {
        pTFGameRules->team_str_home = ALLOC_STRING(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "team_str_moved")
          || FStrEq(pkvd->szKeyName, "t_s_m"))
    {
        pTFGameRules->team_str_moved = ALLOC_STRING(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "team_str_carried")
          || FStrEq(pkvd->szKeyName, "t_s_c"))
    {
        pTFGameRules->team_str_carried = ALLOC_STRING(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "non_team_str_home"))
    {
        pTFGameRules->non_team_str_home = ALLOC_STRING(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "non_team_str_moved"))
    {
        pTFGameRules->non_team_str_moved = ALLOC_STRING(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "non_team_str_carried")
          || FStrEq(pkvd->szKeyName, "n_s_c"))
    {
        pTFGameRules->non_team_str_carried = ALLOC_STRING(pkvd->szValue);
        return true;
    }

#if 0
    ALERT(at_aiconsole, "WARNING! Unhandled TF pair: \'%s\' = \'%s\'\n", pkvd->szKeyName, pkvd->szValue);
#endif
    return false;
}

bool CTFDetect::Spawn()
{
    pev->effects |= EF_NODRAW;
    pev->solid = SOLID_NOT;
    pev->movetype = MOVETYPE_NONE;

    CTeamFortress* pTFGameRules = dynamic_cast<CTeamFortress*>(g_pGameRules);
    pTFGameRules->m_numTeams = number_of_teams;
    pTFGameRules->InitializeTeams();

    return false;
}

//==================================================
// CTFTeamCheck
//==================================================

CTFTeamCheck::CTFTeamCheck() : CBaseToggle()
{
}

bool CTFTeamCheck::Spawn()
{
    pev->effects |= EF_NODRAW;
    pev->solid = SOLID_NOT;
    pev->movetype = MOVETYPE_NONE;
    return true;
}

//==================================================
// CTFTeamSet
//==================================================

CTFTeamSet::CTFTeamSet() : CBaseToggle()
{
}

bool CTFTeamSet::Spawn()
{
    pev->effects |= EF_NODRAW;
    pev->solid = SOLID_NOT;
    pev->movetype = MOVETYPE_NONE;
    SetUse(&CTFTeamSet::TeamSetUse);
    return true;
}

void CTFTeamSet::TeamSetUse(CBaseEntity* activator, CBaseEntity* caller, USE_TYPE use_type, float value)
{
    CTFTeamCheck* team_check = nullptr;
    while ((team_check = (CTFTeamCheck*)util::FindEntityByTargetname(team_check, STRING(pev->target))))
    {
        if (tfv.team_no != 0)
        {
            team_check->tfv.team_no = tfv.team_no;
        }
        else
        {
            /* Toodles TODO: This could rotate through all four teams. */
            team_check->tfv.team_no = (team_check->tfv.team_no == TEAM_BLUE) ? TEAM_RED : TEAM_BLUE; 
        }
    }
}
