#pragma once

#include "framework/Core.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/attributes/AttributeSystem.h"
#include "gameConfigs/combat/WeaponStructs.h"
#include <SFML/Graphics/Color.hpp>
#include <string>
#include <variant>

namespace ly
{
	struct AbilityBehaviorSchema
	{
		inline static const GameplayTag Configured{ "AbilityBehavior.Configured" };
	};

	enum class AbilitySlot
	{
		None,
		PrimaryFire,
		Ability1,
		Ability2,
		Ability3,
		Ability4
	};

	struct AbilityHandle
	{
		unsigned int id = 0;

		bool IsValid() const { return id != 0; }
		bool operator==(const AbilityHandle& other) const { return id == other.id; }
		bool operator<(const AbilityHandle& other) const { return id < other.id; }
	};

	enum class AbilityActivationPolicy
	{
		OnPressed,
		WhileHeld,
		Toggle,
		Passive,
		GameplayEvent
	};

	enum class AbilityLifetimePolicy
	{
		Instant,
		Duration,
		WhileInputHeld,
		UntilCancelled
	};

	enum class AbilityActionPhase
	{
		OnActivate,
		WhileActive,
		OnEnd
	};

	enum class AbilityEndReason
	{
		Completed,
		DurationExpired,
		InputReleased,
		Cancelled,
		Interrupted,
		OwnerDestroyed
	};

	enum class AbilityTargetPolicy
	{
		Self,
		OwnerForward,
		EventTarget,
		EventSource
	};

	enum class AbilitySpawnPolicy
	{
		AtOwner,
		OwnerForward,
		AtEventTarget,
		MouseWorld
	};

	enum class AbilityDirectionPolicy
	{
		OwnerForward,
		OwnerVelocity,
		MouseWorld
	};

	struct ApplyEffectAction
	{
		std::string effectId;
		AbilityTargetPolicy targetPolicy = AbilityTargetPolicy::Self;
	};

	struct SpawnActorAction
	{
		std::string actorDefinitionId;
		AbilitySpawnPolicy spawnPolicy = AbilitySpawnPolicy::AtOwner;
		// Spawn position and launch direction are deliberately independent. A
		// projectile can start in front of its owner while still aiming at cursor.
		AbilityDirectionPolicy directionPolicy = AbilityDirectionPolicy::OwnerForward;
	};

	struct FireWeaponAction
	{
		PrimaryWeaponDefinition weaponDefinition;
	};

	struct ApplyImpulseAction
	{
		float magnitude = 0.f;
		AbilityDirectionPolicy directionPolicy = AbilityDirectionPolicy::OwnerForward;
	};

	struct EmitGameplayEventAction
	{
		GameplayTag eventTag;
		float magnitude = 0.f;
	};

	using AbilityActionData = std::variant<
		ApplyEffectAction,
		SpawnActorAction,
		FireWeaponAction,
		ApplyImpulseAction,
		EmitGameplayEventAction
	>;

	struct AbilityActionSpec
	{
		AbilityActionPhase phase = AbilityActionPhase::OnActivate;
		AbilityActionData action;
		float interval = 0.f;
		int maxExecutions = 1;
	};

	struct AbilityTriggerSpec
	{
		GameplayTag eventTag;
		float internalCooldown = 0.f;
		List<GameplayTag> requiredTags;
		List<GameplayTag> blockedTags;
		List<AbilityActionSpec> actions;
	};

	struct AbilityLevelStep
	{
		List<AttributeModifier> attributeModifiers;
		List<GameplayTag> unlockedUpgradeIds;
		List<AbilityActionSpec> addedActions;
		List<AbilityTriggerSpec> addedTriggers;
	};

	inline List<AbilityLevelStep> MakeRepeatedAbilityLevelProgression(
		std::size_t stepCount,
		const AbilityLevelStep& step
	)
	{
		return List<AbilityLevelStep>(stepCount, step);
	}

	struct AbilityDefinition
	{
		std::string abilityId;
		AbilitySlot slot = AbilitySlot::Ability1;
		AbilityActivationPolicy activationPolicy = AbilityActivationPolicy::OnPressed;
		AbilityLifetimePolicy lifetimePolicy = AbilityLifetimePolicy::Instant;
		float cooldown = 0.f;
		float duration = 0.f;
		int maxCharges = 1;
		List<GameplayTag> abilityTags;
		List<GameplayTag> requiredOwnerTags;
		List<GameplayTag> blockedOwnerTags;
		std::string displayName;
		std::string iconPath;
		std::string inputLabel;
		sf::Color accentColor = sf::Color::White;
		List<AbilityActionSpec> actions;
		List<AbilityTriggerSpec> triggers;
		List<AbilityLevelStep> levelProgression;
		// Indexed by target level minus two: [0] purchases level two.
		// Empty means this ability cannot be purchased through the run economy.
		List<unsigned int> levelUpgradeScrapCosts;
		List<AttributeModifier> attributeModifiers;
		List<AttributeScalingRule> scalingRules;
		List<GameplayTag> unlockedUpgradeIds;
		// Damage identity and attachment compatibility are independent from ability tags and level steps.
		List<GameplayTag> damageTags;
		List<GameplayTag> attachmentCapabilities;
		size_t attachmentSlotCapacity = 2;
		GameplayTag behaviorId = AbilityBehaviorSchema::Configured;

		int GetMaxLevel() const
		{
			return 1 + static_cast<int>(levelProgression.size());
		}

		bool HasScrapCostToReachLevel(int targetLevel) const
		{
			return targetLevel >= 2 &&
				targetLevel <= GetMaxLevel() &&
				levelUpgradeScrapCosts.size() == levelProgression.size() &&
				levelUpgradeScrapCosts[static_cast<size_t>(targetLevel - 2)] > 0;
		}

		unsigned int GetScrapCostToReachLevel(int targetLevel) const
		{
			return HasScrapCostToReachLevel(targetLevel)
				? levelUpgradeScrapCosts[static_cast<size_t>(targetLevel - 2)]
				: 0;
		}

		bool HasUnlockedUpgrade(const GameplayTag& upgradeId) const
		{
			for (const GameplayTag& unlockedUpgradeId : unlockedUpgradeIds)
			{
				if (unlockedUpgradeId.MatchesTag(upgradeId))
				{
					return true;
				}
			}
			return false;
		}
	};
}

namespace AbilityData
{
	ly::AbilityDefinition MakePrimaryFireAbilityDefinition(const PrimaryWeaponDefinition& weaponDefinition);
}


