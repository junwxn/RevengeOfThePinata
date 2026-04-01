/*************************************************************************
@file		SaveSystem.h
@Author		Chiu Jun Wen j.chiu@digipen.edu
@Co-authors	nil
@brief		This file contains the function declarations for the save system,
            including checking for save files, saving, loading, and clearing progress.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/
#pragma once

// Returns true if a save file exists.
bool SaveSystem_HasSave();

// Saves current game progress to disk.
// levelState = the GS_STATES level the player should resume from.
void SaveSystem_Save(int levelState);

// Loads saved game progress from disk.
// Restores g_PlayerAttackCharges and g_Augments.
// Returns the saved GS_STATES value, or -1 if no valid save exists.
int SaveSystem_Load();

// Deletes the save file.
void SaveSystem_ClearSave();
