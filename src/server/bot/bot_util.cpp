//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// Author: Michael S. Booth (mike@turtlerockstudios.com), 2003

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"

#include "bot.h"
#include "bot_util.h"
#include "bot_profile.h"
#include "nav.h"

static short s_iBeamSprite = 0;

//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if given name is already in use by another player
 */
bool UTIL_IsNameTaken( const char *name, bool ignoreHumans )
{
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBaseEntity * player = util::PlayerByIndex( i );

		if (player == nullptr)
			continue;

		if (STRING(player->v.netname)[0] == '\0')
			continue;

		if (player->IsPlayer() && ((CBasePlayer *)player)->IsBot())
		{
			// bots can have prefixes so we need to check the name
			// against the profile name instead.
			CBot *bot = (CBot *)player;
			if (streq(name, bot->GetProfile()->GetName()))
			{
				return true;
			}
		}
		else
		{
			if (!ignoreHumans)
			{
				if (streq( name, STRING( player->v.netname ) ))
					return true;
			}
		}
	}

	return false;
}


//--------------------------------------------------------------------------------------------------------------
int UTIL_ClientsInGame( void )
{
	int iCount = 0;

	for ( int iIndex = 1; iIndex <= gpGlobals->maxClients; iIndex++ )
	{
		CBaseEntity * pPlayer = util::PlayerByIndex( iIndex );

		if ( pPlayer == nullptr )
			continue;

		if (STRING(pPlayer->v.netname)[0] == '\0')
			continue;

		iCount++;
	}

	return iCount;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return number of active players (not spectators) in the game
 */
int UTIL_ActivePlayersInGame( void )
{
	int iCount = 0;

	for (int iIndex = 1; iIndex <= gpGlobals->maxClients; iIndex++ )
	{
		CBaseEntity *entity = util::PlayerByIndex( iIndex );

		if ( entity == nullptr )
			continue;

		if (STRING(entity->v.netname)[0] == '\0')
			continue;

		CBasePlayer *player = static_cast<CBasePlayer *>( entity );

		// ignore spectators
		if (!player->IsPlayer())
			continue;

		iCount++;
	}

	return iCount;
}



//--------------------------------------------------------------------------------------------------------------
int UTIL_HumansInGame( bool ignoreSpectators )
{
	int iCount = 0;

	for (int iIndex = 1; iIndex <= gpGlobals->maxClients; iIndex++ )
	{
		CBaseEntity *entity = util::PlayerByIndex( iIndex );

		if ( entity == nullptr )
			continue;

		if (STRING(entity->v.netname)[0] == '\0')
			continue;

		CBasePlayer *player = static_cast<CBasePlayer *>( entity );

		if (player->IsBot())
			continue;

		if (ignoreSpectators && !player->IsPlayer())
			continue;

		iCount++;
	}

	/*
	if ( engine::IsDedicatedServer() && !ignoreSpectators )
	{
		// If we're counting humans, including spectators, don't count the dedicated server
		--iCount;
	}
	*/

	return iCount;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return the number of non-bots on the given team
 */
int UTIL_HumansOnTeam( int teamID, bool isAlive )
{
	int iCount = 0;

	for (int iIndex = 1; iIndex <= gpGlobals->maxClients; iIndex++ )
	{
		CBaseEntity *entity = util::PlayerByIndex( iIndex );

		if ( entity == nullptr )
			continue;

		if (STRING(entity->v.netname)[0] == '\0')
			continue;

		CBasePlayer *player = static_cast<CBasePlayer *>( entity );

		if (player->IsBot())
			continue;

		if (player->TeamNumber() != teamID)
			continue;

		if (isAlive && !player->IsAlive())
			continue;

		iCount++;
	}

	return iCount;
}


//--------------------------------------------------------------------------------------------------------------
int UTIL_BotsInGame( void )
{
	int iCount = 0;

	for (int iIndex = 1; iIndex <= gpGlobals->maxClients; iIndex++ )
	{
		CBasePlayer *pPlayer = static_cast<CBasePlayer *>(util::PlayerByIndex( iIndex ));

		if ( pPlayer == nullptr )
			continue;

		if (STRING(pPlayer->v.netname)[0] == '\0')
			continue;

		if ( !pPlayer->IsBot() )
			continue;

		iCount++;
	}

	return iCount;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Kick a bot from the given team. If no bot exists on the team, return false.
 */
bool UTIL_KickBotFromTeam( int kickTeam )
{
	int i;

	// try to kick a dead bot first
	for ( i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = static_cast<CBasePlayer *>( util::PlayerByIndex( i ) );

		if (player == nullptr)
			continue;

		const char *name = STRING( player->v.netname );
		if (streq( name, "" ))
			continue;

		if (!player->IsBot())
			continue;	

		if (!player->IsAlive() && (kickTeam == -1 || player->TeamNumber() == kickTeam))
		{
			// its a bot on the right team - kick it
			engine::ServerCommand( util::VarArgs( "kick \"%s\"\n", STRING( player->v.netname ) ) );

			return true;
		}
	}

	// no dead bots, kick any bot on the given team
	for ( i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = static_cast<CBasePlayer *>( util::PlayerByIndex( i ) );

		if (player == nullptr)
			continue;

		const char *name = STRING( player->v.netname );
		if (streq( name, "" ))
			continue;

		if (!player->IsBot())
			continue;	

		if ((kickTeam == -1 || player->TeamNumber() == kickTeam))
		{
			// its a bot on the right team - kick it
			engine::ServerCommand( util::VarArgs( "kick \"%s\"\n", STRING( player->v.netname ) ) );

			return true;
		}
	}

	return false;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if all of the members of the given team are bots
 */
bool UTIL_IsTeamAllBots( int team )
{
	int botCount = 0;

	for( int i=1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = static_cast<CBasePlayer *>( util::PlayerByIndex( i ) );

		if (player == nullptr)
			continue;

		// skip players on other teams
		if (player->TeamNumber() != team)
			continue;

		if (STRING(player->v.netname)[0] == '\0')
			continue;

		// if not a bot, fail the test
		if (!FBitSet( player->v.flags, FL_FAKECLIENT ))
			return false;

		// is a bot on given team
		++botCount;
	}

	// if team is empty, there are no bots
	return (botCount) ? true : false;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return the closest active player to the given position.
 * If 'distance' is non-nullptr, the distance to the closest player is returned in it.
 */
CBasePlayer *UTIL_GetClosestPlayer( const Vector *pos, float *distance, CBasePlayer *ignore )
{
	CBasePlayer *closePlayer = nullptr;
	float closeDistSq = 999999999999.9f;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *player = static_cast<CBasePlayer *>( util::PlayerByIndex( i ) );

		if (!IsEntityValid( player ))
			continue;

		if (player == ignore)
			continue;

		if (!player->IsAlive())
			continue;

		float distSq = (player->v.origin - *pos).LengthSquared();
		if (distSq < closeDistSq)
		{
			closeDistSq = distSq;
			closePlayer = static_cast<CBasePlayer *>( player );
		}
	}
	
	if (distance)
		*distance = sqrt( closeDistSq );

	return closePlayer;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return the closest active player on the given team to the given position.
 * If 'distance' is non-nullptr, the distance to the closest player is returned in it.
 */
CBasePlayer *UTIL_GetClosestPlayer( const Vector *pos, int team, float *distance, CBasePlayer *ignore )
{
	CBasePlayer *closePlayer = nullptr;
	float closeDistSq = 999999999999.9f;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *player = static_cast<CBasePlayer *>( util::PlayerByIndex( i ) );

		if (!IsEntityValid( player ))
			continue;

		if (player == ignore)
			continue;

		if (!player->IsAlive())
			continue;

		if (player->TeamNumber() != team)
			continue;

		float distSq = (player->v.origin - *pos).LengthSquared();
		if (distSq < closeDistSq)
		{
			closeDistSq = distSq;
			closePlayer = static_cast<CBasePlayer *>( player );
		}
	}
	
	if (distance)
		*distance = sqrt( closeDistSq );

	return closePlayer;
}

CBasePlayer *UTIL_GetClosestEnemyPlayer( CBasePlayer *self, float *distance )
{
	CBasePlayer *closePlayer = nullptr;
	float closeDistSq = 999999999999.9f;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *player = static_cast<CBasePlayer *>( util::PlayerByIndex( i ) );

		if (!IsEntityValid( player ))
			continue;

		if (player == self)
			continue;

		if (!player->IsAlive())
			continue;

		if (g_pGameRules->PlayerRelationship(self, player) > GR_NOTTEAMMATE)
			continue;

		float distSq = (player->v.origin - self->v.origin).LengthSquared();
		if (distSq < closeDistSq)
		{
			closeDistSq = distSq;
			closePlayer = static_cast<CBasePlayer *>( player );
		}
	}
	
	if (distance)
		*distance = sqrt( closeDistSq );

	return closePlayer;
}

//--------------------------------------------------------------------------------------------------------------
// returns the string to be used for the bot name prefix.
const char * UTIL_GetBotPrefix()
{
	return cv_bot_prefix.string;
}

//--------------------------------------------------------------------------------------------------------------
// Takes the bot pointer and constructs the net name using the current bot name prefix.
void UTIL_ConstructBotNetName(char *name, int nameLength, const BotProfile *profile)
{
	if (profile == nullptr)
	{
		name[0] = 0;
		return;
	}

	// if there is no bot prefix just use the profile name.
	if ((UTIL_GetBotPrefix() == nullptr) || (strlen(UTIL_GetBotPrefix()) == 0))
	{
		strncpy(name, profile->GetName(), nameLength);
		return;
	}

	snprintf(name, nameLength, "%s %s", UTIL_GetBotPrefix(), profile->GetName());
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if anyone on the given team can see the given spot
 */
bool UTIL_IsVisibleToTeam( const Vector &spot, int team, float maxRange )
{
	for( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = static_cast<CBasePlayer *>( util::PlayerByIndex( i ) );

		if (player == nullptr)
			continue;

		if (STRING(player->v.netname)[0] == '\0')
			continue;

		if (!player->IsAlive())
			continue;

		if (player->TeamNumber() != team)
			continue;

		if (maxRange > 0.0f && (spot - player->Center()) > maxRange)
			continue;

		TraceResult result;
		util::TraceLine( player->EyePosition(), spot, util::ignore_monsters, util::ignore_glass, player, &result );

		if (result.flFraction == 1.0f)
			return true;
	}

	return false;
}


//--------------------------------------------------------------------------------------------------------------


Vector UTIL_ComputeOrigin( Entity * pentEdict )
{
	if ( ( pentEdict->origin.x == 0.0 ) && ( pentEdict->origin.y == 0.0 ) && ( pentEdict->origin.z == 0.0 ) )
		return ( pentEdict->absmax + pentEdict->absmin ) * 0.5;
	else
		return pentEdict->origin;
}


Vector UTIL_ComputeOrigin( CBaseEntity * pEntity )
{
	return UTIL_ComputeOrigin( &pEntity->v );
}


//------------------------------------------------------------------------------------------------------------
void UTIL_DrawBeamFromEnt( int iIndex, Vector vecEnd, int iLifetime, byte bRed, byte bGreen, byte bBlue )
{
	MessageBegin( MSG_PVS, SVC_TEMPENTITY, vecEnd );   // vecEnd = origin???
									WriteByte( TE_BEAMENTPOINT );
									WriteShort( iIndex );
									WriteCoord( vecEnd );
									WriteShort( s_iBeamSprite );
									WriteByte( 0 );		 // startframe
									WriteByte( 0 );		 // framerate
									WriteByte( iLifetime ); // life
									WriteByte( 10 );		 // width
									WriteByte( 0 );		 // noise
									WriteByte( bRed );		 // r, g, b
									WriteByte( bGreen );		 // r, g, b
									WriteByte( bBlue );    // r, g, b
									WriteByte( 255 );	 // brightness
									WriteByte( 0 );		 // speed
									MessageEnd();
}


//------------------------------------------------------------------------------------------------------------
void UTIL_DrawBeamPoints( Vector vecStart, Vector vecEnd, int iLifetime, byte bRed, byte bGreen, byte bBlue )
{
	MessageBegin( MSG_PVS, SVC_TEMPENTITY, vecStart );
									WriteByte( TE_BEAMPOINTS );
									WriteCoord( vecStart );
									WriteCoord( vecEnd );
									WriteShort( s_iBeamSprite );
									WriteByte( 0 );		 // startframe
									WriteByte( 0 );		 // framerate
									WriteByte( iLifetime ); // life
									WriteByte( 10 );		 // width
									WriteByte( 0 );		 // noise
									WriteByte( bRed );		 // r, g, b
									WriteByte( bGreen );		 // r, g, b
									WriteByte( bBlue );    // r, g, b
									WriteByte( 255 );	 // brightness
									WriteByte( 0 );		 // speed
									MessageEnd();
}


//------------------------------------------------------------------------------------------------------------
void CONSOLE_ECHO( char * pszMsg, ... )
{
	va_list     argptr;
	static char szStr[1024];

	va_start( argptr, pszMsg );
	vsprintf( szStr, pszMsg, argptr );
	va_end( argptr );

	engine::ServerPrint( szStr );
}


//------------------------------------------------------------------------------------------------------------
void CONSOLE_ECHO_LOGGED( char * pszMsg, ... )
{
	va_list     argptr;
	static char szStr[1024];

	va_start( argptr, pszMsg );
	vsprintf( szStr, pszMsg, argptr );
	va_end( argptr );

	engine::ServerPrint( szStr );
	util::LogPrintf( szStr );
}


//------------------------------------------------------------------------------------------------------------
void BotPrecache( void )
{
	s_iBeamSprite = engine::PrecacheModel( "sprites/smoke.spr" );
	engine::PrecacheSound( "buttons/bell1.wav" );
	engine::PrecacheSound( "buttons/blip1.wav" );
	engine::PrecacheSound( "buttons/blip2.wav" );
	engine::PrecacheSound( "buttons/button11.wav" );
	engine::PrecacheSound( "buttons/latchunlocked2.wav" );
	engine::PrecacheSound( "buttons/lightswitch2.wav" );

#ifdef CSTRIKE
	engine::PrecacheSound( "ambience/quail1.wav" );

	/// @todo This is for the Tutor - move it somewhere sane
	engine::PrecacheSound( "events/tutor_msg.wav" );
	engine::PrecacheSound( "events/enemy_died.wav" );
	engine::PrecacheSound( "events/friend_died.wav" );

	/// @todo This is for the Career mode UI - move it somewhere sane
	engine::PrecacheSound( "events/task_complete.wav" );
#endif

#ifdef TERRORSTRIKE
	/// @todo Zombie mode experiment
	engine::PrecacheSound( "zombie/attack1.wav" );
	engine::PrecacheSound( "zombie/attack2.wav" );
	engine::PrecacheSound( "zombie/attack3.wav" );
	engine::PrecacheSound( "zombie/attack4.wav" );
	engine::PrecacheSound( "zombie/attack5.wav" );
	engine::PrecacheSound( "zombie/bark1.wav" );
	engine::PrecacheSound( "zombie/bark2.wav" );
	engine::PrecacheSound( "zombie/bark3.wav" );
	engine::PrecacheSound( "zombie/bark4.wav" );
	engine::PrecacheSound( "zombie/bark5.wav" );
	engine::PrecacheSound( "zombie/bark6.wav" );
	engine::PrecacheSound( "zombie/bark7.wav" );
	engine::PrecacheSound( "zombie/breathing1.wav" );
	engine::PrecacheSound( "zombie/breathing2.wav" );
	engine::PrecacheSound( "zombie/breathing3.wav" );
	engine::PrecacheSound( "zombie/breathing4.wav" );
	engine::PrecacheSound( "zombie/groan1.wav" );
	engine::PrecacheSound( "zombie/groan2.wav" );
	engine::PrecacheSound( "zombie/groan3.wav" );
	engine::PrecacheSound( "zombie/hiss1.wav" );
	engine::PrecacheSound( "zombie/hiss2.wav" );
	engine::PrecacheSound( "zombie/hiss3.wav" );
	engine::PrecacheSound( "ambience/the_horror2.wav" );
	engine::PrecacheSound( "scientist/scream20.wav" );
	engine::PrecacheSound( "zombie/human_hurt1.wav" );
	engine::PrecacheSound( "zombie/human_hurt2.wav" );
	engine::PrecacheSound( "zombie/human_hurt3.wav" );
	engine::PrecacheSound( "zombie/human_hurt4.wav" );
	engine::PrecacheSound( "zombie/shout_reloading1.wav" );
	engine::PrecacheSound( "zombie/shout_reloading2.wav" );
	engine::PrecacheSound( "zombie/shout_reloading3.wav" );
	engine::PrecacheSound( "zombie/deep_heartbeat.wav" );
	engine::PrecacheSound( "zombie/deep_heartbeat_fast.wav" );
	engine::PrecacheSound( "zombie/deep_heartbeat_very_fast.wav" );
	engine::PrecacheSound( "zombie/deep_heartbeat_stopping.wav" );
	engine::PrecacheSound( "zombie/zombie_step1.wav" );
	engine::PrecacheSound( "zombie/zombie_step2.wav" );
	engine::PrecacheSound( "zombie/zombie_step3.wav" );
	engine::PrecacheSound( "zombie/zombie_step4.wav" );
	engine::PrecacheSound( "zombie/zombie_step5.wav" );
	engine::PrecacheSound( "zombie/zombie_step6.wav" );
	engine::PrecacheSound( "zombie/zombie_step7.wav" );
	engine::PrecacheSound( "zombie/fear1.wav" );
	engine::PrecacheSound( "zombie/fear2.wav" );
	engine::PrecacheSound( "zombie/fear3.wav" );
	engine::PrecacheSound( "zombie/fear4.wav" );
#endif // TERRORSTRIKE
}

//------------------------------------------------------------------------------------------------------------
#define COS_TABLE_SIZE 256
static float cosTable[ COS_TABLE_SIZE ];

void InitBotTrig( void )
{
	for( int i=0; i<COS_TABLE_SIZE; ++i )
	{
		float angle = 2.0f * M_PI * (float)i / (float)(COS_TABLE_SIZE-1);
		cosTable[i] = cos(angle); 
	}
}

float BotCOS( float angle )
{
	angle = NormalizeAnglePositive( angle );
	int i = angle * (COS_TABLE_SIZE-1) / 360.0f;
	return cosTable[i];
}

float BotSIN( float angle )
{
	angle = NormalizeAnglePositive( angle - 90 );
	int i = angle * (COS_TABLE_SIZE-1) / 360.0f;
	return cosTable[i];
}


//------------------------------------------------------------------------------------------------------------
/**
 * Determine if this event is audible, and if so, return its audible range and priority
 */
bool IsGameEventAudible( GameEventType event, CBaseEntity *entity, CBaseEntity *other, float *range, PriorityType *priority, bool *isHostile )
{
	CBasePlayer *player = static_cast<CBasePlayer *>( entity );
	if (entity == nullptr || !player->IsPlayer())
		player = nullptr;

	const float ShortRange = 1000.0f;
	const float NormalRange = 2000.0f;
	switch( event )
	{
		/// @todo Check weapon type (knives are pretty quiet)
		/// @todo Use actual volume, account for silencers, etc.
		case EVENT_WEAPON_FIRED:
		{
			if (player->m_pActiveWeapon == nullptr)
				return false;

#if 0
			switch( player->m_pActiveWeapon->GetID() )
			{
				// silent "firing"
				case WEAPON_HEGRENADE:
				case WEAPON_SMOKEGRENADE:
				case WEAPON_FLASHBANG:
				case WEAPON_SHIELDGUN:
				case WEAPON_C4:
					return false;

				// quiet
				case WEAPON_KNIFE:
				case WEAPON_TMP:
					*range = ShortRange;
					break;

				// M4A1 - check for silencer
				case WEAPON_M4A1:
					{
						CBasePlayerWeapon *pWeapon = static_cast<CBasePlayerWeapon *>(player->m_pActiveWeapon);
						if ( pWeapon->m_iWeaponState & WPNSTATE_M4A1_SILENCER_ON )
						{
							*range = ShortRange;
						}
						else
						{
							*range = NormalRange;
						}
					}
					break;

				// USP - check for silencer
				case WEAPON_USP:
					{
						CBasePlayerWeapon *pWeapon = static_cast<CBasePlayerWeapon *>(player->m_pActiveWeapon);
						if ( pWeapon->m_iWeaponState & WPNSTATE_USP_SILENCER_ON )
						{
							*range = ShortRange;
						}
						else
						{
							*range = NormalRange;
						}
					}
					break;

				// loud
				case WEAPON_AWP:
					*range = 99999.0f;
					break;

				// normal
				default:
					*range = NormalRange;
					break;
			}
#endif

			*range = NormalRange;
			*priority = PRIORITY_HIGH;
			*isHostile = true;
			return true;
		}

		case EVENT_HE_GRENADE_EXPLODED:
			*range = 99999.0f;
			*priority = PRIORITY_HIGH;
			*isHostile = true;
			return true;

		case EVENT_FLASHBANG_GRENADE_EXPLODED:
			*range = 1000.0f;
			*priority = PRIORITY_LOW;
			*isHostile = true;
			return true;

		case EVENT_SMOKE_GRENADE_EXPLODED:
			*range = 1000.0f;
			*priority = PRIORITY_LOW;
			*isHostile = true;
			return true;

		case EVENT_GRENADE_BOUNCED:
			*range = 500.0f;
			*priority = PRIORITY_LOW;
			*isHostile = true;
			return true;

		case EVENT_BREAK_GLASS:
		case EVENT_BREAK_WOOD:
		case EVENT_BREAK_METAL:
		case EVENT_BREAK_FLESH:
		case EVENT_BREAK_CONCRETE:
			*range = 1100.0f;
			*priority = PRIORITY_MEDIUM;
			*isHostile = true;
			return true;

		case EVENT_DOOR:
			*range = 1100.0f;
			*priority = PRIORITY_MEDIUM;
			*isHostile = false;
			return true;

		case EVENT_WEAPON_FIRED_ON_EMPTY:
		case EVENT_PLAYER_FOOTSTEP:
		case EVENT_WEAPON_RELOADED:
		case EVENT_WEAPON_ZOOMED:
		case EVENT_PLAYER_LANDED_FROM_HEIGHT:
			*range = 1100.0f;
			*priority = PRIORITY_LOW;
			*isHostile = false;
			return true;

		case EVENT_HOSTAGE_USED:
		case EVENT_HOSTAGE_CALLED_FOR_HELP:
			*range = 1200.0f;
			*priority = PRIORITY_MEDIUM;
			*isHostile = false;
			return true;
	}

	return false;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Send a "hint" message to all players, dead or alive.
 */
void HintMessageToAllPlayers( const char *message )
{
	util::hudtextparms_t textParms;

	textParms.x = -1.0f;
	textParms.y = -1.0f;
	textParms.fadeinTime = 1.0f;
	textParms.fadeoutTime = 5.0f;
	textParms.holdTime = 5.0f;
	textParms.fxTime = 0.0f;
	textParms.r1 = 100;
	textParms.g1 = 255;
	textParms.b1 = 100;
	textParms.r2 = 255;
	textParms.g2 = 255;
	textParms.b2 = 255;
	textParms.effect = 0;
	textParms.channel = 0;

	util::HudMessageAll( textParms, message );
}

