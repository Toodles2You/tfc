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
// hud_redraw.cpp
//
#include "hud.h"
#include "cl_util.h"
#include "com_model.h"
#include "r_studioint.h"
#include "triangleapi.h"

#include "vgui_TeamFortressViewport.h"

extern engine_studio_api_t IEngineStudio;

float HUD_GetFOV();

// Think
void CHud::Think()
{
	m_scrinfo.iSize = sizeof(m_scrinfo);
	GetScreenInfo(&m_scrinfo);

	int newfov;
	HUDLIST* pList = m_pHudList;

	while (pList)
	{
		if ((pList->p->m_iFlags & HUD_ACTIVE) != 0)
			pList->p->Think();
		pList = pList->pNext;
	}

	HUD_GetFOV();
}

// Redraw
// step through the local data,  placing the appropriate graphics & text as appropriate
// returns 1 if they've changed, 0 otherwise
bool CHud::Redraw(float flTime, bool intermission)
{
	m_fOldTime = m_flTime; // save time of previous redraw
	m_flTime = flTime;
	m_flTimeDelta = (double)m_flTime - m_fOldTime;
	static float m_flShotTime = 0;

	// Clock was reset, reset delta
	if (m_flTimeDelta < 0)
		m_flTimeDelta = 0;

	gEngfuncs.pTriAPI->CullFace(TRI_FRONT);
	
	bool bWantWidescreen = m_pCvarWidescreen->value != 0.0F;

	if (bWantWidescreen != m_bIsWidescreen)
	{
		if (bWantWidescreen)
		{
			m_iConWidth = m_iConHeight * (ScreenWidth / (float)ScreenHeight);
		}
		else
		{
			m_iConWidth = m_iRes;
		}
		m_flOffsetX = (ScreenWidth - m_iConWidth * m_flScaleX) / 2.0F;
		m_bIsWidescreen = bWantWidescreen;
	}

	if (m_gameMode >= kGamemodeTeamplay && m_pCvarTeamColor->value != 0)
	{
		auto color = GetTeamColor(g_iTeamNumber);
		m_cColors[CHud::COLOR_PRIMARY].r = color[0] * 255;
		m_cColors[CHud::COLOR_PRIMARY].g = color[1] * 255;
		m_cColors[CHud::COLOR_PRIMARY].b = color[2] * 255;
	}
	else
	{
		auto ulRGB = strtoul(m_pCvarColor->string, NULL, 16);
		m_cColors[CHud::COLOR_PRIMARY].r = (ulRGB & 0xFF0000) >> 16;
		m_cColors[CHud::COLOR_PRIMARY].g = (ulRGB & 0xFF00) >> 8;
		m_cColors[CHud::COLOR_PRIMARY].b = (ulRGB & 0xFF);
	}

	// Bring up the scoreboard during intermission
	if (gViewPort)
	{
		if (m_iIntermission && !intermission)
		{
			// Have to do this here so the scoreboard goes away
			m_iIntermission = intermission;
			gViewPort->HideCommandMenu();
			gViewPort->HideScoreBoard();
			gViewPort->UpdateSpectatorPanel();
		}
		else if (!m_iIntermission && intermission)
		{
			m_iIntermission = intermission;
			gViewPort->HideCommandMenu();
			gViewPort->HideVGUIMenu();
			gViewPort->ShowScoreBoard();
			gViewPort->UpdateSpectatorPanel();

			// Take a screenshot if the client's got the cvar set
			if (CVAR_GET_FLOAT("hud_takesshots") != 0)
				m_flShotTime = flTime + 1.0; // Take a screenshot in a second
		}
	}

	if (0 != m_flShotTime && m_flShotTime < flTime)
	{
		gEngfuncs.pfnClientCmd("snapshot\n");
		m_flShotTime = 0;
	}

	m_iIntermission = intermission;

	// draw all registered HUD elements
	if (0 != m_pCvarDraw->value)
	{
		HUDLIST* pList = m_pHudList;

		while (pList)
		{
			if (!intermission)
			{
				if ((pList->p->m_iFlags & HUD_ACTIVE) != 0 && (m_iHideHUDDisplay & HIDEHUD_ALL) == 0)
					pList->p->Draw(flTime);
			}
			else
			{ // it's an intermission,  so only draw hud elements that are set to draw during intermissions
				if ((pList->p->m_iFlags & HUD_INTERMISSION) != 0)
					pList->p->Draw(flTime);
			}

			pList = pList->pNext;
		}
	}

	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

	return true;
}

void ScaleColors(int& r, int& g, int& b, int a)
{
	float x = (float)a / 255;
	r = (int)(r * x);
	g = (int)(g * x);
	b = (int)(b * x);
}

void CHud::DrawHudSprite(HSPRITE pic, int frame, Rect *rect, int x, int y, int r, int g, int b, int a, hudalign_e alignment)
{
	auto sprw = gEngfuncs.pfnSPR_Width (pic, frame);
	auto sprh = gEngfuncs.pfnSPR_Height (pic, frame);

	if (!rect)
	{
		static Rect rc;
		rc.left = 0;
		rc.right = sprw;
		rc.top = 0;
		rc.bottom = sprh;
		rect = &rc;
	}

	float xf = x;
	float yf = y;
	auto width = rect->right - rect->left;
	auto height = rect->bottom - rect->top;
	
	switch (alignment)
	{
	case a_north:
	case a_center:
	case a_south:
		xf -= width / 2.0F - 0.5F;
		break;
	case a_northeast:
	case a_east:
	case a_southeast:
		xf -= width;
		break;
	}

	switch (alignment)
	{
	case a_west:
	case a_center:
	case a_east:
		yf -= height / 2.0F - 0.5F;
		break;
	case a_southwest:
	case a_south:
	case a_southeast:
		yf -= height;
		break;
	}

	if (!IEngineStudio.IsHardware())
	{
		x += m_flOffsetX;
		y += m_flOffsetY;

		ScaleColors(r, g, b, a);
		gEngfuncs.pfnSPR_Set(pic, r, g, b);
		
		// Toodles FIXME: Hack for the crosshair.
		if (alignment == a_center)
		{
			gEngfuncs.pfnSPR_DrawHoles(frame, x, y, rect);
		}
		else
		{
			gEngfuncs.pfnSPR_DrawAdditive(frame, x, y, rect);
		}
		return;
	}
	
	auto pSprite = const_cast<model_t *>(gEngfuncs.GetSpritePointer (pic));

	auto x1 = roundf (m_flOffsetX + xf * m_flScaleX);
	auto y1 = roundf (m_flOffsetY + yf * m_flScaleY);
	auto x2 = roundf (m_flOffsetX + (xf + width) * m_flScaleX);
	auto y2 = roundf (m_flOffsetY + (yf + height) * m_flScaleY);

	auto left = rect->left / (float)sprw;
	auto right = rect->right / (float)sprw;
	auto top = rect->top / (float)sprh;
	auto bottom = rect->bottom / (float)sprh;
	
	gEngfuncs.pTriAPI->SpriteTexture (pSprite, frame);

	auto rendermode = kRenderTransAdd;

	// Toodles FIXME: Hack for the crosshair.
	if (alignment == a_center)
	{
		rendermode = kRenderTransAlpha;
	}

	gEngfuncs.pTriAPI->Color4fRendermode (r / 255.0F, g / 255.0F, b / 255.0F, a / 255.0F, rendermode);
	gEngfuncs.pTriAPI->RenderMode (rendermode);

	gEngfuncs.pTriAPI->Begin (TRI_QUADS);

	gEngfuncs.pTriAPI->TexCoord2f (left, top);
	gEngfuncs.pTriAPI->Vertex3f (x1, y1, 0);

	gEngfuncs.pTriAPI->TexCoord2f (right, top);
	gEngfuncs.pTriAPI->Vertex3f (x2, y1, 0);

	gEngfuncs.pTriAPI->TexCoord2f (right, bottom);
	gEngfuncs.pTriAPI->Vertex3f (x2, y2, 0);

	gEngfuncs.pTriAPI->TexCoord2f (left, bottom);
	gEngfuncs.pTriAPI->Vertex3f (x1, y2, 0);

	gEngfuncs.pTriAPI->End ();
}

void CHud::DrawHudSprite(HSPRITE pic, int frame, Rect *rect, int x, int y, hudcolor_e color, int a, hudalign_e alignment)
{
	int r, g, b;
	GetColor(r, g, b, color);
	DrawHudSprite(pic, frame, rect, x, y, r, g, b, a, alignment);
}

void CHud::DrawHudSpriteIndex(int index, int x, int y, int r, int g, int b, int a, hudalign_e alignment)
{
	DrawHudSprite(GetSprite(index), 0, &GetSpriteRect(index), x, y, r, g, b, a, alignment);
}

void CHud::DrawHudSpriteIndex(int index, int x, int y, hudcolor_e color, int a, hudalign_e alignment)
{
	int r, g, b;
	GetColor(r, g, b, color);
	DrawHudSprite(GetSprite(index), 0, &GetSpriteRect(index), x, y, r, g, b, a, alignment);
}

void CHud::DrawHudFill(int x, int y, int w, int h, int r, int g, int b, int a)
{
	x = roundf (m_flOffsetX + x * m_flScaleX);
	y = roundf (m_flOffsetY + y * m_flScaleY);
	w = roundf (w * m_flScaleX);
	h = roundf (h * m_flScaleY);
	
	gEngfuncs.pfnFillRGBA(x, y, w, h, r, g, b, a);
}

void CHud::DrawHudFill(int x, int y, int w, int h, hudcolor_e color, int a)
{
	int r, g, b;
	GetColor(r, g, b, color);
	DrawHudFill(x, y, w, h, r, g, b, a);
}

int CHud::DrawHudString(int xpos, int ypos, int iMaxX, const char* szIt, int r, int g, int b)
{
	auto x1 = roundf (m_flOffsetX + xpos * m_flScaleX);
	auto y1 = roundf (m_flOffsetY + ypos * m_flScaleY);
	return (gEngfuncs.pfnDrawString(x1, y1, szIt, r, g, b) - m_flOffsetX) / m_flScaleX;
}

int CHud::DrawHudNumberString(int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b)
{
	char szString[32];
	sprintf(szString, "%d", iNumber);
	return DrawHudStringReverse(xpos, ypos, iMinX, szString, r, g, b);
}

// draws a string from right to left (right-aligned)
int CHud::DrawHudStringReverse(int xpos, int ypos, int iMinX, const char* szString, int r, int g, int b)
{
	auto x1 = roundf (m_flOffsetX + xpos * m_flScaleX);
	auto y1 = roundf (m_flOffsetY + ypos * m_flScaleY);
	return (gEngfuncs.pfnDrawStringReverse(x1, y1, szString, r, g, b) - m_flOffsetX) / m_flScaleX;
}

int CHud::DrawHudNumber(int x, int y, int iFlags, int iNumber, int r, int g, int b, int a, hudalign_e alignment)
{
	int iWidth = GetSpriteRect(m_HUD_number_0).right - GetSpriteRect(m_HUD_number_0).left;
	int k;

	if (iNumber > 0)
	{
		// SPR_Draw 100's
		if (iNumber >= 100)
		{
			k = iNumber / 100;
			DrawHudSpriteIndex(m_HUD_number_0 + k, x, y, r, g, b, a, alignment);
			x += iWidth;
		}
		else if ((iFlags & DHN_3DIGITS) != 0)
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw 10's
		if (iNumber >= 10)
		{
			k = (iNumber % 100) / 10;
			DrawHudSpriteIndex(m_HUD_number_0 + k, x, y, r, g, b, a, alignment);
			x += iWidth;
		}
		else if ((iFlags & (DHN_3DIGITS | DHN_2DIGITS)) != 0)
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw ones
		k = iNumber % 10;
		DrawHudSpriteIndex(m_HUD_number_0 + k, x, y, r, g, b, a, alignment);
		x += iWidth;
	}
	else if ((iFlags & DHN_DRAWZERO) != 0)
	{
		// SPR_Draw 100's
		if ((iFlags & DHN_3DIGITS) != 0)
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		if ((iFlags & (DHN_3DIGITS | DHN_2DIGITS)) != 0)
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw ones

		DrawHudSpriteIndex(m_HUD_number_0, x, y, r, g, b, a, alignment);
		x += iWidth;
	}

	return x;
}

int CHud::DrawHudNumber(int x, int y, int iFlags, int iNumber, hudcolor_e color, int a, hudalign_e alignment)
{
	int r, g, b;
	GetColor(r, g, b, color);
	return DrawHudNumber(x, y, iFlags, iNumber, r, g, b, a, alignment);
}

int CHud::GetNumWidth(int iNumber, int iFlags)
{
	if ((iFlags & DHN_3DIGITS) != 0)
		return 3;

	if ((iFlags & DHN_2DIGITS) != 0)
		return 2;

	if (iNumber <= 0)
	{
		if ((iFlags & DHN_DRAWZERO) != 0)
			return 1;
		else
			return 0;
	}

	if (iNumber < 10)
		return 1;

	if (iNumber < 100)
		return 2;

	return 3;
}

int CHud::GetHudNumberWidth(int number, int width, int flags)
{
	const int digitWidth = GetSpriteRect(m_HUD_number_0).right - GetSpriteRect(m_HUD_number_0).left;

	int totalDigits = 0;

	if (number > 0)
	{
		totalDigits = static_cast<int>(log10(number)) + 1;
	}
	else if ((flags & DHN_DRAWZERO) != 0)
	{
		totalDigits = 1;
	}

	totalDigits = std::max(totalDigits, width);

	return totalDigits * digitWidth;
}

int CHud::DrawHudNumberReverse(int x, int y, int number, int flags, int r, int g, int b, int a, hudalign_e alignment)
{
	if (number > 0 || (flags & DHN_DRAWZERO) != 0)
	{
		const int digitWidth = GetSpriteRect(m_HUD_number_0).right - GetSpriteRect(m_HUD_number_0).left;

		int remainder = number;

		do
		{
			const int digit = remainder % 10;
			const int digitSpriteIndex = m_HUD_number_0 + digit;

			//This has to happen *before* drawing because we're drawing in reverse
			x -= digitWidth;

			DrawHudSpriteIndex(digitSpriteIndex, x, y, r, g, b, a, alignment);

			remainder /= 10;
		} while (remainder > 0);
	}

	return x;
}

int CHud::DrawHudNumberReverse(int x, int y, int number, int flags, hudcolor_e color, int a, hudalign_e alignment)
{
	int r, g, b;
	GetColor(r, g, b, color);
	return DrawHudNumberReverse(x, y, number, flags, r, g, b, a, alignment);
}

int CHud::DrawHudString(const char* string, int x, int y)
{
	auto x1 = roundf (m_flOffsetX + x * m_flScaleX);
	auto y1 = roundf (m_flOffsetY + y * m_flScaleY);
	return (gEngfuncs.pfnDrawConsoleString(x1, y1, (char*)string) - m_flOffsetX) / m_flScaleX;
}

void CHud::GetHudStringSize(const char* string, int& width, int& height)
{
	gEngfuncs.pfnDrawConsoleStringLen(string, &width, &height);
	width /= m_flScaleX;
	height /= m_flScaleY;
}

int CHud::HudStringLen(const char* string)
{
	int width, height;
	GetHudStringSize(string, width, height);
	return width;
}

void CHud::GetChatInputPosition(int& x, int& y)
{
	x = roundf (m_flOffsetX + m_SayText.m_iBaseX * m_flScaleX);
	y = roundf (m_flOffsetY + (m_SayText.m_iBaseY + m_SayText.m_iLineHeight) * m_flScaleY);
}

void CHud::DrawWorldSprite(HSPRITE pic, int frame, Rect *rect, Vector origin, hudcolor_e color, int a)
{
	Vector screen;
	if (gEngfuncs.pTriAPI->WorldToScreen(origin, screen) != 0)
	{
		return;
	}

	int x = (1.0f + screen.x) * ScreenWidth * 0.5f;
	int y = (1.0f - screen.y) * ScreenHeight * 0.5f;

	int r, g, b;
	GetColor(r, g, b, color);

	auto sprw = gEngfuncs.pfnSPR_Width (pic, frame);
	auto sprh = gEngfuncs.pfnSPR_Height (pic, frame);

	if (!rect)
	{
		static Rect rc;
		rc.left = 0;
		rc.right = sprw;
		rc.top = 0;
		rc.bottom = sprh;
		rect = &rc;
	}

	float xf = x;
	float yf = y;
	auto width = rect->right - rect->left;
	auto height = rect->bottom - rect->top;

	if (!IEngineStudio.IsHardware())
	{
		ScaleColors(r, g, b, a);
		gEngfuncs.pfnSPR_Set(pic, r, g, b);
		gEngfuncs.pfnSPR_DrawHoles(frame, x, y, rect);
		return;
	}

	xf -= (width / 2.0F - 0.5F) * m_flScaleX; 
	yf -= (height / 2.0F - 0.5F) * m_flScaleY; 

	auto pSprite = const_cast<model_t *>(gEngfuncs.GetSpritePointer (pic));

	auto x1 = roundf (xf);
	auto y1 = roundf (yf);
	auto x2 = roundf (xf + width * m_flScaleX);
	auto y2 = roundf (yf + height * m_flScaleY);

	auto left = rect->left / (float)sprw;
	auto right = rect->right / (float)sprw;
	auto top = rect->top / (float)sprh;
	auto bottom = rect->bottom / (float)sprh;
	
	gEngfuncs.pTriAPI->SpriteTexture (pSprite, frame);

	gEngfuncs.pTriAPI->Color4fRendermode (r / 255.0F, g / 255.0F, b / 255.0F, a / 255.0F, kRenderTransAlpha);
	gEngfuncs.pTriAPI->RenderMode (kRenderTransAlpha);

	gEngfuncs.pTriAPI->Begin (TRI_QUADS);

	gEngfuncs.pTriAPI->TexCoord2f (left, top);
	gEngfuncs.pTriAPI->Vertex3f (x1, y1, 0);

	gEngfuncs.pTriAPI->TexCoord2f (right, top);
	gEngfuncs.pTriAPI->Vertex3f (x2, y1, 0);

	gEngfuncs.pTriAPI->TexCoord2f (right, bottom);
	gEngfuncs.pTriAPI->Vertex3f (x2, y2, 0);

	gEngfuncs.pTriAPI->TexCoord2f (left, bottom);
	gEngfuncs.pTriAPI->Vertex3f (x1, y2, 0);

	gEngfuncs.pTriAPI->End ();
}
