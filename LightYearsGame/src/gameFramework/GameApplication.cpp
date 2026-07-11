#include "gameFramework/GameApplication.h"

#include "gameplay/ability/actors/sunBeam/SunBeamStrikeActor.h"
#include "presentation/effects/shield/ShieldVisual.h"
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
		RegisterSunBeamStrikeActorType();
		RegisterShieldVisuals();

		ly::perf::g_disableLights.store(false);

		LoadWorld<ArenaTestLevel>();
	}

	void GameApplication::Tick(float)
	{
	}
}


