#include "pch.h"
#include "Audio.h"
#include <fstream>
#include <sstream>

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
			enemySFX.push_back(AEAudioLoadSound(filePath.c_str()));
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
			playerSFX.push_back(AEAudioLoadSound(filePath.c_str()));
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

void Audio::PlayBGM(int id)
{
	std::cout << "BGM SIZE: " << BGM.size() << std::endl;
	if (id >= 0 && id < BGM.size())
		AEAudioPlay(BGM[id], audioGroup.BGM, 1.0f, 1.0f, -1); // loop = true
}

void Audio::StopBGM(int id)
{
	if (id >= 0 && id < BGM.size())
	{
		AEAudioUnloadAudio(BGM[id]);
		AEAudioUnloadAudioGroup(audioGroup.BGM);
	}
}