//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Team Fortress map entities
//
// $NoKeywords: $
//=============================================================================

#include "tf_goal.h"
#include "player.h"
#include "game.h"
#include "gamerules.h"

//==================================================
// CTFGoalItem
//==================================================

LINK_ENTITY_TO_CLASS(item_tfgoal, CTFGoalItem);

CTFGoalItem::CTFGoalItem() : CTFGoal()
{
}

bool CTFGoalItem::Spawn()
{
    if (!CTFGoal::Spawn())
    {
        return false;
    }

    pev->classname = MAKE_STRING("item_tfgoal");

    pev->solid = IsGoalActivatedBy(TFGI_SOLID) ? SOLID_BBOX : SOLID_TRIGGER;

    if (FStringNull(pev->netname))
    {
        pev->netname = MAKE_STRING("goalitem");
    }

    if (pausetime <= 0.0F)
    {
        pausetime = (tfv.delay_time > 0.0F) ? tfv.delay_time : 60.0F;
    }

    return true;
}

void CTFGoalItem::PlaceItem()
{
    CTFGoal::PlaceGoal();
    if (IsGoalActivatedBy(TFGI_ITEMGLOWS))
    {
        SetGlow(this, true);
    }
    SetTouch(&CTFGoalItem::ItemTouch);
}

void CTFGoalItem::ReturnItem()
{
    SetGoalState(TFGS_INACTIVE);

    pev->solid = IsGoalActivatedBy(TFGI_SOLID) ? SOLID_BBOX : SOLID_TRIGGER;

    pev->movetype = MOVETYPE_NONE;
    SetVisible(true);

    SetTouch(&CTFGoalItem::ItemTouch);

    if (pev->origin == pev->oldorigin)
    {
        return;
    }

    pev->angles = pev->punchangle;
    SetOrigin(pev->oldorigin);

    EmitSound("items/itembk2.wav", CHAN_WEAPON);

    if (pev->impulse != 0)
    {
        util::SetGoalState(pev->impulse, TFGS_ACTIVE, CWorld::World, &tfv);
    }

    if (IsGoalActivatedBy(TFGI_ITEMGLOWS))
    {
        SetGlow(this, true);
    }

    if (pev->weapons != GI_RET_GOAL)
    {
        CBaseEntity* effect_player;
        for (int i = 0; i < gpGlobals->maxClients; i++)
        {
            effect_player = util::PlayerByIndex(i);

            if (!effect_player)
            {
                continue;
            }

            if (effect_player->TeamNumber() == tfv.GetOwningTeam())
            {
                if (!FStringNull(pev->noise3))
                    util::ShowMessage(STRING(pev->noise3), effect_player);
            }
            else
            {
                if (!FStringNull(noise4))
                    util::ShowMessage(STRING(noise4), effect_player);
            }
        }
    }
}

void CTFGoalItem::RemoveItem()
{
    if (InGoalState(TFGS_ACTIVE))
    {
        return;
    }
    if (IsGoalActivatedBy(TFGI_RETURN_REMOVE))
    {
        pev->weapons = GI_RET_TIME;
        StartReturnItem(0.1F);
        return;
    }
    SetGoalState(TFGS_REMOVED);
}

void CTFGoalItem::SetDropTouch()
{
    SetTouch(&CTFGoalItem::ItemTouch);
    SetThink(&CTFGoalItem::DropThink);
    pev->nextthink = gpGlobals->time + 4.25F;
}

/*
Toodles: There originally was a big check for solid
but, I honestly don't think it's that important.
*/
void CTFGoalItem::DropThink()
{
    pev->movetype = MOVETYPE_TOSS;

    float return_time = pausetime;

    int contents = g_engfuncs.pfnPointContents(pev->origin);
    switch (contents)
    {
        case CONTENTS_SLIME:
            return_time /= 4.0F;
            break;
        case CONTENTS_LAVA:
            return_time = 5.0F;
            break;
        case CONTENTS_SOLID:
        case CONTENTS_SKY:
            return_time = 2.0F;
            break;
    }

    SetThink(&CTFGoalItem::RemoveItem);
    pev->nextthink = gpGlobals->time + return_time;
}

void CTFGoalItem::StartGoal()
{
    SetThink(&CTFGoalItem::PlaceItem);
    pev->nextthink = gpGlobals->time + 0.2F;
    if (InGoalState(TFGS_REMOVED))
    {
        RemoveGoal();
    }
}

void CTFGoalItem::GiveToPlayer(CBaseEntity* player, CTFGoal* activating_goal)
{
    pev->aiment = pev->owner = player->edict();
    pev->movetype = MOVETYPE_FOLLOW;
    pev->solid = SOLID_NOT;
    SetTouch(nullptr);

    pev->sequence = 2;
    pev->frame = 0;
    pev->animtime = gpGlobals->time;

    if (IsGoalActivatedBy(TFGI_GLOW))
    {
        SetGlow(player, true);
    }
    /* Toodles FIXME: I dunno what past me was doing here. */
#if 0
    if (IsGoalActivatedBy(TFGI_ITEMGLOWS))
    {
        SetGlow(this, false);
    }
#endif

#if 0
    if (speed_reduction != 0.0F)
    {
        dynamic_cast<CBasePlayer*>(player)->m_flSpeedReduction = (speed_reduction / 100.0F);
        dynamic_cast<CBasePlayer*>(player)->UpdateMaxSpeed();
    }
    else if (IsGoalActivatedBy(TFGI_SLOW))
    {
        dynamic_cast<CBasePlayer*>(player)->m_flSpeedReduction = 0.5f;
        dynamic_cast<CBasePlayer*>(player)->UpdateMaxSpeed();
    }
#endif

    if (activating_goal != this && HasGoalResults(TFGR_NO_ITEM_RESULTS))
    {
        SetGoalState(TFGS_ACTIVE);
        return;
    }

#if 0
    if (player->PCNumber() == PC_SPY && HasGoalResults(TFGR_REMOVE_DISGUISE))
    {
        dynamic_cast<CBasePlayer*>(player)->RemoveDisguise();
        dynamic_cast<CBasePlayer*>(player)->StopFeigning(); // Toodles: Just in case, I guess.
        dynamic_cast<CBasePlayer*>(player)->m_bPreventDisguise = true;
    }
#endif

    DoResults(player, true);
    DoItemGroupWork(player);
}

void CTFGoalItem::RemoveFromPlayer(CBaseEntity* activating_player, int method)
{
    pev->owner = pev->aiment = nullptr;
    pev->movetype = MOVETYPE_NONE;
    pev->solid = SOLID_NOT;
    pev->angles.x = pev->angles.z = 0.0F;

    pev->sequence = 1;
    pev->frame = 0;
    pev->animtime = gpGlobals->time;

    CTFGoalItem* goal = nullptr;
    bool remove_glow = IsGoalActivatedBy(TFGI_GLOW);
    bool update_speed = IsGoalActivatedBy(TFGI_SLOW) || (speed_reduction != 0.0F);
    float best_reduction = 0.0F;
    bool let_disguise = HasGoalResults(TFGR_REMOVE_DISGUISE);
    int remove_items = tfv.items;

    while ((goal = (CTFGoalItem*)util::FindEntityByClassname(goal, "item_tfgoal")))
    {
        if (goal == this)
        {
            continue;
        }
        if (goal->pev->owner != activating_player->edict())
        {
            continue;
        }

        if (goal->IsGoalActivatedBy(TFGI_GLOW))
        {
            remove_glow = false;
        }

        if (goal->speed_reduction != 0.0F)
        {
            best_reduction = std::max(best_reduction, goal->speed_reduction);
        }
        else if (goal->IsGoalActivatedBy(TFGI_SLOW))
        {
            best_reduction = std::max(best_reduction, 0.5F);
        }

        if (goal->HasGoalResults(TFGR_REMOVE_DISGUISE))
        {
            let_disguise = false;
        }

        remove_items &= ~goal->tfv.items;
    }

    if (remove_glow)
    {
        SetGlow(activating_player, false);
    }
    /* Toodles FIXME: I dunno what past me was doing here. */
#if 0
    if (IsGoalActivatedBy(TFGI_ITEMGLOWS))
    {
        SetGlow(this, true);
    }
#endif

#if 0
    if (update_speed)
    {
        dynamic_cast<CBasePlayer*>(activating_player)->m_flSpeedReduction = best_reduction;
        dynamic_cast<CBasePlayer*>(activating_player)->UpdateMaxSpeed();
    }

    if (let_disguise)
        dynamic_cast<CBasePlayer*>(activating_player)->m_bPreventDisguise = false;
#endif

    dynamic_cast<CBasePlayer*>(activating_player)->m_TFItems &= ~remove_items;

    CBaseEntity* effect_player = nullptr;
    for (int i = 1; i <= gpGlobals->maxClients; i++)
    {
        effect_player = util::PlayerByIndex(i);
        if (!effect_player)
        {
            continue;
        }
        if (IsPlayerAffected(effect_player, activating_player))
        {
            RemoveResults(effect_player);
        }
    }

    if (method == GI_DROP_PLAYERDEATH || method == GI_DROP_PLAYERDROP)
    {
        for (int i = 1; i <= gpGlobals->maxClients; i++)
        {
            effect_player = util::PlayerByIndex(i);
            if (!effect_player)
            {
                continue;
            }

            if (effect_player->TeamNumber() == tfv.GetOwningTeam())
            {
                if (!FStringNull(team_drop))
                    util::ShowMessage(STRING(team_drop), effect_player);
                if (!FStringNull(netname_team_drop))
                    util::ClientPrint(effect_player, HUD_PRINTCONSOLE, STRING(netname_team_drop));
            }
            else
            {
                if (!FStringNull(non_team_drop))
                    util::ShowMessage(STRING(non_team_drop), effect_player);
                if (!FStringNull(netname_non_team_drop))
                    util::ClientPrint(effect_player, HUD_PRINTCONSOLE, STRING(netname_non_team_drop));
            }
        }

        if (IsGoalActivatedBy(TFGI_RETURN_DROP))
        {
            pev->weapons = (method == 0) ? GI_RET_DROP_DEAD : GI_RET_DROP_LIVING;
            StartReturnItem();
        }
        else if (IsGoalActivatedBy(TFGI_DROP))
        {
            DropItem(activating_player, method == GI_DROP_PLAYERDROP && IsGoalActivatedBy(TFGI_CANBEDROPPED));
        }
        else
        {
            SetVisible(false);
            SetGoalState(TFGS_REMOVED);
            return;
        }
        pev->movetype = MOVETYPE_TOSS;
        pev->flags &= ~FL_ONGROUND;
        SetSize(goal_min, goal_max);
        return;
    }
    else if (method == GI_DROP_REMOVEGOAL)
    {
        if (IsGoalActivatedBy(TFGI_RETURN_GOAL))
        {
            pev->weapons = GI_RET_GOAL;
            StartReturnItem();
            return;
        }
        SetVisible(false);
        /* Toodles: This wasn't here originally. */
#if 0
        SetGoalState(TFGS_REMOVED); 
#endif
        return;
    }
}

void CTFGoalItem::DropItem(CBaseEntity* activating_player, bool alive)
{
    pev->movetype = MOVETYPE_TOSS;
    SetOrigin(activating_player->pev->origin);

    pev->velocity =
        Vector(
            g_engfuncs.pfnRandomFloat(-50.0F, 50.0F),
            g_engfuncs.pfnRandomFloat(-50.0F, 50.0F),
            400.0F);

    if (IsGoalActivatedBy(TFGI_ITEMGLOWS))
    {
        SetGlow(this, true);
    }

    SetGoalState(TFGS_INACTIVE);

    pev->solid = IsGoalActivatedBy(TFGI_SOLID) ? SOLID_BBOX : SOLID_TRIGGER;

    if (alive)
    {
        util::MakeVectors(activating_player->pev->v_angle);
        pev->velocity = gpGlobals->v_forward * 400.0F;
        pev->velocity.z = 200.0F;
        SetTouch(nullptr);
        SetThink(&CTFGoalItem::SetDropTouch);
        pev->nextthink = gpGlobals->time + 0.75F;
    }
    else
    {
        SetTouch(&CTFGoalItem::ItemTouch);
        SetThink(&CTFGoalItem::DropThink);
        pev->nextthink = gpGlobals->time + 5.0F;
    }
}

void CTFGoalItem::ItemTouch(CBaseEntity* other)
{
    if (!other->IsPlayer() || !other->IsAlive())
    {
        return;
    }
    /* Replacement for clan battle prematch. */
    if (g_pGameRules->GetState() != GR_STATE_RND_RUNNING)
    {
        return;
    }
#if 0
    if (dynamic_cast<CBasePlayer*>(other)->m_bFeigning)
    {
        return;
    }
#endif
    if (other->edict() == pev->owner)
    {
        return;
    }

    if (tfv.Activated(other, true))
    {
        GiveToPlayer(other, this);

        /* The goal might've killed the activator. */
        if (other->IsAlive())
        {
            SetGoalState(TFGS_ACTIVE);
        }
    }
    else if (tfv.else_goal)
    {
        CTFGoal* goal = util::FindGoal(tfv.else_goal);

        if (goal != nullptr)
        {
            goal->AttemptToActivate(other, &tfv);
        }
    }
}

void CTFGoalItem::StartReturnItem(float delay)
{
    SetTouch(nullptr);
    SetThink(&CTFGoalItem::ReturnItem);
    pev->nextthink = gpGlobals->time + delay;
    pev->owner = pev->aiment = nullptr;
    pev->movetype = MOVETYPE_NONE;
    pev->solid = SOLID_NOT;
    SetVisible(false);
    pev->effects |= EF_NODRAW;
}

void CTFGoalItem::DoItemGroupWork(CBaseEntity* player)
{
    if (distance)
    {
        if (pev->pain_finished == 0)
        {
            ALERT(at_console, "Goal item %i has a distance specified but, no pain_finished\n");
        }
        else if (util::GroupInState(distance, TFGS_ACTIVE, true))
        {
            CTFGoal* goal = util::FindGoal(pev->pain_finished);

            if (goal != nullptr)
            {
                goal->DoResults(player, HasGoalResults(TFGR_ADD_BONUSES));
            }
        }
    }

    if (pev->speed)
    {
        if (attack_finished == 0)
        {
            ALERT(at_console, "Goal item %i has a speed specified but, no attack_finished\n");
        }
        else
        {
            CTFGoal* goal = nullptr;
            bool all_carried = true;
            edict_t* carrier = nullptr;
            while ((goal = (CTFGoal*)util::FindEntityByClassname(goal, "item_tfgoal")) && all_carried)
            {
                if (goal->GetGroup() != pev->speed)
                {
                    continue;
                }

                if (!goal->InGoalState(TFGS_ACTIVE))
                {
                    all_carried = false;
                }
                else
                {
                    if (!carrier)
                    {
                        carrier = goal->pev->owner;
                    }
                    else if (carrier != goal->pev->owner)
                    {
                        all_carried = false;
                    }
                }
            }

            if (all_carried)
            {
                CTFGoal* goal = util::FindGoal(attack_finished);

                if (goal != nullptr)
                {
                    goal->DoResults(player, HasGoalResults(TFGR_ADD_BONUSES));
                }
            }
        }
    }
}

void CTFGoalItem::SetGlow(CBaseEntity* entity, bool active)
{
    if (!active)
    {
        entity->pev->renderfx = kRenderFxNone;
        entity->pev->renderamt = 0;
        entity->pev->rendercolor.x = 0;
        entity->pev->rendercolor.y = 0;
        entity->pev->rendercolor.z = 0;
        return;
    }
    // entity->pev->effects |= EF_LIGHT;
    entity->pev->renderfx = kRenderFxGlowShell;
    entity->pev->renderamt = 0;
    switch (tfv.GetOwningTeam())
    {
    case TEAM_BLUE:
        entity->pev->rendercolor.x = 0;
        entity->pev->rendercolor.y = 0;
        entity->pev->rendercolor.z = 255;
        break;
    case TEAM_RED:
        entity->pev->rendercolor.x = 255;
        entity->pev->rendercolor.y = 0;
        entity->pev->rendercolor.z = 0;
        break;
    case TEAM_YELLOW:
        entity->pev->rendercolor.x = 255;
        entity->pev->rendercolor.y = 255;
        entity->pev->rendercolor.z = 0;
        break;
    case TEAM_GREEN:
        entity->pev->rendercolor.x = 0;
        entity->pev->rendercolor.y = 255;
        entity->pev->rendercolor.z = 0;
        break;
    default:
    case TEAM_UNASSIGNED:
        entity->pev->rendercolor.x = 255;
        entity->pev->rendercolor.y = 255;
        entity->pev->rendercolor.z = 255;
        break;
    }
}
