#pragma once

#include "framework/Core.h"
#include "abilities/AbilityPolicies.h"

#include <SFML/Window/Keyboard.hpp>

namespace ly
{
	struct AbilityInputBinding
	{
		sas::AbilitySlot slot = sas::AbilitySlot::None;
		sf::Keyboard::Key key = sf::Keyboard::Key::Unknown;
		const char* label = "";
	};

	class AbilityInputSchema
	{
	public:
		static const List<AbilityInputBinding>& GetBindings();
		static const AbilityInputBinding* Find(sas::AbilitySlot slot);
		static const char* GetLabel(sas::AbilitySlot slot);
		static sf::Keyboard::Key GetKey(sas::AbilitySlot slot);
	};
}
