#pragma once
#include <AEVec2.h>
#include <random>

namespace Vectors {
	double magnitude(f32 vecX, f32 vecY);
	f32 lerp(f32 start, f32 end, f32 interpolent);
	AEVec2 normalize(f32 magnitude, f32 vecX, f32 vecY);
	int get_random(int min, int max);
}