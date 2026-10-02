#include "attributes/AttributeMath.h"
#include "attributes/AttributeSystem.h"
#include "framework/Actor.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/time/IntervalDebt.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"

#include "gameplay/weapon/PrimaryWeaponHandlerRegistry.h"
#include "internal/PrimaryWeaponDefinitionValidator.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <type_traits>
#include <utility>

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

		bool HasExactId(const List<sas::AttributeId>& ids, const sas::AttributeId& expectedId)
		{
			return std::find(ids.begin(), ids.end(), expectedId) != ids.end();
		}

		PrimaryWeaponExecutionContext WithRuntimeState(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state)
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
			const PrimaryWeaponValidationResult validation = PrimaryWeaponDefinitionValidator::Validate(definition);
			if (!validation.isValid)
			{
				return validation;
			}

			configuration.weaponId = definition.weaponId;
			configuration.handler = PrimaryWeaponHandlerRegistry::FindHandler(definition.weaponType);
			if (!configuration.handler)
			{
				return { false, "No runtime primary weapon handler is registered for weapon type '" + std::string{ PrimaryWeaponTypeName(definition.weaponType) } + "'." };
			}
			if (configuration.handler->GetType() != definition.weaponType)
			{
				return { false, "Runtime primary weapon handler identity does not match weapon type '" + std::string{ PrimaryWeaponTypeName(definition.weaponType) } + "'." };
			}
			configuration.magazine = definition.magazine;
			configuration.cadenceMode = definition.cadenceMode;
			configuration.damageRoundingPolicy = definition.damageRoundingPolicy;
			configuration.empoweredShot = definition.empoweredShot;
			const auto addFeature = [&](PrimaryWeaponFeatureType featureType) -> PrimaryWeaponValidationResult
			{
				const PrimaryWeaponFeatureHandler* feature = PrimaryWeaponHandlerRegistry::FindFeature(featureType);
				if (!feature)
				{
					return { false, "No runtime primary weapon feature handler is registered for feature '" + std::string{ PrimaryWeaponFeatureName(featureType) } + "'." };
				}
				if (feature->GetFeatureType() != featureType)
				{
					return { false, "Runtime primary weapon feature handler identity does not match feature '" + std::string{ PrimaryWeaponFeatureName(featureType) } + "'." };
				}
				const bool alreadyAdded = std::find(configuration.features.begin(), configuration.features.end(), feature) != configuration.features.end();
				if (!alreadyAdded)
				{
					configuration.features.push_back(feature);
				}
				return { true, {} };
			};

			for (const PrimaryWeaponFeatureType featureType : definition.featureTypes)
			{
				const PrimaryWeaponValidationResult featureResult = addFeature(featureType);
				if (!featureResult.isValid)
				{
					return featureResult;
				}
			}
			if (unlockedUpgradeIds)
			{
				for (const PrimaryWeaponLevelStep& step : definition.progressionProfile.ResolveDistinctSteps())
				{
					for (const PrimaryWeaponFeatureType featureType : step.unlockedFeatureTypes)
					{
						if (HasExactId(*unlockedUpgradeIds, PrimaryWeaponFeatureUpgradeId(featureType)))
						{
							const PrimaryWeaponValidationResult featureResult = addFeature(featureType);
							if (!featureResult.isValid)
							{
								return featureResult;
							}
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
			candidate.hitLedger = std::make_shared<PrimaryWeaponHitLedger>();
			candidate.frostMeter = std::make_shared<PrimaryWeaponFrostMeterState>();
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

	namespace
	{
		int FindIntegerAttribute(const sas::GameplayAttributeList& attributes, const sas::AttributeId& id)
		{
			return static_cast<int>(std::round(sas::FindAttributeValue(attributes, id, 0.f)));
		}

		bool IsFinalMagazinePhase(const PrimaryWeaponExecutionContext& context, const PrimaryWeaponRuntimeState& state)
		{
			if (!context.definition.magazine.has_value())
			{
				return false;
			}
			const int finalMagazineRounds = FindIntegerAttribute(
				context.attributes, PrimaryWeaponSchema::Empowered::FinalMagazineRounds);
			return finalMagazineRounds > 0 && state.magazineState.roundsRemaining <= finalMagazineRounds;
		}

		// Decides whether the shot about to fire is empowered. The only side effect
		// is refreshing ability-cast charges; consumption happens after a
		// successful fire (ConsumeEmpoweredShot).
		bool ResolveEmpoweredShot(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state)
		{
			const PrimaryWeaponEmpoweredShotDefinition& emp = *context.definition.empoweredShot;
			switch (emp.trigger)
			{
			case PrimaryWeaponEmpoweredTrigger::EverySuccessfulShots:
			{
				const int every = FindIntegerAttribute(
					context.attributes, PrimaryWeaponSchema::Empowered::EverySuccessfulShots);
				const bool periodic = every > 0 &&
					(state.successfulFireCount + 1) % static_cast<uint64_t>(every) == 0;
				return periodic || IsFinalMagazinePhase(context, state);
			}
			case PrimaryWeaponEmpoweredTrigger::EverySuccessfulHits:
			{
				const int every = FindIntegerAttribute(
					context.attributes, PrimaryWeaponSchema::Empowered::EverySuccessfulHits);
				const bool earned = every > 0 && state.hitLedger &&
					state.hitLedger->hitsSinceEmpowered >= static_cast<uint64_t>(every);
				return earned || IsFinalMagazinePhase(context, state);
			}
			case PrimaryWeaponEmpoweredTrigger::AbilityCastCharges:
			{
				if (const auto* combatant = dynamic_cast<const Combatant*>(&context.owner))
				{
					const uint64_t activations =
						combatant->GetAbilitySystemComponent().GetLoadoutAbilityActivationCount();
					if (!state.abilityActivationCountObserved)
					{
						// Casts made before this weapon existed do not charge it.
						state.observedAbilityActivationCount = activations;
						state.abilityActivationCountObserved = true;
					}
					else if (activations != state.observedAbilityActivationCount)
					{
						state.observedAbilityActivationCount = activations;
						state.empoweredCharges = std::max(0, emp.chargesPerCast);
					}
				}
				return state.empoweredCharges > 0;
			}
			case PrimaryWeaponEmpoweredTrigger::Whiteout:
				return state.featureRequestsEmpoweredShot;
			}
			return false;
		}

		void ConsumeEmpoweredShot(
			const PrimaryWeaponExecutionContext& context,
			PrimaryWeaponRuntimeState& state,
			bool wasEmpowered
		)
		{
			float chargeScale = 1.f;
			if (context.definition.empoweredShot.has_value())
			{
				const PrimaryWeaponEmpoweredShotDefinition& emp = *context.definition.empoweredShot;
				if (wasEmpowered && state.hitLedger &&
					emp.trigger == PrimaryWeaponEmpoweredTrigger::EverySuccessfulHits)
				{
					state.hitLedger->hitsSinceEmpowered = 0;
				}
				if (emp.trigger == PrimaryWeaponEmpoweredTrigger::AbilityCastCharges)
				{
					if (wasEmpowered)
					{
						state.empoweredCharges = std::max(0, state.empoweredCharges - 1);
					}
					if (state.empoweredCharges > 0 && emp.chargedFireRate > 0.f)
					{
						const float fireRate = std::max(0.01f, sas::FindAttributeValue(
							context.attributes, CommonAttributeIds::FireRate, 1.f));
						chargeScale = fireRate / emp.chargedFireRate;
					}
				}
			}
			state.nextFireIntervalScale = chargeScale * std::max(0.01f, state.featureFireIntervalScale);
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
		if (state.HasPendingEndFireCleanup())
		{
			return { false, "Primary weapon runtime has pending EndFire cleanup." };
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
		PreparedRuntimeConfiguration prepared;
		const PrimaryWeaponValidationResult validation = PrepareRuntimeConfiguration(
			definition,
			state,
			unlockedUpgradeIds,
			prepared
		);
		if (!validation.isValid)
		{
			return validation;
		}
		CommitRuntimeConfiguration(state, std::move(prepared));
		return { true, {} };
	}

	PrimaryWeaponValidationResult
		PrimaryWeaponExecutionSystem::PrepareRuntimeConfiguration(
			const PrimaryWeaponDefinition& definition,
			const PrimaryWeaponRuntimeState& state,
			const List<std::string>* unlockedUpgradeIds,
			PreparedRuntimeConfiguration& prepared
		)
	{
		prepared = PreparedRuntimeConfiguration{};
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
		if (state.HasPendingEndFireCleanup())
		{
			return { false, "Primary weapon runtime has pending EndFire cleanup." };
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
			const PrimaryWeaponValidationResult install =
				InstallRuntime(configuration, prepared.replacement);
			if (!install.isValid)
			{
				return install;
			}
			prepared.replaceRuntime = true;
			return { true, {} };
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

		prepared.featureValues =
			RetainFeatureValues(state.featureValues, configuration.features);
		prepared.features = std::move(configuration.features);
		prepared.updateFeatures = true;
		return { true, {} };
	}

	void PrimaryWeaponExecutionSystem::CommitRuntimeConfiguration(
		PrimaryWeaponRuntimeState& state,
		PreparedRuntimeConfiguration&& prepared
	) noexcept
	{
		static_assert(std::is_nothrow_move_assignable_v<PrimaryWeaponRuntimeState>);
		static_assert(std::is_nothrow_move_assignable_v<decltype(state.features)>);
		static_assert(std::is_nothrow_move_assignable_v<decltype(state.featureValues)>);
		if (prepared.replaceRuntime)
		{
			state = std::move(prepared.replacement);
		}
		else if (prepared.updateFeatures)
		{
			state.features = std::move(prepared.features);
			state.featureValues = std::move(prepared.featureValues);
		}
	}

	void PrimaryWeaponExecutionSystem::BeginFire(
		const PrimaryWeaponExecutionContext& context,
		PrimaryWeaponRuntimeState& state
	)
	{
		if (!context.ShouldContinue() || !state.isInitialized || state.isFiring ||
			!state.handler || !state.typeState)
		{
			return;
		}
		state.featureEndFirePending.assign(state.features.size(), uint8_t{ 0 });
		state.isFiring = true;
		state.handlerEndFirePending = true;
		const PrimaryWeaponExecutionContext runtimeContext =
			WithRuntimeState(context, state);
		state.handler->BeginFire(runtimeContext, *state.typeState);
		if (!runtimeContext.ShouldContinue()) return;
		for (std::size_t index = 0; index < state.features.size(); ++index)
		{
			if (!runtimeContext.ShouldContinue()) return;
			state.featureEndFirePending[index] = 1;
			const PrimaryWeaponFeatureHandler* feature = state.features[index];
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
		if (!context.ShouldContinue() || !state.isInitialized || !state.isFiring ||
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
			const bool isEmpowered = ResolveEmpoweredShot(context, state);
			shotMetadata.isEmpowered = isEmpowered;
			shotMetadata.criticalPolicy = (isEmpowered && emp.guaranteedCritical)
				? DamageCriticalPolicy::Guaranteed
				: DamageCriticalPolicy::Random;
			shotMetadata.suppressStatusStacks = emp.statusStacksOnlyWhenEmpowered && !isEmpowered;
			if (emp.trigger == PrimaryWeaponEmpoweredTrigger::EverySuccessfulHits)
			{
				shotMetadata.hitLedger = state.hitLedger;
			}
		}
		if (state.frostMeter && std::find(context.definition.featureTypes.begin(),
			context.definition.featureTypes.end(), PrimaryWeaponFeatureType::FrostMeter) !=
			context.definition.featureTypes.end())
		{
			shotMetadata.frostMeter = state.frostMeter;
			shotMetadata.firedDuringWhiteout = state.frostMeter->IsWhiteout();
		}

		PrimaryWeaponExecutionContext runtimeContext =
			WithRuntimeState(context, state);
		runtimeContext.shotMetadata = shotMetadata;
		// An empowered shot may switch damage type; every handler reads the
		// shot's tags from the context.
		if (shotMetadata.isEmpowered && !context.definition.empoweredShot->damageTags.empty())
		{
			runtimeContext.damageTags = context.definition.empoweredShot->damageTags;
		}
		for (const PrimaryWeaponFeatureHandler* feature : state.features)
		{
			if (!runtimeContext.ShouldContinue()) return false;
			if (!feature->CanFire(runtimeContext, state))
			{
				return false;
			}
		}
		if (!runtimeContext.ShouldContinue()) return false;

		const bool success = state.handler->FireOnce(runtimeContext, *state.typeState);
		if (!success)
		{
			state.requestedCooldown = 0.f;
			return false;
		}
		++state.successfulFireCount;
		ConsumeEmpoweredShot(context, state, shotMetadata.isEmpowered);

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
			if (!runtimeContext.ShouldContinue()) break;
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
		if (!context.ShouldContinue() || !state.isInitialized || !state.isFiring ||
			!state.handler || !state.typeState)
		{
			return;
		}
		const PrimaryWeaponExecutionContext runtimeContext =
			WithRuntimeState(context, state);
		state.handler->TickFire(runtimeContext, *state.typeState, deltaTime);
		for (const PrimaryWeaponFeatureHandler* feature : state.features)
		{
			if (!runtimeContext.ShouldContinue()) return;
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
		if (!context.ShouldContinue()) return result;
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

			while (context.ShouldContinue())
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
					if (!context.ShouldContinue())
					{
						if (applyRequestedWeaponCooldown()) result.lifecycleInterrupted = true;
						return result;
					}
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
					if (!context.ShouldContinue())
					{
						if (applyRequestedWeaponCooldown()) result.lifecycleInterrupted = true;
						return result;
					}
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
					state.fireIntervalRemaining = fireInterval * state.nextFireIntervalScale;
					if (!context.ShouldContinue()) break;
				}
				else
				{
					if (availableTime > 0.f && context.ShouldContinue())
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
		if (!context.ShouldContinue())
		{
			if (applyRequestedWeaponCooldown()) result.lifecycleInterrupted = true;
			return result;
		}
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
		while (context.ShouldContinue() && state.fireIntervalRemaining <= 0.f &&
			catchUpExecutions < time::DefaultMaximumIntervalCatchUp &&
			(maxExecutions <= 0 || (currentExecutionCount + result.executionsProduced) < maxExecutions))
		{
			const bool fired = FireOnce(context, state);
			if (fired)
			{
				++result.executionsProduced;
				++catchUpExecutions;
			}
			time::CommitInterval(state.fireIntervalRemaining, fireInterval * state.nextFireIntervalScale);
			if (!context.ShouldContinue()) break;
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
		if (!context.ShouldContinue() || !state.isInitialized || state.isFiring || deltaTime <= 0.f)
		{
			return;
		}
		AdvanceMagazineReload(context.definition, state, deltaTime);
		state.fireIntervalRemaining = std::max(0.f, state.fireIntervalRemaining - deltaTime);
		const PrimaryWeaponExecutionContext runtimeContext =
			WithRuntimeState(context, state);
		for (const PrimaryWeaponFeatureHandler* feature : state.features)
		{
			if (!runtimeContext.ShouldContinue()) return;
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
		if (!state.isInitialized || !state.handler || !state.typeState ||
			(!state.isFiring && !state.HasPendingEndFireCleanup()))
		{
			return;
		}
		state.isFiring = false;
		const PrimaryWeaponExecutionContext runtimeContext =
			WithRuntimeState(context, state);
		std::exception_ptr error;
		if (state.handlerEndFirePending)
		{
			try
			{
				state.handler->EndFire(runtimeContext, *state.typeState);
				state.handlerEndFirePending = false;
			}
			catch (...)
			{
				error = std::current_exception();
			}
		}
		for (std::size_t index = 0; index < state.features.size(); ++index)
		{
			if (index >= state.featureEndFirePending.size() ||
				state.featureEndFirePending[index] == 0)
			{
				continue;
			}
			try
			{
				state.features[index]->EndFire(runtimeContext, state);
				state.featureEndFirePending[index] = 0;
			}
			catch (...)
			{
				if (!error) error = std::current_exception();
			}
		}
		if (!state.HasPendingEndFireCleanup())
		{
			state.featureEndFirePending.clear();
		}
		if (error) std::rethrow_exception(error);
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
