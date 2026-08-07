#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/GameAbility.h"
#include "attributes/AttributeMath.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "abilities/AbilityEvent.h"
#include "attributes/AttributeSystem.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"
#include "framework/Actor.h"
#include <algorithm>
#include <variant>

namespace ly
{
	GameAbility::GameAbility(
		LightYearsAbilitySystemComponent& abilitySystem,
		sas::AbilityHandle handle,
		const GameAbilityDefinition& definition,
		unique_ptr<GameAbilityBehavior> behavior)
		: sas::GameplayAbilityInstance<
			GameAbilityDefinition,
			GameAbilityExecution
		>{
			handle,
			definition,
			abilitySystem.CreateAbilityInstanceNotifications()
		},
		mAbilitySystem{ abilitySystem },
		mBehavior{ std::move(behavior) }
	{
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
		const bool removed = mAttachments.Remove(attachmentId, hostKind);
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
		for (const EquippedAttachment& equipped : mAttachments.GetEquipped())
		{
			for (const AttachmentEventRule& rule : equipped.definition.eventRules)
			{
				if (!event.eventTag.MatchesTag(rule.eventTag) ||
					(rule.requireOwnerAsEventSource &&
						event.GetSource<Actor>() != &mAbilitySystem.GetOwner()))
				{
					continue;
				}

				const DamageContext* damageContext =
					event.GetContext<DamageContext>();
				const List<GameplayTag>* damageTags = damageContext
					? &damageContext->damageTags
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
						mAbilitySystem.ReduceAbilityCooldowns(magnitude, true);
						break;
					case AttachmentCooldownTarget::AllNonPrimaryAbilities:
						mAbilitySystem.ReduceAbilityCooldowns(magnitude, false);
						break;
					}
					break;
				}
			}
		}
	}

	bool GameAbility::CanActivateContent() const
	{
		// Standard locks are checked here so every current and future game ability
		// shares one activation gate. Producers only need to grant the shared tag.
		const GameplayTag& sharedBlockTag =
			mDefinition.slot == sas::AbilitySlot::PrimaryFire
			? GameplayTagSchema::BlockPrimaryWeaponFire
			: GameplayTagSchema::BlockAbilityActivation;
		return mBehavior &&
			mAbilitySystem.HasAllOwnedTags(mDefinition.requiredOwnerTags) &&
			!mAbilitySystem.HasAnyOwnedTags(mDefinition.blockedOwnerTags) &&
			!mAbilitySystem.HasOwnedTag(sharedBlockTag);
	}

	bool GameAbility::ActivateContent()
	{
		GameAbilityBehaviorContext behaviorContext{
			mAbilitySystem,
			*this,
			mAbilitySystem.GetOwner(),
			mDefinition
		};
		return mBehavior->Activate(behaviorContext);
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
	}

	int GameAbility::GetMaximumLevel() const
	{
		return mBaseDefinition.GetMaxLevel();
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

	float GameAbility::ResolveActiveDuration() const
	{
		const float leveledDuration = sas::CalculateModifiedAttributeValue(
			sas::GameplayAttribute{ CommonAttributeIds::Duration, mDefinition.duration, 0.f },
			mDefinition.attributeModifiers
		);
		return ApplyAttachmentModifiers(
			AttachmentHostKind::Ability,
			sas::GameplayAttribute{ CommonAttributeIds::Duration, leveledDuration, 0.f }
		).currentValue;
	}

	void GameAbility::OnLevelConfigurationChanged()
	{
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
			std::max(0, mRuntimeState.GetLevel() - 1),
			static_cast<int>(mBaseDefinition.levelProgression.size())
		);

		for (int stepIndex = 0; stepIndex < stepsToApply; ++stepIndex)
		{
			const AbilityLevelStep& step = mBaseDefinition.levelProgression[stepIndex];
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
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
			if (sas::FindGameplayAttribute(fireAction->weaponDefinition.attributes, CommonAttributeIds::Damage))
			{
				addCapability(AttachmentSchema::Capability::Damage);
			}
			if (sas::FindGameplayAttribute(fireAction->weaponDefinition.attributes, CommonAttributeIds::FireRate))
			{
				addCapability(AttachmentSchema::Capability::FireRate);
			}
			if (fireAction->weaponDefinition.weaponTypeTag.MatchesTag(PrimaryWeaponSchema::Projectile::FamilyTag))
			{
				addCapability(AttachmentSchema::Capability::Projectile);
			}
			if (fireAction->weaponDefinition.weaponTypeTag.MatchesTag(PrimaryWeaponSchema::Beam::FamilyTag))
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


