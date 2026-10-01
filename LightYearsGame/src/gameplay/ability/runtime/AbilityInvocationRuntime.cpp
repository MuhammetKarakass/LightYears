#include "gameplay/ability/runtime/AbilityInvocationRuntime.h"

#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/content/AbilityContentCatalog.h"

#include <algorithm>
#include <exception>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace ly
{
	AbilityInvocationRuntime::AbilityInvocationRuntime(
		LightYearsAbilitySystemComponent& abilitySystem
	)
		: mAbilitySystem{ abilitySystem }
	{
	}

	GameAbilityDefinition AbilityInvocationRuntime::BuildInvocationDefinition(
		const GameAbilityDefinition& sourceDefinition,
		const AbilityUseRecord& record,
		const List<sas::AttributeScalingRule>& scalingRules,
		float outputMultiplier
	)
	{
		GameAbilityDefinition definition = sourceDefinition;
		definition.levelProgression.clear();
		definition.repeatingLevelProgression.clear();
		definition.scrapCost = {};
		definition.attributeModifiers.clear();
		definition.unlockedUpgradeIds = record.unlockedUpgradeIds;
		definition.scalingRules = scalingRules;
		definition.attributeOutputMultiplier = std::max(0.f, outputMultiplier);
		definition.recordInAbilityHistory = false;

		// The history record is a value snapshot, so the replay must rebuild the
		// source level's numeric balance exactly like GameAbility does. Dropping
		// these modifiers made Echo silently replay level-one actor values even
		// when the recorded ability had already been upgraded.
		definition.actions = sourceDefinition.actions;
		definition.triggers = sourceDefinition.triggers;
		const int sourceSteps = std::max(0, record.level - 1);
		for (int stepIndex = 0; stepIndex < sourceSteps; ++stepIndex)
		{
			const AbilityLevelStep* stepPtr = sourceDefinition.ResolveLevelStep(
				static_cast<std::size_t>(stepIndex)
			);
			if (!stepPtr)
			{
				break;
			}
			const AbilityLevelStep& step = *stepPtr;
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
			{
				if (modifier.attributeId == CommonAttributeIds::Cooldown)
				{
					continue;
				}
				definition.attributeModifiers.push_back(modifier);
			}
			for (const std::string& upgradeId : step.unlockedUpgradeIds)
			{
				const bool alreadyUnlocked = std::any_of(
					definition.unlockedUpgradeIds.begin(),
					definition.unlockedUpgradeIds.end(),
					[&](const std::string& existingUpgradeId)
					{
						return existingUpgradeId == upgradeId;
					}
				);
				if (!alreadyUnlocked)
				{
					definition.unlockedUpgradeIds.push_back(upgradeId);
				}
			}
			const bool selected = std::any_of(
				step.unlockedUpgradeIds.begin(),
				step.unlockedUpgradeIds.end(),
				[&](const std::string& upgradeId)
				{
					return std::find(
						record.unlockedUpgradeIds.begin(),
						record.unlockedUpgradeIds.end(),
						upgradeId
					) != record.unlockedUpgradeIds.end();
				}
			);
			if (!selected)
			{
				continue;
			}
			definition.actions.insert(
				definition.actions.end(),
				step.addedActions.begin(),
				step.addedActions.end()
			);
			definition.triggers.insert(
				definition.triggers.end(),
				step.addedTriggers.begin(),
				step.addedTriggers.end()
			);
		}
		if (sourceSteps > 0)
		{
			definition.attributeModifiers.emplace_back(
				CommonAttributeIds::Cooldown,
				-GetGlobalAbilityCooldownTotalReduction(
					sourceDefinition.cooldown,
					static_cast<std::size_t>(sourceSteps)
				)
			);
		}
		return definition;
	}

	bool AbilityInvocationRuntime::Invoke(
		const AbilityUseRecord& record,
		const List<sas::AttributeScalingRule>& scalingRules,
		float outputMultiplier,
		sas::AbilitySlot controlSlot,
		bool inputHeld,
		std::string* failureReason
	)
	{
		const GameAbilityDefinition* sourceDefinition =
			content::AbilityContentCatalog::FindById(record.abilityId.ToString());
		if (!sourceDefinition)
		{
			if (failureReason)
			{
				*failureReason = "Echo source ability is not present in the content catalog.";
			}
			return false;
		}

		GameAbilityDefinition invocationDefinition = BuildInvocationDefinition(
			*sourceDefinition,
			record,
			scalingRules,
			outputMultiplier
		);
		unique_ptr<GameAbilityBehavior> behavior = GameAbilityBehaviorRegistry::Create(
			invocationDefinition.behaviorType
		);
		if (!behavior)
		{
			if (failureReason)
			{
				*failureReason = "Echo source ability behavior could not be created.";
			}
			return false;
		}

		unique_ptr<GameAbility> ability = std::make_unique<GameAbility>(
			mAbilitySystem,
			sas::AbilityHandle{},
			invocationDefinition,
			std::move(behavior),
			false
		);
		ability->SetActivationOrigin(sas::AbilityActivationOrigin::EchoInvocation);
		ability->ConfigureInvocationLevel(record.level, record.maxLevel);
		ability->SetInputHeld(inputHeld);
		const auto retainIfCleanupRemains = [&]()
		{
			if (ability && (ability->IsActive() || ability->HasPendingCleanup()))
			{
				mActiveInvocations.push_back(ActiveInvocation{
					std::move(ability),
					controlSlot
				});
			}
		};
		bool activated = false;
		try
		{
			activated = ability->TryActivate();
		}
		catch (...)
		{
			const std::exception_ptr error = std::current_exception();
			retainIfCleanupRemains();
			std::rethrow_exception(error);
		}

		if (!activated)
		{
			retainIfCleanupRemains();
			if (failureReason)
			{
				*failureReason = "Echo source ability rejected the invocation.";
			}
			return false;
		}

		retainIfCleanupRemains();
		return true;
	}

	void AbilityInvocationRuntime::SetControlInput(
		sas::AbilitySlot controlSlot,
		bool inputHeld
	)
	{
		for (ActiveInvocation& invocation : mActiveInvocations)
		{
			if (invocation.controlSlot == controlSlot && invocation.ability)
			{
				invocation.ability->SetInputHeld(inputHeld);
			}
		}
	}

	void AbilityInvocationRuntime::Tick(float deltaTime)
	{
		for (std::size_t index = 0; index < mActiveInvocations.size();)
		{
			GameAbility* ability = mActiveInvocations[index].ability.get();
			if (!ability)
			{
				mActiveInvocations.erase(mActiveInvocations.begin() + index);
				continue;
			}

			ability->Tick(deltaTime);
			if (!ability->IsActive() && !ability->HasPendingCleanup())
			{
				mActiveInvocations.erase(mActiveInvocations.begin() + index);
				continue;
			}
			++index;
		}
	}

	void AbilityInvocationRuntime::Clear()
	{
		std::vector<ActiveInvocation> retained;
		retained.reserve(mActiveInvocations.size());
		std::vector<ActiveInvocation> invocations;
		invocations.swap(mActiveInvocations);
		std::exception_ptr error;
		for (ActiveInvocation& invocation : invocations)
		{
			if (invocation.ability)
			{
				try { invocation.ability->Cancel(sas::AbilityEndReason::OwnerDestroyed); }
				catch (...) { if (!error) error = std::current_exception(); }
				if (invocation.ability->IsActive() || invocation.ability->HasPendingCleanup())
				{
					if (!error)
					{
						error = std::make_exception_ptr(std::runtime_error(
							"Ability invocation cleanup is incomplete; retry Clear after cleanup succeeds."
						));
					}
					retained.push_back(std::move(invocation));
				}
			}
		}
		if (!retained.empty())
		{
			if (mActiveInvocations.empty())
			{
				mActiveInvocations.swap(retained);
			}
			else
			{
				mActiveInvocations.insert(
					mActiveInvocations.end(),
					std::make_move_iterator(retained.begin()),
					std::make_move_iterator(retained.end())
				);
			}
		}
		if (error) std::rethrow_exception(error);
	}
}
