#pragma once
#include "pch.h"
#include <fstream>
#include <sstream>

enum BGMSFX
{
	BGM_MAINMENU,
	BGM_BOSS,
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
	GENERAL_TRUMPET
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

		float BGMVolume = 1.0f;
		float GeneralSFXVolume = 1.0f;
		float CombatSFXVolume = 1.0f;
		float PlayerSFXVolume = 1.0f;
		float EnemySFXVolume = 1.0f;

		struct AudioGroups
		{
			AEAudioGroup combat;
			AEAudioGroup player;
			AEAudioGroup enemy;
			AEAudioGroup fireworks;
			AEAudioGroup general;
			AEAudioGroup BGM;
		};

		AudioGroups audioGroup;

		std::vector<AEAudio> BGM;
		std::vector<AEAudio> generalSFX;
		std::vector<AEAudio> fireworksSFX;
		// Combat Sounds
		std::vector<AEAudio> hitSFX, swingSFX, parrySFX;

		// Enemy Sounds
		std::vector<AEAudio> enemyVocal_SFX, enemyHurt_SFX, enemyLaugh_SFX;
		std::vector<AEAudio> playerDeath_SFX, playerHurt_SFX;

		bool muted = false;
};

extern Audio gAudio;