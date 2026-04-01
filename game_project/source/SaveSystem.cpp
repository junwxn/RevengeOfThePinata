/*************************************************************************
@file		SaveSystem.cpp
@Author		Chiu Jun Wen j.chiu@digipen.edu
@brief		This file contains the function definitions for managing game
            save data, including saving, loading, and clearing save files.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/

#include "pch.h"
#include "SaveSystem.h"
#include "Player.h"
#include "AugmentData.h"
#include "GameStateList.h"
#include <fstream>
#include <string>
#include <cstdio>

static const char* SAVE_FILE_PATH = "savegame.json";
static const int   SAVE_VERSION   = 1;

// Helper: find an integer value for a given key in a JSON string
static int FindIntValue(const std::string& json, const char* key, int defaultVal)
{
    std::string search = std::string("\"") + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return defaultVal;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return defaultVal;
    try {
        return std::stoi(json.substr(pos + 1));
    }
    catch (...) {
        return defaultVal;
    }
}

bool SaveSystem_HasSave()
{
    std::ifstream file(SAVE_FILE_PATH);
    return file.good();
}

void SaveSystem_Save(int levelState)
{
    std::ofstream file(SAVE_FILE_PATH);
    if (!file.is_open()) return;

    file << "{\n";
    file << "  \"version\": " << SAVE_VERSION << ",\n";
    file << "  \"attackCharges\": " << g_PlayerAttackCharges << ",\n";
    file << "  \"augment0\": " << static_cast<int>(g_Augments.chosen[0]) << ",\n";
    file << "  \"augment1\": " << static_cast<int>(g_Augments.chosen[1]) << ",\n";
    file << "  \"augment2\": " << static_cast<int>(g_Augments.chosen[2]) << ",\n";
    file << "  \"currentLevel\": " << levelState << "\n";
    file << "}\n";
    file.close();
}

int SaveSystem_Load()
{
    std::ifstream file(SAVE_FILE_PATH);
    if (!file.is_open()) return -1;

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    int version = FindIntValue(content, "version", 0);
    if (version != SAVE_VERSION) return -1;

    g_PlayerAttackCharges       = FindIntValue(content, "attackCharges", DEFAULT_ATTACK_CHARGES);
    g_Augments.chosen[0]        = static_cast<AugmentID>(FindIntValue(content, "augment0", 0));
    g_Augments.chosen[1]        = static_cast<AugmentID>(FindIntValue(content, "augment1", 0));
    g_Augments.chosen[2]        = static_cast<AugmentID>(FindIntValue(content, "augment2", 0));
    int savedLevel              = FindIntValue(content, "currentLevel", -1);

    return savedLevel;
}

void SaveSystem_ClearSave()
{
    std::remove(SAVE_FILE_PATH);
}
