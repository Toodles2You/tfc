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

#pragma once

inline int gmsgShake = 0;
inline int gmsgFade = 0;
inline int gmsgResetHUD = 0;
inline int gmsgInitHUD = 0;
inline int gmsgHealth = 0;
inline int gmsgDamage = 0;
inline int gmsgBattery = 0;
#ifdef HALFLIFE_TRAINCONTROL
inline int gmsgTrain = 0;
#endif
inline int gmsgHudText = 0;
inline int gmsgDeathMsg = 0;
inline int gmsgScoreInfo = 0;
inline int gmsgExtraInfo = 0;
inline int gmsgTeamScore = 0;
inline int gmsgGameMode = 0;
inline int gmsgMOTD = 0;
inline int gmsgVGUIMenu = 0;
inline int gmsgServerName = 0;
inline int gmsgAmmoPickup = 0;
inline int gmsgWeapPickup = 0;
inline int gmsgItemPickup = 0;
inline int gmsgSetCurWeap = 0;
inline int gmsgSayText = 0;
inline int gmsgTextMsg = 0;
inline int gmsgShowMenu = 0;
inline int gmsgVoteMenu = 0;
inline int gmsgValClass = 0;
inline int gmsgTeamNames = 0;
inline int gmsgAllowSpec = 0;

inline int gmsgWeapons = 0;
inline int gmsgAmmo = 0;
inline int gmsgSecAmmoIcon = 0;

inline int gmsgHitFeedback = 0;
inline int gmsgBlood = 0;
inline int gmsgPredictedSound = 0;
inline int gmsgShooter = 0;
inline int gmsgLaserDot = 0;

inline int gmsgStatusIcon = 0;

void LinkUserMessages();
