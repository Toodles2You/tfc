//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Client DLL VGUI Viewport
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include <VGUI_Cursor.h>
#include <VGUI_Frame.h>
#include <VGUI_Label.h>
#include <VGUI_Surface.h>
#include <VGUI_BorderLayout.h>
#include <VGUI_Panel.h>
#include <VGUI_ImagePanel.h>
#include <VGUI_Button.h>
#include <VGUI_ActionSignal.h>
#include <VGUI_InputSignal.h>
#include <VGUI_MenuSeparator.h>
#include <VGUI_TextPanel.h>
#include <VGUI_LoweredBorder.h>
#include <VGUI_LineBorder.h>
#include <VGUI_Scheme.h>
#include <VGUI_Font.h>
#include <VGUI_App.h>
#include <VGUI_BuildGroup.h>

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
#include "pm_shared.h"
#include "keydefs.h"
#include "demo.h"
#include "demo_api.h"
#include "com_weapons.h"

#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_ScorePanel.h"
#include "vgui_SpectatorPanel.h"

#include "shake.h"
#include "screenfade.h"

extern bool g_iVisibleMouse;
extern bool g_bForceSpecialDown;
extern int g_weaponselect;
class CCommandMenu;
int g_iPlayerClass;
int g_iTeamNumber;

// Scoreboard positions
#define SBOARD_INDENT_X XRES(104)
#define SBOARD_INDENT_Y YRES(40)

// low-res scoreboard indents
#define SBOARD_INDENT_X_512 30
#define SBOARD_INDENT_Y_512 30

#define SBOARD_INDENT_X_400 0
#define SBOARD_INDENT_Y_400 20

void Mouse_Reset();
extern CMenuPanel* CMessageWindowPanel_Create(const char* szMOTD, const char* szTitle, bool iShadeFullscreen, bool iRemoveMe, int x, int y, int wide, int tall);

using namespace vgui;


// Used for Class specific buttons
const char* sTFClasses[] =
	{
		"",
		"SCOUT",
		"SNIPER",
		"SOLDIER",
		"DEMOMAN",
		"MEDIC",
		"HWGUY",
		"PYRO",
		"SPY",
		"ENGINEER",
		"CIVILIAN",
};

const char* sLocalisedClasses[] =
	{
		"#Civilian",
		"#Scout",
		"#Sniper",
		"#Soldier",
		"#Demoman",
		"#Medic",
		"#HWGuy",
		"#Pyro",
		"#Spy",
		"#Engineer",
		"#Random",
		"#Civilian",
};

const char* sTFClassSelection[] =
	{
		"civilian",
		"scout",
		"sniper",
		"soldier",
		"demoman",
		"medic",
		"hwguy",
		"pyro",
		"spy",
		"engineer",
		"randompc",
		"civilian",
};

int sTFValidClassInts[] =
	{
        0,
        1,
        2,
        4,
        8,
        16,
        32,
        64,
        256,
        512,
        128,
        -1,
};


// Get the name of TGA file, based on GameDir
char* GetVGUITGAName(const char* pszName)
{
	int i;
	char sz[256];
	static char gd[256];
	const char* gamedir;

	if (ScreenWidth < 640)
		i = 320;
	else
		i = 640;
	sprintf(sz, pszName, i);

	gamedir = client::GetGameDirectory();
	sprintf(gd, "%s/gfx/vgui/%s.tga", gamedir, sz);

	return gd;
}

static void Viewport_VGUIMenu(const int menu)
{
	if (g_iTeamNumber == TEAM_UNASSIGNED
	 || (g_iPlayerClass == PC_UNDEFINED
	 && gViewPort->GetValidClasses(g_iTeamNumber) != 0))
	{
		return;
	}

	if (gViewPort->m_pCurrentMenu != nullptr
	 && gViewPort->m_pCurrentMenu->GetMenuID() == menu)
	{
		gViewPort->HideTopMenu();
		return;
	}

	gViewPort->HideVGUIMenu();
	gViewPort->HideCommandMenu();
	gViewPort->ShowVGUIMenu(menu);
}

void Viewport_ServerMOTD()
{
	Viewport_VGUIMenu(MENU_INTRO);
}

void Viewport_MapBriefing()
{
	Viewport_VGUIMenu(MENU_MAPBRIEFING);
}

void Viewport_ClassHelp()
{
	Viewport_VGUIMenu(MENU_CLASSHELP);
}

void Viewport_ChangeTeam()
{
	if (gHUD.m_gameMode < kGamemodeTeamplay)
	{
		return;
	}
	Viewport_VGUIMenu(MENU_TEAM);
}

void Viewport_ChangeClass()
{
	if (gHUD.m_gameMode < kGamemodeTeamFortress)
	{
		return;
	}
	Viewport_VGUIMenu(MENU_CLASS);
}

//================================================================
// COMMAND MENU
//================================================================
void CCommandMenu::AddButton(CommandButton* pButton)
{
	if (m_iButtons >= MAX_BUTTONS)
		return;

	m_aButtons[m_iButtons] = pButton;
	m_iButtons++;
	pButton->setParent(this);
	pButton->setFont(Scheme::sf_primary3);

	// give the button a default key binding
	if (m_iButtons < 10)
	{
		pButton->setBoundKey(m_iButtons + '0');
	}
	else if (m_iButtons == 10)
	{
		pButton->setBoundKey('0');
	}
}

void CCommandMenu::RemoveAllButtons()
{
	/*
	for(int i=0;i<m_iButtons;i++)
	{
		CommandButton *pTemp = m_aButtons[i]; 
		m_aButtons[i] = nullptr;
		
		pTemp
		if(pTemp)
		{
			delete(pTemp);
		}
		
	}
	*/
	removeAllChildren();
	m_iButtons = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Tries to find a button that has a key bound to the input, and
//			presses the button if found
// Input  : keyNum - the character number of the input key
// Output : Returns true if the command menu should close, false otherwise
//-----------------------------------------------------------------------------
bool CCommandMenu::KeyInput(int keyNum)
{
	// loop through all our buttons looking for one bound to keyNum
	for (int i = 0; i < m_iButtons; i++)
	{
		if (!m_aButtons[i]->IsNotValid())
		{
			if (m_aButtons[i]->getBoundKey() == keyNum)
			{
				// hit the button
				if (m_aButtons[i]->GetSubMenu())
				{
					// open the sub menu
					gViewPort->SetCurrentCommandMenu(m_aButtons[i]->GetSubMenu());
					return false;
				}
				else
				{
					// run the bound command
					m_aButtons[i]->fireActionSignal();
					return true;
				}
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: clears the current menus buttons of any armed (highlighted)
//			state, and all their sub buttons
//-----------------------------------------------------------------------------
void CCommandMenu::ClearButtonsOfArmedState()
{
	for (int i = 0; i < GetNumButtons(); i++)
	{
		m_aButtons[i]->setArmed(false);

		if (m_aButtons[i]->GetSubMenu())
		{
			m_aButtons[i]->GetSubMenu()->ClearButtonsOfArmedState();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : *pSubMenu -
// Output : CommandButton
//-----------------------------------------------------------------------------
CommandButton* CCommandMenu::FindButtonWithSubmenu(CCommandMenu* pSubMenu)
{
	for (int i = 0; i < GetNumButtons(); i++)
	{
		if (m_aButtons[i]->GetSubMenu() == pSubMenu)
			return m_aButtons[i];
	}

	return nullptr;
}

// Recalculate the visible buttons
bool CCommandMenu::RecalculateVisibles(int iYOffset, bool bHideAll)
{
	int i, iCurrentY = 0;
	int iVisibleButtons = 0;

	// Cycle through all the buttons in this menu, and see which will be visible
	for (i = 0; i < m_iButtons; i++)
	{
		int iClass = m_aButtons[i]->GetPlayerClass();

		if ((iClass != PC_UNDEFINED && iClass != g_iPlayerClass) || (m_aButtons[i]->IsNotValid()) || bHideAll)
		{
			m_aButtons[i]->setVisible(false);
			if (m_aButtons[i]->GetSubMenu() != nullptr)
			{
				(m_aButtons[i]->GetSubMenu())->RecalculateVisibles(0, true);
			}
		}
		else
		{
			// If it's got a submenu, force it to check visibilities
			if (m_aButtons[i]->GetSubMenu() != nullptr)
			{
				if (!(m_aButtons[i]->GetSubMenu())->RecalculateVisibles(0, false))
				{
					// The submenu had no visible buttons, so don't display this button
					m_aButtons[i]->setVisible(false);
					continue;
				}
			}

			m_aButtons[i]->setVisible(true);
			iVisibleButtons++;
		}
	}

	// Set Size
	setSize(_size[0], (iVisibleButtons * (m_flButtonSizeY - 1)) + 1);

	if (0 != iYOffset)
	{
		m_iYOffset = iYOffset;
	}

	for (i = 0; i < m_iButtons; i++)
	{
		if (m_aButtons[i]->isVisible())
		{
			if (m_aButtons[i]->GetSubMenu() != nullptr)
				(m_aButtons[i]->GetSubMenu())->RecalculateVisibles(iCurrentY + m_iYOffset, false);


			// Make sure it's at the right Y position
			// m_aButtons[i]->getPos( iXPos, iYPos );

			if (m_iDirection)
			{
				m_aButtons[i]->setPos(0, (iVisibleButtons - 1) * (m_flButtonSizeY - 1) - iCurrentY);
			}
			else
			{
				m_aButtons[i]->setPos(0, iCurrentY);
			}

			iCurrentY += (m_flButtonSizeY - 1);
		}
	}

	return 0 != iVisibleButtons;
}

// Make sure all submenus can fit on the screen
void CCommandMenu::RecalculatePositions(int iYOffset)
{
	int iTop;
	int iAdjust = 0;

	m_iYOffset += iYOffset;

	if (m_iDirection)
		iTop = ScreenHeight - (m_iYOffset + _size[1]);
	else
		iTop = m_iYOffset;

	if (iTop < 0)
		iTop = 0;

	// Calculate if this is going to fit onscreen, and shuffle it up if it won't
	int iBottom = iTop + _size[1];

	if (iBottom > ScreenHeight)
	{
		// Move in increments of button sizes
		while (iAdjust < (iBottom - ScreenHeight))
		{
			iAdjust += m_flButtonSizeY - 1;
		}

		iTop -= iAdjust;

		// Make sure it doesn't move off the top of the screen (the menu's too big to fit it all)
		if (iTop < 0)
		{
			iAdjust -= (0 - iTop);
			iTop = 0;
		}
	}

	setPos(_pos[0], iTop);

	// We need to force all menus below this one to update their positions now, because they
	// might have submenus riding off buttons in this menu that have just shifted.
	for (int i = 0; i < m_iButtons; i++)
		m_aButtons[i]->UpdateSubMenus(iAdjust);
}


// Make this menu and all menus above it in the chain visible
void CCommandMenu::MakeVisible(CCommandMenu* pChildMenu)
{
	/*
	// Push down the button leading to the child menu
	for (int i = 0; i < m_iButtons; i++)
	{
		if ( (pChildMenu != nullptr) && (m_aButtons[i]->GetSubMenu() == pChildMenu) )
		{
			m_aButtons[i]->setArmed( true );
		}
		else
		{
			m_aButtons[i]->setArmed( false );
		}
	}
*/

	setVisible(true);
	if (m_pParentMenu)
		m_pParentMenu->MakeVisible(this);
}

//================================================================
// CreateSubMenu
CCommandMenu* TeamFortressViewport::CreateSubMenu(CommandButton* pButton, CCommandMenu* pParentMenu, int iYOffset, int iXOffset)
{
	int iXPos = 0;
	int iYPos = 0;
	int iWide = CMENU_SIZE_X;
	int iTall = 0;
	bool iDirection = false;

	if (pParentMenu)
	{
		iXPos = m_pCurrentCommandMenu->GetXOffset() + (CMENU_SIZE_X - 1) + iXOffset;
		iYPos = m_pCurrentCommandMenu->GetYOffset() + iYOffset;
		iDirection = pParentMenu->GetDirection();
	}

	CCommandMenu* pMenu = new CCommandMenu(pParentMenu, iDirection, iXPos, iYPos, iWide, iTall);
	pMenu->setParent(this);
	pButton->AddSubMenu(pMenu);
	pButton->setFont(Scheme::sf_primary3);
	pMenu->m_flButtonSizeY = m_pCurrentCommandMenu->m_flButtonSizeY;

	// Create the Submenu-open signal
	InputSignal* pISignal = new CMenuHandler_PopupSubMenuInput(pButton, pMenu);
	pButton->addInputSignal(pISignal);

	// Put a > to show it's a submenu
	CImageLabel* pLabel = new CImageLabel("arrowright", XRES(CMENU_SIZE_X - SUBMENU_SIZE_X), YRES(SUBMENU_SIZE_Y));
	pLabel->setParent(pButton);
	pLabel->addInputSignal(pISignal);

	// Reposition
	pLabel->getPos(iXPos, iYPos);
	pLabel->setPos(pButton->getWide() - pLabel->getImageWide() - 4, -4);

	// Create the mouse off signal for the Label too
	if (!pButton->m_bNoHighlight)
		pLabel->addInputSignal(new CHandler_CommandButtonHighlight(pButton));

	return pMenu;
}

//-----------------------------------------------------------------------------
// Purpose: Makes sure the memory allocated for TeamFortressViewport is nulled out
// Input  : stAllocateBlock -
// Output : void *
//-----------------------------------------------------------------------------
void* TeamFortressViewport::operator new(size_t stAllocateBlock)
{
	//	void *mem = Panel::operator new( stAllocateBlock );
	void* mem = ::operator new(stAllocateBlock);
	memset(mem, 0, stAllocateBlock);
	return mem;
}

//-----------------------------------------------------------------------------
// Purpose: InputSignal handler for the main viewport
//-----------------------------------------------------------------------------
class CViewPortInputHandler : public InputSignal
{
public:
	bool bPressed;

	CViewPortInputHandler()
	{
	}

	void cursorMoved(int x, int y, Panel* panel) override {}
	void cursorEntered(Panel* panel) override {}
	void cursorExited(Panel* panel) override {}
	void mousePressed(MouseCode code, Panel* panel) override
	{
		if (code != MOUSE_LEFT)
		{
			// send a message to close the command menu
			// this needs to be a message, since a direct call screws the timing
			client::ClientCmd("ForceCloseCommandMenu\n");
		}
	}
	void mouseReleased(MouseCode code, Panel* panel) override
	{
	}

	void mouseDoublePressed(MouseCode code, Panel* panel) override {}
	void mouseWheeled(int delta, Panel* panel) override {}
	void keyPressed(KeyCode code, Panel* panel) override {}
	void keyTyped(KeyCode code, Panel* panel) override {}
	void keyReleased(KeyCode code, Panel* panel) override {}
	void keyFocusTicked(Panel* panel) override {}
};


//================================================================
TeamFortressViewport::TeamFortressViewport(int x, int y, int wide, int tall) : Panel(x, y, wide, tall), m_SchemeManager(wide, tall)
{
	gViewPort = this;
	m_iInitialized = false;
	m_pTeamMenu = nullptr;
	m_pClassMenu = nullptr;
	m_pScoreBoard = nullptr;
	m_pSpectatorPanel = nullptr;
	m_pCurrentMenu = nullptr;
	m_pCurrentCommandMenu = nullptr;

	Initialize();
	addInputSignal(new CViewPortInputHandler);

	int r, g, b, a;

	Scheme* pScheme = App::getInstance()->getScheme();

	// primary text color
	// Get the colors
	//!! two different types of scheme here, need to integrate
	SchemeHandle_t hPrimaryScheme = m_SchemeManager.getSchemeHandle("Primary Button Text");
	{
		// font
		pScheme->setFont(Scheme::sf_primary1, m_SchemeManager.getFont(hPrimaryScheme));

		// text color
		m_SchemeManager.getFgColor(hPrimaryScheme, r, g, b, a);
		pScheme->setColor(Scheme::sc_primary1, r, g, b, a); // sc_primary1 is non-transparent orange

		// background color (transparent black)
		m_SchemeManager.getBgColor(hPrimaryScheme, r, g, b, a);
		pScheme->setColor(Scheme::sc_primary3, r, g, b, a);

		// armed foreground color
		m_SchemeManager.getFgArmedColor(hPrimaryScheme, r, g, b, a);
		pScheme->setColor(Scheme::sc_secondary2, r, g, b, a);

		// armed background color
		m_SchemeManager.getBgArmedColor(hPrimaryScheme, r, g, b, a);
		pScheme->setColor(Scheme::sc_primary2, r, g, b, a);

		//!! need to get this color from scheme file
		// used for orange borders around buttons
		m_SchemeManager.getBorderColor(hPrimaryScheme, r, g, b, a);
		// pScheme->setColor(Scheme::sc_secondary1, r, g, b, a );
		pScheme->setColor(Scheme::sc_secondary1, 255 * 0.7, 170 * 0.7, 0, 0);
	}

	// Change the second primary font (used in the scoreboard)
	SchemeHandle_t hScoreboardScheme = m_SchemeManager.getSchemeHandle("Scoreboard Text");
	{
		pScheme->setFont(Scheme::sf_primary2, m_SchemeManager.getFont(hScoreboardScheme));
	}

	// Change the third primary font (used in command menu)
	SchemeHandle_t hCommandMenuScheme = m_SchemeManager.getSchemeHandle("CommandMenu Text");
	{
		pScheme->setFont(Scheme::sf_primary3, m_SchemeManager.getFont(hCommandMenuScheme));
	}

	App::getInstance()->setScheme(pScheme);

	// VGUI MENUS
	CreateTeamMenu();
	CreateClassMenu();
	CreateSpectatorMenu();
	CreateScoreBoard();
	// Init command menus
	m_iNumMenus = 0;
	m_iCurrentTeamNumber = m_iObserverMode = m_iObserverTarget = m_iObserverTarget2 = 0;

	m_StandardMenu = CreateCommandMenu("commandmenu.txt", false, CMENU_TOP, false, CMENU_SIZE_X, BUTTON_SIZE_Y, 0);
	m_SpectatorOptionsMenu = CreateCommandMenu("spectatormenu.txt", true, PANEL_HEIGHT, true, CMENU_SIZE_X, BUTTON_SIZE_Y / 2, 0);						   // above bottom bar, flat design
	m_SpectatorCameraMenu = CreateCommandMenu("spectcammenu.txt", true, PANEL_HEIGHT, true, XRES(200), BUTTON_SIZE_Y / 2, ScreenWidth - (XRES(200) + 15)); // above bottom bar, flat design

	m_PlayerMenu = m_iNumMenus;
	m_iNumMenus++;

	float flLabelSize = ((ScreenWidth - (XRES(CAMOPTIONS_BUTTON_X) + 15)) - XRES(24 + 15)) - XRES((15 + OPTIONS_BUTTON_X + 15) + 38);

	m_pCommandMenus[m_PlayerMenu] = new CCommandMenu(nullptr, true,
		XRES((15 + OPTIONS_BUTTON_X + 15) + 31), PANEL_HEIGHT, flLabelSize, 300);
	m_pCommandMenus[m_PlayerMenu]->setParent(this);
	m_pCommandMenus[m_PlayerMenu]->setVisible(false);
	m_pCommandMenus[m_PlayerMenu]->m_flButtonSizeY = BUTTON_SIZE_Y / 2;
	m_pCommandMenus[m_PlayerMenu]->m_iSpectCmdMenu = true;

	UpdatePlayerMenu(m_PlayerMenu);

	client::AddCommand("servermotd", Viewport_ServerMOTD);
	client::AddCommand("missionbriefing", Viewport_MapBriefing);
	client::AddCommand("classhelp", Viewport_ClassHelp);
	client::AddCommand("changeteam", Viewport_ChangeTeam);
	client::AddCommand("changeclass", Viewport_ChangeClass);
}

//-----------------------------------------------------------------------------
// Purpose: Called everytime a new level is started. Viewport clears out it's data.
//-----------------------------------------------------------------------------
void TeamFortressViewport::Initialize()
{
	// Force each menu to Initialize
	if (m_pTeamMenu)
	{
		m_pTeamMenu->Initialize();
	}
	if (m_pClassMenu)
	{
		m_pClassMenu->Initialize();
	}
	if (m_pScoreBoard)
	{
		m_pScoreBoard->Initialize();
		HideScoreBoard();
	}
	if (m_pSpectatorPanel)
	{
		// Spectator menu doesn't need initializing
		m_pSpectatorPanel->setVisible(false);
	}

	// Make sure all menus are hidden
	HideVGUIMenu();
	HideCommandMenu();

	// Clear out some data
	m_iGotAllMOTD = true;
	m_iRandomPC = false;
	m_flScoreBoardLastUpdated = 0;
	m_flSpectatorPanelLastUpdated = 0;

	// reset player info
	g_iPlayerClass = PC_UNDEFINED;
	g_iTeamNumber = TEAM_UNASSIGNED;

	strcpy(m_sMapName, "");
	strcpy(m_szServerName, "");
	for (int i = 0; i < 5; i++)
	{
		m_iValidClasses[i] = 0;
	}

	App::getInstance()->setCursorOveride(App::getInstance()->getScheme()->getCursor(Scheme::scu_none));
}

//-----------------------------------------------------------------------------
// Purpose: Read the Command Menu structure from the txt file and create the menu.
//			Returns Index of menu in m_pCommandMenus
//-----------------------------------------------------------------------------
int TeamFortressViewport::CreateCommandMenu(const char* menuFile, bool direction, int yOffset, bool flatDesign, float flButtonSizeX, float flButtonSizeY, int xOffset)
{
	// COMMAND MENU
	// Create the root of this new Command Menu

	int newIndex = m_iNumMenus;

	m_pCommandMenus[newIndex] = new CCommandMenu(nullptr, direction, xOffset, yOffset, flButtonSizeX, 300); // This will be resized once we know how many items are in it
	m_pCommandMenus[newIndex]->setParent(this);
	m_pCommandMenus[newIndex]->setVisible(false);
	m_pCommandMenus[newIndex]->m_flButtonSizeY = flButtonSizeY;
	m_pCommandMenus[newIndex]->m_iSpectCmdMenu = direction;

	m_iNumMenus++;

	// Read Command Menu from the txt file
	char token[1024];
	char* pfile = (char*)client::COM_LoadFile(menuFile, 5, nullptr);
	if (!pfile)
	{
		client::Con_DPrintf("Unable to open %s\n", menuFile);
		SetCurrentCommandMenu(nullptr);
		return newIndex;
	}

	// First, read in the localisation strings

	// Detpack strings
	gHUD.m_TextMessage.LocaliseTextString("#DetpackSet_For5Seconds", m_sDetpackStrings[0], MAX_BUTTON_SIZE);
	gHUD.m_TextMessage.LocaliseTextString("#DetpackSet_For20Seconds", m_sDetpackStrings[1], MAX_BUTTON_SIZE);
	gHUD.m_TextMessage.LocaliseTextString("#DetpackSet_For50Seconds", m_sDetpackStrings[2], MAX_BUTTON_SIZE);

	// Now start parsing the menu structure
	m_pCurrentCommandMenu = m_pCommandMenus[newIndex];
	char szLastButtonText[32] = "file start";
	pfile = client::COM_ParseFile(pfile, token);
	while ((strlen(token) > 0) && (m_iNumMenus < MAX_MENUS))
	{
		// Keep looping until we hit the end of this menu
		while (token[0] != '}' && (strlen(token) > 0))
		{
			char cText[32] = "";
			char cBoundKey[32] = "";
			char cCustom[32] = "";
			static const int cCommandLength = 128;
			char cCommand[cCommandLength] = "";
			char szMap[MAX_MAPNAME] = "";
			int iPlayerClass = 0;
			bool iCustom = false;
			int iTeamOnly = -1;
			bool iToggle = false;
			int iButtonY;
			bool bGetExtraToken = true;
			CommandButton* pButton = nullptr;

			// We should never be here without a Command Menu
			if (!m_pCurrentCommandMenu)
			{
				client::Con_Printf("Error in %s file after '%s'.\n", menuFile, szLastButtonText);
				m_iInitialized = false;
				return newIndex;
			}

			// token should already be the bound key, or the custom name
			strncpy(cCustom, token, 32);
			cCustom[31] = '\0';

			// See if it's a custom button
			if (0 == strcmp(cCustom, "CUSTOM"))
			{
				iCustom = true;

				// Get the next token
				pfile = client::COM_ParseFile(pfile, token);
			}
			// See if it's a map
			else if (0 == strcmp(cCustom, "MAP"))
			{
				// Get the mapname
				pfile = client::COM_ParseFile(pfile, token);
				strncpy(szMap, token, MAX_MAPNAME);
				szMap[MAX_MAPNAME - 1] = '\0';

				// Get the next token
				pfile = client::COM_ParseFile(pfile, token);
			}
			else if (0 == strncmp(cCustom, "TEAM", 4)) // TEAM1, TEAM2, TEAM3, TEAM4
			{
				// make it a team only button
				iTeamOnly = atoi(cCustom + 4);

				// Get the next token
				pfile = client::COM_ParseFile(pfile, token);
			}
			else if (0 == strncmp(cCustom, "TOGGLE", 6))
			{
				iToggle = true;
				// Get the next token
				pfile = client::COM_ParseFile(pfile, token);
			}
			else
			{
				// See if it's a Class
				for (int i = PC_SCOUT; i <= PC_ENGINEER; i++)
				{
					if (0 == strcmp(token, sTFClasses[i]))
					{
						// Save it off
						iPlayerClass = i;

						// Get the button text
						pfile = client::COM_ParseFile(pfile, token);
						break;
					}
				}
			}

			// Get the button bound key
			strncpy(cBoundKey, token, 32);
			cText[31] = '\0';

			// Get the button text
			pfile = client::COM_ParseFile(pfile, token);

			CHudTextMessage::LocaliseTextString(token, cText, sizeof(cText));

			// save off the last button text we've come across (for error reporting)
			strcpy(szLastButtonText, cText);

			// Get the button command
			pfile = client::COM_ParseFile(pfile, token);
			strncpy(cCommand, token, cCommandLength);
			cCommand[cCommandLength - 1] = '\0';

			iButtonY = (BUTTON_SIZE_Y - 1) * m_pCurrentCommandMenu->GetNumButtons();

			// Custom button handling
			if (iCustom)
			{
				pButton = CreateCustomButton(cText, cCommand, iButtonY);

				// Get the next token to see if we're a menu
				pfile = client::COM_ParseFile(pfile, token);

				if (token[0] == '{')
				{
					strcpy(cCommand, token);
				}
				else
				{
					bGetExtraToken = false;
				}
			}
			else if (szMap[0] != '\0')
			{
				// create a map button
				pButton = new MapButton(szMap, cText, xOffset, iButtonY, flButtonSizeX, flButtonSizeY);
			}
			else if (iTeamOnly != -1)
			{
				// button that only shows up if the player is on team iTeamOnly
				pButton = new TeamOnlyCommandButton(iTeamOnly, cText, xOffset, iButtonY, flButtonSizeX, flButtonSizeY, flatDesign);
			}
			else if (iToggle && !direction)
			{
				pButton = new ToggleCommandButton(cCommand, cText, xOffset, iButtonY, flButtonSizeX, flButtonSizeY, flatDesign);
			}
			else if (direction)
			{
				if (iToggle)
					pButton = new SpectToggleButton(cCommand, cText, xOffset, iButtonY, flButtonSizeX, flButtonSizeY, flatDesign);
				else
					pButton = new SpectButton(iPlayerClass, cText, xOffset, iButtonY, flButtonSizeX, flButtonSizeY);
			}
			else
			{
				// normal button
				pButton = new CommandButton(iPlayerClass, cText, xOffset, iButtonY, flButtonSizeX, flButtonSizeY, flatDesign);
			}

			// add the button into the command menu
			if (pButton)
			{
				m_pCurrentCommandMenu->AddButton(pButton);
				pButton->setBoundKey(cBoundKey[0]);
				pButton->setParentMenu(m_pCurrentCommandMenu);

				// Override font in CommandMenu
				pButton->setFont(Scheme::sf_primary3);
			}

			// Find out if it's a submenu or a button we're dealing with
			if (cCommand[0] == '{')
			{
				if (m_iNumMenus >= MAX_MENUS)
				{
					client::Con_Printf("Too many menus in %s past '%s'\n", menuFile, szLastButtonText);
				}
				else if (pButton)
				{
					// Create the menu
					m_pCommandMenus[m_iNumMenus] = CreateSubMenu(pButton, m_pCurrentCommandMenu, iButtonY);
					m_pCurrentCommandMenu = m_pCommandMenus[m_iNumMenus];
					m_iNumMenus++;
				}
			}
			else if (!iCustom)
			{
				// Create the button and attach it to the current menu
				if (iToggle)
					pButton->addActionSignal(new CMenuHandler_ToggleCvar(cCommand));
				else
					pButton->addActionSignal(new CMenuHandler_StringCommand(cCommand));
				// Create an input signal that'll popup the current menu
				pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
			}

			// Get the next token
			if (bGetExtraToken)
			{
				pfile = client::COM_ParseFile(pfile, token);
			}
		}

		// Move back up a menu
		m_pCurrentCommandMenu = m_pCurrentCommandMenu->GetParentMenu();

		pfile = client::COM_ParseFile(pfile, token);
	}

	SetCurrentMenu(nullptr);
	SetCurrentCommandMenu(nullptr);
	client::COM_FreeFile(pfile);

	m_iInitialized = true;
	return newIndex;
}

//-----------------------------------------------------------------------------
// Purpose: Creates all the class choices under a spy's disguise menus, and
//			maps a command to them
// Output : CCommandMenu
//-----------------------------------------------------------------------------
CCommandMenu* TeamFortressViewport::CreateDisguiseSubmenu(CommandButton* pButton, CCommandMenu* pParentMenu, const char* commandText, int iYOffset, int iXOffset)
{
	// create the submenu, under which the class choices will be listed
	CCommandMenu* pMenu = CreateSubMenu(pButton, pParentMenu, iYOffset, iXOffset);
	m_pCommandMenus[m_iNumMenus] = pMenu;
	m_iNumMenus++;

	// create the class choice buttons
	for (int i = PC_SCOUT; i <= PC_ENGINEER; i++)
	{
		CommandButton* pDisguiseButton = new CommandButton(CHudTextMessage::BufferedLocaliseTextString(sLocalisedClasses[i]), 0, BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y);

		char sz[256];
		sprintf(sz, "%s %d", commandText, i);
		pDisguiseButton->addActionSignal(new CMenuHandler_StringCommand(sz));

		pMenu->AddButton(pDisguiseButton);
	}

	return pMenu;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : *pButtonText -
//			*pButtonName -
// Output : CommandButton
//-----------------------------------------------------------------------------
CommandButton* TeamFortressViewport::CreateCustomButton(char* pButtonText, char* pButtonName, int iYOffset)
{
	CommandButton* pButton = nullptr;
	CCommandMenu* pMenu = nullptr;

	// ChangeTeam
	if (0 == strcmp(pButtonName, "!CHANGETEAM"))
	{
		// ChangeTeam Submenu
		pButton = new CommandButton(pButtonText, 0, BUTTON_SIZE_Y * 2, CMENU_SIZE_X, BUTTON_SIZE_Y);

		// Create the submenu
		pMenu = CreateSubMenu(pButton, m_pCurrentCommandMenu, iYOffset);
		m_pCommandMenus[m_iNumMenus] = pMenu;
		m_iNumMenus++;

		// ChangeTeam buttons
		for (int i = 0; i < 4; i++)
		{
			char sz[256];
			sprintf(sz, "jointeam %d", i + 1);
			m_pTeamButtons[i] = new TeamButton(i + 1, "teamname", 0, BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y);
			m_pTeamButtons[i]->addActionSignal(new CMenuHandler_StringCommandWatch(sz));
			pMenu->AddButton(m_pTeamButtons[i]);
		}

		// Auto Assign button
		m_pTeamButtons[4] = new TeamButton(5, gHUD.m_TextMessage.BufferedLocaliseTextString("#Team_AutoAssign"), 0, BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y);
		m_pTeamButtons[4]->addActionSignal(new CMenuHandler_StringCommand("jointeam 5"));
		pMenu->AddButton(m_pTeamButtons[4]);

		// Spectate button
		m_pTeamButtons[5] = new SpectateButton(CHudTextMessage::BufferedLocaliseTextString("#Menu_Spectate"), 0, BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y, false);
		m_pTeamButtons[5]->addActionSignal(new CMenuHandler_StringCommand("spectate"));
		pMenu->AddButton(m_pTeamButtons[5]);
	}
	// ChangeClass
	else if (0 == strcmp(pButtonName, "!CHANGECLASS"))
	{
		// Create the Change class menu
		pButton = new ClassButton(-1, pButtonText, 0, BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y, false);

		// ChangeClass Submenu
		pMenu = CreateSubMenu(pButton, m_pCurrentCommandMenu, iYOffset);
		m_pCommandMenus[m_iNumMenus] = pMenu;
		m_iNumMenus++;

		for (int i = PC_SCOUT; i <= PC_RANDOM; i++)
		{
			char sz[256];

			// ChangeClass buttons
			CHudTextMessage::LocaliseTextString(sLocalisedClasses[i], sz, 256);
			ClassButton* pClassButton = new ClassButton(i, sz, 0, BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y, false);

			sprintf(sz, "%s", sTFClassSelection[i]);
			pClassButton->addActionSignal(new CMenuHandler_StringCommandClassSelect(sz));
			pMenu->AddButton(pClassButton);
		}
	}
	// Map Briefing
	else if (0 == strcmp(pButtonName, "!MAPBRIEFING"))
	{
		pButton = new CommandButton(pButtonText, 0, BUTTON_SIZE_Y * m_pCurrentCommandMenu->GetNumButtons(), CMENU_SIZE_X, BUTTON_SIZE_Y);
		pButton->addActionSignal(new CMenuHandler_TextWindow(MENU_MAPBRIEFING));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	// Class Descriptions
	else if (0 == strcmp(pButtonName, "!CLASSDESC"))
	{
		pButton = new ClassButton(0, pButtonText, 0, BUTTON_SIZE_Y * m_pCurrentCommandMenu->GetNumButtons(), CMENU_SIZE_X, BUTTON_SIZE_Y, false);
		pButton->addActionSignal(new CMenuHandler_TextWindow(MENU_CLASSHELP));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	else if (0 == strcmp(pButtonName, "!SERVERINFO"))
	{
		pButton = new ClassButton(0, pButtonText, 0, BUTTON_SIZE_Y * m_pCurrentCommandMenu->GetNumButtons(), CMENU_SIZE_X, BUTTON_SIZE_Y, false);
		pButton->addActionSignal(new CMenuHandler_TextWindow(MENU_INTRO));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	// Spy abilities
	else if (0 == strcmp(pButtonName, "!SPY"))
	{
		pButton = new DisguiseButton(0, pButtonText, 0, BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y);

		m_DisguiseMenu = m_iNumMenus;
	}
	// Feign
	else if (0 == strcmp(pButtonName, "!FEIGN"))
	{
		pButton = new FeignButton(false, pButtonText, 0, BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pButton->addActionSignal(new CMenuHandler_StringCommand("feign"));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	// Feign Silently
	else if (0 == strcmp(pButtonName, "!FEIGNSILENT"))
	{
		pButton = new FeignButton(false, pButtonText, 0, BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pButton->addActionSignal(new CMenuHandler_StringCommand("sfeign"));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	// Stop Feigning
	else if (0 == strcmp(pButtonName, "!FEIGNSTOP"))
	{
		pButton = new FeignButton(true, pButtonText, 0, BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pButton->addActionSignal(new CMenuHandler_StringCommand("feign"));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	// Disguise
	else if (0 == strcmp(pButtonName, "!DISGUISEENEMY"))
	{
		// Create the disguise enemy button, which active only if there are 2 teams
		pButton = new DisguiseButton(DISGUISE_TEAM2, pButtonText, 0, BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y);
		CreateDisguiseSubmenu(pButton, m_pCurrentCommandMenu, "disguise_enemy", iYOffset);
	}
	else if (0 == strcmp(pButtonName, "!DISGUISEFRIENDLY"))
	{
		// Create the disguise friendly button, which active only if there are 1 or 2 teams
		pButton = new DisguiseButton(DISGUISE_TEAM1 | DISGUISE_TEAM2, pButtonText, 0, BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y);
		CreateDisguiseSubmenu(pButton, m_pCurrentCommandMenu, "disguise_friendly", iYOffset);
	}
	else if (0 == strcmp(pButtonName, "!DISGUISE"))
	{
		// Create the Disguise button
		pButton = new DisguiseButton(DISGUISE_TEAM3 | DISGUISE_TEAM4, pButtonText, 0, BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y);
		CCommandMenu* pDisguiseMenu = CreateSubMenu(pButton, m_pCurrentCommandMenu, iYOffset);
		m_pCommandMenus[m_iNumMenus] = pDisguiseMenu;
		m_iNumMenus++;

		// Disguise Enemy submenu buttons
		for (int i = 1; i <= 4; i++)
		{
			// only show the 4th disguise button if we have 4 teams
			m_pDisguiseButtons[i] = new DisguiseButton(((i < 4) ? DISGUISE_TEAM3 : 0) | DISGUISE_TEAM4, "Disguise", 0, BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y);

			pDisguiseMenu->AddButton(m_pDisguiseButtons[i]);
			m_pDisguiseButtons[i]->setParentMenu(pDisguiseMenu);

			char sz[256];
			sprintf(sz, "disguise %d", i);
			CreateDisguiseSubmenu(m_pDisguiseButtons[i], pDisguiseMenu, sz, iYOffset, CMENU_SIZE_X - 1);
		}
	}
	// Start setting a Detpack
	else if (0 == strcmp(pButtonName, "!DETPACKSTART"))
	{
		// Detpack Submenu
		pButton = new DetpackButton(2, pButtonText, 0, BUTTON_SIZE_Y * 2, CMENU_SIZE_X, BUTTON_SIZE_Y);

		// Create the submenu
		pMenu = CreateSubMenu(pButton, m_pCurrentCommandMenu, iYOffset);
		m_pCommandMenus[m_iNumMenus] = pMenu;
		m_iNumMenus++;

		// Set detpack buttons
		CommandButton* pDetButton;
		pDetButton = new CommandButton(m_sDetpackStrings[0], 0, BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pDetButton->addActionSignal(new CMenuHandler_StringCommand("detstart 5"));
		pMenu->AddButton(pDetButton);
		pDetButton = new CommandButton(m_sDetpackStrings[1], 0, BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pDetButton->addActionSignal(new CMenuHandler_StringCommand("detstart 20"));
		pMenu->AddButton(pDetButton);
		pDetButton = new CommandButton(m_sDetpackStrings[2], 0, BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pDetButton->addActionSignal(new CMenuHandler_StringCommand("detstart 50"));
		pMenu->AddButton(pDetButton);
	}
	// Stop setting a Detpack
	else if (0 == strcmp(pButtonName, "!DETPACKSTOP"))
	{
		pButton = new DetpackButton(1, pButtonText, 0, BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pButton->addActionSignal(new CMenuHandler_StringCommand("detstop"));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	// Engineer building
	else if (0 == strcmp(pButtonName, "!BUILD"))
	{
		// only appears if the player is an engineer, and either they have built something or have enough metal to build
		pButton = new BuildButton(BUILDSTATE_BASE, 0, pButtonText, 0, BUTTON_SIZE_Y * 2, CMENU_SIZE_X, BUTTON_SIZE_Y);

		m_BuildMenu = m_iNumMenus;
	}
	else if (0 == strcmp(pButtonName, "!BUILDSENTRY"))
	{
		pButton = new BuildButton(BUILDSTATE_CANBUILD, BuildButton::SENTRYGUN, pButtonText, 0, BUTTON_SIZE_Y * 2, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pButton->addActionSignal(new CMenuHandler_StringCommand("build 2"));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	else if (0 == strcmp(pButtonName, "!BUILDDISPENSER"))
	{
		pButton = new BuildButton(BUILDSTATE_CANBUILD, BuildButton::DISPENSER, pButtonText, 0, BUTTON_SIZE_Y * 2, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pButton->addActionSignal(new CMenuHandler_StringCommand("build 1"));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	else if (0 == strcmp(pButtonName, "!ROTATESENTRY180"))
	{
		pButton = new BuildButton(BUILDSTATE_HASBUILDING, BuildButton::SENTRYGUN, pButtonText, 0, BUTTON_SIZE_Y * 2, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pButton->addActionSignal(new CMenuHandler_StringCommand("rotatesentry180"));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	else if (0 == strcmp(pButtonName, "!ROTATESENTRY"))
	{
		pButton = new BuildButton(BUILDSTATE_HASBUILDING, BuildButton::SENTRYGUN, pButtonText, 0, BUTTON_SIZE_Y * 2, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pButton->addActionSignal(new CMenuHandler_StringCommand("rotatesentry"));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	else if (0 == strcmp(pButtonName, "!DISMANTLEDISPENSER"))
	{
		pButton = new BuildButton(BUILDSTATE_HASBUILDING, BuildButton::DISPENSER, pButtonText, 0, BUTTON_SIZE_Y * 2, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pButton->addActionSignal(new CMenuHandler_StringCommand("dismantle 1"));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	else if (0 == strcmp(pButtonName, "!DISMANTLESENTRY"))
	{
		pButton = new BuildButton(BUILDSTATE_HASBUILDING, BuildButton::SENTRYGUN, pButtonText, 0, BUTTON_SIZE_Y * 2, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pButton->addActionSignal(new CMenuHandler_StringCommand("dismantle 2"));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	else if (0 == strcmp(pButtonName, "!DETONATEDISPENSER"))
	{
		pButton = new BuildButton(BUILDSTATE_HASBUILDING, BuildButton::DISPENSER, pButtonText, 0, BUTTON_SIZE_Y * 2, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pButton->addActionSignal(new CMenuHandler_StringCommand("detdispenser"));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	else if (0 == strcmp(pButtonName, "!DETONATESENTRY"))
	{
		pButton = new BuildButton(BUILDSTATE_HASBUILDING, BuildButton::SENTRYGUN, pButtonText, 0, BUTTON_SIZE_Y * 2, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pButton->addActionSignal(new CMenuHandler_StringCommand("detsentry"));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	else if (0 == strcmp(pButtonName, "!BUILDENTRYTELEPORTER"))
	{
		pButton = new BuildButton(BUILDSTATE_CANBUILD, BuildButton::ENTRY_TELEPORTER, pButtonText, 0, BUTTON_SIZE_Y * 2, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pButton->addActionSignal(new CMenuHandler_StringCommand("build 4"));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	else if (0 == strcmp(pButtonName, "!DISMANTLEENTRYTELEPORTER"))
	{
		pButton = new BuildButton(BUILDSTATE_HASBUILDING, BuildButton::ENTRY_TELEPORTER, pButtonText, 0, BUTTON_SIZE_Y * 2, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pButton->addActionSignal(new CMenuHandler_StringCommand("dismantle 4"));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	else if (0 == strcmp(pButtonName, "!DETONATEENTRYTELEPORTER"))
	{
		pButton = new BuildButton(BUILDSTATE_HASBUILDING, BuildButton::ENTRY_TELEPORTER, pButtonText, 0, BUTTON_SIZE_Y * 2, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pButton->addActionSignal(new CMenuHandler_StringCommand("detentryteleporter"));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	else if (0 == strcmp(pButtonName, "!BUILDEXITTELEPORTER"))
	{
		pButton = new BuildButton(BUILDSTATE_CANBUILD, BuildButton::EXIT_TELEPORTER, pButtonText, 0, BUTTON_SIZE_Y * 2, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pButton->addActionSignal(new CMenuHandler_StringCommand("build 5"));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	else if (0 == strcmp(pButtonName, "!DISMANTLEEXITTELEPORTER"))
	{
		pButton = new BuildButton(BUILDSTATE_HASBUILDING, BuildButton::EXIT_TELEPORTER, pButtonText, 0, BUTTON_SIZE_Y * 2, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pButton->addActionSignal(new CMenuHandler_StringCommand("dismantle 5"));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	else if (0 == strcmp(pButtonName, "!DETONATEEXITTELEPORTER"))
	{
		pButton = new BuildButton(BUILDSTATE_HASBUILDING, BuildButton::EXIT_TELEPORTER, pButtonText, 0, BUTTON_SIZE_Y * 2, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pButton->addActionSignal(new CMenuHandler_StringCommand("detexitteleporter"));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}
	// Stop building
	else if (0 == strcmp(pButtonName, "!BUILDSTOP"))
	{
		pButton = new BuildButton(BUILDSTATE_BUILDING, 0, pButtonText, 0, BUTTON_SIZE_Y * 2, CMENU_SIZE_X, BUTTON_SIZE_Y);
		pButton->addActionSignal(new CMenuHandler_StringCommand("build"));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));
	}

	return pButton;
}

//=======================================================================
void TeamFortressViewport::ShowCommandMenu(int menuIndex)
{
	if (!m_iInitialized)
		return;

	//Already have a menu open.
	if (m_pCurrentMenu)
		return;

	// is the command menu open?
	if (m_pCurrentCommandMenu == m_pCommandMenus[menuIndex])
	{
		HideCommandMenu();
		return;
	}

	// Not visible while in intermission
	if (gHUD.m_iIntermission)
		return;

	// Recalculate visible menus
	UpdateCommandMenu(menuIndex);
	HideVGUIMenu();

	SetCurrentCommandMenu(m_pCommandMenus[menuIndex]);
	m_flMenuOpenTime = gHUD.m_flTime;
	UpdateCursorState();

	// get command menu parameters
	for (int i = 2; i < client::Cmd_Argc(); i++)
	{
		const char* param = client::Cmd_Argv(i - 1);
		if (param)
		{
			if (m_pCurrentCommandMenu->KeyInput(param[0]))
			{
				// kill the menu open time, since the key input is final
				HideCommandMenu();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles the key input of "-commandmenu"
// Input  :
//-----------------------------------------------------------------------------
void TeamFortressViewport::InputSignalHideCommandMenu()
{
	if (!m_iInitialized)
		return;

	// if they've just tapped the command menu key, leave it open
	if ((m_flMenuOpenTime + 0.3) > gHUD.m_flTime)
		return;

	HideCommandMenu();
}

//-----------------------------------------------------------------------------
// Purpose: Hides the command menu
//-----------------------------------------------------------------------------
void TeamFortressViewport::HideCommandMenu()
{
	if (!m_iInitialized)
		return;

	if (m_pCommandMenus[m_StandardMenu])
	{
		m_pCommandMenus[m_StandardMenu]->ClearButtonsOfArmedState();
	}

	if (m_pCommandMenus[m_SpectatorOptionsMenu])
	{
		m_pCommandMenus[m_SpectatorOptionsMenu]->ClearButtonsOfArmedState();
	}

	if (m_pCommandMenus[m_SpectatorCameraMenu])
	{
		m_pCommandMenus[m_SpectatorCameraMenu]->ClearButtonsOfArmedState();
	}

	if (m_pCommandMenus[m_PlayerMenu])
	{
		m_pCommandMenus[m_PlayerMenu]->ClearButtonsOfArmedState();
	}


	m_flMenuOpenTime = 0.0f;
	SetCurrentCommandMenu(nullptr);
	UpdateCursorState();
}

//-----------------------------------------------------------------------------
// Purpose: Bring up the scoreboard
//-----------------------------------------------------------------------------
void TeamFortressViewport::ShowScoreBoard()
{
	if (m_pScoreBoard)
	{
		// No Scoreboard in single-player
		if (util::IsMultiplayer())
		{
			m_pScoreBoard->Open();
			UpdateCursorState();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the scoreboard is up
//-----------------------------------------------------------------------------
bool TeamFortressViewport::IsScoreBoardVisible()
{
	if (m_pScoreBoard)
		return m_pScoreBoard->isVisible();

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Hide the scoreboard
//-----------------------------------------------------------------------------
void TeamFortressViewport::HideScoreBoard()
{
	// Prevent removal of scoreboard during intermission
	if (gHUD.m_iIntermission)
		return;

	if (m_pScoreBoard)
	{
		m_pScoreBoard->setVisible(false);

		GetClientVoiceMgr()->StopSquelchMode();

		UpdateCursorState();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Activate's the player special ability
//			called when the player hits their "special" key
//-----------------------------------------------------------------------------
void TeamFortressViewport::InputPlayerSpecial()
{
	if (!m_iInitialized)
		return;

	if (g_iPlayerClass == PC_SCOUT)
	{
		client::ServerCmd("flaginfo");
	}
	else if (g_iPlayerClass == PC_ENGINEER)
	{
		if ((GetBuildState() & BS_BUILDING) != 0)
		{
			client::ClientCmd("build");
		}
		else
		{
			ShowCommandMenu(m_BuildMenu);
		}
	}
	else if (g_iPlayerClass == PC_MEDIC)
	{
		/* Switch between the Medikit & the previous weapon. */
		if (g_CurrentWeaponId != WEAPON_MEDIKIT)
		{
			g_weaponselect = WEAPON_MEDIKIT;
		}
		else
		{
			g_weaponselect = g_LastWeaponId;
		}
	}
	else if (g_iPlayerClass == PC_HVYWEAP)
	{
		/* Switch between the Assault Cannon & the previous weapon. */
		if (g_CurrentWeaponId != WEAPON_ASSAULT_CANNON)
		{
			g_weaponselect = WEAPON_ASSAULT_CANNON;
		}
		else
		{
			g_weaponselect = g_LastWeaponId;
		}
	}
	else if (g_iPlayerClass == PC_PYRO)
	{
		/* Switch between the Flamethrower & the previous weapon. */
		if (g_CurrentWeaponId != WEAPON_FLAMETHROWER)
		{
			g_weaponselect = WEAPON_FLAMETHROWER;
		}
		else
		{
			g_weaponselect = g_LastWeaponId;
		}
	}
#if 0
	else
	{
		// if it's any other class, just send the command down to the server
		client::ClientCmd("_special");
	}
#endif
}

// Set the submenu of the Command Menu
void TeamFortressViewport::SetCurrentCommandMenu(CCommandMenu* pNewMenu)
{
	for (int i = 0; i < m_iNumMenus; i++)
		m_pCommandMenus[i]->setVisible(false);

	m_pCurrentCommandMenu = pNewMenu;

	if (m_pCurrentCommandMenu)
		m_pCurrentCommandMenu->MakeVisible(nullptr);
}

void TeamFortressViewport::UpdateCommandMenu(int menuIndex)
{
	// if its the player menu update the player list
	if (menuIndex == m_PlayerMenu)
	{
		m_pCommandMenus[m_PlayerMenu]->RemoveAllButtons();
		UpdatePlayerMenu(m_PlayerMenu);
	}

	m_pCommandMenus[menuIndex]->RecalculateVisibles(0, false);
	m_pCommandMenus[menuIndex]->RecalculatePositions(0);
}

void TeamFortressViewport::UpdatePlayerMenu(int menuIndex)
{

	cl_entity_t* pEnt = nullptr;
	float flLabelSize = ((ScreenWidth - (XRES(CAMOPTIONS_BUTTON_X) + 15)) - XRES(24 + 15)) - XRES((15 + OPTIONS_BUTTON_X + 15) + 38);
	gViewPort->GetAllPlayersInfo();


	for (int i = 1; i < MAX_PLAYERS_HUD; i++)
	{
		//if ( g_PlayerInfoList[i].name == nullptr )
		//	continue; // empty player slot, skip

		pEnt = client::GetEntityByIndex(i);

		if (!gHUD.m_Spectator.IsActivePlayer(pEnt))
			continue;

		//if ( g_PlayerExtraInfo[i].teamname[0] == 0 )
		//	continue; // skip over players who are not in a team

		SpectButton* pButton = new SpectButton(1, g_PlayerInfoList[pEnt->index].name,
			XRES((15 + OPTIONS_BUTTON_X + 15) + 31), PANEL_HEIGHT + (i - 1) * CMENU_SIZE_X, flLabelSize, BUTTON_SIZE_Y / 2);

		pButton->setBoundKey((char)255);
		pButton->setContentAlignment(vgui::Label::a_center);
		m_pCommandMenus[menuIndex]->AddButton(pButton);
		pButton->setParentMenu(m_pCommandMenus[menuIndex]);

		// Override font in CommandMenu
		pButton->setFont(Scheme::sf_primary3);

		pButton->addActionSignal(new CMenuHandler_SpectateFollow(g_PlayerInfoList[pEnt->index].name));
		// Create an input signal that'll popup the current menu
		pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCommandMenus[menuIndex]));
	}
}



void COM_FileBase(const char* in, char* out);
const char* GetSpectatorLabel(int iMode);

void TeamFortressViewport::UpdateSpectatorPanel()
{
	m_iObserverMode = gHUD.GetObserverMode();
	m_iObserverTarget = gHUD.GetObserverTarget();
	m_iObserverTarget2 = gHUD.GetObserverTarget2();

	if (!m_pSpectatorPanel)
		return;

	if (gHUD.GetObserverMode() == OBS_FIXED)
	{
		return;
	}

	if (gHUD.IsObserver()
	 && gHUD.GetObserverMode() != OBS_DEATHCAM
	 && 0 != gHUD.m_pCvarDraw->value
	 && !gHUD.m_iIntermission) // don't draw in dev_overview mode
	{
		char bottomText[128];
		char helpString2[128];
		char tempString[128];
		char* name;
		char* pBottomText = nullptr;
		int player = 0;

		// check if spectator combinations are still valid
		gHUD.m_Spectator.CheckSettings();

		if (!m_pSpectatorPanel->isVisible())
		{
			m_pSpectatorPanel->setVisible(true); // show spectator panel, but
			m_pSpectatorPanel->ShowMenu(false);	 // dsiable all menus/buttons

			if (gHUD.IsSpectator())
			{
				snprintf(tempString, sizeof(tempString) - 1, "%c%s", HUD_PRINTCENTER, CHudTextMessage::BufferedLocaliseTextString("#Spec_Duck"));
				tempString[sizeof(tempString) - 1] = '\0';

				gHUD.m_TextMessage.MsgFunc_TextMsg(nullptr, strlen(tempString) + 1, tempString);
			}
		}

		strcpy(bottomText, GetSpectatorLabel(gHUD.GetObserverMode()));
		strcpy(helpString2, GetSpectatorLabel(gHUD.GetObserverMode()));

		if (0 != client::IsSpectateOnly())
			strcat(helpString2, " - HLTV");

		// check if we're locked onto a target, show the player's name
		if ((gHUD.GetObserverTarget() > 0) && (gHUD.GetObserverTarget() <= client::GetMaxClients()) && (gHUD.GetObserverMode() != OBS_ROAMING))
		{
			player = gHUD.GetObserverTarget();
		}

		// special case in free map and inset off, don't show names
		if ((gHUD.GetObserverMode() == OBS_MAP_FREE) && 0 == gHUD.m_Spectator.m_pip->value)
			name = nullptr;
		else
			name = g_PlayerInfoList[player].name;

		// create player & health string
		if (0 != player && name)
		{
			strncpy(bottomText, name, sizeof(bottomText));
			bottomText[sizeof(bottomText) - 1] = 0;
			pBottomText = bottomText;
		}
		else
		{
			pBottomText = CHudTextMessage::BufferedLocaliseTextString(bottomText);
		}

		// in first person mode colorize player names
		if (0 != player && name)
		{
			auto color = gHUD.GetClientColor(player);
			int r = color[0] * 255;
			int g = color[1] * 255;
			int b = color[2] * 255;

			// set team color, a bit transparent
			m_pSpectatorPanel->m_BottomMainLabel->setFgColor(r, g, b, 0);
			m_pSpectatorPanel->m_BottomMainButton->setFgColor(r, g, b, 0);
		}
		else
		{ // restore GUI color
			m_pSpectatorPanel->m_BottomMainLabel->setFgColor(143, 143, 54, 0);
			m_pSpectatorPanel->m_BottomMainButton->setFgColor(143, 143, 54, 0);
		}

		// add sting auto if we are in auto directed mode
		if (0 != gHUD.m_Spectator.m_autoDirector->value)
		{
			char tempString[128];
			sprintf(tempString, "#Spec_Auto %s", helpString2);
			strcpy(helpString2, tempString);
		}

		m_pSpectatorPanel->m_BottomMainLabel->setText("%s", pBottomText);
		m_pSpectatorPanel->m_BottomMainButton->setText(pBottomText);


		// update extra info field
		char szText[64];

		if (0 != client::IsSpectateOnly())
		{
			// in HLTV mode show number of spectators
			snprintf(szText, 63, "%s: %d", CHudTextMessage::BufferedLocaliseTextString("#Spectators"), gHUD.m_Spectator.m_iSpectatorNumber);
		}
		else
		{
			// otherwise show map name
			char szMapName[64];
			COM_FileBase(client::GetLevelName(), szMapName);

			snprintf(szText, 63, "%s: %s", CHudTextMessage::BufferedLocaliseTextString("#Spec_Map"), szMapName);
		}

		szText[63] = 0;

		m_pSpectatorPanel->m_ExtraInfo->setText(szText);

		/*
		int timer = (int)( gHUD.m_roundTimer.m_flTimeEnd - gHUD.m_flTime );

		if ( timer < 0 )
			 timer	= 0;

		snprintf ( szText, 63, "%d:%02d\n", (timer / 60), (timer % 60) );
		
		szText[63] = 0;
				
		m_pSpectatorPanel->m_CurrentTime->setText( szText ); */

		// update spectator panel
		gViewPort->m_pSpectatorPanel->Update();
	}
	else
	{
		if (m_pSpectatorPanel->isVisible())
		{
			m_pSpectatorPanel->setVisible(false);
			m_pSpectatorPanel->ShowMenu(false); // dsiable all menus/buttons
		}
	}

	m_flSpectatorPanelLastUpdated = gHUD.m_flTime + 1.0; // update every second
}

//======================================================================
void TeamFortressViewport::CreateScoreBoard()
{
	int xdent = SBOARD_INDENT_X, ydent = SBOARD_INDENT_Y;
	if (ScreenWidth == 512)
	{
		xdent = SBOARD_INDENT_X_512;
		ydent = SBOARD_INDENT_Y_512;
	}
	else if (ScreenWidth == 400)
	{
		xdent = SBOARD_INDENT_X_400;
		ydent = SBOARD_INDENT_Y_400;
	}

	m_pScoreBoard = new ScoreBoard(xdent, ydent, ScreenWidth - (xdent * 2), ScreenHeight - (ydent * 2));
	m_pScoreBoard->setParent(this);
	m_pScoreBoard->setVisible(false);
}

//======================================================================
// Set the VGUI Menu
void TeamFortressViewport::SetCurrentMenu(CMenuPanel* pMenu)
{
	m_pCurrentMenu = pMenu;
	if (m_pCurrentMenu)
	{
		// Don't open menus in demo playback
		if (0 != client::demo::IsPlayingback())
			return;

		m_pCurrentMenu->Open();
	}
	else
	{
		client::ClientCmd("closemenus;");
	}
}

//================================================================
// Text Window
CMenuPanel* TeamFortressViewport::CreateTextWindow(int iTextToShow)
{
	char sz[256];
	const char* cText = "";
	char* pfile = nullptr;
	static const int MAX_TITLE_LENGTH = 64;
	char cTitle[MAX_TITLE_LENGTH];

	if (iTextToShow == SHOW_MOTD)
	{
		if (!m_szServerName || '\0' == m_szServerName[0])
			strcpy(cTitle, HALFLIFE_GAMEDESC);
		else
			strncpy(cTitle, m_szServerName, sizeof(cTitle));
		cTitle[sizeof(cTitle) - 1] = 0;
		cText = m_szMOTD;
	}
	else if (iTextToShow == SHOW_MAPBRIEFING)
	{
		// Get the current mapname, and open it's map briefing text
		if (m_sMapName && '\0' != m_sMapName[0])
		{
			strcpy(sz, "maps/");
			strcat(sz, m_sMapName);
			strcat(sz, ".txt");
		}
		else
		{
			const char* level = client::GetLevelName();
			if (!level)
				return nullptr;

			strcpy(sz, level);
			char* ch = strchr(sz, '.');
			*ch = '\0';
			strcat(sz, ".txt");

			// pull out the map name
			strcpy(m_sMapName, level);
			ch = strchr(m_sMapName, '.');
			if (ch)
			{
				*ch = 0;
			}

			ch = strchr(m_sMapName, '/');
			if (ch)
			{
				// move the string back over the '/'
				memmove(m_sMapName, ch + 1, strlen(ch) + 1);
			}
		}

		pfile = (char*)client::COM_LoadFile(sz, 5, nullptr);

		if (pfile != nullptr)
			cText = pfile;
		else
			cText = gHUD.m_TextMessage.BufferedLocaliseTextString("#Map_Description_not_available");

		strncpy(cTitle, m_sMapName, MAX_TITLE_LENGTH);
		cTitle[MAX_TITLE_LENGTH - 1] = 0;
	}
	else if (iTextToShow == SHOW_SPECHELP)
	{
		CHudTextMessage::LocaliseTextString("#Spec_Help_Title", cTitle, MAX_TITLE_LENGTH);
		cTitle[MAX_TITLE_LENGTH - 1] = 0;

		char* pfile = CHudTextMessage::BufferedLocaliseTextString("#Spec_Help_Text");
		if (pfile)
		{
			cText = pfile;
		}
	}
	else if (iTextToShow == SHOW_CLASSDESC)
	{
		switch (g_iPlayerClass)
		{
		case PC_SCOUT:
			cText = CHudTextMessage::BufferedLocaliseTextString("#Help_scout");
			CHudTextMessage::LocaliseTextString("#Title_scout", cTitle, MAX_TITLE_LENGTH);
			break;
		case PC_SNIPER:
			cText = CHudTextMessage::BufferedLocaliseTextString("#Help_sniper");
			CHudTextMessage::LocaliseTextString("#Title_sniper", cTitle, MAX_TITLE_LENGTH);
			break;
		case PC_SOLDIER:
			cText = CHudTextMessage::BufferedLocaliseTextString("#Help_soldier");
			CHudTextMessage::LocaliseTextString("#Title_soldier", cTitle, MAX_TITLE_LENGTH);
			break;
		case PC_DEMOMAN:
			cText = CHudTextMessage::BufferedLocaliseTextString("#Help_demoman");
			CHudTextMessage::LocaliseTextString("#Title_demoman", cTitle, MAX_TITLE_LENGTH);
			break;
		case PC_MEDIC:
			cText = CHudTextMessage::BufferedLocaliseTextString("#Help_medic");
			CHudTextMessage::LocaliseTextString("#Title_medic", cTitle, MAX_TITLE_LENGTH);
			break;
		case PC_HVYWEAP:
			cText = CHudTextMessage::BufferedLocaliseTextString("#Help_hwguy");
			CHudTextMessage::LocaliseTextString("#Title_hwguy", cTitle, MAX_TITLE_LENGTH);
			break;
		case PC_PYRO:
			cText = CHudTextMessage::BufferedLocaliseTextString("#Help_pyro");
			CHudTextMessage::LocaliseTextString("#Title_pyro", cTitle, MAX_TITLE_LENGTH);
			break;
		case PC_SPY:
			cText = CHudTextMessage::BufferedLocaliseTextString("#Help_spy");
			CHudTextMessage::LocaliseTextString("#Title_spy", cTitle, MAX_TITLE_LENGTH);
			break;
		case PC_ENGINEER:
			cText = CHudTextMessage::BufferedLocaliseTextString("#Help_engineer");
			CHudTextMessage::LocaliseTextString("#Title_engineer", cTitle, MAX_TITLE_LENGTH);
			break;
		case PC_CIVILIAN:
			cText = CHudTextMessage::BufferedLocaliseTextString("#Help_civilian");
			CHudTextMessage::LocaliseTextString("#Title_civilian", cTitle, MAX_TITLE_LENGTH);
			break;
		default:
			return nullptr;
		}

		if (g_iPlayerClass == PC_CIVILIAN)
		{
			sprintf(sz, "classes/long_civilian.txt");
		}
		else
		{
			sprintf(sz, "classes/long_%s.txt", sTFClassSelection[g_iPlayerClass]);
		}
		char* pfile = (char*)client::COM_LoadFile(sz, 5, nullptr);

		if (pfile != nullptr)
			cText = pfile;
		else
			cText = gHUD.m_TextMessage.BufferedLocaliseTextString("#Map_Description_not_available"); /* Toodles FIXME: */
	}

	// if we're in the game (ie. have selected a class), flag the menu to be only grayed in the dialog box, instead of full screen
	CMenuPanel* pMOTDPanel = CMessageWindowPanel_Create(cText, cTitle, g_iPlayerClass == PC_UNDEFINED, false, 0, 0, ScreenWidth, ScreenHeight);
	pMOTDPanel->setParent(this);

	if (pfile)
		client::COM_FreeFile(pfile);

	return pMOTDPanel;
}

//================================================================
// VGUI Menus
void TeamFortressViewport::ShowVGUIMenu(int iMenu)
{
	CMenuPanel* pNewMenu = nullptr;

	// Don't open menus in demo playback
	if (0 != client::demo::IsPlayingback())
		return;

	// Don't open any menus except the MOTD during intermission
	// MOTD needs to be accepted because it's sent down to the client
	// after map change, before intermission's turned off
	if (gHUD.m_iIntermission && iMenu != MENU_INTRO)
		return;

	// Don't create one if it's already in the list
	if (m_pCurrentMenu)
	{
		CMenuPanel* pMenu = m_pCurrentMenu;
		while (pMenu != nullptr)
		{
			if (pMenu->GetMenuID() == iMenu)
				return;
			pMenu = pMenu->GetNextMenu();
		}
	}

	switch (iMenu)
	{
	case MENU_TEAM:
		pNewMenu = ShowTeamMenu();
		break;

	// Map Briefing removed now that it appears in the team menu
	case MENU_MAPBRIEFING:
		pNewMenu = CreateTextWindow(SHOW_MAPBRIEFING);
		break;

	case MENU_INTRO:
		pNewMenu = CreateTextWindow(SHOW_MOTD);
		break;

	case MENU_CLASSHELP:
		pNewMenu = CreateTextWindow(SHOW_CLASSDESC);
		break;

	case MENU_SPECHELP:
		pNewMenu = CreateTextWindow(SHOW_SPECHELP);
		break;
	case MENU_CLASS:
		pNewMenu = ShowClassMenu();
		break;

	default:
		break;
	}

	if (!pNewMenu)
		return;

	// Close the Command Menu if it's open
	HideCommandMenu();

	pNewMenu->SetMenuID(iMenu);
	pNewMenu->SetActive(true);
	pNewMenu->setParent(this);

	// See if another menu is visible, and if so, cache this one for display once the other one's finished
	if (m_pCurrentMenu)
	{
		if (m_pCurrentMenu->GetMenuID() == MENU_CLASS && iMenu == MENU_TEAM)
		{
			CMenuPanel* temp = m_pCurrentMenu;
			m_pCurrentMenu->Close();
			m_pCurrentMenu = pNewMenu;
			m_pCurrentMenu->SetNextMenu(temp);
			m_pCurrentMenu->Open();
			UpdateCursorState();
		}
		else
		{
			pNewMenu->setVisible(false);
			m_pCurrentMenu->SetNextMenu(pNewMenu);
		}
	}
	else
	{
		m_pCurrentMenu = pNewMenu;
		m_pCurrentMenu->Open();
		UpdateCursorState();
	}
}

// Removes all VGUI Menu's onscreen
void TeamFortressViewport::HideVGUIMenu()
{
	while (m_pCurrentMenu)
	{
		HideTopMenu();
	}
}

// Remove the top VGUI menu, and bring up the next one
void TeamFortressViewport::HideTopMenu()
{
	if (m_pCurrentMenu)
	{
		// Close the top one
		m_pCurrentMenu->Close();

		// Bring up the next one
		gViewPort->SetCurrentMenu(m_pCurrentMenu->GetNextMenu());
	}

	UpdateCursorState();
}

//======================================================================================
// TEAM MENU
//======================================================================================
// Bring up the Team selection Menu
CMenuPanel* TeamFortressViewport::ShowTeamMenu()
{
	// Don't open menus in demo playback
	if (0 != client::demo::IsPlayingback())
		return nullptr;

	m_pTeamMenu->Reset();
	return m_pTeamMenu;
}

void TeamFortressViewport::CreateTeamMenu()
{
	// Create the panel
	m_pTeamMenu = new CTeamMenuPanel(100, false, 0, 0, ScreenWidth, ScreenHeight);
	m_pTeamMenu->setParent(this);
	m_pTeamMenu->setVisible(false);
}

//======================================================================================
// CLASS MENU
//======================================================================================
// Bring up the Class selection Menu
CMenuPanel* TeamFortressViewport::ShowClassMenu()
{
	// Don't open menus in demo playback
	if (0 != client::demo::IsPlayingback())
		return nullptr;

	m_pClassMenu->Reset();
	return m_pClassMenu;
}

void TeamFortressViewport::CreateClassMenu()
{
	// Create the panel
	m_pClassMenu = new CClassMenuPanel(100, false, 0, 0, ScreenWidth, ScreenHeight);
	m_pClassMenu->setParent(this);
	m_pClassMenu->setVisible(false);
}

//======================================================================================
//======================================================================================
// SPECTATOR MENU
//======================================================================================
// Spectator "Menu" explaining the Spectator buttons
void TeamFortressViewport::CreateSpectatorMenu()
{
	// Create the Panel
	m_pSpectatorPanel = new SpectatorPanel(0, 0, ScreenWidth, ScreenHeight);
	m_pSpectatorPanel->setParent(this);
	m_pSpectatorPanel->setVisible(false);
	m_pSpectatorPanel->Initialize();
}

//======================================================================================
// UPDATE HUD SECTIONS
//======================================================================================
// We've got an update on player info
// Recalculate any menus that use it.
void TeamFortressViewport::UpdateOnPlayerInfo()
{
	if (m_pTeamMenu)
		m_pTeamMenu->Update();
	if (m_pClassMenu)
		m_pClassMenu->Update();
	if (m_pScoreBoard)
		m_pScoreBoard->Update();
}

void TeamFortressViewport::UpdateCursorState()
{
	// Need cursor if any VGUI window is up
	if (m_pSpectatorPanel->m_menuVisible || m_pCurrentMenu || m_pTeamMenu->isVisible() || GetClientVoiceMgr()->IsInSquelchMode())
	{
		g_iVisibleMouse = true;
		App::getInstance()->setCursorOveride(App::getInstance()->getScheme()->getCursor(Scheme::scu_arrow));
		return;
	}
	else if (m_pCurrentCommandMenu)
	{
		// commandmenu doesn't have cursor if hud_capturemouse is turned off
		if (gHUD.m_pCvarStealMouse->value != 0.0f)
		{
			g_iVisibleMouse = true;
			App::getInstance()->setCursorOveride(App::getInstance()->getScheme()->getCursor(Scheme::scu_arrow));
			return;
		}
	}

	// Don't reset mouse in demo playback
	if (0 == client::demo::IsPlayingback())
	{
		Mouse_Reset();
	}

	g_iVisibleMouse = false;
	App::getInstance()->setCursorOveride(App::getInstance()->getScheme()->getCursor(Scheme::scu_none));
}

void TeamFortressViewport::UpdateHighlights()
{
	if (m_pCurrentCommandMenu)
		m_pCurrentCommandMenu->MakeVisible(nullptr);
}

void TeamFortressViewport::GetAllPlayersInfo()
{
	for (int i = 1; i < MAX_PLAYERS_HUD; i++)
	{
		client::GetPlayerInfo(i, &g_PlayerInfoList[i]);

		const auto steamID = client::PlayerInfo_ValueForKey(i, "*sid");
		if (steamID != nullptr)
		{
			g_PlayerInfoList[i].m_nSteamID = strtoull(steamID, nullptr, 0);
		}
	}
}

void TeamFortressViewport::paintBackground()
{
	int wide, tall;
	getParent()->getSize(wide, tall);
	setSize(wide, tall);
	if (m_pScoreBoard)
	{
		int x, y;
		getApp()->getCursorPos(x, y);
		m_pScoreBoard->cursorMoved(x, y, m_pScoreBoard);
	}

	// See if the command menu is visible and needs recalculating due to some external change
	if (g_iTeamNumber != m_iCurrentTeamNumber)
	{
		UpdateCommandMenu(m_StandardMenu);

		if (m_pClassMenu)
		{
			m_pClassMenu->Update();
		}

		m_iCurrentTeamNumber = g_iTeamNumber;
	}

	if (g_iPlayerClass != m_iCurrentPlayerClass)
	{
		UpdateCommandMenu(m_StandardMenu);

		m_iCurrentPlayerClass = g_iPlayerClass;
	}

	// See if the Spectator Menu needs to be update
	if ((gHUD.GetObserverMode() != m_iObserverMode || gHUD.GetObserverTarget() != m_iObserverTarget) ||
		(m_flSpectatorPanelLastUpdated < gHUD.m_flTime))
	{
		UpdateSpectatorPanel();
	}

	// Update the Scoreboard, if it's visible
	if (m_pScoreBoard->isVisible()
	 && (m_flScoreBoardLastUpdated < gHUD.m_flTime
	 || GetClientVoiceMgr()->IsInSquelchMode()))
	{
		m_pScoreBoard->Update();
		m_flScoreBoardLastUpdated = gHUD.m_flTime + 0.2;
	}

	int extents[4];
	getAbsExtents(extents[0], extents[1], extents[2], extents[3]);
	VGui_ViewportPaintBackground(extents);
}

//================================================================
// Input Handler for Drag N Drop panels
void CDragNDropHandler::cursorMoved(int x, int y, Panel* panel)
{
	if (m_bDragging)
	{
		App::getInstance()->getCursorPos(x, y);
		m_pPanel->setPos(m_iaDragOrgPos[0] + (x - m_iaDragStart[0]), m_iaDragOrgPos[1] + (y - m_iaDragStart[1]));

		if (m_pPanel->getParent() != null)
		{
			m_pPanel->getParent()->repaint();
		}
	}
}

void CDragNDropHandler::mousePressed(MouseCode code, Panel* panel)
{
	int x, y;
	App::getInstance()->getCursorPos(x, y);
	m_bDragging = true;
	m_iaDragStart[0] = x;
	m_iaDragStart[1] = y;
	m_pPanel->getPos(m_iaDragOrgPos[0], m_iaDragOrgPos[1]);
	App::getInstance()->setMouseCapture(panel);

	m_pPanel->setDragged(m_bDragging);
	m_pPanel->requestFocus();
}

void CDragNDropHandler::mouseReleased(MouseCode code, Panel* panel)
{
	m_bDragging = false;
	m_pPanel->setDragged(m_bDragging);
	App::getInstance()->setMouseCapture(null);
}

//================================================================
// Number Key Input
bool TeamFortressViewport::SlotInput(int iSlot)
{
	// If there's a menu up, give it the input
	if (m_pCurrentMenu)
		return m_pCurrentMenu->SlotInput(iSlot);

	return false;
}

// Direct Key Input
bool TeamFortressViewport::KeyInput(bool down, int keynum, const char* pszCurrentBinding)
{
	if (!down || 0 != client::Con_IsVisible())
	{
		return true;
	}

	// Open Text Window?
	if (m_pCurrentMenu)
	{
		int iMenuID = m_pCurrentMenu->GetMenuID();

		// Get number keys as Input for Team/Class menus
		if (iMenuID == MENU_TEAM || iMenuID == MENU_CLASS)
		{
			for (int i = '0'; i <= '9'; i++)
			{
				if (keynum == i)
				{
					SlotInput(i - '0');
					return false;
				}
			}
		}

		// Grab enter keys to close TextWindows
		if (keynum == K_ENTER || keynum == K_KP_ENTER || keynum == K_SPACE)
		{
			if (iMenuID == MENU_MAPBRIEFING || iMenuID == MENU_INTRO || iMenuID == MENU_CLASSHELP)
			{
				HideTopMenu();
				return false;
			}
		}

		// Grab jump key on Team Menu as autoassign
		if (pszCurrentBinding && 0 == strcmp(pszCurrentBinding, "+jump"))
		{
			if (iMenuID == MENU_TEAM)
			{
				m_pTeamMenu->SlotInput(5);
				return false;
			}
			else if (iMenuID == MENU_CLASS)
			{
				m_pClassMenu->SlotInput(0);
				return false;
			}
		}
	}

	if (keynum == K_MOUSE2 && IsScoreBoardVisible())
	{
		if (!GetClientVoiceMgr()->IsInSquelchMode())
		{
			GetClientVoiceMgr()->StartSquelchMode();
		}
		else
		{
			GetClientVoiceMgr()->StopSquelchMode();
		}
		return false;
	}

	// if we're in a command menu, try hit one of it's buttons
	if (m_pCurrentCommandMenu)
	{
		// only trap the number keys
		if (keynum >= '0' && keynum <= '9')
		{
			if (m_pCurrentCommandMenu->KeyInput(keynum))
			{
				// a final command has been issued, so close the command menu
				HideCommandMenu();
			}

			return false;
		}
	}

	if (gHUD.m_Menu.m_fMenuDisplayed != CHudMenu::kNone)
	{
		if (gHUD.m_Menu.m_fMenuDisplayed == CHudMenu::kVote)
		{
			if (keynum >= K_F1 && keynum <= K_F12)
			{
				gHUD.m_Menu.SelectMenuItem(keynum - K_F1 + 1);
				return false;
			}
		}
		else if (keynum >= '1' && keynum <= '9')
		{
			gHUD.m_Menu.SelectMenuItem(keynum - '0');
			return false;
		}
	}

	return true;
}

void TeamFortressViewport::Update_Detpack(const int setting)
{
	if (setting != 1)
	{
		/* Release the special key. */
		g_bForceSpecialDown = false;
	}

	if (GetIsSettingDetpack() != setting)
	{
		m_iIsSettingDetpack = setting;

		// Force the menu to update
		UpdateCommandMenu(m_StandardMenu);
	}
}

void TeamFortressViewport::Update_BuildSt(const int setting)
{
	if (GetBuildState() != setting)
	{
		m_iBuildState = setting;

		// Force the menu to update
		UpdateCommandMenu(m_StandardMenu);
	}
}

//================================================================
// Message Handlers
bool TeamFortressViewport::MsgFunc_ValClass(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	for (int i = 0; i < 5; i++)
		m_iValidClasses[i] = READ_SHORT();

	// Force the menu to update
	UpdateCommandMenu(m_StandardMenu);

	return true;
}

bool TeamFortressViewport::MsgFunc_TeamNames(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	m_iNumberOfTeams = READ_BYTE();

	for (int i = 0; i < m_iNumberOfTeams; i++)
	{
		int teamNum = i + 1;

		gHUD.m_TextMessage.LocaliseTextString(READ_STRING(), g_TeamInfo[teamNum].name, MAX_TEAMNAME_SIZE);

		// Set the team name buttons
		if (m_pTeamButtons[i])
			m_pTeamButtons[i]->setText(g_TeamInfo[teamNum].name);

		// range check this value...m_pDisguiseButtons[5];
		if (teamNum < 5)
		{
			// Set the disguise buttons
			if (m_pDisguiseButtons[teamNum])
				m_pDisguiseButtons[teamNum]->setText(g_TeamInfo[teamNum].name);
		}
	}

	// Update the Team Menu
	if (m_pTeamMenu)
		m_pTeamMenu->Update();

	return true;
}

bool TeamFortressViewport::MsgFunc_Feign(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	m_iIsFeigning = READ_BYTE() != 0;

	// Force the menu to update
	UpdateCommandMenu(m_StandardMenu);

	return true;
}

bool TeamFortressViewport::MsgFunc_Detpack(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	Update_Detpack(READ_BYTE());

	return true;
}

bool TeamFortressViewport::MsgFunc_VGUIMenu(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int iMenu = READ_BYTE();

	ShowVGUIMenu(iMenu);

	return true;
}

bool TeamFortressViewport::MsgFunc_MOTD(const char* pszName, int iSize, void* pbuf)
{
	if (m_iGotAllMOTD)
		m_szMOTD[0] = 0;

	BEGIN_READ(pbuf, iSize);

	int gotAllMOTD = READ_BYTE();

	m_iGotAllMOTD = gotAllMOTD != 0;

	if (gotAllMOTD == 2)
	{
		char* motd = (char*)client::COM_LoadFile("motd.txt", 5, nullptr);
		if (motd != nullptr)
		{
			strncpy(m_szMOTD, motd, sizeof(m_szMOTD) - 1);
			m_szMOTD[sizeof(m_szMOTD) - 1] = '\0';

			client::COM_FreeFile(motd);
		}

		if (0 == client::IsSpectateOnly())
		{
			ShowVGUIMenu(MENU_INTRO);
		}
		return;
	}

	int roomInArray = sizeof(m_szMOTD) - strlen(m_szMOTD) - 1;

	strncat(m_szMOTD, READ_STRING(), roomInArray >= 0 ? roomInArray : 0);
	m_szMOTD[sizeof(m_szMOTD) - 1] = '\0';

	// don't show MOTD for HLTV spectators
	if (m_iGotAllMOTD && 0 == client::IsSpectateOnly())
	{
		ShowVGUIMenu(MENU_INTRO);
	}

	return true;
}

bool TeamFortressViewport::MsgFunc_BuildSt(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	Update_BuildSt(READ_SHORT());

	return true;
}

bool TeamFortressViewport::MsgFunc_RandomPC(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	m_iRandomPC = READ_BYTE() != 0;

	return true;
}

bool TeamFortressViewport::MsgFunc_ServerName(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	strncpy(m_szServerName, READ_STRING(), sizeof(m_szServerName));
	m_szServerName[sizeof(m_szServerName) - 1] = 0;

	strncpy(m_sMapName, READ_STRING(), sizeof(m_sMapName));
	m_sMapName[sizeof(m_sMapName) - 1] = '\0';

	return true;
}

bool TeamFortressViewport::MsgFunc_ScoreInfo(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	auto cl = READ_BYTE();
	auto score = READ_SHORT();
	auto deaths = READ_SHORT();

	if (cl > 0 && cl <= MAX_PLAYERS_HUD)
	{
		g_PlayerExtraInfo[cl].score = score;
		g_PlayerExtraInfo[cl].deaths = deaths;

		UpdateOnPlayerInfo();
	}

	return true;
}

bool TeamFortressViewport::MsgFunc_ExtraInfo(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	auto cl = READ_BYTE();
	auto role = READ_BYTE();
	auto flags = READ_BYTE();

	if (cl > 0 && cl <= MAX_PLAYERS_HUD)
	{
		extra_player_info_t& info = g_PlayerExtraInfo[cl];

		info.playerclass = role & 31;
		info.teamnumber = role >> 5;
		info.dead = (flags & 1) != 0;
		info.lefthanded = (flags & 2) != 0;
		info.bot = (flags & 4) != 0;

		auto localPlayer = client::GetLocalPlayer();

		/* Not a LAN game. */
		if (g_PlayerInfoList[localPlayer->index].m_nSteamID != 0)
		{
			/* Bot will be indicated by a zero Steam ID. */
			info.bot |= g_PlayerInfoList[cl].m_nSteamID == 0;
		}

		if (cl == localPlayer->index)
		{
			g_iPlayerClass = info.playerclass;
			g_iTeamNumber = info.teamnumber;
		}

		UpdateOnPlayerInfo();
	}

	return true;
}

// Message handler for TeamScore message
// accepts three values:
//		string: team name
//		short: teams kills
//		short: teams deaths
// if this message is never received, then scores will simply be thegViewPort->GetNumberOfTeams combined totals of the players.
bool TeamFortressViewport::MsgFunc_TeamScore(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	auto teamnumber = READ_BYTE();
	auto score = READ_SHORT();

	g_TeamInfo[teamnumber].score = score;

	UpdateOnPlayerInfo();

	return true;
}

bool TeamFortressViewport::MsgFunc_AllowSpec(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	m_iAllowSpectators = READ_BYTE() != 0;

	// Force the menu to update
	UpdateCommandMenu(m_StandardMenu);

	// If the team menu is up, update it too
	if (m_pTeamMenu)
		m_pTeamMenu->Update();

	return true;
}


// used to reset the player's screen immediately
bool TeamFortressViewport::MsgFunc_ResetFade(const char* pszName, int iSize, void* pbuf)
{
	/* Toodles FIXME: */
#if 0
	if (!gpGlobals)
		return 0;

	screenfade_t sf;
	client::GetScreenFade(&sf);

	sf.fader = 0;
	sf.fadeg = 0;
	sf.fadeb = 0;
	sf.fadealpha = 0;

	sf.fadeEnd = 0.1;
	sf.fadeReset = 0.0;
	sf.fadeSpeed = 0.0;

	sf.fadeFlags = FFADE_IN;

	sf.fadeReset += gpGlobals->time;
	sf.fadeEnd += sf.fadeReset;

	client::SetScreenFade(&sf);
#endif

	return true;
}

// used to fade a player's screen out/in when they're spectating someone who is teleported
bool TeamFortressViewport::MsgFunc_SpecFade(const char* pszName, int iSize, void* pbuf)
{
	/* Toodles FIXME: */
#if 0
	BEGIN_READ(pbuf, iSize);

	int iIndex = READ_BYTE();

	// we're in first-person spectator mode (...not first-person in the PIP)
	if (g_iUser1 == OBS_IN_EYE)
	{
		// this is the person we're watching
		if (g_iUser2 == iIndex)
		{
			int iFade = READ_BYTE();
			int iTeam = READ_BYTE();
			float flTime = ((float)READ_SHORT() / 100.0);
			int iAlpha = READ_BYTE();

			Vector team = GetTeamColor(iTeam);

			screenfade_t sf;
			client::GetScreenFade(&sf);

			sf.fader = team[0];
			sf.fadeg = team[1];
			sf.fadeb = team[2];
			sf.fadealpha = iAlpha;

			sf.fadeEnd = flTime;
			sf.fadeReset = 0.0;
			sf.fadeSpeed = 0.0;

			if (iFade == BUILD_TELEPORTER_FADE_OUT)
			{
				sf.fadeFlags = FFADE_OUT;
				sf.fadeReset = flTime;

				if (sf.fadeEnd)
					sf.fadeSpeed = -(float)sf.fadealpha / sf.fadeEnd;

				sf.fadeTotalEnd = sf.fadeEnd += gpGlobals->time;
				sf.fadeReset += sf.fadeEnd;
			}
			else
			{
				sf.fadeFlags = FFADE_IN;

				if (sf.fadeEnd)
					sf.fadeSpeed = (float)sf.fadealpha / sf.fadeEnd;

				sf.fadeReset += gpGlobals->time;
				sf.fadeEnd += sf.fadeReset;
			}

			client::SetScreenFade(&sf);
		}
	}
#endif

	return true;
}
