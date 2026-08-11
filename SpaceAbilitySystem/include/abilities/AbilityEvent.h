#pragma once

#include "abilities/AbilityPolicies.h"
#include "content/ContentId.h"
#include "framework/Core.h"

#include <typeinfo>

namespace sas
{
	struct AbilityEvent
	{
		ly::GameplayTag eventTag;
		float magnitude = 0.f;
		ContentId sourceAbilityId;
		ly::List<ly::GameplayTag> sourceAbilityTags;
		// Optional semantic payload selectors (damage type, status, category,
		// capability...). Identity remains in typed context or dedicated fields.
		ly::List<ly::GameplayTag> payloadTags;

		template <typename Source>
		void SetSource(Source* source)
		{
			sourceObject = source;
			sourceType = source ? &typeid(Source) : nullptr;
		}

		template <typename Source>
		Source* GetSource() const
		{
			return sourceObject &&
				sourceType &&
				*sourceType == typeid(Source)
					? static_cast<Source*>(sourceObject)
					: nullptr;
		}

		template <typename Target>
		void SetTarget(Target* target)
		{
			targetObject = target;
			targetType = target ? &typeid(Target) : nullptr;
		}

		template <typename Target>
		Target* GetTarget() const
		{
			return targetObject &&
				targetType &&
				*targetType == typeid(Target)
					? static_cast<Target*>(targetObject)
					: nullptr;
		}

		template <typename Context>
		void SetContext(const Context* context)
		{
			contextObject = context;
			contextType = context ? &typeid(Context) : nullptr;
		}

		template <typename Context>
		const Context* GetContext() const
		{
			return contextObject &&
				contextType &&
				*contextType == typeid(Context)
					? static_cast<const Context*>(contextObject)
					: nullptr;
		}

	private:
		void* sourceObject = nullptr;
		const std::type_info* sourceType = nullptr;
		void* targetObject = nullptr;
		const std::type_info* targetType = nullptr;
		const void* contextObject = nullptr;
		const std::type_info* contextType = nullptr;
	};

	// Lifecycle events are the public, data-driven boundary for ability state.
	// Internal behavior flow continues to use Activate/End/Cancel callbacks.
	struct AbilityLifecycleEvent final : AbilityEvent
	{
		ContentId abilityId;
		// Semantic tags describe the source ability (category, family and other
		// queryable capabilities). The event kind remains generic; consumers do
		// not need one event tag per ability.
		ly::List<ly::GameplayTag> abilityTags;
		AbilityEndReason endReason = AbilityEndReason::Completed;
	};
}
