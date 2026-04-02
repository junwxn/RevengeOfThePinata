/*************************************************************************
@file		Audio.cpp
@Author		Chew Zheng Hui, Timothy Caleb z.chew@digipen.edu
@Co-authors	nil
@brief		This file contains the implementation of the audio system
			structures and functions, including the global audio state,
			a function to load audio files dynamically from a .txt file
			and a function to retrieve and play an audio based on its group

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/

#include "pch.h"
#include "Audio.h"
//#include <fstream>
//#include <sstream>

// global audio
Audio gAudio;

void Audio::Audio_Load(std::string const& filename)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		//std::cout << "FILE COULD NOT BE FOUND/OPENED." << std::endl;
		return;
	}
		

	std::string line;
	std::string folder, subFolder, category, audioName;
	while (std::getline(file, line))
	{
		//std::cout << line << std::endl;
		if (line.empty()) continue;

		std::istringstream iss(line);
		std::string filePath{ "Assets/Audio/" };

		iss >> folder >> subFolder >> category >> audioName;
		if (folder == "BGM")
		{
			filePath += folder + "/" + audioName;
			//std::cout << filePath << std::endl;
			m_BGM.push_back(AEAudioLoadMusic(filePath.c_str()));
		}
		if (folder == "Combat")
		{
			filePath += folder + "/" + subFolder + "/" + audioName;
			//std::cout << filePath << std::endl;
			if (category == "hit") m_hitSFX.push_back(AEAudioLoadSound(filePath.c_str()));
			if (category == "parry") m_parrySFX.push_back(AEAudioLoadSound(filePath.c_str()));
			if (category == "swing") m_swingSFX.push_back(AEAudioLoadSound(filePath.c_str()));
		}
		if (folder == "Enemy")
		{
			filePath += folder + "/" + audioName;
			//std::cout << filePath << std::endl;
			
			if (subFolder == "vocal") m_enemyVocal_SFX.push_back(AEAudioLoadSound(filePath.c_str()));
			if (subFolder == "hurt") m_enemyHurt_SFX.push_back(AEAudioLoadSound(filePath.c_str()));
			if (subFolder == "laugh") m_enemyLaugh_SFX.push_back(AEAudioLoadSound(filePath.c_str()));
		}
		if (folder == "Fireworks")
		{
			filePath += folder + "/" + audioName;
			//std::cout << filePath << std::endl;
			m_fireworksSFX.push_back(AEAudioLoadSound(filePath.c_str()));
		}
		if (folder == "General")
		{
			filePath += folder + "/" + audioName;
			//std::cout << filePath << std::endl;
			m_generalSFX.push_back(AEAudioLoadSound(filePath.c_str()));
		}
		if (folder == "Player")
		{
			filePath += folder + "/" + audioName;
			//std::cout << filePath << std::endl;
			
			if (subFolder == "death") m_playerDeath_SFX.push_back(AEAudioLoadSound(filePath.c_str()));
			if (subFolder == "hurt") m_playerHurt_SFX.push_back(AEAudioLoadSound(filePath.c_str()));
		}
	}
}

void Audio::Audio_Init()
{
	Audio_Load("Assets/Audio/config.txt");
	gAudio.m_audioGroup.combat = AEAudioCreateGroup();
	gAudio.m_audioGroup.player = AEAudioCreateGroup();
	gAudio.m_audioGroup.enemy = AEAudioCreateGroup();
	gAudio.m_audioGroup.fireworks = AEAudioCreateGroup();
	gAudio.m_audioGroup.general = AEAudioCreateGroup();
	gAudio.m_audioGroup.BGM = AEAudioCreateGroup();
}

void Audio::PlayBGM(BGMSFX bgmSFX)
{
	AEAudioPlay(m_BGM[bgmSFX], m_audioGroup.BGM, 1.0f, 1.0f, -1); // loop = true
}


void Audio::PlayCombatSFX(CombatSFX combatSound)
{
	if (combatSound == COMBAT_HIT)
	{
		int soundID{ Vectors::get_random(0, static_cast<int>(m_hitSFX.size())-1) };
		AEAudioPlay(m_hitSFX[soundID], m_audioGroup.combat, 1.0f, 1.0f, 0);
	}
	if (combatSound == COMBAT_SWING)
	{
		int soundID{ Vectors::get_random(0, static_cast<int>(m_swingSFX.size())-1) };
		AEAudioPlay(m_swingSFX[soundID], m_audioGroup.combat, 1.0f, 1.0f, 0);
	}
	if (combatSound == COMBAT_PARRY)
	{
		int soundID{ Vectors::get_random(0, static_cast<int>(m_parrySFX.size())-1) };
		AEAudioPlay(m_parrySFX[soundID], m_audioGroup.combat, 1.0f, 1.0f, 0);
	}
}

void Audio::PlayEnemySFX(EnemySFX enemySound)
{
	if (enemySound == ENEMY_HURT)
	{
		int soundID{ Vectors::get_random(0, static_cast<int>(m_enemyVocal_SFX.size()) - 1)};
		AEAudioPlay(m_enemyVocal_SFX[soundID], m_audioGroup.enemy, 1.0f, 1.0f, 0);
	}
	if (enemySound == ENEMY_VOCAL)
	{
		int soundID{ Vectors::get_random(0, static_cast<int>(m_enemyHurt_SFX.size()) - 1) };
		AEAudioPlay(m_enemyHurt_SFX[soundID], m_audioGroup.enemy, 1.0f, 1.0f, 0);
	}
	if (enemySound == ENEMY_LAUGH)
	{
		int soundID{ Vectors::get_random(0, static_cast<int>(m_enemyLaugh_SFX.size()) - 1) };
		AEAudioPlay(m_enemyLaugh_SFX[soundID], m_audioGroup.enemy, 1.0f, 1.0f, 0);
	}
}

void Audio::PlayPlayerSFX(PlayerSFX playerSound)
{
	if (playerSound == PLAYER_DEATH)
	{
		int soundID{ Vectors::get_random(0, static_cast<int>(m_playerDeath_SFX.size()) - 1)};
		AEAudioPlay(m_playerDeath_SFX[soundID], m_audioGroup.player, 1.0f, 1.0f, 0);
	}
	if (playerSound == PLAYER_HURT)
	{
		int soundID{ Vectors::get_random(0, static_cast<int>(m_playerHurt_SFX.size()) - 1)};
		AEAudioPlay(m_playerHurt_SFX[soundID], m_audioGroup.player, 1.0f, 1.0f, 0);
	}
}

void Audio::PlayGeneralSFX(GeneralSFX generalSound)
{
	if (generalSound == GENERAL_ANNOUNCEMENT)
	{
		AEAudioPlay(m_generalSFX[GENERAL_ANNOUNCEMENT], m_audioGroup.general, 1.0f, 1.0f, 0);
	}
	if (generalSound == GENERAL_GAMEOVER)
	{
		AEAudioPlay(m_generalSFX[GENERAL_GAMEOVER], m_audioGroup.general, 1.0f, 1.0f, 0);
	}
	if (generalSound == GENERAL_TRUMPET)
	{
		AEAudioPlay(m_generalSFX[GENERAL_TRUMPET], m_audioGroup.general, 1.0f, 1.0f, 0);
	}
	if (generalSound == GENERAL_AUGMENT)
	{
		AEAudioPlay(m_generalSFX[GENERAL_AUGMENT], m_audioGroup.general, 4.0f, 1.0f, 0);
	}
	if (generalSound == GENERAL_BOSS_DASH)
	{
		AEAudioPlay(m_generalSFX[GENERAL_BOSS_DASH], m_audioGroup.general, 2.0f, 1.0f, 0);
	}
	if (generalSound == GENERAL_BOSS_PHASE_CHANGE)
	{
		AEAudioPlay(m_generalSFX[GENERAL_BOSS_PHASE_CHANGE], m_audioGroup.general, 4.0f, 1.0f, 0);
	}
	if (generalSound == GENERAL_BOSS_PHASE3_GUN)
	{
		AEAudioPlay(m_generalSFX[GENERAL_BOSS_PHASE3_GUN], m_audioGroup.general, 4.0f, 1.0f, 0);
	}
	if (generalSound == GENERAL_GAMEVICTORY)
	{
		AEAudioPlay(m_generalSFX[GENERAL_GAMEVICTORY], m_audioGroup.general, 4.0f, 1.0f, 0);
	}
}

void Audio::PlayFireworksSFX()
{
		int soundID{ Vectors::get_random(0, static_cast<int>(m_fireworksSFX.size()) - 1) };
		AEAudioPlay(m_fireworksSFX[soundID], m_audioGroup.fireworks, 1.0f, 1.0f, 0);
}

void Audio::PlayClickSFX()
{
		int soundID{ Vectors::get_random(0, 7) };
		AEAudioPlay(m_enemyVocal_SFX[soundID], m_audioGroup.enemy, 1.0f, 1.0f, 0);
}

void Audio::SetBGMVolume(float v) {
	m_BGMVolume = v;

	// If muted, keep volume at 0
	if (IsMuted()) {
		AEAudioSetGroupVolume(m_audioGroup.BGM, 0.0f);
	}
	else {
		AEAudioSetGroupVolume(m_audioGroup.BGM, m_BGMVolume);
	}
}

void Audio::SetGeneralSFXVolume(float v) {
	m_GeneralSFXVolume = v;

	// If muted, keep volume at 0
	if (IsMuted()) {
		AEAudioSetGroupVolume(m_audioGroup.fireworks, 0.0f);
		AEAudioSetGroupVolume(m_audioGroup.general, 0.0f);
	}
	else {
		AEAudioSetGroupVolume(m_audioGroup.fireworks, m_GeneralSFXVolume);
		AEAudioSetGroupVolume(m_audioGroup.general, m_GeneralSFXVolume);
	}
}

void Audio::SetCombatSFXVolume(float v) {
	m_CombatSFXVolume = v;

	// If muted, keep volume at 0
	if (IsMuted()) {
		AEAudioSetGroupVolume(m_audioGroup.combat, 0.0f);
	}
	else {
		AEAudioSetGroupVolume(m_audioGroup.combat, m_CombatSFXVolume);
	}
}

void Audio::SetPlayerSFXVolume(float v) {
	m_PlayerSFXVolume = v;

	// If muted, keep volume at 0
	if (IsMuted()) {
		AEAudioSetGroupVolume(m_audioGroup.player, 0.0f);
	}
	else {
		AEAudioSetGroupVolume(m_audioGroup.player, m_PlayerSFXVolume);
	}
}

void Audio::SetEnemySFXVolume(float v) {
	m_EnemySFXVolume = v;

	// If muted, keep volume at 0
	if (IsMuted()) {
		AEAudioSetGroupVolume(m_audioGroup.enemy, 0.0f);
	}
	else {
		AEAudioSetGroupVolume(m_audioGroup.enemy, m_EnemySFXVolume);
	}
}

void Audio::UnloadBGM()
{
	AEAudioStopGroup(m_audioGroup.BGM);

	for (int i{}; i < m_BGM.size(); ++i) AEAudioUnloadAudio(m_BGM[i]);

	m_BGM.clear();
	AEAudioUnloadAudioGroup(m_audioGroup.BGM);
}
void Audio::UnloadCombatSFX()
{
	AEAudioStopGroup(m_audioGroup.combat);

	for (int i{}; i < m_hitSFX.size(); ++i) AEAudioUnloadAudio(m_hitSFX[i]);
 	for (int i{}; i < m_swingSFX.size(); ++i) AEAudioUnloadAudio(m_swingSFX[i]);
	for (int i{}; i < m_parrySFX.size(); ++i) AEAudioUnloadAudio(m_parrySFX[i]);

	m_hitSFX.clear();
	m_swingSFX.clear();
	m_parrySFX.clear();
	AEAudioUnloadAudioGroup(m_audioGroup.combat);
}
void Audio::UnloadEnemySFX()
{
	AEAudioStopGroup(m_audioGroup.enemy);

	for (int i{}; i < m_enemyVocal_SFX.size(); ++i) AEAudioUnloadAudio(m_enemyVocal_SFX[i]);
	for (int i{}; i < m_enemyHurt_SFX.size(); ++i) AEAudioUnloadAudio(m_enemyHurt_SFX[i]);
	for (int i{}; i < m_enemyLaugh_SFX.size(); ++i) AEAudioUnloadAudio(m_enemyLaugh_SFX[i]);

	m_enemyVocal_SFX.clear();
	m_enemyHurt_SFX.clear();
	m_enemyLaugh_SFX.clear();
	AEAudioUnloadAudioGroup(m_audioGroup.enemy);
}
void Audio::UnloadPlayerSFX()
{
	AEAudioStopGroup(m_audioGroup.player);

	for (int i{}; i < m_playerDeath_SFX.size(); ++i) AEAudioUnloadAudio(m_playerDeath_SFX[i]);
	for (int i{}; i < m_playerHurt_SFX.size(); ++i) AEAudioUnloadAudio(m_playerHurt_SFX[i]);

	m_playerDeath_SFX.clear();
	m_playerHurt_SFX.clear();
	AEAudioUnloadAudioGroup(m_audioGroup.player);
}
void Audio::UnloadGeneralSFX()
{
	AEAudioStopGroup(m_audioGroup.general);

	for (int i{}; i < m_generalSFX.size(); ++i) AEAudioUnloadAudio(m_generalSFX[i]);

	m_generalSFX.clear();
	AEAudioUnloadAudioGroup(m_audioGroup.general);
}
void Audio::ToggleMute()
{
	m_muted = !m_muted;
	float vol = m_muted ? 0.0f : 1.0f;
	AEAudioSetGroupVolume(m_audioGroup.combat, vol);
	AEAudioSetGroupVolume(m_audioGroup.player, vol);
	AEAudioSetGroupVolume(m_audioGroup.enemy, vol);
	AEAudioSetGroupVolume(m_audioGroup.fireworks, vol);
	AEAudioSetGroupVolume(m_audioGroup.general, vol);
	AEAudioSetGroupVolume(m_audioGroup.BGM, vol);
}

bool Audio::IsMuted() const
{
	return m_muted;
}

void Audio::UnloadFireworksSFX()
{
	AEAudioStopGroup(m_audioGroup.fireworks);

	for (int i{}; i < m_fireworksSFX.size(); ++i) AEAudioUnloadAudio(m_fireworksSFX[i]);

	m_fireworksSFX.clear();
	AEAudioUnloadAudioGroup(m_audioGroup.fireworks);
}