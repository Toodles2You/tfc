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

#pragma once

#define MAX_WEAPON_NAME 128

struct WEAPON
{
	char szName[MAX_WEAPON_NAME];
	int iAmmoType;
	int iAmmo2Type;
	int iSlot;
	int iSlotPos;
	int iFlags;
	int iId;
	int iClip;

	HSPRITE hActive;
	Rect rcActive;
	HSPRITE hInactive;
	Rect rcInactive;
	HSPRITE hCrosshair;
	Rect rcCrosshair;
	HSPRITE hAutoaim;
	Rect rcAutoaim;
	HSPRITE hZoomedCrosshair;
	Rect rcZoomedCrosshair;
	HSPRITE hZoomedAutoaim;
	Rect rcZoomedAutoaim;
};

struct AMMO
{
	byte iCount;
	byte iMax;
	HSPRITE sprite;
	Rect rect;
};
