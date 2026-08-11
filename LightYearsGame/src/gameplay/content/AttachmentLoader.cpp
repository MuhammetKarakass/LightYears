#include "gameplay/content/AttachmentLoader.h"

#include "attributes/AttributeId.h"
#include "gameplay/attributes/AttributeIdSchema.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/tags/GameplayTagSchema.h"

#include "framework/JsonDocumentLoader.h"

#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace ly::content
{
	namespace
	{
		using Json = JsonDocumentLoader::Json;

		std::string ReadRequiredString(
			const Json& object,
			const char* fieldName)
		{
			return object.at(fieldName).get<std::string>();
		}

		AttachmentHostKind ParseHostKind(const std::string& value)
		{
			if (value == "Ability")
			{
				return AttachmentHostKind::Ability;
			}
			if (value == "PrimaryWeapon")
			{
				return AttachmentHostKind::PrimaryWeapon;
			}

			throw std::runtime_error("Unknown attachment host kind: " + value);
		}

		AttachmentConditionType ParseConditionType(const std::string& value)
		{
			if (value == "Always")
			{
				return AttachmentConditionType::Always;
			}
			if (value == "HasDamageTag")
			{
				return AttachmentConditionType::HasDamageTag;
			}
			if (value == "MissingDamageTag")
			{
				return AttachmentConditionType::MissingDamageTag;
			}
			if (value == "AttributeLessThan")
			{
				return AttachmentConditionType::AttributeLessThan;
			}
			if (value == "AttributeGreaterThanOrEqual")
			{
				return AttachmentConditionType::AttributeGreaterThanOrEqual;
			}

			throw std::runtime_error("Unknown attachment condition type: " + value);
		}

		sas::AttributeModifierOperation ParseModifierOperation(
			const std::string& value)
		{
			if (value == "Add")
			{
				return sas::AttributeModifierOperation::Add;
			}
			if (value == "Multiply")
			{
				return sas::AttributeModifierOperation::Multiply;
			}
			if (value == "Override")
			{
				return sas::AttributeModifierOperation::Override;
			}

			throw std::runtime_error("Unknown attribute modifier operation: " + value);
		}

		AttachmentEventAction ParseEventAction(const std::string& value)
		{
			if (value == "ReduceCooldown")
			{
				return AttachmentEventAction::ReduceCooldown;
			}
			if (value == "ApplyEffect")
			{
				return AttachmentEventAction::ApplyEffect;
			}
			if (value == "RemoveEffects")
			{
				return AttachmentEventAction::RemoveEffects;
			}

			throw std::runtime_error("Unknown attachment event action: " + value);
		}

		sas::GameplayEffectDisposition ParseEffectDisposition(
			const std::string& value
		)
		{
			if (value == "Beneficial") return sas::GameplayEffectDisposition::Beneficial;
			if (value == "Harmful") return sas::GameplayEffectDisposition::Harmful;
			if (value == "Neutral") return sas::GameplayEffectDisposition::Neutral;
			throw std::runtime_error("Unknown attachment effect disposition: " + value);
		}

		AttachmentCooldownTarget ParseCooldownTarget(const std::string& value)
		{
			if (value == "Host")
			{
				return AttachmentCooldownTarget::Host;
			}
			if (value == "AllOwnerAbilities")
			{
				return AttachmentCooldownTarget::AllOwnerAbilities;
			}
			if (value == "AllNonPrimaryAbilities")
			{
				return AttachmentCooldownTarget::AllNonPrimaryAbilities;
			}

			throw std::runtime_error("Unknown attachment cooldown target: " + value);
		}

		sas::AbilityEndReason ParseAbilityEndReason(const std::string& value)
		{
			if (value == "Completed") return sas::AbilityEndReason::Completed;
			if (value == "DurationExpired") return sas::AbilityEndReason::DurationExpired;
			if (value == "InputReleased") return sas::AbilityEndReason::InputReleased;
			if (value == "Cancelled") return sas::AbilityEndReason::Cancelled;
			if (value == "Interrupted") return sas::AbilityEndReason::Interrupted;
			if (value == "OwnerDestroyed") return sas::AbilityEndReason::OwnerDestroyed;
			throw std::runtime_error("Unknown ability end reason: " + value);
		}

		sas::GameplayAttribute ParseGameplayAttribute(const Json& object)
		{
			return sas::GameplayAttribute{
				sas::AttributeId{ ReadRequiredString(object, "id") },
				object.at("baseValue").get<float>(),
				object.value("minValue", 0.f),
				object.value(
					"maxValue",
					std::numeric_limits<float>::max()
				)
			};
		}

		sas::AttributeModifier ParseAttributeModifier(const Json& object)
		{
			return sas::AttributeModifier{
				sas::AttributeId{ ReadRequiredString(object, "attributeId") },
				ParseModifierOperation(
					ReadRequiredString(object, "operation")
				),
				object.at("magnitude").get<float>(),
				object.value("priority", 0)
			};
		}

		AttachmentCondition ParseCondition(const Json& object)
		{
			AttachmentCondition condition;
			condition.type = ParseConditionType(
				ReadRequiredString(object, "type")
			);
			const std::string subject = object.value("subjectTag", std::string{});
			if (condition.type == AttachmentConditionType::AttributeLessThan ||
				condition.type == AttachmentConditionType::AttributeGreaterThanOrEqual)
			{
				condition.subjectAttributeId = sas::AttributeId{ subject };
			}
			else
			{
				condition.subjectTag = GameplayTag{ subject };
			}
			condition.threshold = object.value("threshold", 0.f);
			return condition;
		}

		ConditionalAttributeModifier ParseConditionalModifier(const Json& object)
		{
			return ConditionalAttributeModifier{
				ParseCondition(object.at("condition")),
				ParseAttributeModifier(object.at("modifier"))
			};
		}

		AttachmentEventRule ParseEventRule(const Json& object)
		{
			AttachmentEventRule rule;
			rule.eventTag = GameplayTag{
				ReadRequiredString(object, "eventTag")
			};
			rule.action = ParseEventAction(
				ReadRequiredString(object, "action")
			);
			if (rule.action == AttachmentEventAction::ReduceCooldown)
			{
				rule.cooldownTarget = ParseCooldownTarget(
					ReadRequiredString(object, "cooldownTarget")
				);
				rule.magnitudeAttributeId = sas::AttributeId{
					ReadRequiredString(object, "magnitudeAttributeId")
				};
			}
			rule.baseMagnitude = object.value("baseMagnitude", 0.f);
			rule.requireOwnerAsEventSource = object.value(
				"requireOwnerAsEventSource",
				true
			);
			rule.abilityId = sas::ContentId{
				object.value("abilityId", std::string{})
			};
			if (object.contains("endReason"))
			{
				rule.endReason = ParseAbilityEndReason(
					object.at("endReason").get<std::string>()
				);
			}
			for (const Json& tag : object.value("requiredAbilityTags", Json::array()))
			{
				rule.requiredAbilityTags.emplace_back(
					GameplayTag{ tag.get<std::string>() }
				);
			}
			for (const Json& tag : object.value("blockedAbilityTags", Json::array()))
			{
				rule.blockedAbilityTags.emplace_back(
					GameplayTag{ tag.get<std::string>() }
				);
			}
			rule.consumeOnMatch = object.value("consumeOnMatch", false);
			rule.maxMatches = object.value("maxMatches", 0);

			for (const Json& tag : object.value("requiredDamageTags", Json::array()))
			{
				rule.requiredDamageTags.emplace_back(
					GameplayTag{ tag.get<std::string>() }
				);
			}
			if (rule.action == AttachmentEventAction::ApplyEffect)
			{
				rule.effectId = sas::ContentId{ ReadRequiredString(object, "effectId") };
			}
			if (rule.action == AttachmentEventAction::RemoveEffects)
			{
				if (object.contains("disposition"))
				{
					rule.effectDisposition = ParseEffectDisposition(
						object.at("disposition").get<std::string>()
					);
				}
				rule.effectCleanseableOnly = object.value("cleanseableOnly", false);
				rule.effectCategory = object.value("category", std::string{});
				rule.effectImmunityCategory = object.value("immunityCategory", std::string{});
			}
			for (const Json& tag : object.value("requiredOwnerTags", Json::array()))
			{
				rule.requiredOwnerTags.emplace_back(
					GameplayTag{ tag.get<std::string>() }
				);
			}
			for (const Json& tag : object.value("blockedOwnerTags", Json::array()))
			{
				rule.blockedOwnerTags.emplace_back(
					GameplayTag{ tag.get<std::string>() }
				);
			}

			return rule;
		}

		AttachmentDefinition ParseAttachment(const Json& object)
		{
			AttachmentDefinition definition;
			definition.attachmentId = sas::ContentId{ ReadRequiredString(object, "id") };
			definition.displayName = ReadRequiredString(object, "displayName");

			for (const Json& host : object.at("allowedHosts"))
			{
				definition.allowedHosts.emplace_back(
					ParseHostKind(host.get<std::string>())
				);
			}

			for (const Json& capability : object.at("requiredCapabilities"))
			{
				definition.requiredCapabilities.emplace_back(
					GameplayTag{ capability.get<std::string>() }
				);
			}

			for (const Json& attribute : object.value("grantedAttributes", Json::array()))
			{
				definition.grantedAttributes.emplace_back(
					ParseGameplayAttribute(attribute)
				);
			}

			for (const Json& modifier : object.value("attributeModifiers", Json::array()))
			{
				definition.attributeModifiers.emplace_back(
					ParseAttributeModifier(modifier)
				);
			}

			for (const Json& modifier : object.value(
				"conditionalAttributeModifiers",
				Json::array()))
			{
				definition.conditionalAttributeModifiers.emplace_back(
					ParseConditionalModifier(modifier)
				);
			}

			definition.replaceDamageType = GameplayTag{
				object.value("replaceDamageType", std::string{})
			};
			definition.damageTypePriority = object.value(
				"damageTypePriority",
				0
			);

			for (const Json& eventRule : object.value("eventRules", Json::array()))
			{
				definition.eventRules.emplace_back(ParseEventRule(eventRule));
			}

			return definition;
		}

		void RequireValidTag(
			const GameplayTag& tag,
			GameplayTagKind kind,
			const char* usage
		)
		{
			std::string failureReason;
			if (!GameplayTagSchema::Validate(tag, kind, &failureReason))
			{
				throw std::runtime_error(std::string{ usage } + ": " + failureReason);
			}
		}

		void RequireValidAttribute(
			const sas::AttributeId& id,
			const char* usage
		)
		{
			if (!AttributeIdSchema::Validate(id, nullptr))
			{
				throw std::runtime_error(std::string{ usage } + ": invalid AttributeId");
			}
		}

		void ValidateAttachmentTags(const AttachmentDefinition& definition)
		{
			std::string idFailureReason;
			if (!ContentIdSchema::ValidateAttachmentId(
				definition.attachmentId.ToString(),
				&idFailureReason
			))
			{
				throw std::runtime_error("Attachment ID: " + idFailureReason);
			}
			for (const GameplayTag& capability : definition.requiredCapabilities)
			{
				RequireValidTag(capability, GameplayTagKind::AttachmentCapability, "Attachment capability");
			}
			for (const sas::GameplayAttribute& attribute : definition.grantedAttributes)
			{
				RequireValidAttribute(attribute.id, "Attachment attribute");
			}
			for (const sas::AttributeModifier& modifier : definition.attributeModifiers)
			{
				RequireValidAttribute(modifier.attributeId, "Attachment modifier attribute");
			}
			for (const ConditionalAttributeModifier& conditional : definition.conditionalAttributeModifiers)
			{
				RequireValidAttribute(conditional.modifier.attributeId, "Attachment conditional modifier");
				if (conditional.condition.type == AttachmentConditionType::Always)
				{
					continue;
				}
				if (conditional.condition.type == AttachmentConditionType::HasDamageTag ||
					conditional.condition.type == AttachmentConditionType::MissingDamageTag)
				{
					RequireValidTag(conditional.condition.subjectTag, GameplayTagKind::DamageType, "Attachment condition");
				}
				else
				{
					RequireValidAttribute(conditional.condition.subjectAttributeId, "Attachment condition");
				}
			}
			if (definition.replaceDamageType.IsValid())
			{
				RequireValidTag(definition.replaceDamageType, GameplayTagKind::DamageType, "Attachment damage replacement");
			}
			for (const AttachmentEventRule& rule : definition.eventRules)
			{
				if (rule.maxMatches < 0)
				{
					throw std::runtime_error(
						"Attachment event maxMatches cannot be negative."
					);
				}
				RequireValidTag(rule.eventTag, GameplayTagKind::Event, "Attachment event");
				for (const GameplayTag& tag : rule.requiredOwnerTags)
				{
					if (!GameplayTagSchema::ValidateAbilityOwnerConditionTag(tag, &idFailureReason))
					{
						throw std::runtime_error("Attachment required owner tag: " + idFailureReason);
					}
				}
				for (const GameplayTag& tag : rule.blockedOwnerTags)
				{
					if (!GameplayTagSchema::ValidateAbilityOwnerConditionTag(tag, &idFailureReason))
					{
						throw std::runtime_error("Attachment blocked owner tag: " + idFailureReason);
					}
				}
				for (const GameplayTag& tag : rule.requiredAbilityTags)
				{
					if (!GameplayTagSchema::ValidateAbilityOwnerConditionTag(tag, &idFailureReason))
					{
						throw std::runtime_error("Attachment required ability tag: " + idFailureReason);
					}
				}
				for (const GameplayTag& tag : rule.blockedAbilityTags)
				{
					if (!GameplayTagSchema::ValidateAbilityOwnerConditionTag(tag, &idFailureReason))
					{
						throw std::runtime_error("Attachment blocked ability tag: " + idFailureReason);
					}
				}
				if (rule.action == AttachmentEventAction::ReduceCooldown)
				{
					RequireValidAttribute(rule.magnitudeAttributeId, "Attachment event magnitude");
				}
				if (rule.action == AttachmentEventAction::ApplyEffect)
				{
					if (!ContentIdSchema::ValidateEffectId(rule.effectId.ToString(), &idFailureReason))
					{
						throw std::runtime_error("Attachment event effect ID: " + idFailureReason);
					}
				}
				if (rule.action == AttachmentEventAction::RemoveEffects &&
					!rule.effectDisposition.has_value() &&
					!rule.effectCleanseableOnly &&
					rule.effectCategory.empty() &&
					rule.effectImmunityCategory.empty())
				{
					throw std::runtime_error(
						"Attachment RemoveEffects action requires a metadata filter."
					);
				}
				for (const GameplayTag& damageTag : rule.requiredDamageTags)
				{
					RequireValidTag(damageTag, GameplayTagKind::DamageType, "Attachment event damage tag");
				}
			}
		}
	}

	AttachmentLoader::Result AttachmentLoader::LoadFromFile(
		const std::filesystem::path& filePath)
	{
		const JsonDocumentLoader::Result documentResult =
			JsonDocumentLoader::LoadFromFile(filePath);

		if (!documentResult.Succeeded())
		{
			return Result{ {}, documentResult.error };
		}

		try
		{
			const Json& root = *documentResult.document;
			if (root.at("schemaVersion").get<int>() != 1)
			{
				return Result{
					{},
					"Unsupported attachment schema version"
				};
			}

			Result result;
			std::set<std::string> attachmentIds;
			for (const Json& attachment : root.at("attachments"))
			{
				AttachmentDefinition definition = ParseAttachment(attachment);
				ValidateAttachmentTags(definition);
				const std::string& id = definition.attachmentId.ToString();
				if (!attachmentIds.insert(id).second)
				{
					throw std::runtime_error("Duplicate attachment ID: " + id);
				}
				result.definitions.emplace_back(std::move(definition));
			}

			return result;
		}
		catch (const std::exception& exception)
		{
			return Result{
				{},
				"Failed to load attachments from '" +
				filePath.string() +
				"': " +
				exception.what()
			};
		}
	}
}
