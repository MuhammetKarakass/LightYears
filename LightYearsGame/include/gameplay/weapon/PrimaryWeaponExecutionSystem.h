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
		struct ActiveFireSimulationResult
		{
			int executionsProduced = 0;
			bool lifecycleInterrupted = false;
		};

		static ActiveFireSimulationResult SimulateActiveFire(
			const PrimaryWeaponExecutionContext& context,
			PrimaryWeaponRuntimeState& state,
			float deltaTime,
			float fireInterval,
			int maxExecutions = 0,
			int currentExecutionCount = 0
		);
		static void TickInactive(
			const PrimaryWeaponExecutionContext& context,
			PrimaryWeaponRuntimeState& state,
			float deltaTime
		);
		// Advances the shared magazine state without ticking weapon-type features.
		static void AdvanceMagazineReload(
			const PrimaryWeaponDefinition& definition,
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
		static float CalculateAttackSpeedMultiplier(const Actor& owner);
		static float CalculateReloadDuration(
			float baseReloadTime,
			const Actor& owner
		);
	};
}
