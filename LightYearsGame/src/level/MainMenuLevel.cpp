#include "level/MainMenuLevel.h"
#include "widget/MainMenuHUD.h"
#include "framework/Application.h"
#include "framework/AudioManager.h"
#include "level/LevelOne.h"

namespace ly
{
	MainMenuLevel::MainMenuLevel(Application* owningApplication):
		World(owningApplication)
	{
		mMainMenuHUD = SpawnHUD<MainMenuHUD>();
	}

	void MainMenuLevel::BeginPlay()
	{
		World::BeginPlay();

		mMainMenuHUD.lock()->onStartButtonClicked.BindAction(GetWeakPtr(), &MainMenuLevel::StartGame);
		mMainMenuHUD.lock()->onQuitButtonClicked.BindAction(GetWeakPtr(), &MainMenuLevel::QuitGame);


		AudioManager::GetAudioManager().PlayMusic(
			"SpaceShooterRedux/Musics/menu_theme.ogg",
			AudioType::Music,
			70.0f,
			1.0f,
			true
		);
	}

	void MainMenuLevel::StartGame()
	{
		// Menü müziðini durdur
		AudioManager::GetAudioManager().StopMusic();
		
		GetApplication()->LoadWorld<LevelOne>();
	}

	void MainMenuLevel::QuitGame()
	{
		GetApplication()->QuitApplication();
	}
}