/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "shake.h"
#include "UserMessages.h"

void LinkUserMessages()
{
	// Already taken care of?
	if (0 != gmsgInitHUD)
	{
		return;
	}

	gmsgHealth = g_engfuncs.pfnRegUserMsg("Health", 2);
	gmsgDamage = g_engfuncs.pfnRegUserMsg("Damage", 12);
	gmsgBattery = g_engfuncs.pfnRegUserMsg("Battery", 2);
#ifdef HALFLIFE_TRAINCONTROL
	gmsgTrain = g_engfuncs.pfnRegUserMsg("Train", 1);
#endif
	//gmsgHudText = g_engfuncs.pfnRegUserMsg( "HudTextPro", -1 );
	gmsgHudText = g_engfuncs.pfnRegUserMsg("HudText", -1); // we don't use the message but 3rd party addons may!
	gmsgSayText = g_engfuncs.pfnRegUserMsg("SayText", -1);
	gmsgTextMsg = g_engfuncs.pfnRegUserMsg("TextMsg", -1);
	gmsgResetHUD = g_engfuncs.pfnRegUserMsg("ResetHUD", 0); // called every respawn
	gmsgInitHUD = g_engfuncs.pfnRegUserMsg("InitHUD", 0);	// called every time a new player joins the server
	gmsgShowGameTitle = g_engfuncs.pfnRegUserMsg("GameTitle", 1);
	gmsgDeathMsg = g_engfuncs.pfnRegUserMsg("DeathMsg", -1);
	gmsgScoreInfo = g_engfuncs.pfnRegUserMsg("ScoreInfo", 5);
	gmsgExtraInfo = g_engfuncs.pfnRegUserMsg("ExtraInfo", 3);
	gmsgTeamScore = g_engfuncs.pfnRegUserMsg("TeamScore", 3); // sets the score of a team on the scoreboard
	gmsgGameMode = g_engfuncs.pfnRegUserMsg("GameMode", 1);
	gmsgMOTD = g_engfuncs.pfnRegUserMsg("MOTD", -1);
	gmsgVGUIMenu = g_engfuncs.pfnRegUserMsg("VGUIMenu", 1);
	gmsgServerName = g_engfuncs.pfnRegUserMsg("ServerName", -1);
	gmsgAmmoPickup = g_engfuncs.pfnRegUserMsg("AmmoPickup", 2);
	gmsgWeapPickup = g_engfuncs.pfnRegUserMsg("WeapPickup", 1);
	gmsgItemPickup = g_engfuncs.pfnRegUserMsg("ItemPickup", -1);
	gmsgShowMenu = g_engfuncs.pfnRegUserMsg("ShowMenu", -1);
	gmsgVoteMenu = g_engfuncs.pfnRegUserMsg("VoteMenu", -1);
	gmsgShake = g_engfuncs.pfnRegUserMsg("ScreenShake", sizeof(ScreenShake));
	gmsgFade = g_engfuncs.pfnRegUserMsg("ScreenFade", sizeof(ScreenFade));
	gmsgTeamNames = g_engfuncs.pfnRegUserMsg("TeamNames", -1);
	gmsgAllowSpec = g_engfuncs.pfnRegUserMsg("AllowSpec", 1);

	gmsgWeapons = g_engfuncs.pfnRegUserMsg("Weapons", 8);
	gmsgAmmo = g_engfuncs.pfnRegUserMsg("Ammo", AMMO_TYPES);

	gmsgHitFeedback = g_engfuncs.pfnRegUserMsg("HitFeedback", 4);
	gmsgBlood = g_engfuncs.pfnRegUserMsg("Blood", -1);
	gmsgPredictedSound = g_engfuncs.pfnRegUserMsg("PredSound", -1);

	gmsgStatusIcon = g_engfuncs.pfnRegUserMsg("StatusIcon", -1);
}
