#include "attributes/AttributeMath.h"
#include "attributes/AttributeSystem.h"
#include "framework/Actor.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/time/IntervalDebt.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"

#include "gameplay/weapon/PrimaryWeaponHandlerRegistry.h"
#include "internal/PrimaryWeaponDefinitionValidator.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		struct RuntimeConfiguration
		{
			std::string weaponId;
			const PrimaryWeaponHandler* handler = nullptr;
			List<const PrimaryWeaponFeatureHandler*> features;
			std::optional<PrimaryWeaponMagazineDefinition> magazine;
			PrimaryWeaponCadenceMode cadenceMode = PrimaryWeaponCadenceMode::AuthoredScaling;
			PrimaryWeaponDamageRoundingPolicy damageRoundingPolicy = PrimaryWeaponDamageRoundingPolicy::None;
			std::optional<PrimaryWeaponEmpoweredShotDefinition> empoweredShot;
		};

		bool HasExactId(const List<std::string>& ids, const std::string& expectedId)
		{
			return std::any_of(ids.begin(), ids.end(), [&](const std::string& id)
			{
				return id == expectedId;
			});
		}

		bool HasExactId(
			const List<sas::AttributeId>& ids,
			const sas::AttributeId& expectedId
		)
		{
			return std::find(ids.begin(), ids.end(), expectedId) != ids.end();
		}

		PrimaryWeaponExecutionContext WithRuntimeState(
			const PrimaryWeaponExecutionContext& context,
			PrimaryWeaponRuntimeState& state
		)
		{
			PrimaryWeaponExecutionContext runtimeContext = context;
			runtimeContext.runtime = &state;
			return runtimeContext;
		}

		PrimaryWeaponValidationResult ResolveRuntimeConfiguration(
			const PrimaryWeaponDefinition& definition,
			const List<std::string>* unlockedUpgradeIds,
			RuntimeConfiguration& configuration
		)
		{
			const PrimaryWeaponValidationResult validation =
				PrimaryWeaponDefinitionValidator::Validate(definition);
			if (!validation.isValid)
			{
				return validation;
			}

			configuration.weaponId = definition.weaponId;
			configuration.handler =
				PrimaryWeaponHandlerRegistry::FindHandler(definition.weaponType);
			configuration.magazine = definition.magazine;
			configuration.cadenceMode = definition.cadenceMode;
			configuration.damageRoundingPolicy = definition.damageRoundingPolicy;
			configuration.empoweredShot = definition.empoweredShot;
			const auto addFeature = [&](PrimaryWeaponFeatureType featureType)
			{
				const PrimaryWeaponFeatureHandler* feature =
					PrimaryWeaponHandlerRegistry::FindFeature(featureType);
				if (feature && std::find(
						configuration.features.begin(),
						configuration.features.end(),
						feature
					) == configuration.features.end())
				{
					configuration.features.push_back(feature);
				}
			};

			for (const PrimaryWeaponFeatureType featureType : definition.featureTypes)
			{
				addFeature(featureType);
			}
			if (unlockedUpgradeIds)
			{
				for (const PrimaryWeaponLevelStep& step : definition.progressionProfile.ResolveLevelSteps())
				{
					for (const PrimaryWeaponFeatureType featureType : step.unlockedFeatureTypes)
					{
						if (HasExactId(
							*unlockedUpgradeIds,
							PrimaryWeaponFeatureUpgradeId(featureType)
						))
						{
							addFeature(featureType);
						}
					}
				}
			}
			return { true, {} };
		}

		PrimaryWeaponValidationResult InstallRuntime(
			const RuntimeConfiguration& configuration,
			PrimaryWeaponRuntimeState& state
		)
		{
			unique_ptr<PrimaryWeaponTypeRuntimeState> typeState =
				configuration.handler
					? configuration.handler->CreateRuntimeState()
					: nullptr;
			if (!typeState)
			{
				return {
					false,
					"Primary weapon runtime state could not be created."
				};
			}

			PrimaryWeaponRuntimeState candidate;
			candidate.handler = configuration.handler;
			candidate.features = configuration.features;
			candidate.typeState = std::move(typeState);
			candidate.configuredWeaponId = configuration.weaponId;
			candidate.configuredMagazine = configuration.magazine;
			candidate.configuredCadenceMode = configuration.cadenceMode;
			candidate.configuredDamageRoundingPolicy = configuration.damageRoundingPolicy;
			candidate.configuredEmpoweredShot = configuration.empoweredShot;
			if (configuration.magazine.has_value())
			{
				candidate.magazineState.roundsRemaining = configuration.magazine->capacity;
				candidate.magazineState.reloadRemaining = 0.0;
			}
			candidate.isInitialized = true;
			state = std::move(candidate);
			return { true, {} };
		}

		bool OwnsRuntimeValue(
			const List<const PrimaryWeaponFeatureHandler*>& features,
			const sas::AttributeId& key
		)
		{
			return std::any_of(features.begin(), features.end(), [&](const auto* feature)
			{
				return feature && HasExactId(feature->GetRuntimeValueKeys(), key);
			});
		}

		Map<sas::AttributeId, float> RetainFeatureValues(
			const Map<sas::AttributeId, float>& values,
			const List<const PrimaryWeaponFeatureHandler*>& features
		)
		{
			Map<sas::AttributeId, float> retainedValues;
			for (const auto& [key, value] : values)
			{
				if (OwnsRuntimeValue(features, key))
				{
					retainedValues.emplace(key, value);
				}
			}
			return retainedValues;
		}
	}

	PrimaryWeaponValidationResult PrimaryWeaponExecutionSystem::ValidateDefinition(
		const PrimaryWeaponDefinition& definition
	)
	{
		return PrimaryWeaponDefinitionValidator::Validate(definition);
	}

	PrimaryWeaponValidationResult PrimaryWeaponExecutionSystem::InitializeRuntime(
		const PrimaryWeaponDefinition& definition,
		PrimaryWeaponRuntimeState& state,
		const List<std::string>* unlockedUpgradeIds
	)
	{
		RuntimeConfiguration configuration;
		const PrimaryWeaponValidationResult validation =
			ResolveRuntimeConfiguration(
				definition,
				unlockedUpgradeIds,
				configuration
			);
		if (!validation.isValid)
		{
			return validation;
		}
		if (state.isFiring)
		{
			return {
				false,
				"Primary weapon runtime cannot be replaced while it is firing."
			};
		}
		return InstallRuntime(configuration, state);
	}

	PrimaryWeaponValidationResult
		PrimaryWeaponExecutionSystem::EnsureRuntimeConfigured(
			const PrimaryWeaponDefinition& definition,
			PrimaryWeaponRuntimeState& state,
			const List<std::string>* unlockedUpgradeIds
		)
	{
		RuntimeConfiguration configuration;
		const PrimaryWeaponValidationResult validation =
			ResolveRuntimeConfiguration(
				definition,
				unlockedUpgradeIds,
				configuration
			);
		if (!validation.isValid)
		{
			return validation;
		}

		const bool sameWeapon =
			state.configuredWeaponId == configuration.weaponId;
		const bool sameHandler =
			state.handler == configuration.handler;
		const bool hasUsableRuntime =
			state.isInitialized && state.typeState;
		if (!sameWeapon || !sameHandler || !hasUsableRuntime)
		{
			if (state.isFiring)
			{
				return {
					false,
					"Primary weapon runtime configuration cannot change while firing."
				};
			}
			return InstallRuntime(configuration, state);
		}

		if (state.configuredMagazine != configuration.magazine)
		{
			return {
				false,
				"Primary weapon magazine configuration cannot change on an existing runtime."
			};
		}
		if (state.configuredCadenceMode != configuration.cadenceMode)
		{
			return {
				false,
				"Primary weapon cadence mode cannot change on an existing runtime."
			};
		}
		if (state.configuredDamageRoundingPolicy != configuration.damageRoundingPolicy)
		{
			return { false, "Primary weapon damage rounding policy cannot change on an existing runtime." };
		}
		if (state.configuredEmpoweredShot != configuration.empoweredShot)
		{
			return {
				false,
				"Primary weapon empowered shot configuration cannot change on an existing runtime."
			};
		}

		if (state.features == configuration.features)
		{
			return { true, {} };
		}
		if (state.isFiring)
		{
			return {
				false,
				"Primary weapon features cannot change while firing."
			};
		}

		Map<sas::AttributeId, float> retainedValues =
			RetainFeatureValues(state.featureValues, configuration.features);
		state.features = std::move(configuration.features);
		state.featureValues = std::move(retainedValues);
		return { true, {} };
	}

	void PrimaryWeaponExecutionSystem::BeginFire(
		const PrimaryWeaponExecutionContext& context,
		PrimaryWeaponRuntimeState& state
	)
	{
		if (!state.isInitialized || state.isFiring ||
			!state.handler || !state.typeState)
		{
			return;
		}
		state.isFiring = true;
		const PrimaryWeaponExecutionContext runtimeContext =
			WithRuntimeState(context, state);
		state.handler->BeginFire(runtimeContext, *state.typeState);
		for (const PrimaryWeaponFeatureHandler* feature : state.features)
		{
			feature->BeginFire(runtimeContext, state);
		}
	}

	bool PrimaryWeaponExecutionSystem::UsesIntervalFire(
		const PrimaryWeaponRuntimeState& state
	)
	{
		return state.isInitialized &&
			state.handler &&
			state.handler->UsesIntervalFire();
	}

	bool PrimaryWeaponExecutionSystem::FireOnce(
		const PrimaryWeaponExecutionContext& context,
		PrimaryWeaponRuntimeState& state
	)
	{
		if (!state.isInitialized || !state.isFiring ||
			!state.handler || !state.typeState)
		{
			return false;
		}

		if (context.definition.magazine.has_value())
		{
			if (state.magazineState.reloadRemaining > 0.0 ||
				state.magazineState.roundsRemaining <= 0)
			{
				return false;
			}
		}

		PrimaryWeaponShotMetadata shotMetadata;
		shotMetadata.roundFinalDamageUp = context.definition.damageRoundingPolicy == PrimaryWeaponDamageRoundingPolicy::CeilFinalDamage;
		if (context.definition.empoweredShot.has_value())
		{
			const auto& emp = *context.definition.empoweredShot;
			const int everySuccessfulShots = static_cast<int>(std::round(sas::FindAttributeValue(
				context.attributes, PrimaryWeaponSchema::Empowered::EverySuccessfulShots, 0.f)));
			const int finalMagazineRounds = static_cast<int>(std::round(sas::FindAttributeValue(
				context.attributes, PrimaryWeaponSchema::Empowered::FinalMagazineRounds, 0.f)));
			bool isEmpowered = false;
			const uint64_t nextSuccessfulFireCount = state.successfulFireCount + 1;
			const bool periodic = everySuccessfulShots > 0 && nextSuccessfulFireCount % static_cast<uint64_t>(everySuccessfulShots) == 0;
			bool finalPhase = false;
			if (context.definition.magazine.has_value())
			{
				const int roundsBeforeFire = state.magazineState.roundsRemaining;
				finalPhase = finalMagazineRounds > 0 && roundsBeforeFire <= finalMagazineRounds;
			}
			isEmpowered = periodic || finalPhase;
			shotMetadata.isEmpowered = isEmpowered;
			shotMetadata.criticalPolicy = (isEmpowered && emp.guaranteedCritical)
				? DamageCriticalPolicy::Guaranteed
				: DamageCriticalPolicy::Random;
		}

		PrimaryWeaponExecutionContext runtimeContext =
			WithRuntimeState(context, state);
		runtimeContext.shotMetadata = shotMetadata;
		for (const PrimaryWeaponFeatureHandler* feature : state.features)
		{
			if (!feature->CanFire(runtimeContext, state))
			{
				return false;
			}
		}

		const bool success = state.handler->FireOnce(runtimeContext, *state.typeState);
		if (!success)
		{
			state.requestedCooldown = 0.f;
			return false;
		}
		++state.successfulFireCount;

		if (context.definition.magazine.has_value())
		{
			--state.magazineState.roundsRemaining;
			if (state.magazineState.roundsRemaining <= 0)
			{
				state.magazineState.roundsRemaining = 0;
				const float reloadDuration = CalculateReloadDuration(
					context.definition.magazine->baseReloadTime,
					context.owner
				);
				state.magazineState.reloadRemaining = static_cast<double>(reloadDuration);
			}
		}

		for (const PrimaryWeaponFeatureHandler* feature : state.features)
		{
			feature->AfterFire(runtimeContext, state);
		}
		return true;
	}

	void PrimaryWeaponExecutionSystem::TickFire(
		const PrimaryWeaponExecutionContext& context,
		PrimaryWeaponRuntimeState& state,
		float deltaTime
	)
	{
		if (!state.isInitialized || !state.isFiring ||
			!state.handler || !state.typeState)
		{
			return;
		}
		const PrimaryWeaponExecutionContext runtimeContext =
			WithRuntimeState(context, state);
		state.handler->TickFire(runtimeContext, *state.typeState, deltaTime);
		for (const PrimaryWeaponFeatureHandler* feature : state.features)
		{
			feature->TickFire(runtimeContext, state, deltaTime);
		}
	}

	PrimaryWeaponExecutionSystem::ActiveFireSimulationResult
		PrimaryWeaponExecutionSystem::SimulateActiveFire(
			const PrimaryWeaponExecutionContext& context,
			PrimaryWeaponRuntimeState& state,
			float deltaTime,
			float fireInterval,
			int maxExecutions,
			int currentExecutionCount
		)
	{
		ActiveFireSimulationResult result;
		const auto applyRequestedWeaponCooldown = [&]()
		{
			const float requestedCooldown = ConsumeRequestedCooldown(state);
			if (requestedCooldown <= 0.f)
			{
				return false;
			}
			EndFire(context, state);
			state.fireIntervalRemaining = std::max(state.fireIntervalRemaining, requestedCooldown);
			return true;
		};

		if (context.definition.magazine.has_value())
		{
			if (applyRequestedWeaponCooldown())
			{
				result.lifecycleInterrupted = true;
				return result;
			}
			if (!UsesIntervalFire(state))
			{
				TickFire(context, state, deltaTime);
				if (applyRequestedWeaponCooldown())
				{
					result.lifecycleInterrupted = true;
				}
				return result;
			}

			float availableTime = deltaTime + state.unprocessedSimulationTime;
			state.unprocessedSimulationTime = 0.f;
			constexpr float TimeEpsilon = 1e-5f;
			int catchUpExecutions = 0;

			while (true)
			{
				float timeToReady = 0.f;
				if (state.magazineState.reloadRemaining > 0.0)
				{
					timeToReady = std::max(
						std::max(0.f, state.fireIntervalRemaining),
						static_cast<float>(state.magazineState.reloadRemaining)
					);
				}
				else
				{
					timeToReady = std::max(0.f, state.fireIntervalRemaining);
				}

				if (availableTime < timeToReady - TimeEpsilon)
				{
					const float advanceTime = availableTime;
					AdvanceMagazineReload(context.definition, state, advanceTime);
					state.fireIntervalRemaining = std::max(0.f, state.fireIntervalRemaining - advanceTime);
					TickFire(context, state, advanceTime);
					availableTime = 0.f;
					break;
				}

				if (catchUpExecutions >= time::DefaultMaximumIntervalCatchUp)
				{
					state.unprocessedSimulationTime = availableTime;
					break;
				}

				const float stepTime = std::min(availableTime, timeToReady);
				if (stepTime > 0.f)
				{
					AdvanceMagazineReload(context.definition, state, stepTime);
					state.fireIntervalRemaining = std::max(0.f, state.fireIntervalRemaining - stepTime);
					TickFire(context, state, stepTime);
					availableTime -= stepTime;
					if (availableTime < TimeEpsilon)
					{
						availableTime = 0.f;
					}
				}

				if (maxExecutions > 0 &&
					(currentExecutionCount + result.executionsProduced) >= maxExecutions)
				{
					break;
				}
				if (applyRequestedWeaponCooldown())
				{
					result.lifecycleInterrupted = true;
					break;
				}

				const bool fired = FireOnce(context, state);
				if (fired)
				{
					++result.executionsProduced;
					++catchUpExecutions;
					state.fireIntervalRemaining = fireInterval;
				}
				else
				{
					if (availableTime > 0.f)
					{
						TickFire(context, state, availableTime);
						availableTime = 0.f;
					}
					state.unprocessedSimulationTime = 0.f;
					break;
				}
			}

			if (applyRequestedWeaponCooldown())
			{
				result.lifecycleInterrupted = true;
			}
			return result;
		}

		// Non-magazine timing path
		time::AdvanceIntervalDebt(state.fireIntervalRemaining, deltaTime);
		if (!state.isFiring && state.fireIntervalRemaining > 0.f)
		{
			return result;
		}
		TickFire(context, state, deltaTime);
		if (applyRequestedWeaponCooldown())
		{
			result.lifecycleInterrupted = true;
			return result;
		}
		if (!UsesIntervalFire(state))
		{
			return result;
		}
		int catchUpExecutions = 0;
		while (state.fireIntervalRemaining <= 0.f &&
			catchUpExecutions < time::DefaultMaximumIntervalCatchUp &&
			(maxExecutions <= 0 || (currentExecutionCount + result.executionsProduced) < maxExecutions))
		{
			const bool fired = FireOnce(context, state);
			if (fired)
			{
				++result.executionsProduced;
				++catchUpExecutions;
			}
			time::CommitInterval(state.fireIntervalRemaining, fireInterval);
			if (!fired)
			{
				break;
			}
		}
		if (applyRequestedWeaponCooldown())
		{
			result.lifecycleInterrupted = true;
		}
		return result;
	}

	void PrimaryWeaponExecutionSystem::TickInactive(
		const PrimaryWeaponExecutionContext& context,
		PrimaryWeaponRuntimeState& state,
		float deltaTime
	)
	{
		if (!state.isInitialized || state.isFiring || deltaTime <= 0.f)
		{
			return;
		}
		AdvanceMagazineReload(context.definition, state, deltaTime);
		state.fireIntervalRemaining = std::max(0.f, state.fireIntervalRemaining - deltaTime);
		const PrimaryWeaponExecutionContext runtimeContext =
			WithRuntimeState(context, state);
		for (const PrimaryWeaponFeatureHandler* feature : state.features)
		{
			feature->TickInactive(runtimeContext, state, deltaTime);
		}
	}

	void PrimaryWeaponExecutionSystem::AdvanceMagazineReload(const PrimaryWeaponDefinition& definition, PrimaryWeaponRuntimeState& state, float deltaTime)
	{
		if (!definition.magazine.has_value() || state.magazineState.reloadRemaining <= 0.0 || deltaTime <= 0.f) return;
		state.magazineState.reloadRemaining = std::max(
			0.0,
			state.magazineState.reloadRemaining - static_cast<double>(deltaTime)
		);
		if (state.magazineState.reloadRemaining <= 0.0)
		{
			state.magazineState.roundsRemaining = definition.magazine->capacity;
		}
	}

	void PrimaryWeaponExecutionSystem::EndFire(
		const PrimaryWeaponExecutionContext& context,
		PrimaryWeaponRuntimeState& state
	)
	{
		if (!state.isInitialized || !state.isFiring ||
			!state.handler || !state.typeState)
		{
			return;
		}
		const PrimaryWeaponExecutionContext runtimeContext =
			WithRuntimeState(context, state);
		state.handler->EndFire(runtimeContext, *state.typeState);
		for (const PrimaryWeaponFeatureHandler* feature : state.features)
		{
			feature->EndFire(runtimeContext, state);
		}
		state.isFiring = false;
	}

	float PrimaryWeaponExecutionSystem::ConsumeRequestedCooldown(
		PrimaryWeaponRuntimeState& state
	)
	{
		return state.ConsumeRequestedCooldown();
	}

	float PrimaryWeaponExecutionSystem::BuildBaseFireInterval(
		const sas::GameplayAttributeList& attributes,
		float actionInterval
	)
	{
		if (actionInterval > 0.f)
		{
			return actionInterval;
		}
		const float fireRate = std::max(
			0.01f,
			sas::FindAttributeValue(
				attributes,
				CommonAttributeIds::FireRate,
				1.f
			)
		);
		return 1.f / fireRate;
	}

	float PrimaryWeaponExecutionSystem::CalculateAttackSpeedMultiplier(const Actor& owner)
	{
		float attackSpeed = 0.f;
		if (const auto* combatant = dynamic_cast<const Combatant*>(&owner))
		{
			attackSpeed = combatant->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(
				OwnerAttributeIds::AttackSpeed
			);
		}
		return 1.f + std::max(0.f, attackSpeed) / sas::AttributeMath::PercentageRatingScale;
	}

	float PrimaryWeaponExecutionSystem::CalculateReloadDuration(
		float baseReloadTime,
		const Actor& owner
	)
	{
		const float multiplier = CalculateAttackSpeedMultiplier(owner);
		return multiplier > 0.f ? baseReloadTime / multiplier : baseReloadTime;
	}
}
