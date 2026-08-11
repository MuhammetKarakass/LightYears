#pragma once

#include "framework/Core.h"
#include "gameplay/tags/GameplayTags.h"

#include <cctype>
#include <string>

namespace ly
{
	// This schema owns only project-wide tag domains and shared semantic tags.
	// Ability/weapon/effect families keep their own leaf tags beside their feature.
	enum class GameplayTagKind
	{
		Any,
		Ability,
		AbilityCategory,
		AbilityState,
		Event,
		AbilityEvent,
		Status,
		EffectState,
		ActionLock,
		DamageType,
		AttachmentCapability,
		Cooldown
	};

	class GameplayTagSchema final
	{
	public:
		inline static const GameplayTag& AbilityRoot = GameplayTags::Ability::Root;
		inline static const GameplayTag& AbilityPrimary = GameplayTags::Ability::Primary;
		inline static const GameplayTag& AbilityOffense = GameplayTags::Ability::Offense;
		inline static const GameplayTag& AbilityDefense = GameplayTags::Ability::Defense;
		inline static const GameplayTag& AbilityMovement = GameplayTags::Ability::Movement;
		inline static const GameplayTag& AbilityControl = GameplayTags::Ability::Control;
		inline static const GameplayTag& AbilityUtility = GameplayTags::Ability::Utility;
		inline static const GameplayTag& AbilityStateRoot = GameplayTags::State::Ability::Root;
		inline static const GameplayTag& EventRoot = GameplayTags::Event::Root;
		inline static const GameplayTag& AbilityEventRoot = GameplayTags::Event::Ability::Root;
		inline static const GameplayTag& EventOwnerRoot = GameplayTags::Event::Owner::Root;
		inline static const GameplayTag& EventSourceRoot = GameplayTags::Event::Source::Root;
		inline static const GameplayTag& StatusRoot = GameplayTags::Status::Root;
		inline static const GameplayTag& EffectStateRoot = GameplayTags::State::Effect::Root;
		inline static const GameplayTag& ActionLockRoot = GameplayTags::State::ActionLock::Root;
		inline static const GameplayTag& DamageTypeRoot = GameplayTags::Damage::Type::Root;
		inline static const GameplayTag& AttachmentRoot = GameplayTags::Attachment::Root;
		inline static const GameplayTag& AttachmentCapabilityRoot = GameplayTags::Attachment::Capability::Root;
		inline static const GameplayTag& CooldownRoot = GameplayTags::Cooldown::Root;

		// These locks are intentionally shared. A producing ability grants one of
		// them; every GameAbility consumes the matching lock during activation.
		inline static const GameplayTag& BlockAbilityActivation = GameplayTags::State::ActionLock::AbilityActivation;
		inline static const GameplayTag& BlockPrimaryWeaponFire = GameplayTags::State::ActionLock::PrimaryWeaponFire;

		static bool Validate(
			const GameplayTag& tag,
			GameplayTagKind kind,
			std::string* failureReason = nullptr
		)
		{
			if (tag.name.rfind("Attribute.", 0) == 0)
			{
				return Fail(failureReason, "Numeric gameplay values must use AttributeId, not GameplayTag.");
			}

			if (!IsCanonical(tag))
			{
				return Fail(failureReason, "Gameplay tag must use non-empty dot-separated identifier segments.");
			}

			if (tag.MatchesTag(ActionLockRoot) && !IsActionLock(tag))
			{
				return Fail(failureReason, "State.ActionLock only permits registered shared action-lock tags.");
			}

			const bool matchesKind = [&]()
			{
				switch (kind)
				{
				case GameplayTagKind::Any:
					return true;
				case GameplayTagKind::Ability:
					return IsChildOf(tag, AbilityRoot);
				case GameplayTagKind::AbilityCategory:
					return IsAbilityCategory(tag);
				case GameplayTagKind::AbilityState:
					return IsChildOf(tag, AbilityStateRoot);
				case GameplayTagKind::Event:
					return IsChildOf(tag, EventRoot);
				case GameplayTagKind::AbilityEvent:
					return IsChildOf(tag, AbilityEventRoot);
				case GameplayTagKind::Status:
					return IsChildOf(tag, StatusRoot);
				case GameplayTagKind::EffectState:
					return IsChildOf(tag, EffectStateRoot);
				case GameplayTagKind::ActionLock:
					return IsActionLock(tag);
				case GameplayTagKind::DamageType:
					return IsChildOf(tag, DamageTypeRoot);
				case GameplayTagKind::AttachmentCapability:
					return IsChildOf(tag, AttachmentCapabilityRoot);
				case GameplayTagKind::Cooldown:
					return IsChildOf(tag, CooldownRoot);
				}
				return false;
			}();

			return matchesKind || Fail(failureReason, "Gameplay tag does not belong to the expected project-wide domain.");
		}

		static bool IsActionLock(const GameplayTag& tag)
		{
			return tag.MatchesTagExact(BlockAbilityActivation) ||
				tag.MatchesTagExact(BlockPrimaryWeaponFire);
		}

		static bool IsAbilityCategory(const GameplayTag& tag)
		{
			return tag.MatchesTagExact(AbilityPrimary) ||
				tag.MatchesTagExact(AbilityOffense) ||
				tag.MatchesTagExact(AbilityDefense) ||
				tag.MatchesTagExact(AbilityMovement) ||
				tag.MatchesTagExact(AbilityControl) ||
				tag.MatchesTagExact(AbilityUtility);
		}

		// Granted effect state is deliberately narrower than arbitrary GameplayTag
		// data: a gameplay effect may grant a State.Effect.* or Status.* marker.
		static bool ValidateEffectGrantedTag(
			const GameplayTag& tag,
			std::string* failureReason = nullptr
		)
		{
			if (Validate(tag, GameplayTagKind::EffectState, nullptr) ||
				Validate(tag, GameplayTagKind::Status, nullptr))
			{
				return true;
			}
			return Fail(
				failureReason,
				"Gameplay effect granted tags must belong to State.Effect.* or Status.*."
			);
		}

		// Conditions can observe state produced by abilities and events in addition
		// to effect/status state, but they still cannot use unrelated data domains
		// such as raw numeric attribute IDs or Damage.Type.* tags.
		static bool ValidateEffectApplicationTag(
			const GameplayTag& tag,
			std::string* failureReason = nullptr
		)
		{
			if (Validate(tag, GameplayTagKind::EffectState, nullptr) ||
				Validate(tag, GameplayTagKind::Status, nullptr) ||
				Validate(tag, GameplayTagKind::Ability, nullptr) ||
				Validate(tag, GameplayTagKind::AbilityState, nullptr) ||
				Validate(tag, GameplayTagKind::Event, nullptr) ||
				Validate(tag, GameplayTagKind::ActionLock, nullptr) ||
				Validate(tag, GameplayTagKind::Cooldown, nullptr))
			{
				return true;
			}
			return Fail(
				failureReason,
				"Gameplay effect application tags must belong to an allowed effect, status, ability, state, event, or action-lock domain."
			);
		}

		// Ability activation conditions read persistent owner state. Events are
		// deliberately excluded: they are transient signals, not owner state that
		// can safely gate an ability on a later frame.
		static bool ValidateAbilityOwnerConditionTag(
			const GameplayTag& tag,
			std::string* failureReason = nullptr
		)
		{
			if (Validate(tag, GameplayTagKind::EffectState, nullptr) ||
				Validate(tag, GameplayTagKind::Status, nullptr) ||
				Validate(tag, GameplayTagKind::Ability, nullptr) ||
				Validate(tag, GameplayTagKind::AbilityState, nullptr) ||
				Validate(tag, GameplayTagKind::ActionLock, nullptr) ||
				Validate(tag, GameplayTagKind::Cooldown, nullptr))
			{
				return true;
			}
			return Fail(
				failureReason,
				"Ability owner condition tags must belong to State.Effect.*, Status.*, Ability.*, State.Ability.*, or State.ActionLock.*."
			);
		}

	private:
		static bool Fail(std::string* failureReason, const char* reason)
		{
			if (failureReason)
			{
				*failureReason = reason;
			}
			return false;
		}

		static bool IsCanonical(const GameplayTag& tag)
		{
			const std::string& value = tag.name;
			if (value.empty() || value.front() == '.' || value.back() == '.')
			{
				return false;
			}

			bool beginsSegment = true;
			for (const unsigned char character : value)
			{
				if (character == '.')
				{
					if (beginsSegment)
					{
						return false;
					}
					beginsSegment = true;
					continue;
				}
				if (!std::isalnum(character) && character != '_')
				{
					return false;
				}
				beginsSegment = false;
			}
			return !beginsSegment;
		}

		static bool IsChildOf(const GameplayTag& tag, const GameplayTag& root)
		{
			return tag.name != root.name && tag.MatchesTag(root);
		}

	};
}
