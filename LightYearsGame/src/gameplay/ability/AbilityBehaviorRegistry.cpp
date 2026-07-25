#include "gameplay/ability/AbilityBehaviorRegistry.h"

namespace ly
{
	namespace
	{
		using AbilityBehaviorFactoryMap = Dictionary<
			GameplayTag,
			AbilityBehaviorFactory,
			GameplayTagHash
		>;

		AbilityBehaviorFactoryMap& GetFactories()
		{
			static AbilityBehaviorFactoryMap factories;
			return factories;
		}

		void EnsureConfiguredBehavior()
		{
			static const bool initialized = []
			{
				GetFactories().emplace(
					AbilityBehaviorSchema::Configured,
					[] { return std::make_unique<AbilityBehavior>(); }
				);
				return true;
			}();
			(void)initialized;
		}
	}

	bool AbilityBehaviorRegistry::Register(
		const GameplayTag& behaviorId,
		AbilityBehaviorFactory factory)
	{
		EnsureConfiguredBehavior();
		if (!behaviorId.IsValid() || !factory)
		{
			return false;
		}
		return GetFactories().emplace(behaviorId, std::move(factory)).second;
	}

	bool AbilityBehaviorRegistry::IsRegistered(const GameplayTag& behaviorId)
	{
		EnsureConfiguredBehavior();
		return GetFactories().find(behaviorId) != GetFactories().end();
	}

	unique_ptr<AbilityBehavior> AbilityBehaviorRegistry::Create(const GameplayTag& behaviorId)
	{
		EnsureConfiguredBehavior();
		const auto found = GetFactories().find(behaviorId);
		return found != GetFactories().end() ? found->second() : nullptr;
	}
}
