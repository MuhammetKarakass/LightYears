#include "gameplay/content/AttachmentLoader.h"

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

			throw std::runtime_error("Unknown attachment event action: " + value);
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

		sas::GameplayAttribute ParseGameplayAttribute(const Json& object)
		{
			return sas::GameplayAttribute{
				GameplayTag{ ReadRequiredString(object, "id") },
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
				GameplayTag{ ReadRequiredString(object, "attributeId") },
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
			condition.subjectTag = GameplayTag{
				object.value("subjectTag", std::string{})
			};
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
			rule.cooldownTarget = ParseCooldownTarget(
				ReadRequiredString(object, "cooldownTarget")
			);
			rule.magnitudeAttributeId = GameplayTag{
				ReadRequiredString(object, "magnitudeAttributeId")
			};
			rule.baseMagnitude = object.value("baseMagnitude", 0.f);
		rule.requireOwnerAsEventSource = object.value(
				"requireOwnerAsEventSource",
				true
			);

			for (const Json& tag : object.value("requiredDamageTags", Json::array()))
			{
				rule.requiredDamageTags.emplace_back(
					GameplayTag{ tag.get<std::string>() }
				);
			}

			return rule;
		}

		AttachmentDefinition ParseAttachment(const Json& object)
		{
			AttachmentDefinition definition;
			definition.attachmentId = ReadRequiredString(object, "id");
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

		void ValidateAttachmentTags(const AttachmentDefinition& definition)
		{
			std::string idFailureReason;
			if (!ContentIdSchema::ValidateAttachmentId(
				definition.attachmentId,
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
				RequireValidTag(attribute.id, GameplayTagKind::Attribute, "Attachment attribute");
			}
			for (const sas::AttributeModifier& modifier : definition.attributeModifiers)
			{
				RequireValidTag(modifier.attributeId, GameplayTagKind::Attribute, "Attachment modifier attribute");
			}
			for (const ConditionalAttributeModifier& conditional : definition.conditionalAttributeModifiers)
			{
				RequireValidTag(conditional.modifier.attributeId, GameplayTagKind::Attribute, "Attachment conditional modifier");
				if (conditional.condition.type == AttachmentConditionType::Always)
				{
					continue;
				}
				const GameplayTagKind conditionTagKind =
					conditional.condition.type == AttachmentConditionType::HasDamageTag ||
					conditional.condition.type == AttachmentConditionType::MissingDamageTag
					? GameplayTagKind::DamageType
					: GameplayTagKind::Attribute;
				RequireValidTag(conditional.condition.subjectTag, conditionTagKind, "Attachment condition");
			}
			if (definition.replaceDamageType.IsValid())
			{
				RequireValidTag(definition.replaceDamageType, GameplayTagKind::DamageType, "Attachment damage replacement");
			}
			for (const AttachmentEventRule& rule : definition.eventRules)
			{
				RequireValidTag(rule.eventTag, GameplayTagKind::Event, "Attachment event");
				RequireValidTag(rule.magnitudeAttributeId, GameplayTagKind::Attribute, "Attachment event magnitude");
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
				const std::string& id = definition.attachmentId;
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
