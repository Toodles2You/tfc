//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Player movement interface
//
// $NoKeywords: $
//=============================================================================

#pragma once

#include "bitvec.h"

class CBasePlayer;


class CGameMovement
{
public:
    CGameMovement(playermove_t* _pmove, CBasePlayer* _player)
        : pmove{_pmove}, player{_player} {}

    static inline CGameMovement* g_pCurrentMovement;

    virtual void Move() { g_pCurrentMovement = this; }
    virtual bool ShouldCollide(physent_t* other) { return true; }

    static int g_ShouldIgnore(physent_t* other);

protected:
    void InitTrace(trace_t* trace, const Vector& end);
    void TraceModel(physent_t* physent, const Vector& start, const Vector& end, trace_t* trace);

public:
    playermove_t* pmove;
    CBasePlayer* player;
};


class CHalfLifeMovement : public CGameMovement
{
public:
    CHalfLifeMovement(playermove_t* _pmove, CBasePlayer* _player);

    virtual void Move() override;
    virtual bool ShouldCollide(physent_t* other) override;

protected:
    void BuildWishMove(const Vector& move);
    void BuildFreeWishMove(const Vector& move);
    float GetSpeedModifier();
    virtual void CheckParameters();
    virtual void NoClip();
    virtual int FlyMove();
    virtual void CategorizePosition();
    virtual void CheckContents();
    virtual void Accelerate(const Vector& wishDir, float wishSpeed);

    bool IsSubmerged() { return pmove->waterlevel >= kWaterLevelWaist; }
    bool IsStuck();
    void CheckVelocity();
    void AddCorrectGravity();
    void FixUpGravity();
    bool AddToTouched(pmtrace_t& tr, Vector& velocity);
    void CheckFalling();

    virtual void Walk();
    virtual void WalkMove();
    virtual void AirMove();
    virtual void Jump();
    virtual void ApplyFriction();
    void CheckStepSound();
    virtual void StepSound();
    virtual void CategorizeGround();
    void StayOnGround();

    virtual void CheckDucking();
    bool BeginDucking();
    void FinishDucking();
    void FixDuckStuck(int direction);
    bool BeginUnducking();
    void FinishUnducking();

    virtual void Swim();
    bool BeginWaterJump();
    bool WaterJump();

    virtual bool Climb(physent_t* ladder);
    bool CheckLadder();

    void BuildCollisionMask();

public:
    static void ClipVelocity(const Vector& in, const Vector& normal, Vector& out, const float overbounce);

protected:
    Vector m_flatForward;
    Vector m_flatRight;
    Vector m_wishVel;
    Vector m_wishDir;
    float m_wishSpeed;
    Vector m_freeWishDir;
    float m_freeWishSpeed;
    CBitVec<MAX_PLAYERS> m_shouldCollide;
};


inline int CGameMovement::g_ShouldIgnore(physent_t* other)
{
    return g_pCurrentMovement->ShouldCollide(other) ? 0 : 1;
}


inline void CGameMovement::InitTrace(trace_t* trace, const Vector& end)
{
	memset(trace, 0, sizeof(trace_t));
    trace->endpos = end;
	trace->allsolid = 1;
	trace->fraction = 1.0F;
}


inline void CGameMovement::TraceModel(physent_t* physent, const Vector& start, const Vector& end, trace_t* trace)
{
	CGameMovement::InitTrace(trace, end);
	pmove->PM_TraceModel(physent, start, end, trace);
}

