/*************************************************************************
@file		MathFunctions.h
@Author		Chew Zheng Hui, Timothy Caleb z.chew@digipen.edu
@Co-authors	nil
@brief		This file contains the declarations for the MathFunctions data

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/

#pragma once
#include <AEVec2.h>
#include <random>

namespace Vectors {
	double magnitude(f32 vecX, f32 vecY);
	f32 lerp(f32 start, f32 end, f32 interpolent);
	AEVec2 normalize(f32 magnitude, f32 vecX, f32 vecY);
	int get_random(int min, int max);
}