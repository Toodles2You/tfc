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
	client::GetScreenInfo(&m_scrinfo);

	int newfov;
	HUDLIST* pList = m_pHudList;

	while (pList)
	{
		if (pList->p->IsActive())
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

	client::tri::CullFace(TRI_FRONT);
	
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
		auto ulRGB = strtoul(m_pCvarColor->string, nullptr, 16);
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
			if (client::GetCvarFloat("hud_takesshots") != 0)
				m_flShotTime = flTime + 1.0; // Take a screenshot in a second
		}
	}

	if (0 != m_flShotTime && m_flShotTime < flTime)
	{
		client::ClientCmd("snapshot\n");
		m_flShotTime = 0;
	}

	m_iIntermission = intermission;

	// draw all registered HUD elements
	if (0 != m_pCvarDraw->value)
	{
		HUDLIST* pList = m_pHudList;

		while (pList)
		{
			if (pList->p->IsActive())
			{
				pList->p->Draw(flTime);
			}

			pList = pList->pNext;
		}
	}

	client::tri::RenderMode(kRenderNormal);

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
	auto sprw = client::SPR_Width (pic, frame);
	auto sprh = client::SPR_Height (pic, frame);

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
		client::SPR_Set(pic, r, g, b);
		client::SPR_DrawAdditive(frame, x, y, rect);
		return;
	}
	
	const auto pSprite = const_cast<model_t *>(client::GetSpritePointer (pic));

	if (pSprite == nullptr)
		return;

	const auto x1 = roundf (m_flOffsetX + xf * m_flScaleX);
	const auto y1 = roundf (m_flOffsetY + yf * m_flScaleY);
	const auto x2 = roundf (m_flOffsetX + (xf + width) * m_flScaleX);
	const auto y2 = roundf (m_flOffsetY + (yf + height) * m_flScaleY);

	const auto left = rect->left / (float)sprw;
	const auto right = rect->right / (float)sprw;
	const auto top = rect->top / (float)sprh;
	const auto bottom = rect->bottom / (float)sprh;
	
	client::tri::SpriteTexture (pSprite, frame);

	const auto rendermode = kRenderTransAdd;

	client::tri::Color4fRendermode (r / 255.0F, g / 255.0F, b / 255.0F, a / 255.0F, rendermode);
	client::tri::RenderMode (rendermode);

	client::tri::Begin (TRI_QUADS);

	client::tri::TexCoord2f (left, top);
	client::tri::Vertex3f (x1, y1, 0);

	client::tri::TexCoord2f (right, top);
	client::tri::Vertex3f (x2, y1, 0);

	client::tri::TexCoord2f (right, bottom);
	client::tri::Vertex3f (x2, y2, 0);

	client::tri::TexCoord2f (left, bottom);
	client::tri::Vertex3f (x1, y2, 0);

	client::tri::End ();
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
	
	client::FillRGBA(x, y, w, h, r, g, b, a);
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
	return (client::DrawString(x1, y1, szIt, r, g, b) - m_flOffsetX) / m_flScaleX;
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
	return (client::DrawStringReverse(x1, y1, szString, r, g, b) - m_flOffsetX) / m_flScaleX;
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
			//client::SPR_DrawAdditive( 0, x, y, &rc );
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
			//client::SPR_DrawAdditive( 0, x, y, &rc );
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
			//client::SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		if ((iFlags & (DHN_3DIGITS | DHN_2DIGITS)) != 0)
		{
			//client::SPR_DrawAdditive( 0, x, y, &rc );
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
	return (client::DrawConsoleString(x1, y1, (char*)string) - m_flOffsetX) / m_flScaleX;
}

void CHud::GetHudStringSize(const char* string, int& width, int& height)
{
	client::DrawConsoleStringLen(string, &width, &height);
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

void CHud::DrawHudBackground(int left, int top, int right, int bottom, const bool highlight)
{
	if (!IEngineStudio.IsHardware())
	{
		return;
	}

	const auto pSprite = const_cast<model_t *>(client::GetSpritePointer (m_hBackground));

	if (pSprite == nullptr)
		return;

	/* Shrink the bounds slightly to compensate for the border. */
	left += 1;
	top += 1;
	right -= 1;
	bottom -= 1;

	const auto x1 = roundf (m_flOffsetX + left * m_flScaleX);
	const auto y1 = roundf (m_flOffsetY + top * m_flScaleY);
	const auto x2 = roundf (m_flOffsetX + right * m_flScaleX);
	const auto y2 = roundf (m_flOffsetY + bottom * m_flScaleY);

	const auto x0 = roundf (m_flOffsetX + (left - 4) * m_flScaleX);
	const auto y0 = roundf (m_flOffsetY + (top - 4) * m_flScaleY);
	const auto x3 = roundf (m_flOffsetX + (right + 4) * m_flScaleX);
	const auto y3 = roundf (m_flOffsetY + (bottom + 4) * m_flScaleY);

	client::tri::SpriteTexture (pSprite, 0);

	const auto value = highlight ? 0.4F : 0.0F;

	client::tri::Color4fRendermode (value, 0.0F, 0.0F, 1.0F / 2.0F, kRenderTransAlpha);
	client::tri::RenderMode (kRenderTransAlpha);

	client::tri::Begin (TRI_QUADS);

	/* Left Top */
	client::tri::TexCoord2f (0, 0);
	client::tri::Vertex3f (x0, y0, 0);

	client::tri::TexCoord2f (0.25, 0);
	client::tri::Vertex3f (x1, y0, 0);

	client::tri::TexCoord2f (0.25, 0.25);
	client::tri::Vertex3f (x1, y1, 0);

	client::tri::TexCoord2f (0, 0.25);
	client::tri::Vertex3f (x0, y1, 0);

	/* Left Center */
	client::tri::TexCoord2f (0, 0.25);
	client::tri::Vertex3f (x0, y1, 0);

	client::tri::TexCoord2f (0.25, 0.25);
	client::tri::Vertex3f (x1, y1, 0);

	client::tri::TexCoord2f (0.25, 0.75);
	client::tri::Vertex3f (x1, y2, 0);

	client::tri::TexCoord2f (0, 0.75);
	client::tri::Vertex3f (x0, y2, 0);

	/* Left Bottom */
	client::tri::TexCoord2f (0, 0.75);
	client::tri::Vertex3f (x0, y2, 0);

	client::tri::TexCoord2f (0.25, 0.75);
	client::tri::Vertex3f (x1, y2, 0);

	client::tri::TexCoord2f (0.25, 1);
	client::tri::Vertex3f (x1, y3, 0);

	client::tri::TexCoord2f (0, 1);
	client::tri::Vertex3f (x0, y3, 0);

	/* Center Top */
	client::tri::TexCoord2f (0.25, 0);
	client::tri::Vertex3f (x1, y0, 0);

	client::tri::TexCoord2f (0.75, 0);
	client::tri::Vertex3f (x2, y0, 0);

	client::tri::TexCoord2f (0.75, 0.25);
	client::tri::Vertex3f (x2, y1, 0);

	client::tri::TexCoord2f (0.25, 0.25);
	client::tri::Vertex3f (x1, y1, 0);

	/* Center Center */
	client::tri::TexCoord2f (0.25, 0.25);
	client::tri::Vertex3f (x1, y1, 0);

	client::tri::TexCoord2f (0.75, 0.25);
	client::tri::Vertex3f (x2, y1, 0);

	client::tri::TexCoord2f (0.75, 0.75);
	client::tri::Vertex3f (x2, y2, 0);

	client::tri::TexCoord2f (0.25, 0.75);
	client::tri::Vertex3f (x1, y2, 0);

	/* Center Bottom */
	client::tri::TexCoord2f (0.25, 0.75);
	client::tri::Vertex3f (x1, y2, 0);

	client::tri::TexCoord2f (0.75, 0.75);
	client::tri::Vertex3f (x2, y2, 0);

	client::tri::TexCoord2f (0.75, 1);
	client::tri::Vertex3f (x2, y3, 0);

	client::tri::TexCoord2f (0.25, 1);
	client::tri::Vertex3f (x1, y3, 0);

	/* Right Top */
	client::tri::TexCoord2f (0.75, 0);
	client::tri::Vertex3f (x2, y0, 0);

	client::tri::TexCoord2f (1, 0);
	client::tri::Vertex3f (x3, y0, 0);

	client::tri::TexCoord2f (1, 0.25);
	client::tri::Vertex3f (x3, y1, 0);

	client::tri::TexCoord2f (0.75, 0.25);
	client::tri::Vertex3f (x2, y1, 0);

	/* Right Center */
	client::tri::TexCoord2f (0.75, 0.25);
	client::tri::Vertex3f (x2, y1, 0);

	client::tri::TexCoord2f (1, 0.25);
	client::tri::Vertex3f (x3, y1, 0);

	client::tri::TexCoord2f (1, 0.75);
	client::tri::Vertex3f (x3, y2, 0);

	client::tri::TexCoord2f (0.75, 0.75);
	client::tri::Vertex3f (x2, y2, 0);

	/* Right Bottom */
	client::tri::TexCoord2f (0.75, 0.75);
	client::tri::Vertex3f (x2, y2, 0);

	client::tri::TexCoord2f (1, 0.75);
	client::tri::Vertex3f (x3, y2, 0);

	client::tri::TexCoord2f (1, 1);
	client::tri::Vertex3f (x3, y3, 0);

	client::tri::TexCoord2f (0.75, 1);
	client::tri::Vertex3f (x2, y3, 0);

	client::tri::End ();
}

void CHud::DrawWorldSprite(HSPRITE pic, int frame, Rect *rect, Vector origin, hudcolor_e color, int a)
{
	Vector screen;
	if (client::tri::WorldToScreen(origin, screen) != 0)
	{
		return;
	}

	int x = (1.0f + screen.x) * ScreenWidth * 0.5f;
	int y = (1.0f - screen.y) * ScreenHeight * 0.5f;

	int r, g, b;
	GetColor(r, g, b, color);

	auto sprw = client::SPR_Width (pic, frame);
	auto sprh = client::SPR_Height (pic, frame);

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
		client::SPR_Set(pic, r, g, b);
		client::SPR_DrawHoles(frame, x, y, rect);
		return;
	}

	xf -= (width / 2.0F - 0.5F) * m_flScaleX; 
	yf -= (height / 2.0F - 0.5F) * m_flScaleY; 

	auto pSprite = const_cast<model_t *>(client::GetSpritePointer (pic));

	if (pSprite == nullptr)
		return;

	auto x1 = roundf (xf);
	auto y1 = roundf (yf);
	auto x2 = roundf (xf + width * m_flScaleX);
	auto y2 = roundf (yf + height * m_flScaleY);

	auto left = rect->left / (float)sprw;
	auto right = rect->right / (float)sprw;
	auto top = rect->top / (float)sprh;
	auto bottom = rect->bottom / (float)sprh;
	
	client::tri::SpriteTexture (pSprite, frame);

	client::tri::Color4fRendermode (r / 255.0F, g / 255.0F, b / 255.0F, a / 255.0F, kRenderTransAlpha);
	client::tri::RenderMode (kRenderTransAlpha);

	client::tri::Begin (TRI_QUADS);

	client::tri::TexCoord2f (left, top);
	client::tri::Vertex3f (x1, y1, 0);

	client::tri::TexCoord2f (right, top);
	client::tri::Vertex3f (x2, y1, 0);

	client::tri::TexCoord2f (right, bottom);
	client::tri::Vertex3f (x2, y2, 0);

	client::tri::TexCoord2f (left, bottom);
	client::tri::Vertex3f (x1, y2, 0);

	client::tri::End ();
}
