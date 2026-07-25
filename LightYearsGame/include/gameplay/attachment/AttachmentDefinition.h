#pragma once

#include "gameplay/attributes/AttributeSystem.h"
#include "gameplay/damage/DamageTypeSystem.h"

namespace ly
{
	struct AttachmentSchema
	{
		struct Capability
		{
			inline static const GameplayTag Damage{ "Attachment.Capability.Damage" };
			inline static const GameplayTag Cooldown{ "Attachment.Capability.Cooldown" };
			inline static const GameplayTag FireRate{ "Attachment.Capability.FireRate" };
			inline static const GameplayTag Projectile{ "Attachment.Capability.Projectile" };
			inline static const GameplayTag Beam{ "Attachment.Capability.Beam" };
			inline static const GameplayTag Area{ "Attachment.Capability.Area" };
		};

		struct Attribute
		{
			inline static const GameplayTag CooldownReductionOnIgnite{
				"Attribute.Attachment.CooldownReductionOnIgnite"
			};
		};

		struct Event
		{
			inline static const GameplayTag SourceDamageDealt{ "Event.Source.DamageDealt" };
			inline static const GameplayTag SourceIgniteApplied{ "Event.Source.StatusApplied.Ignite" };
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
	};

	struct ConditionalAttributeModifier
	{
		AttachmentCondition condition;
		AttributeModifier modifier;
	};

	enum class AttachmentEventAction
	{
		ReduceCooldown
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
		GameplayTag magnitudeAttributeId;
		float baseMagnitude = 0.f;
		bool requireOwnerAsEventSource = true;
		List<GameplayTag> requiredDamageTags;
	};

	// Definitions are data only. The small runtime resolver interprets these generic rules.
	struct AttachmentDefinition
	{
		GameplayTag attachmentId;
		std::string displayName;
		List<AttachmentHostKind> allowedHosts;
		List<GameplayTag> requiredCapabilities;
		GameplayAttributeList grantedAttributes;
		List<AttributeModifier> attributeModifiers;
		List<ConditionalAttributeModifier> conditionalAttributeModifiers;
		GameplayTag replaceDamageType;
		int damageTypePriority = 0;
		List<AttachmentEventRule> eventRules;
	};

	struct EquippedAttachment
	{
		AttachmentDefinition definition;
		AttachmentHostKind hostKind = AttachmentHostKind::Ability;
	};
}
