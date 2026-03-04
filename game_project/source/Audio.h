#pragma once
#include "pch.h"

class Audio
{
	public:
		void Audio_Load(std::string const&);
		void Audio_Init();
		void PlayBGM(int id);
		void StopBGM(int id);

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
		std::vector<AEAudio> hitSFX, swingSFX, parrySFX;

		std::vector<AEAudio> enemySFX, playerSFX, hurtSFX;
};

extern Audio gAudio;