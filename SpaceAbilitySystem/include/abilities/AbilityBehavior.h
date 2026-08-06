#pragma once

#include "abilities/AbilityPolicies.h"

#include <string>

namespace sas
{
	template <typename Definition, typename Context>
	class AbilityBehavior
	{
	public:
		virtual ~AbilityBehavior() = default;

		virtual bool Validate(const Definition&,std::string* = nullptr) const
		{
			return true;
		}

		virtual bool Activate(Context&)
		{
			return true;
		}

		virtual void Tick(Context&, float)
		{
		}

		virtual void End(Context&, AbilityEndReason)
		{
		}
	};
}
