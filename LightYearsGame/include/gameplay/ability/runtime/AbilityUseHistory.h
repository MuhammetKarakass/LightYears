#pragma once

#include "abilities/AbilityEvent.h"
#include "attributes/AttributeSystem.h"
#include "framework/Core.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <string>

namespace ly
{
	struct AbilityScalingChannel
	{
		sas::AttributeId targetAttributeId;
		sas::AttributeId sourceAttributeId;
		// Echo must preserve the source channel's operation and coefficient when
		// one source attribute drives more than one target. For example, Gravity
		// Anomaly uses MaxHealth for both Radius and Duration, but those targets
		// intentionally have very different coefficients.
		sas::AttributeModifierOperation operation =
			sas::AttributeModifierOperation::Add;
		float sourceCoefficient = 0.f;
	};

	// This is a value snapshot. It deliberately owns copied metadata rather
	// than pointers to a live GameAbility, Actor or behavior instance.
	struct AbilityUseRecord
	{
		std::uint64_t sequence = 0;
		sas::AbilityHandle abilityHandle;
		sas::ContentId abilityId;
		sas::AbilitySlot slot = sas::AbilitySlot::None;
		int level = 1;
		int maxLevel = 1;
		List<GameplayTag> abilityTags;
		List<std::string> unlockedUpgradeIds;
		List<AbilityScalingChannel> scalingChannels;
		sas::AbilityActivationOrigin activationOrigin =
			sas::AbilityActivationOrigin::NormalInput;
		// Consumed records stay in the shared history for future systems. Echo
		// keeps its own sequence cursor so it cannot fall back to older records.
		bool consumed = false;
	};

	class AbilityUseHistory
	{
	public:
		static constexpr std::size_t MaxRecords = 10;

		std::uint64_t Record(AbilityUseRecord record);
		// Returns the newest unconsumed snapshot. Consumed records remain in the
		// shared history because other future systems may still inspect them.
		const AbilityUseRecord* FindLatestUnconsumed(
			const std::function<bool(const AbilityUseRecord&)>& predicate = {}
		) const;
		bool Consume(std::uint64_t sequence);
		void Clear();

		const std::deque<AbilityUseRecord>& GetRecords() const
		{
			return mRecords;
		}

	private:
		std::deque<AbilityUseRecord> mRecords;
		std::uint64_t mNextSequence = 1;
	};
}
