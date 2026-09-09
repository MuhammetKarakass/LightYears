#include "gameplay/ability/stormMark/StormMarkAbility.h"

#include "attributes/AttributeSystem.h"
#include "framework/TimerManager.h"
#include "framework/World.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/stormMark/StormMarkContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/targeting/AutoTargeting.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameplay/weapon/visuals/ElectricArcVisualActor.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/stormMark/StormMarkPresentationIds.h"
#include "presentation/ability/stormMark/StormMarkPresentationProfile.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>

namespace ly
{
	namespace
	{
		constexpr std::size_t RequiredAttributeCount = 10;
		constexpr std::size_t RequiredProgressionStepCount = 14;
		constexpr float BaseCooldown = 9.f;
		constexpr float BaseDuration = 0.f;
		constexpr float BaseDamage = 25.f;
		constexpr float BaseSearchRadius = 600.f;
		constexpr float BaseTargetCount = 4.f;
		constexpr float BaseFocusDuration = 0.30f;
		constexpr float BaseStrikeDuration = 0.20f;
		constexpr float BaseLuckPerExtraTarget = 50.f;
		constexpr float BaseElectricStacks = 1.f;
		constexpr float BaseElectricDamageTakenMultiplierPerStack = 0.04f;
		constexpr float BaseElectricDuration = 3.f;
		constexpr float BaseElectricMaxStacks = 4.f;
		constexpr float DamagePerLevel = 4.f;
		constexpr float CooldownPerLevel = -0.20f;
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
					AbilityData::StormMark::CategoryTag
				);
				hasFamily = hasFamily || tag.MatchesTagExact(
					AbilityData::StormMark::FamilyTag
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
			return std::any_of(
				step.attributeModifiers.begin(),
				step.attributeModifiers.end(),
				[&](const sas::AttributeModifier& modifier)
				{
					return modifier.attributeId == attributeId &&
						modifier.operation == sas::AttributeModifierOperation::Add &&
						NearlyEqual(modifier.magnitude, expectedMagnitude);
				}
			);
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

		CollisionLayer GetOpposingLayer(const Actor& owner)
		{
			if (owner.GetCollisionLayer() == CollisionLayer::Player)
			{
				return CollisionLayer::Enemy;
			}
			if (owner.GetCollisionLayer() == CollisionLayer::Enemy)
			{
				return CollisionLayer::Player;
			}
			return CollisionLayer::None;
		}

		int ResolveTargetLimit(
			const sas::GameplayAttributeList& values,
			const sas::AbilitySystemComponent& ownerAbilitySystem
		)
		{
			const int baseTargetCount = std::max(
				1,
				static_cast<int>(std::lround(sas::FindAttributeValue(
					values,
					AbilityData::StormMark::Attribute::BaseTargetCount,
					BaseTargetCount
				)))
			);
			const float luckPerExtraTarget = std::max(
				0.01f,
				sas::FindAttributeValue(
					values,
					AbilityData::StormMark::Attribute::LuckPerExtraTarget,
					BaseLuckPerExtraTarget
				)
			);
			const float luckRating = std::max(
				0.f,
				ownerAbilitySystem.GetAttributes().GetCurrentValue(
					OwnerAttributeIds::Luck
				)
			);
			const float expectedExtraTargets = luckRating / luckPerExtraTarget;
			const float safeExtraTargets = std::min(
				std::max(0.f, expectedExtraTargets),
				static_cast<float>(std::numeric_limits<int>::max() - baseTargetCount)
			);
			const int guaranteedExtraTargets = static_cast<int>(
				std::floor(safeExtraTargets)
			);
			const float fractionalExtraTarget = safeExtraTargets -
				static_cast<float>(guaranteedExtraTargets);
			const int randomExtraTarget = RandRange(0.f, 1.f) <
				fractionalExtraTarget ? 1 : 0;
			return baseTargetCount + guaranteedExtraTargets + randomExtraTarget;
		}
	}

	bool StormMarkAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const bool validIdentity =
			definition.abilityId == AbilityData::StormMark::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::StormMark &&
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
			definition.damageTags.front().MatchesTagExact(DamageTypeSchema::Electric);

		const bool validAttributes =
			definition.attributes.size() == RequiredAttributeCount &&
			HasValidAttribute(definition, CommonAttributeIds::Damage, 0.f) &&
			NearlyEqual(FindValue(definition, CommonAttributeIds::Damage, 0.f), BaseDamage) &&
			HasValidAttribute(definition, CommonAttributeIds::Range, 0.01f) &&
			NearlyEqual(FindValue(definition, CommonAttributeIds::Range, 0.f), BaseSearchRadius) &&
			HasValidAttribute(
				definition,
				AbilityData::StormMark::Attribute::BaseTargetCount,
				1.f,
				true
			) &&
			NearlyEqual(
				FindValue(
					definition,
					AbilityData::StormMark::Attribute::BaseTargetCount,
					0.f
				),
				BaseTargetCount
			) &&
			HasValidAttribute(
				definition,
				AbilityData::StormMark::Attribute::FocusDuration,
				0.01f
			) &&
			NearlyEqual(
				FindValue(
					definition,
					AbilityData::StormMark::Attribute::FocusDuration,
					0.f
				),
				BaseFocusDuration
			) &&
			HasValidAttribute(
				definition,
				AbilityData::StormMark::Attribute::StrikeDuration,
				0.01f
			) &&
			NearlyEqual(
				FindValue(
					definition,
					AbilityData::StormMark::Attribute::StrikeDuration,
					0.f
				),
				BaseStrikeDuration
			) &&
			HasValidAttribute(
				definition,
				AbilityData::StormMark::Attribute::LuckPerExtraTarget,
				0.01f
			) &&
			NearlyEqual(
				FindValue(
					definition,
					AbilityData::StormMark::Attribute::LuckPerExtraTarget,
					0.f
				),
				BaseLuckPerExtraTarget
			) &&
			HasValidAttribute(
				definition,
				AbilityData::StormMark::Attribute::ElectricStacks,
				1.f,
				true
			) &&
			NearlyEqual(
				FindValue(
					definition,
					AbilityData::StormMark::Attribute::ElectricStacks,
					0.f
				),
				BaseElectricStacks
			) &&
			HasValidAttribute(
				definition,
				AbilityData::StormMark::Attribute::ElectricDamageTakenMultiplierPerStack,
				0.f
			) &&
			NearlyEqual(
				FindValue(
					definition,
					AbilityData::StormMark::Attribute::ElectricDamageTakenMultiplierPerStack,
					0.f
				),
				BaseElectricDamageTakenMultiplierPerStack
			) &&
			HasValidAttribute(
				definition,
				AbilityData::StormMark::Attribute::ElectricDuration,
				0.f
			) &&
			NearlyEqual(
				FindValue(
					definition,
					AbilityData::StormMark::Attribute::ElectricDuration,
					0.f
				),
				BaseElectricDuration
			) &&
			HasValidAttribute(
				definition,
				AbilityData::StormMark::Attribute::ElectricMaxStacks,
				1.f,
				true
			) &&
			NearlyEqual(
				FindValue(
					definition,
					AbilityData::StormMark::Attribute::ElectricMaxStacks,
					0.f
				),
				BaseElectricMaxStacks
			);

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
					!HasExpectedModifier(step, CommonAttributeIds::Damage, DamagePerLevel) ||
					!HasExpectedModifier(step, CommonAttributeIds::Cooldown, CooldownPerLevel))
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

		const bool validScaling = definition.scalingRules.size() == 1 &&
			definition.scalingRules.front().targetAttributeId == CommonAttributeIds::Damage &&
			definition.scalingRules.front().sourceAttributeId == OwnerAttributeIds::EnergyMax &&
			definition.scalingRules.front().operation == sas::AttributeModifierOperation::Add &&
			NearlyEqual(definition.scalingRules.front().coefficient, 0.25f);

		// Target selection, strike timing and status application are owned by this
		// behavior. Generic actions would fire immediately and break the focus
		// window or apply a second damage path.
		const bool validRuntimeOwnership =
			definition.actions.empty() &&
			definition.triggers.empty() &&
			definition.effectSpecs.empty();

		if (!validIdentity || !validLifecycle || !validDamageIdentity ||
			!validAttributes || !validProgression || !validScaling ||
			!validRuntimeOwnership)
		{
			if (failureReason)
			{
				*failureReason =
					"Storm Mark requires its electric identity, ten target/timing/status attributes, EnergyMax damage scaling, and fourteen damage/cooldown progression steps.";
			}
			return false;
		}
		return true;
	}

	bool StormMarkAbility::Activate(GameAbilityBehaviorContext& context)
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
		const float searchRadius = std::max(
			0.f,
			sas::FindAttributeValue(
				values,
				AbilityData::StormMark::Attribute::SearchRadius,
				BaseSearchRadius
			)
		);
		const float focusDuration = std::max(
			0.001f,
			sas::FindAttributeValue(
				values,
				AbilityData::StormMark::Attribute::FocusDuration,
				BaseFocusDuration
			)
		);
		const float strikeDuration = std::max(
			0.f,
			sas::FindAttributeValue(
				values,
				AbilityData::StormMark::Attribute::StrikeDuration,
				BaseStrikeDuration
			)
		);
		const float damage = std::max(
			0.f,
			sas::FindAttributeValue(
				values,
				AbilityData::StormMark::Attribute::Damage,
				BaseDamage
			)
		);
		if (searchRadius <= 0.f || damage <= 0.f)
		{
			return true;
		}

		const int targetLimit = ResolveTargetLimit(values, context.abilitySystem);
		const List<GameplayTag> damageTags =
			context.instance.GetResolvedDamageTags(AttachmentHostKind::Ability);
		DamagePayload payload = DamageTypeSystem::BuildPayload(damageTags, values);
		payload.electricStacks = std::max(
			1,
			static_cast<int>(std::lround(sas::FindAttributeValue(
				values,
				AbilityData::StormMark::Attribute::ElectricStacks,
				BaseElectricStacks
			)))
		);
		payload.electricDamageTakenMultiplierPerStack = std::max(
			0.f,
			sas::FindAttributeValue(
				values,
				AbilityData::StormMark::Attribute::ElectricDamageTakenMultiplierPerStack,
				BaseElectricDamageTakenMultiplierPerStack
			)
		);
		payload.electricDuration = std::max(
			0.f,
			sas::FindAttributeValue(
				values,
				AbilityData::StormMark::Attribute::ElectricDuration,
				BaseElectricDuration
			)
		);
		payload.electricMaxStacks = std::max(
			1,
			static_cast<int>(std::lround(sas::FindAttributeValue(
				values,
				AbilityData::StormMark::Attribute::ElectricMaxStacks,
				BaseElectricMaxStacks
			)))
		);

		const StormMarkPresentationProfile* presentationProfile =
			PresentationProfileRegistry<StormMarkPresentationProfile>::Find(
				StormMarkPresentationIds::LightningBasic
			);
		if (!presentationProfile)
		{
			return false;
		}

		const weak_ptr<Actor> ownerWeak = MakeWeakActor(&context.owner);
		const weak_ptr<Object> timerOwner = context.owner.GetWeakPtr();
		const sas::ContentId sourceAbilityId{ context.definition.abilityId };
		const List<GameplayTag> sourceAbilityTags = context.definition.abilityTags;
		const sf::Color lightningColor = presentationProfile->lightningColor;
		const float strikeHeight = std::max(1.f, presentationProfile->strikeHeight);
		const int safeTargetLimit = std::max(1, targetLimit);

		TimerManager::GetGameTimerManager().SetTimer(
			timerOwner,
			[
				world,
				ownerWeak,
				timerOwner,
				searchRadius,
				safeTargetLimit,
				strikeDuration,
				focusDuration,
				damage,
				payload,
				damageTags,
				sourceAbilityId,
				sourceAbilityTags,
				lightningColor,
				strikeHeight
			]()
			{
				const shared_ptr<Actor> owner = ownerWeak.lock();
				if (!owner || owner->GetIsPendingDestroy() || !world)
				{
					return;
				}

				const CollisionLayer opposingLayer = GetOpposingLayer(*owner);
				if (opposingLayer == CollisionLayer::None)
				{
					return;
				}

				targeting::TargetingQuery query;
				query.source = owner.get();
				query.origin = owner->GetActorLocation();
				query.range = searchRadius;
				query.shape = targeting::TargetingShape::Radius;
				query.maxTargets = static_cast<std::size_t>(safeTargetLimit);
				query.requiredTargetLayers = opposingLayer;
				query.requireCollisionCompatibility = true;
				query.filter = [](
					const Actor*,
					const Actor& candidate,
					const targeting::TargetingCandidate&
				)
				{
					// Combatant is the shared boundary for live ships. Projectiles,
					// pickups, fields and other world actors are intentionally ignored.
					return dynamic_cast<const Combatant*>(&candidate) != nullptr;
				};

				const List<targeting::TargetingCandidate> targets =
					targeting::AutoTargeting::FindTargets(*world, query);
				if (targets.empty())
				{
					return;
				}

				const std::size_t targetCount = targets.size();
				for (std::size_t index = 0; index < targetCount; ++index)
				{
					const shared_ptr<Actor>& target = targets[index].actor;
					if (!target)
					{
						continue;
					}

					const float normalizedIndex = targetCount <= 1
						? 0.f
						: static_cast<float>(index) /
							static_cast<float>(targetCount - 1);
					const float strikeDelay = focusDuration + strikeDuration * normalizedIndex;
					const weak_ptr<Actor> targetWeak = target;
					TimerManager::GetGameTimerManager().SetTimer(
						timerOwner,
						[
								world,
								ownerWeak,
								targetWeak,
								damage,
								payload,
								damageTags,
								sourceAbilityId,
								sourceAbilityTags,
								lightningColor,
								strikeHeight
						]()
						{
							const shared_ptr<Actor> owner = ownerWeak.lock();
							const shared_ptr<Actor> target = targetWeak.lock();
							if (!world || !owner || owner->GetIsPendingDestroy() ||
								!target || target->GetIsPendingDestroy() ||
								dynamic_cast<Combatant*>(target.get()) == nullptr)
							{
								return;
							}

							const sf::Vector2f targetLocation = target->GetActorLocation();
							world->SpawnActor<ElectricArcVisualActor>(
								targetLocation + sf::Vector2f{ 0.f, -strikeHeight },
								targetLocation,
								lightningColor
							);
							ApplyCombatDamage(
								*target,
								damage,
								owner.get(),
								damageTags,
								payload,
								sourceAbilityId,
								sourceAbilityTags,
								DamageDeliveryType::Direct
							);
						},
						strikeDelay,
						false
					);
				}
			},
			focusDuration,
			false
		);
		return true;
	}
}
