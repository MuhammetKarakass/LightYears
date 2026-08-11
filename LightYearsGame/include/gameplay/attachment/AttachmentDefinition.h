#pragma once

#include "attributes/AttributeSystem.h"
#include "abilities/AbilityPolicies.h"
#include "content/ContentId.h"
#include "effects/GameplayEffectPolicies.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/tags/GameplayTags.h"

#include <optional>

namespace ly
{
	struct AttachmentSchema
	{
		struct Capability
		{
			inline static const GameplayTag& Damage = GameplayTags::Attachment::Capability::Damage;
			inline static const GameplayTag& Cooldown = GameplayTags::Attachment::Capability::Cooldown;
			inline static const GameplayTag& FireRate = GameplayTags::Attachment::Capability::FireRate;
			inline static const GameplayTag& Projectile = GameplayTags::Attachment::Capability::Projectile;
			inline static const GameplayTag& Beam = GameplayTags::Attachment::Capability::Beam;
			inline static const GameplayTag& Area = GameplayTags::Attachment::Capability::Area;
		};

		struct AttributeIds
		{
			inline static const sas::AttributeId CooldownReductionOnIgnite{
				"Attachment.CooldownReductionOnIgnite"
			};
		};

		struct Event
		{
			inline static const GameplayTag& SourceDamageDealt = GameplayTags::Event::Source::DamageDealt;
			inline static const GameplayTag& SourceStatusIgniteApplied =
				GameplayTags::Event::Source::StatusApplied::Ignite;
		};
	};

	enum class AttachmentHostKind
	{
		Ability,
		PrimaryWeapon
	};

	enum class AttachmentConditionType
	{
		Always,
		HasDamageTag,
		MissingDamageTag,
		AttributeLessThan,
		AttributeGreaterThanOrEqual
	};

	struct AttachmentCondition
	{
		AttachmentConditionType type = AttachmentConditionType::Always;
		GameplayTag subjectTag;
		float threshold = 0.f;
		sas::AttributeId subjectAttributeId;
	};

	struct ConditionalAttributeModifier
	{
		AttachmentCondition condition;
		sas::AttributeModifier modifier;
	};

	enum class AttachmentEventAction
	{
		ReduceCooldown,
		ApplyEffect,
		RemoveEffects
	};

	enum class AttachmentCooldownTarget
	{
		Host,
		AllOwnerAbilities,
		AllNonPrimaryAbilities
	};

	struct AttachmentEventRule
	{
		GameplayTag eventTag;
		AttachmentEventAction action = AttachmentEventAction::ReduceCooldown;
		AttachmentCooldownTarget cooldownTarget = AttachmentCooldownTarget::Host;
		sas::AttributeId magnitudeAttributeId;
		float baseMagnitude = 0.f;
		bool requireOwnerAsEventSource = true;
		List<GameplayTag> requiredDamageTags;
		// Empty means any ability lifecycle source. These are payload filters,
		// not additional event tags.
		sas::ContentId abilityId;
		std::optional<sas::AbilityEndReason> endReason;
		// Source-ability category/family filters stay in payload data. They allow
		// one attachment rule to observe every Offensive or Defense ability.
		List<GameplayTag> requiredAbilityTags;
		List<GameplayTag> blockedAbilityTags;
		// Zero means unlimited matches. consumeOnMatch is the compact one-shot
		// form and takes precedence over maxMatches.
		bool consumeOnMatch = false;
		int maxMatches = 0;
		// Generic effect actions are optional extensions of the event rule. They
		// keep attachment reactions data-driven without adding a new subsystem.
		sas::ContentId effectId;
		std::optional<sas::GameplayEffectDisposition> effectDisposition;
		bool effectCleanseableOnly = false;
		std::string effectCategory;
		std::string effectImmunityCategory;
		List<GameplayTag> requiredOwnerTags;
		List<GameplayTag> blockedOwnerTags;
	};

	// Definitions are data only. The small runtime resolver interprets these generic rules.
	struct AttachmentDefinition
	{
		sas::ContentId attachmentId;
		std::string displayName;
		List<AttachmentHostKind> allowedHosts;
		List<GameplayTag> requiredCapabilities;
		sas::GameplayAttributeList grantedAttributes;
		List<sas::AttributeModifier> attributeModifiers;
		List<ConditionalAttributeModifier> conditionalAttributeModifiers;
		GameplayTag replaceDamageType;
		int damageTypePriority = 0;
		List<AttachmentEventRule> eventRules;
	};

	struct EquippedAttachment
	{
		AttachmentDefinition definition;
		AttachmentHostKind hostKind = AttachmentHostKind::Ability;
		List<int> eventMatchCounts;
	};
}
