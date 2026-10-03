#include "gameplay/weapon/runtime/PrimaryWeaponAbilityState.h"

#include <algorithm>

namespace ly
{
	void PrimaryWeaponAbilityState::SetFireIntervalRemaining(float interval)
	{
		runtime.fireIntervalRemaining = std::max(0.f, interval);
	}

	void PrimaryWeaponAbilityState::TickFireInterval(float deltaTime)
	{
		runtime.fireIntervalRemaining = std::max(0.f, runtime.fireIntervalRemaining - deltaTime);
	}

	void PrimaryWeaponAbilityState::UpdateContext(
		const PrimaryWeaponDefinition& weaponDefinition,
		const sas::GameplayAttributeList& newAttributes,
		const List<GameplayTag>& newDamageTags,
		uint64_t newAttributeRevision,
		uint64_t newConfigurationRevision,
		uint64_t newModifierRevision
	)
	{
		if (weaponId != weaponDefinition.weaponId)
		{
			runtime = PrimaryWeaponRuntimeState{};
			weaponId = weaponDefinition.weaponId;
			hasResolvedContext = false;
		}
		attributes = newAttributes;
		damageTags = newDamageTags;
		attributeRevision = newAttributeRevision;
		configurationRevision = newConfigurationRevision;
		modifierRevision = newModifierRevision;
		hasResolvedContext = true;
	}

	void PrimaryWeaponAbilityState::ApplyResolvedAttributes(
		sas::GameplayAttributeList&& newAttributes,
		uint64_t newAttributeRevision,
		uint64_t newModifierRevision,
		uint64_t newConfigurationRevision
	) noexcept
	{
		attributes = std::move(newAttributes);
		attributeRevision = newAttributeRevision;
		modifierRevision = newModifierRevision;
		configurationRevision = newConfigurationRevision;
		hasResolvedContext = true;
	}

	void PrimaryWeaponAbilityState::CommitPrepared(uint64_t newConfigurationRevision) noexcept
	{
		if (prepared)
		{
			PreparedConfiguration& preparedConfiguration = *prepared;
			PrimaryWeaponExecutionSystem::CommitRuntimeConfiguration(runtime, std::move(preparedConfiguration.runtime));
			ApplyResolvedAttributes(
				std::move(preparedConfiguration.attributes),
				preparedConfiguration.attributeRevision,
				preparedConfiguration.modifierRevision,
				newConfigurationRevision
			);
		}
		prepared.reset();
	}
}
