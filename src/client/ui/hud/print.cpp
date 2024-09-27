//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Yapping
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

#include "vgui_TeamFortressViewport.h"

bool CHudPrint::Init ()
{
    m_centerPrint[kMaxCenterPrintLength - 1] = '\0';

    Reset ();

	return CHudBase::Init ();
}

void CHudPrint::Draw (const float time)
{
    if (m_expireTime <= time)
    {
        Reset ();

        return;
    }

	/* Draw the central print bar! */

    client::DrawSetTextColor (1.0F, 0.7F, 0.0F);

	gHUD.DrawHudBackground (
        m_extents.left, m_extents.bottom,
        m_extents.right, m_extents.top);

	gHUD.DrawHudString (m_centerPrint,
        m_extents.left, m_extents.bottom);
}

void CHudPrint::Reset ()
{
    m_centerPrint[0] = '\0';

	m_expireTime = 0.0F;

    SetActive (false);
}

void CHudPrint::CenterPrint (const char* string, const float duration)
{
    if (string == nullptr || string[0] == '\0' || duration <= 0.0F)
    {
        Reset ();

        return;
    }

    /* Copy the text. */

    const auto len = std::min (strlen (string),
        static_cast<size_t>(kMaxCenterPrintLength - 1));

    strncpy (m_centerPrint, string, len);

    /* Strip trailing white space. */

    for (int i = len - 1; i >= 0; i--)
    {
        if (!isspace (m_centerPrint[i]))
        {
            m_centerPrint[i + 1] = '\0';

            break;
        }
    }

    m_expireTime = client::GetClientTime () + duration;

	/* Get the text size. */

    int w, h;
	gHUD.GetHudStringSize (m_centerPrint, w, h);

    if (w == 0 || h == 0)
    {
        Reset ();

        return;
    }

	/* Center the text. */

	m_extents.left = std::max (0, ((int)gHUD.GetWidth () >> 1) - (w >> 1));
	m_extents.bottom = std::max (0, ((int)gHUD.GetHeight () >> 2) - (h >> 1));

    m_extents.right = m_extents.left + w;
    m_extents.top = m_extents.bottom + h;

    SetActive (true);
}
