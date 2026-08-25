#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/GameAbility.h"
#include "attributes/AttributeMath.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/tags/GameplayTags.h"
#include "abilities/AbilityEvent.h"
#include "attributes/AttributeSystem.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "framework/Actor.h"
#include <algorithm>
#include <variant>

namespace ly
{
	GameAbility::GameAbility(
		LightYearsAbilitySystemComponent& abilitySystem,
		sas::AbilityHandle handle,
		const GameAbilityDefinition& definition,
		unique_ptr<GameAbilityBehavior> behavior,
		bool emitNotifications)
		: sas::GameplayAbilityInstance<
			GameAbilityDefinition,
			GameAbilityExecution
		>{
			handle,
			definition,
			emitNotifications
				? abilitySystem.CreateAbilityInstanceNotifications()
				: sas::AbilityInstanceNotifications{}
		},
		mAbilitySystem{ abilitySystem },
		mBehavior{ std::move(behavior) }
	{
	}

	void GameAbility::ConfigureInvocationLevel(int level, int maximumLevel)
	{
		mInvocationMaximumLevel = std::max(1, maximumLevel);
		mRuntimeState.SetLevel(level, mInvocationMaximumLevel);
	}

	void GameAbility::SetWeaponFireIntervalRemaining(float interval)
	{
		mWeaponFireIntervalRemaining = std::max(0.f, interval);
	}

	bool GameAbility::TryEquipAttachment(
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
		if (equipped)
		{
			mAbilitySystem.NotifyAbilityChanged(mHandle);
		}
		return equipped;
	}

	bool GameAbility::RemoveAttachment(const std::string& attachmentId, AttachmentHostKind hostKind)
	{
		const bool removed = mAttachments.Remove(sas::ContentId{ attachmentId }, hostKind);
		if (removed)
		{
			mAbilitySystem.NotifyAbilityChanged(mHandle);
		}
		return removed;
	}

	sas::GameplayAttributeList GameAbility::MergeAttachmentAttributes(
		AttachmentHostKind hostKind,
		const sas::GameplayAttributeList& attributes
	) const
	{
		return mAttachments.MergeGrantedAttributes(hostKind, attributes);
	}

	sas::GameplayAttribute GameAbility::ApplyAttachmentModifiers(
		AttachmentHostKind hostKind,
		const sas::GameplayAttribute& attribute
	) const
	{
		return mAttachments.ApplyStaticModifiers(hostKind, attribute);
	}

	sas::GameplayAttributeList GameAbility::ApplyAttachmentConditions(
		AttachmentHostKind hostKind,
		const sas::GameplayAttributeList& attributes,
		const List<GameplayTag>& originalDamageTags
	) const
	{
		return mAttachments.ApplyConditionalModifiers(hostKind, attributes, originalDamageTags);
	}

	List<GameplayTag> GameAbility::GetResolvedDamageTags(AttachmentHostKind hostKind) const
	{
		const List<GameplayTag> baseDamageTags = mDefinition.damageTags.empty()
			? List<GameplayTag>{ DamageTypeSchema::Photonic }
			: mDefinition.damageTags;
		return mAttachments.ResolveDamageTags(hostKind, baseDamageTags);
	}

	void GameAbility::HandleAttachmentEvent(
		const sas::AbilityEvent& event
	)
	{
		HandleAttachmentEventInternal(event, nullptr);
	}

	void GameAbility::HandleAbilityLifecycleEvent(
		const sas::AbilityLifecycleEvent& event
	)
	{
		if (event.eventTag == GameplayTags::Event::Ability::Activated &&
			event.abilityId != mDefinition.abilityId)
		{
			// The generic lifecycle dispatcher calls this for every granted ability.
			// Only a successful activation is emitted here, so a behavior such as
			// Phase Drift can end itself on a real action without observing failed
			// cooldown or blocked-input attempts.
			NotifyOwnerAbilityActivated(event);
		}
		HandleAttachmentEventInternal(event, &event);
	}

	void GameAbility::HandleGameplayEvent(const sas::AbilityEvent& event)
	{
		if (!mBehavior || !IsActive())
		{
			return;
		}

		GameAbilityBehaviorContext behaviorContext{
			mAbilitySystem,
			*this,
			mAbilitySystem.GetOwner(),
			mDefinition
		};
		mBehavior->OnGameplayEvent(behaviorContext, event);
	}

	void GameAbility::HandleAttachmentEventInternal(
		const sas::AbilityEvent& event,
		const sas::AbilityLifecycleEvent* lifecycleEvent
	)
	{
		const List<GameplayTag>* sourceAbilityTags = lifecycleEvent
			? &lifecycleEvent->abilityTags
			: &event.sourceAbilityTags;
		GameplayTagContainer sourceAbilityTagContainer;
		for (const GameplayTag& tag : *sourceAbilityTags)
		{
			sourceAbilityTagContainer.AddTag(tag);
		}

		const DamageContext* damageContext = event.GetContext<DamageContext>();
		const List<GameplayTag>* damageTags = damageContext
			? &damageContext->damageTags
			: nullptr;

		List<EquippedAttachment>& equippedAttachments = mAttachments.GetEquipped();
		for (EquippedAttachment& equipped : equippedAttachments)
		{
			for (std::size_t ruleIndex = 0;
				ruleIndex < equipped.definition.eventRules.size();
				++ruleIndex)
			{
				const AttachmentEventRule& rule = equipped.definition.eventRules[ruleIndex];
				const int matchLimit = rule.consumeOnMatch ? 1 : rule.maxMatches;
				if (matchLimit > 0 &&
					ruleIndex < equipped.eventMatchCounts.size() &&
					equipped.eventMatchCounts[ruleIndex] >= matchLimit)
				{
					continue;
				}
				if (!MatchesAttachmentEventRule(
					rule,
					event,
					lifecycleEvent,
					sourceAbilityTagContainer,
					damageTags
				))
				{
					continue;
				}
				if (ExecuteAttachmentEventRule(equipped, rule) && matchLimit > 0)
				{
					if (equipped.eventMatchCounts.size() < equipped.definition.eventRules.size())
					{
						equipped.eventMatchCounts.resize(equipped.definition.eventRules.size(), 0);
					}
					++equipped.eventMatchCounts[ruleIndex];
				}
			}
		}
	}

	bool GameAbility::MatchesAttachmentEventRule(
		const AttachmentEventRule& rule,
		const sas::AbilityEvent& event,
		const sas::AbilityLifecycleEvent* lifecycleEvent,
		const GameplayTagContainer& sourceAbilityTags,
		const List<GameplayTag>* damageTags
	) const
	{
		if (!event.eventTag.MatchesTag(rule.eventTag) ||
			!mAbilitySystem.HasAllOwnedTags(rule.requiredOwnerTags) ||
			mAbilitySystem.HasAnyOwnedTags(rule.blockedOwnerTags) ||
			!sourceAbilityTags.HasAll(rule.requiredAbilityTags) ||
			sourceAbilityTags.HasAny(rule.blockedAbilityTags) ||
			(rule.abilityId.IsValid() &&
				(!lifecycleEvent || lifecycleEvent->abilityId != rule.abilityId)) ||
			(rule.endReason.has_value() &&
				(!lifecycleEvent || lifecycleEvent->endReason != *rule.endReason)) ||
			(rule.requireOwnerAsEventSource &&
				event.GetSource<Actor>() != &mAbilitySystem.GetOwner()))
		{
			return false;
		}

		return std::all_of(
			rule.requiredDamageTags.begin(),
			rule.requiredDamageTags.end(),
			[&](const GameplayTag& requiredTag)
			{
				return damageTags && std::any_of(
					damageTags->begin(),
					damageTags->end(),
				[&](const GameplayTag& tag)
					{
						return tag.MatchesTag(requiredTag);
					}
				);
			}
		);
	}

	bool GameAbility::ExecuteAttachmentEventRule(
		EquippedAttachment& equipped,
		const AttachmentEventRule& rule
	)
	{
		const float magnitude = mAttachments.ResolveGrantedAttributeValue(
			equipped.hostKind,
			rule.magnitudeAttributeId,
			rule.baseMagnitude
		);
		if (rule.action == AttachmentEventAction::ReduceCooldown && magnitude <= 0.f)
		{
			return false;
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
				mAbilitySystem.ReduceAbilityCooldowns(magnitude, true);
				break;
			case AttachmentCooldownTarget::AllNonPrimaryAbilities:
				mAbilitySystem.ReduceAbilityCooldowns(magnitude, false);
				break;
			}
			return true;
		case AttachmentEventAction::ApplyEffect:
			if (const sas::GameplayEffectDefinition* definition =
				EffectData::FindGameplayEffectDefinition(rule.effectId.ToString()))
			{
				if (!definition->sourceParameterized)
				{
					mAbilitySystem.ApplyGameplayEffect(*definition, &mAbilitySystem.GetOwner());
					return true;
				}
			}
			return false;
		case AttachmentEventAction::RemoveEffects:
			mAbilitySystem.RemoveGameplayEffectsIf(
				[&](const sas::ActiveGameplayEffect& activeEffect)
				{
					const sas::GameplayEffectDefinition& definition =
						activeEffect.spec.definition;
					return
						(!rule.effectDisposition.has_value() ||
							definition.disposition == *rule.effectDisposition) &&
						(!rule.effectCleanseableOnly || definition.cleanseable) &&
						(rule.effectCategory.empty() ||
							definition.category == rule.effectCategory) &&
						(rule.effectImmunityCategory.empty() ||
							definition.immunityCategory == rule.effectImmunityCategory);
				}
			);
			return true;
		}
		return false;
	}

	bool GameAbility::CanActivateContent() const
	{
		// Standard locks are checked here so every current and future game ability
		// shares one activation gate. Producers only need to grant the shared tag.
		const GameplayTag& sharedBlockTag =
			mDefinition.slot == sas::AbilitySlot::PrimaryFire
			? GameplayTagSchema::BlockPrimaryWeaponFire
			: GameplayTagSchema::BlockAbilityActivation;
		const bool isMovementAbility = std::any_of(
			mDefinition.abilityTags.begin(),
			mDefinition.abilityTags.end(),
			[](const GameplayTag& tag)
			{
				return tag.MatchesTagExact(GameplayTags::Ability::Movement);
			}
		);
		return mBehavior &&
			mAbilitySystem.HasAllOwnedTags(mDefinition.requiredOwnerTags) &&
			!mAbilitySystem.HasAnyOwnedTags(mDefinition.blockedOwnerTags) &&
			!mAbilitySystem.HasOwnedTag(GameplayTags::State::Effect::Control::Stunned) &&
			!mAbilitySystem.HasOwnedTag(sharedBlockTag) &&
			(!isMovementAbility ||
				!mAbilitySystem.HasOwnedTag(GameplayTagSchema::BlockMovementInput));
	}

	bool GameAbility::ActivateContent()
	{
		mDeferActiveDurationStart = false;
		const sas::AbilityLifecycleEvent lifecycleEvent = BuildLifecycleEvent(
			GameplayTags::Event::Ability::Activated,
			sas::AbilityEndReason::Completed
		);
		if (!mAbilitySystem.EvaluateAbilityActivation(lifecycleEvent))
		{
			return false;
		}

		GameAbilityBehaviorContext behaviorContext{
			mAbilitySystem,
			*this,
			mAbilitySystem.GetOwner(),
			mDefinition
		};
		if (!mBehavior->Activate(behaviorContext))
		{
			mDeferActiveDurationStart = false;
			return false;
		}

		mAbilitySystem.HandleAbilityLifecycleEvent(lifecycleEvent);
		return true;
	}

	void GameAbility::BeginExecution()
	{
		mExecution.actions.clear();
		AbilityExecutionContext context{
			&mAbilitySystem,
			&mDefinition,
			nullptr,
			this
		};
		GameAbilityActionExecutor::BeginExecution(mExecution, context);
	}

	void GameAbility::TickExecution(float deltaTime)
	{
		if (mAbilitySystem.HasOwnedTag(GameplayTags::State::Effect::Control::Stunned))
		{
			Cancel(sas::AbilityEndReason::Interrupted);
			return;
		}

		AbilityExecutionContext context{
			&mAbilitySystem,
			&mDefinition,
			nullptr,
			this
		};
		GameAbilityActionExecutor::TickExecution(
			mExecution,
			context,
			deltaTime
		);
		if (mBehavior)
		{
			GameAbilityBehaviorContext behaviorContext{
				mAbilitySystem,
				*this,
				mAbilitySystem.GetOwner(),
				mDefinition
			};
			mBehavior->Tick(behaviorContext, deltaTime);
		}
	}

	void GameAbility::EndExecution(sas::AbilityEndReason reason)
	{
		AbilityExecutionContext context{
			&mAbilitySystem,
			&mDefinition,
			nullptr,
			this
		};
		GameAbilityActionExecutor::EndExecution(mExecution, context, reason);
		mExecution.actions.clear();
	}

	void GameAbility::TickInactive(float deltaTime)
	{
		TickInactivePrimaryWeaponRuntime(deltaTime);
		UpdateWeaponFireInterval(deltaTime);
	}

	void GameAbility::EndContent(sas::AbilityEndReason reason)
	{
		if (mBehavior)
		{
			GameAbilityBehaviorContext behaviorContext{
				mAbilitySystem,
				*this,
				mAbilitySystem.GetOwner(),
				mDefinition
			};
			mBehavior->End(behaviorContext, reason);
		}

		const sas::AbilityLifecycleEvent event = BuildLifecycleEvent(
			GameplayTags::Event::Ability::Ended,
			reason
		);
		mAbilitySystem.HandleAbilityLifecycleEvent(event);
		mDeferActiveDurationStart = false;
	}

	bool GameAbility::HandleInputPressed()
	{
		if (!mBehavior || !IsActive())
		{
			return false;
		}

		GameAbilityBehaviorContext behaviorContext{
			mAbilitySystem,
			*this,
			mAbilitySystem.GetOwner(),
			mDefinition
		};
		return mBehavior->OnInputPressed(behaviorContext);
	}

	sas::AbilityLifecycleEvent GameAbility::BuildLifecycleEvent(
		const GameplayTag& eventTag,
		sas::AbilityEndReason endReason
	) const
	{
		sas::AbilityLifecycleEvent event;
		event.eventTag = eventTag;
		event.abilityHandle = GetHandle();
		event.abilityId = sas::ContentId{ mDefinition.abilityId };
		event.slot = mDefinition.slot;
		event.abilityLevel = GetLevel();
		event.abilityMaxLevel = GetMaxLevel();
		event.activationOrigin = mActivationOrigin;
		event.abilityTags = mDefinition.abilityTags;
		event.sourceAbilityId = event.abilityId;
		event.sourceAbilityTags = event.abilityTags;
		event.endReason = endReason;
		event.SetSource(&mAbilitySystem.GetOwner());
		event.SetTarget(&mAbilitySystem.GetOwner());
		return event;
	}

	int GameAbility::GetMaximumLevel() const
	{
		if (mInvocationMaximumLevel > 0)
		{
			return mInvocationMaximumLevel;
		}

		return std::max(
			1,
			mBaseDefinition.GetMaxLevel() +
				mAbilitySystem.GetScopedAbilityLevelBonus(mBaseDefinition)
		);
	}

	float GameAbility::ResolveCooldownDuration() const
	{
		/*
		 * Effective cooldown order:
		 * 1. Start with this GameAbility's private definition copy.
		 * 2. Ability-specific levels mutate only this copy: base cooldown + summed Cooldown value modifiers.
		 * 3. Ship-wide AbilityHaste rating is converted by the shared asymptotic
		 *    curve. Cooldown reduction therefore has diminishing returns and
		 *    never reaches a zero-second cooldown.
		 */
		const float leveledCooldown = sas::CalculateModifiedAttributeValue(
			sas::GameplayAttribute{ CommonAttributeIds::Cooldown, mDefinition.cooldown, 0.f },
			mDefinition.attributeModifiers
		);
		const float attachmentModifiedCooldown = ApplyAttachmentModifiers(
			AttachmentHostKind::Ability,
			sas::GameplayAttribute{ CommonAttributeIds::Cooldown, leveledCooldown, 0.f }
		).currentValue;
		const float hasteMultiplier =
			sas::AttributeMath::GetAbilityCooldownMultiplier(
				mAbilitySystem.GetAttributes().GetCurrentValue(
					OwnerAttributeIds::AbilityHaste
				)
			);
		return std::max(0.f, attachmentModifiedCooldown * hasteMultiplier);
	}

	float GameAbility::ResolveCooldownDurationOnEnd(sas::AbilityEndReason reason)
	{
		const float baseCooldown = ResolveCooldownDuration();
		if (!mBehavior)
		{
			return baseCooldown;
		}

		GameAbilityBehaviorContext behaviorContext{
			mAbilitySystem,
			*this,
			mAbilitySystem.GetOwner(),
			mDefinition
		};
		return std::max(
			0.f,
			mBehavior->ResolveCooldownDurationOnEnd(
				behaviorContext,
				reason,
				baseCooldown
			)
		);
	}

	float GameAbility::ResolveActiveDuration() const
	{
		const float leveledDuration = sas::CalculateModifiedAttributeValue(
			sas::GameplayAttribute{ CommonAttributeIds::Duration, mDefinition.duration, 0.f },
			mDefinition.attributeModifiers
		);
		const float attachmentModifiedDuration = ApplyAttachmentModifiers(
			AttachmentHostKind::Ability,
			sas::GameplayAttribute{ CommonAttributeIds::Duration, leveledDuration, 0.f }
		).currentValue;
		if (!mBehavior)
		{
			return attachmentModifiedDuration;
		}

		GameAbilityBehaviorContext behaviorContext{
			const_cast<LightYearsAbilitySystemComponent&>(mAbilitySystem),
			const_cast<GameAbility&>(*this),
			const_cast<Actor&>(mAbilitySystem.GetOwner()),
			mDefinition
		};
		return std::max(
			0.f,
			mBehavior->ResolveActiveDuration(behaviorContext, attachmentModifiedDuration)
		);
	}

	void GameAbility::NotifyOwnerAbilityActivated(
		const sas::AbilityLifecycleEvent& event
	)
	{
		if (!mBehavior || !IsActive())
		{
			return;
		}

		GameAbilityBehaviorContext behaviorContext{
			mAbilitySystem,
			*this,
			mAbilitySystem.GetOwner(),
			mDefinition
		};
		mBehavior->OnOwnerAbilityActivated(behaviorContext, event);
	}

	void GameAbility::OnLevelConfigurationChanged()
	{
		RefreshPrimaryWeaponRuntimeConfiguration();
	}

	void GameAbility::RefreshScopedConfiguration()
	{
		RebuildDefinitionForLevel();
		RefreshPrimaryWeaponRuntimeConfiguration();
	}

	void GameAbility::UpdateWeaponFireInterval(float deltaTime)
	{
		mWeaponFireIntervalRemaining = std::max(0.f, mWeaponFireIntervalRemaining - deltaTime);
	}

	void GameAbility::UpdatePrimaryWeaponRuntimeContext(
		const PrimaryWeaponDefinition& weaponDefinition,
		const sas::GameplayAttributeList& attributes,
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

	void GameAbility::TickInactivePrimaryWeaponRuntime(float deltaTime)
	{
		if (!mPrimaryWeaponRuntime.isInitialized ||
			mPrimaryWeaponRuntime.isFiring ||
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
					mAbilitySystem.GetOwner(),
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

	void GameAbility::RebuildDefinitionForLevel()
	{
		mDefinition = mBaseDefinition;

		const int stepsToApply = std::min(
			std::max(
				0,
				mRuntimeState.GetLevel() - 1 +
					mAbilitySystem.GetScopedAbilityLevelBonus(mBaseDefinition)
			),
			static_cast<int>(mBaseDefinition.levelProgression.size())
		);

		for (int stepIndex = 0; stepIndex < stepsToApply; ++stepIndex)
		{
			const AbilityLevelStep& step = mBaseDefinition.levelProgression[stepIndex];
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
			{
				mDefinition.attributeModifiers.push_back(modifier);
			}
			for (const std::string& upgradeId : step.unlockedUpgradeIds)
			{
				const bool alreadyUnlocked = std::any_of(
					mDefinition.unlockedUpgradeIds.begin(),
					mDefinition.unlockedUpgradeIds.end(),
					[&](const std::string& existingUpgradeId)
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

	void GameAbility::RefreshPrimaryWeaponRuntimeConfiguration()
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

	List<GameplayTag> GameAbility::GetAttachmentCapabilities(AttachmentHostKind hostKind) const
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
				return tag.MatchesTag(GameplayTagSchema::AbilityOffense);
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
			if (sas::FindAttribute(fireAction->weaponDefinition.attributes, CommonAttributeIds::Damage))
			{
				addCapability(AttachmentSchema::Capability::Damage);
			}
			if (sas::FindAttribute(fireAction->weaponDefinition.attributes, CommonAttributeIds::FireRate))
			{
				addCapability(AttachmentSchema::Capability::FireRate);
			}
			if (IsProjectileWeaponType(fireAction->weaponDefinition.weaponType))
			{
				addCapability(AttachmentSchema::Capability::Projectile);
			}
			if (IsBeamWeaponType(fireAction->weaponDefinition.weaponType))
			{
				addCapability(AttachmentSchema::Capability::Beam);
			}
		}
		return capabilities;
	}

	size_t GameAbility::GetAttachmentSlotCapacity(AttachmentHostKind hostKind) const
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


