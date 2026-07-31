#pragma once

namespace sas
{
	class AbilitySystemComponent;

	class AbilitySystemInterface
	{
	public:
		virtual ~AbilitySystemInterface() = default;

		virtual AbilitySystemComponent& GetAbilitySystemComponent() = 0;
		virtual const AbilitySystemComponent&
			GetAbilitySystemComponent() const = 0;
	};
}
