//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Whatever
//
// $NoKeywords: $
//=============================================================================

#include "vgui_avatarimage.h"
#include "vgui_loadtga.h"

#include "hud.h"
#include "cl_util.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_ScorePanel.h"

CAvatarImage::CAvatarImage()
	: m_sPersonaStateChangedCallback(this, &CAvatarImage::OnPersonaStateChanged)
{
	ClearAvatarSteamID();
	m_bLoadPending = false;
	m_fNextLoadTime = 0.0f;
	m_AvatarSize = k_EAvatarSize32x32;

	setSize(32, 32);
}

//-----------------------------------------------------------------------------
// Purpose: reset the image to a default state (will render with the default image)
//-----------------------------------------------------------------------------
void CAvatarImage::ClearAvatarSteamID()
{
	m_bValid = false;
	m_bLoadPending = false;
	m_fNextLoadTime = 0.0f;
	m_SteamID.Set(0, k_EUniverseInvalid, k_EAccountTypeInvalid);
	m_sPersonaStateChangedCallback.Unregister();
}

//-----------------------------------------------------------------------------
// Purpose: Set the CSteamID for this image; this will cause a deferred load
//-----------------------------------------------------------------------------
bool CAvatarImage::SetAvatarSteamID(CSteamID steamIDUser, EAvatarSize avatarSize)
{
	ClearAvatarSteamID();

	m_SteamID = steamIDUser;
	m_AvatarSize = avatarSize;
	m_bLoadPending = true;
	m_fNextLoadTime = 0.0f;

	switch (m_AvatarSize)
	{
		case k_EAvatarSize32x32:
			setSize(32, 32);
			break;
		case k_EAvatarSize64x64:
			setSize(64, 64);
			break;
		case k_EAvatarSize184x184:
			setSize(184, 184);
			break;
	}

	m_sPersonaStateChangedCallback.Register(this, &CAvatarImage::OnPersonaStateChanged);

	LoadAvatarImage();
	UpdateFriendStatus();

	return m_bValid;
}

//-----------------------------------------------------------------------------
// Purpose: Called when somebody changes their avatar image
//-----------------------------------------------------------------------------
void CAvatarImage::OnPersonaStateChanged(PersonaStateChange_t* info)
{
	if ((info->m_ulSteamID == m_SteamID.ConvertToUint64()) && (info->m_nChangeFlags & k_EPersonaChangeAvatar))
	{
		// Mark us as invalid.
		m_bValid = false;
		m_bLoadPending = true;
		m_fNextLoadTime = 0.0f;

		// Poll
		LoadAvatarImage();
	}
}

//-----------------------------------------------------------------------------
// Purpose: load the avatar image if we have a load pending
//-----------------------------------------------------------------------------
void CAvatarImage::LoadAvatarImage()
{
	// attempt to retrieve the avatar image from Steam
	if (m_bLoadPending && gHUD.m_flTime >= m_fNextLoadTime)
	{
		if (!g_pSteamContext->SteamFriends()->RequestUserInformation(m_SteamID, false))
		{
			int iAvatar = 0;
			switch (m_AvatarSize)
			{
			case k_EAvatarSize32x32:
				iAvatar = g_pSteamContext->SteamFriends()->GetSmallFriendAvatar(m_SteamID);
				break;
			case k_EAvatarSize64x64:
				iAvatar = g_pSteamContext->SteamFriends()->GetMediumFriendAvatar(m_SteamID);
				break;
			case k_EAvatarSize184x184:
				iAvatar = g_pSteamContext->SteamFriends()->GetLargeFriendAvatar(m_SteamID);
				break;
			}

			// Msg( "Got avatar %d for SteamID %llud (%s)\n", iAvatar, m_SteamID.ConvertToUint64(), g_pSteamContext->SteamFriends()->GetFriendPersonaName( m_SteamID ) );

			if (iAvatar > 0) // if its zero, user doesn't have an avatar.  If -1, Steam is telling us that it's fetching it
			{
				uint32 wide = 0, tall = 0;
				if (g_pSteamContext->SteamUtils()->GetImageSize(iAvatar, &wide, &tall) && wide > 0 && tall > 0)
				{
					int destBufferSize = wide * tall * 4;
					byte* rgbDest = (byte*)malloc(destBufferSize);

					if (g_pSteamContext->SteamUtils()->GetImageRGBA(iAvatar, rgbDest, destBufferSize))
                    {
						InitFromRGBA(rgbDest, wide, tall);
                    }

					free(rgbDest);
				}
			}
		}

		if (m_bValid)
		{
			// if we have a valid image, don't attempt to load it again
			m_bLoadPending = false;
		}
		else
		{
			// otherwise schedule another attempt to retrieve the image
			m_fNextLoadTime = gHUD.m_flTime + 1.0f;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Query Steam to set the m_bFriend status flag
//-----------------------------------------------------------------------------
void CAvatarImage::UpdateFriendStatus()
{
	if (!m_SteamID.IsValid())
	{
		return;
	}

	// 	m_bFriend = g_pSteamContext->SteamFriends()->HasFriend(m_SteamID, k_EFriendFlagImmediate);
}

//-----------------------------------------------------------------------------
// Purpose: Initialize the surface with the supplied raw RGBA image data
//-----------------------------------------------------------------------------
void CAvatarImage::InitFromRGBA(const byte* rgba, int width, int height)
{
    setSize(width, height);
	memcpy(_rgba, rgba, width * height * 4);
	m_bValid = true;
}

CAvatarImagePanel::CAvatarImagePanel() : ImagePanel()
{
	m_pImage = new CAvatarImage();
	m_bSizeDirty = true;

	// set up friend icon
	m_pFriendIcon = vgui_LoadTGA("resource/icon_friend_indicator_scoreboard.tga");
	m_pDefaultImage = vgui_LoadTGA("resource/avatar.tga");
}

//-----------------------------------------------------------------------------
// Purpose: Set the avatar by entity number
//-----------------------------------------------------------------------------
void CAvatarImagePanel::SetPlayer(int entindex, EAvatarSize avatarSize)
{
    auto steamID = g_PlayerInfoList[entindex].m_nSteamID;

    if (m_pImage->m_SteamID == steamID)
    {
        return;
    }

    if (steamID != 0)
    {
        CSteamID steamIDForPlayer(steamID, 1, g_pSteamContext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual);
        SetPlayer(steamIDForPlayer, avatarSize);
    }
    else
    {
        m_pImage->ClearAvatarSteamID();
    }
}

//-----------------------------------------------------------------------------
// Purpose: Set the avatar by SteamID
//-----------------------------------------------------------------------------
void CAvatarImagePanel::SetPlayer(CSteamID steamIDForPlayer, EAvatarSize avatarSize)
{
	if (m_pImage->m_SteamID == steamIDForPlayer)
	{
		return;
	}

	if (steamIDForPlayer.GetAccountID() != 0)
    {
		m_pImage->SetAvatarSteamID(steamIDForPlayer, avatarSize);
    }
	else
	{
		m_pImage->ClearAvatarSteamID();
	}
}

void CAvatarImagePanel::UpdateAvatar()
{
	if (m_bSizeDirty)
    {
		UpdateSize();
    }

	if (m_pImage->IsLoadPending())
	{
		m_pImage->LoadAvatarImage();
	}

	if (ScoreBoard::ShowPlayerAvatars() && m_pImage->IsValid())
	{
		setImage(m_pImage);
	}
	else
	{
		setImage(m_pDefaultImage);
	}
}

void CAvatarImagePanel::ClearAvatar()
{
	m_pImage->ClearAvatarSteamID();
}

void CAvatarImagePanel::SetDefaultAvatar(vgui::Bitmap* pDefaultAvatar)
{
	m_pDefaultImage = pDefaultAvatar;
}

void CAvatarImagePanel::UpdateSize()
{
    // the image is in charge of the panel size
    int wide, tall;
	m_pImage->getSize(wide, tall);

    setSize(wide, tall);

	m_bSizeDirty = false;
}
