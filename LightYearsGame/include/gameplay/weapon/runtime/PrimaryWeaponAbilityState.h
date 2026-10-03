#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"
#include "gameplay/weapon/PrimaryWeaponHandler.h"

#include <cstdint>
#include <optional>
#include <string>

namespace ly
{
	// Per-ability persistent weapon state. Any ability carrying a FireWeaponAction
	// uses it, not only PrimaryFire definitions, so GameAbility owns it by value.
	// This class holds only state; resolving it stays with the owning ability.
	class PrimaryWeaponAbilityState
	{
	public:
		struct PreparedConfiguration
		{
			PrimaryWeaponExecutionSystem::PreparedRuntimeConfiguration runtime;
			sas::GameplayAttributeList attributes;
			uint64_t attributeRevision = 0;
			uint64_t modifierRevision = 0;
		};

		PrimaryWeaponRuntimeState runtime;
		std::string weaponId;
		sas::GameplayAttributeList attributes;
		List<GameplayTag> damageTags;
		uint64_t attributeRevision = 0;
		uint64_t configurationRevision = 0;
		uint64_t modifierRevision = 0;
		bool hasResolvedContext = false;
		std::optional<PreparedConfiguration> prepared;

		void SetFireIntervalRemaining(float interval);
		void TickFireInterval(float deltaTime);
		void UpdateContext(
			const PrimaryWeaponDefinition& weaponDefinition,
			const sas::GameplayAttributeList& newAttributes,
			const List<GameplayTag>& newDamageTags,
			uint64_t newAttributeRevision,
			uint64_t newConfigurationRevision,
			uint64_t newModifierRevision
		);
		void ApplyResolvedAttributes(
			sas::GameplayAttributeList&& newAttributes,
			uint64_t newAttributeRevision,
			uint64_t newModifierRevision,
			uint64_t newConfigurationRevision
		) noexcept;
		void CommitPrepared(uint64_t newConfigurationRevision) noexcept;
	};
}
