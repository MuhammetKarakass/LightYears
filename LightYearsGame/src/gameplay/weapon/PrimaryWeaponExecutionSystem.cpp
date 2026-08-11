#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"

#include "gameplay/weapon/PrimaryWeaponHandlerRegistry.h"
#include "internal/PrimaryWeaponDefinitionValidator.h"

#include <algorithm>

namespace ly
{
	namespace
	{
		struct RuntimeConfiguration
		{
			std::string weaponId;
			const PrimaryWeaponHandler* handler = nullptr;
			List<const PrimaryWeaponFeatureHandler*> features;
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
		const PrimaryWeaponExecutionContext runtimeContext =
			WithRuntimeState(context, state);
		for (const PrimaryWeaponFeatureHandler* feature : state.features)
		{
			if (!feature->CanFire(runtimeContext, state))
			{
				return false;
			}
		}

		state.handler->FireOnce(runtimeContext, *state.typeState);
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
		const PrimaryWeaponExecutionContext runtimeContext =
			WithRuntimeState(context, state);
		for (const PrimaryWeaponFeatureHandler* feature : state.features)
		{
			feature->TickInactive(runtimeContext, state, deltaTime);
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
}
