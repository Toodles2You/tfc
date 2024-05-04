//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: You want it? It's yours, my friend!
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"


CPipeBomb* CPipeBomb::CreatePipeBomb(
	const Vector& origin,
	const Vector& dir,
	const float damageMax,
	const float damageMin,
	const float radius,
	CBaseEntity* owner,
	CPipeBombLauncher* launcher)
{
	auto pipebomb = GetClassPtr((CPipeBomb*)nullptr);

	pipebomb->pev->origin = origin;
	pipebomb->pev->angles = dir;
	pipebomb->pev->dmg = damageMax;
	pipebomb->pev->dmg_save = damageMin;
	pipebomb->pev->dmg_take = radius;
    if (launcher != nullptr)
    {
        launcher->AddPipeBomb(pipebomb);
        pipebomb->pev->skin = 0;
    }
    else
    {
        pipebomb->pev->skin = 1;
    }
	pipebomb->pev->owner = owner->edict();
	pipebomb->pev->team = owner->TeamNumber();
	pipebomb->Spawn();

	return pipebomb;
}


bool CPipeBomb::Spawn()
{
	pev->movetype = MOVETYPE_BOUNCE;

	if (pev->skin != 0)
	{
		pev->classname = MAKE_STRING("glgrenade");
		pev->solid = SOLID_TRIGGER;
	}
	else
	{
		pev->classname = MAKE_STRING("pipebomb");
		pev->solid = SOLID_BBOX;
	}

	SetModel("models/pipebomb.mdl");

	SetSize(g_vecZero, g_vecZero);

	SetOrigin(pev->origin);

	pev->velocity = pev->angles * 800;
	pev->angles = util::VecToAngles(pev->angles);
    pev->avelocity = Vector(300, 300, 300);
    pev->friction = 0.5F;

	SetTouch(&CPipeBomb::PipeBombTouch);

    if (pev->skin != 0)
    {
        SetThink(&CPipeBomb::Detonate);
        pev->nextthink = gpGlobals->time + 2.5F;
    }
    else
    {
        pev->pain_finished = gpGlobals->time + 0.5F;
    }

	tent::RocketTrail(this, false);

	return true;
}


void CPipeBomb::PipeBombTouch(CBaseEntity* pOther)
{
	if (g_engfuncs.pfnPointContents(pev->origin) != CONTENTS_SKY)
	{
		CBaseEntity* owner = this;

		if (pev->owner != nullptr)
		{
			owner = CBaseEntity::Instance(pev->owner);

			if (pOther == owner)
			{
				return;
			}
		}

        if (pev->skin != 0 && pOther->pev->takedamage != DAMAGE_NO)
        {
    		ExplodeTouch(pOther);
        }
        else
		{
			if (pev->flags & FL_ONGROUND)
			{
				pev->velocity = pev->velocity * 0.75F;
				pev->angles.x = pev->angles.z = 0.0F;

				if (pev->velocity.Length() <= 20.0F)
				{
					pev->avelocity = g_vecZero;
				}
			}

			BounceTouch(pOther);
		}
	}
    else
    {
    	Remove();        
    }
}
