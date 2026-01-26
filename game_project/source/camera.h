#pragma once
#include "AEEngine.h"

class Camera
{
public:
	void Init(f32 startX, f32 startY);
	void Update(f32 dt, f32 playerX, f32 playerY);

	float GetX() const { return m_X; }
	float GetY() const { return m_Y; }

private:
	//position
	f32 m_X;
	f32 m_Y;

	//settings
	f32 m_Speed;
	f32 m_LookDist;
};
