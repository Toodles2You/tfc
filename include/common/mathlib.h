/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
// mathlib.h

#pragma once

#include <cmath>

typedef float vec_t;

#include "vector.h"

typedef vec_t vec4_t[4]; // x,y,z,w
typedef vec_t vec5_t[5];

typedef short vec_s_t;
typedef vec_s_t vec3s_t[3];
typedef vec_s_t vec4s_t[4]; // x,y,z,w
typedef vec_s_t vec5s_t[5];

typedef int fixed4_t;
typedef int fixed8_t;
typedef int fixed16_t;
#ifndef M_PI
#define M_PI 3.14159265358979323846 // matches value in gcc v2 math.h
#endif

struct mplane_s;

constexpr Vector g_vecZero(0, 0, 0);

float Distance(const Vector& v1, const Vector& v2);

void AngleVectors(const Vector& angles, Vector* forward, Vector* right, Vector* up);
void AngleVectorsTranspose(const Vector& angles, Vector* forward, Vector* right, Vector* up);

void AngleMatrix(const Vector& angles, float (*matrix)[4]);
void VectorTransform(const Vector& in1, float in2[3][4], Vector& out);
void VectorIRotate(const Vector& in1, const float in2[3][4], Vector& out);

void NormalizeAngles(Vector& angles);

void VectorAngles(const Vector& forward, Vector& angles);

float anglemod(float a);

float SplineFraction(float value, float scale = 1.0F);
