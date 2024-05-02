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
// ammohistory.h
//

#pragma once

// this is the max number of items in each bucket
#define MAX_WEAPON_POSITIONS MAX_WEAPON_SLOTS

class WeaponsResource
{
private:
	// Information about weapons & ammo
	WEAPON rgWeapons[WEAPON_TYPES]; // Weapons Array
	AMMO rgAmmo[AMMO_TYPES];

	// counts of weapons * ammo
	WEAPON* rgSlots[MAX_WEAPON_SLOTS + 1][MAX_WEAPON_POSITIONS + 1]; // The slots currently in use by weapons.  The value is a pointer to the weapon;  if it's NULL, no weapon is there

public:
	void Init();

	void Reset()
	{
		iOldWeaponBits = 0;
		memset(rgSlots, 0, sizeof (rgSlots));

		for (int i = 0; i < AMMO_TYPES; i++)
		{
			rgAmmo[i].iCount = 0;
		}
	}

	///// WEAPON /////
	std::uint64_t iOldWeaponBits;

	WEAPON* GetWeapon(int iId) { return &rgWeapons[iId]; }
	void AddWeapon(WEAPON* wp)
	{
		rgWeapons[wp->iId] = *wp;
		LoadWeaponSprites(&rgWeapons[wp->iId]);
	}

	void PickupWeapon(WEAPON* wp)
	{
		if (wp->iSlot != -1)
		{
			rgSlots[wp->iSlot][wp->iSlotPos] = wp;
		}
	}

	void DropWeapon(WEAPON* wp)
	{
		if (wp->iSlot != -1)
		{
			rgSlots[wp->iSlot][wp->iSlotPos] = NULL;
		}
	}

	void DropAllWeapons()
	{
		for (int i = 0; i < WEAPON_TYPES; i++)
		{
			DropWeapon(&rgWeapons[i]);
		}
	}

	WEAPON* GetWeaponSlot(int slot, int pos) { return rgSlots[slot][pos]; }

	void LoadWeaponSprites(WEAPON* wp);
	void LoadAllWeaponSprites();
	void LoadAmmoSprites(int iType, const char* pszIcon);
	WEAPON* GetFirstPos(int iSlot);
	void SelectSlot(int iSlot, bool fAdvance, int iDirection);
	WEAPON* GetNextActivePos(int iSlot, int iSlotPos);

	bool HasAmmo(WEAPON* p);

	///// AMMO /////
	void SetAmmo(int iId, int iCount) { rgAmmo[iId].iCount = iCount; }
	void SetMaxAmmo(int iId, int iCount) { rgAmmo[iId].iMax = std::max(iCount, 1); }

	int CountAmmo(int iId);
	int MaxAmmo(int iId);

	HSPRITE* GetAmmoPicFromWeapon(int iAmmoId, Rect& rect);
};

extern WeaponsResource gWR;


#define MAX_HISTORY 12
enum
{
	HISTSLOT_EMPTY,
	HISTSLOT_AMMO,
	HISTSLOT_WEAP,
	HISTSLOT_ITEM,
};

class HistoryResource
{
private:
	struct HIST_ITEM
	{
		int type;
		float DisplayTime; // the time at which this item should be removed from the history
		int iCount;
		int iId;
	};

	HIST_ITEM rgAmmoHistory[MAX_HISTORY];

public:
	void Init()
	{
		Reset();
	}

	void Reset()
	{
		memset(rgAmmoHistory, 0, sizeof rgAmmoHistory);
		iHistoryGap = 45;
		iCurrentHistorySlot = 0;
	}

	int iHistoryGap;
	int iCurrentHistorySlot;

	void AddToHistory(int iType, int iId, int iCount = 0);
	void AddToHistory(int iType, const char* szName, int iCount = 0);

	void CheckClearHistory();
	bool DrawAmmoHistory(float flTime);
};

extern HistoryResource gHR;
