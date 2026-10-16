#pragma once

#include <string>
#include <filesystem>
#include <vector>

namespace ly
{
	class EngineContext;
	// Composition root for shipped game content. It owns loading, registration,
	// and validation order; runtime components do not participate in startup.
	class GameContentBootstrap final
	{
	public:
		static bool Register();
		static void Reload(EngineContext& context, const std::vector<std::filesystem::path>& paths);
	};
}
