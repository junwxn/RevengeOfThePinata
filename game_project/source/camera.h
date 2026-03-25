#pragma once
#include "AEEngine.h"

struct ScreenShake
{
	bool on{};
	f32 timer{};
	f32 duration{};
	f32 magnitude{};
};

class Camera
{
public:
	void Init(f32 startX, f32 startY);
	void Update(f32 dt, f32 playerX, f32 playerY, bool preventing_movement);

	f32 GetRenderX() const { return m_X + m_shakeOffsetX; }
	f32 GetRenderY() const { return m_Y + m_shakeOffsetY; }
	float GetX() const { return m_X; }
	float GetY() const { return m_Y; }

	void SetScreenShakeTimer(f32 set) { m_ScreenShake.timer = set; m_ScreenShake.duration = set; }

private:
	//position
	f32 m_X;
	f32 m_Y;
	f32 m_shakeOffsetX{};
	f32 m_shakeOffsetY{};

	//settings
	f32 m_Speed;
	f32 m_LookDist;

	ScreenShake m_ScreenShake{ false, 0.0, 0.0, 5.0 };
	//std::mt19937 m_rng{ std::random_device{}() };
};
