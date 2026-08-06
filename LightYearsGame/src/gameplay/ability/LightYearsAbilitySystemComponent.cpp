#include "attributes/AttributeSystem.h"
#include <iostream>
#include "abilities/AbilityDefinitionValidation.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/dash/DashContracts.h"
#include "gameConfigs/ability/offensive/GravityAnomalyConfig.h"
#include "gameConfigs/ability/offensive/InfernoSprayConfig.h"
#include "gameConfigs/ability/offensive/RocketConfig.h"
#include "gameplay/ability/shield/ShieldContracts.h"
#include "gameConfigs/ability/offensive/SunBeamConfig.h"
#include "gameplay/ability/dash/DashAbility.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyAbility.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyFieldActor.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyProjectileActor.h"
#include "gameplay/ability/infernoSpray/InfernoSprayAbility.h"
#include "gameplay/ability/infernoSpray/InfernoSprayActor.h"
#include "gameplay/ability/rocket/RocketAbility.h"
#include "gameplay/ability/rocket/RocketProjectileActor.h"
#include "gameplay/ability/shield/ShieldAbility.h"
#include "gameplay/ability/sunBeam/SunBeamAbility.h"
#include "gameplay/ability/sunBeam/SunBeamStrikeActor.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "effects/GameplayEffectDefinitionValidation.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/effects/content/barrier/BarrierEffectBehavior.h"
#include "gameplay/effects/gravityAnomaly/GravityAnomalyEffectBehavior.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"
#include "presentation/ability/AbilityPresentationContent.h"
#include "presentation/effects/GameplayEffectVisualRegistry.h"
#include "presentation/effects/gravityAnomaly/GravityAnomalyEffectVisualContent.h"
#include "presentation/effects/shield/ShieldVisualContent.h"

#include "framework/AssetManager.h"

#include <filesystem>
#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		bool Fail(std::string* failureReason, const std::string& reason)
		{
			if (failureReason)
			{
				*failureReason = reason;
			}
			return false;
		}

		bool ValidateWeaponActions(const List<AbilityActionSpec>& actions, std::string* failureReason)
		{
			for (const AbilityActionSpec& action : actions)
			{
				if (!std::holds_alternative<FireWeaponAction>(action.action))
				{
					continue;
				}

				const PrimaryWeaponValidationResult validation = PrimaryWeaponExecutionSystem::ValidateDefinition(
					std::get<FireWeaponAction>(action.action).weaponDefinition
				);
				if (!validation.isValid)
				{
					if (failureReason)
					{
						*failureReason = validation.reason;
					}
					return false;
				}
			}
			return true;
		}

		bool ValidateActorActions(const List<AbilityActionSpec>& actions, std::string* failureReason)
		{
			for (const AbilityActionSpec& action : actions)
			{
				if (!std::holds_alternative<SpawnActorAction>(action.action))
				{
					continue;
				}

				const SpawnActorAction& spawnAction = std::get<SpawnActorAction>(action.action);
				const AbilityActorDefinition* actorDefinition = AbilityData::FindAbilityActorDefinition(
					spawnAction.actorDefinitionId
				);
				if (!actorDefinition)
				{
					if (failureReason)
					{
						*failureReason = "Spawn actor action references an unknown ability actor definition.";
					}
					return false;
				}

				const AbilityActorValidationResult validation = AbilityActorRegistry::ValidateDefinition(*actorDefinition);
				if (!validation.isValid)
				{
					if (failureReason)
					{
						*failureReason = validation.reason;
					}
					return false;
				}
			}
			return true;
		}

		bool ValidateEffectActions(
			const List<AbilityActionSpec>& actions,
			std::string* failureReason
		)
		{
			for (const AbilityActionSpec& action : actions)
			{
				if (!std::holds_alternative<ApplyEffectAction>(action.action))
				{
					continue;
				}
				const ApplyEffectAction& effectAction =
					std::get<ApplyEffectAction>(action.action);
				const sas::GameplayEffectDefinition* definition =
					EffectData::FindGameplayEffectDefinition(effectAction.effectId);
				if (!definition)
				{
					if (failureReason)
					{
						*failureReason =
							"Apply effect action references an unknown gameplay effect definition.";
					}
					return false;
				}
				if (!LightYearsAbilitySystemComponent::
					ValidateGameplayEffectDefinition(*definition, failureReason))
				{
					return false;
				}
			}
			return true;
		}

		bool ValidateActions(const List<AbilityActionSpec>& actions, std::string* failureReason)
		{
			return ValidateWeaponActions(actions, failureReason) &&
				ValidateActorActions(actions, failureReason) &&
				ValidateEffectActions(actions, failureReason);
		}

		bool ValidateSourceEffectSpecs(
			const GameAbilityDefinition& ability,
			const List<AbilityActionSpec>& actions,
			std::string* failureReason
		)
		{
			for (const AbilityActionSpec& action : actions)
			{
				const auto* applyEffect = std::get_if<ApplyEffectAction>(&action.action);
				if (!applyEffect)
				{
					continue;
				}
				const sas::GameplayEffectDefinition* effect =
					EffectData::FindGameplayEffectDefinition(applyEffect->effectId);
				if (effect && effect->sourceParameterized &&
					!ability.FindEffectSpec(applyEffect->effectId))
				{
					if (failureReason)
					{
						*failureReason = "Ability '" + ability.abilityId +
							"' must own an effectSpecs entry for '" + applyEffect->effectId + "'.";
					}
					return false;
				}
			}
			return true;
		}

		bool ValidateOwnedEffectSpecs(
			const GameAbilityDefinition& ability,
			std::string* failureReason
		)
		{
			for (const AbilityEffectSpecDefinition& spec : ability.effectSpecs)
			{
				const sas::GameplayEffectDefinition* effect =
					EffectData::FindGameplayEffectDefinition(spec.effectId);
				if (!effect)
				{
					if (failureReason) *failureReason = "Ability effectSpecs references an unknown effect.";
					return false;
				}
				const float duration = spec.useAbilityDuration
					? ability.duration
					: spec.duration.value_or(effect->duration);
				if (effect->durationPolicy == sas::GameplayEffectDurationPolicy::Duration &&
					(!std::isfinite(duration) || duration <= 0.f))
				{
					if (failureReason) *failureReason = "Ability-owned duration must be positive for effect '" + spec.effectId + "'.";
					return false;
				}
				if (spec.maxStacks.has_value() && *spec.maxStacks < 1)
				{
					if (failureReason) *failureReason = "Ability-owned maxStacks must be at least one.";
					return false;
				}
			}
			return true;
		}

		bool ValidateLevelProgression(const GameAbilityDefinition& definition, std::string* failureReason)
		{
			if (!definition.levelUpgradeScrapCosts.empty())
			{
				if (definition.levelUpgradeScrapCosts.size() != definition.levelProgression.size())
				{
					if (failureReason)
					{
						*failureReason = "Ability upgrade scrap costs must match the number of level steps.";
					}
					return false;
				}
				for (const unsigned int cost : definition.levelUpgradeScrapCosts)
				{
					if (cost == 0)
					{
						if (failureReason)
						{
							*failureReason = "Ability upgrade scrap costs must be greater than zero.";
						}
						return false;
					}
				}
			}

			List<GameplayTag> declaredUpgradeIds;
			for (const GameplayTag& upgradeId : definition.unlockedUpgradeIds)
			{
				const bool alreadyDeclared = std::find(
					declaredUpgradeIds.begin(),
					declaredUpgradeIds.end(),
					upgradeId
				) != declaredUpgradeIds.end();
				if (!upgradeId.IsValid() || alreadyDeclared)
				{
					if (failureReason)
					{
						*failureReason = "Ability upgrade IDs must be valid and unique.";
					}
					return false;
				}
				declaredUpgradeIds.push_back(upgradeId);
			}

			for (const AbilityLevelStep& step : definition.levelProgression)
			{
				for (const sas::AttributeModifier& modifier : step.attributeModifiers)
				{
					if (!modifier.attributeId.IsValid())
					{
						if (failureReason)
						{
							*failureReason = "Ability level modifiers require a valid attribute ID.";
						}
						return false;
					}
				}

				for (const GameplayTag& upgradeId : step.unlockedUpgradeIds)
				{
					const bool alreadyDeclared = std::any_of(
						declaredUpgradeIds.begin(),
						declaredUpgradeIds.end(),
						[&](const GameplayTag& declaredUpgradeId)
						{
							return declaredUpgradeId == upgradeId;
						}
					);
					if (!upgradeId.IsValid() || alreadyDeclared)
					{
						if (failureReason)
						{
							*failureReason = "Ability level upgrade IDs must be valid and unique.";
						}
						return false;
					}
					declaredUpgradeIds.push_back(upgradeId);
				}

				if (!ValidateActions(step.addedActions, failureReason))
				{
					return false;
				}
				for (const AbilityTriggerSpec& trigger : step.addedTriggers)
				{
					if (!trigger.eventTag.IsValid())
					{
						if (failureReason)
						{
							*failureReason = "Ability level triggers require a valid event tag.";
						}
						return false;
					}
					if (!ValidateActions(trigger.actions, failureReason))
					{
						return false;
					}
				}
			}
			return true;
		}
	}

	LightYearsAbilitySystemComponent::EffectBehaviorRuntime&
	LightYearsAbilitySystemComponent::GetEffectBehaviorRuntime()
	{
		static EffectBehaviorRuntime runtime;
		return runtime;
	}

	void LightYearsAbilitySystemComponent::InitializeOwnerAttributes(
		float maxHealth
	)
	{
		sas::AttributeSystem& attributes = GetAttributes();
		attributes.RegisterAttribute(OwnerAttributeIds::MaxHealth, maxHealth);
		attributes.RegisterAttribute(OwnerAttributeIds::HealthRegen, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::EnergyMax, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::EnergyRegen, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::AttackPower, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::AttackSpeed, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::AbilityHaste, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::MoveSpeedHorizontal, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::MoveSpeedVertical, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::Armor, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::Luck, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::CriticalChance, 0.f);
	}

	bool LightYearsAbilitySystemComponent::RegisterGameContent()
	{
		static const bool registered = []
		{
			std::filesystem::path assetRoot =
				AssetManager::GetAssetManager().GetAssetRootDirectory();
			if (assetRoot.empty())
			{
				assetRoot = "LightYearsGame/assets";
			}

			std::string weaponLoadFailureReason;
			const bool weaponsLoaded = content::WeaponContentCatalog::LoadFromFile(
				assetRoot / "content/data/weapons.json",
				&weaponLoadFailureReason
			);
			if (!weaponsLoaded)
			{
				LY_GAME_ERROR(
					"Failed to load weapon content: %s",
					weaponLoadFailureReason.c_str()
				);
			}

			std::string shipLoadFailureReason;
			const bool shipsLoaded = content::ShipContentCatalog::LoadFromFile(
				assetRoot / "content/data/ships.json",
				&shipLoadFailureReason
			);
			if (!shipsLoaded)
			{
				LY_GAME_ERROR(
					"Failed to load ship content: %s",
					shipLoadFailureReason.c_str()
				);
			}

			std::string abilityLoadFailureReason;
			const bool abilitiesLoaded = content::AbilityContentCatalog::LoadFromFile(
				assetRoot / "content/data/abilities.json",
				AbilityData::GetBuiltinShippedAbilityDefinitions(),
				AbilityData::GetBuiltinAbilityActorDefinitions(),
				&abilityLoadFailureReason
			);
			if (!abilitiesLoaded)
			{
				LY_GAME_ERROR(
					"Failed to load ability content: %s",
					abilityLoadFailureReason.c_str()
				);
			}

			std::string effectLoadFailureReason;
			const bool effectsLoaded = content::EffectContentCatalog::LoadFromFile(
				assetRoot / "content/data/effects.json",
				EffectData::GetBuiltinGameplayEffectDefinitions(),
				&effectLoadFailureReason
			);
			if (!effectsLoaded)
			{
				LY_GAME_ERROR(
					"Failed to load gameplay effect content: %s",
					effectLoadFailureReason.c_str()
				);
			}

			const bool presentationRegistered =
				RegisterGameAbilityPresentationContent();
			const bool configuredRegistered =
				GameAbilityBehaviorRegistry::Register(
					AbilityBehaviorSchema::Configured,
					[] { return std::make_unique<GameAbilityBehavior>(); }
				);
			const bool abilitiesRegistered =
				configuredRegistered &&
				GameAbilityBehaviorRegistry::Register(
					AbilityData::Dash::BehaviorId,
					[] { return std::make_unique<DashAbility>(); }
				) &&
				GameAbilityBehaviorRegistry::Register(
					AbilityData::Shield::BehaviorId,
					[] { return std::make_unique<ShieldAbility>(); }
				) &&
				GameAbilityBehaviorRegistry::Register(
					AbilityData::GravityAnomaly::BehaviorId,
					[] { return std::make_unique<GravityAnomalyAbility>(); }
				) &&
				GameAbilityBehaviorRegistry::Register(
					AbilityData::Rocket::BehaviorId,
					[] { return std::make_unique<RocketAbility>(); }
				) &&
				GameAbilityBehaviorRegistry::Register(
					AbilityData::SunBeam::BehaviorId,
					[] { return std::make_unique<SunBeamAbility>(); }
				) &&
				GameAbilityBehaviorRegistry::Register(
					AbilityData::InfernoSpray::BehaviorId,
					[] { return std::make_unique<InfernoSprayAbility>(); }
				);

			const bool abilityActorsRegistered =
				RegisterGravityAnomalyProjectileActorType() &&
				RegisterGravityAnomalyFieldActorType() &&
				RegisterRocketProjectileActorType() &&
				RegisterSunBeamStrikeActorType() &&
				RegisterInfernoSprayActorType();

			const bool effectsRegistered =
				RegisterGravityAnomalyEffectVisuals() &&
				RegisterShieldVisuals() &&
				BarrierEffectBehavior::RegisterBarrierEffectBehavior() &&
				DamageTypeSystem::RegisterDamageEffectBehaviors() &&
				GravityAnomalyEffectBehavior::
					RegisterGravityAnomalyEffectBehavior();
			std::string abilityValidationFailureReason;
			const bool abilitiesValidated =
				abilitiesRegistered &&
				LightYearsAbilitySystemComponent::
					ValidateShippedAbilityDefinitions(&abilityValidationFailureReason);
			if (!abilitiesValidated && !abilityValidationFailureReason.empty())
			{
				LY_GAME_ERROR(
					"Shipped ability validation failed: %s",
					abilityValidationFailureReason.c_str()
				);
			}

			std::string effectValidationFailureReason;
			const bool effectsValidated =
				LightYearsAbilitySystemComponent::
				ValidateShippedGameplayEffectDefinitions(&effectValidationFailureReason);
			if (!effectsValidated && !effectValidationFailureReason.empty())
			{
				LY_GAME_ERROR(
					"Shipped gameplay effect validation failed: %s",
					effectValidationFailureReason.c_str()
				);
			}

			return weaponsLoaded &&
				shipsLoaded &&
				abilitiesLoaded &&
				effectsLoaded &&
				presentationRegistered &&
				abilitiesValidated &&
				abilityActorsRegistered &&
				effectsRegistered &&
				effectsValidated;
		}();
		return registered;
	}

	bool LightYearsAbilitySystemComponent::ValidateGameplayEffectDefinition(
		const sas::GameplayEffectDefinition& definition,
		std::string* failureReason
	)
	{
		if (!sas::ValidateGameplayEffectDefinition(definition, failureReason))
		{
			return false;
		}
		if (definition.behaviorTag.IsValid() &&
			!GetEffectBehaviorRuntime().IsRegistered(
				definition.behaviorTag
			))
		{
			return Fail(
				failureReason,
				"Gameplay effect '" + definition.effectId +
					"' references an unregistered behavior."
			);
		}
		if (!definition.activeVisualId.empty() &&
			!GameplayEffectVisualRegistry::IsRegistered(
				definition.activeVisualId
			))
		{
			return Fail(
				failureReason,
				"Gameplay effect '" + definition.effectId +
					"' references an unregistered visual."
			);
		}
		return true;
	}

	bool LightYearsAbilitySystemComponent::
		ValidateShippedGameplayEffectDefinitions(std::string* failureReason)
	{
		const List<const sas::GameplayEffectDefinition*>& definitions =
			EffectData::GetShippedGameplayEffectDefinitions();
		if (!sas::ValidateGameplayEffectDefinitionCatalog(
			definitions,
			failureReason
		))
		{
			return false;
		}
		for (const sas::GameplayEffectDefinition* definition : definitions)
		{
			if (!definition ||
				!ValidateGameplayEffectDefinition(
					*definition,
					failureReason
				))
			{
				return false;
			}
		}
		return true;
	}

	bool LightYearsAbilitySystemComponent::ValidateDefinition(const GameAbilityDefinition& definition, std::string* failureReason)
	{
		if (!sas::ValidateAbilityDefinition(definition, failureReason))
		{
			return false;
		}
		if (!ValidateActions(definition.actions, failureReason))
		{
			return false;
		}
		if (!ValidateOwnedEffectSpecs(definition, failureReason) ||
			!ValidateSourceEffectSpecs(definition, definition.actions, failureReason))
		{
			return false;
		}
		for (const AbilityTriggerSpec& trigger : definition.triggers)
		{
			if (!trigger.eventTag.IsValid() || !ValidateActions(trigger.actions, failureReason))
			{
				if (failureReason && failureReason->empty())
				{
					*failureReason = "Ability triggers require a valid event tag.";
				}
				return false;
			}
			if (!ValidateSourceEffectSpecs(definition, trigger.actions, failureReason))
			{
				return false;
			}
		}
		if (!ValidateLevelProgression(definition, failureReason))
		{
			return false;
		}

		unique_ptr<GameAbilityBehavior> behavior = GameAbilityBehaviorRegistry::Create(definition.behaviorId);
		if (!behavior)
		{
			if (failureReason)
			{
				*failureReason = "Ability definition references an unregistered behavior.";
			}
			return false;
		}
		if (!behavior->Validate(definition, failureReason))
		{
			if (failureReason && failureReason->empty())
			{
				*failureReason = "Ability behavior rejected its definition.";
			}
			return false;
		}
		return true;
	}

	bool LightYearsAbilitySystemComponent::ValidateCatalog(
		const List<const GameAbilityDefinition*>& definitions,
		std::string* failureReason)
	{
		return sas::ValidateAbilityDefinitionCatalog(
			definitions,
			[](const GameAbilityDefinition& definition,
				std::string* validationFailure)
			{
				return ValidateDefinition(
					definition,
					validationFailure
				);
			},
			failureReason
		);
	}

	bool LightYearsAbilitySystemComponent::ValidateShippedAbilityDefinitions(
		std::string* failureReason
	)
	{
		return ValidateCatalog(
			AbilityData::GetShippedAbilityDefinitions(),
			failureReason
		);
	}

	LightYearsAbilitySystemComponent::LightYearsAbilitySystemComponent(
		Actor& owner
	)
		: mOwner{ owner },
		mComponentRuntime{
			InitializeAbilitySystem<
				GameAbilityDefinition,
				GameAbility
			>(MaxPassiveAbilities)
		}
	{
		sas::AbilityRuntimeSystem<
			GameAbilityDefinition,
			GameAbility
		>::Callbacks callbacks;
		callbacks.validate =
			[](const GameAbilityDefinition& definition, std::string* failureReason)
			{
				return ValidateDefinition(definition, failureReason);
			};
		callbacks.create =
			[this](
				sas::AbilityHandle handle,
				const GameAbilityDefinition& definition,
				std::string* failureReason
			) -> unique_ptr<GameAbility>
			{
				unique_ptr<GameAbilityBehavior> behavior =
					GameAbilityBehaviorRegistry::Create(definition.behaviorId);
				if (!behavior)
				{
					if (failureReason)
					{
						*failureReason = "Ability behavior could not be created.";
					}
					return {};
				}
				return std::make_unique<GameAbility>(
					*this,
					handle,
					definition,
					std::move(behavior)
				);
			};
		callbacks.cancel =
			[](GameAbility& ability, sas::AbilityEndReason reason)
			{
				ability.Cancel(reason);
			};
		callbacks.tick =
			[](GameAbility& ability, float deltaTime)
			{
				ability.Tick(deltaTime);
			};
		callbacks.setInput =
			[](GameAbility& ability, bool inputHeld)
			{
				ability.SetInputHeld(inputHeld);
			};
		callbacks.snapshot =
			[](const GameAbility& ability)
			{
				return ability.BuildSnapshot();
			};
		ConfigureAbilityRuntime(
			mComponentRuntime,
			std::move(callbacks)
		);
		SetGameplayEventHandler<sas::AbilityEvent>(
			[this](const sas::AbilityEvent& event)
			{
				ProcessGameGameplayEvent(event);
			}
		);
	}

	bool LightYearsAbilitySystemComponent::TryEquipAttachment(
		sas::AbilityHandle handle,
		const AttachmentDefinition& definition,
		AttachmentHostKind hostKind,
		std::string* failureReason
	)
	{
		GameAbility* ability = GetAbility(handle);
		if (!ability)
		{
			if (failureReason)
			{
				*failureReason = "Attachment host ability was not found.";
			}
			return false;
		}
		return ability->TryEquipAttachment(definition, hostKind, failureReason);
	}

	bool LightYearsAbilitySystemComponent::TryEquipAttachment(
		sas::AbilitySlot slot,
		const AttachmentDefinition& definition,
		AttachmentHostKind hostKind,
		std::string* failureReason
	)
	{
		GameAbility* ability = GetAbility(slot);
		if (!ability)
		{
			if (failureReason)
			{
				*failureReason = "Attachment host ability was not found.";
			}
			return false;
		}
		return ability->TryEquipAttachment(definition, hostKind, failureReason);
	}

	bool LightYearsAbilitySystemComponent::RemoveAttachment(
		sas::AbilityHandle handle,
		const GameplayTag& attachmentId,
		AttachmentHostKind hostKind
	)
	{
		GameAbility* ability = GetAbility(handle);
		return ability && ability->RemoveAttachment(attachmentId, hostKind);
	}

	GameAbility* LightYearsAbilitySystemComponent::GetAbility(sas::AbilitySlot slot)
	{
		return FindAbility<GameAbility>(slot);
	}

	const GameAbility* LightYearsAbilitySystemComponent::GetAbility(sas::AbilitySlot slot) const
	{
		return FindAbility<GameAbility>(slot);
	}

	GameAbility* LightYearsAbilitySystemComponent::GetAbility(sas::AbilityHandle handle)
	{
		return FindAbility<GameAbility>(handle);
	}

	const GameAbility* LightYearsAbilitySystemComponent::GetAbility(sas::AbilityHandle handle) const
	{
		return FindAbility<GameAbility>(handle);
	}

	GameAbility* LightYearsAbilitySystemComponent::GetAbilityById(const std::string& abilityId)
	{
		return FindAbilityById<GameAbility>(abilityId);
	}

	const GameAbility* LightYearsAbilitySystemComponent::GetAbilityById(const std::string& abilityId) const
	{
		return FindAbilityById<GameAbility>(abilityId);
	}

	void LightYearsAbilitySystemComponent::ProcessGameGameplayEvent(const sas::AbilityEvent& event)
	{
		mComponentRuntime.HandleGameplayEvent(
			event,
			GetOwnedTags(),
			[](GameAbility& ability, const sas::AbilityEvent& abilityEvent)
			{
				ability.HandleAttachmentEvent(abilityEvent);
			},
			[this](
				GameAbility& ability,
				const GameAbilityDefinition& definition,
				const AbilityTriggerSpec& trigger,
				const sas::AbilityEvent& abilityEvent
			)
			{
				GameAbilityDefinition triggerDefinition = definition;
				triggerDefinition.actions = trigger.actions;

				GameAbilityExecution execution;
				AbilityExecutionContext context{
					this,
					&triggerDefinition,
					&abilityEvent,
					&ability
				};
				GameAbilityActionExecutor::BeginExecution(execution, context);
				GameAbilityActionExecutor::TickExecution(execution, context, 0.f);
				GameAbilityActionExecutor::EndExecution(
					execution,
					context,
					sas::AbilityEndReason::Completed
				);
			}
		);
	}

}
