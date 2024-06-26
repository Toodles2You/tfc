//=========== (C) Copyright 1996-2002 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: TFC Class Menu
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "VGUI_Font.h"
#include <VGUI_TextImage.h>

#include "hud.h"
#include "cl_util.h"
#include "camera.h"
#include "kbutton.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"
#include "camera.h"
#include "in_defs.h"
#include "parsemsg.h"

#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"

// Class Menu Dimensions
#define CLASSMENU_TITLE_X XRES(40)
#define CLASSMENU_TITLE_Y YRES(32)
#define CLASSMENU_TOPLEFT_BUTTON_X XRES(40)
#define CLASSMENU_TOPLEFT_BUTTON_Y YRES(80)
#define CLASSMENU_BUTTON_SIZE_X XRES(124)
#define CLASSMENU_BUTTON_SIZE_Y YRES(24)
#define CLASSMENU_BUTTON_SPACER_Y YRES(8)
#define CLASSMENU_WINDOW_X XRES(176)
#define CLASSMENU_WINDOW_Y YRES(80)
#define CLASSMENU_WINDOW_SIZE_X XRES(424)
#define CLASSMENU_WINDOW_SIZE_Y YRES(312)
#define CLASSMENU_WINDOW_TEXT_X XRES(150)
#define CLASSMENU_WINDOW_TEXT_Y YRES(80)
#define CLASSMENU_WINDOW_NAME_X XRES(150)
#define CLASSMENU_WINDOW_NAME_Y YRES(8)
#define CLASSMENU_WINDOW_PLAYERS_Y YRES(42)

// Creation
CClassMenuPanel::CClassMenuPanel(int iTrans, bool iRemoveMe, int x, int y, int wide, int tall) : CMenuPanel(iTrans, iRemoveMe, x, y, wide, tall)
{
	// don't show class graphics at below 640x480 resolution
	bool bShowClassGraphic = true;
	if (ScreenWidth < 640)
	{
		bShowClassGraphic = false;
	}

	memset(m_pClassImages, 0, sizeof(m_pClassImages));

	// Get the scheme used for the Titles
	CSchemeManager* pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle("Title Font");
	SchemeHandle_t hClassWindowText = pSchemes->getSchemeHandle("Briefing Text");

	// color schemes
	int r, g, b, a;

	// Create the title
	Label* pLabel = new Label("", CLASSMENU_TITLE_X, CLASSMENU_TITLE_Y);
	pLabel->setParent(this);
	pLabel->setFont(pSchemes->getFont(hTitleScheme));
	pSchemes->getFgColor(hTitleScheme, r, g, b, a);
	pLabel->setFgColor(r, g, b, a);
	pSchemes->getBgColor(hTitleScheme, r, g, b, a);
	pLabel->setBgColor(r, g, b, a);
	pLabel->setContentAlignment(vgui::Label::a_west);
	pLabel->setText(gHUD.m_TextMessage.BufferedLocaliseTextString("#Title_SelectYourClass"));

	// Create the Scroll panel
	m_pScrollPanel = new CTFScrollPanel(CLASSMENU_WINDOW_X, CLASSMENU_WINDOW_Y, CLASSMENU_WINDOW_SIZE_X, CLASSMENU_WINDOW_SIZE_Y);
	m_pScrollPanel->setParent(this);
	//force the scrollbars on, so after the validate clientClip will be smaller
	m_pScrollPanel->setScrollBarAutoVisible(false, false);
	m_pScrollPanel->setScrollBarVisible(true, true);
	m_pScrollPanel->setBorder(new LineBorder(Color(255 * 0.7, 170 * 0.7, 0, 0)));
	m_pScrollPanel->validate();

	int clientWide = m_pScrollPanel->getClient()->getWide();

	//turn scrollpanel back into auto show scrollbar mode and validate
	m_pScrollPanel->setScrollBarAutoVisible(false, true);
	m_pScrollPanel->setScrollBarVisible(false, false);
	m_pScrollPanel->validate();

	// Create the Class buttons
	for (int i = 0; i <= PC_RANDOM; i++)
	{
		char sz[256];
		int iYPos = CLASSMENU_TOPLEFT_BUTTON_Y + ((CLASSMENU_BUTTON_SIZE_Y + CLASSMENU_BUTTON_SPACER_Y) * i);

		ActionSignal* pASignal = new CMenuHandler_StringCommandClassSelect(sTFClassSelection[i], true);

		// Class button
		sprintf(sz, "%s", CHudTextMessage::BufferedLocaliseTextString(sLocalisedClasses[i]));
		m_pButtons[i] = new ClassButton(i, sz, CLASSMENU_TOPLEFT_BUTTON_X, iYPos, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y, true);
		// RandomPC uses '0'
		if (i > PC_UNDEFINED && i < PC_RANDOM)
		{
			sprintf(sz, "%d", i);
		}
		else
		{
			sprintf(sz, "0");
		}
		m_pButtons[i]->setBoundKey(sz[0]);
		m_pButtons[i]->setContentAlignment(vgui::Label::a_west);
		m_pButtons[i]->addActionSignal(pASignal);
		m_pButtons[i]->addInputSignal(new CHandler_MenuButtonOver(this, i));
		m_pButtons[i]->setParent(this);

		// Create the Class Info Window
		m_pClassInfoPanel[i] = new CTransparentPanel(255, 0, 0, clientWide, CLASSMENU_WINDOW_SIZE_Y);
		m_pClassInfoPanel[i]->setParent(m_pScrollPanel->getClient());

		// don't show class pic in lower resolutions
		int textOffs = XRES(8);

		if (bShowClassGraphic)
		{
			textOffs = CLASSMENU_WINDOW_NAME_X;
		}

		// Create the Class Name Label
		sprintf(sz, "#Title_%s", sTFClassSelection[i]);
		char* localName = CHudTextMessage::BufferedLocaliseTextString(sz);
		Label* pNameLabel = new Label("", textOffs, CLASSMENU_WINDOW_NAME_Y);
		pNameLabel->setFont(pSchemes->getFont(hTitleScheme));
		pNameLabel->setParent(m_pClassInfoPanel[i]);
		pSchemes->getFgColor(hTitleScheme, r, g, b, a);
		pNameLabel->setFgColor(r, g, b, a);
		pSchemes->getBgColor(hTitleScheme, r, g, b, a);
		pNameLabel->setBgColor(r, g, b, a);
		pNameLabel->setContentAlignment(vgui::Label::a_west);
		//pNameLabel->setBorder(new LineBorder());
		pNameLabel->setText("%s", localName);

		// Create the Class Image
		if (bShowClassGraphic)
		{
			for (int team = 0; team < 2; team++)
			{
				if (team == 1)
				{
					sprintf(sz, "%sred", sTFClassSelection[i]);
				}
				else
				{
					sprintf(sz, "%sblue", sTFClassSelection[i]);
				}

				m_pClassImages[team][i] = new CImageLabel(sz, 0, 0, CLASSMENU_WINDOW_TEXT_X, CLASSMENU_WINDOW_TEXT_Y);

				CImageLabel* pLabel = m_pClassImages[team][i];
				pLabel->setParent(m_pClassInfoPanel[i]);

				if (team != 1)
				{
					pLabel->setVisible(false);
				}

				// Reposition it based upon it's size
				int xOut, yOut;
				pNameLabel->getTextSize(xOut, yOut);
				pLabel->setPos((CLASSMENU_WINDOW_TEXT_X - pLabel->getWide()) / 2, yOut / 2);
			}
		}

		// Create the Player count string
		gHUD.m_TextMessage.LocaliseTextString("#Title_CurrentlyOnYourTeam", m_sPlayersOnTeamString, STRLENMAX_PLAYERSONTEAM);
		m_pPlayers[i] = new Label("", textOffs, CLASSMENU_WINDOW_PLAYERS_Y);
		m_pPlayers[i]->setParent(m_pClassInfoPanel[i]);
		m_pPlayers[i]->setBgColor(0, 0, 0, 255);
		m_pPlayers[i]->setContentAlignment(vgui::Label::a_west);
		m_pPlayers[i]->setFont(pSchemes->getFont(hClassWindowText));

		// Open up the Class Briefing File
		sprintf(sz, "classes/short_%s.txt", sTFClassSelection[i]);
		char* cText;
		char* pfile = (char*)client::COM_LoadFile(sz, 5, nullptr);
		if (pfile != nullptr)
		{
			cText = pfile;
		}
		else
		{
			cText = gHUD.m_TextMessage.BufferedLocaliseTextString("#Class_Description_not_available");
		}

		// Create the Text info window
		TextPanel* pTextWindow = new TextPanel(cText, textOffs, CLASSMENU_WINDOW_TEXT_Y, (CLASSMENU_WINDOW_SIZE_X - textOffs) - 5, CLASSMENU_WINDOW_SIZE_Y - CLASSMENU_WINDOW_TEXT_Y);
		pTextWindow->setParent(m_pClassInfoPanel[i]);
		pTextWindow->setFont(pSchemes->getFont(hClassWindowText));
		pSchemes->getFgColor(hClassWindowText, r, g, b, a);
		pTextWindow->setFgColor(r, g, b, a);
		pSchemes->getBgColor(hClassWindowText, r, g, b, a);
		pTextWindow->setBgColor(r, g, b, a);

		// Resize the Info panel to fit it all
		int wide, tall;
		pTextWindow->getTextImage()->getTextSizeWrapped(wide, tall);
		pTextWindow->setSize(wide, tall);

		int xx, yy;
		pTextWindow->getPos(xx, yy);
		int maxX = xx + wide;
		int maxY = yy + tall;

		//check to see if the image goes lower than the text
		//just use the red teams [0] images
		if (m_pClassImages[0][i] != null)
		{
			m_pClassImages[0][i]->getPos(xx, yy);
			if ((yy + m_pClassImages[0][i]->getTall()) > maxY)
			{
				maxY = yy + m_pClassImages[0][i]->getTall();
			}
		}

		m_pClassInfoPanel[i]->setSize(maxX, maxY);
		if (pfile)
			client::COM_FreeFile(pfile);
	}

	// Create the Cancel button
	m_pCancelButton = new CommandButton(gHUD.m_TextMessage.BufferedLocaliseTextString("#Menu_Cancel"), CLASSMENU_TOPLEFT_BUTTON_X, 0, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y);
	m_pCancelButton->setParent(this);
	m_pCancelButton->addActionSignal(new CMenuHandler_TextWindow(HIDE_TEXTWINDOW));
	m_pCancelButton->addInputSignal(new CHandler_MenuButtonOver(this, 11));

	m_iCurrentInfo = 0;
}


// Update
void CClassMenuPanel::Update()
{
	// Don't allow the player to join a class if they're not in a team
	if (g_iTeamNumber == TEAM_UNASSIGNED)
		return;

	int iYPos = CLASSMENU_TOPLEFT_BUTTON_Y;

	// Cycle through the rest of the buttons
	for (int i = 0; i <= PC_RANDOM; i++)
	{
		bool bCivilian = (gViewPort->GetValidClasses(g_iTeamNumber) == -1);

		if (bCivilian)
		{
			// If this team can only be civilians, only the civilian button's visible
			if (i == 0)
			{
				m_pButtons[0]->setVisible(true);
				SetActiveInfo(0);
				iYPos += CLASSMENU_BUTTON_SIZE_Y + CLASSMENU_BUTTON_SPACER_Y;
			}
			else
			{
				m_pButtons[i]->setVisible(false);
			}
		}
		else
		{
			if (m_pButtons[i]->IsNotValid() || i == 0)
			{
				m_pButtons[i]->setVisible(false);
			}
			else
			{
				m_pButtons[i]->setVisible(true);
				m_pButtons[i]->setPos(CLASSMENU_TOPLEFT_BUTTON_X, iYPos);
				iYPos += CLASSMENU_BUTTON_SIZE_Y + CLASSMENU_BUTTON_SPACER_Y;

				// Start with the first option up
				if (m_iCurrentInfo == 0)
				{
					SetActiveInfo(i);
				}
			}
		}

		// Now count the number of teammembers of this class
		int iTotal = 0;
		for (int j = 1; j < MAX_PLAYERS_HUD; j++)
		{
			if (g_PlayerInfoList[j].name == nullptr)
				continue; // empty player slot, skip
			if (g_PlayerExtraInfo[j].teamnumber != g_iTeamNumber)
				continue; // skip over players in other teams

			// If this team is forced to be civilians, just count the number of teammates
			if (g_PlayerExtraInfo[j].playerclass != i && !bCivilian)
				continue;

			iTotal++;
		}

		char sz[256];
		sprintf(sz, m_sPlayersOnTeamString, iTotal);
		m_pPlayers[i]->setText("%s", sz);

		// Set the text color to the teamcolor
		auto color = gHUD.GetTeamColor(g_iTeamNumber);
		m_pPlayers[i]->setFgColor(
			color[0] * 255,
			color[1] * 255,
			color[2] * 255,
			0);

		// set the graphic to be the team pick
		for (int team = 0; team < 2; team++)
		{
			// unset all the other images
			if (m_pClassImages[team][i])
			{
				m_pClassImages[team][i]->setVisible(false);
			}
		}

		// set the current team image
		if (m_pClassImages[g_iTeamNumber - 1][i] != nullptr)
		{
			m_pClassImages[g_iTeamNumber - 1][i]->setVisible(true);
		}
		else if (m_pClassImages[0][i])
		{
			m_pClassImages[0][i]->setVisible(true);
		}
	}

	// If the player already has a class, make the cancel button visible
	if (g_iPlayerClass != PC_UNDEFINED)
	{
		m_pCancelButton->setPos(CLASSMENU_TOPLEFT_BUTTON_X, iYPos);
		m_pCancelButton->setVisible(true);
	}
	else
	{
		m_pCancelButton->setVisible(false);
	}
}

//======================================
// Key inputs for the Class Menu
bool CClassMenuPanel::SlotInput(int iSlot)
{
	if (iSlot < 0 || iSlot >= PC_RANDOM)
		return false;
	if (!m_pButtons[iSlot])
		return false;

	// Is the button pushable? (0 is special case)
	if (iSlot == 0)
	{
		// Selects Civilian and RandomPC
		if (gViewPort->GetValidClasses(g_iTeamNumber) == -1)
		{
			m_pButtons[0]->fireActionSignal();
			return true;
		}

		// Select RandomPC
		iSlot = PC_RANDOM;
	}

	if (!(m_pButtons[iSlot]->IsNotValid()))
	{
		m_pButtons[iSlot]->fireActionSignal();
		return true;
	}

	return false;
}

//======================================
// Update the Class menu before opening it
void CClassMenuPanel::Open()
{
	// Start with the current class up
	if (g_iPlayerClass != PC_UNDEFINED)
	{
		SetActiveInfo(g_iPlayerClass);
	}
	Update();
	CMenuPanel::Open();
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void CClassMenuPanel::Initialize()
{
	setVisible(false);
	m_pScrollPanel->setScrollValue(0, 0);
}

//======================================
// Mouse is over a class button, bring up the class info
void CClassMenuPanel::SetActiveInfo(int iInput)
{
	// Remove all the Info panels and bring up the specified one

	if (iInput == 11)
	{
		m_pCancelButton->setArmed(true);
	}
	else
	{
		m_pCancelButton->setArmed(false);

		for (int i = 0; i <= PC_RANDOM; i++)
		{
			m_pButtons[i]->setArmed(false);
			m_pClassInfoPanel[i]->setVisible(false);
		}

		m_pButtons[iInput]->setArmed(true);
		m_pClassInfoPanel[iInput]->setVisible(true);
	}

	m_iCurrentInfo = iInput;

	m_pScrollPanel->setScrollValue(0, 0);
	m_pScrollPanel->validate();
}
