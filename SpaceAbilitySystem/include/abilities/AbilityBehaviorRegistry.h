#pragma once

#include "abilities/FactoryRegistry.h"

#include <functional>
#include <memory>
#include <utility>

namespace sas
{
	template <typename Behavior, typename Key, typename Hash = std::hash<Key>>
	class AbilityBehaviorRegistry
	{
	public:
		using Factory = std::function<std::unique_ptr<Behavior>()>;
		using Registry = FactoryRegistry<Key, Behavior, Hash>;

		static bool Register(const Key& behaviorId, Factory factory)
		{
			return factory
				? GetRegistry().Register(behaviorId, std::move(factory))
				: false;
		}

		static bool IsRegistered(const Key& behaviorId)
		{
			return GetRegistry().IsRegistered(behaviorId);
		}

		static std::unique_ptr<Behavior> Create(const Key& behaviorId)
		{
			return GetRegistry().Create(behaviorId);
		}

	private:
		static Registry& GetRegistry()
		{
			static Registry registry;
			return registry;
		}
	};
}
