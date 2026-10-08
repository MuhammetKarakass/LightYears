#pragma once
#include "abilities/AbilityTypes.h"
#include "gameplay/tags/GameplayTags.h"

namespace ly
{
	inline sas::AbilitySlot AbilitySlotForModuleSystem(const GameplayTag& system)
	{
		if (system == GameplayTags::Module::System::AbilitySlot1) return sas::AbilitySlot::Ability1;
		if (system == GameplayTags::Module::System::AbilitySlot2) return sas::AbilitySlot::Ability2;
		if (system == GameplayTags::Module::System::AbilitySlot3) return sas::AbilitySlot::Ability3;
		if (system == GameplayTags::Module::System::AbilitySlot4) return sas::AbilitySlot::Ability4;
		return sas::AbilitySlot::None;
	}
}
