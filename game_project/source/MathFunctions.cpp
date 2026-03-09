#include "pch.h"
#include "MathFunctions.h"

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

	int get_random(int min, int max) {
		// These are initialized only ONCE the first time the function is called
		static std::random_device rd;
		static std::mt19937 gen(rd());

		// The distribution can be local or static
		std::uniform_int_distribution<> distr(min, max);

		return distr(gen); // The engine 'gen' remembers its state for the next call
	}
}
