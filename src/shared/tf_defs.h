//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Whatever
//
// $NoKeywords: $
//=============================================================================

#pragma once

#include "Platform.h"
#include "mathlib.h"
#include "const.h"
#include "cdll_dll.h"

enum
{
	AT_SAVESHOT = 1,		// Kevlar  	 : Reduces bullet damage
	AT_SAVENAIL = 2,		// Wood :) 	 : Reduces nail damage
	AT_SAVEEXPLOSION = 4,	// Blast   	 : Reduces explosion damage
	AT_SAVEELECTRICITY = 8, // Shock	 : Reduces electricity damage
	AT_SAVEFIRE = 16,		// Asbestos	 : Reduces fire damage
};

struct PCInfo
{
	int maxHealth;		  // Maximum Health Level
	int maxSpeed;		  // Maximum movement speed
	int maxArmor;		  // Maximum Armor Level, of any armor class
	int initArmor;		  // Armor level when respawned
	float maxArmorType;	  // Maximum level of Armor absorption
	float initArmorType;  // Absorption Level of armor when respawned
	int armorClasses;	  // Armor Classes allowed for this class
	int initArmorClasses; // Armorclass worn when respawned
	int weapons[4];
	int grenades[2];
	int maxAmmo[AMMO_TYPES];
	int initAmmo[AMMO_TYPES];
	const char* model;
	const char* modelPath;
	byte colormap[2][2];
};

extern PCInfo sTFClassInfo[PC_LASTCLASS];

enum
{
	BUILD_NONE,
	BUILD_DISPENSER,
	BUILD_SENTRYGUN,
	BUILD_MORTAR,
	BUILD_ENTRY_TELEPORTER,
	BUILD_EXIT_TELEPORTER,
};

enum
{
	BS_BUILDING				= 1 << 0,
	BS_HAS_DISPENSER		= 1 << 1,
	BS_HAS_SENTRYGUN		= 1 << 2,
	BS_HAS_MORTAR			= 1 << 3,
	BS_HAS_ENTRY_TELEPORTER	= 1 << 4,
	BS_HAS_EXIT_TELEPORTER	= 1 << 5,
	BS_CAN_DISPENSER		= 1 << 6,
	BS_CAN_SENTRYGUN		= 1 << 7,
	BS_CAN_MORTAR			= 1 << 8,
	BS_CAN_ENTRY_TELEPORTER	= 1 << 9,
	BS_CAN_EXIT_TELEPORTER	= 1 << 10,
};
