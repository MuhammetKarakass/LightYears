#include "attributes/AttributeSystem.h"
#include "gameplay/attachment/AttachmentLoadout.h"
#include "gameplay/attributes/AttributeIdSchema.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/tags/GameplayTagSchema.h"

#include <algorithm>
#include <utility>

namespace ly
{
	namespace
	{
		bool HasExactTag(const List<GameplayTag>& tags, const GameplayTag& expected)
		{
			return std::any_of(tags.begin(), tags.end(), [&](const GameplayTag& tag)
			{
				return tag.MatchesTagExact(expected);
			});
		}

		bool HasMatchingTag(const List<GameplayTag>& tags, const GameplayTag& expected)
		{
			return std::any_of(tags.begin(), tags.end(), [&](const GameplayTag& tag)
			{
				return tag.MatchesTag(expected);
			});
		}

		sas::GameplayAttributeList ApplyModifiers(
			sas::GameplayAttributeList attributes,
			const List<sas::AttributeModifier>& modifiers
		)
		{
			List<sas::AttributeId> processedAttributes;
			for (const sas::AttributeModifier& modifier : modifiers)
			{
				if (!AttributeIdSchema::Validate(modifier.attributeId, nullptr))
				{
					continue;
				}
				if (std::find(processedAttributes.begin(), processedAttributes.end(), modifier.attributeId) != processedAttributes.end())
				{
					continue;
				}
				processedAttributes.push_back(modifier.attributeId);

				sas::GameplayAttribute* attribute = sas::FindAttribute(attributes, modifier.attributeId);
				if (!attribute)
				{
					attributes.push_back(sas::GameplayAttribute{ modifier.attributeId, 0.f, 0.f });
					attribute = &attributes.back();
				}

				List<sas::AttributeModifier> targetModifiers;
				for (const sas::AttributeModifier& candidate : modifiers)
				{
					if (candidate.attributeId == modifier.attributeId)
					{
						targetModifiers.push_back(candidate);
					}
				}

				sas::GameplayAttribute input{
					attribute->id,
					attribute->currentValue,
					attribute->minValue,
					attribute->maxValue
				};
				attribute->baseValue = input.baseValue;
				attribute->currentValue = sas::CalculateModifiedAttributeValue(input, targetModifiers);
			}

			return attributes;
		}
	}

	bool AttachmentLoadout::TryEquip(
		const AttachmentDefinition& definition,
		AttachmentHostKind hostKind,
		const List<GameplayTag>& hostCapabilities,
		size_t slotCapacity,
		std::string* failureReason
	)
	{
		std::string tagFailureReason;
		if (!content::ContentIdSchema::ValidateAttachmentId(
			definition.attachmentId.ToString(),
			&tagFailureReason
		))
		{
			if (failureReason)
			{
				*failureReason = "Attachment requires a valid ID: " + tagFailureReason;
			}
			return false;
		}
		for (const GameplayTag& capability : definition.requiredCapabilities)
		{
			if (!GameplayTagSchema::Validate(
				capability,
				GameplayTagKind::AttachmentCapability,
				&tagFailureReason
			))
			{
				if (failureReason) *failureReason = "Attachment capability tag is invalid: " + tagFailureReason;
				return false;
			}
		}
		for (const GameplayTag& capability : hostCapabilities)
		{
			if (!GameplayTagSchema::Validate(
				capability,
				GameplayTagKind::AttachmentCapability,
				&tagFailureReason
			))
			{
				if (failureReason) *failureReason = "Host attachment capability tag is invalid: " + tagFailureReason;
				return false;
			}
		}
		if (definition.replaceDamageType.IsValid() && !GameplayTagSchema::Validate(
			definition.replaceDamageType,
			GameplayTagKind::DamageType,
			&tagFailureReason
		))
		{
			if (failureReason) *failureReason = "Attachment damage type tag is invalid: " + tagFailureReason;
			return false;
		}
		for (const sas::GameplayAttribute& attribute : definition.grantedAttributes)
		{
			if (!AttributeIdSchema::Validate(attribute.id, nullptr))
			{
				if (failureReason) *failureReason = "Attachment attribute ID is invalid.";
				return false;
			}
		}
		for (const sas::AttributeModifier& modifier : definition.attributeModifiers)
		{
			if (!AttributeIdSchema::Validate(modifier.attributeId, nullptr))
			{
				if (failureReason) *failureReason = "Attachment modifier attribute ID is invalid.";
				return false;
			}
		}
		for (const ConditionalAttributeModifier& conditional : definition.conditionalAttributeModifiers)
		{
			if (!AttributeIdSchema::Validate(conditional.modifier.attributeId, nullptr))
			{
				if (failureReason) *failureReason = "Attachment conditional modifier ID is invalid.";
				return false;
			}
			if (conditional.condition.type == AttachmentConditionType::HasDamageTag ||
				conditional.condition.type == AttachmentConditionType::MissingDamageTag)
			{
				if (!GameplayTagSchema::Validate(
					conditional.condition.subjectTag,
					GameplayTagKind::DamageType,
					&tagFailureReason
				))
				{
					if (failureReason) *failureReason = "Attachment condition tag is invalid: " + tagFailureReason;
					return false;
				}
			}
			else if (conditional.condition.type == AttachmentConditionType::AttributeLessThan ||
				conditional.condition.type == AttachmentConditionType::AttributeGreaterThanOrEqual)
			{
				if (!AttributeIdSchema::Validate(conditional.condition.subjectAttributeId, nullptr))
				{
					if (failureReason) *failureReason = "Attachment condition attribute ID is invalid.";
					return false;
				}
			}
		}
		for (const AttachmentEventRule& rule : definition.eventRules)
		{
			if (rule.maxMatches < 0)
			{
				if (failureReason)
				{
					*failureReason = "Attachment event maxMatches cannot be negative.";
				}
				return false;
			}
			if (!GameplayTagSchema::Validate(rule.eventTag, GameplayTagKind::Event, &tagFailureReason))
			{
				if (failureReason) *failureReason = "Attachment event tag or magnitude attribute ID is invalid: " + tagFailureReason;
				return false;
			}
			if (rule.action == AttachmentEventAction::ReduceCooldown &&
				!AttributeIdSchema::Validate(rule.magnitudeAttributeId, nullptr))
			{
				if (failureReason) *failureReason = "Attachment event magnitude attribute ID is invalid.";
				return false;
			}
			if (rule.abilityId.IsValid())
			{
				std::string abilityIdFailure;
				if (!content::ContentIdSchema::ValidateAbilityId(
					rule.abilityId.ToString(),
					&abilityIdFailure
				))
				{
					if (failureReason) *failureReason = "Attachment event ability ID is invalid: " + abilityIdFailure;
					return false;
				}
			}
			for (const GameplayTag& damageTag : rule.requiredDamageTags)
			{
				if (!GameplayTagSchema::Validate(
					damageTag,
					GameplayTagKind::DamageType,
					&tagFailureReason
				))
				{
					if (failureReason) *failureReason = "Attachment event damage tag is invalid: " + tagFailureReason;
					return false;
				}
			}
		}
		if (std::find(definition.allowedHosts.begin(), definition.allowedHosts.end(), hostKind) == definition.allowedHosts.end())
		{
			if (failureReason)
			{
				*failureReason = "Attachment cannot be installed on this host type.";
			}
			return false;
		}
		if (!HasAllCapabilities(definition, hostCapabilities))
		{
			if (failureReason)
			{
				*failureReason = "Attachment host does not provide the required capabilities.";
			}
			return false;
		}
		if (!HasCapacity(hostKind, slotCapacity))
		{
			if (failureReason)
			{
				*failureReason = "Attachment slot capacity reached.";
			}
			return false;
		}
		for (const EquippedAttachment& equipped : mEquipped)
		{
			if (equipped.hostKind != hostKind)
			{
				continue;
			}
			if (equipped.definition.attachmentId == definition.attachmentId)
			{
				if (failureReason)
				{
					*failureReason = "The attachment is already installed on this host.";
				}
				return false;
			}
			if (definition.replaceDamageType.IsValid() && equipped.definition.replaceDamageType.IsValid())
			{
				if (failureReason)
				{
					*failureReason = "Only one damage-type conversion can be installed on a host.";
				}
				return false;
			}
		}

		EquippedAttachment equipped{ definition, hostKind };
		equipped.eventMatchCounts.resize(definition.eventRules.size(), 0);
		mEquipped.push_back(std::move(equipped));
		++mRevision;
		return true;
	}

	bool AttachmentLoadout::Remove(const sas::ContentId& attachmentId, AttachmentHostKind hostKind)
	{
		auto found = std::find_if(mEquipped.begin(), mEquipped.end(), [&](const EquippedAttachment& equipped)
		{
			return equipped.hostKind == hostKind && equipped.definition.attachmentId == attachmentId;
		});
		if (found == mEquipped.end())
		{
			return false;
		}
		mEquipped.erase(found);
		++mRevision;
		return true;
	}

	void AttachmentLoadout::Clear()
	{
		mEquipped.clear();
		++mRevision;
	}

	sas::GameplayAttributeList AttachmentLoadout::MergeGrantedAttributes(
		AttachmentHostKind hostKind,
		const sas::GameplayAttributeList& sourceAttributes
	) const
	{
		sas::GameplayAttributeList merged = sourceAttributes;
		for (const EquippedAttachment& equipped : mEquipped)
		{
			if (equipped.hostKind != hostKind)
			{
				continue;
			}
			for (const sas::GameplayAttribute& granted : equipped.definition.grantedAttributes)
			{
				if (AttributeIdSchema::Validate(granted.id, nullptr) && !sas::FindAttribute(merged, granted.id))
				{
					merged.push_back(granted);
				}
			}
		}
		return merged;
	}

	sas::GameplayAttribute AttachmentLoadout::ApplyStaticModifiers(
		AttachmentHostKind hostKind,
		const sas::GameplayAttribute& attribute
	) const
	{
		sas::GameplayAttribute result = attribute;
		const List<sas::AttributeModifier> modifiers = CollectStaticModifiers(hostKind);
		result.currentValue = sas::CalculateModifiedAttributeValue(
			sas::GameplayAttribute{ attribute.id, attribute.currentValue, attribute.minValue, attribute.maxValue },
			modifiers
		);
		return result;
	}

	sas::GameplayAttributeList AttachmentLoadout::ApplyConditionalModifiers(
		AttachmentHostKind hostKind,
		const sas::GameplayAttributeList& resolvedAttributes,
		const List<GameplayTag>& originalDamageTags
	) const
	{
		List<sas::AttributeModifier> activeModifiers;
		for (const EquippedAttachment& equipped : mEquipped)
		{
			if (equipped.hostKind != hostKind)
			{
				continue;
			}
			for (const ConditionalAttributeModifier& conditional : equipped.definition.conditionalAttributeModifiers)
			{
				if (IsConditionMet(conditional.condition, resolvedAttributes, originalDamageTags))
				{
					activeModifiers.push_back(conditional.modifier);
				}
			}
		}
		return ApplyModifiers(resolvedAttributes, activeModifiers);
	}

	List<GameplayTag> AttachmentLoadout::ResolveDamageTags(
		AttachmentHostKind hostKind,
		const List<GameplayTag>& baseDamageTags
	) const
	{
		List<const EquippedAttachment*> conversions;
		for (const EquippedAttachment& equipped : mEquipped)
		{
			if (equipped.hostKind == hostKind && equipped.definition.replaceDamageType.IsValid())
			{
				conversions.push_back(&equipped);
			}
		}
		std::stable_sort(conversions.begin(), conversions.end(), [](const EquippedAttachment* left, const EquippedAttachment* right)
		{
			return left->definition.damageTypePriority < right->definition.damageTypePriority;
		});

		List<GameplayTag> tags = baseDamageTags;
		for (const EquippedAttachment* conversion : conversions)
		{
			tags.erase(std::remove_if(tags.begin(), tags.end(), [](const GameplayTag& tag)
			{
				return tag.MatchesTag(DamageTypeSchema::Root);
			}), tags.end());
			tags.push_back(conversion->definition.replaceDamageType);
		}
		return tags;
	}

	float AttachmentLoadout::ResolveGrantedAttributeValue(
		AttachmentHostKind hostKind,
		const sas::AttributeId& attributeId,
		float fallback
	) const
	{
		sas::GameplayAttribute value{ attributeId, fallback, 0.f };
		for (const EquippedAttachment& equipped : mEquipped)
		{
			if (equipped.hostKind != hostKind)
			{
				continue;
			}
			for (const sas::GameplayAttribute& granted : equipped.definition.grantedAttributes)
			{
				if (granted.id == attributeId)
				{
					value.baseValue += granted.baseValue;
					value.currentValue = value.baseValue;
				}
			}
		}
		return ApplyStaticModifiers(hostKind, value).currentValue;
	}

	bool AttachmentLoadout::HasCapacity(AttachmentHostKind hostKind, size_t slotCapacity) const
	{
		return static_cast<size_t>(std::count_if(mEquipped.begin(), mEquipped.end(), [&](const EquippedAttachment& equipped)
		{
			return equipped.hostKind == hostKind;
		})) < slotCapacity;
	}

	bool AttachmentLoadout::HasAllCapabilities(
		const AttachmentDefinition& definition,
		const List<GameplayTag>& hostCapabilities
	) const
	{
		return std::all_of(definition.requiredCapabilities.begin(), definition.requiredCapabilities.end(), [&](const GameplayTag& required)
		{
			return HasMatchingTag(hostCapabilities, required);
		});
	}

	bool AttachmentLoadout::IsConditionMet(
		const AttachmentCondition& condition,
		const sas::GameplayAttributeList& resolvedAttributes,
		const List<GameplayTag>& originalDamageTags
	) const
	{
		switch (condition.type)
		{
		case AttachmentConditionType::Always:
			return true;
		case AttachmentConditionType::HasDamageTag:
			return HasMatchingTag(originalDamageTags, condition.subjectTag);
		case AttachmentConditionType::MissingDamageTag:
			return !HasMatchingTag(originalDamageTags, condition.subjectTag);
		case AttachmentConditionType::AttributeLessThan:
			return sas::FindAttributeValue(resolvedAttributes, condition.subjectAttributeId) < condition.threshold;
		case AttachmentConditionType::AttributeGreaterThanOrEqual:
			return sas::FindAttributeValue(resolvedAttributes, condition.subjectAttributeId) >= condition.threshold;
		}
		return false;
	}

	List<sas::AttributeModifier> AttachmentLoadout::CollectStaticModifiers(AttachmentHostKind hostKind) const
	{
		List<sas::AttributeModifier> modifiers;
		for (const EquippedAttachment& equipped : mEquipped)
		{
			if (equipped.hostKind == hostKind)
			{
				modifiers.insert(
					modifiers.end(),
					equipped.definition.attributeModifiers.begin(),
					equipped.definition.attributeModifiers.end()
				);
			}
		}
		return modifiers;
	}
}
