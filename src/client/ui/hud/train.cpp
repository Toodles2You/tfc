/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
//
// Train.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"

DECLARE_MESSAGE(m_Train, Train)


bool CHudTrain::Init()
{
	HOOK_MESSAGE(Train);

	m_iPos = 0;

	return CHudBase::Init();
}

void CHudTrain::VidInit()
{
	m_hSprite = LoadSprite("sprites/%d_train.spr");
}

void CHudTrain::Draw(const float time)
{
	if (0 != m_iPos)
	{
		int x, y;

		// This should show up to the right and part way up the armor number
		y = gHUD.GetHeight() - SPR_Height(m_hSprite, 0) - gHUD.m_iFontHeight;
		x = gHUD.GetWidth() / 3 + SPR_Width(m_hSprite, 0) / 4;

		gHUD.DrawHudSprite(m_hSprite, m_iPos - 1, nullptr, x, y, CHud::COLOR_PRIMARY, 255);
	}
}


bool CHudTrain::MsgFunc_Train(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	// update Train data
	m_iPos = READ_BYTE();

	SetActive(0 != m_iPos);

	return true;
}
