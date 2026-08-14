#pragma once

#include "framework/Core.h"
#include "framework/Delegate.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/input/AbilityInputSchema.h"
#include <string>

namespace ly
{
	struct AbilitySlotUIData
	{
		sas::AbilitySlot slot = sas::AbilitySlot::Ability1;
		std::string displayName;
		std::string iconPath;
		std::string inputLabel;
		sf::Color accentColor = sf::Color::White;
		float cooldownPercent = 0.f;      // 0.f = ready, 1.f = fully on cooldown
		float cooldownRemaining = 0.f;    // actual seconds remaining
		bool isActive = false;
		float activePercent = 0.f;        // 1.f -> 0.f as duration counts down
		bool isAvailable = false;         // slot has a controller assigned
	};

	class AbilityUIViewModel
	{
	public:
		void RegisterSlot(sas::AbilitySlot slot, const GameAbilityDefinition& definition);
		void UpdateCooldown(sas::AbilitySlot slot, float remaining, float total);
		void UpdateActive(sas::AbilitySlot slot, bool active, float remaining, float total);
		void SetAvailable(sas::AbilitySlot slot, bool available);

		const AbilitySlotUIData* GetSlotData(sas::AbilitySlot slot) const;
		const Map<sas::AbilitySlot, AbilitySlotUIData>& GetAllSlotData() const { return mSlotData; }

		Delegate<sas::AbilitySlot> onSlotDataChanged;

	private:
		AbilitySlotUIData& GetOrCreateSlotData(sas::AbilitySlot slot);

		Map<sas::AbilitySlot, AbilitySlotUIData> mSlotData;
	};

	inline void AbilityUIViewModel::RegisterSlot(sas::AbilitySlot slot, const GameAbilityDefinition& definition)
	{
		AbilitySlotUIData& data = GetOrCreateSlotData(slot);
		data.displayName = definition.displayName;
		data.iconPath = definition.iconPath;
		// The key label belongs to the shared input schema, not to ability
		// content. This remains correct after a runtime slot move.
		data.inputLabel = AbilityInputSchema::GetLabel(slot);
		data.accentColor = definition.accentColor;
		data.isAvailable = true;
		onSlotDataChanged.Broadcast(slot);
	}

	inline void AbilityUIViewModel::UpdateCooldown(sas::AbilitySlot slot, float remaining, float total)
	{
		AbilitySlotUIData& data = GetOrCreateSlotData(slot);
		data.cooldownPercent = (total > 0.f) ? (remaining / total) : 0.f;
		data.cooldownRemaining = remaining;
		onSlotDataChanged.Broadcast(slot);
	}

	inline void AbilityUIViewModel::UpdateActive(sas::AbilitySlot slot, bool active, float remaining, float total)
	{
		AbilitySlotUIData& data = GetOrCreateSlotData(slot);
		data.isActive = active;
		data.activePercent = (active && total > 0.f) ? (remaining / total) : 0.f;
		onSlotDataChanged.Broadcast(slot);
	}

	inline void AbilityUIViewModel::SetAvailable(sas::AbilitySlot slot, bool available)
	{
		AbilitySlotUIData& data = GetOrCreateSlotData(slot);
		data.isAvailable = available;
		onSlotDataChanged.Broadcast(slot);
	}

	inline const AbilitySlotUIData* AbilityUIViewModel::GetSlotData(sas::AbilitySlot slot) const
	{
		auto found = mSlotData.find(slot);
		return found != mSlotData.end() ? &found->second : nullptr;
	}

	inline AbilitySlotUIData& AbilityUIViewModel::GetOrCreateSlotData(sas::AbilitySlot slot)
	{
		auto found = mSlotData.find(slot);
		if (found != mSlotData.end())
		{
			return found->second;
		}

		AbilitySlotUIData newData;
		newData.slot = slot;
		mSlotData[slot] = newData;
		return mSlotData[slot];
	}
}


