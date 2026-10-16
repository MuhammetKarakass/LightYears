#pragma once

#include "abilities/AbilityRuntimeSnapshot.h"
#include "effects/GameplayEffectRuntimeSnapshot.h"
#include "effects/GameplayEffectSpec.h"
#include <chrono>
#include <functional>
#include <memory>
#include <vector>

namespace ly { class Actor; class World; }

namespace sas
{
	class AbilitySystemComponent;
	struct AbilityDebugEntry
	{
		AbilityRuntimeSnapshot runtime;
		std::string displayName;
		std::string blockedReason;
		bool passive = false;
		const char* State() const
		{
			return runtime.active ? "active" : !blockedReason.empty() ? "blocked" :
				(runtime.cooldownRemaining > 0.f && (runtime.chargeCapacity <= 0 || runtime.charges == 0)) ? "cooldown" : "ready";
		}
	};
	struct AbilityDebugEvent
	{
		double seconds = 0;
		std::string message;
	};
	struct AbilitySystemDebugSnapshot
	{
		std::vector<AbilityDebugEntry> abilities;
		ly::List<GameplayEffectRuntimeSnapshot> effects;
		GameplayAttributeList attributes;
		std::vector<std::pair<ly::GameplayTag, int>> tags;
		std::vector<AbilityDebugEvent> events;
	};
	// The game installs these adapters. Neither the editor nor SAS knows a game actor type.
	struct AbilitySystemDebugView
	{
		inline static std::function<AbilitySystemComponent*(ly::Actor&)> resolve;
		inline static std::function<std::shared_ptr<ly::Actor>(ly::World&)> player;
		inline static std::function<ly::List<GameplayEffectSpec>()> effectDefinitions;
		// Zero means no periodic execution. Games resolve behavior-owned clocks here.
		inline static std::function<float(const GameplayEffectRuntimeSnapshot&)> effectPeriod;
		inline static bool recording = false;
		inline static std::uint64_t session = 0;
		inline static std::chrono::steady_clock::time_point epoch;
		static void BeginPlay()
		{
			++session;
			epoch = std::chrono::steady_clock::now();
			recording = true;
		}
	};
}
