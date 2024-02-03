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
	int maxAmmo[4];
	int initAmmo[4];
};

extern PCInfo sTFClassInfo[PC_LASTCLASS];
