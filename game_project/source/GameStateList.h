/*************************************************************************
@file		GameStateList.h
@Author		Chiu Jun Wen j.chiu@digipen.edu
@Co-authors	nil
@brief		This file contains the enumeration definitions for various game
			states, including their initialization, updating, rendering,
			and transitioning between states.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/

#pragma once
enum GS_STATES
{
	GS_MAINMENU = 0,
	GS_TUTORIAL,
	GS_LEVEL1,
	GS_LEVEL2,
	GS_LEVEL3,
	GS_BOSSLEVEL,
	GS_VICTORY,
	GS_GAMEOVER,
	GS_TEST,
	GS_QUIT,
	GS_RESTART,
	GS_NONE,
	GS_SPLASH
};