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

#include <string.h>
#include <stdio.h>

#include "ammohistory.h"
#include "vgui_TeamFortressViewport.h"

WEAPON* gpActiveSel; // nullptr means off, 1 means just the menu bar, otherwise
					 // this points to the active weapon menu item
WEAPON* gpLastSel;	 // Last weapon menu selection

client_sprite_t* GetSpriteList(client_sprite_t* pList, const char* psz, int iRes, int iCount);

WeaponsResource gWR;

int g_weaponselect = -1;
int g_lastselect = -1;

void WeaponsResource::LoadAllWeaponSprites()
{
	for (int i = 0; i < WEAPON_TYPES; i++)
	{
		LoadWeaponSprites(&rgWeapons[i]);
	}

	LoadAmmoSprites(AMMO_SHELLS, "shell");
	LoadAmmoSprites(AMMO_NAILS, "nail");
	LoadAmmoSprites(AMMO_ROCKETS, "rocket");
	LoadAmmoSprites(AMMO_CELLS, "cell");
	LoadAmmoSprites(AMMO_GRENADES1, "grenade");
	LoadAmmoSprites(AMMO_GRENADES2, "grenade");
}

void WeaponsResource::LoadAmmoSprites(int iType, const char* pszIcon)
{
	int i = gHUD.GetSpriteIndex(pszIcon);
	rgAmmo[iType].sprite = gHUD.GetSprite(i);
	rgAmmo[iType].rect = gHUD.GetSpriteRect(i);
}

int WeaponsResource::CountAmmo(int iId)
{
	if (iId < 0)
	{
		return 0;
	}
	return rgAmmo[iId].iCount;
}

int WeaponsResource::MaxAmmo(int iId)
{
	if (iId < 0)
	{
		return 0;
	}
	return rgAmmo[iId].iMax;
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
	pWeapon->hInactive = 0;
	pWeapon->hActive = 0;

	sprintf(sz, "sprites/%s.txt", pWeapon->szName);
	client_sprite_t* pList = client::SPR_GetList(sz, &i);

	if (!pList)
		return;

	client_sprite_t* p;

	p = GetSpriteList(pList, "crosshair", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hCrosshair = client::SPR_Load(sz);
		pWeapon->rcCrosshair = p->rc;
	}
	else
		pWeapon->hCrosshair = 0;

	p = GetSpriteList(pList, "autoaim", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hAutoaim = client::SPR_Load(sz);
		pWeapon->rcAutoaim = p->rc;
	}
	else
		pWeapon->hAutoaim = 0;

	p = GetSpriteList(pList, "zoom", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hZoomedCrosshair = client::SPR_Load(sz);
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
		pWeapon->hZoomedAutoaim = client::SPR_Load(sz);
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
		pWeapon->hInactive = client::SPR_Load(sz);
		pWeapon->rcInactive = p->rc;
	}
	else
		pWeapon->hInactive = 0;

	p = GetSpriteList(pList, "weapon_s", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hActive = client::SPR_Load(sz);
		pWeapon->rcActive = p->rc;
	}
	else
		pWeapon->hActive = 0;
}

// Returns the first weapon for a given slot.
WEAPON* WeaponsResource::GetFirstPos(int iSlot)
{
	WEAPON* pret = nullptr;

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

DECLARE_MESSAGE(m_Ammo, AmmoPickup); // flashes an ammo pickup record
DECLARE_MESSAGE(m_Ammo, WeapPickup); // flashes a weapon pickup record
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
	HOOK_MESSAGE(AmmoPickup);
	HOOK_MESSAGE(WeapPickup);
	HOOK_MESSAGE(ItemPickup);
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

	hud_fastswitch = client::RegisterVariable("hud_fastswitch", "0", FCVAR_ARCHIVE); // controls whether or not weapons can be selected in one keypress
	hud_selection_fadeout = client::RegisterVariable("hud_selection_fadeout", "0.5", FCVAR_ARCHIVE);
	hud_selection_timeout = client::RegisterVariable("hud_selection_timeout", "1.5", FCVAR_ARCHIVE);

	m_bWeaponSelectDisabled = false;

	gWR.Init();
	gHR.Init();

	return CHudBase::Init();
}

void CHudAmmo::Reset()
{
	CHudBase::Reset();

	gpActiveSel = nullptr;

	m_flSelectionTime = -1000.0F;
	m_flHitFeedbackTime = -1000.0F;

	gWR.Reset();
	gHR.Reset();
}

void CHudAmmo::VidInit()
{
	// Load sprites for buckets (top row of weapon menu)
	m_HUD_bucket0 = gHUD.GetSpriteIndex("bucket1");
	m_HUD_selection = gHUD.GetSpriteIndex("selection");

	ghsprBuckets = gHUD.GetSprite(m_HUD_bucket0);
	giBucketWidth = gHUD.GetSpriteRect(m_HUD_bucket0).right - gHUD.GetSpriteRect(m_HUD_bucket0).left;
	giBucketHeight = gHUD.GetSpriteRect(m_HUD_bucket0).bottom - gHUD.GetSpriteRect(m_HUD_bucket0).top;

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
}

//
// Think:
//  Used for selection of weapon menu item.
//
void CHudAmmo::Think()
{
	CHudBase::Think();

	if (gHUD.m_iWeaponBits != gWR.iOldWeaponBits)
	{
		gWR.iOldWeaponBits = gHUD.m_iWeaponBits;

		for (int i = WEAPON_TYPES - 1; i >= 0; i--)
		{
			WEAPON* p = gWR.GetWeapon(i);

			if (p != nullptr)
			{
				if (gHUD.HasWeapon(i))
				{
					gWR.PickupWeapon(p);
				}
				else
				{
					gWR.DropWeapon(p);
				}
			}
		}
	}

	if (gpActiveSel == nullptr
	 || gpActiveSel == (WEAPON*)1
	 || hud_fastswitch->value != 0.0F)
	{
		return;
	}

	// has the player selected one?
	if ((gHUD.m_iKeyBits & IN_ATTACK) != 0)
	{
		if (!m_bWeaponSelectDisabled)
		{
			g_weaponselect = gpActiveSel->iId;
			gHUD.m_iKeyBits &= ~IN_ATTACK;
			client::PlaySoundByName("common/wpn_select.wav", VOL_NORM);
		}
		else
		{
			client::PlaySoundByName("common/wpn_denyselect.wav", VOL_NORM);
		}

		gpLastSel = gpActiveSel;
		gpActiveSel = nullptr;
	}
}

//
// Helper function to return a Ammo pointer from id
//

HSPRITE* WeaponsResource::GetAmmoPicFromWeapon(int iAmmoId, Rect& rect)
{
	if (iAmmoId < 0 || iAmmoId >= AMMO_TYPES)
	{
		return nullptr;
	}

	rect = rgAmmo[iAmmoId].rect;
	return &rgAmmo[iAmmoId].sprite;
}


// Menu Selection Code

void WeaponsResource::SelectSlot(int iSlot, bool fAdvance, int iDirection)
{
	if (iSlot > MAX_WEAPON_SLOTS)
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
		if (fastSwitch == 0 || fastSwitch == 2)
		{
			gpActiveSel = (WEAPON*)1;
			gHUD.m_Ammo.m_flSelectionTime = client::GetClientTime();
		}
		else
		{
			gpActiveSel = nullptr;
		}

		client::PlaySoundByName("common/wpn_denyselect.wav", VOL_NORM);
	}
	else
	{
		if (fastSwitch != 0)
		{
			if (!gHUD.m_Ammo.m_bWeaponSelectDisabled)
			{
				g_weaponselect = weapon->iId;
			}
			else
			{
				client::PlaySoundByName("common/wpn_denyselect.wav", VOL_NORM);
			}

			gpLastSel = weapon;
		}
		else
		{
			gpActiveSel = weapon;
			gHUD.m_Ammo.m_flSelectionTime = client::GetClientTime();
			client::PlaySoundByName("common/wpn_moveselect.wav", VOL_NORM);
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
				Flash();
			}
		}
	}

	gWR.SetAmmo(iIndex, iCount);
}

void CHudAmmo::Update_CurWeapon(int iState, int iId, int iClip, bool bCanHolster)
{
	static Rect nullrc;
	bool fOnTarget = false;

	// detect if we're also on target
	if (iState > 1)
	{
		fOnTarget = true;
	}

	m_bWeaponSelectDisabled = !bCanHolster;

	if (iId == -1)
	{
		m_pWeapon = nullptr;
		g_lastselect = -1;
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
		Flash();
	}

	if (m_pWeapon != pWeapon)
	{
		if (m_pWeapon != nullptr)
		{
			g_lastselect = m_pWeapon->iId;
		}

		if (hud_fastswitch->value == 2.0F)
		{
			gpActiveSel = pWeapon;
			m_flSelectionTime = client::GetClientTime();
		}

		m_pWeapon = pWeapon;
		Flash();
	}
}

void CHudAmmo::Update_Detpack(const int setting)
{
	/* Toodles FIXME: Stupid wrapper because spaghetti header files are killing me. */
	gViewPort->Update_Detpack(setting);
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
	gHR.AddToHistory(szName);

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

	client::PlaySoundByNameAtPitch(
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
	if (gpActiveSel != nullptr)
	{
		if (hud_fastswitch->value == 0.0F
		 && gpActiveSel != (WEAPON*)1)
		{
			client::PlaySoundByName("common/wpn_hudoff.wav", VOL_NORM);
		}

		gpLastSel = gpActiveSel;
		gpActiveSel = nullptr;
	}
	else
	{
		client::ClientCmd("escape");
	}
}


// Selects the next item in the weapon menu
void CHudAmmo::UserCmd_NextWeapon()
{
	bool open = false;

	auto sel = gpActiveSel;

	if (sel == nullptr || sel == (WEAPON*)1)
	{
		sel = m_pWeapon;
		m_flSelectionTime = client::GetClientTime();
		open = true;
	}

	int pos = 0;
	int slot = 0;
	if (sel != nullptr)
	{
		pos = sel->iSlotPos + 1;
		slot = sel->iSlot;
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
						if (!m_bWeaponSelectDisabled)
						{
							g_weaponselect = wsp->iId;
							gHUD.m_iKeyBits &= ~IN_ATTACK;
						}
						else
						{
							client::PlaySoundByName("common/wpn_denyselect.wav", VOL_NORM);
						}
						gpLastSel = wsp;
						return;
					}
					client::PlaySoundByName("common/wpn_moveselect.wav", VOL_NORM);
					gpActiveSel = wsp;
					m_flSelectionTime = client::GetClientTime();
					return;
				}
			}

			pos = 0;
		}

		slot = 0; // start looking from the first slot again
	}

	if (hud_fastswitch->value == 0.0F)
	{
		client::PlaySoundByName("common/wpn_denyselect.wav", VOL_NORM);
		gpActiveSel = nullptr;
	}
}

// Selects the previous item in the menu
void CHudAmmo::UserCmd_PrevWeapon()
{
	bool open = false;

	auto sel = gpActiveSel;

	if (sel == nullptr || sel == (WEAPON*)1)
	{
		sel = m_pWeapon;
		m_flSelectionTime = client::GetClientTime();
		open = true;
	}

	int pos = MAX_WEAPON_POSITIONS - 1;
	int slot = MAX_WEAPON_SLOTS - 1;
	if (sel != nullptr)
	{
		pos = sel->iSlotPos - 1;
		slot = sel->iSlot;
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
						if (!m_bWeaponSelectDisabled)
						{
							g_weaponselect = wsp->iId;
							gHUD.m_iKeyBits &= ~IN_ATTACK;
						}
						else
						{
							client::PlaySoundByName("common/wpn_denyselect.wav", VOL_NORM);
						}
						gpLastSel = wsp;
						return;
					}
					client::PlaySoundByName("common/wpn_moveselect.wav", VOL_NORM);
					gpActiveSel = wsp;
					m_flSelectionTime = client::GetClientTime();
					return;
				}
			}

			pos = MAX_WEAPON_POSITIONS - 1;
		}

		slot = MAX_WEAPON_SLOTS - 1;
	}

	if (hud_fastswitch->value == 0.0F)
	{
		client::PlaySoundByName("common/wpn_denyselect.wav", VOL_NORM);
		gpActiveSel = nullptr;
	}
}

// Selects the previous item in the menu
void CHudAmmo::UserCmd_LastWeapon()
{
	if (g_lastselect == -1)
	{
		return;
	}

	auto pWeapon = gWR.GetWeapon(g_lastselect);

	if (!m_bWeaponSelectDisabled && pWeapon && gHUD.HasWeapon(g_lastselect))
	{
		g_weaponselect = g_lastselect;
		gHUD.m_iKeyBits &= ~IN_ATTACK;
	}
	else
	{
		client::PlaySoundByName("common/wpn_denyselect.wav", VOL_NORM);
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

	gHUD.DrawWorldSprite(pic, 0, rc, g_CrosshairTarget, CHud::COLOR_DEFAULT, a);
}

//-------------------------------------------------------------------------
// Drawing code
//-------------------------------------------------------------------------

void CHudAmmo::Draw(const float time)
{
	// Draw Weapon Menu
	DrawWList(time);

	// Draw ammo pickup history
	gHR.DrawAmmoHistory(time);

	if (m_pWeapon == nullptr)
	{
		return;
	}

	if (gHUD.m_pCvarCrosshair->value != 0)
	{
		DrawCrosshair(
			m_pWeapon,
			255,
			gHUD.IsViewZoomed(),
			time - m_flHitFeedbackTime < 0.2);
	}

	const auto color = CHud::COLOR_PRIMARY;
	const auto alpha = GetAlpha();

	/* Draw the weapon ammo in the lower right corner of the HUD. */
	const auto x = gHUD.GetWidth() - 10;
	const auto y = gHUD.GetHeight() - 26;

	const auto usesAmmo = m_pWeapon->iAmmoType > -1;
	const auto usesClip = m_pWeapon->iClip > -1;

	/* Fill the background if this weapon uses ammo and/or a clip. */
	if (usesAmmo || usesClip)
	{
		/* Widen the background if it uses both ammo & a clip. */
		const auto w = (usesClip == usesAmmo) ? 142 : 92;

		gHUD.DrawHudBackground(
			x - w,
			y - 16,
			x,
			y + 16);
	}

	if (usesAmmo)
	{
		/* Draw the ammo sprite on the right side. */
		Rect ammoRect;
		const auto ammoSprite = *gWR.GetAmmoPicFromWeapon(m_pWeapon->iAmmoType, ammoRect);

		gHUD.DrawHudSprite(
			ammoSprite,
			0,
			&ammoRect,
			x - 16,
			y,
			color,
			alpha,
			CHud::a_center);

		/* Draw the weapon ammo to the left of the ammo sprite. */
		gHUD.DrawHudNumberReverse(
			x - 32,
			y,
			gWR.CountAmmo(m_pWeapon->iAmmoType),
			DHN_DRAWZERO | DHN_3DIGITS,
			color,
			alpha,
			CHud::a_west);
	}

	if (usesClip)
	{
		/* Vertical divider bar. */
		gHUD.DrawHudFill(
			x - 98,
			y - 12,
			2,
			24,
			color,
			CHudBase::kMinAlpha);

		/* Draw the weapon clip to the left of the weapon ammo. */
		gHUD.DrawHudNumberReverse(
			x - 102,
			y,
			m_pWeapon->iClip,
			DHN_DRAWZERO | DHN_2DIGITS,
			color,
			alpha,
			CHud::a_west);
	}
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
void CHudAmmo::DrawWList(const float time)
{
	if (gpActiveSel == nullptr)
	{
		return;
	}

	auto fadeDelta = 1.0F;

	if (hud_selection_timeout->value > 0.0F)
	{
		auto selectionDelta = time - m_flSelectionTime;

		if (selectionDelta >= hud_selection_fadeout->value + hud_selection_timeout->value)
		{
			UserCmd_Close();
			return;
		}

		fadeDelta =
			std::max(selectionDelta - hud_selection_fadeout->value, 0.0F);
		
		fadeDelta = 1.0F - (fadeDelta / hud_selection_timeout->value);
	}

	const auto activeSlot = (gpActiveSel != (WEAPON*)1) ? gpActiveSel->iSlot : -1;
	const auto activePos = (gpActiveSel != (WEAPON*)1) ? gpActiveSel->iSlotPos : -1;

	/* Draw the weapon list in the top left corner of the HUD. */
	auto x = 10;

	/* Widen the background for each weapon slot. */
	const auto w = (activeSlot > -1) ? 282 : 132;

	/* Fill the background behind the top bar. */
	gHUD.DrawHudBackground(x, 10, x + w, 30);

	for (auto slot = 0; slot < MAX_WEAPON_SLOTS; slot++)
	{
		const auto isActiveSlot = slot == activeSlot;

		auto alpha = isActiveSlot ? CHudBase::kMaxAlpha : CHudBase::kMinAlpha;

		auto y = 10;

		/* Draw the top bar. */
		gHUD.DrawHudSpriteIndex(
			m_HUD_bucket0 + slot,
			x,
			y,
			CHud::COLOR_PRIMARY,
			alpha * fadeDelta);

		y += 28;

		if (!isActiveSlot)
		{
			/* Draw a background behind the small squares. */
			auto weaponCount = 0;
			for (auto pos = 0; pos < MAX_WEAPON_POSITIONS; pos++)
			{
				if (gWR.GetWeaponSlot(slot, pos) != nullptr)
				{
					weaponCount++;
				}
			}

			if (weaponCount == 0)
			{
				x += 28;
				continue;
			}

			gHUD.DrawHudBackground(
				x,
				y,
				x + 20,
				y + 20 + 25 * (weaponCount - 1));
		}

		/* Draw all of the icons just below. */
		for (auto pos = 0; pos < MAX_WEAPON_POSITIONS; pos++)
		{
			const auto weapon = gWR.GetWeaponSlot(slot, pos);

			if (weapon == nullptr)
			{
				continue;
			}

			const auto hasAmmo = gWR.HasAmmo(weapon);
			const auto color = hasAmmo ? CHud::COLOR_PRIMARY : CHud::COLOR_WARNING;

			if (!isActiveSlot)
			{
				/* This is not the active slot, so draw small squares. */
				gHUD.DrawHudFill(
					x,
					y,
					20,
					20,
					color,
					alpha * fadeDelta);

				y += 25;
				continue;
			}

			const auto isActivePos = pos == activePos;

			alpha = isActivePos ? CHudBase::kMaxAlpha : CHudBase::kMinAlpha;

			/* This is the active slot, so draw the weapon icons. */
			gHUD.DrawHudBackground(
				x,
				y,
				x + 170,
				y + 45);

			/* Draw the active or inactive weapon icon. */
			gHUD.DrawHudSprite(
				isActivePos ? weapon->hActive : weapon->hInactive,
				0,
				isActivePos ? &weapon->rcActive : &weapon->rcInactive,
				x,
				y,
				color,
				alpha * fadeDelta);

			/* Draw the weapon ammo bar on the top left of the weapon icon. */
			DrawAmmoBar(
				weapon,
				x + 5,
				y + 2,
				20,
				4,
				hasAmmo ? CHud::COLOR_SECONDARY : color,
				alpha * fadeDelta);

			y += 53;
		}

		x += isActiveSlot ? 178 : 28;
	}
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
		return nullptr;

	int i = iCount;
	client_sprite_t* p = pList;

	while (0 != i--)
	{
		if ((0 == strcmp(psz, p->szName)) && (p->iRes == iRes))
			return p;
		p++;
	}

	return nullptr;
}
