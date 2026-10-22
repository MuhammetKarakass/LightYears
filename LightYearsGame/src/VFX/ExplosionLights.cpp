#include "VFX/ExplosionLights.h"

#include <algorithm>

namespace vega
{
	namespace
	{
		std::vector<ExplosionFlash>& Flashes()
		{
			static std::vector<ExplosionFlash> flashes;
			return flashes;
		}
	}

	void ExplosionLights::Add(const ExplosionFlash& flash)
	{
		Flashes().push_back(flash);
	}

	void ExplosionLights::Update(float deltaSeconds)
	{
		std::vector<ExplosionFlash>& flashes = Flashes();
		for (ExplosionFlash& flash : flashes)
		{
			flash.age += deltaSeconds;
		}
		flashes.erase(std::remove_if(flashes.begin(), flashes.end(), [](const ExplosionFlash& flash) { return flash.age >= flash.lifetime; }), flashes.end());
	}

	const std::vector<ExplosionFlash>& ExplosionLights::Active()
	{
		return Flashes();
	}
}
