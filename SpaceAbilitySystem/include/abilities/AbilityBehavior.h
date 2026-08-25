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

		// Active abilities may consume a new press themselves. This is used by
		// staged abilities such as a two-point portal, while ordinary abilities
		// continue through the normal lifecycle evaluator.
		virtual bool OnInputPressed(Context&)
		{
			return false;
		}

		virtual void Tick(Context&, float)
		{
		}

		virtual void End(Context&, AbilityEndReason)
		{
		}
	};
}
