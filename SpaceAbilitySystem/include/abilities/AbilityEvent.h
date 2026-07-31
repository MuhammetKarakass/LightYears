#pragma once

#include "framework/Core.h"

#include <typeinfo>

namespace sas
{
	struct AbilityEvent
	{
		ly::GameplayTag eventTag;
		float magnitude = 0.f;

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
}
