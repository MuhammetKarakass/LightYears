#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/GameAbility.h"

#include <type_traits>
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "attributes/AttributeMath.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/tags/GameplayTags.h"
#include "abilities/AbilityEvent.h"
#include "attributes/AttributeSystem.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/damage/DamageTypeSystem.h"
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
			abilitySystem.CreateAbilityInstanceNotifications(emitNotifications)
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
		mPrimaryWeaponState.SetFireIntervalRemaining(interval);
	}

	List<GameplayTag> GameAbility::GetResolvedDamageTags() const
	{
		return mDefinition.damageTags.empty()
			? List<GameplayTag>{ DamageTypeSchema::Photonic }
			: mDefinition.damageTags;
	}

	void GameAbility::HandleAbilityLifecycleEvent(
		const sas::AbilityLifecycleEvent& event
	)
	{
		return RunInstanceOperation([&]()
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
		});
	}

	void GameAbility::HandleGameplayEvent(const sas::AbilityEvent& event)
	{
		return RunInstanceOperation([&]()
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
		});
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
		const bool allowedByStandardRules = mBehavior &&
			mAbilitySystem.HasAllOwnedTags(mDefinition.requiredOwnerTags) &&
			!mAbilitySystem.HasAnyOwnedTags(mDefinition.blockedOwnerTags) &&
			!mAbilitySystem.HasOwnedTag(GameplayTags::State::Effect::Control::Stunned) &&
			!mAbilitySystem.HasOwnedTag(sharedBlockTag) &&
			(!isMovementAbility ||
				!mAbilitySystem.HasOwnedTag(GameplayTagSchema::BlockMovementInput));
		if (!allowedByStandardRules) return false;
		if (mDefinition.slot == sas::AbilitySlot::PrimaryFire)
		{
			return mAbilitySystem.IsModuleSystemEnabled(GameplayTags::Module::System::PrimaryWeapon);
		}
		return mAbilitySystem.IsModuleAbilityAvailable(mDefinition.abilityId);
	}

	bool GameAbility::ActivateContent()
	{
		mDamageHitGroup = std::make_shared<DamageHitGroup>();
		mDeferActiveDurationStart = false;
		mPendingActivationCooldownMultiplier = 1.f;
		mEndLifecycleNotificationAttempted = false;
		const sas::AbilityLifecycleEvent lifecycleEvent = BuildLifecycleEvent(
			GameplayTags::Event::Ability::Activated,
			sas::AbilityEndReason::Completed
		);
		if (!mAbilitySystem.EvaluateAbilityActivation(lifecycleEvent) || ActivationInterrupted())
		{
			return false;
		}

		GameAbilityBehaviorContext behaviorContext{
			mAbilitySystem,
			*this,
			mAbilitySystem.GetOwner(),
			mDefinition
		};
		mBehaviorStarted = true;
		if (!mBehavior->Activate(behaviorContext) || ActivationInterrupted())
		{
			mDeferActiveDurationStart = false;
			return false;
		}

		return true;
	}

	void GameAbility::OnActivationCommitted()
	{
		mAbilitySystem.HandleAbilityLifecycleEvent(BuildLifecycleEvent(
			GameplayTags::Event::Ability::Activated, sas::AbilityEndReason::Completed));
	}

	void GameAbility::AbortActivationContent()
	{
		mDeferActiveDurationStart = false;
		EndBehavior(sas::AbilityEndReason::Interrupted);
	}

	void GameAbility::BeginExecution()
	{
		AbilityExecutionContext context{
			&mAbilitySystem,
			&mDefinition,
			nullptr,
			this,
			[this] { return ShouldContinueExecution(); }
		};
		GameAbilityActionExecutor::BeginExecution(mExecution, context);
	}

	void GameAbility::TickExecution(float deltaTime)
	{
		const bool primaryWeaponUnavailable = mDefinition.slot == sas::AbilitySlot::PrimaryFire &&
			!mAbilitySystem.IsModuleSystemEnabled(GameplayTags::Module::System::PrimaryWeapon);
		const bool loadoutAbilityUnavailable =
			!mAbilitySystem.IsModuleAbilityAvailable(mDefinition.abilityId);
		if (primaryWeaponUnavailable || loadoutAbilityUnavailable)
		{
			Cancel(sas::AbilityEndReason::Interrupted);
			return;
		}
		if (mAbilitySystem.HasOwnedTag(GameplayTags::State::Effect::Control::Stunned))
		{
			Cancel(sas::AbilityEndReason::Interrupted);
			return;
		}

		AbilityExecutionContext context{
			&mAbilitySystem,
			&mDefinition,
			nullptr,
			this,
			[this] { return ShouldContinueExecution(); }
		};
		GameAbilityActionExecutor::TickExecution(
			mExecution,
			context,
			deltaTime
		);
		if (!ShouldContinueExecution()) return;
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
			this,
			[] { return true; }
		};
		GameAbilityActionExecutor::EndExecution(mExecution, context, reason);
	}

	void GameAbility::OnInstanceOperationCompleted()
	{
		mAbilitySystem.FlushDeferredScopedAbilityRules();
	}

	void GameAbility::TickInactive(float deltaTime)
	{
		TickInactivePrimaryWeaponRuntime(deltaTime);
	}

	void GameAbility::EndContent(sas::AbilityEndReason reason)
	{
		mDeferActiveDurationStart = false;
		std::exception_ptr error;
		try { EndBehavior(reason); }
		catch (...) { error = std::current_exception(); }

		if (!mEndLifecycleNotificationAttempted)
		{
			mEndLifecycleNotificationAttempted = true;
			try
			{
				const sas::AbilityLifecycleEvent event = BuildLifecycleEvent(
					GameplayTags::Event::Ability::Ended,
					reason
				);
				mAbilitySystem.HandleAbilityLifecycleEvent(event);
			}
			catch (...) { if (!error) error = std::current_exception(); }
		}
		if (error) std::rethrow_exception(error);
	}

	void GameAbility::RetryContentCleanup()
	{
		if (mBehaviorCleanupPending)
		{
			EndBehavior(mBehaviorEndReason);
		}
	}

	void GameAbility::EndBehavior(sas::AbilityEndReason reason)
	{
		if (!mBehavior || !mBehaviorStarted)
		{
			mBehaviorCleanupPending = false;
			return;
		}

		mBehaviorEndReason = reason;
		mBehaviorCleanupPending = true;
		GameAbilityBehaviorContext context{
			mAbilitySystem,
			*this,
			mAbilitySystem.GetOwner(),
			mDefinition
		};
		mBehavior->End(context, reason);
		mBehaviorStarted = false;
		mBehaviorCleanupPending = false;
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
			GameAbilityDefinition::kUnboundedAbilityLevel +
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
		const float moduleMultiplier = IsOnCooldown() && mCooldownModuleMultiplierSnapshot
			? *mCooldownModuleMultiplierSnapshot
			: mAbilitySystem.GetScopedAbilityCooldownMultiplier(mDefinition);
		return ResolveCooldownDurationWithModuleMultiplier(moduleMultiplier);
	}

	float GameAbility::ResolveCooldownDurationForStart() const
	{
		mCooldownModuleMultiplierSnapshot = mAbilitySystem.GetScopedAbilityCooldownMultiplier(mDefinition);
		return ResolveCooldownDurationWithModuleMultiplier(*mCooldownModuleMultiplierSnapshot);
	}

	float GameAbility::ResolveCooldownDurationWithModuleMultiplier(float moduleMultiplier) const
	{
		const float leveledCooldown = sas::CalculateModifiedAttributeValue(
			sas::GameplayAttribute{ CommonAttributeIds::Cooldown, mDefinition.cooldown, 0.f },
			mDefinition.attributeModifiers
		);
		const float hasteMultiplier =
			sas::AttributeMath::GetAbilityCooldownMultiplier(
				mAbilitySystem.GetAttributes().GetCurrentValue(
					OwnerAttributeIds::AbilityHaste
				)
			);
		const float unreducedCooldown = leveledCooldown * hasteMultiplier;
		// A module reduction never takes the cooldown below its absolute floor, and never raises it above the unreduced value.
		const float floorSeconds = moduleMultiplier < 1.f ? std::min(unreducedCooldown, mAbilitySystem.GetScopedAbilityCooldownMinimum(mDefinition)) : 0.f;
		return std::max(floorSeconds, std::max(0.f, unreducedCooldown * moduleMultiplier));
	}

	void GameAbility::RefundActivationCooldown(float fraction)
	{
		const float safeFraction = std::clamp(fraction, 0.f, 1.f);
		if (mDefinition.cooldownStartPolicy == sas::AbilityCooldownStartPolicy::OnActivation)
			ReduceCooldownRemaining(GetCooldownRemaining() * safeFraction);
		else if (IsActive()) mPendingActivationCooldownMultiplier *= 1.f - safeFraction;
	}

	float GameAbility::ResolveCooldownDurationOnEnd(sas::AbilityEndReason reason)
	{
		const float baseCooldown = ResolveCooldownDurationForStart();
		const float refundMultiplier = std::exchange(mPendingActivationCooldownMultiplier, 1.f);
		if (!mBehavior)
		{
			return baseCooldown * refundMultiplier;
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
		) * refundMultiplier;
	}

	float GameAbility::ResolveActiveDuration() const
	{
		const float leveledDuration = sas::CalculateModifiedAttributeValue(
			sas::GameplayAttribute{ CommonAttributeIds::Duration, mDefinition.duration, 0.f },
			mDefinition.attributeModifiers
		);
		if (!mBehavior)
		{
			return leveledDuration;
		}

		GameAbilityBehaviorContext behaviorContext{
			const_cast<LightYearsAbilitySystemComponent&>(mAbilitySystem),
			const_cast<GameAbility&>(*this),
			const_cast<Actor&>(mAbilitySystem.GetOwner()),
			mDefinition
		};
		return std::max(
			0.f,
			mBehavior->ResolveActiveDuration(behaviorContext, leveledDuration)
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
		return RunInstanceOperation([&]()
		{
			EnsurePendingCleanupDrained();
			if (mAbilitySystem.IsClearPending()) return;
			RebuildDefinitionForLevel();
			RefreshPrimaryWeaponRuntimeConfiguration();
		});
	}

	void GameAbility::UpdatePrimaryWeaponRuntimeContext(
		const PrimaryWeaponDefinition& weaponDefinition,
		const sas::GameplayAttributeList& attributes,
		const List<GameplayTag>& damageTags
	)
	{
		mPrimaryWeaponState.UpdateContext(
			weaponDefinition,
			attributes,
			damageTags,
			mAbilitySystem.GetAttributes().GetRevision(),
			mConfigurationRevision,
			mAbilitySystem.GetPrimaryWeaponModifierRevision()
		);
	}

	void GameAbility::TickInactivePrimaryWeaponRuntime(float deltaTime)
	{
		PrimaryWeaponRuntimeState& weaponRuntime = mPrimaryWeaponState.runtime;
		if (weaponRuntime.isInitialized && !weaponRuntime.isFiring)
		{
			RefreshPrimaryWeaponRuntimeConfiguration();
		}
		if (!weaponRuntime.isInitialized ||
			weaponRuntime.isFiring ||
			mPrimaryWeaponState.weaponId.empty() || mPrimaryWeaponState.attributes.empty())
		{
			mPrimaryWeaponState.TickFireInterval(deltaTime);
			return;
		}

		for (const AbilityActionSpec& action : mDefinition.actions)
		{
			const FireWeaponAction* fireAction = std::get_if<FireWeaponAction>(&action.action);
			if (!fireAction || fireAction->weaponDefinition.weaponId != mPrimaryWeaponState.weaponId)
			{
				continue;
			}

			PrimaryWeaponExecutionSystem::TickInactive(
				PrimaryWeaponExecutionContext{
					mAbilitySystem.GetOwner(),
					fireAction->weaponDefinition,
					mPrimaryWeaponState.attributes,
					mPrimaryWeaponState.damageTags,
					&mDefinition.unlockedUpgradeIds,
					&weaponRuntime
				},
				weaponRuntime,
				deltaTime
			);
			return;
		}
		mPrimaryWeaponState.TickFireInterval(deltaTime);
	}

	bool GameAbility::PrepareDefinitionForLevel(int level)
	{
		mPrimaryWeaponState.prepared.reset();
		mPreparedLevelDefinition.emplace(BuildDefinitionForLevel(level));
		if (mPrimaryWeaponState.runtime.isInitialized &&
			!mPrimaryWeaponState.weaponId.empty())
		{
			for (const AbilityActionSpec& action : mPreparedLevelDefinition->actions)
			{
				const FireWeaponAction* fireAction =
					std::get_if<FireWeaponAction>(&action.action);
				if (!fireAction || fireAction->weaponDefinition.weaponId !=
					mPrimaryWeaponState.weaponId)
				{
					continue;
				}

				PrimaryWeaponAbilityState::PreparedConfiguration prepared;
				const PrimaryWeaponValidationResult validation =
					PrimaryWeaponExecutionSystem::PrepareRuntimeConfiguration(
						fireAction->weaponDefinition,
						mPrimaryWeaponState.runtime,
						&mPreparedLevelDefinition->unlockedUpgradeIds,
						prepared.runtime
					);
				if (!validation.isValid)
				{
					mPreparedLevelDefinition.reset();
					return false;
				}

				AbilityExecutionContext context{
					&mAbilitySystem,
					&*mPreparedLevelDefinition,
					nullptr,
					this
				};
				prepared.attributes = AbilityActionAttributeResolver::ResolveAttributes(
					context,
					&fireAction->weaponDefinition,
					fireAction->weaponDefinition.attributes
				);
				prepared.attributeRevision = mAbilitySystem.GetAttributes().GetRevision();
				prepared.modifierRevision = mAbilitySystem.GetPrimaryWeaponModifierRevision();
				mPrimaryWeaponState.prepared.emplace(std::move(prepared));
				break;
			}
		}
		return true;
	}

	void GameAbility::CommitPreparedDefinitionForLevel() noexcept
	{
		static_assert(std::is_nothrow_move_assignable_v<GameAbilityDefinition>);
		static_assert(std::is_nothrow_move_assignable_v<sas::GameplayAttributeList>);
		mDefinition = std::move(*mPreparedLevelDefinition);
		++mConfigurationRevision;
		mPrimaryWeaponState.CommitPrepared(mConfigurationRevision);
		mPreparedLevelDefinition.reset();
	}

	void GameAbility::RebuildDefinitionForLevel()
	{
		GameAbilityDefinition definition = BuildDefinitionForLevel(mRuntimeState.GetLevel());
		mDefinition = std::move(definition);
		++mConfigurationRevision;
	}

	GameAbilityDefinition GameAbility::BuildDefinitionForLevel(int level) const
	{
		GameAbilityDefinition definition = mBaseDefinition;

		const int stepsToApply = std::max(
			0,
			level - 1 + mAbilitySystem.GetScopedAbilityLevelBonus(mBaseDefinition)
		);

		for (int stepIndex = 0; stepIndex < stepsToApply; ++stepIndex)
		{
			const AbilityLevelStep* stepPtr = mBaseDefinition.ResolveLevelStep(
				static_cast<std::size_t>(stepIndex)
			);
			if (!stepPtr)
			{
				// No cycle: no further stat gain (cooldown is added below).
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
			for (const sas::AttributeScalingRule& scalingRule : step.scalingRules)
			{
				definition.levelScalingRules.push_back(scalingRule);
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
		if (stepsToApply > 0)
		{
			definition.attributeModifiers.emplace_back(
				CommonAttributeIds::Cooldown,
				-GetGlobalAbilityCooldownTotalReduction(
					mBaseDefinition.cooldown,
					static_cast<std::size_t>(stepsToApply)
				)
			);
		}
		return definition;
	}

	void GameAbility::RefreshPrimaryWeaponRuntimeConfiguration()
	{
		if (!mPrimaryWeaponState.runtime.isInitialized ||
			mPrimaryWeaponState.weaponId.empty())
		{
			return;
		}

		for (const AbilityActionSpec& action : mDefinition.actions)
		{
			const FireWeaponAction* fireAction =
				std::get_if<FireWeaponAction>(&action.action);
			if (!fireAction ||
				fireAction->weaponDefinition.weaponId !=
					mPrimaryWeaponState.weaponId)
			{
				continue;
			}
			const uint64_t attributeRevision = mAbilitySystem.GetAttributes().GetRevision();
			const bool configurationChanged = !mPrimaryWeaponState.hasResolvedContext ||
				mPrimaryWeaponState.configurationRevision != mConfigurationRevision;
			if (configurationChanged)
			{
				const PrimaryWeaponValidationResult validation =
					PrimaryWeaponExecutionSystem::EnsureRuntimeConfigured(
						fireAction->weaponDefinition,
						mPrimaryWeaponState.runtime,
						&mDefinition.unlockedUpgradeIds
					);
				if (!validation.isValid)
				{
					return;
				}
			}
			const uint64_t modifierRevision = mAbilitySystem.GetPrimaryWeaponModifierRevision();
			if (!configurationChanged &&
				mPrimaryWeaponState.attributeRevision == attributeRevision &&
				mPrimaryWeaponState.modifierRevision == modifierRevision)
			{
				return;
			}
			AbilityExecutionContext context{ &mAbilitySystem, &mDefinition, nullptr, this };
			mPrimaryWeaponState.ApplyResolvedAttributes(
				AbilityActionAttributeResolver::ResolveAttributes(
					context,
					&fireAction->weaponDefinition,
					fireAction->weaponDefinition.attributes
				),
				attributeRevision,
				modifierRevision,
				mConfigurationRevision
			);
			return;
		}
	}
}
