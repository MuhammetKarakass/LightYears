#pragma once

#include "effects/GameplayEffectRuntimeState.h"
#include "effects/GameplayEffectPolicies.h"

#include <memory>
#include <utility>

namespace sas
{
	class GameplayEffectRuntimeContext
	{
	public:
		virtual ~GameplayEffectRuntimeContext() = default;
	};

	struct GameplayEffectSourceContext
	{
		GameplayEffectSourceContext(
			const void* object = nullptr,
			const void* scope = nullptr,
			std::shared_ptr<GameplayEffectRuntimeContext> context = {}
		)
			: sourceObject{ object },
			sourceScope{ scope },
			runtimeContext{ std::move(context) }
		{
		}

		const void* sourceObject = nullptr;
		const void* sourceScope = nullptr;
		std::shared_ptr<GameplayEffectRuntimeContext> runtimeContext;
	};

	template <typename Spec>
	struct GameplayEffectRuntimeEntry : GameplayEffectRuntimeState
	{
		void ResetRuntimeAttributesFromSpec()
		{
			ResetRuntimeAttributes(spec.attributes);
		}

		// Stack policies with no custom behavior treat the incoming spec as the
		// latest runtime configuration. Custom stack behavior owns this refresh
		// itself because its runtime attributes may represent accumulated state.
		void RefreshRuntimeAttributesFromSpec()
		{
			ResetRuntimeAttributesFromSpec();
		}

		GameplayEffectRuntimeSnapshot BuildRuntimeSnapshot() const
		{
			auto snapshot = BuildSnapshot(spec.definition.effectId);
			snapshot.handle = handle;
			snapshot.source = sourceObject ? std::to_string(reinterpret_cast<std::uintptr_t>(sourceObject)) : "unspecified";
			snapshot.infinite = spec.definition.durationPolicy == GameplayEffectDurationPolicy::Infinite;
			snapshot.modifiers = spec.modifiers;
			snapshot.grantedTags = spec.definition.grantedTags;
			return snapshot;
		}

		Spec spec;
		const void* sourceObject = nullptr;
		const void* sourceScope = nullptr;
		std::shared_ptr<GameplayEffectRuntimeContext> runtimeContext;

		template <typename Source>
		Source* GetSourceObject() const
		{
			return static_cast<Source*>(const_cast<void*>(sourceObject));
		}
	};
}
