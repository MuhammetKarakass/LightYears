#pragma once
#include <framework/EngineAssets.h>
#include <framework/EngineContext.h>

namespace vega
{
	// Called by the game module and by standalone test hosts before creating UI
	// or lights. Linking reflection must never change another project's defaults.
	inline void InstallVegaEngineAssets()
	{
		EngineContext::Current().Defaults() = {
			"SpaceShooterRedux/Bonus/kenvector_future.ttf",
			"SpaceShooterRedux/PNG/UI/buttonBlue.png",
			"SpaceShooterRedux/Bonus/sfx_zap.ogg",
			"SpaceShooterRedux/Shaders/point_light.frag",
			"SpaceShooterRedux/Shaders/simple_color.frag"
		};
	}
}
