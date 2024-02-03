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
// Ammo.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "pm_shared.h"
#include "triangleapi.h"

#include <string.h>
#include <stdio.h>

#include "ammohistory.h"
#include "vgui_TeamFortressViewport.h"

WEAPON* gpActiveSel; // NULL means off, 1 means just the menu bar, otherwise
					 // this points to the active weapon menu item
WEAPON* gpLastSel;	 // Last weapon menu selection

client_sprite_t* GetSpriteList(client_sprite_t* pList, const char* psz, int iRes, int iCount);

WeaponsResource gWR;

int g_weaponselect = WEAPON_NONE;
int g_lastselect = WEAPON_NONE;

void WeaponsResource::LoadAllWeaponSprites()
{
	for (int i = 0; i < WEAPON_LAST; i++)
	{
		if (0 != rgWeapons[i].iId)
			LoadWeaponSprites(&rgWeapons[i]);
	}
}

int WeaponsResource::CountAmmo(int iId)
{
	if (iId < 0)
	{
		return 0;
	}
	return riAmmo[iId];
}

int WeaponsResource::MaxAmmo(int iId)
{
	return riMaxAmmo[iId];
}

bool WeaponsResource::HasAmmo(WEAPON* p)
{
	if (!p)
		return false;

	return p->iAmmoType == -1
		 || (p->iFlags & WEAPON_FLAG_SELECTONEMPTY) != 0
		 || p->iClip > 0
		 || 0 != CountAmmo(p->iAmmoType)
		 || 0 != CountAmmo(p->iAmmo2Type);
}


void WeaponsResource::LoadWeaponSprites(WEAPON* pWeapon)
{
	int i, iRes;

	if (ScreenWidth < 640)
		iRes = 320;
	else
		iRes = 640;

	char sz[256];

	if (!pWeapon)
		return;

	memset(&pWeapon->rcActive, 0, sizeof(Rect));
	memset(&pWeapon->rcInactive, 0, sizeof(Rect));
	memset(&pWeapon->rcAmmo, 0, sizeof(Rect));
	memset(&pWeapon->rcAmmo2, 0, sizeof(Rect));
	pWeapon->hInactive = 0;
	pWeapon->hActive = 0;
	pWeapon->hAmmo = 0;
	pWeapon->hAmmo2 = 0;

	sprintf(sz, "sprites/%s.txt", pWeapon->szName);
	client_sprite_t* pList = SPR_GetList(sz, &i);

	if (!pList)
		return;

	client_sprite_t* p;

	p = GetSpriteList(pList, "crosshair", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hCrosshair = SPR_Load(sz);
		pWeapon->rcCrosshair = p->rc;
	}
	else
		pWeapon->hCrosshair = 0;

	p = GetSpriteList(pList, "autoaim", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hAutoaim = SPR_Load(sz);
		pWeapon->rcAutoaim = p->rc;
	}
	else
		pWeapon->hAutoaim = 0;

	p = GetSpriteList(pList, "zoom", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hZoomedCrosshair = SPR_Load(sz);
		pWeapon->rcZoomedCrosshair = p->rc;
	}
	else
	{
		pWeapon->hZoomedCrosshair = pWeapon->hCrosshair; //default to non-zoomed crosshair
		pWeapon->rcZoomedCrosshair = pWeapon->rcCrosshair;
	}

	p = GetSpriteList(pList, "zoom_autoaim", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hZoomedAutoaim = SPR_Load(sz);
		pWeapon->rcZoomedAutoaim = p->rc;
	}
	else
	{
		pWeapon->hZoomedAutoaim = pWeapon->hZoomedCrosshair; //default to zoomed crosshair
		pWeapon->rcZoomedAutoaim = pWeapon->rcZoomedCrosshair;
	}

	p = GetSpriteList(pList, "weapon", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hInactive = SPR_Load(sz);
		pWeapon->rcInactive = p->rc;

		gHR.iHistoryGap = std::max(gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top);
	}
	else
		pWeapon->hInactive = 0;

	p = GetSpriteList(pList, "weapon_s", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hActive = SPR_Load(sz);
		pWeapon->rcActive = p->rc;
	}
	else
		pWeapon->hActive = 0;

	p = GetSpriteList(pList, "ammo", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hAmmo = SPR_Load(sz);
		pWeapon->rcAmmo = p->rc;

		gHR.iHistoryGap = std::max(gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top);
	}
	else
		pWeapon->hAmmo = 0;

	p = GetSpriteList(pList, "ammo2", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hAmmo2 = SPR_Load(sz);
		pWeapon->rcAmmo2 = p->rc;

		gHR.iHistoryGap = std::max(gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top);
	}
	else
		pWeapon->hAmmo2 = 0;
}

// Returns the first weapon for a given slot.
WEAPON* WeaponsResource::GetFirstPos(int iSlot)
{
	WEAPON* pret = NULL;

	for (int i = 0; i < MAX_WEAPON_POSITIONS; i++)
	{
		if (rgSlots[iSlot][i] != nullptr)
		{
			pret = rgSlots[iSlot][i];
			break;
		}
	}

	return pret;
}


WEAPON* WeaponsResource::GetNextActivePos(int iSlot, int iSlotPos)
{
	if (iSlotPos >= MAX_WEAPON_POSITIONS || iSlot >= MAX_WEAPON_SLOTS)
		return nullptr;

	WEAPON* p = gWR.rgSlots[iSlot][iSlotPos + 1];

	if (p == nullptr)
		return GetNextActivePos(iSlot, iSlotPos + 1);

	return p;
}


int giBucketHeight, giBucketWidth, giABHeight, giABWidth; // Ammo Bar width and height

HSPRITE ghsprBuckets; // Sprite for top row of weapons menu

DECLARE_MESSAGE(m_Ammo, CurWeapon);	 // Current weapon and clip
DECLARE_MESSAGE(m_Ammo, AmmoPickup); // flashes an ammo pickup record
DECLARE_MESSAGE(m_Ammo, WeapPickup); // flashes a weapon pickup record
DECLARE_MESSAGE(m_Ammo, HideWeapon); // hides the weapon, ammo, and crosshair displays temporarily
DECLARE_MESSAGE(m_Ammo, ItemPickup);
DECLARE_MESSAGE(m_Ammo, HitFeedback);

DECLARE_COMMAND(m_Ammo, Slot1);
DECLARE_COMMAND(m_Ammo, Slot2);
DECLARE_COMMAND(m_Ammo, Slot3);
DECLARE_COMMAND(m_Ammo, Slot4);
DECLARE_COMMAND(m_Ammo, Slot5);
DECLARE_COMMAND(m_Ammo, Slot6);
DECLARE_COMMAND(m_Ammo, Slot7);
DECLARE_COMMAND(m_Ammo, Slot8);
DECLARE_COMMAND(m_Ammo, Slot9);
DECLARE_COMMAND(m_Ammo, Slot10);
DECLARE_COMMAND(m_Ammo, Close);
DECLARE_COMMAND(m_Ammo, NextWeapon);
DECLARE_COMMAND(m_Ammo, PrevWeapon);
DECLARE_COMMAND(m_Ammo, LastWeapon);

bool CHudAmmo::Init()
{
	gHUD.AddHudElem(this);

	HOOK_MESSAGE(CurWeapon);
	HOOK_MESSAGE(AmmoPickup);
	HOOK_MESSAGE(WeapPickup);
	HOOK_MESSAGE(ItemPickup);
	HOOK_MESSAGE(HideWeapon);
	HOOK_MESSAGE(HitFeedback);

	HOOK_COMMAND("slot1", Slot1);
	HOOK_COMMAND("slot2", Slot2);
	HOOK_COMMAND("slot3", Slot3);
	HOOK_COMMAND("slot4", Slot4);
	HOOK_COMMAND("slot5", Slot5);
	HOOK_COMMAND("slot6", Slot6);
	HOOK_COMMAND("slot7", Slot7);
	HOOK_COMMAND("slot8", Slot8);
	HOOK_COMMAND("slot9", Slot9);
	HOOK_COMMAND("slot10", Slot10);
	HOOK_COMMAND("cancelselect", Close);
	HOOK_COMMAND("invnext", NextWeapon);
	HOOK_COMMAND("invprev", PrevWeapon);
	HOOK_COMMAND("lastinv", LastWeapon);

	Reset();

	CVAR_CREATE("hud_drawhistory_time", "5", 0);
	hud_fastswitch = CVAR_CREATE("hud_fastswitch", "0", FCVAR_ARCHIVE); // controls whether or not weapons can be selected in one keypress
	hud_selection_fadeout = CVAR_CREATE("hud_selection_fadeout", "0.5", FCVAR_ARCHIVE);
	hud_selection_timeout = CVAR_CREATE("hud_selection_timeout", "1.5", FCVAR_ARCHIVE);

	m_iFlags |= HUD_ACTIVE; //!!!

	gWR.Init();
	gHR.Init();

	return true;
}

void CHudAmmo::Reset()
{
	m_fFade = 0;
	m_iFlags |= HUD_ACTIVE; //!!!

	gpActiveSel = NULL;
	gHUD.m_iHideHUDDisplay = 0;

	m_flSelectionTime = -1000.0F;
	m_flHitFeedbackTime = -1000.0F;

	gWR.Reset();
	gHR.Reset();
}

bool CHudAmmo::VidInit()
{
	// Load sprites for buckets (top row of weapon menu)
	m_HUD_bucket0 = gHUD.GetSpriteIndex("bucket1");
	m_HUD_selection = gHUD.GetSpriteIndex("selection");

	ghsprBuckets = gHUD.GetSprite(m_HUD_bucket0);
	giBucketWidth = gHUD.GetSpriteRect(m_HUD_bucket0).right - gHUD.GetSpriteRect(m_HUD_bucket0).left;
	giBucketHeight = gHUD.GetSpriteRect(m_HUD_bucket0).bottom - gHUD.GetSpriteRect(m_HUD_bucket0).top;

	gHR.iHistoryGap = std::max(gHR.iHistoryGap, gHUD.GetSpriteRect(m_HUD_bucket0).bottom - gHUD.GetSpriteRect(m_HUD_bucket0).top);

	// If we've already loaded weapons, let's get new sprites
	gWR.LoadAllWeaponSprites();

	if (ScreenWidth >= 640)
	{
		giABWidth = 20;
		giABHeight = 4;
	}
	else
	{
		giABWidth = 10;
		giABHeight = 2;
	}

	return true;
}

//
// Think:
//  Used for selection of weapon menu item.
//
void CHudAmmo::Think()
{
	if (gHUD.m_iWeaponBits != gWR.iOldWeaponBits)
	{
		gWR.iOldWeaponBits = gHUD.m_iWeaponBits;

		for (int i = WEAPON_LAST - 1; i > 0; i--)
		{
			WEAPON* p = gWR.GetWeapon(i);

			if (p && WEAPON_NONE != p->iId)
			{
				if (gHUD.HasWeapon(p->iId))
					gWR.PickupWeapon(p);
				else
					gWR.DropWeapon(p);
			}
		}
	}

	if (!gpActiveSel)
		return;

	// has the player selected one?
	if ((gHUD.m_iKeyBits & IN_ATTACK) != 0)
	{
		if (gpActiveSel != (WEAPON*)1)
		{
			g_weaponselect = gpActiveSel->iId;
		}

		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		gHUD.m_iKeyBits &= ~IN_ATTACK;

		PlaySound("common/wpn_select.wav", 1);
	}
}

//
// Helper function to return a Ammo pointer from id
//

HSPRITE* WeaponsResource::GetAmmoPicFromWeapon(int iAmmoId, Rect& rect)
{
	for (int i = 0; i < WEAPON_LAST; i++)
	{
		if (rgWeapons[i].iAmmoType == iAmmoId)
		{
			rect = rgWeapons[i].rcAmmo;
			return &rgWeapons[i].hAmmo;
		}
		else if (rgWeapons[i].iAmmo2Type == iAmmoId)
		{
			rect = rgWeapons[i].rcAmmo2;
			return &rgWeapons[i].hAmmo2;
		}
	}

	return NULL;
}


// Menu Selection Code

void WeaponsResource::SelectSlot(int iSlot, bool fAdvance, int iDirection)
{
	if (iSlot > MAX_WEAPON_SLOTS)
		return;

	if ((gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) != 0)
		return;

	if (!gHUD.HasAnyWeapons())
		return;

	const auto fastSwitch = gHUD.m_Ammo.hud_fastswitch->value;

	WEAPON *weapon = nullptr;
	WEAPON *checkWeapon = gpActiveSel;

	if (fastSwitch != 0)
	{
		checkWeapon = gHUD.m_Ammo.m_pWeapon;
	}

	const auto newSlot =
		(checkWeapon == nullptr) || (checkWeapon == (WEAPON*)1) || (iSlot != checkWeapon->iSlot);

	if (!newSlot)
	{
		weapon = GetNextActivePos(checkWeapon->iSlot, checkWeapon->iSlotPos);
	}

	if (!weapon)
	{
		weapon = GetFirstPos(iSlot);
	}

	if (!weapon)
	{
		if (fastSwitch != 0)
		{
			gpActiveSel = nullptr;
		}
		else
		{
			gpActiveSel = (WEAPON*)1;
			gHUD.m_Ammo.m_flSelectionTime = gEngfuncs.GetClientTime();
		}

		PlaySound("common/wpn_denyselect.wav", 1);
	}
	else
	{
		if (fastSwitch != 0)
		{
			g_weaponselect = weapon->iId;
			gpLastSel = weapon;
			gpActiveSel = nullptr;
			gHUD.m_iKeyBits &= ~IN_ATTACK;
		}
		else
		{
			gpActiveSel = weapon;
			gHUD.m_Ammo.m_flSelectionTime = gEngfuncs.GetClientTime();

			if (newSlot)
			{
				PlaySound("common/wpn_hudon.wav", 1);
			}
			else
			{
				PlaySound("common/wpn_moveselect.wav", 1);
			}
		}
	}
}

//------------------------------------------------------------------------
// Message Handlers
//------------------------------------------------------------------------

void CHudAmmo::Update_AmmoX(int iIndex, int iCount)
{
	iCount = abs(iCount);

	if (m_pWeapon != nullptr)
	{
		if (m_pWeapon->iAmmoType == iIndex
		 || m_pWeapon->iAmmo2Type == iIndex)
		{
			if (gWR.CountAmmo(iIndex) != iCount)
			{
				m_fFade = 200.0f;
			}
		}
	}

	gWR.SetAmmo(iIndex, iCount);
}

void CHudAmmo::Update_CurWeapon(int iState, int iId, int iClip)
{
	static Rect nullrc;
	bool fOnTarget = false;

	// detect if we're also on target
	if (iState > 1)
	{
		fOnTarget = true;
	}

	if (iId < 1)
	{
		m_pWeapon = nullptr;
		g_lastselect = WEAPON_NONE;
		return;
	}

	if (iId == -1 && iClip == -1)
	{
		gpActiveSel = nullptr;
		return;
	}

	WEAPON* pWeapon = gWR.GetWeapon(iId);

	if (pWeapon == nullptr)
		return;

	if (iState == 0) // we're not the current weapon, so update no more
		return;

	if (iClip < -1)
	{
		iClip = abs(iClip);
	}

	if (pWeapon->iClip != iClip)
	{
		pWeapon->iClip = iClip;
		m_fFade = 200.0f;
	}

	if (m_pWeapon != pWeapon)
	{
		if (m_pWeapon != nullptr)
		{
			g_lastselect = m_pWeapon->iId;
		}

		m_pWeapon = pWeapon;
		m_fFade = 200.0f;
	}

	m_iFlags |= HUD_ACTIVE;
}

bool CHudAmmo::MsgFunc_AmmoPickup(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int iIndex = READ_BYTE();
	int iCount = READ_BYTE();

	// Add ammo to the history
	gHR.AddToHistory(HISTSLOT_AMMO, iIndex, abs(iCount));

	return true;
}

bool CHudAmmo::MsgFunc_WeapPickup(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int iIndex = READ_BYTE();

	// Add the weapon to the history
	gHR.AddToHistory(HISTSLOT_WEAP, iIndex);

	return true;
}

bool CHudAmmo::MsgFunc_ItemPickup(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const char* szName = READ_STRING();

	// Add the weapon to the history
	gHR.AddToHistory(HISTSLOT_ITEM, szName);

	return true;
}


bool CHudAmmo::MsgFunc_HideWeapon(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	gHUD.m_iHideHUDDisplay = READ_BYTE();

	if (0 != gEngfuncs.IsSpectateOnly())
		return true;

	if ((gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) != 0)
	{
		gpActiveSel = NULL;
	}

	return true;
}

//
//  CurWeapon: Update hud state with the current weapon and clip count. Ammo
//  counts are updated with AmmoX. Server assures that the Weapon ammo type
//  numbers match a real ammo type.
//
bool CHudAmmo::MsgFunc_CurWeapon(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int iState = READ_BYTE();
	int iId = READ_CHAR();
	int iClip = READ_CHAR();

	Update_CurWeapon(iState, iId, iClip);

	return true;
}

bool CHudAmmo::MsgFunc_HitFeedback(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int victim = READ_BYTE();
	int flags = READ_BYTE();
	int damage = READ_SHORT();

	if (damage <= 0 || (flags & kDamageFlagSelf) != 0)
	{
		return true;
	}

	m_flHitFeedbackTime = gHUD.m_flTime;

	if (gHUD.m_pCvarSuitVolume->value <= 0.0F)
	{
		return true;
	}
	
	const char *sample = "misc/hit.wav";
	byte pitch = PITCH_NORM;

	if ((flags & kDamageFlagDead) != 0)
	{
		sample = "misc/kill.wav";
	}
	else if ((flags & kDamageFlagHeadshot) != 0)
	{
		sample = "misc/crit.wav";
	}
	else
	{
		pitch = pitch * std::max(1.5F - (damage / 100.0F), 0.5F);
	}

	gEngfuncs.pfnPlaySoundByNameAtPitch(
		sample,
		gHUD.m_pCvarSuitVolume->value,
		pitch);

	return true;
}

//------------------------------------------------------------------------
// Command Handlers
//------------------------------------------------------------------------
// Slot button pressed
void CHudAmmo::SlotInput(int iSlot)
{
	if (gViewPort && gViewPort->SlotInput(iSlot))
		return;

	gWR.SelectSlot(iSlot, false, 1);
}

void CHudAmmo::UserCmd_Slot1()
{
	SlotInput(0);
}

void CHudAmmo::UserCmd_Slot2()
{
	SlotInput(1);
}

void CHudAmmo::UserCmd_Slot3()
{
	SlotInput(2);
}

void CHudAmmo::UserCmd_Slot4()
{
	SlotInput(3);
}

void CHudAmmo::UserCmd_Slot5()
{
	SlotInput(4);
}

void CHudAmmo::UserCmd_Slot6()
{
	SlotInput(5);
}

void CHudAmmo::UserCmd_Slot7()
{
	SlotInput(6);
}

void CHudAmmo::UserCmd_Slot8()
{
	SlotInput(7);
}

void CHudAmmo::UserCmd_Slot9()
{
	SlotInput(8);
}

void CHudAmmo::UserCmd_Slot10()
{
	SlotInput(9);
}

void CHudAmmo::UserCmd_Close()
{
	if (gpActiveSel)
	{
		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		PlaySound("common/wpn_hudoff.wav", 1);
	}
	else
		EngineClientCmd("escape");
}


// Selects the next item in the weapon menu
void CHudAmmo::UserCmd_NextWeapon()
{
	if ((gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) != 0)
		return;

	bool open = false;

	if (!gpActiveSel || gpActiveSel == (WEAPON*)1)
	{
		gpActiveSel = m_pWeapon;
		m_flSelectionTime = gEngfuncs.GetClientTime();
		open = true;
	}

	int pos = 0;
	int slot = 0;
	if (gpActiveSel)
	{
		pos = gpActiveSel->iSlotPos + 1;
		slot = gpActiveSel->iSlot;
	}

	for (int loop = 0; loop <= 1; loop++)
	{
		for (; slot < MAX_WEAPON_SLOTS; slot++)
		{
			for (; pos < MAX_WEAPON_POSITIONS; pos++)
			{
				WEAPON* wsp = gWR.GetWeaponSlot(slot, pos);

				if (wsp != nullptr)
				{
					if (hud_fastswitch->value != 0)
					{
						g_weaponselect = wsp->iId;
						gpLastSel = wsp;
						gpActiveSel = nullptr;
						gHUD.m_iKeyBits &= ~IN_ATTACK;
						return;
					}
					if (open || gpActiveSel->iSlot != wsp->iSlot)
					{
						PlaySound("common/wpn_hudon.wav", 1);
					}
					else
					{
						PlaySound("common/wpn_moveselect.wav", 1);
					}
					gpActiveSel = wsp;
					m_flSelectionTime = gEngfuncs.GetClientTime();
					return;
				}
			}

			pos = 0;
		}

		slot = 0; // start looking from the first slot again
	}

	PlaySound("common/wpn_denyselect.wav", 1);
	gpActiveSel = NULL;
}

// Selects the previous item in the menu
void CHudAmmo::UserCmd_PrevWeapon()
{
	if ((gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) != 0)
		return;

	bool open = false;

	if (!gpActiveSel || gpActiveSel == (WEAPON*)1)
	{
		gpActiveSel = m_pWeapon;
		m_flSelectionTime = gEngfuncs.GetClientTime();
		open = true;
	}

	int pos = MAX_WEAPON_POSITIONS - 1;
	int slot = MAX_WEAPON_SLOTS - 1;
	if (gpActiveSel)
	{
		pos = gpActiveSel->iSlotPos - 1;
		slot = gpActiveSel->iSlot;
	}

	for (int loop = 0; loop <= 1; loop++)
	{
		for (; slot >= 0; slot--)
		{
			for (; pos >= 0; pos--)
			{
				WEAPON* wsp = gWR.GetWeaponSlot(slot, pos);

				if (wsp != nullptr)
				{
					if (hud_fastswitch->value != 0)
					{
						g_weaponselect = wsp->iId;
						gpLastSel = wsp;
						gpActiveSel = nullptr;
						gHUD.m_iKeyBits &= ~IN_ATTACK;
						return;
					}
					if (open || gpActiveSel->iSlot != wsp->iSlot)
					{
						PlaySound("common/wpn_hudon.wav", 1);
					}
					else
					{
						PlaySound("common/wpn_moveselect.wav", 1);
					}
					gpActiveSel = wsp;
					m_flSelectionTime = gEngfuncs.GetClientTime();
					return;
				}
			}

			pos = MAX_WEAPON_POSITIONS - 1;
		}

		slot = MAX_WEAPON_SLOTS - 1;
	}

	PlaySound("common/wpn_denyselect.wav", 1);
	gpActiveSel = NULL;
}

// Selects the previous item in the menu
void CHudAmmo::UserCmd_LastWeapon()
{
	if (g_lastselect <= WEAPON_NONE)
		return;

	auto pWeapon = gWR.GetWeapon(g_lastselect);

	if (pWeapon && gHUD.HasWeapon(g_lastselect))
	{
		g_weaponselect = g_lastselect;
		gHUD.m_iKeyBits &= ~IN_ATTACK;
	}
	else
	{
		PlaySound("common/wpn_denyselect.wav", 1);
	}
}

extern Vector g_CrosshairTarget;

void CHudAmmo::DrawCrosshair(WEAPON *pWeapon, int a, bool zoom, bool autoaim)
{
	HSPRITE pic = pWeapon->hCrosshair;
	Rect *rc = &pWeapon->rcCrosshair;
	
	if (zoom && autoaim && 0 != pWeapon->hZoomedAutoaim)
	{
		pic = pWeapon->hZoomedAutoaim;
		rc = &pWeapon->rcZoomedAutoaim;
	}
	else if (zoom && 0 != pWeapon->hZoomedCrosshair)
	{
		pic = pWeapon->hZoomedCrosshair;
		rc = &pWeapon->rcZoomedCrosshair;
	}
	else if (autoaim && 0 != pWeapon->hAutoaim)
	{
		pic = pWeapon->hAutoaim;
		rc = &pWeapon->rcAutoaim;
	}

	if (pic <= 0)
	{
		return;
	}

	Vector screen;
	if (gEngfuncs.pTriAPI->WorldToScreen(g_CrosshairTarget, screen) != 0)
	{
		return;
	}

	screen.x = (1.0f + screen.x) * gHUD.GetWidth() * 0.5f;
	screen.y = (1.0f - screen.y) * gHUD.GetHeight() * 0.5f;
	gHUD.DrawHudSprite(
		pic,
		0,
		rc,
		(int)screen.x,
		(int)screen.y,
		CHud::COLOR_DEFAULT,
		a,
		CHud::a_center
	);
}

//-------------------------------------------------------------------------
// Drawing code
//-------------------------------------------------------------------------

bool CHudAmmo::Draw(float flTime)
{
	int a, x, y;
	int AmmoWidth;

	if ((gHUD.m_iHideHUDDisplay & HIDEHUD_WEAPONS) != 0)
		return true;

	// Draw Weapon Menu
	DrawWList(flTime);

	// Draw ammo pickup history
	gHR.DrawAmmoHistory(flTime);

	if ((m_iFlags & HUD_ACTIVE) == 0)
		return false;

	if (!m_pWeapon)
		return false;

	a = (int)std::max(MIN_ALPHA, m_fFade);

	if (m_fFade > 0)
		m_fFade -= (gHUD.m_flTimeDelta * 20);

	WEAPON* pw = m_pWeapon; // shorthand

	if (gHUD.m_pCvarCrosshair->value != 0)
	{
		DrawCrosshair(
			pw,
			255,
			gHUD.IsViewZoomed(),
			flTime - m_flHitFeedbackTime < 0.2);
	}

	// SPR_Draw Ammo
	if (pw->iClip < 0
	 && pw->iAmmoType == -1
	 && pw->iAmmo2Type == -1)
	{
		return false;
	}


	int iFlags = DHN_DRAWZERO; // draw 0 values

	AmmoWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).right - gHUD.GetSpriteRect(gHUD.m_HUD_number_0).left;

	// Does this weapon have a clip?
	y = gHUD.GetHeight() - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;

	// Does weapon have any ammo at all?
	if (pw->iClip >= 0 || m_pWeapon->iAmmoType != -1)
	{
		int iIconWidth = m_pWeapon->rcAmmo.right - m_pWeapon->rcAmmo.left;

		if (pw->iClip >= 0 && m_pWeapon->iAmmoType != -1)
		{
			// room for the number and the '|' and the current ammo

			x = gHUD.GetWidth() - (8 * AmmoWidth) - iIconWidth;
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, pw->iClip, CHud::COLOR_PRIMARY, a);

			int iBarWidth = AmmoWidth / 10;

			x += AmmoWidth / 2;

			// draw the | bar
			gHUD.DrawHudFill(x, y, iBarWidth, gHUD.m_iFontHeight, CHud::COLOR_PRIMARY, MIN_ALPHA);

			x += iBarWidth + AmmoWidth / 2;

			// GL Seems to need this
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, gWR.CountAmmo(pw->iAmmoType), CHud::COLOR_PRIMARY, a);
		}
		else
		{
			auto ammoCount =
				(pw->iClip >= 0) ? pw->iClip : gWR.CountAmmo(pw->iAmmoType);

			// SPR_Draw a bullets only line
			x = gHUD.GetWidth() - 4 * AmmoWidth - iIconWidth;
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, ammoCount, CHud::COLOR_PRIMARY, a);
		}

		// Draw the ammo Icon
		int iOffset = (m_pWeapon->rcAmmo.bottom - m_pWeapon->rcAmmo.top) / 8;
		gHUD.DrawHudSprite(m_pWeapon->hAmmo, 0, &m_pWeapon->rcAmmo, x, y - iOffset, CHud::COLOR_PRIMARY, a);
	}

	// Does weapon have seconday ammo?
	if (pw->iAmmo2Type != -1)
	{
		int iIconWidth = m_pWeapon->rcAmmo2.right - m_pWeapon->rcAmmo2.left;

		// Do we have secondary ammo?
		if (gWR.CountAmmo(pw->iAmmo2Type) > 0)
		{
			y -= gHUD.m_iFontHeight + gHUD.m_iFontHeight / 4;
			x = gHUD.GetWidth() - 4 * AmmoWidth - iIconWidth;
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, gWR.CountAmmo(pw->iAmmo2Type), CHud::COLOR_PRIMARY, a);

			// Draw the ammo Icon
			int iOffset = (m_pWeapon->rcAmmo2.bottom - m_pWeapon->rcAmmo2.top) / 8;
			gHUD.DrawHudSprite(m_pWeapon->hAmmo2, 0, &m_pWeapon->rcAmmo2, x, y - iOffset, CHud::COLOR_PRIMARY, a);
		}
	}
	return true;
}


//
// Draws the ammo bar on the hud
//
int CHudAmmo::DrawBar(int x, int y, int width, int height, float f, int color, int a)
{
	if (f < 0)
		f = 0;
	if (f > 1)
		f = 1;

	if (0 != f)
	{
		int w = f * width;

		// Always show at least one pixel if we have ammo.
		if (w <= 0)
			w = 1;
		gHUD.DrawHudFill(x, y, w, height, CHud::COLOR_SECONDARY, a);
		x += w;
		width -= w;
	}

	gHUD.DrawHudFill(x, y, width, height, static_cast<CHud::hudcolor_e>(color), a / 2);

	return (x + width);
}



void CHudAmmo::DrawAmmoBar(WEAPON* p, int x, int y, int width, int height, int color, int a)
{
	if (!p)
		return;

	float f;

	if (p->iAmmoType != -1)
	{
		f =gWR.CountAmmo(p->iAmmoType) / (float)gWR.MaxAmmo(p->iAmmoType);

		x = DrawBar(
			x,
			y,
			width,
			height,
			f,
			color,
			a
		);

		x += 5;
	}

	// Do we have secondary ammo too?

	if (p->iAmmo2Type != -1)
	{
		f = gWR.CountAmmo(p->iAmmo2Type) / (float)gWR.MaxAmmo(p->iAmmo2Type);

		x = DrawBar(
			x,
			y,
			width,
			height,
			f,
			color,
			a
		);

		x += 5;
	}
}




//
// Draw Weapon Menu
//
bool CHudAmmo::DrawWList(float flTime)
{
	int x, y, i;

	if (!gpActiveSel)
		return false;

	float fadeDelta = 1.0F;

	if (hud_selection_timeout->value > 0.0F)
	{
		float selectionDelta = flTime - m_flSelectionTime;

		if (selectionDelta >= hud_selection_fadeout->value + hud_selection_timeout->value)
		{
			UserCmd_Close();
			return false;
		}

		fadeDelta =
			std::max(selectionDelta - hud_selection_fadeout->value, 0.0F);
		
		fadeDelta = 1.0F - (fadeDelta / hud_selection_timeout->value);
	}

	int iActiveSlot;

	if (gpActiveSel == (WEAPON*)1)
		iActiveSlot = -1; // current slot has no weapons
	else
		iActiveSlot = gpActiveSel->iSlot;

	x = 10; //!!!
	y = 10; //!!!


	// Ensure that there are available choices in the active slot
	if (iActiveSlot > 0)
	{
		if (!gWR.GetFirstPos(iActiveSlot))
		{
			gpActiveSel = (WEAPON*)1;
			iActiveSlot = -1;
		}
	}

	// Draw top line
	for (i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		int iWidth;

		// make active slot wide enough to accomodate gun pictures
		if (i == iActiveSlot)
		{
			WEAPON* p = gWR.GetFirstPos(iActiveSlot);
			if (p)
				iWidth = p->rcActive.right - p->rcActive.left;
			else
				iWidth = giBucketWidth;
		}
		else
		{
			iWidth = giBucketWidth;
		}

		gHUD.DrawHudSpriteIndex(
			m_HUD_bucket0 + i,
			x,
			y,
			CHud::COLOR_PRIMARY,
			(iActiveSlot == i ? 255 : 192) * fadeDelta
		);

		x += iWidth + 5;
	}

	x = 10;

	// Draw all of the buckets
	for (i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		y = giBucketHeight + 10;

		// If this is the active slot, draw the bigger pictures,
		// otherwise just draw boxes
		if (i == iActiveSlot)
		{
			WEAPON* p = gWR.GetFirstPos(i);
			int iWidth = giBucketWidth;
			if (p)
				iWidth = p->rcActive.right - p->rcActive.left;

			for (int iPos = 0; iPos < MAX_WEAPON_POSITIONS; iPos++)
			{
				p = gWR.GetWeaponSlot(i, iPos);

				if (!p || 0 == p->iId)
					continue;

				// Draw Weapon if Red if no ammo
				auto ammo = gWR.HasAmmo(p);
				auto color = ammo ? CHud::COLOR_PRIMARY : CHud::COLOR_WARNING;

				if (gpActiveSel == p)
				{
					gHUD.DrawHudSprite(p->hActive, 0, &p->rcActive, x, y, color, 255 * fadeDelta);
					gHUD.DrawHudSpriteIndex(m_HUD_selection, x, y, color, 255 * fadeDelta);
				}
				else
				{
					gHUD.DrawHudSprite(p->hInactive, 0, &p->rcInactive, x, y, color, (ammo ? 192 : 128) * fadeDelta);
				}

				// Draw Ammo Bar
				if (ammo)
				{
					DrawAmmoBar(p, x + giABWidth / 2, y, giABWidth, giABHeight, color, 192 * fadeDelta);
				}

				y += p->rcActive.bottom - p->rcActive.top + 5;
			}

			x += iWidth + 5;
		}
		else
		{
			// Draw Row of weapons.

			for (int iPos = 0; iPos < MAX_WEAPON_POSITIONS; iPos++)
			{
				WEAPON* p = gWR.GetWeaponSlot(i, iPos);

				if (!p || 0 == p->iId)
					continue;

				bool ammo = gWR.HasAmmo(p);

				gHUD.DrawHudFill(
					x,
					y,
					giBucketWidth, giBucketHeight,
					ammo ? CHud::COLOR_PRIMARY : CHud::COLOR_WARNING,
					(ammo ? 128 : 96) * fadeDelta
				);

				y += giBucketHeight + 5;
			}

			x += giBucketWidth + 5;
		}
	}

	return true;
}


/* =================================
	GetSpriteList

Finds and returns the matching 
sprite name 'psz' and resolution 'iRes'
in the given sprite list 'pList'
iCount is the number of items in the pList
================================= */
client_sprite_t* GetSpriteList(client_sprite_t* pList, const char* psz, int iRes, int iCount)
{
	if (!pList)
		return NULL;

	int i = iCount;
	client_sprite_t* p = pList;

	while (0 != i--)
	{
		if ((0 == strcmp(psz, p->szName)) && (p->iRes == iRes))
			return p;
		p++;
	}

	return NULL;
}
