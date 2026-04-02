/*************************************************************************
@file		Splash.h
@Author		Chew Zheng Hui, Timothy Caleb z.chew@digipen.edu
@Co-authors	nil
@brief		This file contains the definitions for Digipen logo
			splash data, including their initializer, loader, 
			draw, free, unload and update functions

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/

#pragma once
#ifndef SPLASH_H
#define SPLASH_H

void Splash_Load();
void Splash_Init();
void Splash_Update(float dt);
void Splash_Draw();
void Splash_Free();
void Splash_Unload();

#endif