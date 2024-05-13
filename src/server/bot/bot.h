//========= Copyright ï¿½ 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BOT_H
#define BOT_H

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "weaponbox.h"

#include "bot_manager.h"
#include "bot_util.h"
#include "bot_constants.h"

extern float g_flBotCommandInterval;
extern float g_flBotFullThinkInterval;

extern CBotManager *g_pBotMan;

class BotProfile;


//--------------------------------------------------------------------------------------------------------
template <class T> T * CreateBot( const BotProfile *profile )
{
	Entity * pentBot;

	if ( UTIL_ClientsInGame() >= gpGlobals->maxClients )
	{
		CONSOLE_ECHO( "Unable to create bot: Server is full (%d/%d clients).\n", UTIL_ClientsInGame(), gpGlobals->maxClients );
		return nullptr;
	}

	char netname[64];

	UTIL_ConstructBotNetName(netname, 64, profile);

	pentBot = engine::CreateFakeClient( netname );

	if ( pentBot == nullptr )
	{
		CONSOLE_ECHO( "Unable to create bot: pfnCreateFakeClient() returned null.\n" );
		return nullptr;
	}
	else
	{
		T * pBot = nullptr;

		engine::FreeEntPrivateData(pentBot);

		pentBot->flags |= FL_FAKECLIENT;

		pBot = pentBot->GetNew<T>();

		// initialize the bot
		pBot->Initialize( profile );

		return pBot;
	}
}

//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
/**
 * The base bot class from which bots for specific games are derived
 */
class CBot : public CBasePlayer 
{
public:
	CBot( Entity* containingEntity );											///< constructor initializes all values to zero

	virtual bool Initialize( const BotProfile *profile );	///< (EXTEND) prepare bot for action

	unsigned int GetID( void ) const	{ return m_id; }	///< return bot's unique ID

	virtual bool IsBot( void ) override { return true; }	

	virtual void SpawnBot( void ) = 0;
	virtual void Upkeep( void ) = 0;						///< lightweight maintenance, invoked frequently
	virtual void Update( void ) = 0;						///< heavyweight algorithms, invoked less often


	virtual void Run( void );
	virtual void Walk( void );
	bool IsRunning( void ) const	{ return m_isRunning; }
	
	virtual void Crouch( void );
	virtual void StandUp( void );
	bool IsCrouching( void ) const	{ return m_isCrouching; }

	void PushPostureContext( void );						///< push the current posture context onto the top of the stack
	void PopPostureContext( void );							///< restore the posture context to the next context on the stack

	virtual void MoveForward( void );
	virtual void MoveBackward( void );
	virtual void StrafeLeft( void );
	virtual void StrafeRight( void );

	#define MUST_JUMP true
	virtual bool Jump( bool mustJump = false );				///< returns true if jump was started
	bool IsJumping( void );																						///< returns true if we are in the midst of a jump
	float GetJumpTimestamp( void ) const	{ return m_jumpTimestamp; }	///< return time last jump began

	virtual void ClearMovement( void );						///< zero any MoveForward(), Jump(), etc

	//------------------------------------------------------------------------------------
	// Weapon interface
	//
	virtual void UseEnvironment( void );
	virtual void PrimaryAttack( void );
	virtual void ClearPrimaryAttack( void );
	virtual void TogglePrimaryAttack( void );
	virtual void SecondaryAttack( void );
	virtual void Reload( void );

	float GetActiveWeaponAmmoRatio( void ) const;			///< returns ratio of ammo left to max ammo (1 = full clip, 0 = empty)
	bool IsActiveWeaponClipEmpty( void ) const;				///< return true if active weapon has any empty clip
	bool IsActiveWeaponOutOfAmmo( void ) const;				///< return true if active weapon has no ammo at all
	bool IsActiveWeaponReloading( void ) const;				///< is the weapon in the middle of a reload
	bool IsActiveWeaponRecoilHigh( void ) const;			///< return true if active weapon's bullet spray has become large and inaccurate
	CBasePlayerWeapon *GetActiveWeapon( void ) const;		///< return the weapon the bot is currently using
	bool IsUsingScope( void ) const;						///< return true if looking thru weapon's scope


	//------------------------------------------------------------------------------------
	// Event hooks
	//

	/// invoked when injured by something (EXTEND)
	virtual bool TakeDamage( CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType ) override
	{
		return CBasePlayer::TakeDamage( inflictor, attacker, flDamage, bitsDamageType );
	}

	/// invoked when killed (EXTEND)
	virtual void Killed( CBaseEntity* inflictor, CBaseEntity* attacker, int bitsDamageType ) override
	{ 
		CBasePlayer::Killed( inflictor, attacker, bitsDamageType );
	}

	virtual void OnTouchingWeapon( CWeaponBox *box ) { }	///< invoked when in contact with a CWeaponBox

	/// invoked when event occurs in the game (some events have nullptr entities)
	virtual void OnEvent( GameEventType event, CBaseEntity *entity = nullptr, CBaseEntity *other = nullptr ) { }

	//------------------------------------------------------------------------------------
	// Vision
	//

	enum VisiblePartType
	{
		NONE		= 0x00,
		CHEST		= 0x01,
		HEAD		= 0x02,
		LEFT_SIDE	= 0x04,			///< the left side of the object from our point of view (not their left side)
		RIGHT_SIDE	= 0x08,			///< the right side of the object from our point of view (not their right side)
		FEET		= 0x10
	};

	#define CHECK_FOV true
	virtual bool IsVisible( const Vector *pos, bool testFOV = false ) = 0;	///< return true if we can see the point
	virtual bool IsVisible( CBasePlayer *player, bool testFOV = false, unsigned char *visParts = nullptr ) = 0;	///< return true if we can see any part of the player

	virtual bool IsEnemyPartVisible( VisiblePartType part ) = 0;	///< if enemy is visible, return the part we see

	virtual bool IsPlayerFacingMe( CBasePlayer *enemy ) const;		///< return true if player is facing towards us
	virtual bool IsPlayerLookingAtMe( CBasePlayer *enemy ) const;	///< returns true if other player is pointing right at us


	//------------------------------------------------------------------------------------
	// Information query
	//

	bool IsEnemy( CBaseEntity *ent ) const;					///< returns TRUE if given entity is our enemy
	int GetEnemiesRemaining( void ) const;					///< return number of enemies left alive
	int GetFriendsRemaining( void ) const;					///< return number of friends left alive

	bool IsLocalPlayerWatchingMe( void ) const;				///< return true if local player is observing this bot

	void Print( char *format, ... ) const;					///< output message to console
	void PrintIfWatched( char *format, ... ) const;			///< output message to console if we are being watched by the local player

	virtual void ExecuteCommand( void );
	virtual void SetModel( const char *modelName );
	virtual Vector GetAimVector( void );

	bool Spawn( void ) override;
	void BotThink( void );
	bool IsNetClient( void ) override			{ return false; }
#ifdef HALFLIFE_SAVERESTORE
	int Save( CSave &save )	override			{ return 0; }
	int Restore( CRestore &restore ) override	{ return 0; }
#endif
	virtual void Think( void ) override { }

	const BotProfile *GetProfile( void ) const		{ return m_profile; }	///< return our personality profile

	virtual void CmdStart(const usercmd_t& cmd, unsigned int randomSeed) override;

protected:
	// Do a "client command" - useful for invoking menu choices, etc.
	void ClientCommand( const char *cmd, const char *arg1 = nullptr, const char *arg2 = nullptr, const char *arg3 = nullptr );

	const BotProfile *m_profile;							///< the "personality" profile of this bot

private:
	unsigned int m_id;										///< unique bot ID

	// Think mechanism variables
	float m_flNextBotThink;
	float m_flNextFullBotThink;

	// Command interface variables
	float m_flPreviousCommandTime;

	bool m_isRunning;										///< run/walk mode
	bool m_isCrouching;										///< true if crouching (ducking)
	float m_forwardSpeed;
	float m_strafeSpeed;
	float m_verticalSpeed;
	unsigned short m_buttonFlags;							///< bitfield of movement buttons

	float m_jumpTimestamp;									///< time when we last began a jump

	/// the PostureContext represents the current settings of walking and crouching
	struct PostureContext
	{
		bool isRunning;
		bool isCrouching;
	};
	enum { MAX_POSTURE_STACK = 8 };
	PostureContext m_postureStack[ MAX_POSTURE_STACK ];
	int m_postureStackIndex;								///< index of top of stack

	void ResetCommand( void );
	byte ThrottledMsec( void ) const;

	float GetMoveSpeed( void );								///< returns current movement speed (for walk/run)
};


//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------
//
// Inlines
//

//--------------------------------------------------------------------------------------------------------------
inline void CBot::SetModel( const char *modelName )
{
	SET_CLIENT_KEY_VALUE( entindex(), GET_USERINFO(edict()), "model", (char *)modelName );
}

//-----------------------------------------------------------------------------------------------------------
inline float CBot::GetMoveSpeed( void )
{
	if (m_isRunning || m_isCrouching)
		return v.maxspeed;

	// should be 0.52, but when bots strafe, they break the run/walk threshold
	return 0.4f * v.maxspeed;
}

//-----------------------------------------------------------------------------------------------------------
inline void CBot::Run( void )
{
	m_isRunning = true;
}

//-----------------------------------------------------------------------------------------------------------
inline void CBot::Walk( void )
{
	m_isRunning = false;
}

//-----------------------------------------------------------------------------------------------------------
inline CBasePlayerWeapon *CBot::GetActiveWeapon( void ) const
{
	return m_pActiveWeapon;
}

//-----------------------------------------------------------------------------------------------------------
inline bool CBot::IsActiveWeaponReloading( void ) const
{
	CBasePlayerWeapon *gun = GetActiveWeapon();
	if (gun == nullptr)
		return false;

	return gun->m_fInReload;
}

//-----------------------------------------------------------------------------------------------------------
inline bool CBot::IsActiveWeaponRecoilHigh( void ) const
{
	CBasePlayerWeapon *gun = GetActiveWeapon();
	if (gun == nullptr)
		return false;

	const float highRecoil = 0.4f;
#ifdef CSTRIKE
	return (gun->m_flAccuracy > highRecoil) ? true : false;
#else
	return false;
#endif
}

//-----------------------------------------------------------------------------------------------------------
inline void CBot::PushPostureContext( void )
{
	if (m_postureStackIndex == MAX_POSTURE_STACK)
	{
#if 0
		if (pev)
#endif
			PrintIfWatched( "PushPostureContext() overflow error!\n" );
		return;
	}

	m_postureStack[ m_postureStackIndex ].isRunning = m_isRunning;
	m_postureStack[ m_postureStackIndex ].isCrouching = m_isCrouching;
	++m_postureStackIndex;
}

//-----------------------------------------------------------------------------------------------------------
inline void CBot::PopPostureContext( void )
{
	if (m_postureStackIndex == 0)
	{
#if 0
		if (pev)
#endif
			PrintIfWatched( "PopPostureContext() underflow error!\n" );
		m_isRunning = true;
		m_isCrouching = false;
		return;
	}

	--m_postureStackIndex;
	m_isRunning = m_postureStack[ m_postureStackIndex ].isRunning;
	m_isCrouching = m_postureStack[ m_postureStackIndex ].isCrouching;
}

//-----------------------------------------------------------------------------------------------------------
inline bool CBot::IsPlayerFacingMe( CBasePlayer *other ) const
{
	Vector toOther = other->v.origin - v.origin;

	// compute the unit vector along our other player's
	util::MakeVectors( other->v.v_angle + other->v.punchangle );
	Vector otherDir = gpGlobals->v_forward;

	if (otherDir.x * toOther.x + otherDir.y * toOther.y < 0.0f)
		return true;

	return false;
}

//-----------------------------------------------------------------------------------------------------------
inline bool CBot::IsPlayerLookingAtMe( CBasePlayer *other ) const
{
	Vector toOther = other->v.origin - v.origin;
	toOther.NormalizeInPlace();

	// compute the unit vector along our other player's
	util::MakeVectors( other->v.v_angle + other->v.punchangle );
	Vector otherDir = gpGlobals->v_forward;

	// other player must be pointing nearly right at us to be "looking at" us
	const float lookAtCos = 0.9f;
	if (otherDir.x * toOther.x + otherDir.y * toOther.y < -lookAtCos)
	{
		// other player must have clear line of sight to us to be "looking at" us
		Vector vec(other->EyePosition());
		if (IsVisible( &vec ))
			return true;
	}

	return false;
}

inline void CBot::CmdStart(const usercmd_t& cmd, unsigned int randomSeed)
{
	CBasePlayer::CmdStart(cmd, randomSeed);
	m_randomSeed = engine::RandomLong(0, 255);
}


//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------

extern void InstallBotControl( void );
extern void Bot_ServerCommand( void );
extern void Bot_RegisterCvars( void );


#endif // BOT_H
