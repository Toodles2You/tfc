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
// cl_util.h
//

#pragma once

#include "cvardef.h"

#include "Platform.h"

// Macros to hook function calls into the HUD object
#define HOOK_MESSAGE(x) client::HookUserMsg(#x, __MsgFunc_##x);

#define DECLARE_MESSAGE(y, x)                                     \
	int __MsgFunc_##x(const char* pszName, int iSize, void* pbuf) \
	{                                                             \
		return gHUD.y.MsgFunc_##x(pszName, iSize, pbuf);          \
	}


#define HOOK_COMMAND(x, y) client::AddCommand(x, __CmdFunc_##y);
#define DECLARE_COMMAND(y, x) \
	void __CmdFunc_##x()      \
	{                         \
		gHUD.y.UserCmd_##x(); \
	}

// ScreenHeight returns the height of the screen, in pixels
#define ScreenHeight (gHUD.m_scrinfo.iHeight)
// ScreenWidth returns the width of the screen, in pixels
#define ScreenWidth (gHUD.m_scrinfo.iWidth)

#define BASE_XRES 640.f

// use this to project world coordinates to screen coordinates
#define XPROJECT(x) ((1.0f + (x)) * ScreenWidth * 0.5f)
#define YPROJECT(y) ((1.0f - (y)) * ScreenHeight * 0.5f)

#define XRES(x) ((x) * ((float)ScreenWidth / 640))
#define YRES(y) ((y) * ((float)ScreenHeight / 480))

inline client_textmessage_t* TextMessageGet(const char* pName) { return client::TextMessageGet(pName); }
inline int TextMessageDrawChar(int x, int y, int number, int r, int g, int b)
{
	return client::DrawCharacter(x, y, number, r, g, b);
}

inline int DrawConsoleString(int x, int y, const char* string)
{
	return client::DrawConsoleString(x, y, (char*)string);
}

inline void GetConsoleStringSize(const char* string, int* width, int* height)
{
	client::DrawConsoleStringLen(string, width, height);
}

inline int ConsoleStringLen(const char* string)
{
	int _width, _height;
	GetConsoleStringSize(string, &_width, &_height);
	return _width;
}

inline void ConsolePrint(const char* string)
{
	client::ConsolePrint(string);
}


inline char* safe_strcpy(char* dst, const char* src, int len_dst)
{
	if (len_dst <= 0)
	{
		return nullptr; // this is bad
	}

	strncpy(dst, src, len_dst);
	dst[len_dst - 1] = '\0';

	return dst;
}

inline int safe_sprintf(char* dst, int len_dst, const char* format, ...)
{
	if (len_dst <= 0)
	{
		return -1; // this is bad
	}

	va_list v;

	va_start(v, format);

	vsnprintf(dst, len_dst, format, v);

	va_end(v);

	dst[len_dst - 1] = '\0';

	return 0;
}

void ScaleColors(int& r, int& g, int& b, int a);

// disable 'possible loss of data converting float to int' warning message
#pragma warning(disable : 4244)
// disable 'truncation from 'const double' to 'float' warning message
#pragma warning(disable : 4305)

inline void UnpackRGB(int& r, int& g, int& b, unsigned long ulRGB)
{
	r = (ulRGB & 0xFF0000) >> 16;
	g = (ulRGB & 0xFF00) >> 8;
	b = ulRGB & 0xFF;
}

HSPRITE LoadSprite(const char* pszName);
