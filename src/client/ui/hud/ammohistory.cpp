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
#include <algorithm>

#include "ammohistory.h"

HistoryResource gHR;

HistoryResource::HIST_ITEM* HistoryResource::GetFreeItem()
{
	int i;
	HIST_ITEM* item;

	for (i = 0; i < MAX_HISTORY; i++)
	{
		item = rgAmmoHistory + i;

		if (item->type == HISTSLOT_EMPTY)
		{
			break;
		}
	}

	if (i == MAX_HISTORY)
	{
		memmove(
			rgAmmoHistory,
			rgAmmoHistory + 1,
			sizeof(HIST_ITEM) * MAX_HISTORY);

		i = MAX_HISTORY - 1;
	}

	return item;
}

void HistoryResource::AddToHistory(int iType, int iId, int iCount)
{
	auto item = GetFreeItem();

	item->type = iType;
	item->iId = iId;
	item->iCount = iCount;
	item->DisplayTime = gHUD.m_flTime;
}

void HistoryResource::AddToHistory(const char* szName, int iCount)
{
	const auto itemIndex = gHUD.GetSpriteIndex(szName);

	/* Unknown sprite */
	if (itemIndex == -1)
	{
		return;
	}

	auto item = GetFreeItem();

	item->iId = itemIndex;
	item->type = HISTSLOT_ITEM;
	item->iCount = iCount;
	item->DisplayTime = gHUD.m_flTime;
}

void HistoryResource::DrawAmmoHistory(float time)
{
	const auto color = CHud::COLOR_PRIMARY;

	/* Draw the history in the lower right corner of the HUD, above the secondary ammo. */
	const auto x = gHUD.GetWidth() - 10;

	auto y = gHUD.GetHeight() - 99;

	for (auto i = 0; i < MAX_HISTORY; i++)
	{
		auto item = rgAmmoHistory + i;

		if (item->type == HISTSLOT_EMPTY)
		{
			break;
		}

		const auto deltaTime = time - item->DisplayTime;
		const auto displayTime = hud_drawhistory_time->value;

		if (deltaTime >= displayTime)
		{
			/* Display time has expired; Remove the current item from the list. */
			memmove(rgAmmoHistory + i, rgAmmoHistory + i + 1, sizeof(HIST_ITEM) * (MAX_HISTORY - i));
			i--;
			continue;
		}

		const auto alpha = CHudBase::kMaxAlpha * (1.0F - deltaTime / displayTime);

		switch (item->type)
		{
			case HISTSLOT_AMMO:
			{
				Rect ammoRect;
				const auto ammoSprite = *gWR.GetAmmoPicFromWeapon(item->iId, ammoRect);

				if (ammoSprite != 0)
				{
					gHUD.DrawHudSprite(
						ammoSprite,
						0,
						&ammoRect,
						x,
						y,
						color,
						alpha,
						CHud::a_southeast);

					/* Draw the count to the left of the icon. */
					int r, g, b;
					gHUD.GetColor(r, g, b, color);
					ScaleColors(r, g, b, alpha);

					char str[4];
					sprintf(str, "%d", item->iCount);

					gHUD.DrawHudStringReverse(x - 32, y - 16, x - 128, str, r, g, b);
				}

				y -= 32;
				break;
			}

			case HISTSLOT_WEAP:
			{
				auto weapon = gWR.GetWeapon(item->iId);

				if (weapon != nullptr)
				{
					const auto hasAmmo = gWR.HasAmmo(weapon);

					gHUD.DrawHudSprite(
						weapon->hInactive,
						0,
						&weapon->rcInactive,
						x,
						y,
						hasAmmo ? color : CHud::COLOR_WARNING,
						alpha,
						CHud::a_southeast);
				}

				y -= 70;
				break;
			}

			case HISTSLOT_ITEM:
			{
				if (item->iId != 0)
				{
					gHUD.DrawHudSpriteIndex(
						item->iId,
						x,
						y,
						color,
						alpha,
						CHud::a_southeast);
				}

				y -= 32;
				break;
			}
		}
	}
}
