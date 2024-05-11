/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#pragma once

#define SF_BEAM_STARTON 0x0001
#define SF_BEAM_TOGGLE 0x0002
#define SF_BEAM_RANDOM 0x0004
#define SF_BEAM_RING 0x0008
#define SF_BEAM_SPARKSTART 0x0010
#define SF_BEAM_SPARKEND 0x0020
#define SF_BEAM_DECALS 0x0040
#define SF_BEAM_SHADEIN 0x0080
#define SF_BEAM_SHADEOUT 0x0100
#define SF_BEAM_TEMPORARY 0x8000

#define SF_SPRITE_STARTON 0x0001
#define SF_SPRITE_ONCE 0x0002
#define SF_SPRITE_TEMPORARY 0x8000

class CSprite : public CPointEntity
{
public:
	CSprite(Entity* containingEntity) : CPointEntity(containingEntity) {}

	DECLARE_SAVERESTORE()

	bool Spawn() override;
	void Precache() override;

	int ObjectCaps() override
	{
		int flags = 0;
#ifdef HALFLIFE_SAVERESTORE
		if (v.spawnflags & SF_SPRITE_TEMPORARY)
			flags = FCAP_DONT_SAVE;
#endif
		return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | flags;
	}
	void EXPORT AnimateThink();
	void EXPORT ExpandThink();
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void Animate(float frames);
	void Expand(float scaleSpeed, float fadeSpeed);
	void SpriteInit(const char* pSpriteName, const Vector& origin);

	inline void SetAttachment(Entity* pEntity, int attachment)
	{
		if (pEntity)
		{
			v.skin = ENTINDEX(pEntity);
			v.body = attachment;
			v.aiment = pEntity;
			v.movetype = MOVETYPE_FOLLOW;
		}
	}
	void TurnOff();
	void TurnOn();
	inline float Frames() { return m_maxFrame; }
	inline void SetTransparency(int rendermode, int r, int g, int b, int a, int fx)
	{
		v.rendermode = rendermode;
		v.rendercolor.x = r;
		v.rendercolor.y = g;
		v.rendercolor.z = b;
		v.renderamt = a;
		v.renderfx = fx;
	}
	inline void SetTexture(int spriteIndex) { v.modelindex = spriteIndex; }
	inline void SetScale(float scale) { v.scale = scale; }
	inline void SetColor(int r, int g, int b)
	{
		v.rendercolor.x = r;
		v.rendercolor.y = g;
		v.rendercolor.z = b;
	}
	inline void SetBrightness(int brightness) { v.renderamt = brightness; }

	inline void AnimateAndDie(float framerate)
	{
		SetThink(&CSprite::AnimateUntilDead);
		v.framerate = framerate;
		v.dmgtime = gpGlobals->time + (m_maxFrame / framerate);
		v.nextthink = gpGlobals->time;
	}

	void EXPORT AnimateUntilDead();

	static CSprite* SpriteCreate(const char* pSpriteName, const Vector& origin, bool animate);

private:
	float m_lastTime;
	float m_maxFrame;
};


class CBeam : public CBaseEntity
{
public:
	CBeam(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	bool Spawn() override;
	void Precache() override;
	int ObjectCaps() override
	{
		int flags = 0;
#ifdef HALFLIFE_SAVERESTORE
		if (v.spawnflags & SF_BEAM_TEMPORARY)
			flags = FCAP_DONT_SAVE;
#endif
		return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | flags;
	}

	void EXPORT TriggerTouch(CBaseEntity* pOther);

	// These functions are here to show the way beams are encoded as entities.
	// Encoding beams as entities simplifies their management in the client/server architecture
	inline void SetType(int type) { v.rendermode = (v.rendermode & 0xF0) | (type & 0x0F); }
	inline void SetFlags(int flags) { v.rendermode = (v.rendermode & 0x0F) | (flags & 0xF0); }
	inline void SetStartPos(const Vector& pos) { v.origin = pos; }
	inline void SetEndPos(const Vector& pos) { v.angles = pos; }
	void SetStartEntity(int entityIndex);
	void SetEndEntity(int entityIndex);

	inline void SetStartAttachment(int attachment) { v.sequence = (v.sequence & 0x0FFF) | ((attachment & 0xF) << 12); }
	inline void SetEndAttachment(int attachment) { v.skin = (v.skin & 0x0FFF) | ((attachment & 0xF) << 12); }

	inline void SetTexture(int spriteIndex) { v.modelindex = spriteIndex; }
	inline void SetWidth(int width) { v.scale = width; }
	inline void SetNoise(int amplitude) { v.body = amplitude; }
	inline void SetColor(int r, int g, int b)
	{
		v.rendercolor.x = r;
		v.rendercolor.y = g;
		v.rendercolor.z = b;
	}
	inline void SetBrightness(int brightness) { v.renderamt = brightness; }
	inline void SetFrame(float frame) { v.frame = frame; }
	inline void SetScrollRate(int speed) { v.animtime = speed; }

	inline int GetType() { return v.rendermode & 0x0F; }
	inline int GetFlags() { return v.rendermode & 0xF0; }
	inline int GetStartEntity() { return v.sequence & 0xFFF; }
	inline int GetEndEntity() { return v.skin & 0xFFF; }

	const Vector& GetStartPos();
	const Vector& GetEndPos();

	Vector Center() override { return (GetStartPos() + GetEndPos()) * 0.5; } // center point of beam

	inline int GetTexture() { return v.modelindex; }
	inline int GetWidth() { return v.scale; }
	inline int GetNoise() { return v.body; }
	// inline void GetColor( int r, int g, int b ) { v.rendercolor.x = r; v.rendercolor.y = g; v.rendercolor.z = b; }
	inline int GetBrightness() { return v.renderamt; }
	inline int GetFrame() { return v.frame; }
	inline int GetScrollRate() { return v.animtime; }

	// Call after you change start/end positions
	void RelinkBeam();
	//	void		SetObjectCollisionBox();

	void DoSparks(const Vector& start, const Vector& end);
	CBaseEntity* RandomTargetname(const char* szName);
	void BeamDamage(TraceResult* ptr);
	// Init after BeamCreate()
	void BeamInit(const char* pSpriteName, int width);
	void PointsInit(const Vector& start, const Vector& end);
	void PointEntInit(const Vector& start, int endIndex);
	void EntsInit(int startIndex, int endIndex);
	void HoseInit(const Vector& start, const Vector& direction);

	static CBeam* BeamCreate(const char* pSpriteName, int width);

	inline void LiveForTime(float time)
	{
		SetThink(&CBeam::Remove);
		v.nextthink = gpGlobals->time + time;
	}
	inline void BeamDamageInstant(TraceResult* ptr, float damage)
	{
		v.dmg = damage;
		v.dmgtime = gpGlobals->time - 1;
		BeamDamage(ptr);
	}
};


#define SF_MESSAGE_ONCE 0x0001 // Fade in, not out
#define SF_MESSAGE_ALL 0x0002  // Send to all clients


class CLaser : public CBeam
{
public:
	CLaser(Entity* containingEntity) : CBeam(containingEntity) {}

	DECLARE_SAVERESTORE()

	bool Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;

	void TurnOn();
	void TurnOff();
	bool IsOn();

	void FireAtPoint(TraceResult& point);

	void EXPORT StrikeThink();
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	CSprite* m_pSprite;
	int m_iszSpriteName;
	Vector m_firePosition;
};
