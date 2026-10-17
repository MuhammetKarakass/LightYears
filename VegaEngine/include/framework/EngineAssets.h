#pragma once
#include <string>

namespace vega
{
	// Optional defaults for engine widgets and lights. Games install their own
	// paths; an empty entry means the resource is not part of this project.
	struct EngineAssets
	{
		std::string font, buttonTexture, buttonSound, pointLightShader, simpleColorShader;
		// Deprecated: use the owning EngineContext::Defaults().
		static EngineAssets& Get();
	};
}
