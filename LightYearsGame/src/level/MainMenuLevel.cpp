#include "level/MainMenuLevel.h"
#include "widget/MainMenuHUD.h"
#include "framework/Application.h"
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
		mMainMenuHUD.lock()->onStartButtonClicked.BindAction(GetWeakPtr(), &MainMenuLevel::StartGame);
		mMainMenuHUD.lock()->onQuitButtonClicked.BindAction(GetWeakPtr(), &MainMenuLevel::QuitGame);
	}
	void MainMenuLevel::StartGame()
	{
		GetApplication()->LoadWorld<LevelOne>();
	}
	void MainMenuLevel::QuitGame()
	{
		GetApplication()->QuitApplication();
	}
}