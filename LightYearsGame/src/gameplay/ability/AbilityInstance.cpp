#include "gameplay/ability/AbilityInstance.h"
#include "gameplay/attributes/AttributeMath.h"
#include "gameplay/ability/AbilityExecutor.h"
#include "gameplay/ability/AbilitySystem.h"
#include "gameplay/ability/AbilityEvent.h"
#include "gameplay/attributes/AttributeSystem.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"
#include <algorithm>
#include <variant>

namespace ly
{
	AbilityInstance::AbilityInstance(
		AbilitySystem& abilitySystem,
		AbilityHandle handle,
		const AbilityDefinition& definition,
		unique_ptr<AbilityBehavior> behavior)
		: mAbilitySystem{ &abilitySystem },
		mHandle{ handle },
		mBaseDefinition{ definition },
		mDefinition{ definition },
		mLevel{ 1 },
		mCharges{ definition.maxCharges },
		mBehavior{ std::move(behavior) }
	{
	}

	void AbilityInstance::SetInputHeld(bool inputHeld)
	{
		mInputHeld = inputHeld;
	}

	void AbilityInstance::SetWeaponFireIntervalRemaining(float interval)
	{
		mWeaponFireIntervalRemaining = std::max(0.f, interval);
	}

	bool AbilityInstance::TryEquipAttachment(
		const AttachmentDefinition& definition,
		AttachmentHostKind hostKind,
		std::string* failureReason
	)
	{
		const bool equipped = mAttachments.TryEquip(
			definition,
			hostKind,
			GetAttachmentCapabilities(hostKind),
			GetAttachmentSlotCapacity(hostKind),
			failureReason
		);
		if (equipped && mAbilitySystem)
		{
			mAbilitySystem->NotifyAbilityChanged(mHandle);
		}
		return equipped;
	}

	bool AbilityInstance::RemoveAttachment(const GameplayTag& attachmentId, AttachmentHostKind hostKind)
	{
		const bool removed = mAttachments.Remove(attachmentId, hostKind);
		if (removed && mAbilitySystem)
		{
			mAbilitySystem->NotifyAbilityChanged(mHandle);
		}
		return removed;
	}

	GameplayAttributeList AbilityInstance::MergeAttachmentAttributes(
		AttachmentHostKind hostKind,
		const GameplayAttributeList& attributes
	) const
	{
		return mAttachments.MergeGrantedAttributes(hostKind, attributes);
	}

	GameplayAttribute AbilityInstance::ApplyAttachmentModifiers(
		AttachmentHostKind hostKind,
		const GameplayAttribute& attribute
	) const
	{
		return mAttachments.ApplyStaticModifiers(hostKind, attribute);
	}

	GameplayAttributeList AbilityInstance::ApplyAttachmentConditions(
		AttachmentHostKind hostKind,
		const GameplayAttributeList& attributes,
		const List<GameplayTag>& originalDamageTags
	) const
	{
		return mAttachments.ApplyConditionalModifiers(hostKind, attributes, originalDamageTags);
	}

	List<GameplayTag> AbilityInstance::GetResolvedDamageTags(AttachmentHostKind hostKind) const
	{
		const List<GameplayTag> baseDamageTags = mDefinition.damageTags.empty()
			? List<GameplayTag>{ DamageTypeSchema::Photonic }
			: mDefinition.damageTags;
		return mAttachments.ResolveDamageTags(hostKind, baseDamageTags);
	}

	void AbilityInstance::HandleAttachmentEvent(const AbilityEvent& event)
	{
		if (!mAbilitySystem)
		{
			return;
		}

		for (const EquippedAttachment& equipped : mAttachments.GetEquipped())
		{
			for (const AttachmentEventRule& rule : equipped.definition.eventRules)
			{
				if (!event.eventTag.MatchesTag(rule.eventTag) ||
					(rule.requireOwnerAsEventSource && event.source != &mAbilitySystem->GetOwner()))
				{
					continue;
				}

				const List<GameplayTag>* damageTags = event.damageContext
					? &event.damageContext->damageTags
					: nullptr;
				const bool matchesDamageTags = std::all_of(
					rule.requiredDamageTags.begin(),
					rule.requiredDamageTags.end(),
					[&](const GameplayTag& requiredTag)
					{
						return damageTags && std::any_of(damageTags->begin(), damageTags->end(), [&](const GameplayTag& tag)
						{
							return tag.MatchesTag(requiredTag);
						});
					}
				);
				if (!matchesDamageTags)
				{
					continue;
				}

				const float magnitude = mAttachments.ResolveGrantedAttributeValue(
					equipped.hostKind,
					rule.magnitudeAttributeId,
					rule.baseMagnitude
				);
				if (magnitude <= 0.f)
				{
					continue;
				}

				switch (rule.action)
				{
				case AttachmentEventAction::ReduceCooldown:
					switch (rule.cooldownTarget)
					{
					case AttachmentCooldownTarget::Host:
						ReduceCooldownRemaining(magnitude);
						break;
					case AttachmentCooldownTarget::AllOwnerAbilities:
						mAbilitySystem->ReduceCooldowns(magnitude, true);
						break;
					case AttachmentCooldownTarget::AllNonPrimaryAbilities:
						mAbilitySystem->ReduceCooldowns(magnitude, false);
						break;
					}
					break;
				}
			}
		}
	}

	void AbilityInstance::ReduceCooldownRemaining(float amount)
	{
		if (amount <= 0.f || mCooldownRemaining <= 0.f)
		{
			return;
		}
		const float previous = mCooldownRemaining;
		mCooldownRemaining = std::max(0.f, mCooldownRemaining - amount);
		if (mAbilitySystem && previous != mCooldownRemaining)
		{
			mAbilitySystem->NotifyAbilityChanged(mHandle);
		}
	}

	bool AbilityInstance::SetLevel(int level)
	{
		const int newLevel = std::clamp(level, 1, mBaseDefinition.GetMaxLevel());
		if (newLevel == mLevel)
		{
			return false;
		}

		if (mIsActive)
		{
			EndAbility(AbilityEndReason::Interrupted);
		}
		mLevel = newLevel;
		RebuildDefinitionForLevel();
		RefreshPrimaryWeaponRuntimeConfiguration();
		if (mAbilitySystem)
		{
			mAbilitySystem->NotifyAbilityLevelChanged(mHandle, mLevel);
		}
		return true;
	}

	void AbilityInstance::Tick(float deltaTime)
	{
		UpdateCooldown(deltaTime);
		UpdateInputActivation();

		if (mIsActive)
		{
			TickActiveExecution(deltaTime);

			if (mDefinition.lifetimePolicy == AbilityLifetimePolicy::Duration && mActiveTimeRemaining > 0.f)
			{
				const float previousActiveTimeRemaining = mActiveTimeRemaining;
				mActiveTimeRemaining = std::max(0.f, mActiveTimeRemaining - deltaTime);
				if (mAbilitySystem && previousActiveTimeRemaining != mActiveTimeRemaining)
				{
					mAbilitySystem->NotifyAbilityChanged(mHandle);
				}
				if (mActiveTimeRemaining <= 0.f)
				{
					EndAbility(AbilityEndReason::DurationExpired);
				}
			}
		}
		else
		{
			TickInactivePrimaryWeaponRuntime(deltaTime);
			UpdateWeaponFireInterval(deltaTime);
		}

		mWasInputHeld = mInputHeld;
	}

	bool AbilityInstance::TryActivate()
	{
		if (mIsActive || IsOnCooldown())
		{
			return false;
		}

		if (mDefinition.maxCharges > 0 && mCharges <= 0)
		{
			return false;
		}
		if (mAbilitySystem &&
			(!mAbilitySystem->HasAllOwnerTags(mDefinition.requiredOwnerTags) ||
				mAbilitySystem->HasAnyOwnerTags(mDefinition.blockedOwnerTags)))
		{
			return false;
		}
		if (!mAbilitySystem || !mBehavior)
		{
			return false;
		}

		AbilityBehaviorContext behaviorContext{
			*mAbilitySystem,
			*this,
			mAbilitySystem->GetOwner(),
			mDefinition
		};
		if (!mBehavior->Activate(behaviorContext))
		{
			return false;
		}

		if (mDefinition.maxCharges > 0)
		{
			--mCharges;
		}

		mIsActive = true;
		mActiveTimeRemaining = GetActiveDuration();
		mExecution.actions.clear();
		if (mAbilitySystem)
		{
			mAbilitySystem->NotifyAbilityActivated(mHandle);
		}

		AbilityExecutionContext context{ mAbilitySystem, &mDefinition, nullptr, this };
		AbilityExecutor::BeginExecution(mExecution, context);

		if (mDefinition.lifetimePolicy == AbilityLifetimePolicy::Instant)
		{
			EndAbility(AbilityEndReason::Completed);
		}

		return true;
	}

	void AbilityInstance::Cancel(AbilityEndReason reason)
	{
		if (mIsActive)
		{
			EndAbility(reason);
		}
	}

	float AbilityInstance::GetCooldownDuration() const
	{
		/*
		 * Effective cooldown order:
		 * 1. Start with this AbilityInstance's private definition copy.
		 * 2. Ability-specific levels mutate only this copy: base cooldown + summed Cooldown value modifiers.
		 * 3. Ship-wide AbilityHaste rating is converted by the shared asymptotic
		 *    curve. Cooldown reduction therefore has diminishing returns and
		 *    never reaches a zero-second cooldown.
		 */
		const float leveledCooldown = CalculateModifiedAttributeValue(
			GameplayAttribute{ CommonAttributeIds::Cooldown, mDefinition.cooldown, 0.f },
			mDefinition.attributeModifiers
		);
		const float attachmentModifiedCooldown = ApplyAttachmentModifiers(
			AttachmentHostKind::Ability,
			GameplayAttribute{ CommonAttributeIds::Cooldown, leveledCooldown, 0.f }
		).currentValue;
		const float hasteMultiplier = mAbilitySystem
			? AttributeMath::GetAbilityCooldownMultiplier(
				mAbilitySystem->GetAttributes().GetCurrentValue(OwnerAttributeIds::AbilityHaste)
			)
			: 1.f;
		return std::max(0.f, attachmentModifiedCooldown * hasteMultiplier);
	}

	float AbilityInstance::GetActiveDuration() const
	{
		const float leveledDuration = CalculateModifiedAttributeValue(
			GameplayAttribute{ CommonAttributeIds::Duration, mDefinition.duration, 0.f },
			mDefinition.attributeModifiers
		);
		return ApplyAttachmentModifiers(
			AttachmentHostKind::Ability,
			GameplayAttribute{ CommonAttributeIds::Duration, leveledDuration, 0.f }
		).currentValue;
	}

	AbilityRuntimeSnapshot AbilityInstance::BuildSnapshot() const
	{
		return AbilityRuntimeSnapshot{
			mHandle,
			&mDefinition,
			mLevel,
			GetMaxLevel(),
			mIsActive,
			mCooldownRemaining,
			GetCooldownDuration(),
			mActiveTimeRemaining,
			GetActiveDuration(),
			mCharges
		};
	}

	void AbilityInstance::UpdateCooldown(float deltaTime)
	{
		if (mCooldownRemaining > 0.f)
		{
			const float previousCooldownRemaining = mCooldownRemaining;
			const int previousCharges = mCharges;
			mCooldownRemaining = std::min(mCooldownRemaining, GetCooldownDuration());
			mCooldownRemaining = std::max(0.f, mCooldownRemaining - deltaTime);
			if (mCooldownRemaining <= 0.f && mDefinition.maxCharges > 0)
			{
				mCharges = mDefinition.maxCharges;
			}
			if (mAbilitySystem && (previousCooldownRemaining != mCooldownRemaining || previousCharges != mCharges))
			{
				mAbilitySystem->NotifyAbilityChanged(mHandle);
			}
		}
	}

	void AbilityInstance::UpdateWeaponFireInterval(float deltaTime)
	{
		mWeaponFireIntervalRemaining = std::max(0.f, mWeaponFireIntervalRemaining - deltaTime);
	}

	void AbilityInstance::UpdatePrimaryWeaponRuntimeContext(
		const PrimaryWeaponDefinition& weaponDefinition,
		const GameplayAttributeList& attributes,
		const List<GameplayTag>& damageTags
	)
	{
		if (mPrimaryWeaponRuntimeWeaponId != weaponDefinition.weaponId)
		{
			mPrimaryWeaponRuntime = PrimaryWeaponRuntimeState{};
			mPrimaryWeaponRuntimeWeaponId = weaponDefinition.weaponId;
		}
		mPrimaryWeaponRuntimeAttributes = attributes;
		mPrimaryWeaponRuntimeDamageTags = damageTags;
	}

	void AbilityInstance::TickInactivePrimaryWeaponRuntime(float deltaTime)
	{
		if (!mAbilitySystem || !mPrimaryWeaponRuntime.isInitialized || mPrimaryWeaponRuntime.isFiring ||
			mPrimaryWeaponRuntimeWeaponId.empty() || mPrimaryWeaponRuntimeAttributes.empty())
		{
			return;
		}

		for (const AbilityActionSpec& action : mDefinition.actions)
		{
			const FireWeaponAction* fireAction = std::get_if<FireWeaponAction>(&action.action);
			if (!fireAction || fireAction->weaponDefinition.weaponId != mPrimaryWeaponRuntimeWeaponId)
			{
				continue;
			}

			PrimaryWeaponExecutionSystem::TickInactive(
				PrimaryWeaponExecutionContext{
					mAbilitySystem->GetOwner(),
					fireAction->weaponDefinition,
					mPrimaryWeaponRuntimeAttributes,
					mPrimaryWeaponRuntimeDamageTags,
					&mDefinition.unlockedUpgradeIds,
					&mPrimaryWeaponRuntime
				},
				mPrimaryWeaponRuntime,
				deltaTime
			);
			return;
		}
	}

	void AbilityInstance::UpdateInputActivation()
	{
		switch (mDefinition.activationPolicy)
		{
		case AbilityActivationPolicy::OnPressed:
			if (IsPressedThisFrame())
			{
				TryActivate();
			}
			break;
		case AbilityActivationPolicy::WhileHeld:
			if (mInputHeld)
			{
				TryActivate();
			}
			if (!mInputHeld && mIsActive && mDefinition.lifetimePolicy == AbilityLifetimePolicy::WhileInputHeld)
			{
				EndAbility(AbilityEndReason::InputReleased);
			}
			break;
		case AbilityActivationPolicy::Toggle:
			if (IsPressedThisFrame())
			{
				mIsActive ? EndAbility(AbilityEndReason::Cancelled) : TryActivate();
			}
			break;
		case AbilityActivationPolicy::Passive:
			TryActivate();
			break;
		case AbilityActivationPolicy::GameplayEvent:
			break;
		}
	}

	void AbilityInstance::TickActiveExecution(float deltaTime)
	{
		AbilityExecutionContext context{ mAbilitySystem, &mDefinition, nullptr, this };
		AbilityExecutor::TickExecution(mExecution, context, deltaTime);
		if (mAbilitySystem && mBehavior)
		{
			AbilityBehaviorContext behaviorContext{
				*mAbilitySystem,
				*this,
				mAbilitySystem->GetOwner(),
				mDefinition
			};
			mBehavior->Tick(behaviorContext, deltaTime);
		}
	}

	void AbilityInstance::EndAbility(AbilityEndReason reason)
	{
		if (!mIsActive)
		{
			return;
		}

		if (mAbilitySystem && mBehavior)
		{
			AbilityBehaviorContext behaviorContext{
				*mAbilitySystem,
				*this,
				mAbilitySystem->GetOwner(),
				mDefinition
			};
			mBehavior->End(behaviorContext, reason);
		}

		AbilityExecutionContext context{ mAbilitySystem, &mDefinition, nullptr, this };
		AbilityExecutor::EndExecution(mExecution, context, reason);
		mExecution.actions.clear();
		mIsActive = false;
		mActiveTimeRemaining = 0.f;
		mCooldownRemaining = GetCooldownDuration();
		if (mCooldownRemaining <= 0.f && mDefinition.maxCharges > 0)
		{
			mCharges = mDefinition.maxCharges;
		}
		if (mAbilitySystem)
		{
			mAbilitySystem->NotifyAbilityEnded(mHandle, reason);
		}
	}

	void AbilityInstance::RebuildDefinitionForLevel()
	{
		mDefinition = mBaseDefinition;

		const int stepsToApply = std::min(
			std::max(0, mLevel - 1),
			static_cast<int>(mBaseDefinition.levelProgression.size())
		);

		for (int stepIndex = 0; stepIndex < stepsToApply; ++stepIndex)
		{
			const AbilityLevelStep& step = mBaseDefinition.levelProgression[stepIndex];
			for (const AttributeModifier& modifier : step.attributeModifiers)
			{
				mDefinition.attributeModifiers.push_back(modifier);
			}
			for (const GameplayTag& upgradeId : step.unlockedUpgradeIds)
			{
				const bool alreadyUnlocked = std::any_of(
					mDefinition.unlockedUpgradeIds.begin(),
					mDefinition.unlockedUpgradeIds.end(),
					[&](const GameplayTag& existingUpgradeId)
					{
						return existingUpgradeId == upgradeId;
					}
				);
				if (!alreadyUnlocked)
				{
					mDefinition.unlockedUpgradeIds.push_back(upgradeId);
				}
			}
			mDefinition.actions.insert(
				mDefinition.actions.end(),
				step.addedActions.begin(),
				step.addedActions.end()
			);
			mDefinition.triggers.insert(
				mDefinition.triggers.end(),
				step.addedTriggers.begin(),
				step.addedTriggers.end()
			);
		}
	}

	void AbilityInstance::RefreshPrimaryWeaponRuntimeConfiguration()
	{
		if (!mPrimaryWeaponRuntime.isInitialized ||
			mPrimaryWeaponRuntimeWeaponId.empty())
		{
			return;
		}

		for (const AbilityActionSpec& action : mDefinition.actions)
		{
			const FireWeaponAction* fireAction =
				std::get_if<FireWeaponAction>(&action.action);
			if (!fireAction ||
				fireAction->weaponDefinition.weaponId !=
					mPrimaryWeaponRuntimeWeaponId)
			{
				continue;
			}

			PrimaryWeaponExecutionSystem::EnsureRuntimeConfigured(
				fireAction->weaponDefinition,
				mPrimaryWeaponRuntime,
				&mDefinition.unlockedUpgradeIds
			);
			return;
		}
	}

	bool AbilityInstance::IsPressedThisFrame() const
	{
		return mInputHeld && !mWasInputHeld;
	}

	List<GameplayTag> AbilityInstance::GetAttachmentCapabilities(AttachmentHostKind hostKind) const
	{
		List<GameplayTag> capabilities = hostKind == AttachmentHostKind::Ability
			? mDefinition.attachmentCapabilities
			: List<GameplayTag>{};
		const auto addCapability = [&](const GameplayTag& capability)
		{
			if (std::find(capabilities.begin(), capabilities.end(), capability) == capabilities.end())
			{
				capabilities.push_back(capability);
			}
		};

		if (hostKind == AttachmentHostKind::Ability)
		{
			if (mDefinition.cooldown > 0.f)
			{
				addCapability(AttachmentSchema::Capability::Cooldown);
			}
			if (std::any_of(mDefinition.abilityTags.begin(), mDefinition.abilityTags.end(), [](const GameplayTag& tag)
			{
				return tag.MatchesTag(GameplayTag{ "Ability.Offense" });
			}))
			{
				addCapability(AttachmentSchema::Capability::Damage);
			}
			return capabilities;
		}

		for (const AbilityActionSpec& action : mDefinition.actions)
		{
			const FireWeaponAction* fireAction = std::get_if<FireWeaponAction>(&action.action);
			if (!fireAction)
			{
				continue;
			}
			for (const GameplayTag& capability : fireAction->weaponDefinition.attachmentCapabilities)
			{
				addCapability(capability);
			}
			if (FindGameplayAttribute(fireAction->weaponDefinition.attributes, CommonAttributeIds::Damage))
			{
				addCapability(AttachmentSchema::Capability::Damage);
			}
			if (FindGameplayAttribute(fireAction->weaponDefinition.attributes, CommonAttributeIds::FireRate))
			{
				addCapability(AttachmentSchema::Capability::FireRate);
			}
			if (fireAction->weaponDefinition.weaponTypeTag.MatchesTag(PrimaryWeaponSchema::Projectile::FamilyId))
			{
				addCapability(AttachmentSchema::Capability::Projectile);
			}
			if (fireAction->weaponDefinition.weaponTypeTag.MatchesTag(PrimaryWeaponSchema::Beam::FamilyId))
			{
				addCapability(AttachmentSchema::Capability::Beam);
			}
		}
		return capabilities;
	}

	size_t AbilityInstance::GetAttachmentSlotCapacity(AttachmentHostKind hostKind) const
	{
		if (hostKind == AttachmentHostKind::Ability)
		{
			return mDefinition.attachmentSlotCapacity;
		}
		for (const AbilityActionSpec& action : mDefinition.actions)
		{
			if (const FireWeaponAction* fireAction = std::get_if<FireWeaponAction>(&action.action))
			{
				return fireAction->weaponDefinition.attachmentSlotCapacity;
			}
		}
		return 0;
	}
}


