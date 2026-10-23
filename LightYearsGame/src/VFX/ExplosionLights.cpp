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

	void ExplosionLights::Update(float deltaSeconds, const World* world)
	{
		std::vector<ExplosionFlash>& flashes = Flashes();
		for (ExplosionFlash& flash : flashes)
		{
			// Two pipelines can render different worlds in sequence; only the
			// rendered world owns the ageing clock of its flashes.
			if (flash.world == world)
			{
				flash.age += deltaSeconds;
			}
		}
		flashes.erase(std::remove_if(flashes.begin(), flashes.end(), [](const ExplosionFlash& flash) { return flash.age >= flash.lifetime; }), flashes.end());
	}

	std::vector<ExplosionFlash> ExplosionLights::Active(const World* world)
	{
		std::vector<ExplosionFlash> active;
		for (const ExplosionFlash& flash : Flashes())
		{
			if (flash.world == world)
			{
				active.push_back(flash);
			}
		}
		return active;
	}

	void ExplosionLights::Clear()
	{
		Flashes().clear();
	}
}
