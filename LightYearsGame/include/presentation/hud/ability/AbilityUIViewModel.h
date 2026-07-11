#pragma once

#include "framework/Core.h"
#include "framework/Delegate.h"
#include "gameConfigs/AbilityStructs.h"
#include <string>

namespace ly
{
	struct AbilitySlotUIData
	{
		AbilitySlot slot = AbilitySlot::Ability1;
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
		void RegisterSlot(AbilitySlot slot, const AbilityDefinition& definition);
		void UpdateCooldown(AbilitySlot slot, float remaining, float total);
		void UpdateActive(AbilitySlot slot, bool active, float remaining, float total);
		void SetAvailable(AbilitySlot slot, bool available);

		const AbilitySlotUIData* GetSlotData(AbilitySlot slot) const;
		const Map<AbilitySlot, AbilitySlotUIData>& GetAllSlotData() const { return mSlotData; }

		Delegate<AbilitySlot> onSlotDataChanged;

	private:
		AbilitySlotUIData& GetOrCreateSlotData(AbilitySlot slot);

		Map<AbilitySlot, AbilitySlotUIData> mSlotData;
	};

	inline void AbilityUIViewModel::RegisterSlot(AbilitySlot slot, const AbilityDefinition& definition)
	{
		AbilitySlotUIData& data = GetOrCreateSlotData(slot);
		data.displayName = definition.displayName;
		data.iconPath = definition.iconPath;
		data.inputLabel = definition.inputLabel;
		data.accentColor = definition.accentColor;
		data.isAvailable = true;
		onSlotDataChanged.Broadcast(slot);
	}

	inline void AbilityUIViewModel::UpdateCooldown(AbilitySlot slot, float remaining, float total)
	{
		AbilitySlotUIData& data = GetOrCreateSlotData(slot);
		data.cooldownPercent = (total > 0.f) ? (remaining / total) : 0.f;
		data.cooldownRemaining = remaining;
		onSlotDataChanged.Broadcast(slot);
	}

	inline void AbilityUIViewModel::UpdateActive(AbilitySlot slot, bool active, float remaining, float total)
	{
		AbilitySlotUIData& data = GetOrCreateSlotData(slot);
		data.isActive = active;
		data.activePercent = (active && total > 0.f) ? (remaining / total) : 0.f;
		onSlotDataChanged.Broadcast(slot);
	}

	inline void AbilityUIViewModel::SetAvailable(AbilitySlot slot, bool available)
	{
		AbilitySlotUIData& data = GetOrCreateSlotData(slot);
		data.isAvailable = available;
		onSlotDataChanged.Broadcast(slot);
	}

	inline const AbilitySlotUIData* AbilityUIViewModel::GetSlotData(AbilitySlot slot) const
	{
		auto found = mSlotData.find(slot);
		return found != mSlotData.end() ? &found->second : nullptr;
	}

	inline AbilitySlotUIData& AbilityUIViewModel::GetOrCreateSlotData(AbilitySlot slot)
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


