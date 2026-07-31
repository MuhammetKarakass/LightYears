#pragma once

#include "AbilitySystemComponent.h"
#include "abilities/AbilityEvent.h"
#include "effects/GameplayEffectBehaviorRuntime.h"
#include "framework/Core.h"
#include "gameplay/ability/GameAbility.h"

namespace ly
{
	class Actor;
	struct AttachmentDefinition;
	struct DamageContext;
	enum class AttachmentHostKind;

	class LightYearsAbilitySystemComponent
		: public sas::AbilitySystemComponent
	{
	public:
		enum class IncomingDamagePhase
		{
			PreMitigation,
			Standard
		};

		using EffectBehaviorRuntime =
			sas::GameplayEffectBehaviorRuntime<
				sas::ActiveGameplayEffect,
				Actor,
				DamageContext,
				IncomingDamagePhase
			>;

		static constexpr size_t MaxPassiveAbilities = 2;

		explicit LightYearsAbilitySystemComponent(Actor& owner);
		void InitializeOwnerAttributes(float maxHealth);

		static EffectBehaviorRuntime& GetEffectBehaviorRuntime();
		static bool RegisterGameContent();
		static bool ValidateDefinition(const GameAbilityDefinition& definition, std::string* failureReason = nullptr);
		static bool ValidateCatalog(
			const List<const GameAbilityDefinition*>& definitions,
			std::string* failureReason = nullptr
		);
		static bool ValidateShippedAbilityDefinitions(
			std::string* failureReason = nullptr
		);
		static bool ValidateGameplayEffectDefinition(
			const sas::GameplayEffectDefinition& definition,
			std::string* failureReason = nullptr
		);
		static bool ValidateShippedGameplayEffectDefinitions(
			std::string* failureReason = nullptr
		);

		bool TryEquipAttachment(
			sas::AbilityHandle handle,
			const AttachmentDefinition& definition,
			AttachmentHostKind hostKind,
			std::string* failureReason = nullptr
		);
		bool TryEquipAttachment(
			sas::AbilitySlot slot,
			const AttachmentDefinition& definition,
			AttachmentHostKind hostKind,
			std::string* failureReason = nullptr
		);
		bool RemoveAttachment(sas::AbilityHandle handle, const GameplayTag& attachmentId, AttachmentHostKind hostKind);
		GameAbility* GetAbility(sas::AbilitySlot slot);
		const GameAbility* GetAbility(sas::AbilitySlot slot) const;
		GameAbility* GetAbility(sas::AbilityHandle handle);
		const GameAbility* GetAbility(sas::AbilityHandle handle) const;
		GameAbility* GetAbilityById(const std::string& abilityId);
		const GameAbility* GetAbilityById(const std::string& abilityId) const;
		Actor& GetOwner() { return mOwner; }

	private:
		void ProcessGameGameplayEvent(const sas::AbilityEvent& event);

		Actor& mOwner;
		using ComponentRuntime =
			sas::AbilitySystemRuntime<GameAbilityDefinition, GameAbility>;
		ComponentRuntime& mComponentRuntime;
	};
}


