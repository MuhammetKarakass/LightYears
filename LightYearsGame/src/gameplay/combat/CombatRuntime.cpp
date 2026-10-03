#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/CombatRuntime.h"
#include "framework/Actor.h"
#include "abilities/AbilityEvent.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/HealthComponent.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"
#include "presentation/effects/GameplayEffectPresentationBinding.h"
#include "attributes/AttributeMath.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace ly
{
	std::shared_ptr<Actor> CombatRuntime::LockActor(Actor* actor)
	{
		if (!actor)
		{
			return {};
		}
		return std::static_pointer_cast<Actor>(actor->GetWeakPtr().lock());
	}

	CombatRuntime::ScopedDamageDispatchFrame::ScopedDamageDispatchFrame(
		CombatRuntime& runtime,
		DamageDispatchFrame& frame
	)
		: runtime{ runtime },
		previousFrame{ runtime.mCurrentDamageDispatchFrame }
	{
		runtime.mCurrentDamageDispatchFrame = &frame;
	}

	CombatRuntime::ScopedDamageDispatchFrame::~ScopedDamageDispatchFrame()
	{
		runtime.mCurrentDamageDispatchFrame = previousFrame;
	}

	CombatRuntime::CombatRuntime(Actor& owner)
		: mOwner{ owner },
		mAbilitySystemComponent{ owner },
		mEffectPresentation{ std::make_unique<GameplayEffectPresentationBinding>(owner) }
	{
		mAbilitySystemComponent.mOwnerClearCompletion = [this] { CompleteClear(); };
		sas::AbilitySystemComponent::EffectCallbacks callbacks;
		callbacks.addStack =
			[](sas::ActiveGameplayEffect& effect)
			{
				return GetEffectBehaviorRuntime().AddStack(effect);
			};
		callbacks.cappedReapply =
			[](sas::ActiveGameplayEffect& effect, const sas::GameplayEffectSpec& incomingSpec)
			{
				if (effect.spec.definition.effectId != DamageStatusEffectIds::IgniteEffectId)
				{
					return false;
				}
				effect.spec.attributes = incomingSpec.attributes;
				effect.ResetRuntimeAttributesFromSpec();
				return true;
			};
		callbacks.tick =
			[this](sas::ActiveGameplayEffect& effect, float deltaTime)
			{
				return GetEffectBehaviorRuntime().Tick(
						effect,
						mOwner,
						deltaTime
					);
			};
		callbacks.behaviorEvent =
			[this](const sas::GameplayEffectBehaviorEvent& event)
			{
				QueueEffectEvent(event);
			};
		callbacks.activated =
			[this](sas::ActiveGameplayEffect& effect)
			{
				mEffectPresentation->Activate(effect);
			};
		callbacks.changed =
			[this](sas::ActiveGameplayEffect& effect)
			{
				mEffectPresentation->Synchronize(effect);
			};
		callbacks.stackChanged =
			[this](sas::ActiveGameplayEffect& effect)
			{
				DamageTypeSystem::SynchronizeStatusEffect(
					mAbilitySystemComponent,
					effect
				);
			};
		callbacks.removing =
			[this](sas::ActiveGameplayEffect& effect)
			{
				mEffectPresentation->Remove(effect);
			};
		mAbilitySystemComponent.SetEffectRuntimeCallbacks(std::move(callbacks));
	}

	CombatRuntime::~CombatRuntime() = default;

	CombatRuntime::OutgoingDamageRuleRegistration::~OutgoingDamageRuleRegistration()
	{
		Reset();
	}

	CombatRuntime::OutgoingDamageRuleRegistration::OutgoingDamageRuleRegistration(
		OutgoingDamageRuleRegistration&& other
	) noexcept
		: mState{ std::move(other.mState) }, mGeneration{ std::exchange(other.mGeneration, 0) }
	{
	}

	CombatRuntime::OutgoingDamageRuleRegistration& CombatRuntime::OutgoingDamageRuleRegistration::operator=(
		OutgoingDamageRuleRegistration&& other
	) noexcept
	{
		if (this == &other) return *this;
		Reset();
		mState = std::move(other.mState);
		mGeneration = std::exchange(other.mGeneration, 0);
		return *this;
	}

	void CombatRuntime::OutgoingDamageRuleRegistration::ReplaceRules(
		const List<ResolvedModuleDamageRule>& rules
	)
	{
		if (mGeneration == 0) return;
		if (std::shared_ptr<ModuleDamageRuleState> state = mState.lock())
		{
			ReplaceRegisteredDamageRules(*state, mGeneration, rules);
		}
	}

	void CombatRuntime::OutgoingDamageRuleRegistration::Reset() noexcept
	{
		if (mGeneration == 0) return;
		if (std::shared_ptr<ModuleDamageRuleState> state = mState.lock())
		{
			UnregisterDamageRules(*state, mGeneration);
		}
		mState.reset();
		mGeneration = 0;
	}

	void CombatRuntime::InitializeOwnerAttributes(float maxHealth)
	{
		sas::AttributeSystem& attributes = mAbilitySystemComponent.GetAttributes();
		attributes.RegisterAttribute(OwnerAttributeIds::MaxHealth, maxHealth);
		attributes.RegisterAttribute(OwnerAttributeIds::HealthRegen, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::EnergyPower, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::AttackPower, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::AttackSpeed, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::AbilityHaste, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::MoveSpeedHorizontal, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::MoveSpeedVertical, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::Armor, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::Luck, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::CriticalChance, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::CriticalDamage, 1.5f);
		attributes.RegisterAttribute(OwnerAttributeIds::OffenseDominant, 0.f);
		attributes.RegisterAttribute(OwnerAttributeIds::OffenseSecondary, 0.f);
	}

	float CombatRuntime::GetCriticalChance() const
	{
		return sas::AttributeMath::GetCriticalChance(
			mAbilitySystemComponent.GetAttributes().GetCurrentValue(
				OwnerAttributeIds::CriticalChance
			)
		);
	}

	float CombatRuntime::GetCombatLuckFactor() const
	{
		return sas::AttributeMath::GetCombatLuckFactor(
			mAbilitySystemComponent.GetAttributes().GetCurrentValue(
				OwnerAttributeIds::Luck
			)
		);
	}

	float CombatRuntime::GetCriticalDamageMultiplier() const
	{
		return std::max(1.f, mAbilitySystemComponent.GetAttributes().GetCurrentValue(OwnerAttributeIds::CriticalDamage));
	}

	void CombatRuntime::SetDamageProtection(
		const std::string& sourceId,
		bool blocksIncomingDamage,
		bool blocksOutgoingDamage
	)
	{
		if (sourceId.empty())
		{
			return;
		}
		mDamageProtections[sourceId] = DamageProtection{
			blocksIncomingDamage,
			blocksOutgoingDamage
		};
	}

	void CombatRuntime::RemoveDamageProtection(const std::string& sourceId)
	{
		mDamageProtections.erase(sourceId);
	}

	bool CombatRuntime::BlocksIncomingDamage() const
	{
		for (const auto& [sourceId, protection] : mDamageProtections)
		{
			(void)sourceId;
			if (protection.blocksIncomingDamage)
			{
				return true;
			}
		}
		return false;
	}

	bool CombatRuntime::BlocksOutgoingDamage() const
	{
		for (const auto& [sourceId, protection] : mDamageProtections)
		{
			(void)sourceId;
			if (protection.blocksOutgoingDamage)
			{
				return true;
			}
		}
		return false;
	}

	void CombatRuntime::Tick(float deltaTime)
	{
		if (deltaTime > 0.f)
		{
			for (ModuleDamageRuleRegistrationEntry& registration : mModuleDamageRuleState->registrations)
			{
				for (ActiveModuleDamageRule& active : registration.rules)
				{
					if (active.resolved.rule.activeWhileTrigger != ModuleTriggerKind::None)
						active.remainingSeconds = std::max(0.f, active.remainingSeconds - deltaTime);
				}
			}
		}
		mAbilitySystemComponent.Tick(deltaTime);
		DispatchPendingEffectEvents();
	}

	bool CombatRuntime::SetRuntimeModifier(const std::string& sourceId, CombatRuntimeModifier modifier)
	{
		return mRuntimeModifiers.Set(sourceId, modifier);
	}

	bool CombatRuntime::RemoveRuntimeModifier(const std::string& sourceId)
	{
		return mRuntimeModifiers.Remove(sourceId);
	}

	float CombatRuntime::GetOutgoingDamageMultiplier() const noexcept
	{
		return mRuntimeModifiers.GetOutgoingDamageMultiplier();
	}

	CombatRuntime::OutgoingDamageRuleRegistration CombatRuntime::RegisterOutgoingDamageRules()
	{
		std::uint64_t generation = mModuleDamageRuleState->nextGeneration++;
		if (generation == 0) generation = mModuleDamageRuleState->nextGeneration++;
		mModuleDamageRuleState->registrations.push_back({ generation, {} });
		return OutgoingDamageRuleRegistration{ mModuleDamageRuleState, generation };
	}

	void CombatRuntime::ActivateOutgoingDamageRules(ModuleTriggerKind trigger)
	{
		if (trigger == ModuleTriggerKind::None) return;
		for (ModuleDamageRuleRegistrationEntry& registration : mModuleDamageRuleState->registrations)
		{
			for (ActiveModuleDamageRule& active : registration.rules)
			{
				const ModuleDamageRule& rule = active.resolved.rule;
				if (rule.activeWhileTrigger == trigger && rule.activeDurationSeconds > 0.f)
					active.remainingSeconds = rule.activeDurationSeconds;
			}
		}
	}

	CombatRuntime::OutgoingDamagePlan CombatRuntime::ResolveOutgoingDamagePlan(
		const Actor& target,
		const List<GameplayTag>& sourceAbilityTags
	) const
	{
		OutgoingDamagePlan plan;
		const bool isPrimaryShot = std::any_of(sourceAbilityTags.begin(), sourceAbilityTags.end(),
			[](const GameplayTag& tag) { return tag.MatchesTag(GameplayTags::Ability::Primary); });
		const HealthComponent* targetHealth = target.FindComponent<HealthComponent>();
		const float healthFraction = targetHealth && targetHealth->GetMaxHealth() > 0.f
			? targetHealth->GetHealth() / targetHealth->GetMaxHealth()
			: 1.f;
		for (const ModuleDamageRuleRegistrationEntry& registration : mModuleDamageRuleState->registrations)
		{
			for (const ActiveModuleDamageRule& active : registration.rules)
			{
				const ModuleDamageRule& rule = active.resolved.rule;
				if (rule.activeWhileTrigger != ModuleTriggerKind::None && active.remainingSeconds <= 0.f) continue;
				const bool conditionMet = rule.condition == ModuleDamageCondition::PrimaryShotOnly
					? isPrimaryShot
					: healthFraction <= rule.conditionValue;
				if (!conditionMet) continue;
				plan.damageMultiplier *= rule.damageMultiplier;
				if (rule.bonusDamageFraction > 0.f && rule.bonusDamageType.IsValid())
					plan.bonuses.push_back({ rule.bonusDamageFraction, rule.bonusDamageType });
			}
		}
		return plan;
	}

	bool CombatRuntime::SameDamageRule(const ModuleDamageRule& left, const ModuleDamageRule& right)
	{
		return left.condition == right.condition && left.conditionValue == right.conditionValue &&
			left.damageMultiplier == right.damageMultiplier && left.bonusDamageFraction == right.bonusDamageFraction &&
			left.bonusDamageType == right.bonusDamageType && left.bonusCriticalPolicy == right.bonusCriticalPolicy &&
			left.activeWhileTrigger == right.activeWhileTrigger &&
			left.activeDurationSeconds == right.activeDurationSeconds;
	}

	void CombatRuntime::ReplaceRegisteredDamageRules(
		ModuleDamageRuleState& state,
		std::uint64_t generation,
		const List<ResolvedModuleDamageRule>& rules
	)
	{
		const auto registration = std::find_if(state.registrations.begin(), state.registrations.end(),
			[generation](const ModuleDamageRuleRegistrationEntry& entry) { return entry.generation == generation; });
		if (registration == state.registrations.end()) return;

		List<ActiveModuleDamageRule> nextRules;
		nextRules.reserve(rules.size());
		for (const ResolvedModuleDamageRule& resolved : rules)
		{
			ActiveModuleDamageRule next{ resolved, 0.f };
			const auto previous = std::find_if(registration->rules.begin(), registration->rules.end(),
				[&resolved](const ActiveModuleDamageRule& active)
				{
					return active.resolved.instanceId == resolved.instanceId &&
						active.resolved.ruleIndex == resolved.ruleIndex &&
						SameDamageRule(active.resolved.rule, resolved.rule);
				});
			if (previous != registration->rules.end()) next.remainingSeconds = previous->remainingSeconds;
			nextRules.push_back(std::move(next));
		}
		registration->rules = std::move(nextRules);
	}

	void CombatRuntime::UnregisterDamageRules(ModuleDamageRuleState& state, std::uint64_t generation)
	{
		const auto registration = std::find_if(state.registrations.begin(), state.registrations.end(),
			[generation](const ModuleDamageRuleRegistrationEntry& entry) { return entry.generation == generation; });
		if (registration != state.registrations.end()) state.registrations.erase(registration);
	}

	sas::ClearResult CombatRuntime::Clear()
	{
		++mEffectEventGeneration;
		mPendingEffectEvents.clear();
		mClearRequested = true;
		return mAbilitySystemComponent.Clear();
	}

	void CombatRuntime::CompleteClear()
	{
		if (!mClearRequested) return;
		mClearRequested = false;
		mContactDamageGuardRegistry.Clear();
		std::exception_ptr error;
		try { mEffectPresentation->Clear(); }
		catch (...) { error = std::current_exception(); }
		mPendingEffectEvents.clear();
		mDamageProtections.clear();
		mRuntimeModifiers.Clear();
		for (ModuleDamageRuleRegistrationEntry& registration : mModuleDamageRuleState->registrations)
		{
			for (ActiveModuleDamageRule& active : registration.rules) active.remainingSeconds = 0.f;
		}
		if (error) std::rethrow_exception(error);
	}

	void CombatRuntime::ProcessIncomingDamage(DamageContext& context)
	{
		DamageDispatchFrame frame;
		frame.context = &context;
		frame.generation = mEffectEventGeneration;
		frame.sourceLifetime = LockActor(context.source);
		frame.targetLifetime = LockActor(context.target);
		frame.deliveryActorLifetime = LockActor(context.deliveryActor);
		frame.ownerLifetime = LockActor(&mOwner);
		ScopedDamageDispatchFrame frameScope{ *this, frame };
		const auto isFrameCurrent = [this, &frame]
		{
			return mEffectEventGeneration == frame.generation;
		};

		// Capture target identity and location before any status/death callback can
		// destroy the owning ship. Kill-driven abilities consume this snapshot
		// instead of dereferencing a possibly invalid event target pointer.
		context.targetWasEnemyCombatant =
			mOwner.GetCollisionLayer() == CollisionLayer::Enemy &&
			dynamic_cast<Combatant*>(&mOwner) != nullptr;
		const sf::Vector2f targetLocation = mOwner.GetActorLocation();
		context.targetLocationAtResolution.x = targetLocation.x;
		context.targetLocationAtResolution.y = targetLocation.y;
		context.targetArmorShred = DamageTypeSystem::ResolveArmorShred(mAbilitySystemComponent);
		frame.capturesEffectEvents = true;
		mAbilitySystemComponent.ProcessGameplayEffectEvent(
			context,
			std::vector<IncomingDamagePhase>{
				IncomingDamagePhase::PreMitigation,
				IncomingDamagePhase::Standard
			},
			[](const sas::ActiveGameplayEffect& effect)
			{
				return GetEffectBehaviorRuntime().GetEventPhase(
					effect,
					IncomingDamagePhase::Standard
				);
			},
			[](sas::ActiveGameplayEffect& effect, DamageContext& damageContext)
			{
				return GetEffectBehaviorRuntime().ProcessEvent(
					effect,
					damageContext
				);
			},
			[](const DamageContext& damageContext)
			{
				return damageContext.remainingDamage > 0.f;
			}
		);
		frame.capturesEffectEvents = false;
		if (!isFrameCurrent())
		{
			return;
		}
		DispatchPendingEffectEvents();
		if (!isFrameCurrent())
		{
			return;
		}
		DispatchDamageEffectEvents(frame);
		if (!isFrameCurrent())
		{
			return;
		}

		// Armor is resolved after shield absorption by the owning combatant.
		// Keep incoming event and status timing above unchanged.

		// One generic combat event carries the incoming damage context and its
		// semantic payload tags. Ability triggers can filter owner
		// status tags and payload damage tags without per-combination event tags.
		sas::AbilityEvent damageEvent;
		damageEvent.eventTag = GameplayTags::Event::Combat::DamageReceived;
		damageEvent.SetSource(context.source);
		damageEvent.SetTarget(&mOwner);
		damageEvent.payloadTags = context.damageTags;
		damageEvent.sourceAbilityId = context.sourceAbilityId;
		damageEvent.sourceAbilityTags = context.sourceAbilityTags;
		damageEvent.SetContext(&context);
		mAbilitySystemComponent.HandleGameplayEvent(damageEvent);
		if (!isFrameCurrent())
		{
			return;
		}

		const List<GameplayTag> appliedStatuses =
			DamageTypeSystem::ApplyStatusEffects(
				mAbilitySystemComponent,
				context
			);
		if (!isFrameCurrent())
		{
			return;
		}
		// HealthComponent::HealthEmpty can synchronously destroy the ship and
		// clear its effects. Capture Cryo state now, after this hit's Cryo status
		// application, so KillConfirmed can still observe it later in the source
		// combatant's notification path.
		const auto* cryoEffect = mAbilitySystemComponent.FindGameplayEffectById(
			DamageStatusEffectIds::CryoSlowedEffectId
		);
		context.targetWasCryoAffected = cryoEffect != nullptr;
		context.targetCryoStacksAtResolution = cryoEffect ? std::max(0, cryoEffect->stackCount) : 0;
		if (auto* sourceCombatant = context.source ? dynamic_cast<Combatant*>(context.source) : nullptr)
		{
			for (const GameplayTag& status : appliedStatuses)
			{
				if (status != DamageStatusSchema::Ignite)
				{
					continue;
				}
				sas::AbilityEvent event;
				event.eventTag = GameplayTags::Event::Source::StatusApplied::Ignite;
				event.SetSource(context.source);
				event.SetTarget(context.target);
				event.magnitude = context.remainingDamage;
				event.SetContext(&context);
				sourceCombatant->GetAbilitySystemComponent().HandleGameplayEvent(event);
				if (!isFrameCurrent())
				{
					return;
				}
			}
		}
		onDamageProcessed.Broadcast(context);
	}

	void CombatRuntime::ApplyHullDamageMitigation(DamageContext& context)
	{
		// Kinetic armor shred lowers the target's Armor first (any hit, any damage
		// type); the hit's own armor penetration then applies to what remains.
		const float armor = std::max(
			0.f,
			mAbilitySystemComponent.GetAttributes().GetCurrentValue(OwnerAttributeIds::Armor)
		);
		const float shreddedArmor = armor * (1.f - std::clamp(context.targetArmorShred, 0.f, 1.f));
		const float armorDamageMultiplier = sas::AttributeMath::GetArmorDamageMultiplier(shreddedArmor);
		// Preserve the established penetration semantics while resolving from the
		// multiplier directly. This avoids 1 - reduction cancellation at extreme
		// but finite Armor values, where float reduction can round to exactly 1.
		const float finalArmorDamageMultiplier = armorDamageMultiplier +
			context.payload.armorPenetration * (1.f - armorDamageMultiplier);
		const float damageBeforeArmor = context.remainingDamage;
		context.remainingDamage = std::max(0.f, damageBeforeArmor * finalArmorDamageMultiplier);
		context.mitigatedDamage += damageBeforeArmor - context.remainingDamage;
		context.modifiedDamage = context.remainingDamage;
	}

	void CombatRuntime::QueueEffectEvent(
		const sas::GameplayEffectBehaviorEvent& behaviorEvent
	)
	{
		DamageDispatchFrame* frame = mCurrentDamageDispatchFrame;
		if (frame && frame->capturesEffectEvents)
		{
			if (frame->generation != mEffectEventGeneration)
			{
				return;
			}

			QueuedEffectEvent queuedEvent;
			queuedEvent.eventTag = behaviorEvent.eventTag;
			queuedEvent.magnitude = behaviorEvent.magnitude;
			queuedEvent.hasSource = frame->context->source != nullptr;
			queuedEvent.source = LockActor(frame->context->source);
			queuedEvent.target = LockActor(&mOwner);
			queuedEvent.generation = frame->generation;
			frame->effectEvents.push_back(std::move(queuedEvent));
			return;
		}

		QueuedEffectEvent queuedEvent;
		queuedEvent.eventTag = behaviorEvent.eventTag;
		queuedEvent.magnitude = behaviorEvent.magnitude;
		queuedEvent.hasSource = true;
		queuedEvent.source = LockActor(&mOwner);
		queuedEvent.target = LockActor(&mOwner);
		queuedEvent.generation = mEffectEventGeneration;
		mPendingEffectEvents.push_back(std::move(queuedEvent));
	}

	void CombatRuntime::DispatchPendingEffectEvents()
	{
		List<QueuedEffectEvent> events = std::move(mPendingEffectEvents);
		mPendingEffectEvents.clear();
		const std::uint64_t batchGeneration = mEffectEventGeneration;
		for (const QueuedEffectEvent& event : events)
		{
			if (mEffectEventGeneration != batchGeneration || event.generation != batchGeneration)
			{
				break;
			}
			DispatchEffectEvent(event, nullptr, batchGeneration);
			if (mEffectEventGeneration != batchGeneration)
			{
				break;
			}
		}
	}

	void CombatRuntime::DispatchDamageEffectEvents(DamageDispatchFrame& frame)
	{
		List<QueuedEffectEvent> events = std::move(frame.effectEvents);
		frame.effectEvents.clear();
		for (const QueuedEffectEvent& event : events)
		{
			if (mEffectEventGeneration != frame.generation || event.generation != frame.generation)
			{
				break;
			}
			DispatchEffectEvent(event, frame.context, frame.generation);
			if (mEffectEventGeneration != frame.generation)
			{
				break;
			}
		}
	}

	void CombatRuntime::DispatchEffectEvent(
		const QueuedEffectEvent& queuedEvent,
		const DamageContext* context,
		std::uint64_t batchGeneration
	)
	{
		if (mEffectEventGeneration != batchGeneration || queuedEvent.generation != batchGeneration)
		{
			return;
		}

		const std::shared_ptr<Actor> source = queuedEvent.hasSource ? queuedEvent.source.lock() : nullptr;
		const std::shared_ptr<Actor> target = queuedEvent.target.lock();
		if ((queuedEvent.hasSource && !source) || !target)
		{
			return;
		}

		std::shared_ptr<Actor> contextSource;
		std::shared_ptr<Actor> contextTarget;
		std::shared_ptr<Actor> contextDeliveryActor;
		if (context)
		{
			contextSource = LockActor(context->source);
			contextTarget = LockActor(context->target);
			contextDeliveryActor = LockActor(context->deliveryActor);
			if ((context->source && !contextSource) ||
				(context->target && !contextTarget) ||
				(context->deliveryActor && !contextDeliveryActor))
			{
				return;
			}
		}

		if (mEffectEventGeneration != batchGeneration)
		{
			return;
		}
		sas::AbilityEvent event;
		event.eventTag = queuedEvent.eventTag;
		event.SetSource(source.get());
		event.SetTarget(target.get());
		event.magnitude = queuedEvent.magnitude;
		event.SetContext(context);
		mAbilitySystemComponent.HandleGameplayEvent(event);
	}

	void CombatRuntime::NotifyDamageResolved(const DamageContext& context)
	{
		if (context.appliedDamage <= 0.f)
		{
			return;
		}

		sas::AbilityEvent event;
		event.eventTag = CombatEventSchema::OwnerDamageTaken;
		event.SetSource(context.source);
		event.SetTarget(context.target);
		event.magnitude = context.appliedDamage;
		event.payloadTags = context.damageTags;
		event.sourceAbilityId = context.sourceAbilityId;
		event.sourceAbilityTags = context.sourceAbilityTags;
		event.SetContext(&context);
		mAbilitySystemComponent.HandleGameplayEvent(event);

		if (auto* sourceCombatant = context.source ? dynamic_cast<Combatant*>(context.source) : nullptr)
		{
			sas::AbilityEvent sourceEvent;
			sourceEvent.eventTag = GameplayTags::Event::Source::DamageDealt;
			sourceEvent.SetSource(context.source);
			sourceEvent.SetTarget(context.target);
			sourceEvent.magnitude = context.appliedDamage;
			sourceEvent.payloadTags = context.damageTags;
			sourceEvent.sourceAbilityId = context.sourceAbilityId;
			sourceEvent.sourceAbilityTags = context.sourceAbilityTags;
			sourceEvent.SetContext(&context);
			sourceCombatant->GetAbilitySystemComponent().HandleGameplayEvent(sourceEvent);

			sas::AbilityEvent combatEvent;
			combatEvent.eventTag = GameplayTags::Event::Combat::DamageDealt;
			combatEvent.SetSource(context.source);
			combatEvent.SetTarget(context.target);
			combatEvent.magnitude = context.appliedDamage;
			combatEvent.payloadTags = context.damageTags;
			combatEvent.sourceAbilityId = context.sourceAbilityId;
			combatEvent.sourceAbilityTags = context.sourceAbilityTags;
			combatEvent.SetContext(&context);
			sourceCombatant->GetAbilitySystemComponent().HandleGameplayEvent(combatEvent);
			sourceCombatant->GetCombatRuntime().onDamageDealt.Broadcast(context);

			if (context.targetWasKilled)
			{
				sas::AbilityEvent killEvent;
				killEvent.eventTag = GameplayTags::Event::Combat::KillConfirmed;
				killEvent.SetSource(context.source);
				killEvent.SetTarget(context.target);
				killEvent.magnitude = context.appliedDamage;
				killEvent.payloadTags = context.damageTags;
				killEvent.sourceAbilityId = context.sourceAbilityId;
				killEvent.sourceAbilityTags = context.sourceAbilityTags;
				killEvent.SetContext(&context);
				sourceCombatant->GetAbilitySystemComponent().HandleGameplayEvent(killEvent);
			}
		}
		onDamageResolved.Broadcast(context);
	}
}
