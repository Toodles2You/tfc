//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: VGUI scoreboard
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================


#include <VGUI_LineBorder.h>

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "com_weapons.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_ScorePanel.h"
#include "vgui_helpers.h"
#include "vgui_loadtga.h"
#include "voice_status.h"
#include "vgui_SpectatorPanel.h"

extern hud_player_info_t g_PlayerInfoList[MAX_PLAYERS_HUD + 1];	   // player info from the engine
extern extra_player_info_t g_PlayerExtraInfo[MAX_PLAYERS_HUD + 1]; // additional player info sent directly to the client dll
team_info_t g_TeamInfo[TEAM_SPECTATORS];

// Scoreboard dimensions
#define SBOARD_TITLE_SIZE_Y YRES(22)

#define X_BORDER XRES(4)

// Column sizes
class SBColumnInfo
{
public:
	const char* m_pTitle; // If null, ignore, if starts with #, it's localized, otherwise use the string directly.
	int m_Width;		  // Based on 640 width. Scaled to fit other resolutions.
	Label::Alignment m_Alignment;
};

// grid size is marked out for 640x480 screen
static SBColumnInfo g_ColumnInfo[NUM_COLUMNS] =
{
	{nullptr,		24,		Label::a_east},		// tracker column
	{nullptr,		140,	Label::a_west},		// name
	{nullptr,		102,	Label::a_east},		// class
	{"#SCORE",		40,		Label::a_east},		// score
	{"#LATENCY",	46,		Label::a_east},		// ping
	{nullptr,		40,		Label::a_east},		// voice
	{nullptr,		2,		Label::a_east},		// blank column to take up the slack
};

//-----------------------------------------------------------------------------
// Purpose: Create the ScoreBoard panel
//-----------------------------------------------------------------------------
ScorePanel::ScorePanel(int x, int y, int wide, int tall, int team) : Panel(x, y, wide, tall)
{
	CSchemeManager* pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle("Scoreboard Title Text");
	SchemeHandle_t hSmallScheme = pSchemes->getSchemeHandle("Scoreboard Small Text");
	Font* tfont = pSchemes->getFont(hTitleScheme);
	Font* smallfont = pSchemes->getFont(hSmallScheme);

	m_iTeamNumber = team;

	setPaintBackgroundEnabled(false);

	// Setup the header (labels like "name", "class", etc..).
	m_HeaderGrid.SetDimensions(NUM_COLUMNS, 1);
	m_HeaderGrid.SetSpacing(0, 0);

	for (int i = 0; i < NUM_COLUMNS; i++)
	{
		if (g_ColumnInfo[i].m_pTitle && g_ColumnInfo[i].m_pTitle[0] == '#')
			m_HeaderLabels[i].setText(CHudTextMessage::BufferedLocaliseTextString(g_ColumnInfo[i].m_pTitle));
		else if (g_ColumnInfo[i].m_pTitle)
			m_HeaderLabels[i].setText(g_ColumnInfo[i].m_pTitle);

		int xwide = g_ColumnInfo[i].m_Width;
		if (m_iTeamNumber != TEAM_UNASSIGNED)
		{
			xwide /= 2;
		}
		if (ScreenWidth >= 640)
		{
			xwide = XRES(xwide);
		}
		else if (ScreenWidth == 400)
		{
			// hack to make 400x300 resolution scoreboard fit
			if (i == 1)
			{
				// reduces size of player name cell
				xwide -= 28;
			}
			else if (i == 0)
			{
				// tracker icon cell
				xwide -= 8;
			}
		}

		m_HeaderGrid.SetColumnWidth(i, xwide);
		m_HeaderGrid.SetEntry(i, 0, &m_HeaderLabels[i]);

		m_HeaderLabels[i].setBgColor(0, 0, 0, 255);
		m_HeaderLabels[i].setFgColor(Scheme::sc_primary1);
		m_HeaderLabels[i].setFont(smallfont);
		m_HeaderLabels[i].setContentAlignment(g_ColumnInfo[i].m_Alignment);

		if (i == COLUMN_NAME)
		{
			m_HeaderLabels[i].setFont(tfont);
		}

		int yres = 12;
		if (ScreenHeight >= 480)
		{
			yres = YRES(yres);
		}
		m_HeaderLabels[i].setSize(50, yres);
	}

	// Set the width of the last column to be the remaining space.
	int ex, ey, ew, eh;
	m_HeaderGrid.GetEntryBox(NUM_COLUMNS - 2, 0, ex, ey, ew, eh);
	m_HeaderGrid.SetColumnWidth(NUM_COLUMNS - 1, (wide - X_BORDER) - (ex + ew));

	m_HeaderGrid.AutoSetRowHeights();
	m_HeaderGrid.setBounds(X_BORDER, SBOARD_TITLE_SIZE_Y, wide - X_BORDER * 2, m_HeaderGrid.GetRowHeight(0));
	m_HeaderGrid.setParent(this);
	m_HeaderGrid.setBgColor(0, 0, 0, 255);


	// Now setup the listbox with the actual player data in it.
	int headerX, headerY, headerWidth, headerHeight;
	m_HeaderGrid.getBounds(headerX, headerY, headerWidth, headerHeight);
	m_PlayerList.setBounds(headerX, headerY + headerHeight, headerWidth, tall - headerY - headerHeight - 6);
	m_PlayerList.setBgColor(0, 0, 0, 255);
	m_PlayerList.setParent(this);

	for (int row = 0; row < MAX_PLAYERS_HUD; row++)
	{
		CGrid* pGridRow = &m_PlayerGrids[row];

		pGridRow->SetDimensions(NUM_COLUMNS, 1);

		for (int col = 0; col < NUM_COLUMNS; col++)
		{
			m_PlayerEntries[col][row].setContentFitted(false);
			m_PlayerEntries[col][row].setTeam(m_iTeamNumber);
			m_PlayerEntries[col][row].setRow(row);
			m_PlayerEntries[col][row].addInputSignal(this);
			pGridRow->SetEntry(col, 0, &m_PlayerEntries[col][row]);
		}

		pGridRow->setBgColor(0, 0, 0, 255);
		pGridRow->SetSpacing(0, 0);
		pGridRow->CopyColumnWidths(&m_HeaderGrid);
		pGridRow->AutoSetRowHeights();
		pGridRow->setSize(PanelWidth(pGridRow), pGridRow->CalcDrawHeight());
		pGridRow->RepositionContents();

		m_PlayerList.AddItem(pGridRow);
	}
}

bool HACK_GetPlayerUniqueID(int iPlayer, char playerID[16])
{
	return 0 != gEngfuncs.GetPlayerUniqueID(iPlayer, playerID); // TODO remove after testing
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate the internal scoreboard data
//-----------------------------------------------------------------------------
void ScorePanel::Update()
{
	int i;

	if (m_iTeamNumber != TEAM_UNASSIGNED)
	{
		m_HeaderLabels[COLUMN_NAME].setText(gViewPort->GetTeamName(m_iTeamNumber));

		auto color = gHUD.GetTeamColor(m_iTeamNumber);
		m_HeaderLabels[COLUMN_NAME].setFgColor(
			color[0] * 255,
			color[1] * 255,
			color[2] * 255,
			0);
	}

	m_iRows = 0;
	gViewPort->GetAllPlayersInfo();

	// Clear out sorts
	for (i = 0; i < MAX_PLAYERS_HUD; i++)
	{
		m_iSortedRows[i] = 0;
	}

	// If it's not teamplay, sort all the players. Otherwise, sort the teams.
	SortPlayers();

	// set scrollbar range
	m_PlayerList.SetScrollRange(m_iRows);

	FillGrid();
}

//-----------------------------------------------------------------------------
// Purpose: Sort a list of players
//-----------------------------------------------------------------------------
void ScorePanel::SortPlayers()
{
	// draw the players in order
	while (true)
	{
		// Find the top ranking player
		int highest_score = -99999;
		int lowest_deaths = 99999;
		int best_player;
		best_player = 0;

		for (int i = 1; i < MAX_PLAYERS_HUD; i++)
		{
			if (!gViewPort->GetScoreBoard()->m_bHasBeenSorted[i]
			 && g_PlayerInfoList[i].name
			 && g_PlayerExtraInfo[i].score >= highest_score)
			{
				if (m_iTeamNumber == TEAM_UNASSIGNED
				 || m_iTeamNumber == g_PlayerExtraInfo[i].teamnumber)
				{
					extra_player_info_t* pl_info = &g_PlayerExtraInfo[i];
					if (pl_info->score > highest_score || pl_info->deaths < lowest_deaths)
					{
						best_player = i;
						lowest_deaths = pl_info->deaths;
						highest_score = pl_info->score;
					}
				}
			}
		}

		if (0 == best_player)
			break;

		// Put this player in the sorted list
		m_iSortedRows[m_iRows] = best_player;
		gViewPort->GetScoreBoard()->m_bHasBeenSorted[best_player] = true;
		m_iRows++;
	}
}


void ScorePanel::FillGrid()
{
	CSchemeManager* pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hScheme = pSchemes->getSchemeHandle("Scoreboard Text");
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle("Scoreboard Title Text");
	SchemeHandle_t hSmallScheme = pSchemes->getSchemeHandle("Scoreboard Small Text");

	Font* sfont = pSchemes->getFont(hScheme);
	Font* tfont = pSchemes->getFont(hTitleScheme);
	Font* smallfont = pSchemes->getFont(hSmallScheme);

	// update highlight position
	int x, y;
	getApp()->getCursorPos(x, y);
	cursorMoved(x, y, this);

	bool bNextRowIsGap = false;
	int row;
	for (row = 0; row < MAX_PLAYERS_HUD; row++)
	{
		CGrid* pGridRow = &m_PlayerGrids[row];
		pGridRow->SetRowUnderline(0, false, 0, 0, 0, 0, 0);

		if (row >= m_iRows)
		{
			for (int col = 0; col < NUM_COLUMNS; col++)
				m_PlayerEntries[col][row].setVisible(false);

			continue;
		}

		bool bRowIsGap = false;
		if (bNextRowIsGap)
		{
			bNextRowIsGap = false;
			bRowIsGap = true;
		}

		for (int col = 0; col < NUM_COLUMNS; col++)
		{
			CLabelHeader* pLabel = &m_PlayerEntries[col][row];

			pLabel->setVisible(true);
			pLabel->setText2("");
			pLabel->setImage(nullptr);
			pLabel->setFont(sfont);
			pLabel->setTextOffset(0, 0);

			int rowheight = 13;
			if (ScreenHeight > 480)
			{
				rowheight = YRES(rowheight);
			}
			else
			{
				// more tweaking, make sure icons fit at low res
				rowheight = 15;
			}
			pLabel->setSize(pLabel->getWide(), rowheight);
			pLabel->setBgColor(0, 0, 0, 255);

			char sz[128];
			hud_player_info_t* pl_info = nullptr;
			team_info_t* team_info = nullptr;

			auto color = gHUD.GetClientColor(m_iSortedRows[row]);

			// team color text for player names
			pLabel->setFgColor(color[0] * 255, color[1] * 255, color[2] * 255, 0);

			// Get the player's data
			pl_info = &g_PlayerInfoList[m_iSortedRows[row]];

			// Set background color
			if (0 != pl_info->thisplayer) // if it is their name, draw it a different color
			{
				// Highlight this player
				pLabel->setFgColor(Scheme::sc_white);
				pLabel->setBgColor(color[0] * 255, color[1] * 255, color[2] * 255, 196);
			}

			// Align
			if (col == COLUMN_NAME)
			{
				pLabel->setContentAlignment(vgui::Label::a_west);
			}
			else if (col == COLUMN_TRACKER || col == COLUMN_CLASS)
			{
				pLabel->setContentAlignment(vgui::Label::a_center);
			}
			else
			{
				pLabel->setContentAlignment(vgui::Label::a_east);
			}

			// Fill out with the correct data
			strcpy(sz, "");

			switch (col)
			{
			case COLUMN_TRACKER:
				break;
			case COLUMN_NAME:
				sprintf(sz, "%s  ", pl_info->name);
				break;
			case COLUMN_VOICE:
				sz[0] = 0;
				GetClientVoiceMgr()->UpdateSpeakerImage(pLabel, m_iSortedRows[row]);
				break;
			case COLUMN_CLASS:
				sz[0] = '\0';
				if (g_PlayerExtraInfo[m_iSortedRows[row]].dead)
				{
					strcpy(sz, CHudTextMessage::BufferedLocaliseTextString("#DEAD"));
				}
				break;
			case COLUMN_SCORE:
				sprintf(sz, "%d", g_PlayerExtraInfo[m_iSortedRows[row]].score);
				break;
			case COLUMN_LATENCY:
				if (g_PlayerInfoList[m_iSortedRows[row]].m_nSteamID == 0)
				{
					strcpy(sz, CHudTextMessage::BufferedLocaliseTextString("#BOT"));
				}
				else
				{
					sprintf(sz, "%d", g_PlayerInfoList[m_iSortedRows[row]].ping);
				}
				break;
			default:
				break;
			}

			pLabel->setText(sz);
		}
	}

	for (row = 0; row < MAX_PLAYERS_HUD; row++)
	{
		CGrid* pGridRow = &m_PlayerGrids[row];

		pGridRow->AutoSetRowHeights();
		pGridRow->setSize(PanelWidth(pGridRow), pGridRow->CalcDrawHeight());
		pGridRow->RepositionContents();
	}

	// hack, for the thing to resize
	m_PlayerList.getSize(x, y);
	m_PlayerList.setSize(x, y);
}


void ScorePanel::Open()
{
	Update();
	setVisible(true);
}


void ScorePanel::mousePressed(MouseCode code, Panel* panel)
{
	if (gHUD.m_iIntermission)
		return;

	if (!GetClientVoiceMgr()->IsInSquelchMode())
		return;

	if (gViewPort->GetScoreBoard()->m_iHighlightTeam != m_iTeamNumber)
		return;

	int iPlayer = m_iSortedRows[gViewPort->GetScoreBoard()->m_iHighlightRow];
	if (iPlayer <= 0)
		return;

	// print text message
	hud_player_info_t* pl_info = &g_PlayerInfoList[iPlayer];

	if (pl_info && pl_info->name && '\0' != pl_info->name[0])
	{
		char string[256];
		if (GetClientVoiceMgr()->IsPlayerBlocked(iPlayer))
		{
			char string[1024];

			// remove mute
			GetClientVoiceMgr()->SetPlayerBlockedState(iPlayer, false);

			sprintf(string, CHudTextMessage::BufferedLocaliseTextString("#Unmuted"), pl_info->name);
			gHUD.m_SayText.SayTextPrint(string);
		}
		else
		{
			char string[1024];

			// mute the player
			GetClientVoiceMgr()->SetPlayerBlockedState(iPlayer, true);

			sprintf(string, CHudTextMessage::BufferedLocaliseTextString("#Muted"), pl_info->name);
			gHUD.m_SayText.SayTextPrint(string);
			sprintf(string, CHudTextMessage::BufferedLocaliseTextString("#No_longer_hear_that_player"), pl_info->name);
			gHUD.m_SayText.SayTextPrint(string);
		}
	}
}


void ScorePanel::cursorMoved(int x, int y, Panel* panel)
{
	if (GetClientVoiceMgr()->IsInSquelchMode())
	{
		// look for which cell the mouse is currently over
		for (int i = 0; i < MAX_PLAYERS_HUD; i++)
		{
			int row, col;
			if (m_PlayerGrids[i].getCellAtPoint(x, y, row, col))
			{
				MouseOverCell(i, col);
				return;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles mouse movement over a cell
// Input  : row -
//			col -
//-----------------------------------------------------------------------------
void ScorePanel::MouseOverCell(int row, int col)
{
	CLabelHeader* label = &m_PlayerEntries[col][row];

	// clear the previously highlighted label
	if (!label)
		return;

	// don't act on disconnected players or ourselves
	hud_player_info_t* pl_info = &g_PlayerInfoList[m_iSortedRows[row]];
	if (!pl_info->name || '\0' == pl_info->name[0])
		return;

	if (0 != pl_info->thisplayer && 0 == gEngfuncs.IsSpectateOnly())
		return;

	// setup the new highlight
	gViewPort->GetScoreBoard()->m_iHighlightTeam = m_iTeamNumber;
	gViewPort->GetScoreBoard()->m_iHighlightRow = row;
}


//-----------------------------------------------------------------------------
// Purpose: Label paint functions - take into account current highligh status
//-----------------------------------------------------------------------------
void CLabelHeader::paintBackground()
{
	Color oldBg;
	getBgColor(oldBg);

	if (gViewPort->GetScoreBoard()->m_iHighlightTeam == _team
	 && gViewPort->GetScoreBoard()->m_iHighlightRow == _row)
	{
		setBgColor(134, 91, 19, 0);
	}

	Panel::paintBackground();

	setBgColor(oldBg);
}


//-----------------------------------------------------------------------------
// Purpose: Label paint functions - take into account current highligh status
//-----------------------------------------------------------------------------
void CLabelHeader::paint()
{
	Color oldFg;
	getFgColor(oldFg);

	if (gViewPort->GetScoreBoard()->m_iHighlightTeam == _team
	 && gViewPort->GetScoreBoard()->m_iHighlightRow == _row)
	{
		setFgColor(255, 255, 255, 0);
	}

	// draw text
	int x, y, iwide, itall;
	getTextSize(iwide, itall);
	calcAlignment(iwide, itall, x, y);
	_dualImage->setPos(x, y);

	int x1, y1;
	_dualImage->GetImage(1)->getPos(x1, y1);
	_dualImage->GetImage(1)->setPos(_gap, y1);

	_dualImage->doPaint(this);

	// get size of the panel and the image
	if (_image)
	{
		Color imgColor;
		getFgColor(imgColor);
		if (_useFgColorAsImageColor)
		{
			_image->setColor(imgColor);
		}

		_image->getSize(iwide, itall);
		calcAlignment(iwide, itall, x, y);
		_image->setPos(x, y);
		_image->doPaint(this);
	}

	setFgColor(oldFg[0], oldFg[1], oldFg[2], oldFg[3]);
}


void CLabelHeader::calcAlignment(int iwide, int itall, int& x, int& y)
{
	// calculate alignment ourselves, since vgui is so broken
	int wide, tall;
	getSize(wide, tall);

	x = 0, y = 0;

	// align left/right
	switch (_contentAlignment)
	{
	// left
	case Label::a_northwest:
	case Label::a_west:
	case Label::a_southwest:
	{
		x = 0;
		break;
	}

	// center
	case Label::a_north:
	case Label::a_center:
	case Label::a_south:
	{
		x = (wide - iwide) / 2;
		break;
	}

	// right
	case Label::a_northeast:
	case Label::a_east:
	case Label::a_southeast:
	{
		x = wide - iwide;
		break;
	}
	}

	// top/down
	switch (_contentAlignment)
	{
	// top
	case Label::a_northwest:
	case Label::a_north:
	case Label::a_northeast:
	{
		y = 0;
		break;
	}

	// center
	case Label::a_west:
	case Label::a_center:
	case Label::a_east:
	{
		y = (tall - itall) / 2;
		break;
	}

	// south
	case Label::a_southwest:
	case Label::a_south:
	case Label::a_southeast:
	{
		y = tall - itall;
		break;
	}
	}

	// don't clip to Y
	//	if (y < 0)
	//	{
	//		y = 0;
	//	}
	if (x < 0)
	{
		x = 0;
	}

	x += _offset[0];
	y += _offset[1];
}


ScoreBoard::ScoreBoard(int x, int y, int wide, int tall) : Panel(x, y, wide, tall)
{
	CSchemeManager* pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle("Scoreboard Title Text");
	SchemeHandle_t hSmallScheme = pSchemes->getSchemeHandle("Scoreboard Small Text");
	Font* tfont = pSchemes->getFont(hTitleScheme);
	Font* smallfont = pSchemes->getFont(hSmallScheme);

	setBgColor(0, 0, 0, 96);
	m_iHighlightTeam = -1;
	m_iHighlightRow = -1;

	// Initialize the top title.
	m_TitleLabel.setFont(tfont);
	m_TitleLabel.setText("");
	m_TitleLabel.setBgColor(0, 0, 0, 255);
	m_TitleLabel.setFgColor(Scheme::sc_primary1);
	m_TitleLabel.setContentAlignment(vgui::Label::a_west);

	LineBorder* border = new LineBorder(Color(60, 60, 60, 128));
	setBorder(border);
	setPaintBorderEnabled(true);

	int xpos = g_ColumnInfo[0].m_Width + 3;
	if (ScreenWidth >= 640)
	{
		// only expand column size for res greater than 640
		xpos = XRES(xpos);
	}
	m_TitleLabel.setBounds(xpos, 4, wide, SBOARD_TITLE_SIZE_Y);
	m_TitleLabel.setContentFitted(false);
	m_TitleLabel.setParent(this);

	m_pScorePanels[TEAM_UNASSIGNED] = new ScorePanel(0, 0, wide, tall, TEAM_UNASSIGNED);
	m_pScorePanels[TEAM_UNASSIGNED]->setParent(this);
	m_pScorePanels[TEAM_UNASSIGNED]->setVisible(false);

	int panelTall = tall;

	for (int i = TEAM_DEFAULT; i < TEAM_SPECTATORS; i++)
	{
		m_pScorePanels[i] =
			new ScorePanel(
				((i - 1) % 2) * (wide / 2),
				((i - 1) / 2) * (panelTall / 2),
				wide / 2,
				panelTall,
				i);

		m_pScorePanels[i]->setParent(this);
		m_pScorePanels[i]->setVisible(false);
	}

	m_pCloseButton = new CommandButton("x", wide - XRES(12 + 4), YRES(2), XRES(12), YRES(12));
	m_pCloseButton->setParent(this);
	m_pCloseButton->addActionSignal(new CMenuHandler_StringCommandWatch("-showscores", true));
	m_pCloseButton->setBgColor(0, 0, 0, 255);
	m_pCloseButton->setFgColor(255, 255, 255, 0);
	m_pCloseButton->setFont(tfont);
	m_pCloseButton->setBoundKey((char)255);
	m_pCloseButton->setContentAlignment(Label::a_center);

	Initialize();
}


void ScoreBoard::Initialize()
{
	// Clear out scoreboard data
	memset(g_PlayerExtraInfo, 0, sizeof g_PlayerExtraInfo);
	memset(g_TeamInfo, 0, sizeof g_TeamInfo);
}


void ScoreBoard::Open()
{
	if (util::GetGameMode() < kGamemodeTeamplay)
	{
		m_pScorePanels[TEAM_UNASSIGNED]->setVisible(true);

		for (int i = TEAM_DEFAULT; i < TEAM_SPECTATORS; i++)
		{
			m_pScorePanels[i]->setVisible(false);
		}
	}
	else
	{
		m_pScorePanels[TEAM_UNASSIGNED]->setVisible(false);

		int wide, tall;
		m_pScorePanels[TEAM_UNASSIGNED]->getSize(wide, tall);
		if (gViewPort->GetNumberOfTeams() > 2)
		{
			tall /= 2;
		}

		for (int i = TEAM_DEFAULT; i < TEAM_SPECTATORS; i++)
		{
			m_pScorePanels[i]->setVisible(i <= gViewPort->GetNumberOfTeams());
			m_pScorePanels[i]->setSize(wide, tall);
		}
	}

	Update();
	setVisible(true);
}


void ScoreBoard::Update()
{
	if (gViewPort->m_szServerName)
	{
		char sz[MAX_SERVERNAME_LENGTH + 16];
		sprintf(sz, "%s", gViewPort->m_szServerName);
		m_TitleLabel.setText(sz);
	}

	gViewPort->GetAllPlayersInfo();

	memset(m_bHasBeenSorted, 0, sizeof(m_bHasBeenSorted));

	m_iHighlightTeam = -1;
	m_iHighlightRow = -1;

	if (util::GetGameMode() < kGamemodeTeamplay)
	{
		m_pScorePanels[TEAM_UNASSIGNED]->Update();
	}
	else
	{
		for (int i = TEAM_DEFAULT; i <= gViewPort->GetNumberOfTeams(); i++)
		{
			m_pScorePanels[i]->Update();
		}
	}

	m_pCloseButton->setVisible(gViewPort->m_pSpectatorPanel->m_menuVisible);
}

