#include "gameFramework/GameApplication.h"

#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "level/ArenaTestLevel.h"

#include <framework/AssetManager.h>
#include <Config.h>
#include "framework/PerfMonitor.h"

ly::Application* GetApplication()
{
	return new ly::GameApplication();
}

namespace ly
{
	GameApplication::GameApplication()
		: Application({ 1920, 1080}, 64, std::string("LightYears"), sf::Style::Close | sf::Style::Titlebar)
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory(getResourceDir());
		if (!LightYearsAbilitySystemComponent::RegisterGameContent())
		{
			LY_GAME_ERROR("Failed to register shipped game content; application will not start the game world");
			QuitApplication();
			return;
		}

		ly::perf::g_disableLights.store(false);

		LoadWorld<ArenaTestLevel>();
	}

	void GameApplication::Tick(float)
	{
	}
}


