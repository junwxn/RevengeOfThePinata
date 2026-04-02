/*************************************************************************
@file		Audio.h
@Author		Chew Zheng Hui, Timothy Caleb z.chew@digipen.edu
@Co-authors	nil
@brief		This file contains the definitions for audio data,
			including their IDs, sets, play functions, and vectors
			storing each group of audio type.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/

#pragma once
#include "pch.h"
#include <fstream>
#include <sstream>

enum BGMSFX
{
	BGM_BOSS,
	BGM_MAINMENU,
	BGM_WAVE
};

enum CombatSFX
{
	COMBAT_HIT,
	COMBAT_SWING,
	COMBAT_PARRY
};

enum EnemySFX
{
	ENEMY_VOCAL,
	ENEMY_HURT,
	ENEMY_LAUGH
};

enum PlayerSFX
{
	PLAYER_DEATH,
	PLAYER_HURT,
};

enum GeneralSFX
{
	GENERAL_ANNOUNCEMENT,
	GENERAL_GAMEOVER,
	GENERAL_TRUMPET,
	GENERAL_AUGMENT,
	GENERAL_BOSS_DASH,
	GENERAL_BOSS_PHASE_CHANGE,
	GENERAL_BOSS_PHASE3_GUN,
	GENERAL_GAMEVICTORY
};


class Audio
{
	public:
		void Audio_Load(std::string const&);
		void Audio_Init();
		void PlayBGM(BGMSFX bgmSFX);
		void UnloadBGM();

		void PlayCombatSFX(CombatSFX combatSound);
		void UnloadCombatSFX();

		void PlayEnemySFX(EnemySFX enemySound);
		void UnloadEnemySFX();

		void PlayPlayerSFX(PlayerSFX playerSound);
		void UnloadPlayerSFX();

		void PlayGeneralSFX(GeneralSFX generalSound);
		void UnloadGeneralSFX();

		void PlayFireworksSFX();
		void UnloadFireworksSFX();

		void PlayClickSFX();

		void ToggleMute();
		bool IsMuted() const;

		void SetBGMVolume(float v);
		void SetGeneralSFXVolume(float v);
		void SetCombatSFXVolume(float v);
		void SetPlayerSFXVolume(float v);
		void SetEnemySFXVolume(float v);

		float m_BGMVolume = 1.0f;
		float m_GeneralSFXVolume = 1.0f;
		float m_CombatSFXVolume = 1.0f;
		float m_PlayerSFXVolume = 1.0f;
		float m_EnemySFXVolume = 1.0f;

		struct AudioGroups
		{
			AEAudioGroup combat;
			AEAudioGroup player;
			AEAudioGroup enemy;
			AEAudioGroup fireworks;
			AEAudioGroup general;
			AEAudioGroup BGM;
		};

		AudioGroups m_audioGroup;

		std::vector<AEAudio> m_BGM;
		std::vector<AEAudio> m_generalSFX;
		std::vector<AEAudio> m_fireworksSFX;
		// Combat Sounds
		std::vector<AEAudio> m_hitSFX, m_swingSFX, m_parrySFX;

		// Enemy Sounds
		std::vector<AEAudio> m_enemyVocal_SFX, m_enemyHurt_SFX, m_enemyLaugh_SFX;
		std::vector<AEAudio> m_playerDeath_SFX, m_playerHurt_SFX;

		bool m_muted = false;
};

extern Audio gAudio;