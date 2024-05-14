//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any

#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in client::pTriAPI

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "com_model.h"
#include "r_studioint.h"
#include "triangleapi.h"
#include "Exports.h"

#include "particleman.h"
#include "tri.h"

extern engine_studio_api_t IEngineStudio;
extern IParticleMan* g_pParticleMan;

#include <algorithm>

void CHud::RedrawZoomOverlay(float time)
{
	if (!IEngineStudio.IsHardware())
	{
		return;
	}

	auto sprite = const_cast<struct model_s*>(client::GetSpritePointer(m_hSniperScope));

	float scopeLeft = (ScreenWidth - ScreenHeight) / 2.0F;
	float scopeRight = (ScreenWidth + ScreenHeight) / 2.0F;

	client::tri::SpriteTexture(sprite, 0);

	client::tri::Color4fRendermode(1.0F, 1.0F, 1.0F, 1.0F, kRenderTransAlpha);
	client::tri::RenderMode(kRenderTransAlpha);

	client::tri::Begin(TRI_QUADS);


	client::tri::TexCoord2f(0, 1);
	client::tri::Vertex3f(scopeLeft, 0, 0);

	client::tri::TexCoord2f(1, 1);
	client::tri::Vertex3f(scopeRight, 0, 0);

	client::tri::TexCoord2f(1, 0);
	client::tri::Vertex3f(scopeRight, ScreenHeight, 0);

	client::tri::TexCoord2f(0, 0);
	client::tri::Vertex3f(scopeLeft, ScreenHeight, 0);


	client::tri::TexCoord2f(0, 0);

	client::tri::Vertex3f(0, 0, 0);
	client::tri::Vertex3f(scopeLeft, 0, 0);
	client::tri::Vertex3f(scopeLeft, ScreenHeight, 0);
	client::tri::Vertex3f(0, ScreenHeight, 0);


	client::tri::TexCoord2f(1, 0);
	client::tri::Vertex3f(scopeRight, 0, 0);

	client::tri::TexCoord2f(1, 0);
	client::tri::Vertex3f(ScreenWidth, 0, 0);

	client::tri::TexCoord2f(1, 0);
	client::tri::Vertex3f(ScreenWidth, ScreenHeight, 0);

	client::tri::TexCoord2f(1, 0);
	client::tri::Vertex3f(scopeRight, ScreenHeight, 0);


	client::tri::End();
}

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void HUD_DrawNormalTriangles()
{
	gHUD.m_Spectator.DrawOverview();
}


/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
void HUD_DrawTransparentTriangles()
{
	if (g_pParticleMan)
		g_pParticleMan->Update();
}
