//========= Copyright ï¿½ 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#pragma once

#include <VGUI_Panel.h>
#include <VGUI_TablePanel.h>
#include <VGUI_HeaderPanel.h>
#include <VGUI_TextGrid.h>
#include <VGUI_Label.h>
#include <VGUI_TextImage.h>
#include "vgui_listbox.h"
#include "vgui_avatarimage.h"

#include <ctype.h>

// Scoreboard cells
enum
{
	COLUMN_AVATAR = 0,
	COLUMN_NAME,
	COLUMN_CLASS,
	COLUMN_SCORE,
	COLUMN_LATENCY,
	COLUMN_VOICE,
	COLUMN_BLANK,
	NUM_COLUMNS
};

using namespace vgui;

class CTextImage2 : public Image
{
public:
	CTextImage2()
	{
		_image[0] = new TextImage("");
		_image[1] = new TextImage("");
	}

	~CTextImage2()
	{
		delete _image[0];
		delete _image[1];
	}

	TextImage* GetImage(int image)
	{
		return _image[image];
	}

	void getSize(int& wide, int& tall) override
	{
		int w1, w2, t1, t2;
		_image[0]->getTextSize(w1, t1);
		_image[1]->getTextSize(w2, t2);

		wide = w1 + w2;
		tall = std::max(t1, t2);
		setSize(wide, tall);
	}

	void doPaint(Panel* panel) override
	{
		_image[0]->doPaint(panel);
		_image[1]->doPaint(panel);
	}

	void setPos(int x, int y) override
	{
		_image[0]->setPos(x, y);

		int swide, stall;
		_image[0]->getSize(swide, stall);

		int wide, tall;
		_image[1]->getSize(wide, tall);
		_image[1]->setPos(x + wide, y + (stall * 0.9) - tall);
	}

	void setColor(Color color) override
	{
		_image[0]->setColor(color);
	}

	void setColor2(Color color)
	{
		_image[1]->setColor(color);
	}

private:
	TextImage* _image[2];
};

//-----------------------------------------------------------------------------
// Purpose: Custom label for cells in the Scoreboard's Table Header
//-----------------------------------------------------------------------------
class CLabelHeader : public Label
{
public:
	CLabelHeader() : Label("")
	{
		_dualImage = new CTextImage2();
		_dualImage->setColor2(Color(255, 170, 0, 0));
		_row = -2;
		_useFgColorAsImageColor = true;
		_offset[0] = 0;
		_offset[1] = 0;
	}

	~CLabelHeader()
	{
		delete _dualImage;
	}

	void setTeam(int team)
	{
		_team = team;
	}

	void setRow(int row)
	{
		_row = row;
	}

	void setFgColorAsImageColor(bool state)
	{
		_useFgColorAsImageColor = state;
	}

	void setText(int textBufferLen, const char* text) override
	{
		_dualImage->GetImage(0)->setText(text);

		// calculate the text size
		Font* font = _dualImage->GetImage(0)->getFont();
		_gap = 0;
		for (const char* ch = text; *ch != 0; ch++)
		{
			int a, b, c;
			font->getCharABCwide(*ch, a, b, c);
			_gap += (a + b + c);
		}

		_gap += XRES(5);
	}

	virtual void setText(const char* text)
	{
		// strip any non-alnum characters from the end
		char buf[512];
		strcpy(buf, text);

		int len = strlen(buf);
		while (len && isspace(buf[--len]))
		{
			buf[len] = 0;
		}

		CLabelHeader::setText(0, buf);
	}

	void setText2(const char* text)
	{
		_dualImage->GetImage(1)->setText(text);
	}

	void getTextSize(int& wide, int& tall) override
	{
		_dualImage->getSize(wide, tall);
	}

	void setFgColor(int r, int g, int b, int a) override
	{
		Label::setFgColor(r, g, b, a);
		Color color(r, g, b, a);
		_dualImage->setColor(color);
		_dualImage->setColor2(color);
		repaint();
	}

	void setFgColor(Scheme::SchemeColor sc) override
	{
		int r, g, b, a;
		Label::setFgColor(sc);
		Label::getFgColor(r, g, b, a);

		// Call the r,g,b,a version so it sets the color in the dualImage..
		setFgColor(r, g, b, a);
	}

	void setFont(Font* font) override
	{
		_dualImage->GetImage(0)->setFont(font);
	}

	void setFont2(Font* font)
	{
		_dualImage->GetImage(1)->setFont(font);
	}

	// this adjust the absolute position of the text after alignment is calculated
	void setTextOffset(int x, int y)
	{
		_offset[0] = x;
		_offset[1] = y;
	}

	void paint() override;
	void paintBackground() override;
	void calcAlignment(int iwide, int itall, int& x, int& y);

private:
	CTextImage2* _dualImage;
	int _team;
	int _row;
	int _gap;
	int _offset[2];
	bool _useFgColorAsImageColor;
};

class ScoreTablePanel;

#include "vgui_grid.h"
#include "vgui_defaultinputsignal.h"

//-----------------------------------------------------------------------------
// Purpose: Scoreboard back panel
//-----------------------------------------------------------------------------
class ScorePanel : public Panel, public vgui::CDefaultInputSignal
{
private:
	CGrid m_HeaderGrid;
	CLabelHeader m_HeaderLabels[NUM_COLUMNS];

	vgui::CListBox m_PlayerList;
	CGrid m_PlayerGrids[MAX_PLAYERS_HUD];
	CLabelHeader m_PlayerEntries[NUM_COLUMNS][MAX_PLAYERS_HUD];
	int m_iTeamNumber;

	bool _showAvatars;
	CAvatarImagePanel _playerAvatars[MAX_PLAYERS_HUD];

public:
	int m_iRows;
	int m_iSortedRows[MAX_PLAYERS_HUD];

public:
	ScorePanel(int x, int y, int wide, int tall, int team);

	void Update();

	void SortPlayers();

	void FillGrid();

	void Open();

	void MouseOverCell(int row, int col);

protected:
	bool ShowAvatars() { return _showAvatars; }

public:
	void mousePressed(MouseCode code, Panel* panel) override;
	void cursorMoved(int x, int y, Panel* panel) override;

	friend class CLabelHeader;
};

class ScoreBoard : public Panel, public vgui::CDefaultInputSignal
{
private:
	Label m_TitleLabel;

	ScorePanel* m_pScorePanels[TEAM_SPECTATORS];

	CommandButton* m_pCloseButton;

	static cvar_t *_showPlayerAvatars;

public:
	bool m_bHasBeenSorted[MAX_PLAYERS_HUD];

	int m_iHighlightTeam;
	int m_iHighlightRow;

	static bool ShowPlayerAvatars() { return _showPlayerAvatars->value != 0.0F; }

public:
	ScoreBoard(int x, int y, int wide, int tall);

	void Initialize();
	void Open();
	void Update();
};
