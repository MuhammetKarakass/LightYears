#include "framework/scene/SceneEntityComponent.h"
#include "framework/reflection/Reflection.h"
#include <random>
#include <utility>

namespace ly
{
	SceneEntityComponent::SceneEntityComponent(Actor& owner, std::string entityGuid, std::string entityName)
		: ActorComponent{ owner }, guid{ entityGuid.empty() ? GenerateGuid() : std::move(entityGuid) }, name{ std::move(entityName) }
	{
	}

	std::string SceneEntityComponent::GenerateGuid()
	{
		static thread_local std::mt19937_64 random = []()
		{
			std::random_device entropy;
			std::seed_seq seed{ entropy(), entropy(), entropy(), entropy(), entropy(), entropy(), entropy(), entropy() };
			return std::mt19937_64{ seed };
		}();
		constexpr char hex[] = "0123456789abcdef";
		std::string result(32, '0');
		for (int half = 0; half < 2; ++half)
		{
			const auto bits = random();
			for (int digit = 0; digit < 16; ++digit)
				result[half * 16 + digit] = hex[(bits >> (digit * 4)) & 15];
		}
		return result;
	}

	VEGA_REFLECT(SceneEntityComponent, ActorComponent)
	{
		VEGA_CATEGORY("Scene");
		VEGA_PROPERTY("Name", GetName, SetName);
		VEGA_READONLY("GUID", GetGuid);
	}
}
