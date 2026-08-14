#pragma once

#include "abilities/AbilityHandle.h"
#include "abilities/AbilityRuntimeSnapshot.h"
#include "abilities/AbilityRuntimeState.h"

namespace sas
{
	template <typename Definition, typename Execution>
	class AbilityRuntimeEntry
	{
	protected:
		AbilityRuntimeEntry(
			AbilityHandle handle,
			const Definition& definition,
			int initialCharges
		)
			: mHandle{ handle },
			mBaseDefinition{ definition },
			mDefinition{ definition },
			mRuntimeState{ initialCharges }
		{
		}

		AbilityRuntimeSnapshot BuildRuntimeSnapshot(
			int maxLevel,
			float cooldownDuration,
			float activeDuration
		) const
		{
			return AbilityRuntimeSnapshot{
				mHandle,
				mDefinition.abilityId,
				mDefinition.slot,
				mRuntimeState.GetLevel(),
				maxLevel,
				mRuntimeState.IsActive(),
				mRuntimeState.GetCooldownRemaining(),
				cooldownDuration,
				mRuntimeState.GetActiveTimeRemaining(),
				activeDuration,
				mRuntimeState.GetCharges()
			};
		}

	public:
		void SetRuntimeSlot(AbilitySlot slot)
		{
			// Level rebuilds start from mBaseDefinition, so keep the effective
			// binding in both runtime copies until the legacy definition slot is
			// fully removed from the content model.
			mBaseDefinition.slot = slot;
			mDefinition.slot = slot;
		}

		AbilitySlot GetRuntimeSlot() const
		{
			return mDefinition.slot;
		}

	protected:

		AbilityHandle mHandle;
		Definition mBaseDefinition;
		Definition mDefinition;
		AbilityRuntimeState mRuntimeState;
		Execution mExecution;
	};
}
