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

	gmsgHealth = engine::RegUserMsg("Health", 2);
	gmsgDamage = engine::RegUserMsg("Damage", 12);
	gmsgBattery = engine::RegUserMsg("Battery", 2);
#ifdef HALFLIFE_TRAINCONTROL
	gmsgTrain = engine::RegUserMsg("Train", 1);
#endif
	//gmsgHudText = engine::RegUserMsg( "HudTextPro", -1 );
	gmsgHudText = engine::RegUserMsg("HudText", -1); // we don't use the message but 3rd party addons may!
	gmsgSayText = engine::RegUserMsg("SayText", -1);
	gmsgTextMsg = engine::RegUserMsg("TextMsg", -1);
	gmsgResetHUD = engine::RegUserMsg("ResetHUD", 0); // called every respawn
	gmsgInitHUD = engine::RegUserMsg("InitHUD", 0);	// called every time a new player joins the server
	gmsgDeathMsg = engine::RegUserMsg("DeathMsg", -1);
	gmsgScoreInfo = engine::RegUserMsg("ScoreInfo", 5);
	gmsgExtraInfo = engine::RegUserMsg("ExtraInfo", 3);
	gmsgTeamScore = engine::RegUserMsg("TeamScore", 3); // sets the score of a team on the scoreboard
	gmsgGameMode = engine::RegUserMsg("GameMode", 1);
	gmsgMOTD = engine::RegUserMsg("MOTD", -1);
	gmsgVGUIMenu = engine::RegUserMsg("VGUIMenu", 1);
	gmsgServerName = engine::RegUserMsg("ServerName", -1);
	gmsgAmmoPickup = engine::RegUserMsg("AmmoPickup", 2);
	gmsgWeapPickup = engine::RegUserMsg("WeapPickup", 1);
	gmsgItemPickup = engine::RegUserMsg("ItemPickup", -1);
	gmsgShowMenu = engine::RegUserMsg("ShowMenu", -1);
	gmsgVoteMenu = engine::RegUserMsg("VoteMenu", -1);
	gmsgShake = engine::RegUserMsg("ScreenShake", sizeof(ScreenShake));
	gmsgFade = engine::RegUserMsg("ScreenFade", sizeof(ScreenFade));
	gmsgValClass = engine::RegUserMsg("ValClass", 10);
	gmsgTeamNames = engine::RegUserMsg("TeamNames", -1);
	gmsgAllowSpec = engine::RegUserMsg("AllowSpec", 1);

	gmsgWeapons = engine::RegUserMsg("Weapons", 8);
	gmsgAmmo = engine::RegUserMsg("Ammo", AMMO_TYPES);
	gmsgSecAmmoIcon = engine::RegUserMsg("SecAmmoIcon", -1);

	gmsgHitFeedback = engine::RegUserMsg("HitFeedback", 4);
	gmsgBlood = engine::RegUserMsg("Blood", -1);
	gmsgLaserDot = engine::RegUserMsg("Laser", 1);
	gmsgPredictedSound = engine::RegUserMsg("PredSound", -1);
	gmsgShooter = engine::RegUserMsg("Shooter", -1);

	gmsgStatusIcon = engine::RegUserMsg("StatusIcon", -1);
}
