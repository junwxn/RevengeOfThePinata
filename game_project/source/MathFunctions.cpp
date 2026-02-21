#include "MathFunctions.h"
#include <AEEngine.h>

namespace Vectors {
	double magnitude(f32 vecX, f32 vecY) {
		return sqrt(vecX * vecX + vecY * vecY);
	}

	f32 lerp(f32 start, f32 end, f32 interpolent) {
		return start + (end - start) * interpolent;
	}

	AEVec2 normalize(f32 magnitude, f32 vecX, f32 vecY)
	{
		AEVec2 normalized{ (1 / magnitude) * vecX, (1 / magnitude) * vecY };
		return normalized;
	}
}
