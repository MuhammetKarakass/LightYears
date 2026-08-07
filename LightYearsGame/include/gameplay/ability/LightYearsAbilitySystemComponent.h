#pragma once

#include "AbilitySystemComponent.h"
#include "abilities/AbilityEvent.h"
#include "gameplay/ability/GameAbility.h"

namespace ly
{
	class Actor;
	struct AttachmentDefinition;
	enum class AttachmentHostKind;

	class LightYearsAbilitySystemComponent
		: public sas::AbilitySystemComponent
	{
	public:
		static constexpr size_t MaxPassiveAbilities = 2;

		explicit LightYearsAbilitySystemComponent(Actor& owner);


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
		bool RemoveAttachment(sas::AbilityHandle handle, const std::string& attachmentId, AttachmentHostKind hostKind);
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


