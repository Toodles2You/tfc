//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Whatever
//
// $NoKeywords: $
//=============================================================================

#pragma once

#include <VGUI_Panel.h>
#include <VGUI_Image.h>
#include <VGUI_ImagePanel.h>
#include <VGUI_Bitmap.h>
#include <VGUI_BitmapTGA.h>

#include "steam_utils.h"

enum EAvatarSize
{
	k_EAvatarSize32x32 = 0,
	k_EAvatarSize64x64 = 1,
	k_EAvatarSize184x184 = 2,
};

class CAvatarImagePanel;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CAvatarImage : public vgui::Bitmap
{
public:
	friend class CAvatarImagePanel;

	CAvatarImage();

	bool SetAvatarSteamID(CSteamID steamIDUser, EAvatarSize avatarSize = k_EAvatarSize32x32);
	void UpdateFriendStatus();
	void ClearAvatarSteamID();

	bool IsValid() { return m_bValid; }

protected:
	void InitFromRGBA(const byte* rgba, int width, int height);

	bool IsLoadPending() { return m_bLoadPending; }
	void LoadAvatarImage();

private:
	bool m_bValid;
	bool m_bLoadPending;
	float m_fNextLoadTime; // Used to throttle load attempts

	EAvatarSize m_AvatarSize;
	CSteamID m_SteamID;

	CCallback<CAvatarImage, PersonaStateChange_t, false> m_sPersonaStateChangedCallback;

	void OnPersonaStateChanged(PersonaStateChange_t* info);
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CAvatarImagePanel : public vgui::ImagePanel
{
public:
	CAvatarImagePanel();

	// reset the image to its default value, clearing any info retrieved from Steam
	void ClearAvatar();

	// Set the player that this Avatar should display for

	// Overloaded function to go straight to entity index
	void SetPlayer(int entityIndex, EAvatarSize avatarSize = k_EAvatarSize32x32);

	// Lower level function that expects a steam ID instead of a player
	void SetPlayer(CSteamID steamIDForPlayer, EAvatarSize avatarSize = k_EAvatarSize32x32);

	// specify a fallback image to use
	void SetDefaultAvatar(vgui::Bitmap* pDefaultAvatar);

	void UpdateAvatar();

	bool IsValid() { return m_pImage->IsValid(); }

protected:
	void UpdateSize();

private:
	CAvatarImage* m_pImage;
	vgui::BitmapTGA *m_pFriendIcon;
	// Image to use as a fallback when get from Steam fails (or not called)
	vgui::Bitmap* m_pDefaultImage;
	bool m_bSizeDirty;
};
