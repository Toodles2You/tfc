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
	auto pipebomb = Entity::Create<CPipeBomb>();

	pipebomb->v.origin = origin;
	pipebomb->v.angles = dir;
	pipebomb->v.dmg = damageMax;
	pipebomb->v.dmg_save = damageMin;
	pipebomb->v.dmg_take = radius;
    if (launcher != nullptr)
    {
        launcher->AddPipeBomb(pipebomb);
        pipebomb->v.skin = 0;
    }
    else
    {
        pipebomb->v.skin = 1;
    }
	pipebomb->v.owner = &owner->v;
	pipebomb->v.team = owner->TeamNumber();
	pipebomb->Spawn();

	return pipebomb;
}


bool CPipeBomb::Spawn()
{
	v.movetype = MOVETYPE_BOUNCE;

	if (v.skin != 0)
	{
		v.classname = MAKE_STRING("glgrenade");
		v.solid = SOLID_TRIGGER;
	}
	else
	{
		v.classname = MAKE_STRING("pipebomb");
		v.solid = SOLID_BBOX;
	}

	SetModel("models/pipebomb.mdl");

	SetSize(g_vecZero, g_vecZero);

	SetOrigin(v.origin);

	v.velocity = v.angles * 800;
	v.angles = util::VecToAngles(v.angles);
    v.avelocity = Vector(300, 300, 300);
    v.friction = 0.5F;

	SetTouch(&CPipeBomb::PipeBombTouch);

    if (v.skin != 0)
    {
        SetThink(&CPipeBomb::Detonate);
        v.nextthink = gpGlobals->time + 2.5F;
    }
    else
    {
        v.pain_finished = gpGlobals->time + 0.5F;
    }

	v.air_finished = gpGlobals->time;

	tent::RocketTrail(this, false);

	return true;
}


void CPipeBomb::PipeBombTouch(CBaseEntity* pOther)
{
	if (engine::PointContents(v.origin) != CONTENTS_SKY)
	{
		CBaseEntity* owner = this;

		if (v.owner != nullptr)
		{
			owner = v.owner->Get<CBaseEntity>();

			if (pOther == owner)
			{
				return;
			}
		}

        if (v.skin != 0 && pOther->v.takedamage != DAMAGE_NO)
        {
    		ExplodeTouch(pOther);
        }
        else
		{
			if (v.flags & FL_ONGROUND)
			{
				v.velocity = v.velocity * 0.75F;
				v.angles.x = v.angles.z = 0.0F;

				if (v.velocity.Length() <= 20.0F)
				{
					v.avelocity = g_vecZero;
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
