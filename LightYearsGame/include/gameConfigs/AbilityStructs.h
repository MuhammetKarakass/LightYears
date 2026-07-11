#pragma once

#include "framework/Core.h"
#include "gameConfigs/AbilityActorStructs.h"
#include "gameplay/attributes/AttributeSystem.h"
#include "gameConfigs/WeaponStructs.h"
#include <SFML/Graphics/Color.hpp>
#include <string>
#include <variant>

namespace ly
{
	// High-level ability families. Concrete definitions still use AbilityDefinition.
	struct AbilitySchema
	{
		struct Shield
		{
			inline static const GameplayTag FamilyTag{ "Ability.Defense.Shield" };
		};

		struct SunBeam
		{
			inline static const GameplayTag FamilyTag{ "Ability.Offense.SunBeam" };
		};
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
		List<AttributeModifier> modifiers;
	};

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
		List<AttributeModifier> attributeModifiers;
		List<AttributeScalingRule> scalingRules;
	};
}

namespace AbilityData
{
	ly::AbilityDefinition MakePrimaryFireAbilityDefinition(const PrimaryWeaponDefinition& weaponDefinition);
}


