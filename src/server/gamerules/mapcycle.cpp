//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Whatever
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "gamerules.h"
#include "game.h"
#include "filesystem_utils.h"
#include "vote_manager.h"

#include <algorithm>

#define MAX_RULE_BUFFER 1024

typedef struct mapcycle_item_s
{
	struct mapcycle_item_s* next;

	char mapname[32];
	int minplayers, maxplayers;
	char rulebuffer[MAX_RULE_BUFFER];
} mapcycle_item_t;

typedef struct mapcycle_s
{
	struct mapcycle_item_s* items;
	struct mapcycle_item_s* next_item;
} mapcycle_t;

static char szPreviousMapCycleFile[256];
static mapcycle_t mapcycle;
static std::string nextlevel;

/*
==============
DestroyMapCycle

Clean up memory used by mapcycle when switching it
==============
*/
static void DestroyMapCycle(mapcycle_t* cycle)
{
	mapcycle_item_t *p, *n, *start;
	p = cycle->items;
	if (p)
	{
		start = p;
		p = p->next;
		while (p != start)
		{
			n = p->next;
			delete p;
			p = n;
		}

		delete cycle->items;
	}
	cycle->items = nullptr;
	cycle->next_item = nullptr;
}

static char com_token[1500];

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
static char* COM_Parse(char* data)
{
	int c;
	int len;

	len = 0;
	com_token[0] = 0;

	if (!data)
		return nullptr;

// skip whitespace
skipwhite:
	while ((c = *data) <= ' ')
	{
		if (c == 0)
			return nullptr; // end of file;
		data++;
	}

	// skip // comments
	if (c == '/' && data[1] == '/')
	{
		while ('\0' != *data && *data != '\n')
			data++;
		goto skipwhite;
	}


	// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (true)
		{
			c = *data++;
			if (c == '\"' || '\0' == c)
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
	}

	// parse single characters
	if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ',')
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data + 1;
	}

	// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
		if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ',')
			break;
	} while (c > 32);

	com_token[len] = 0;
	return data;
}

/*
==============
COM_TokenWaiting

Returns 1 if additional data is waiting to be processed on this line
==============
*/
static bool COM_TokenWaiting(char* buffer)
{
	char* p;

	p = buffer;
	while ('\0' != *p && *p != '\n')
	{
		if (0 == isspace(*p) || 0 != isalnum(*p))
			return true;

		p++;
	}

	return false;
}



/*
==============
ReloadMapCycleFile


Parses mapcycle.txt file into mapcycle_t structure
==============
*/
static bool ReloadMapCycleFile(char* filename, mapcycle_t* cycle)
{
	char szBuffer[MAX_RULE_BUFFER];
	char szMap[32];
	int length;
	char* pFileList;
	char* aFileList = pFileList = (char*)engine::LoadFileForMe(filename, &length);
	bool hasbuffer;
	mapcycle_item_s *item, *newlist = nullptr, *next;

	if (pFileList && 0 != length)
	{
		// the first map name in the file becomes the default
		while (true)
		{
			hasbuffer = false;
			memset(szBuffer, 0, MAX_RULE_BUFFER);

			pFileList = COM_Parse(pFileList);
			if (strlen(com_token) <= 0)
				break;

			strcpy(szMap, com_token);

			// Any more tokens on this line?
			if (COM_TokenWaiting(pFileList))
			{
				pFileList = COM_Parse(pFileList);
				if (strlen(com_token) > 0)
				{
					hasbuffer = true;
					strcpy(szBuffer, com_token);
				}
			}

			// Check map
			if (engine::IsMapValid(szMap))
			{
				// Create entry
				char* s;

				item = new mapcycle_item_s;

				strcpy(item->mapname, szMap);

				item->minplayers = 0;
				item->maxplayers = 0;

				memset(item->rulebuffer, 0, MAX_RULE_BUFFER);

				if (hasbuffer)
				{
					s = engine::InfoKeyValue(szBuffer, "minplayers");
					if (s && '\0' != s[0])
					{
						item->minplayers = atoi(s);
						item->minplayers = std::max(item->minplayers, 0);
						item->minplayers = std::min(item->minplayers, gpGlobals->maxClients);
					}
					s = engine::InfoKeyValue(szBuffer, "maxplayers");
					if (s && '\0' != s[0])
					{
						item->maxplayers = atoi(s);
						item->maxplayers = std::max(item->maxplayers, 0);
						item->maxplayers = std::min(item->maxplayers, gpGlobals->maxClients);
					}

					// Remove keys
					//
					engine::Info_RemoveKey(szBuffer, "minplayers");
					engine::Info_RemoveKey(szBuffer, "maxplayers");

					strcpy(item->rulebuffer, szBuffer);
				}

				item->next = cycle->items;
				cycle->items = item;
			}
			else
			{
				engine::AlertMessage(at_console, "Skipping %s from mapcycle, not a valid map\n", szMap);
			}
		}

		engine::FreeFile(aFileList);
	}

	// Fixup circular list pointer
	item = cycle->items;

	// Reverse it to get original order
	while (item)
	{
		next = item->next;
		item->next = newlist;
		newlist = item;
		item = next;
	}
	cycle->items = newlist;
	item = cycle->items;

	// Didn't parse anything
	if (!item)
	{
		return false;
	}

	while (item->next)
	{
		item = item->next;
	}
	item->next = cycle->items;

	cycle->next_item = item->next;

	return true;
}

/*
==============
CountPlayers

Determine the current # of active players on the server for map cycling logic
==============
*/
int CountPlayers()
{
	int num = 0;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pEnt = util::PlayerByIndex(i);

		if (pEnt && !pEnt->IsBot())
		{
			num = num + 1;
		}
	}

	return num;
}

/*
==============
ExtractCommandString

Parse commands/key value pairs to issue right after map xxx command is issued on server
 level transition
==============
*/
static void ExtractCommandString(char* s, char* szCommand)
{
	// Now make rules happen
	char pkey[512];
	char value[512]; // use two buffers so compares
					 // work without stomping on each other
	char* o;

	if (*s == '\\')
		s++;

	while (true)
	{
		o = pkey;
		while (*s != '\\')
		{
			if ('\0' == *s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;

		while (*s != '\\' && '\0' != *s)
		{
			if ('\0' == *s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		strcat(szCommand, pkey);
		if (strlen(value) > 0)
		{
			strcat(szCommand, " ");
			strcat(szCommand, value);
		}
		strcat(szCommand, "\n");

		if ('\0' == *s)
			return;
		s++;
	}
}

static bool CheckMapCycle()
{
	char* mapcfile = (char*)engine::CVarGetString("mapcyclefile");

	// Has the map cycle filename changed?
	if (stricmp(mapcfile, szPreviousMapCycleFile))
	{
		strcpy(szPreviousMapCycleFile, mapcfile);

		DestroyMapCycle(&mapcycle);

		if (!ReloadMapCycleFile(mapcfile, &mapcycle) || (!mapcycle.items))
		{
			engine::AlertMessage(at_console, "Unable to load map cycle file %s\n", mapcfile);
			return false;
		}
	}

	return mapcycle.items != nullptr;
}

/*
==============
ChangeLevel

Server is changing to a new level, check mapcycle.txt for map name and setup info
==============
*/
void CHalfLifeMultiplay::ChangeLevel()
{
	char szNextMap[32];
	char szFirstMapInList[32];
	char szCommands[1500];
	char szRules[1500];
	int minplayers = 0, maxplayers = 0;
	strcpy(szFirstMapInList, "crossfire"); // the absolute default level is purgatory

	if (nextlevel.length() != 0 && engine::IsMapValid(nextlevel.c_str()) != 0)
	{
		EnterState(GR_STATE_GAME_OVER);

		engine::AlertMessage(at_console, "CHANGE LEVEL: %s\n", nextlevel.c_str());

		engine::ChangeLevel(nextlevel.c_str(), nullptr);
		nextlevel.clear();
		return;
	}

	int curplayers;

	// find the map to change to

	szCommands[0] = '\0';
	szRules[0] = '\0';

	curplayers = CountPlayers();

	if (CheckMapCycle())
	{
		bool keeplooking = false;
		bool found = false;
		mapcycle_item_s* item;

		// Assume current map
		strcpy(szNextMap, STRING(gpGlobals->mapname));
		strcpy(szFirstMapInList, STRING(gpGlobals->mapname));

		// Traverse list
		for (item = mapcycle.next_item; item->next != mapcycle.next_item; item = item->next)
		{
			keeplooking = false;

			if (item->minplayers != 0)
			{
				if (curplayers >= item->minplayers)
				{
					found = true;
					minplayers = item->minplayers;
				}
				else
				{
					keeplooking = true;
				}
			}

			if (item->maxplayers != 0)
			{
				if (curplayers <= item->maxplayers)
				{
					found = true;
					maxplayers = item->maxplayers;
				}
				else
				{
					keeplooking = true;
				}
			}

			if (keeplooking)
				continue;

			found = true;
			break;
		}

		if (!found)
		{
			item = mapcycle.next_item;
		}

		// Increment next item pointer
		mapcycle.next_item = item->next;

		// Perform logic on current item
		strcpy(szNextMap, item->mapname);

		ExtractCommandString(item->rulebuffer, szCommands);
		strcpy(szRules, item->rulebuffer);
	}

	if (!engine::IsMapValid(szNextMap))
	{
		strcpy(szNextMap, szFirstMapInList);
	}

	EnterState(GR_STATE_GAME_OVER);

	engine::AlertMessage(at_console, "CHANGE LEVEL: %s\n", szNextMap);
	if (0 != minplayers || 0 != maxplayers)
	{
		engine::AlertMessage(at_console, "PLAYER COUNT:  min %i max %i current %i\n", minplayers, maxplayers, curplayers);
	}
	if (strlen(szRules) > 0)
	{
		engine::AlertMessage(at_console, "RULES:  %s\n", szRules);
	}

	engine::ChangeLevel(szNextMap, nullptr);
	if (strlen(szCommands) > 0)
	{
		engine::ServerCommand(szCommands);
	}
}


CLevelPoll::CLevelPoll(const std::vector<CVoteManager::LevelNominee>& nominees)
{
	m_LevelNames.clear();

    for (auto n = nominees.begin(); m_LevelNames.size() < 4 && n != nominees.end(); n++)
    {
        m_LevelNames.push_back((*n).name);
    }

	if (CheckMapCycle())
	{
		for (auto i = mapcycle.next_item; m_LevelNames.size() < 4 && i->next != mapcycle.next_item; i = i->next)
		{
			/* Make sure it wasn't already nominated */
			if (std::find_if(m_LevelNames.begin(), m_LevelNames.end(), [=](const auto& candidate)
				{ return stricmp(candidate.c_str(), i->mapname) == 0; }) == m_LevelNames.end())
			{
				m_LevelNames.push_back(i->mapname);
			}

			/* New pool of maps for the next vote or level change */
			mapcycle.next_item = i->next;
		}
	}

	util::ClientPrintAll(HUD_PRINTTALK, "#Vote_level_begin");
	Begin("#Vote_level_title", m_LevelNames);
}


void CLevelPoll::DoResults(const int winner, const std::vector<int>& tally)
{
	nextlevel = m_LevelNames[winner];
	util::ClientPrintAll(HUD_PRINTTALK, "#Vote_level_end", nextlevel.c_str());
}

