//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#pragma once

#define MAX_ENT_LEAFS 48

#include "progdefs.h"
#ifdef GAME_DLL
#include "enginecallback.h"
#endif

class Entity
{
public:
	qboolean free;
	int serialnumber;
	link_t area; // linked to a division node or leaf

	int headnode; // -1 to use normal leaf check
	int num_leafs;
	short leafnums[MAX_ENT_LEAFS];

	float freetime; // sv.time when the object was freed

	void* pvPrivateData; // Alloced and freed by engine, used by DLLs

public:
	string_t classname;
	string_t globalname;

	Vector origin;
	Vector oldorigin;
	Vector velocity;
	Vector basevelocity;
	Vector clbasevelocity; // Base velocity that was passed in to server physics so
						   //  client can predict conveyors correctly.  Server zeroes it, so we need to store here, too.
	Vector movedir;

	Vector angles;	   // Model angles
	Vector avelocity;  // angle velocity (degrees per second)
	Vector punchangle; // auto-decaying view angle adjustment
	Vector v_angle;	   // Viewing angle (player only)

	// For parametric entities
	Vector endpos;
	Vector startpos;
	float impacttime;
	float starttime;

	int fixangle; // 0:nothing, 1:force view angles, 2:add avelocity
	float idealpitch;
	float pitch_speed;
	float ideal_yaw;
	float yaw_speed;

	int modelindex;
	string_t model;

	int viewmodel;	 // player's viewmodel
	int weaponmodel; // what other players see

	Vector absmin; // BB max translated to world coord
	Vector absmax; // BB max translated to world coord
	Vector mins;   // local BB min
	Vector maxs;   // local BB max
	Vector size;   // maxs - mins

	float ltime;
	float nextthink;

	int movetype;
	int solid;

	int skin;
	int body; // sub-model selection for studiomodels
	int effects;

	float gravity;	// % of "normal" gravity
	float friction; // inverse elasticity of MOVETYPE_BOUNCE

	int light_level;

	int sequence;							// animation sequence
	int gaitsequence;						// movement animation sequence for player (0 for none)
	float frame;							// % playback position in animation sequences (0..255)
	float animtime;							// world time when frame was set
	float framerate;						// animation playback rate (-8x to 8x)
	byte controller[NUM_ENT_CONTROLLERS];	// bone controller setting (0..255)
	byte blending[NUM_ENT_BLENDERS];		// blending amount between sub-sequences (0..255)

	float scale; // sprite rendering scale (0..255)

	int rendermode;
	float renderamt;
	Vector rendercolor;
	int renderfx;

	float health;
	float frags;
	int weapons; // bit mask for available weapons
	float takedamage;

	int deadflag;
	Vector view_ofs; // eye position

	int button;
	int impulse;

	Entity* chain; // Entity pointer when linked into a linked list
	Entity* dmg_inflictor;
	Entity* enemy;
	Entity* aiment; // entity pointer when MOVETYPE_FOLLOW
	Entity* owner;
	Entity* groundentity;

	int spawnflags;
	int flags;

	int colormap; // lowbyte topcolor, highbyte bottomcolor
	int team;

	float max_health;
	float teleport_time;
	float armortype;
	float armorvalue;
	int waterlevel;
	int watertype;

	string_t target;
	string_t targetname;
	string_t netname;
	string_t message;

	float dmg_take;
	float dmg_save;
	float dmg;
	float dmgtime;

	string_t noise;
	string_t noise1;
	string_t noise2;
	string_t noise3;

	float speed;
	float air_finished;
	float pain_finished;
	float radsuit_finished;

	Entity* pContainingEntity;

	int playerclass;
	float maxspeed;

	float fov;
	int weaponanim;

	int pushmsec;

	int bInDuck;
	int flTimeStepSound;
	int flSwimTime;
	int flDuckTime;
	int iStepLeft;
	float flFallVelocity;

	int gamestate;

	int oldbuttons;

	int groupinfo;

	// For mods
	int iuser1;
	int iuser2;
	int iuser3;
	int iuser4;
	float fuser1;
	float fuser2;
	float fuser3;
	float fuser4;
	Vector vuser1;
	Vector vuser2;
	Vector vuser3;
	Vector vuser4;
	Entity* euser1;
	Entity* euser2;
	Entity* euser3;
	Entity* euser4;

public:
	[[nodiscard]] static Entity* FromEntvars(entvars_t* entvars)
	{
		return reinterpret_cast<Entity*>((char*)entvars - offsetof(Entity, classname));
	}
	[[nodiscard]] entvars_t* ToEntvars() { return reinterpret_cast<entvars_t*>(&this->classname); }

	[[nodiscard]] constexpr bool IsFree() const { return free != 0; }
	[[nodiscard]] constexpr int GetSerialNumber() const { return serialnumber; }

	template <class T> [[nodiscard]] constexpr T* Get() { return static_cast<T*>(pvPrivateData); }
	template <class T> [[nodiscard]] constexpr T* GetNew();
	template <class T> [[nodiscard]] constexpr void Free();

#ifdef GAME_DLL
	template <class T> [[nodiscard]] static T* Create();
#endif

#ifdef GAME_DLL
	[[nodiscard]] int GetIndex() { return engine::IndexOfEdict(this); }

	[[nodiscard]] static Entity* FromIndex(const int index) { return engine::PEntityOfEntIndex(index); }
#endif
};


template <class T>
[[nodiscard]] constexpr
T* Entity::GetNew()
{
#ifdef CLIENT_DLL
	free = false;
	pContainingEntity = this;
#endif

	if (pvPrivateData == nullptr)
	{
		T* instance = new T {this};
		pvPrivateData = instance;
	}

	return static_cast<T*>(pvPrivateData);
}


template <class T>
[[nodiscard]] constexpr
void Entity::Free()
{
	if (pvPrivateData != nullptr)
	{
		auto instance = reinterpret_cast<T*>(pvPrivateData);
		delete instance;
		pvPrivateData = nullptr;
	}
}


#ifdef GAME_DLL

template <class T>
[[nodiscard]]
T* Entity::Create()
{
	Entity* entity = engine::CreateEntity();

	if (entity != nullptr)
	{
		T* instance = new T {entity};
		entity->pvPrivateData = instance;

		return static_cast<T*>(entity->pvPrivateData);
	}

	return nullptr;
}

#endif /* GAME_DLL */
