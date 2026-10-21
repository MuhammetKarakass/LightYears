#include "level/MainMenuLevel.h"
#include "widget/MainMenuHUD.h"
#include "framework/Application.h"
#include "framework/AudioManager.h"
#include "level/ArenaTestLevel.h"

namespace vega
{
	// Headless levels (tests, validation) have no application; they use the bound scope explicitly.
	MainMenuLevel::MainMenuLevel(Application* owningApplication):
		World(owningApplication, owningApplication ? nullptr : EngineContext::TryCurrent())
	{
		mMainMenuHUD = SpawnHUD<MainMenuHUD>();
	}

	void MainMenuLevel::BeginPlay()
	{
		World::BeginPlay();

		mMainMenuHUD.lock()->onStartButtonClicked.BindAction(GetWeakPtr(), &MainMenuLevel::StartGame);
		mMainMenuHUD.lock()->onQuitButtonClicked.BindAction(GetWeakPtr(), &MainMenuLevel::QuitGame);


		GetEngineContext().Audio().PlayMusic(
			"SpaceShooterRedux/Musics/menu_theme.ogg",
			AudioType::Music,
			30.0f,
			1.0f,
			true
		);
	}

	void MainMenuLevel::StartGame()
	{
		// Stop menu music.
		GetEngineContext().Audio().StopMusic();
		
		GetApplication()->LoadWorld<ArenaTestLevel>();
	}

	void MainMenuLevel::QuitGame()
	{
		GetApplication()->QuitApplication();
	}
}

