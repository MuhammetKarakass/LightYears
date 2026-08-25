#include "gameplay/ability/chainLightning/ChainLightningAbility.h"

#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/chainLightning/ChainLightningContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/targeting/AutoTargeting.h"
#include "gameplay/targeting/TargetingTypes.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameplay/weapon/visuals/ElectricArcVisualActor.h"
#include "framework/TimerManager.h"
#include "framework/World.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/chainLightning/ChainLightningPresentationIds.h"
#include "presentation/ability/chainLightning/ChainLightningPresentationProfile.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace ly
{
	namespace
	{
		constexpr std::size_t RequiredAttributeCount = 9;
		constexpr std::size_t RequiredProgressionStepCount = 14;
		constexpr float BaseCooldown = 7.f;
		constexpr float BaseDuration = 0.f;
		constexpr float BaseDamage = 28.f;
		constexpr float BaseInitialTargetRange = 700.f;
		constexpr float BaseBounceRange = 350.f;
		constexpr float BaseBounceCount = 5.f;
		constexpr float BaseLinkTravelTime = 0.22f;
		constexpr float BaseElectricStacks = 1.f;
		constexpr float BaseElectricDamageTakenMultiplierPerStack = 0.04f;
		constexpr float BaseElectricDuration = 3.f;
		constexpr float BaseElectricMaxStacks = 4.f;
		constexpr float DamagePerLevel = 4.f;
		constexpr float CooldownPerLevel = -0.20f;
		constexpr int MaximumBonusBounces = 8;
		constexpr float Epsilon = 0.0001f;

		bool NearlyEqual(float left, float right)
		{
			return std::isfinite(left) && std::isfinite(right) &&
				std::abs(left - right) <= Epsilon;
		}

		const sas::GameplayAttribute* FindAttribute(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& attributeId
		)
		{
			return sas::FindAttribute(definition.attributes, attributeId);
		}

		bool HasValidAttribute(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& attributeId,
			float minimumBaseValue,
			bool requireWholeNumber = false
		)
		{
			const sas::GameplayAttribute* attribute = FindAttribute(
				definition,
				attributeId
			);
			if (!attribute ||
				!std::isfinite(attribute->baseValue) ||
				!std::isfinite(attribute->currentValue) ||
				!std::isfinite(attribute->minValue) ||
				!std::isfinite(attribute->maxValue) ||
				attribute->minValue > attribute->maxValue ||
				attribute->baseValue < minimumBaseValue ||
				attribute->baseValue < attribute->minValue ||
				attribute->baseValue > attribute->maxValue)
			{
				return false;
			}
			return !requireWholeNumber ||
				std::round(attribute->baseValue) == attribute->baseValue;
		}

		float FindValue(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& attributeId,
			float fallback
		)
		{
			const sas::GameplayAttribute* attribute = FindAttribute(
				definition,
				attributeId
			);
			return attribute && std::isfinite(attribute->baseValue)
				? attribute->baseValue
				: fallback;
		}

		bool HasExactAbilityTags(const GameAbilityDefinition& definition)
		{
			if (definition.abilityTags.size() != 2)
			{
				return false;
			}

			bool hasCategory = false;
			bool hasFamily = false;
			for (const GameplayTag& tag : definition.abilityTags)
			{
				hasCategory = hasCategory || tag.MatchesTagExact(
					AbilityData::ChainLightning::CategoryTag
				);
				hasFamily = hasFamily || tag.MatchesTagExact(
					AbilityData::ChainLightning::FamilyTag
				);
			}
			return hasCategory && hasFamily;
		}

		bool HasExpectedModifier(
			const AbilityLevelStep& step,
			const sas::AttributeId& attributeId,
			float expectedMagnitude
		)
		{
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
			{
				if (modifier.attributeId == attributeId &&
					modifier.operation == sas::AttributeModifierOperation::Add &&
					NearlyEqual(modifier.magnitude, expectedMagnitude))
				{
					return true;
				}
			}
			return false;
		}

		float FindOwnerLuckFactor(const Actor& owner)
		{
			const Combatant* combatant = dynamic_cast<const Combatant*>(&owner);
			return combatant
				? std::max(0.f, combatant->GetCombatRuntime().GetCombatLuckFactor())
				: 0.f;
		}

		weak_ptr<Actor> MakeWeakActor(Actor* actor)
		{
			if (!actor)
			{
				return {};
			}
			const shared_ptr<Object> object = actor->GetWeakPtr().lock();
			return object
				? std::dynamic_pointer_cast<Actor>(object)
				: weak_ptr<Actor>{};
		}
	}

	bool ChainLightningAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const bool validIdentity =
			definition.abilityId == AbilityData::ChainLightning::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::ChainLightning &&
			HasExactAbilityTags(definition);

		const bool validLifecycle =
			sas::IsLoadoutAbilitySlot(definition.slot) &&
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Instant &&
			definition.maxCharges == 1 &&
			NearlyEqual(definition.cooldown, BaseCooldown) &&
			NearlyEqual(definition.duration, BaseDuration);

		const bool validDamageIdentity =
			definition.damageTags.size() == 1 &&
			definition.damageTags.front().MatchesTagExact(
				DamageTypeSchema::Electric
			);

		const bool validAttributes =
			definition.attributes.size() == RequiredAttributeCount &&
			HasValidAttribute(
				definition,
				AbilityData::ChainLightning::Attribute::Damage,
				0.f
			) &&
			NearlyEqual(
				FindValue(
					definition,
					AbilityData::ChainLightning::Attribute::Damage,
					0.f
				),
				BaseDamage
			) &&
			HasValidAttribute(
				definition,
				AbilityData::ChainLightning::Attribute::InitialTargetRange,
				0.01f
			) &&
			NearlyEqual(
				FindValue(
					definition,
					AbilityData::ChainLightning::Attribute::InitialTargetRange,
					0.f
				),
				BaseInitialTargetRange
			) &&
			HasValidAttribute(
				definition,
				AbilityData::ChainLightning::Attribute::BounceRange,
				0.01f
			) &&
			NearlyEqual(
				FindValue(
					definition,
					AbilityData::ChainLightning::Attribute::BounceRange,
					0.f
				),
				BaseBounceRange
			) &&
			HasValidAttribute(
				definition,
				AbilityData::ChainLightning::Attribute::BaseBounceCount,
				1.f,
				true
			) &&
			NearlyEqual(
				FindValue(
					definition,
					AbilityData::ChainLightning::Attribute::BaseBounceCount,
					0.f
				),
				BaseBounceCount
			) &&
			HasValidAttribute(
				definition,
				AbilityData::ChainLightning::Attribute::LinkTravelTime,
				0.01f
			) &&
			NearlyEqual(
				FindValue(
					definition,
					AbilityData::ChainLightning::Attribute::LinkTravelTime,
					0.f
				),
				BaseLinkTravelTime
			) &&
			HasValidAttribute(
				definition,
				AbilityData::ChainLightning::Attribute::ElectricStacks,
				1.f,
				true
			) &&
			NearlyEqual(
				FindValue(
					definition,
					AbilityData::ChainLightning::Attribute::ElectricStacks,
					0.f
				),
				BaseElectricStacks
			) &&
			HasValidAttribute(
				definition,
				AbilityData::ChainLightning::Attribute::ElectricDamageTakenMultiplierPerStack,
				0.f
			) &&
			NearlyEqual(
				FindValue(
					definition,
					AbilityData::ChainLightning::Attribute::ElectricDamageTakenMultiplierPerStack,
					0.f
				),
				BaseElectricDamageTakenMultiplierPerStack
			) &&
			HasValidAttribute(
				definition,
				AbilityData::ChainLightning::Attribute::ElectricDuration,
				0.f
			) &&
			NearlyEqual(
				FindValue(
					definition,
					AbilityData::ChainLightning::Attribute::ElectricDuration,
					0.f
				),
				BaseElectricDuration
			) &&
			HasValidAttribute(
				definition,
				AbilityData::ChainLightning::Attribute::ElectricMaxStacks,
				1.f,
				true
			) &&
			NearlyEqual(
				FindValue(
					definition,
					AbilityData::ChainLightning::Attribute::ElectricMaxStacks,
					0.f
				),
				BaseElectricMaxStacks
			);

		// Chain Lightning is a behavior-owned traversal. Generic actions would
		// execute immediately and could accidentally create a second damage path.
		const bool validRuntimeOwnership =
			definition.actions.empty() &&
			definition.triggers.empty() &&
			definition.effectSpecs.empty() &&
			definition.scalingRules.empty();

		bool validProgression = definition.levelProgression.size() ==
			RequiredProgressionStepCount;
		float resolvedCooldown = definition.cooldown;
		if (validProgression)
		{
			for (const AbilityLevelStep& step : definition.levelProgression)
			{
				if (step.attributeModifiers.size() != 2 ||
					!step.unlockedUpgradeIds.empty() ||
					!step.addedActions.empty() ||
					!step.addedTriggers.empty() ||
					!HasExpectedModifier(
						step,
						CommonAttributeIds::Damage,
						DamagePerLevel
					) ||
					!HasExpectedModifier(
						step,
						CommonAttributeIds::Cooldown,
						CooldownPerLevel
					))
				{
					validProgression = false;
					break;
				}

				resolvedCooldown += CooldownPerLevel;
				if (!std::isfinite(resolvedCooldown) || resolvedCooldown <= 0.f)
				{
					validProgression = false;
					break;
				}
			}
		}

		if (!validIdentity || !validLifecycle || !validDamageIdentity ||
			!validAttributes || !validRuntimeOwnership || !validProgression)
		{
			if (failureReason)
			{
				*failureReason =
					"Chain Lightning requires its electric identity, nine declared traversal and electric payload attributes, and fourteen damage/cooldown progression steps.";
			}
			return false;
		}

		return true;
	}

	bool ChainLightningAbility::Activate(GameAbilityBehaviorContext& context)
	{
		World* world = context.owner.GetWorld();
		if (!world)
		{
			return false;
		}

		AbilityExecutionContext executionContext{
			&context.abilitySystem,
			&context.definition,
			nullptr,
			&context.instance
		};
		const sas::GameplayAttributeList values =
			AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
		const float initialRange = std::max(
			0.f,
			sas::FindAttributeValue(
				values,
				AbilityData::ChainLightning::Attribute::InitialTargetRange,
				BaseInitialTargetRange
			)
		);
		const float bounceRange = std::max(
			0.f,
			sas::FindAttributeValue(
				values,
				AbilityData::ChainLightning::Attribute::BounceRange,
				BaseBounceRange
			)
		);
		const int baseBounceCount = std::max(
			0,
			static_cast<int>(std::lround(sas::FindAttributeValue(
				values,
				AbilityData::ChainLightning::Attribute::BaseBounceCount,
				BaseBounceCount
			)))
		);
		const float linkTravelTime = std::max(
			0.001f,
			sas::FindAttributeValue(
				values,
				AbilityData::ChainLightning::Attribute::LinkTravelTime,
				BaseLinkTravelTime
			)
		);

		const CollisionLayer opposingLayer =
			context.owner.GetCollisionLayer() == CollisionLayer::Player
			? CollisionLayer::Enemy
			: context.owner.GetCollisionLayer() == CollisionLayer::Enemy
				? CollisionLayer::Player
				: CollisionLayer::None;
		if (opposingLayer == CollisionLayer::None || initialRange <= 0.f || bounceRange <= 0.f)
		{
			return false;
		}

		const sf::Vector2f mouseLocation = world->GetMouseWorldPosition();
		targeting::TargetingQuery initialQuery;
		initialQuery.source = &context.owner;
		initialQuery.origin = context.owner.GetActorLocation();
		initialQuery.range = initialRange;
		initialQuery.requiredTargetLayers = opposingLayer;
		initialQuery.requireCollisionCompatibility = true;
		initialQuery.comparator = [mouseLocation](
			const targeting::TargetingCandidate& left,
			const targeting::TargetingCandidate& right
		)
		{
			const auto distanceToMouse = [&](const shared_ptr<Actor>& actor)
			{
				if (!actor)
				{
					return std::numeric_limits<float>::max();
				}
				const sf::Vector2f delta = actor->GetActorLocation() - mouseLocation;
				return delta.x * delta.x + delta.y * delta.y;
			};
			return distanceToMouse(left.actor) < distanceToMouse(right.actor);
		};
		initialQuery.filter = [](
			const Actor*,
			const Actor& candidate,
			const targeting::TargetingCandidate&
		)
		{
			return dynamic_cast<const Combatant*>(&candidate) != nullptr;
		};

		const List<targeting::TargetingCandidate> initialTargets =
			targeting::AutoTargeting::FindTargets(*world, initialQuery);
		if (initialTargets.empty() || !initialTargets.front().actor)
		{
			LY_GAME_INFO("Chain Lightning: activation rejected because no initial target was found.");
			return false;
		}

		List<weak_ptr<Actor>> chainTargets;
		chainTargets.push_back(initialTargets.front().actor);
		std::unordered_set<Actor*> struckTargets;
		struckTargets.insert(initialTargets.front().actor.get());
		shared_ptr<Actor> currentTarget = initialTargets.front().actor;
		int remainingBaseBounces = baseBounceCount;
		int remainingBonusBounces = MaximumBonusBounces;
		const float bonusBounceChance = 0.35f * FindOwnerLuckFactor(context.owner);

		while (currentTarget)
		{
			targeting::TargetingQuery bounceQuery;
			bounceQuery.source = &context.owner;
			bounceQuery.origin = currentTarget->GetActorLocation();
			bounceQuery.range = bounceRange;
			bounceQuery.requiredTargetLayers = opposingLayer;
			bounceQuery.requireCollisionCompatibility = true;
			bounceQuery.excludeSource = true;
			bounceQuery.excludedTargets.push_back(currentTarget.get());
			for (Actor* struckTarget : struckTargets)
			{
				bounceQuery.excludedTargets.push_back(struckTarget);
			}
			bounceQuery.filter = [](
				const Actor*,
				const Actor& candidate,
				const targeting::TargetingCandidate&
			)
			{
				return dynamic_cast<const Combatant*>(&candidate) != nullptr;
			};
			bounceQuery.comparator = [&struckTargets](
				const targeting::TargetingCandidate& left,
				const targeting::TargetingCandidate& right
			)
			{
				const bool leftWasStruck = left.actor &&
					struckTargets.find(left.actor.get()) != struckTargets.end();
				const bool rightWasStruck = right.actor &&
					struckTargets.find(right.actor.get()) != struckTargets.end();
				if (leftWasStruck != rightWasStruck)
				{
					return !leftWasStruck;
				}
				return left.distanceSquared < right.distanceSquared;
			};

			const List<targeting::TargetingCandidate> nextTargets =
				targeting::AutoTargeting::FindTargets(*world, bounceQuery);
			if (nextTargets.empty() || !nextTargets.front().actor)
			{
				break;
			}

			if (remainingBaseBounces > 0)
			{
				--remainingBaseBounces;
			}
			else if (remainingBonusBounces <= 0 ||
				RandRange(0.f, 1.f) >= bonusBounceChance)
			{
				break;
			}
			else
			{
				--remainingBonusBounces;
			}

			currentTarget = nextTargets.front().actor;
			chainTargets.push_back(currentTarget);
			struckTargets.insert(currentTarget.get());
		}

		const ChainLightningPresentationProfile* profile =
			PresentationProfileRegistry<ChainLightningPresentationProfile>::Find(
				ChainLightningPresentationIds::ArcBasic
			);
		if (!profile)
		{
			LY_GAME_INFO("Chain Lightning: activation rejected because its arc presentation profile is missing.");
			return false;
		}

		const List<GameplayTag> damageTags =
			context.instance.GetResolvedDamageTags(AttachmentHostKind::Ability);
		DamagePayload payload = DamageTypeSystem::BuildPayload(damageTags, values);
		payload.electricStacks = std::max(
			1,
			static_cast<int>(std::lround(sas::FindAttributeValue(
				values,
				AbilityData::ChainLightning::Attribute::ElectricStacks,
				BaseElectricStacks
			)))
		);
		payload.electricMaxStacks = std::max(
			1,
			static_cast<int>(std::lround(sas::FindAttributeValue(
				values,
				AbilityData::ChainLightning::Attribute::ElectricMaxStacks,
				BaseElectricMaxStacks
			)))
		);
		payload.electricDamageTakenMultiplierPerStack = std::max(
			0.f,
			sas::FindAttributeValue(
				values,
				AbilityData::ChainLightning::Attribute::ElectricDamageTakenMultiplierPerStack,
				BaseElectricDamageTakenMultiplierPerStack
			)
		);
		payload.electricDuration = std::max(
			0.f,
			sas::FindAttributeValue(
				values,
				AbilityData::ChainLightning::Attribute::ElectricDuration,
				BaseElectricDuration
			)
		);

		const float damage = std::max(
			0.f,
			sas::FindAttributeValue(values, CommonAttributeIds::Damage, BaseDamage)
		);
		const weak_ptr<Actor> ownerWeak = MakeWeakActor(&context.owner);
		const weak_ptr<Object> timerOwner = context.owner.GetWeakPtr();
		const sf::Vector2f ownerLocation = context.owner.GetActorLocation();
		const sas::ContentId sourceAbilityId{ context.definition.abilityId };
		const List<GameplayTag> sourceAbilityTags = context.definition.abilityTags;
		LY_GAME_INFO(
			"Chain Lightning: scheduled %zu link(s); first link delay=%.3fs, damage=%.2f.",
			chainTargets.size(),
			linkTravelTime,
			damage
		);

		// The timer owns the delayed traversal timeline. ElectricArcVisualActor is
		// the existing, shared electric-line renderer already used by weapons;
		// reusing it keeps Chain Lightning's presentation reliable without another
		// custom world actor lifecycle.
		for (std::size_t linkIndex = 0; linkIndex < chainTargets.size(); ++linkIndex)
		{
			const weak_ptr<Actor> startActor = linkIndex == 0
				? ownerWeak
				: chainTargets[linkIndex - 1];
			const weak_ptr<Actor> targetActor = chainTargets[linkIndex];
			const float delay = linkTravelTime * static_cast<float>(linkIndex + 1);
			TimerManager::GetGameTimerManager().SetTimer(
				timerOwner,
				[
					world,
					ownerWeak,
					startActor,
					targetActor,
					ownerLocation,
					damage,
					damageTags,
					payload,
					sourceAbilityId,
					sourceAbilityTags,
					arcColor = profile->outerColor
				]()
				{
					LY_GAME_INFO("Chain Lightning: link timer callback entered.");
					const shared_ptr<Actor> owner = ownerWeak.lock();
					const shared_ptr<Actor> target = targetActor.lock();
					if (!owner || owner->GetIsPendingDestroy() || !target ||
						target->GetIsPendingDestroy())
					{
						LY_GAME_INFO("Chain Lightning: link cancelled because its owner or target no longer exists.");
						return;
					}

					const shared_ptr<Actor> start = startActor.lock();
					const sf::Vector2f startLocation = start && !start->GetIsPendingDestroy()
						? start->GetActorLocation()
						: ownerLocation;
					const sf::Vector2f targetLocation = target->GetActorLocation();
					world->SpawnActor<ElectricArcVisualActor>(
						startLocation,
						targetLocation,
						arcColor
					);
					LY_GAME_INFO("Chain Lightning: arc spawned; applying link damage.");
					ApplyCombatDamage(
						*target,
						damage,
						owner.get(),
						damageTags,
						payload,
						sourceAbilityId,
						sourceAbilityTags,
						DamageDeliveryType::Beam
					);
				},
				delay,
				false
			);
		}
		return true;
	}
}
