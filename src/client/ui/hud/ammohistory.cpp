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
//  ammohistory.cpp
//


#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

#include "ammohistory.h"

HistoryResource gHR;

#define AMMO_PICKUP_GAP (gHR.iHistoryGap + 5)
#define AMMO_PICKUP_PICK_HEIGHT (32 + (gHR.iHistoryGap * 2))
#define AMMO_PICKUP_HEIGHT_MAX (gHUD.GetHeight() - 100)

#define MAX_ITEM_NAME 32
int HISTORY_DRAW_TIME = 5;

// keep a list of items
struct ITEM_INFO
{
	char szName[MAX_ITEM_NAME];
	HSPRITE spr;
	Rect rect;
};

void HistoryResource::AddToHistory(int iType, int iId, int iCount)
{
	if (iType == HISTSLOT_AMMO && 0 == iCount)
		return; // no amount, so don't add

	if ((((AMMO_PICKUP_GAP * iCurrentHistorySlot) + AMMO_PICKUP_PICK_HEIGHT) > AMMO_PICKUP_HEIGHT_MAX) || (iCurrentHistorySlot >= MAX_HISTORY))
	{ // the pic would have to be drawn too high
		// so start from the bottom
		iCurrentHistorySlot = 0;
	}

	HIST_ITEM* freeslot = &rgAmmoHistory[iCurrentHistorySlot++]; // default to just writing to the first slot
	HISTORY_DRAW_TIME = client::GetCvarFloat("hud_drawhistory_time");

	freeslot->type = iType;
	freeslot->iId = iId;
	freeslot->iCount = iCount;
	freeslot->DisplayTime = gHUD.m_flTime + HISTORY_DRAW_TIME;
}

void HistoryResource::AddToHistory(int iType, const char* szName, int iCount)
{
	if (iType != HISTSLOT_ITEM)
		return;

	if ((((AMMO_PICKUP_GAP * iCurrentHistorySlot) + AMMO_PICKUP_PICK_HEIGHT) > AMMO_PICKUP_HEIGHT_MAX) || (iCurrentHistorySlot >= MAX_HISTORY))
	{ // the pic would have to be drawn too high
		// so start from the bottom
		iCurrentHistorySlot = 0;
	}

	HIST_ITEM* freeslot = &rgAmmoHistory[iCurrentHistorySlot++]; // default to just writing to the first slot

	// I am really unhappy with all the code in this file

	int i = gHUD.GetSpriteIndex(szName);
	if (i == -1)
		return; // unknown sprite name, don't add it to history

	freeslot->iId = i;
	freeslot->type = iType;
	freeslot->iCount = iCount;

	HISTORY_DRAW_TIME = client::GetCvarFloat("hud_drawhistory_time");
	freeslot->DisplayTime = gHUD.m_flTime + HISTORY_DRAW_TIME;
}


void HistoryResource::CheckClearHistory()
{
	for (int i = 0; i < MAX_HISTORY; i++)
	{
		if (HISTSLOT_EMPTY != rgAmmoHistory[i].type)
			return;
	}

	iCurrentHistorySlot = 0;
}

//
// Draw Ammo pickup history
//
bool HistoryResource::DrawAmmoHistory(float flTime)
{
	float scale;

	for (int i = 0; i < MAX_HISTORY; i++)
	{
		if (HISTSLOT_EMPTY != rgAmmoHistory[i].type)
		{
			rgAmmoHistory[i].DisplayTime = std::min(rgAmmoHistory[i].DisplayTime, gHUD.m_flTime + HISTORY_DRAW_TIME);

			scale = (rgAmmoHistory[i].DisplayTime - flTime) * 80;
			scale = std::min(scale, 255.0F);

			if (rgAmmoHistory[i].DisplayTime <= flTime)
			{ // pic drawing time has expired
				memset(&rgAmmoHistory[i], 0, sizeof(HIST_ITEM));
				CheckClearHistory();
			}
			else if (rgAmmoHistory[i].type == HISTSLOT_AMMO)
			{
				Rect rcPic;
				HSPRITE* spr = gWR.GetAmmoPicFromWeapon(rgAmmoHistory[i].iId, rcPic);


				// Draw the pic
				int ypos = gHUD.GetHeight() - (AMMO_PICKUP_PICK_HEIGHT + (AMMO_PICKUP_GAP * i));
				int xpos = gHUD.GetWidth() - 24;
				if (spr && 0 != *spr) // weapon isn't loaded yet so just don't draw the pic
				{					  // the dll has to make sure it has sent info the weapons you need
					gHUD.DrawHudSprite(*spr, 0, &rcPic, xpos, ypos, CHud::COLOR_PRIMARY, scale);
				}

				int r, g, b;
				gHUD.GetColor(r, g, b, CHud::COLOR_PRIMARY);
				ScaleColors(r, g, b, scale);

				// Draw the number
				gHUD.DrawHudNumberString(xpos - 10, ypos, xpos - 100, rgAmmoHistory[i].iCount, r, g, b);
			}
			else if (rgAmmoHistory[i].type == HISTSLOT_WEAP)
			{
				WEAPON* weap = gWR.GetWeapon(rgAmmoHistory[i].iId);

				if (!weap)
					return true; // we don't know about the weapon yet, so don't draw anything

				int ypos = gHUD.GetHeight() - (AMMO_PICKUP_PICK_HEIGHT + (AMMO_PICKUP_GAP * i));
				int xpos = gHUD.GetWidth() - (weap->rcInactive.right - weap->rcInactive.left);

				gHUD.DrawHudSprite(
					weap->hInactive,
					0,
					&weap->rcInactive,
					xpos,
					ypos,
					!gWR.HasAmmo(weap) ? CHud::COLOR_WARNING : CHud::COLOR_PRIMARY,
					scale
				);
			}
			else if (rgAmmoHistory[i].type == HISTSLOT_ITEM)
			{
				if (0 == rgAmmoHistory[i].iId)
					continue; // sprite not loaded

				Rect rect = gHUD.GetSpriteRect(rgAmmoHistory[i].iId);

				int ypos = gHUD.GetHeight() - (AMMO_PICKUP_PICK_HEIGHT + (AMMO_PICKUP_GAP * i));
				int xpos = gHUD.GetWidth() - (rect.right - rect.left) - 10;

				gHUD.DrawHudSpriteIndex(rgAmmoHistory[i].iId, xpos, ypos, CHud::COLOR_PRIMARY, scale);
			}
		}
	}


	return true;
}
