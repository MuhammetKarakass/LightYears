#include "attributes/AttributeSystem.h"
#include "gameplay/attachment/AttachmentLoadout.h"

#include <algorithm>

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
			List<GameplayTag> processedAttributes;
			for (const sas::AttributeModifier& modifier : modifiers)
			{
				if (!modifier.attributeId.IsValid())
				{
					continue;
				}
				if (std::find(processedAttributes.begin(), processedAttributes.end(), modifier.attributeId) != processedAttributes.end())
				{
					continue;
				}
				processedAttributes.push_back(modifier.attributeId);

				sas::GameplayAttribute* attribute = sas::FindGameplayAttribute(attributes, modifier.attributeId);
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
		if (!definition.attachmentId.IsValid())
		{
			if (failureReason)
			{
				*failureReason = "Attachment requires a valid ID.";
			}
			return false;
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

		mEquipped.push_back(EquippedAttachment{ definition, hostKind });
		++mRevision;
		return true;
	}

	bool AttachmentLoadout::Remove(const GameplayTag& attachmentId, AttachmentHostKind hostKind)
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
				if (granted.id.IsValid() && !sas::FindGameplayAttribute(merged, granted.id))
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
		const GameplayTag& attributeId,
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
			return sas::FindGameplayAttributeValue(resolvedAttributes, condition.subjectTag) < condition.threshold;
		case AttachmentConditionType::AttributeGreaterThanOrEqual:
			return sas::FindGameplayAttributeValue(resolvedAttributes, condition.subjectTag) >= condition.threshold;
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
