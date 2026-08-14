#include "gameplay/input/AbilityInputSchema.h"

namespace ly
{
	const List<AbilityInputBinding>& AbilityInputSchema::GetBindings()
	{
		static const List<AbilityInputBinding> bindings{
			{ sas::AbilitySlot::PrimaryFire, sf::Keyboard::Key::Space, "Space" },
			{ sas::AbilitySlot::Ability1, sf::Keyboard::Key::Q, "Q" },
			{ sas::AbilitySlot::Ability2, sf::Keyboard::Key::E, "E" },
			{ sas::AbilitySlot::Ability3, sf::Keyboard::Key::F, "F" },
			{ sas::AbilitySlot::Ability4, sf::Keyboard::Key::R, "R" }
		};
		return bindings;
	}

	const AbilityInputBinding* AbilityInputSchema::Find(sas::AbilitySlot slot)
	{
		for (const AbilityInputBinding& binding : GetBindings())
		{
			if (binding.slot == slot)
			{
				return &binding;
			}
		}
		return nullptr;
	}

	const char* AbilityInputSchema::GetLabel(sas::AbilitySlot slot)
	{
		const AbilityInputBinding* binding = Find(slot);
		return binding ? binding->label : "";
	}

	sf::Keyboard::Key AbilityInputSchema::GetKey(sas::AbilitySlot slot)
	{
		const AbilityInputBinding* binding = Find(slot);
		return binding ? binding->key : sf::Keyboard::Key::Unknown;
	}
}
