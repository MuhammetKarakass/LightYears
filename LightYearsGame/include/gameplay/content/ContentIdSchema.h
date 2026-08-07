#pragma once

#include "gameplay/tags/GameplayTagSchema.h"

#include <string>

namespace ly::content
{
	// Content IDs are string identifiers, not GameplayTags stored in a generic
	// registry. This schema keeps their grammar in one place so loaders and
	// runtime validation cannot silently drift apart.
	struct ParsedAbilityId
	{
		GameplayTag categoryTag;
		GameplayTag familyTag;
	};

	struct ParsedFamilyRoleId
	{
		std::string family;
		std::string role;
	};

	struct ParsedWeaponId
	{
		std::string family;
	};

	class ContentIdSchema final
	{
	public:
		static bool ParseAbilityId(
			const std::string& id,
			ParsedAbilityId& parsed,
			std::string* failureReason = nullptr
		)
		{
			if (!GameplayTagSchema::Validate(
				GameplayTag{ id },
				GameplayTagKind::Ability,
				failureReason
			))
			{
				return false;
			}

			const std::size_t categoryStart = std::string{ "Ability." }.size();
			const std::size_t familySeparator = id.find('.', categoryStart);
			const std::size_t variantSeparator = familySeparator == std::string::npos
				? std::string::npos
				: id.find('.', familySeparator + 1);
			if (familySeparator == std::string::npos ||
				variantSeparator == std::string::npos ||
				variantSeparator + 1 >= id.size())
			{
				return Fail(
					failureReason,
					"Ability ID must use the format Ability.<Category>.<Family>.<Variant>."
				);
			}

			parsed.categoryTag = GameplayTag{ id.substr(0, familySeparator) };
			parsed.familyTag = GameplayTag{ id.substr(0, variantSeparator) };
			if (!GameplayTagSchema::Validate(
				parsed.categoryTag,
				GameplayTagKind::AbilityCategory,
				failureReason
			) || !GameplayTagSchema::Validate(
				parsed.familyTag,
				GameplayTagKind::Ability,
				failureReason
			))
			{
				return false;
			}
			return true;
		}

		static bool ValidateAbilityId(
			const std::string& id,
			std::string* failureReason = nullptr
		)
		{
			ParsedAbilityId ignored;
			return ParseAbilityId(id, ignored, failureReason);
		}

		static bool ValidateEffectId(
			const std::string& id,
			std::string* failureReason = nullptr
		)
		{
			return ValidatePrefixedId(
				id,
				"Effect.",
				3,
				"Effect ID must use the format Effect.<Family>.<Variant>.",
				failureReason
			);
		}

		static bool ValidateAbilityActorDefinitionId(
			const std::string& id,
			std::string* failureReason = nullptr
		)
		{
			ParsedFamilyRoleId ignored;
			return ParseAbilityActorDefinitionId(id, ignored, failureReason);
		}

		static bool ParseAbilityActorDefinitionId(
			const std::string& id,
			ParsedFamilyRoleId& parsed,
			std::string* failureReason = nullptr
		)
		{
			if (!ValidatePrefixedId(
				id, "Actor.Ability.", 5,
				"Ability actor definition ID must use Actor.Ability.<Family>.<Role>.<Variant>.",
				failureReason
			))
			{
				return false;
			}
			parsed.family = SegmentAt(id, 2);
			parsed.role = SegmentAt(id, 3);
			return true;
		}

		static bool ValidateAbilityAttributeProfileId(
			const std::string& id,
			std::string* failureReason = nullptr
		)
		{
			return ValidatePrefixedId(
				id, "AttributeProfile.", 4,
				"Ability attribute profile ID must use AttributeProfile.<Family>.<Role>.<Variant>.",
				failureReason
			);
		}

		static bool ValidateAbilityPresentationProfileId(
			const std::string& id,
			std::string* failureReason = nullptr
		)
		{
			ParsedFamilyRoleId ignored;
			return ParseAbilityPresentationProfileId(id, ignored, failureReason);
		}

		static bool ParseAbilityPresentationProfileId(
			const std::string& id,
			ParsedFamilyRoleId& parsed,
			std::string* failureReason = nullptr
		)
		{
			if (!ValidatePrefixedId(
				id, "Presentation.Ability.", 5,
				"Ability presentation profile ID must use Presentation.Ability.<Family>.<Role>.<Variant>.",
				failureReason
			))
			{
				return false;
			}
			parsed.family = SegmentAt(id, 2);
			parsed.role = SegmentAt(id, 3);
			return true;
		}

		static bool ValidateWeaponId(
			const std::string& id,
			std::string* failureReason = nullptr
		)
		{
			ParsedWeaponId ignored;
			return ParseWeaponId(id, ignored, failureReason);
		}

		static bool ParseWeaponId(
			const std::string& id,
			ParsedWeaponId& parsed,
			std::string* failureReason = nullptr
		)
		{
			if (!ValidatePrefixedId(
				id, "Weapon.", 4,
				"Weapon ID must use Weapon.<Family>.<Name>.<Variant>.",
				failureReason
			))
			{
				return false;
			}
			parsed.family = SegmentAt(id, 1);
			return true;
		}

		static bool ValidateShipId(
			const std::string& id,
			std::string* failureReason = nullptr
		)
		{
			return ValidatePrefixedId(
				id, "Ship.", 4,
				"Ship ID must use Ship.<Faction>.<Name>.<Variant>.",
				failureReason
			);
		}

		static bool ValidateAttachmentId(
			const std::string& id,
			std::string* failureReason = nullptr
		)
		{
			return ValidatePrefixedId(
				id, "Attachment.", 4,
				"Attachment ID must use Attachment.<Family>.<Name>.<Variant>.",
				failureReason
			);
		}

		static bool ValidateGameplayEffectVisualId(
			const std::string& id,
			std::string* failureReason = nullptr
		)
		{
			return ValidatePrefixedId(
				id, "Visual.Effect.", 4,
				"Gameplay effect visual ID must use Visual.Effect.<Family>.<VariantPath>.",
				failureReason
			);
		}

	private:
		static std::string SegmentAt(const std::string& id, std::size_t index)
		{
			std::size_t segmentStart = 0;
			for (std::size_t currentIndex = 0; currentIndex < index; ++currentIndex)
			{
				segmentStart = id.find('.', segmentStart);
				if (segmentStart == std::string::npos)
				{
					return {};
				}
				++segmentStart;
			}
			const std::size_t segmentEnd = id.find('.', segmentStart);
			return id.substr(segmentStart, segmentEnd - segmentStart);
		}

		static bool ValidatePrefixedId(
			const std::string& id,
			const char* prefix,
			std::size_t minimumSegments,
			const char* formatError,
			std::string* failureReason
		)
		{
			if (!GameplayTagSchema::Validate(
				GameplayTag{ id }, GameplayTagKind::Any, failureReason
			))
			{
				return false;
			}
			if (id.rfind(prefix, 0) != 0)
			{
				return Fail(failureReason, formatError);
			}
			std::size_t segmentCount = 1;
			for (const char character : id)
			{
				if (character == '.')
				{
					++segmentCount;
				}
			}
			return segmentCount >= minimumSegments || Fail(failureReason, formatError);
		}

		static bool Fail(std::string* failureReason, const char* reason)
		{
			if (failureReason)
			{
				*failureReason = reason;
			}
			return false;
		}
	};
}
