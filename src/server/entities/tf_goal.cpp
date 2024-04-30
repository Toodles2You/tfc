//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Team Fortress map entities
//
// $NoKeywords: $
//=============================================================================

#include <algorithm>

#include "tf_goal.h"
#include "player.h"
#include "game.h"
#include "gamerules.h"

//===========================================================================
// Functions handling Team Fortress map entities
//===========================================================================

CTFGoal* util::FindGoal(int goal_no)
{
    if (goal_no <= 0)
    {
        return nullptr;
    }

    CTFGoal* goal = nullptr;
    while ((goal = (CTFGoal*)util::FindEntityByClassname(goal, "info_tfgoal")))
    {
        if (goal->GetNumber() == goal_no)
        {
            return goal;
        }
    }

    ALERT(at_aiconsole, "Could not find a goal with a goal_no of %i\n", goal_no);
    return nullptr;
}

CTFGoalItem* util::FindItem(int item_no)
{
    if (item_no <= 0)
    {
        return nullptr;
    }

    CTFGoalItem* goal = nullptr;
    while ((goal = (CTFGoalItem*)util::FindEntityByClassname(goal, "item_tfgoal")))
    {
        if (goal->GetNumber() == item_no)
        {
            return goal;
        }
    }

    ALERT(at_aiconsole, "Could not find an item with a goal_no of %i\n", item_no);
    return nullptr;
}

CTFSpawn* util::FindTeamSpawn(int spawn_no)
{
    if (spawn_no <= 0)
    {
        return nullptr;
    }

    CTFSpawn* spawn = nullptr;
    while ((spawn = (CTFSpawn*)util::FindEntityByClassname(spawn, "info_player_teamspawn")))
    {
        if (spawn->tfv.goal_no == spawn_no)
        {
            return spawn;
        }
    }

    ALERT(at_aiconsole, "Could not find a spawn with a goal_no of %i\n", spawn_no);
    return nullptr;
}

bool util::GoalInState(int goal_no, TFGoalState state)
{
    if (goal_no <= 0)
    {
        return false;
    }

    CTFGoal* goal = util::FindGoal(goal_no);

    return goal != nullptr && goal->InGoalState(state);
}

bool util::GroupInState(int group_no, TFGoalState state, bool check_items = false)
{
    if (group_no <= 0)
    {
        return false;
    }

    bool found = false;

    const char* classname = check_items ? "item_tfgoal" : "info_tfgoal";

    CTFGoal* goal = nullptr;
    while ((goal = (CTFGoal*)util::FindEntityByClassname(goal, classname)))
    {
        if (goal->GetGroup() == group_no)
        {
            if (!goal->InGoalState(state))
            {
                return false;
            }
            found = true;
        }
    }

    if (!found)
    {
        ALERT(at_aiconsole, "Could not find any goals with a group_no of %i\n", group_no);
    }

    return true;
}

void util::SetGoalState(int goal_no, TFGoalState state, CBaseEntity* player = nullptr, CTFVars* activating_goal = nullptr)
{
    if (goal_no <= 0)
    {
        return;
    }

    CTFGoal* goal = util::FindGoal(goal_no);
    if (goal)
    {
        switch (state)
        {
            case TFGS_ACTIVE:
                goal->AttemptToActivate(player, activating_goal);
                break;
            case TFGS_INACTIVE:
                goal->InactivateGoal();
                break;
            case TFGS_RESTORED:
                goal->RestoreGoal();
                break;
            case TFGS_REMOVED:
                goal->RemoveGoal();
                break;
        }
    }
}

void util::SetGroupState(int group_no, TFGoalState state, CBaseEntity* player = nullptr, CTFVars* activating_goal = nullptr)
{
    if (group_no <= 0)
    {
        return;
    }

    bool found = false;

    CTFGoal* goal = nullptr;
    while ((goal = (CTFGoal*)util::FindEntityByClassname(goal, "info_tfgoal")))
    {
        if (goal->GetGroup() == group_no)
        {
            switch (state)
            {
                case TFGS_ACTIVE:
                    goal->AttemptToActivate(player, nullptr);
                    break;
                case TFGS_INACTIVE:
                    goal->InactivateGoal();
                    break;
                case TFGS_RESTORED:
                    goal->RestoreGoal();
                    break;
                case TFGS_REMOVED:
                    goal->RemoveGoal();
                    break;
            }
            found = true;
        }
    }

    if (!found)
    {
        ALERT(at_aiconsole, "Could not find any goals with a group_no of %i\n", group_no);
    }
}

void util::SetSpawnGroupActive(int group_no, bool active)
{
    if (group_no <= 0)
    {
        return;
    }

    bool found = false;

    CTFSpawn* spawn = nullptr;
    while ((spawn = (CTFSpawn*)util::FindEntityByClassname(spawn, "info_player_teamspawn")))
    {
        if (spawn->tfv.group_no == group_no)
        {
            /* Why, no, this is not intuitive at all! */
            /* Toodles FIXME: Use TFGS_REMOVED? */
            spawn->tfv.goal_state = active ? TFGS_INACTIVE : TFGS_ACTIVE;
            spawn->DebugState();
            found = true;
        }
    }

    if (!found)
    {
        ALERT(at_aiconsole, "Could not find any spawns with a group_no of %i\n", group_no);
    }
}

void util::GoalDetpackUse(const Vector& origin, CBaseEntity* activator, CBaseEntity* caller)
{
    CTFGoal* goal = nullptr;
    while ((goal = (CTFGoal*)util::FindEntityByClassname(goal, "info_tfgoal")))
    {
        if (goal->Classify() != CLASS_TFGOAL)
        {
            continue;
        }

        if (!goal->IsGoalActivatedBy(TFGA_TOUCH_DETPACK))
        {
            continue;
        }

        if ((goal->pev->origin - origin).Length() > WEAP_DETPACK_GOAL_SIZE)
        {
            continue;
        }

        TraceResult trace;
        util::TraceLine(goal->pev->origin, origin, util::ignore_monsters, goal, &trace);

        if (trace.flFraction != 1.0F && trace.pHit != activator->pev->pContainingEntity)
        {
            continue;
        }

        goal->GoalUse(activator, caller, USE_TOGGLE, 0.0F);
    }
}

//==================================================
// CTFGoal
//==================================================

LINK_ENTITY_TO_CLASS(info_tfgoal, CTFGoal);
LINK_ENTITY_TO_CLASS(i_t_g, CTFGoal);

CTFGoal::CTFGoal() : CBaseToggle()
{
    goal_min = Vector(-16, -16, 0);
    goal_max = Vector(16, 16, 56);

    search_time = 0.0F;
    pausetime = 0.0F;
    noise4 = iStringNull;
    team_drop = iStringNull;
    netname_team_drop = iStringNull;
    non_team_drop = iStringNull;
    netname_non_team_drop = iStringNull;
    distance = 0.0F;
    attack_finished = 0.0F;
    speed_reduction = 0.0F;

    m_pPlayer = nullptr;
    m_bAddBonuses = false;
}

bool CTFGoal::KeyValue(KeyValueData* pkvd)
{
    if (CBaseToggle::KeyValue(pkvd))
        return true;

    if (FStrEq(pkvd->szKeyName, "goal_min"))
    {
        util::StringToVector(goal_min, pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "goal_max"))
    {
        util::StringToVector(goal_max, pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "search_time"))
    {
        search_time = atof(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "pausetime"))
    {
        pausetime = atof(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "noise4"))
    {
        noise4 = ALLOC_STRING(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "team_drop"))
    {
        team_drop = ALLOC_STRING(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "netname_team_drop"))
    {
        netname_team_drop = ALLOC_STRING(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "non_team_drop"))
    {
        non_team_drop = ALLOC_STRING(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "netname_non_team_drop"))
    {
        netname_non_team_drop = ALLOC_STRING(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "distance"))
    {
        distance = atof(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "attack_finished"))
    {
        attack_finished = atof(pkvd->szValue);
        return true;
    }
    else if (FStrEq(pkvd->szKeyName, "speed_reduction"))
    {
        speed_reduction = atof(pkvd->szValue);
        return true;
    }

    return false;
}

void CTFGoal::Precache()
{
    if (!FStringNull(tfv.mdl))
        g_engfuncs.pfnPrecacheModel(STRING(tfv.mdl));
    if (!FStringNull(pev->noise))
        g_engfuncs.pfnPrecacheSound(STRING(pev->noise));
}

bool CTFGoal::Spawn()
{
    /* Toodles FIXME: Check existence? */
    Precache();
    pev->classname = MAKE_STRING("info_tfgoal");

    const auto bIsBSPModel = !FStringNull(pev->model) && STRING(pev->model)[0] == '*';

    if (!bIsBSPModel && !FStringNull(tfv.mdl))
    {
        g_engfuncs.pfnSetModel(edict(), STRING(tfv.mdl));
    }
    else
    {
        if (bIsBSPModel)
        {
            SetModel(STRING(pev->model));
        }
        else if (g_bDeveloperMode)
        {
            g_engfuncs.pfnPrecacheModel("models/presentsm.mdl");
            SetModel("models/presentsm.mdl");
        }
    }

    if (g_bDeveloperMode)
    {
        /* Render information saved in debug mode. */
        prev_rendermode = pev->rendermode;
        prev_renderamt = pev->renderamt;
        prev_rendercolor = pev->rendercolor;
        prev_renderfx = pev->renderfx;
    }

    pev->solid = SOLID_TRIGGER;

    SetOrigin(pev->origin);

    if (!bIsBSPModel)
    {
        SetSize(goal_min, goal_max);
    }

    StartGoal();

    if (!bIsBSPModel)
    {
        pev->sequence = 0;
        pev->frame = 0;
        // ResetSequenceInfo();
    }

    return true;
}

void CTFGoal::GoalUse(CBaseEntity* activator, CBaseEntity* caller, USE_TYPE use_type, float value)
{
    /* Toodles TODO: Activating goal could be caller. */
    AttemptToActivate(activator, &tfv);
}

void CTFGoal::PlaceGoal()
{
    if (Classify() == CLASS_TFGOAL && IsGoalActivatedBy(TFGA_TOUCH))
    {
        SetTouch(&CTFGoal::GoalTouch);
    }

    if (IsGoalActivatedBy(TFGA_DROPTOGROUND))
    {
        pev->movetype = MOVETYPE_TOSS;
        const float oldZed = pev->origin.z;
        pev->origin.z += 6;
        if (!g_engfuncs.pfnDropToFloor(edict()))
        {
#if 1
            ALERT(at_error, "Goal %s failed to drop to ground at %f, %f, %f\n", STRING(pev->classname), pev->origin.x, pev->origin.y, pev->origin.z);
            pev->origin.z = oldZed;
#else
            ALERT(at_error, "Goal %s fell out of level at %f, %f, %f\n", STRING(pev->classname), pev->origin.x, pev->origin.y, pev->origin.z);
            Remove();
            return;
#endif
        }
    }

    /* Toodles TODO: Team Fortress added FL_ITEM. Maybe make the hitbox bigger? */
    pev->movetype = MOVETYPE_NONE;
    pev->velocity = g_vecZero;
    pev->oldorigin = pev->origin;
    pev->punchangle = pev->angles;
}

void CTFGoal::StartGoal()
{
    SetVisible(true);
    SetUse(&CTFGoal::GoalUse);
    SetThink(&CTFGoal::PlaceGoal);
    pev->nextthink = gpGlobals->time + 0.2F;
    if (InGoalState(TFGS_REMOVED))
    {
        RemoveGoal();
    }
}

void CTFGoal::InactivateGoal()
{
    if (!InGoalState(TFGS_ACTIVE))
    {
        return;
    }

    if (Classify() != CLASS_TFGOAL_TIMER)
    {
        if (Classify() == CLASS_TFGOAL_ITEM && IsGoalActivatedBy(TFGI_SOLID))
            pev->solid = SOLID_BBOX;
        else
            pev->solid = SOLID_TRIGGER;
    }

    SetGoalState(TFGS_INACTIVE);
    SetVisible(true);
}

void CTFGoal::RestoreGoal()
{
    if (!InGoalState(TFGS_REMOVED))
    {
        return;
    }

    if (Classify() != CLASS_TFGOAL_TIMER)
    {
        if (Classify() == CLASS_TFGOAL_ITEM && IsGoalActivatedBy(TFGI_SOLID))
            pev->solid = SOLID_BBOX;
        else
            pev->solid = SOLID_TRIGGER;
    }
    else
    {
        pev->nextthink = gpGlobals->time + search_time;
    }

    SetGoalState(TFGS_INACTIVE);
    SetVisible(true);
}

void CTFGoal::RemoveGoal()
{
    pev->solid = SOLID_NOT;
    SetGoalState(TFGS_REMOVED);
    SetVisible(false);
}

void CTFGoal::GoalTouch(CBaseEntity* other)
{
    if (!IsGoalActivatedBy(TFGA_TOUCH))
    {
        return;
    }

    if (!other->IsPlayer())
    {
        return;
    }

    /* Replacement for clan battle prematch. */
    if (g_pGameRules->GetState() != GR_STATE_RND_RUNNING)
    {
        return;
    }

    if (InGoalState(TFGS_ACTIVE))
    {
        return;
    }

    AttemptToActivate(other, &tfv);
}

bool CTFGoal::IsPlayerAffected(CBaseEntity* player, CBaseEntity* activating_player)
{
    if (player->PCNumber() == PC_UNDEFINED)
    {
        return false;
    }

    if (HasGoalEffects(TFGE_SAME_ENVIRONMENT)
     && g_engfuncs.pfnPointContents(player->pev->origin) != g_engfuncs.pfnPointContents(pev->origin))
    {
        return false;
    }

    /* Toodles: This was originally an early return if true. */
    if (tfv.t_length != 0)
    {
        if ((pev->origin - player->pev->origin).LengthSquared() > tfv.t_length * tfv.t_length)
        {
            return false;
        }

        if (HasGoalEffects(TFGE_WALL))
        {
            TraceResult trace;
            util::TraceLine(pev->origin, player->pev->origin, util::IGNORE_MONSTERS::ignore_monsters, this, &trace);

            if (trace.flFraction != 1.0F)
            {
                return false;
            }
        }
    }

    if (Classify() != CLASS_TFGOAL_TIMER)
    {
        bool is_ap = (player == activating_player);
        if (is_ap && HasGoalEffects(TFGE_AP))
        {
            return true;
        }

        if (player->TeamNumber() == activating_player->TeamNumber())
        {
            if (HasGoalEffects(TFGE_AP_TEAM))
            {
                return true;
            }
        }
        else if (HasGoalEffects(TFGE_NOT_AP_TEAM))
        {
            return true;
        }

        if (!is_ap && HasGoalEffects(TFGE_NOT_AP))
        {
            return true;
        }
    }

    if (tfv.maxammo_shells && player->TeamNumber() == tfv.maxammo_shells)
    {
        return true;
    }

    if (tfv.maxammo_nails && player->TeamNumber() != tfv.maxammo_nails)
    {
        return true;
    }

    return false;
}

bool CTFGoal::AttemptToActivate(CBaseEntity* player, CTFVars* activating_goal)
{
    if (player == nullptr)
    {
        player = CWorld::World;
    }

    if (tfv.Activated(player, (Classify() == CLASS_TFGOAL_ITEM)))
    {
        if (activating_goal == &tfv)
        {
            DoResults(player, true);
        }
        else if (activating_goal != nullptr)
        {
            DoResults(player, (activating_goal->goal_result & TFGR_ADD_BONUSES) != 0);
        }
        else
        {
            DoResults(player, false);
        }

        return true;
    }
    else if (tfv.else_goal)
    {
        CTFGoal* goal = util::FindGoal(tfv.else_goal);

        if (goal != nullptr)
        {
            goal->AttemptToActivate(player, &tfv);
        }
    }

    return false;
}

void CTFGoal::DoResults(CBaseEntity* player, bool add_bonuses)
{
    /* Replacement for clan battle prematch. */
    if (g_pGameRules->GetState() != GR_STATE_RND_RUNNING)
    {
        return;
    }

    if (InGoalState(TFGS_ACTIVE))
    {
        return;
    }

    if (tfv.delay_time > 0.0F)
    {
        if (!InGoalState(TFGS_DELAYED))
        {
            SetGoalState(TFGS_DELAYED);
            SetThink(&CTFGoal::DelayedResults);
            pev->nextthink = gpGlobals->time + tfv.delay_time;
            m_pPlayer = player;
            m_bAddBonuses = add_bonuses;
            return;
        }
        else if (pev->nextthink > gpGlobals->time)
        {
            /*
            Toodles: The original code had what seemed like a flaw where results
            could be called during delay time. This might break something. I dunno.
            */
            ALERT(at_error, "Attempted to call results for goal %i while delaying\n", GetNumber());
            return;
        }
    }

    SetGoalState(TFGS_INACTIVE);

    if (Classify() != CLASS_TFGOAL_ITEM)
    {
        SetVisible(false);
    }

    if (!FStringNull(pev->noise))
    {
        EmitSound(STRING(pev->noise), CHAN_ITEM);
    }

    if (tfv.increase_team1)
    {
        g_pGameRules->AddPointsToTeam(TEAM_BLUE, tfv.increase_team1, true);
    }
    if (tfv.increase_team2)
    {
        g_pGameRules->AddPointsToTeam(TEAM_RED, tfv.increase_team2, true);
    }
    if (tfv.increase_team3)
    {
        g_pGameRules->AddPointsToTeam(TEAM_YELLOW, tfv.increase_team3, true);
    }
    if (tfv.increase_team4)
    {
        g_pGameRules->AddPointsToTeam(TEAM_GREEN, tfv.increase_team4, true);
    }

    if (Classify() != CLASS_TFGOAL_ITEM)
    {
        SetGoalState(TFGS_ACTIVE);
    }

    if (HasGoalResults(TFGR_ENDGAME))
    {
        g_pGameRules->EndMultiplayerGame(tfv.endround_time);
    }

    tfv.DoGroupWork(player);
    tfv.DoGoalWork(player);
    DoTriggerWork(player);

    /*
    Toodles: Moving this loop after all the trigger work fixes goals that both toggle spawn groups and force respawns.
    I'm not certain of how TFC actually handles this. I doubt this'll break any maps but, you never know.
    */

    if (!FStringNull(tfv.broadcast))
        util::ShowMessageAll(STRING(tfv.broadcast));
    if (!FStringNull(tfv.netname_broadcast))
        util::ClientPrintAll(HUD_PRINTCONSOLE, STRING(tfv.netname_broadcast));
    if (!FStringNull(tfv.speak))
        util::ClientHearVoxAll(STRING(tfv.speak));

    CBaseEntity* effect_player = nullptr;
    for (int i = 1; i <= gpGlobals->maxClients; i++)
    {
        effect_player = util::PlayerByIndex(i);
        if (!effect_player)
        {
            continue;
        }

        if (g_pGameRules->GetState() == GR_STATE_GAME_OVER)
        {
            switch (effect_player->TeamNumber())
            {
            case TEAM_BLUE:
            {
                if (!FStringNull(tfv.endround_team1))
                    util::ShowMessage(STRING(tfv.endround_team1), effect_player);
                break;
            }
            case TEAM_RED:
            {
                if (!FStringNull(tfv.endround_team2))
                    util::ShowMessage(STRING(tfv.endround_team2), effect_player);
                break;
            }
            case TEAM_YELLOW:
            {
                if (!FStringNull(tfv.endround_team3))
                    util::ShowMessage(STRING(tfv.endround_team3), effect_player);
                break;
            }
            case TEAM_GREEN:
            {
                if (!FStringNull(tfv.endround_team4))
                    util::ShowMessage(STRING(tfv.endround_team4), effect_player);
                break;
            }
            }
        }

        if (player && player->IsClient() && effect_player == player)
        {
            if (!FStringNull(pev->message))
                util::ShowMessage(STRING(pev->message), effect_player);
            if (!FStringNull(tfv.ap_speak))
                util::ClientHearVox(effect_player, STRING(tfv.ap_speak));
        }
        else
        {
            if (effect_player->TeamNumber() == tfv.GetOwningTeam())
            {
                if (g_pGameRules->GetState() == GR_STATE_GAME_OVER && !FStringNull(tfv.endround_owned_by))
                    util::ShowMessage(STRING(tfv.endround_owned_by), effect_player);
                if (!FStringNull(tfv.owners_team_broadcast))
                    util::ShowMessage(STRING(tfv.owners_team_broadcast), effect_player);
                if (!FStringNull(tfv.netname_owners_team_broadcast))
                    util::ClientPrint(effect_player, HUD_PRINTCONSOLE, STRING(tfv.netname_owners_team_broadcast));
                if (!FStringNull(tfv.owners_team_speak))
                    util::ClientHearVox(effect_player, STRING(tfv.owners_team_speak));
            }
            else
            {
                if (g_pGameRules->GetState() == GR_STATE_GAME_OVER && !FStringNull(tfv.endround_non_owned_by))
                    util::ShowMessage(STRING(tfv.endround_non_owned_by), effect_player);
                if (!FStringNull(tfv.non_owners_team_broadcast))
                    util::ShowMessage(STRING(tfv.non_owners_team_broadcast), effect_player);
                if (!FStringNull(tfv.non_owners_team_speak))
                    util::ClientHearVox(effect_player, STRING(tfv.non_owners_team_speak));

                if (player && player->IsClient() && effect_player->TeamNumber() == player->TeamNumber())
                {
                    if (!FStringNull(tfv.team_broadcast))
                        util::ShowMessage(STRING(tfv.team_broadcast), effect_player);
                    if (!FStringNull(tfv.netname_team_broadcast))
                        util::ClientPrint(effect_player, HUD_PRINTCONSOLE, STRING(tfv.netname_team_broadcast));
                    if (!FStringNull(tfv.team_speak))
                        util::ClientHearVox(effect_player, STRING(tfv.team_speak));
                }
                else
                {
                    if (!FStringNull(tfv.non_team_broadcast))
                        util::ShowMessage(STRING(tfv.non_team_broadcast), effect_player);
                    if (!FStringNull(tfv.netname_non_team_broadcast))
                        util::ClientPrint(effect_player, HUD_PRINTCONSOLE, STRING(tfv.netname_non_team_broadcast));
                    if (!FStringNull(tfv.non_team_speak))
                        util::ClientHearVox(effect_player, STRING(tfv.non_team_speak));
                }
            }
        }

        if (IsPlayerAffected(effect_player, player))
        {
            if (Classify() == CLASS_TFGOAL_TIMER && HasGoalEffects(TFGE_TIMER_CHECK_AP))
            {
                if (tfv.PlayerMeetsCriteria(effect_player))
                {
                    ApplyResults(effect_player, player, add_bonuses);
                }
            }
            else
            {
                ApplyResults(effect_player, player, add_bonuses);
            }
        }
    }

    if (Classify() != CLASS_TFGOAL_ITEM)
    {
        SetupRespawn();
    }
}

void CTFGoal::RemoveResults(CBaseEntity* player)
{
    if (player == nullptr || !player->IsPlayer())
    {
        return;
    }

    if (Classify() == CLASS_TFGOAL_ITEM)
    {
        if (!(dynamic_cast<CBasePlayer*>(player)->m_TFItems & tfv.items))
        {
            return;
        }

        if (IsGoalActivatedBy(TFGI_DONTREMOVERES))
        {
            return;
        }

        dynamic_cast<CBasePlayer*>(player)->m_TFItems &= ~tfv.items;
    }

    CBasePlayer* pl = dynamic_cast<CBasePlayer*>(player);

    if (pev->health > 0.0F)
    {
        pl->TakeDamage(this, this, pev->health, DMG_GENERIC);
    }
    else if (pev->health < 0.0F)
    {
        pl->GiveHealth(fabsf(pev->health), DMG_GENERIC);
    }

    pl->pev->armorvalue = std::clamp(pl->pev->armorvalue - pev->armorvalue, 0.0F, pl->m_flArmorMax);
    /* Toodles FIXME: Armor class */

    pl->GiveAmmo(-tfv.ammo_shells, AMMO_SHELLS);
    pl->GiveAmmo(-tfv.ammo_nails, AMMO_NAILS);
    pl->GiveAmmo(-tfv.ammo_rockets, AMMO_ROCKETS);
    pl->GiveAmmo(-tfv.ammo_cells, AMMO_CELLS);
    if (pl->PCNumber() == PC_DEMOMAN && tfv.ammo_detpack != 0)
    {
        pl->m_bDetpackReady = tfv.ammo_detpack < 0;
    }
    pl->GiveAmmo(-tfv.no_grenades_1, AMMO_GRENADES1);
    pl->GiveAmmo(-tfv.no_grenades_2, AMMO_GRENADES2);

    /* Toodles FIXME: Check power-ups */

    /* Toodles FIXME: Lives */

    if (pev->frags)
    {
        g_pGameRules->AddPointsToPlayer(pl, -pev->frags, true);
    }
}

void CTFGoal::DelayedResults()
{
    if (!InGoalState(TFGS_DELAYED))
    {
        ALERT(at_error, "Unexpected call to delayed results from goal %i\n", GetNumber());
        return;
    }
    DoResults(m_pPlayer, m_bAddBonuses);
    m_pPlayer = nullptr;
    m_bAddBonuses = false;
}

void CTFGoal::ApplyResults(CBaseEntity* player, CBaseEntity* activating_player, bool add_bonuses)
{
    if (Classify() == CLASS_TFGOAL_ITEM)
    {
        if (player && player->IsPlayer())
        {
            dynamic_cast<CBasePlayer*>(player)->m_TFItems |= tfv.items;
        }
    }

    if (activating_player && activating_player->IsClient() && player == activating_player && tfv.count && player->TeamNumber())
    {
        g_pGameRules->AddPointsToTeam(player->TeamNumber(), tfv.count, true);
    }

    if (add_bonuses)
    {
        CBasePlayer* pl = dynamic_cast<CBasePlayer*>(player);

        if (!pl->IsAlive())
        {
            goto dead_bonuses;
        }

        /* Toodles: Give activator credit for damage applied. */
        if (pev->health > 0.0F)
        {
            pl->GiveHealth(pev->health, DMG_GENERIC);
        }
        else if (pev->health < 0.0F)
        {
            pl->TakeDamage(this, activating_player, fabsf(pev->health), DMG_GENERIC);
        }

        pl->GiveArmor(pev->armortype, pev->armorvalue);
        /* Toodles FIXME: Armor class */

        pl->GiveAmmo(tfv.ammo_shells, AMMO_SHELLS);
        pl->GiveAmmo(tfv.ammo_nails, AMMO_NAILS);
        pl->GiveAmmo(tfv.ammo_rockets, AMMO_ROCKETS);
        pl->GiveAmmo(tfv.ammo_cells, AMMO_CELLS);
        if (pl->PCNumber() == PC_DEMOMAN && tfv.ammo_detpack != 0)
        {
            pl->m_bDetpackReady = tfv.ammo_detpack > 0;
        }
        pl->GiveAmmo(tfv.no_grenades_1, AMMO_GRENADES1);
        pl->GiveAmmo(tfv.no_grenades_2, AMMO_GRENADES2);

        /* Toodles FIXME: Check power-ups */

dead_bonuses:
        /* Toodles FIXME: Lives */

        if (pev->frags)
        {
            g_pGameRules->AddPointsToPlayer(pl, pev->frags, true);
        }
    }

#if 0
    if (player->PCNumber() == PC_SPY && HasGoalResults(TFGR_REMOVE_DISGUISE))
    {
        dynamic_cast<CBasePlayer*>(player)->RemoveDisguise();
        dynamic_cast<CBasePlayer*>(player)->StopFeigning();
    }
    if (player->PCNumber() == PC_ENGINEER && HasGoalResults(TFGR_DESTROY_BUILDINGS))
    {
        dynamic_cast<CBasePlayer*>(player)->RemoveAllBuildings(false);
    }
#endif

    CTFGoalItem* goal = nullptr;

    if (Classify() != CLASS_TFGOAL_ITEM && tfv.items)
    {
        goal = util::FindItem(tfv.items);

        if (goal != nullptr && goal != this)
        {
            goal->GiveToPlayer(player, this);
        }
    }

    if (tfv.axhitme)
    {
        goal = util::FindItem(tfv.axhitme);

        if (goal != nullptr && goal->pev->owner == player->edict())
        {
            goal->RemoveFromPlayer(player, GI_DROP_REMOVEGOAL);
        }
    }

    if (tfv.remove_item_group)
    {
        goal = nullptr;

        while ((goal = (CTFGoalItem*)util::FindEntityByClassname(goal, "item_tfgoal")))
        {
            if (goal->GetGroup() == tfv.remove_item_group
             && goal->pev->owner == activating_player->edict())
            {
                goal->RemoveFromPlayer(player, GI_DROP_REMOVEGOAL);
            }
        }
    }

    /* Toodles FIXME: Report item info */

    if (HasGoalResults(TFGR_FORCE_RESPAWN))
    {
        dynamic_cast<CBasePlayer*>(player)->RemoveAllWeapons();
        player->Spawn();
    }
}

void CTFGoal::DoTriggerWork(CBaseEntity* player)
{
    if (FStringNull(pev->target) && FStringNull(m_iszKillTarget))
    {
        return;
    }

    if (m_flDelay != 0)
    {
        CBaseEntity* pTemp = GetClassPtr((CBaseEntity*)nullptr);
        pTemp->pev->classname = MAKE_STRING("DelayedUse");

        pTemp->pev->nextthink = gpGlobals->time + m_flDelay;

        pTemp->SetThink(&CBaseEntity::DelayThink);

        pTemp->pev->button = (int)USE_TOGGLE;
        pTemp->m_iszKillTarget = m_iszKillTarget;
        pTemp->m_flDelay = 0;
        pTemp->pev->target = pev->target;

        if (player && player->IsClient())
        {
            pTemp->pev->owner = player->edict();
        }
        else
        {
            pTemp->pev->owner = nullptr;
        }
        return;
    }

    if (!FStringNull(m_iszKillTarget))
    {
        edict_t* pentKillTarget = NULL;

        ALERT(at_aiconsole, "KillTarget: %s\n", STRING(m_iszKillTarget));
        pentKillTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(m_iszKillTarget));
        while (!FNullEnt(pentKillTarget))
        {
            CBaseEntity::Instance(pentKillTarget)->Remove();

            ALERT(at_aiconsole, "killing %s\n", STRING(pentKillTarget->v.classname));
            pentKillTarget = FIND_ENTITY_BY_TARGETNAME(pentKillTarget, STRING(m_iszKillTarget));
        }
    }

    if (!FStringNull(pev->target))
    {
        util::FireTargets(STRING(pev->target), player, this, USE_TOGGLE, 0.0F);
    }
}

void CTFGoal::SetupRespawn()
{
    if (HasGoalResults(TFGR_SINGLE))
    {
        RemoveGoal();
        return;
    }
    if (tfv.wait != 0.0F)
    {
        if (tfv.wait > 0.0F)
        {
            SetThink(&CTFGoal::DoRespawn);
            pev->nextthink = gpGlobals->time + tfv.wait;
        }
        return;
    }
    InactivateGoal();
}

void CTFGoal::DoRespawn()
{
    RestoreGoal();
    InactivateGoal();
    if (!FStringNull(tfv.mdl))
    {
        EmitSound("items/itembk2.wav", CHAN_WEAPON, VOL_NORM, ATTN_IDLE);
    }
}

#if 0
bool CTFGoal::OnSpannerHit(CBaseEntity* player)
{
    if (!IsGoalActivatedBy(TFGA_SPANNER))
    {
        return false;
    }
    return AttemptToActivate(player, &tfv);
}
#endif

void CTFGoal::SetVisible(bool visible)
{
    const auto bHasModel = !FStringNull(tfv.mdl);

    if (!g_bDeveloperMode && !bHasModel)
    {
        pev->effects |= EF_NODRAW;
        return;
    }

    /* Goal items still become invisible while returning in debug mode. */
    if (visible)
    {
        pev->effects &= ~EF_NODRAW;
    }

    if (!g_bDeveloperMode)
    {
        if (!visible)
        {
            pev->effects |= EF_NODRAW;
        }
        return;
    }

    if (bHasModel && visible)
    {
        pev->rendermode = prev_rendermode;
        pev->renderamt = prev_renderamt;
        pev->rendercolor = prev_rendercolor;
        pev->renderfx = prev_renderfx;
    }
    else
    {
        pev->rendermode = bHasModel ? kRenderTransTexture : kRenderTransAdd;
        pev->renderamt = 64;
    }
}
