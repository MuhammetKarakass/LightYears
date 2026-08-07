#pragma once

#include <string>

namespace ly
{
	// Composition root for shipped game content. It owns loading, registration,
	// and validation order; runtime components do not participate in startup.
	class GameContentBootstrap final
	{
	public:
		static bool Register();
	};
}
