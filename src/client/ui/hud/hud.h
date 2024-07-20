/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
//  hud.h
//
// class CHud declaration
//
// CHud handles the message, calculation, and drawing the HUD
//

#pragma once

#define RGB_YELLOWISH 0x00FFA000 //255,160,0
#define RGB_REDISH 0x00FF1010	 //255,160,0
#define RGB_GREENISH 0x0000A000	 //0,160,0

#include "common_types.h"
#include "cl_dll.h"
#include "ammo.h"
#include "const.h"

#define DHN_DRAWZERO 1
#define DHN_2DIGITS 2
#define DHN_3DIGITS 4
#define MIN_ALPHA 100.0F

#define HUDELEM_ACTIVE 1

#include "global_consts.h"

typedef struct
{
	unsigned char r, g, b, a;
} RGBA;

typedef struct cvar_s cvar_t;

#define MAX_PLAYER_NAME_LENGTH 32

#define MAX_MOTD_LENGTH 1536

//
//-----------------------------------------------------
//
class CHudBase
{
public:
	friend class CHud;
	friend class HistoryResource;
	virtual ~CHudBase() {}

protected:
	enum
	{
		kActive = 1,
	};

	/* Toodles TODO: Make these console variables. */
	constexpr static float kMinAlpha = 150.0F;
	constexpr static float kMaxAlpha = 225.0F;
	constexpr static float kFadeTime = 100.0F;

private:
	int m_iFlags;
	float m_fFade;

protected:
	virtual int GetAlpha() { return kMinAlpha + (kMaxAlpha - kMinAlpha) * (m_fFade / kFadeTime); }
	void Flash() { m_fFade = kFadeTime; }

public:
	virtual bool IsActive();

	bool SetActive(const bool active)
	{
		const auto alreadyActive = IsActive();

		if (active != alreadyActive)
		{
			if (active)
			{
				m_iFlags |= kActive;
				Flash();
			}
			else
			{
				m_iFlags &= ~kActive;
			}
		}
	}

	virtual bool Init();
	virtual void VidInit() {}
	virtual void Draw(const float time) {}
	virtual void Think();
	virtual bool ShouldReset(const bool reinitialize) { return true; }
	virtual void Reset() {}
};

class CHudStatus : public CHudBase
{
public:
	bool IsActive() override;
};

struct HUDLIST
{
	CHudBase* p;
	HUDLIST* pNext;
};



//
//-----------------------------------------------------
//
#include "voice_status.h" // base voice handling class
#include "hud_spectator.h"

class WeaponsResource;

//
//-----------------------------------------------------
//
class CHudAmmo : public CHudStatus
{
public:
	bool Init() override;
	void VidInit() override;
	void Draw(const float time) override;
	void Think() override;
	void Reset() override;
	void DrawWList(const float time);
	void Update_AmmoX(int iIndex, int iCount);
	void Update_CurWeapon(int iState, int iId, int iClip, bool bCanHolster);
	void Update_Detpack(const int setting);
	bool MsgFunc_AmmoPickup(const char* pszName, int iSize, void* pbuf);
	bool MsgFunc_WeapPickup(const char* pszName, int iSize, void* pbuf);
	bool MsgFunc_ItemPickup(const char* pszName, int iSize, void* pbuf);
	bool MsgFunc_HitFeedback(const char* pszName, int iSize, void* pbuf);

	void SlotInput(int iSlot);
	void UserCmd_Slot1();
	void UserCmd_Slot2();
	void UserCmd_Slot3();
	void UserCmd_Slot4();
	void UserCmd_Slot5();
	void UserCmd_Slot6();
	void UserCmd_Slot7();
	void UserCmd_Slot8();
	void UserCmd_Slot9();
	void UserCmd_Slot10();
	void UserCmd_Close();
	void UserCmd_NextWeapon();
	void UserCmd_PrevWeapon();
	void UserCmd_LastWeapon();

	friend class WeaponsResource;

protected:
	void DrawCrosshair(WEAPON *pWeapon, int a = 255, bool zoom = false, bool autoaim = false);
	int DrawBar(int x, int y, int width, int height, float f, int color, int a);
	void DrawAmmoBar(WEAPON* p, int x, int y, int width, int height, int color, int a);

	RGBA m_rgba;
	WEAPON* m_pWeapon;
	int m_HUD_bucket0;
	int m_HUD_selection;
	cvar_t* hud_fastswitch;
	cvar_t* hud_selection_fadeout;
	cvar_t* hud_selection_timeout;
	float m_flSelectionTime;
	float m_flHitFeedbackTime;
	bool m_bWeaponSelectDisabled;
};

//
//-----------------------------------------------------
//

class CHudAmmoSecondary : public CHudStatus
{
public:
	bool IsActive() override { return CHudBase::IsActive() && CHudStatus::IsActive(); }
	bool Init() override;
	void VidInit() override;
	void Draw(const float time) override;

	void Update_SecAmmoVal(int iIndex, int iCount);
	bool MsgFunc_SecAmmoVal(const char* pszName, int iSize, void* pbuf);
	bool MsgFunc_SecAmmoIcon(const char* pszName, int iSize, void* pbuf);

private:
	enum
	{
		MAX_SEC_AMMO_VALUES = 4
	};

	int m_HUD_ammoicon; // sprite indices
	int m_iAmmoAmounts[MAX_SEC_AMMO_VALUES];
};


#include "health.h"


#ifdef HALFLIFE_HUD_GEIGER

//
//-----------------------------------------------------
//
class CHudGeiger : public CHudBase
{
public:
	bool Init() override;
	void Draw(const float time) override;
	bool MsgFunc_Geiger(const char* pszName, int iSize, void* pbuf);

private:
	int m_iGeigerRange;
};

#endif

//
//-----------------------------------------------------
//
#ifdef HALFLIFE_TRAINCONTROL
class CHudTrain : public CHudBase
{
public:
	bool Init() override;
	void VidInit() override;
	void Draw(const float time) override;
	bool MsgFunc_Train(const char* pszName, int iSize, void* pbuf);

private:
	HSPRITE m_hSprite;
	int m_iPos;
};
#endif

//
//-----------------------------------------------------
//
class CHudStatusBar : public CHudBase
{
public:
	bool Init() override;
	void Draw(const float time) override;
	void Reset() override;

	void UpdateStatusBar(cl_entity_t* entity);

protected:
	enum
	{
		MAX_STATUSTEXT_LENGTH = 128,
	};

	char m_szStatusBar[MAX_STATUSTEXT_LENGTH];	 // the constructed bar that is drawn
	int m_targetIndex;
	float m_targetExpireTime;
	cvar_t *hud_expireid;
};

struct extra_player_info_t
{
	short score;
	short deaths;
	short playerclass;
	short health;
	bool dead;
	short teamnumber;
	char teamname[MAX_TEAM_NAME];
	bool lefthanded;
	bool bot;
};

struct team_info_t
{
	char name[MAX_TEAM_NAME];
	short score;
	bool ownteam;
	short players;
	bool already_drawn;
};

#include "player_info.h"

//
//-----------------------------------------------------
//
class CHudDeathNotice : public CHudBase
{
public:
	bool Init() override;
	bool ShouldReset(const bool reinitialize) override { return reinitialize; }
	void Reset() override;
	void VidInit() override;
	void Draw(const float time) override;
	bool MsgFunc_DeathMsg(const char* pszName, int iSize, void* pbuf);

private:
	int m_HUD_d_skull; // sprite index of skull icon
	int m_HUD_d_headshot;
	int m_string_height;
	int m_headshot_width;
	int m_headshot_height;
	cvar_t *hud_deathnotice_time;
};

//
//-----------------------------------------------------
//
class CHudMenu : public CHudBase
{
public:
	typedef enum {
		kNone = 0,
		kMenu,
		kVote,
	} menu_e;

	bool Init() override;
	void VidInit() override;
	bool ShouldReset(const bool reinitialize) override { return reinitialize; }
	void Reset() override;
	void Draw(const float time) override;
	bool MsgFunc_ShowMenu(const char* pszName, int iSize, void* pbuf);
	bool MsgFunc_VoteMenu(const char* pszName, int iSize, void* pbuf);

	void SelectMenuItem(int menu_item);

	menu_e m_fMenuDisplayed;

protected:
	int m_bitsValidSlots;
	float m_flShutoffTime;
	bool m_fWaitingForMore;
};

//
//-----------------------------------------------------
//
class CHudSayText : public CHudBase
{
public:
	bool Init() override;
	bool ShouldReset(const bool reinitialize) override { return reinitialize; }
	void Reset() override;
	void VidInit() override;
	void Draw(const float time) override;
	bool MsgFunc_SayText(const char* pszName, int iSize, void* pbuf);
	void SayTextPrint(const char* pszBuf, int clientIndex = -1);
	void EnsureTextFitsInOneLineAndWrapIfHaveTo(int line);
	friend class CHud;
	friend class CHudSpectator;

private:
	int m_iBaseX;
	int m_iBaseY;
	int m_iLineHeight;
	struct cvar_s* m_HUD_saytext;
	struct cvar_s* m_HUD_saytext_time;
	struct cvar_s* m_con_color;
};

//
//-----------------------------------------------------
//
class CHudBattery : public CHudStatus
{
public:
	friend class CHudHealth;
	
	bool Init() override;
	void VidInit() override;
	void Draw(const float time) override;
	void Update_Battery(int iBat, float flType = 0.0F);
	bool MsgFunc_Battery(const char* pszName, int iSize, void* pbuf);

private:
	HSPRITE m_hSuit;
	Rect m_rcSuit;
	int m_iBat;
	float m_flType;
	char m_szString[32];
};


//
//-----------------------------------------------------
//
class CHudFlashlight : public CHudStatus
{
public:
	bool Init() override;
	void VidInit() override;
	void Draw(const float time) override;
	void Reset() override;
	void Update_Flashlight(const bool on);
	bool IsFlashlightOn() { return m_fOn; }

private:
	bool m_fOn;
#ifdef HALFLIFE_HUD_FLASHLIGHT
	HSPRITE m_hSprite1;
	HSPRITE m_hSprite2;
	HSPRITE m_hBeam;
	Rect* m_prc1;
	Rect* m_prc2;
	Rect* m_prcBeam;
	float m_flBat;
	int m_iBat;
	int m_iWidth; // width of the battery innards
#endif
};

//
//-----------------------------------------------------
//
const int maxHUDMessages = 16;
struct message_parms_t
{
	client_textmessage_t* pMessage;
	float time;
	int x, y;
	int totalWidth, totalHeight;
	int width;
	int lines;
	int lineLength;
	int length;
	int r, g, b;
	int text;
	int fadeBlend;
	float charTime;
	float fadeTime;
};

//
//-----------------------------------------------------
//

class CHudTextMessage : public CHudBase
{
public:
	bool Init() override;
	static char* LocaliseTextString(const char* msg, char* dst_buffer, int buffer_size);
	static char* BufferedLocaliseTextString(const char* msg);
	const char* LookupString(const char* msg_name, int* msg_dest = nullptr);
	bool MsgFunc_TextMsg(const char* pszName, int iSize, void* pbuf);
};

//
//-----------------------------------------------------
//

class CHudMessage : public CHudBase
{
public:
	bool Init() override;
	void VidInit() override;
	void Draw(const float time) override;
	bool MsgFunc_HudText(const char* pszName, int iSize, void* pbuf);
	bool MsgFunc_HudTextPro(const char* pszName, int iSize, void* pbuf);

	float FadeBlend(float fadein, float fadeout, float hold, float localTime);
	int XPosition(float x, int width, int lineWidth);
	int YPosition(float y, int height);

	void MessageAdd(const char* pName, float time);
	void MessageAdd(client_textmessage_t* newMessage);
	void MessageDrawScan(client_textmessage_t* pMessage, float time);
	void MessageScanStart();
	void MessageScanNextChar();
	bool ShouldReset(const bool reinitialize) { return reinitialize; }
	void Reset() override;

private:
	client_textmessage_t* m_pMessages[maxHUDMessages];
	float m_startTime[maxHUDMessages];
	message_parms_t m_parms;
};

//
//-----------------------------------------------------
//
#define MAX_SPRITE_NAME_LENGTH 24

class CHudStatusIcons : public CHudStatus
{
public:
	bool Init() override;
	void VidInit() override;
	void Reset() override;
	void Draw(const float time) override;
	bool MsgFunc_StatusIcon(const char* pszName, int iSize, void* pbuf);

	enum
	{
		MAX_ICONSPRITENAME_LENGTH = MAX_SPRITE_NAME_LENGTH,
		MAX_ICONSPRITES = 4,
	};


	//had to make these public so CHud could access them (to enable concussion icon)
	//could use a friend declaration instead...
	void EnableIcon(const char* pszIconName, int red = 0, int green = 0, int blue = 0);
	void DisableIcon(const char* pszIconName);

private:
#ifdef HALFLIFE_GRENADES
	bool DrawTimer(float flTime);
#endif

	typedef struct
	{
		char szSpriteName[MAX_ICONSPRITENAME_LENGTH];
		HSPRITE spr;
		Rect rc;
		unsigned char r, g, b;
	} icon_sprite_t;

	icon_sprite_t m_IconList[MAX_ICONSPRITES];

#ifdef HALFLIFE_GRENADES
	bool m_bTimerActive;
	float m_flTimerStart;

	HSPRITE m_hTimer;
	
	HSPRITE m_hGrenade;
	Rect m_rcGrenade;
#endif
};

//
//-----------------------------------------------------
//


class CHud
{
private:
	HUDLIST* m_pHudList;
	client_sprite_t* m_pSpriteList;
	int m_iSpriteCount;
	int m_iSpriteCountAllRes;
	float m_flMouseSensitivity;
	int m_iConcussionEffect;
	unsigned int m_iConWidth;
	unsigned int m_iConHeight;
	float m_flScaleX;
	float m_flScaleY;
	float m_flOffsetX;
	float m_flOffsetY;
	byte m_bIsWidescreen;
	HSPRITE m_hSniperScope;

public:
	float m_flTime;		  // the current client time
	float m_fOldTime;	  // the time at which the HUD was last redrawn
	double m_flTimeDelta; // the difference between flTime and fOldTime
	Vector m_vecOrigin;
	Vector m_vecAngles;
	int m_iKeyBits;
	int m_iFOV;
	bool m_bTranquilized;
	float m_flOverrideFOV;
	float m_flOverrideSensitivity;
	gamemode_e m_gameMode;
	int m_iRes;
	HSPRITE m_hBackground;
	cvar_t* m_pCvarStealMouse;
	cvar_t* m_pCvarDraw;
	cvar_t* m_pCvarWidescreen;
	cvar_t* m_pCvarColor;
	cvar_t *m_pCvarTeamColor;
	cvar_t *m_pCvarCrosshair;
	cvar_t *m_pCvarSuitVolume;

	typedef enum {
		COLOR_DEFAULT = 0,
		COLOR_PRIMARY,
		COLOR_SECONDARY,
		COLOR_WARNING,
		COLOR_COUNT
	} hudcolor_e;

	typedef enum {
		a_northwest = 0,
		a_north,
		a_northeast,
		a_west,
		a_center,
		a_east,
		a_southwest,
		a_south,
		a_southeast,
	} hudalign_e;

	int m_iFontHeight;
	void DrawHudSprite(HSPRITE pic, int frame, Rect *rect, int x, int y, int r, int g, int b, int a = 255, hudalign_e alignment = a_northwest);
	void DrawHudSprite(HSPRITE pic, int frame, Rect *rect, int x, int y, hudcolor_e color, int a = 255, hudalign_e alignment = a_northwest);
	void DrawHudSpriteIndex(int index, int x, int y, hudcolor_e color, int a = 255, hudalign_e alignment = a_northwest);
	void DrawHudSpriteIndex(int index, int x, int y, int r, int g, int b, int a = 255, hudalign_e alignment = a_northwest);
	void DrawHudFill(int x, int y, int w, int h, int r, int g, int b, int a);
	void DrawHudFill(int x, int y, int w, int h, hudcolor_e color, int a);
	int DrawHudNumber(int x, int y, int iFlags, int iNumber, int r, int g, int b, int a = 255, hudalign_e alignment = a_northwest);
	int DrawHudNumber(int x, int y, int iFlags, int iNumber, hudcolor_e color, int a = 255, hudalign_e alignment = a_northwest);
	int DrawHudString(int x, int y, int iMaxX, const char* szString, int r, int g, int b);
	int DrawHudStringReverse(int xpos, int ypos, int iMinX, const char* szString, int r, int g, int b);
	int DrawHudNumberString(int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b);
	int GetNumWidth(int iNumber, int iFlags);

	int GetHudNumberWidth(int number, int width, int flags);
	int DrawHudNumberReverse(int x, int y, int number, int flags, int r, int g, int b, int a = 255, hudalign_e alignment = a_northwest);
	int DrawHudNumberReverse(int x, int y, int number, int flags, hudcolor_e color, int a = 255, hudalign_e alignment = a_northwest);

	int DrawHudString(const char* string, int x, int y);
	void GetHudStringSize(const char* string, int& width, int& height);
	int HudStringLen(const char* string);

	void DrawHudBackground(int left, int top, int right, int bottom, const bool highlight = false);

	void RedrawZoomOverlay(float time);

	void DrawWorldSprite(HSPRITE pic, int frame, Rect *rect, Vector origin, hudcolor_e color, int a);

	void GetChatInputPosition(int& x, int& y);

	bool HasWeapon(int id) const
	{
		return (m_iWeaponBits & (1ULL << id)) != 0;
	}

	bool HasAnyWeapons() const
	{
		return m_iWeaponBits != 0;
	}

	inline unsigned int GetWidth() const
	{
		return m_iConWidth;
	}

	inline unsigned int GetHeight() const
	{
		return m_iConHeight;
	}

	inline void GetColor(int &r, int &g, int &b, hudcolor_e color) const
	{
		r = m_cColors[color].r;
		g = m_cColors[color].g;
		b = m_cColors[color].b;
	}

private:
	// the memory for these arrays are allocated in the first call to CHud::VidInit(), when the hud.txt and associated sprites are loaded.
	// freed in ~CHud()
	HSPRITE* m_rghSprites; /*[HUD_SPRITE_COUNT]*/ // the sprites loaded from hud.txt
	Rect* m_rgrcRects;							  /*[HUD_SPRITE_COUNT]*/
	char* m_rgszSpriteNames;					  /*[HUD_SPRITE_COUNT][MAX_SPRITE_NAME_LENGTH]*/

	cvar_t *cl_fov;
	cvar_t *zoom_sensitivity_ratio;

public:
	HSPRITE GetSprite(int index)
	{
		return (index < 0) ? 0 : m_rghSprites[index];
	}

	Rect& GetSpriteRect(int index)
	{
		return m_rgrcRects[index];
	}


	int GetSpriteIndex(const char* SpriteName); // gets a sprite index, for use in the m_rghSprites[] array

	CHudAmmo m_Ammo;
	CHudHealth m_Health;
	CHudSpectator m_Spectator;
#ifdef HALFLIFE_HUD_GEIGER
	CHudGeiger m_Geiger;
#endif
	CHudBattery m_Battery;
#ifdef HALFLIFE_TRAINCONTROL
	CHudTrain m_Train;
#endif
	CHudFlashlight m_Flash;
	CHudMessage m_Message;
	CHudStatusBar m_StatusBar;
	CHudDeathNotice m_DeathNotice;
	CHudSayText m_SayText;
	CHudMenu m_Menu;
	CHudAmmoSecondary m_AmmoSecondary;
	CHudTextMessage m_TextMessage;
	CHudStatusIcons m_StatusIcons;

	void Init();
	void VidInit();
	void Think();
	bool Redraw(float flTime, bool intermission);
	bool UpdateClientData(client_data_t* cdata, float time);

	CHud() : m_iSpriteCount(0), m_pHudList(nullptr) {}
	~CHud(); // destructor, frees allocated memory

	void Update_SetFOV(int iFov);
	void Update_Concuss(int iConcuss);
	void Update_Tranquilization(const bool bTranquilized);
	// user messages
	bool MsgFunc_GameMode(const char* pszName, int iSize, void* pbuf);
	bool MsgFunc_ResetHUD(const char* pszName, int iSize, void* pbuf);
	void MsgFunc_InitHUD(const char* pszName, int iSize, void* pbuf);
	void MsgFunc_ViewMode(const char* pszName, int iSize, void* pbuf);
	bool MsgFunc_Concuss(const char* pszName, int iSize, void* pbuf);
	bool MsgFunc_Weapons(const char* pszName, int iSize, void* pbuf);
	bool MsgFunc_Ammo(const char* pszName, int iSize, void* pbuf);

	// Screen information
	SCREENINFO m_scrinfo;

	std::uint64_t m_iWeaponBits;
	bool m_iIntermission;
	RGBA m_cColors[CHud::COLOR_COUNT];

	// sprite indexes
	int m_HUD_number_0;


	void AddHudElem(CHudBase* p);

	float GetDefaultFOV();
	float GetDefaultSensitivity();

	float GetSensitivity();

	bool ImpulseCommands(int impulse);

	const Vector& GetTeamColor(int teamNumber) const;
	const Vector& GetClientColor(int clientIndex) const;

	bool IsAlive();
	int GetObserverMode();
	int GetObserverTarget();
	int GetObserverTarget2();
	bool IsObserver();
	bool IsSpectator();

	bool IsViewZoomed();
};

extern CHud gHUD;

extern int g_iPlayerClass;
extern int g_iTeamNumber;
