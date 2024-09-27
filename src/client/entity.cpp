// Client side entity management functions

#include <memory.h>

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "studio_event.h" // def. of mstudioevent_t
#include "r_efx.h"
#include "event_api.h"
#include "pm_defs.h"
#include "pmtrace.h"
#include "pm_shared.h"
#include "Exports.h"
#include "view.h"
#include "eventscripts.h"

#include "particleman.h"
extern IParticleMan* g_pParticleMan;

void Game_AddObjects();

extern Vector v_origin;

extern int g_iObserverMode;
extern int g_iObserverTarget;
extern int g_iObserverTarget2;

int gTempEntCount;

/*
=========================
HUD_TxferLocalOverrides

The server sends us our origin with extra precision as part of the clientdata structure, not during the normal
playerstate update in entity_state_t.  In order for these overrides to eventually get to the appropriate playerstate
structure, we need to copy them into the state structure at this point.
=========================
*/
void HUD_TxferLocalOverrides(struct entity_state_s* state, const struct clientdata_s* client)
{
	state->origin = client->origin;

	state->iuser1 = client->iuser1;
	state->iuser2 = client->iuser2;
	state->iuser3 = client->iuser3;
	state->iuser4 = client->iuser4;

	state->health = static_cast<int>(client->health);
	state->fuser1 = static_cast<int>(client->vuser4.z);
}

/*
=========================
HUD_TxferPredictionData

Because we can predict an arbitrary number of frames before the server responds with an update, we need to be able to copy client side prediction data in
 from the state that the server ack'd receiving, which can be anywhere along the predicted frame path ( i.e., we could predict 20 frames into the future and the server ack's
 up through 10 of those frames, so we need to copy persistent client-side only state from the 10th predicted frame to the slot the server
 update is occupying.
=========================
*/
void HUD_TxferPredictionData(struct entity_state_s* ps, const struct entity_state_s* pps, struct clientdata_s* pcd, const struct clientdata_s* ppcd, struct weapon_data_s* wd, const struct weapon_data_s* pwd)
{
	ps->oldbuttons = pps->oldbuttons;
	ps->flFallVelocity = pps->flFallVelocity;
	ps->iStepLeft = pps->iStepLeft;
	ps->playerclass = pps->playerclass;

	ps->frame = pps->frame;
	ps->sequence = pps->sequence;
	ps->framerate = pps->framerate;
	ps->gaitsequence = pps->gaitsequence;

	pcd->viewmodel = ppcd->viewmodel;
	pcd->m_iId = ppcd->m_iId;
	pcd->ammo_shells = ppcd->ammo_shells;
	pcd->ammo_nails = ppcd->ammo_nails;
	pcd->ammo_cells = ppcd->ammo_cells;
	pcd->ammo_rockets = ppcd->ammo_rockets;
	pcd->m_flNextAttack = ppcd->m_flNextAttack;
	pcd->fov = ppcd->fov;
	pcd->weaponanim = ppcd->weaponanim;
	pcd->tfstate = ppcd->tfstate;
	pcd->maxspeed = ppcd->maxspeed;

	pcd->deadflag = ppcd->deadflag;

	pcd->iuser1 = ppcd->iuser1;
	pcd->iuser2 = ppcd->iuser2;
	pcd->iuser3 = ppcd->iuser3;
	pcd->iuser4 = ppcd->iuser4;

	if (0 != client::IsSpectateOnly())
	{
		// in specator mode we tell the engine who we want to spectate and how
		// iuser3 is not used for duck prevention (since the spectator can't duck at all)
		pcd->iuser1 = g_iObserverMode; // observer mode
		pcd->iuser2 = g_iObserverTarget; // first target
		pcd->iuser3 = g_iObserverTarget2; // second target
	}

	memcpy(wd, pwd, WEAPON_TYPES * sizeof(weapon_data_t));
}

/*
=========================
HUD_CreateEntities
	
Gives us a chance to add additional entities to the render this frame
=========================
*/
void HUD_CreateEntities()
{
	// Add in any game specific objects
	Game_AddObjects();

	GetClientVoiceMgr()->CreateEntities();
}


/*
=========================
HUD_StudioEvent

The entity's studio model description indicated an event was
fired during this frame, handle the event by it's tag ( e.g., muzzleflash, sound )
=========================
*/
void HUD_StudioEvent(const struct mstudioevent_s* event, const struct cl_entity_s* entity)
{
	switch (event->event)
	{
	case 5001:
		client::efx::MuzzleFlash(const_cast<cl_entity_t*>(entity)->attachment[0], atoi(event->options));
		break;
	case 5011:
		client::efx::MuzzleFlash(const_cast<cl_entity_t*>(entity)->attachment[1], atoi(event->options));
		break;
	case 5021:
		client::efx::MuzzleFlash(const_cast<cl_entity_t*>(entity)->attachment[2], atoi(event->options));
		break;
	case 5031:
		client::efx::MuzzleFlash(const_cast<cl_entity_t*>(entity)->attachment[3], atoi(event->options));
		break;
	case 5002:
		client::efx::SparkEffect(const_cast<cl_entity_t*>(entity)->attachment[0], atoi(event->options), -100, 100);
		break;
	// Client side sound
	case 5004:
		client::PlaySoundByNameAtLocation((char*)event->options, 1.0, const_cast<cl_entity_t*>(entity)->attachment[0]);
		break;
	default:
		break;
	}
}

/*
=================
CL_UpdateTEnts

Simulation and cleanup of temporary entities
=================
*/
void HUD_TempEntUpdate(
	double frametime,			  // Simulation time
	double client_time,			  // Absolute time on client
	double cl_gravity,			  // True gravity on client
	TEMPENTITY** ppTempEntFree,	  // List of freed temporary ents
	TEMPENTITY** ppTempEntActive, // List
	int (*Callback_AddVisibleEntity)(cl_entity_t* pEntity),
	void (*Callback_TempEntPlaySound)(TEMPENTITY* pTemp, float damp))
{
	static int gTempEntFrame = 0;
	int i;
	TEMPENTITY *pTemp, *pnext, *pprev;
	float freq, gravity, gravitySlow, life, fastFreq;
	int hull;

	Vector vAngles;

	client::GetViewAngles(vAngles);

	if (g_pParticleMan)
		g_pParticleMan->SetVariables(cl_gravity, vAngles);

	// Nothing to simulate
	if (!*ppTempEntActive)
		return;

	// in order to have tents collide with players, we have to run the player prediction code so
	// that the client has the player list. We run this code once when we detect any COLLIDEALL
	// tent, then set this bool to true so the code doesn't get run again if there's more than
	// one COLLIDEALL ent for this update. (often are).

	EV_TracePush(-1);

	// !!!BUGBUG	-- This needs to be time based
	gTempEntFrame = (gTempEntFrame + 1) & 31;

	pTemp = *ppTempEntActive;

	// !!! Don't simulate while paused....  This is sort of a hack, revisit.
	if (frametime <= 0)
	{
		while (pTemp)
		{
			if ((pTemp->flags & FTENT_NOMODEL) == 0)
			{
				Callback_AddVisibleEntity(&pTemp->entity);
			}
			pTemp = pTemp->next;
		}
		goto finish;
	}

	pprev = nullptr;
	freq = client_time * 0.01;
	fastFreq = client_time * 5.5;
	gravity = -frametime * cl_gravity;
	gravitySlow = gravity * 0.5;

	gTempEntCount = 0;

	while (pTemp)
	{
		bool active;

		active = true;

		life = pTemp->die - client_time;
		pnext = pTemp->next;
		if (life < 0)
		{
			if ((pTemp->flags & FTENT_FADEOUT) != 0)
			{
				if (pTemp->entity.curstate.rendermode == kRenderNormal)
				{
					pTemp->entity.curstate.rendermode = kRenderTransTexture;
					pTemp->entity.baseline.renderamt = 255;
				}
				pTemp->entity.curstate.renderamt = pTemp->entity.baseline.renderamt * (1 + life * pTemp->fadeSpeed);
				if (pTemp->entity.curstate.renderamt <= 0)
					active = false;
			}
			else
				active = false;
		}
		if (!active) // Kill it
		{
			pTemp->next = *ppTempEntFree;
			*ppTempEntFree = pTemp;
			if (!pprev) // Deleting at head of list
				*ppTempEntActive = pnext;
			else
				pprev->next = pnext;
		}
		else
		{
			gTempEntCount++;

			pprev = pTemp;

			hull = (pTemp->entity.curstate.renderfx == kRenderFxDeadPlayer) ? kHullPlayer : kHullPoint;

			pTemp->entity.prevstate.origin = pTemp->entity.origin;

			if ((pTemp->flags & FTENT_SPARKSHOWER) != 0)
			{
				// Adjust speed if it's time
				// Scale is next think time
				if (client_time > pTemp->entity.baseline.scale)
				{
					// Show Sparks
					client::efx::SparkEffect(pTemp->entity.origin, 8, -200, 200);

					// Reduce life
					pTemp->entity.baseline.framerate -= 0.1;

					if (pTemp->entity.baseline.framerate <= 0.0)
					{
						pTemp->die = client_time;
						gTempEntCount--;
					}
					else
					{
						// So it will die no matter what
						pTemp->die = client_time + 0.5;

						// Next think
						pTemp->entity.baseline.scale = client_time + 0.1;
					}
				}
			}
			else if ((pTemp->flags & FTENT_PLYRATTACHMENT) != 0)
			{
				cl_entity_t* pClient;

				pClient = client::GetEntityByIndex(pTemp->clientIndex);

				pTemp->entity.origin = pClient->origin + pTemp->tentOffset;
			}
			else if ((pTemp->flags & FTENT_SINEWAVE) != 0)
			{
				pTemp->x += pTemp->entity.baseline.origin[0] * frametime;
				pTemp->y += pTemp->entity.baseline.origin[1] * frametime;

				pTemp->entity.origin[0] = pTemp->x + sin(pTemp->entity.baseline.origin[2] + client_time * pTemp->entity.prevstate.frame) * (10 * pTemp->entity.curstate.framerate);
				pTemp->entity.origin[1] = pTemp->y + sin(pTemp->entity.baseline.origin[2] + fastFreq + 0.7) * (8 * pTemp->entity.curstate.framerate);
				pTemp->entity.origin[2] += pTemp->entity.baseline.origin[2] * frametime;
			}
			else if ((pTemp->flags & FTENT_SPIRAL) != 0)
			{
				float s, c;
				s = sin(pTemp->entity.baseline.origin[2] + fastFreq);
				c = cos(pTemp->entity.baseline.origin[2] + fastFreq);

				pTemp->entity.origin[0] += pTemp->entity.baseline.origin[0] * frametime + 8 * sin(client_time * 20 + (int)pTemp);
				pTemp->entity.origin[1] += pTemp->entity.baseline.origin[1] * frametime + 4 * sin(client_time * 30 + (int)pTemp);
				pTemp->entity.origin[2] += pTemp->entity.baseline.origin[2] * frametime;
			}

			else
			{
				for (i = 0; i < 3; i++)
					pTemp->entity.origin[i] += pTemp->entity.baseline.origin[i] * frametime;
			}

			if (pTemp->entity.curstate.renderfx == kRenderFxDeadPlayer)
			{
				pTemp->entity.curstate.frame += 255 * frametime * pTemp->entity.curstate.framerate;
			}
			else if ((pTemp->flags & FTENT_SPRANIMATE) != 0)
			{
				pTemp->entity.curstate.frame += frametime * pTemp->entity.curstate.framerate;
				if (pTemp->entity.curstate.frame >= pTemp->frameMax)
				{
					pTemp->entity.curstate.frame = pTemp->entity.curstate.frame - (int)(pTemp->entity.curstate.frame);

					if ((pTemp->flags & FTENT_SPRANIMATELOOP) == 0)
					{
						// this animating sprite isn't set to loop, so destroy it.
						pTemp->die = client_time;
						pTemp = pnext;
						gTempEntCount--;
						continue;
					}
				}
			}
			else if ((pTemp->flags & FTENT_SPRCYCLE) != 0)
			{
				pTemp->entity.curstate.frame += frametime * 10;
				if (pTemp->entity.curstate.frame >= pTemp->frameMax)
				{
					pTemp->entity.curstate.frame = pTemp->entity.curstate.frame - (int)(pTemp->entity.curstate.frame);
				}
			}
// Experiment
#if 0
			if ( pTemp->flags & FTENT_SCALE )
				pTemp->entity.curstate.framerate += 20.0 * (frametime / pTemp->entity.curstate.framerate);
#endif

			if ((pTemp->flags & FTENT_ROTATE) != 0)
			{
				pTemp->entity.angles[0] += pTemp->entity.baseline.angles[0] * frametime;
				pTemp->entity.angles[1] += pTemp->entity.baseline.angles[1] * frametime;
				pTemp->entity.angles[2] += pTemp->entity.baseline.angles[2] * frametime;

				pTemp->entity.latched.prevangles = pTemp->entity.angles;
			}

			if ((pTemp->flags & (FTENT_COLLIDEALL | FTENT_COLLIDEWORLD | FTENT_COLLIDENONCLIENTS)) != 0)
			{
				Vector traceNormal;
				float traceFraction = 1;

				if ((pTemp->flags & (FTENT_COLLIDEALL | FTENT_COLLIDENONCLIENTS)) != 0)
				{
					pmtrace_t pmtrace;
					physent_t* pe;

					client::event::SetTraceHull(hull);

					EV_TraceLine(pTemp->entity.prevstate.origin, pTemp->entity.origin,
						((pTemp->flags & FTENT_COLLIDENONCLIENTS) != 0) ? PM_STUDIO_IGNORE : PM_NORMAL, pTemp->clientIndex, pmtrace);

					if (pmtrace.fraction != 1)
					{
						traceFraction = pmtrace.fraction;
						traceNormal = pmtrace.plane.normal;

						if (pTemp->hitcallback)
						{
							(*pTemp->hitcallback)(pTemp, &pmtrace);
						}
					}
				}
				else if ((pTemp->flags & FTENT_COLLIDEWORLD) != 0)
				{
					pmtrace_t pmtrace;

					client::event::SetTraceHull(hull);

					EV_TraceLine(pTemp->entity.prevstate.origin, pTemp->entity.origin,
						PM_WORLD_ONLY, pTemp->clientIndex, pmtrace);

					if (pmtrace.fraction != 1)
					{
						traceFraction = pmtrace.fraction;
						traceNormal = pmtrace.plane.normal;

						if ((pTemp->flags & FTENT_SPARKSHOWER) != 0)
						{
							// Chop spark speeds a bit more
							//
							pTemp->entity.baseline.origin = pTemp->entity.baseline.origin * 0.6F;

							if (pTemp->entity.baseline.origin.Length() < 10)
							{
								pTemp->entity.baseline.framerate = 0.0;
							}
						}

						if (pTemp->hitcallback)
						{
							(*pTemp->hitcallback)(pTemp, &pmtrace);
						}
					}
				}

				if (traceFraction != 1) // Decent collision now, and damping works
				{
					float proj, damp;

					// Place at contact point
					pTemp->entity.origin = pTemp->entity.prevstate.origin + traceFraction * frametime * pTemp->entity.baseline.origin;
					// Damp velocity
					damp = pTemp->bounceFactor;
					if ((pTemp->flags & (FTENT_GRAVITY | FTENT_SLOWGRAVITY)) != 0)
					{
						damp *= 0.5;
						if (traceNormal[2] > 0.9) // Hit floor?
						{
							if (pTemp->entity.baseline.origin[2] <= 0 && pTemp->entity.baseline.origin[2] >= gravity * 3)
							{
								damp = 0; // Stop
								pTemp->flags &= ~(FTENT_ROTATE | FTENT_GRAVITY | FTENT_SLOWGRAVITY | FTENT_COLLIDEWORLD | FTENT_SMOKETRAIL);
								pTemp->entity.angles[0] = 0;
								pTemp->entity.angles[2] = 0;
							}
						}
					}

					if ((pTemp->hitSound) != 0)
					{
						Callback_TempEntPlaySound(pTemp, damp);
					}

					if ((pTemp->flags & FTENT_COLLIDEKILL) != 0)
					{
						// die on impact
						pTemp->flags &= ~FTENT_FADEOUT;
						pTemp->die = client_time;
						gTempEntCount--;
					}
					else if (pTemp->entity.curstate.renderfx != kRenderFxDeadPlayer)
					{
						// Reflect velocity
						if (damp != 0)
						{
							proj = DotProduct(pTemp->entity.baseline.origin, traceNormal);
							pTemp->entity.baseline.origin = pTemp->entity.baseline.origin + -proj * 2 * traceNormal;
							// Reflect rotation (fake)

							pTemp->entity.angles[1] = -pTemp->entity.angles[1];
						}

						if (damp != 1)
						{
							pTemp->entity.baseline.origin = pTemp->entity.baseline.origin * damp;
							pTemp->entity.angles = pTemp->entity.angles * 0.9F;
						}
					}
				}
			}


			if ((pTemp->flags & FTENT_FLICKER) != 0 && gTempEntFrame == pTemp->entity.curstate.effects)
			{
				dlight_t* dl = client::efx::AllocDlight(0);
				dl->origin = pTemp->entity.origin;
				dl->radius = 60;
				dl->color.r = 255;
				dl->color.g = 120;
				dl->color.b = 0;
				dl->die = client_time + 0.01;
			}

			if ((pTemp->flags & FTENT_SMOKETRAIL) != 0)
			{
				auto sequence = 2;
				if (pTemp->entity.baseline.sequence == 69)
				{
					sequence = 1;
				}
				else if (pTemp->entity.baseline.sequence == 70)
				{
					sequence = 0;
				}
				client::efx::RocketTrail(pTemp->entity.prevstate.origin, pTemp->entity.origin, sequence);
			}

			if ((pTemp->flags & FTENT_GRAVITY) != 0)
			{
				pTemp->entity.baseline.origin[2] += gravity;
			}
			else if ((pTemp->flags & FTENT_SLOWGRAVITY) != 0)
			{
				pTemp->entity.baseline.origin[2] += gravitySlow;
			}
			else if (pTemp->entity.baseline.gravity != 0.0F)
			{
				pTemp->entity.baseline.origin[2] += -frametime * pTemp->entity.baseline.gravity;
			}

			if ((pTemp->flags & FTENT_CLIENTCUSTOM) != 0)
			{
				if (pTemp->callback)
				{
					(*pTemp->callback)(pTemp, frametime, client_time);
				}
			}

			// Cull to PVS (not frustum cull, just PVS)
			if ((pTemp->flags & FTENT_NOMODEL) == 0)
			{
				if (0 == Callback_AddVisibleEntity(&pTemp->entity))
				{
					if ((pTemp->flags & FTENT_PERSIST) == 0)
					{
						pTemp->die = client_time;		// If we can't draw it this frame, just dump it.
						pTemp->flags &= ~FTENT_FADEOUT; // Don't fade out, just die
						gTempEntCount--;
					}
				}
			}
		}
		pTemp = pnext;
	}

finish:
	// Restore state info
	EV_TracePop();
}

/*
=================
HUD_GetUserEntity

If you specify negative numbers for beam start and end point entities, then
  the engine will call back into this function requesting a pointer to a cl_entity_t 
  object that describes the entity to attach the beam onto.

Indices must start at 1, not zero.
=================
*/
cl_entity_t* HUD_GetUserEntity(int index)
{
	return nullptr;
}
