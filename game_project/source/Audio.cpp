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
			BGM.push_back(AEAudioLoadMusic(filePath.c_str()));
		}
		if (folder == "Combat")
		{
			filePath += folder + "/" + subFolder + "/" + audioName;
			//std::cout << filePath << std::endl;
			if (category == "hit") hitSFX.push_back(AEAudioLoadSound(filePath.c_str()));
			if (category == "parry") parrySFX.push_back(AEAudioLoadSound(filePath.c_str()));
			if (category == "swing") swingSFX.push_back(AEAudioLoadSound(filePath.c_str()));
		}
		if (folder == "Enemy")
		{
			filePath += folder + "/" + audioName;
			//std::cout << filePath << std::endl;
			
			if (subFolder == "vocal") enemyVocal_SFX.push_back(AEAudioLoadSound(filePath.c_str()));
			if (subFolder == "hurt") enemyHurt_SFX.push_back(AEAudioLoadSound(filePath.c_str()));
			if (subFolder == "laugh") enemyLaugh_SFX.push_back(AEAudioLoadSound(filePath.c_str()));
		}
		if (folder == "Fireworks")
		{
			filePath += folder + "/" + audioName;
			//std::cout << filePath << std::endl;
			fireworksSFX.push_back(AEAudioLoadSound(filePath.c_str()));
		}
		if (folder == "General")
		{
			filePath += folder + "/" + audioName;
			//std::cout << filePath << std::endl;
			generalSFX.push_back(AEAudioLoadSound(filePath.c_str()));
		}
		if (folder == "Player")
		{
			filePath += folder + "/" + audioName;
			//std::cout << filePath << std::endl;
			
			if (subFolder == "death") playerDeath_SFX.push_back(AEAudioLoadSound(filePath.c_str()));
			if (subFolder == "hurt") playerHurt_SFX.push_back(AEAudioLoadSound(filePath.c_str()));
		}
	}
}

void Audio::Audio_Init()
{
	Audio_Load("Assets/Audio/config.txt");
	gAudio.audioGroup.combat = AEAudioCreateGroup();
	gAudio.audioGroup.player = AEAudioCreateGroup();
	gAudio.audioGroup.enemy = AEAudioCreateGroup();
	gAudio.audioGroup.fireworks = AEAudioCreateGroup();
	gAudio.audioGroup.general = AEAudioCreateGroup();
	gAudio.audioGroup.BGM = AEAudioCreateGroup();
}

void Audio::PlayBGM(BGMSFX bgmSFX)
{
	AEAudioPlay(BGM[bgmSFX], audioGroup.BGM, 1.0f, 1.0f, -1); // loop = true
}


void Audio::PlayCombatSFX(CombatSFX combatSound)
{
	if (combatSound == COMBAT_HIT)
	{
		int soundID{ Vectors::get_random(0, hitSFX.size()-1) };
		AEAudioPlay(hitSFX[soundID], audioGroup.combat, 1.0f, 1.0f, 0);
	}
	if (combatSound == COMBAT_SWING)
	{
		int soundID{ Vectors::get_random(0, swingSFX.size()-1) };
		AEAudioPlay(swingSFX[soundID], audioGroup.combat, 1.0f, 1.0f, 0);
	}
	if (combatSound == COMBAT_PARRY)
	{
		int soundID{ Vectors::get_random(0, parrySFX.size()-1) };
		AEAudioPlay(parrySFX[soundID], audioGroup.combat, 1.0f, 1.0f, 0);
	}
}

void Audio::PlayEnemySFX(EnemySFX enemySound)
{
	if (enemySound == ENEMY_HURT)
	{
		int soundID{ Vectors::get_random(0, enemyVocal_SFX.size() - 1)};
		AEAudioPlay(enemyVocal_SFX[soundID], audioGroup.enemy, 1.0f, 1.0f, 0);
	}
	if (enemySound == ENEMY_VOCAL)
	{
		int soundID{ Vectors::get_random(0, enemyHurt_SFX.size() - 1) };
		AEAudioPlay(enemyHurt_SFX[soundID], audioGroup.enemy, 1.0f, 1.0f, 0);
	}
	if (enemySound == ENEMY_LAUGH)
	{
		int soundID{ Vectors::get_random(0, enemyLaugh_SFX.size() - 1) };
		AEAudioPlay(enemyLaugh_SFX[soundID], audioGroup.enemy, 1.0f, 1.0f, 0);
	}
}

void Audio::PlayPlayerSFX(PlayerSFX playerSound)
{
	if (playerSound == PLAYER_DEATH)
	{
		int soundID{ Vectors::get_random(0, playerDeath_SFX.size() - 1)};
		AEAudioPlay(playerDeath_SFX[soundID], audioGroup.player, 1.0f, 1.0f, 0);
	}
	if (playerSound == PLAYER_HURT)
	{
		int soundID{ Vectors::get_random(0, playerHurt_SFX.size() - 1)};
		AEAudioPlay(playerHurt_SFX[soundID], audioGroup.player, 1.0f, 1.0f, 0);
	}
}

void Audio::PlayGeneralSFX(GeneralSFX generalSound)
{
	if (generalSound == GENERAL_ANNOUNCEMENT)
	{
		AEAudioPlay(generalSFX[GENERAL_ANNOUNCEMENT], audioGroup.general, 1.0f, 1.0f, 0);
	}
	if (generalSound == GENERAL_GAMEOVER)
	{
		AEAudioPlay(generalSFX[GENERAL_GAMEOVER], audioGroup.general, 1.0f, 1.0f, 0);
	}
	if (generalSound == GENERAL_TRUMPET)
	{
		AEAudioPlay(generalSFX[GENERAL_TRUMPET], audioGroup.general, 1.0f, 1.0f, 0);
	}
}

void Audio::PlayFireworksSFX()
{
		int soundID{ Vectors::get_random(0, fireworksSFX.size() - 1) };
		AEAudioPlay(fireworksSFX[soundID], audioGroup.fireworks, 1.0f, 1.0f, 0);
}

void Audio::UnloadBGM()
{
	AEAudioStopGroup(audioGroup.BGM);

	for (int i{}; i < BGM.size(); ++i) AEAudioUnloadAudio(BGM[i]);

	BGM.clear();
	AEAudioUnloadAudioGroup(audioGroup.BGM);
}
void Audio::UnloadCombatSFX()
{
	AEAudioStopGroup(audioGroup.combat);

	for (int i{}; i < hitSFX.size(); ++i) AEAudioUnloadAudio(hitSFX[i]);
 	for (int i{}; i < swingSFX.size(); ++i) AEAudioUnloadAudio(swingSFX[i]);
	for (int i{}; i < parrySFX.size(); ++i) AEAudioUnloadAudio(parrySFX[i]);

	hitSFX.clear();
	swingSFX.clear();
	parrySFX.clear();
	AEAudioUnloadAudioGroup(audioGroup.combat);
}
void Audio::UnloadEnemySFX()
{
	AEAudioStopGroup(audioGroup.enemy);

	for (int i{}; i < enemyVocal_SFX.size(); ++i) AEAudioUnloadAudio(enemyVocal_SFX[i]);
	for (int i{}; i < enemyHurt_SFX.size(); ++i) AEAudioUnloadAudio(enemyHurt_SFX[i]);
	for (int i{}; i < enemyLaugh_SFX.size(); ++i) AEAudioUnloadAudio(enemyLaugh_SFX[i]);

	enemyVocal_SFX.clear();
	enemyHurt_SFX.clear();
	enemyLaugh_SFX.clear();
	AEAudioUnloadAudioGroup(audioGroup.enemy);
}
void Audio::UnloadPlayerSFX()
{
	AEAudioStopGroup(audioGroup.player);

	for (int i{}; i < playerDeath_SFX.size(); ++i) AEAudioUnloadAudio(playerDeath_SFX[i]);
	for (int i{}; i < playerHurt_SFX.size(); ++i) AEAudioUnloadAudio(playerHurt_SFX[i]);

	playerDeath_SFX.clear();
	playerHurt_SFX.clear();
	AEAudioUnloadAudioGroup(audioGroup.player);
}
void Audio::UnloadGeneralSFX()
{
	AEAudioStopGroup(audioGroup.general);

	for (int i{}; i < generalSFX.size(); ++i) AEAudioUnloadAudio(generalSFX[i]);

	generalSFX.clear();
	AEAudioUnloadAudioGroup(audioGroup.general);
}
void Audio::UnloadFireworksSFX()
{
	AEAudioStopGroup(audioGroup.fireworks);

	for (int i{}; i < fireworksSFX.size(); ++i) AEAudioUnloadAudio(fireworksSFX[i]);

	fireworksSFX.clear();
	AEAudioUnloadAudioGroup(audioGroup.fireworks);
}