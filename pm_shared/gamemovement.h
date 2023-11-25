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

    virtual void Move() = 0;

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

public:
    static void ClipVelocity(const Vector& in, const Vector& normal, Vector& out, const float overbounce);

protected:
    Vector m_flatForward;
    Vector m_flatRight;
    Vector m_wishVel;
    Vector m_wishDir;
    float m_wishSpeed;
};

