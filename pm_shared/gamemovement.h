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
    CGameMovement(struct playermove_s* _pmove, CBasePlayer* _player);
    virtual ~CGameMovement();

    virtual void Move();

protected:
    virtual void CheckParameters();
    virtual void NoClip();

public:
    struct playermove_s* pmove;
    CBasePlayer* player;
};
