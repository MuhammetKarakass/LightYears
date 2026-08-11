#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/weapon/PrimaryWeaponHandler.h"

namespace ly
{
	class PrimaryWeaponExecutionSystem
	{
	public:
		static PrimaryWeaponValidationResult ValidateDefinition(
			const PrimaryWeaponDefinition& definition
		);
		static PrimaryWeaponValidationResult InitializeRuntime(
			const PrimaryWeaponDefinition& definition,
			PrimaryWeaponRuntimeState& state,
			const List<std::string>* unlockedUpgradeIds = nullptr
		);
		static PrimaryWeaponValidationResult EnsureRuntimeConfigured(
			const PrimaryWeaponDefinition& definition,
			PrimaryWeaponRuntimeState& state,
			const List<std::string>* unlockedUpgradeIds = nullptr
		);

		static void BeginFire(
			const PrimaryWeaponExecutionContext& context,
			PrimaryWeaponRuntimeState& state
		);
		static bool UsesIntervalFire(const PrimaryWeaponRuntimeState& state);
		static bool FireOnce(
			const PrimaryWeaponExecutionContext& context,
			PrimaryWeaponRuntimeState& state
		);
		static void TickFire(
			const PrimaryWeaponExecutionContext& context,
			PrimaryWeaponRuntimeState& state,
			float deltaTime
		);
		static void TickInactive(
			const PrimaryWeaponExecutionContext& context,
			PrimaryWeaponRuntimeState& state,
			float deltaTime
		);
		static void EndFire(
			const PrimaryWeaponExecutionContext& context,
			PrimaryWeaponRuntimeState& state
		);

		static float ConsumeRequestedCooldown(PrimaryWeaponRuntimeState& state);
		static float BuildBaseFireInterval(
			const sas::GameplayAttributeList& attributes,
			float actionInterval
		);
	};
}
