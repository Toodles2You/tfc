//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Stupid bipedal dumb ass movement
//
// $NoKeywords: $
//=============================================================================

#pragma once

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

public:
    playermove_t* pmove;
    CBasePlayer* player;
};


class CHalfLifeMovement : public CGameMovement
{
public:
    CHalfLifeMovement(playermove_t* _pmove, CBasePlayer* _player);

    virtual void Move() override;

protected:
    virtual void CheckParameters();
    virtual void NoClip();
    virtual int FlyMove();
    virtual void CategorizePosition();
    virtual void Accelerate(const Vector& wishDir, float wishSpeed);

    bool IsStuck();
    void CheckVelocity();
    void AddCorrectGravity();
    void FixUpGravity();
    bool AddToTouched(pmtrace_t& tr, Vector& velocity);

    virtual void Walk();
    virtual void WalkMove();
    virtual void AirMove();
    virtual void Jump();
    virtual void ApplyFriction();
    void StayOnGround();

public:
    static void ClipVelocity(const Vector& in, const Vector& normal, Vector& out, const float overbounce);

protected:
    Vector m_flatForward;
    Vector m_flatRight;
    Vector m_wishVel;
    Vector m_wishDir;
    float m_wishSpeed;
};


inline int CGameMovement::g_ShouldIgnore(physent_t* other)
{
    return g_pCurrentMovement->ShouldCollide(other) ? 0 : 1;
}

