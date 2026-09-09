#include "gameplay/ability/emberSwarm/EmberSwarmAbility.h"

#include "framework/World.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/ability/emberSwarm/EmberDroneActor.h"
#include "gameplay/ability/emberSwarm/EmberSwarmContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/emberSwarm/EmberSwarmPresentationIds.h"
#include "presentation/ability/emberSwarm/EmberSwarmPresentationProfile.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		constexpr std::size_t DroneCount = 3;
		constexpr std::size_t RequiredProgressionStepCount = 14;
		constexpr float BaseCooldown = 14.f;
		constexpr float BaseDuration = 6.f;
		constexpr float CooldownPerLevel = -0.20f;
		constexpr float Epsilon = 0.0001f;

		int GetTrueIgniteStacks(const Actor* target)
		{
			if (!target)
			{
				return 0;
			}
			const auto* combatant = dynamic_cast<const Combatant*>(target);
			if (!combatant)
			{
				return 0;
			}
			const sas::ActiveGameplayEffect* activeIgnite =
				combatant->GetAbilitySystemComponent().FindGameplayEffectById(
					DamageStatusEffectIds::IgniteEffectId
				);
			return activeIgnite ? activeIgnite->stackCount : 0;
		}

		bool NearlyEqual(float left, float right)
		{
			return std::isfinite(left) && std::isfinite(right) &&
				std::abs(left - right) <= Epsilon;
		}

		bool HasExactAbilityTags(const GameAbilityDefinition& definition)
		{
			if (definition.abilityTags.size() != 2)
			{
				return false;
			}

			bool hasCategory = false;
			bool hasFamily = false;
			for (const GameplayTag& tag : definition.abilityTags)
			{
				hasCategory = hasCategory || tag.MatchesTagExact(
					AbilityData::EmberSwarm::CategoryTag
				);
				hasFamily = hasFamily || tag.MatchesTagExact(
					AbilityData::EmberSwarm::FamilyTag
				);
			}
			return hasCategory && hasFamily;
		}

		bool HasExpectedModifier(
			const AbilityLevelStep& step,
			const sas::AttributeId& attributeId,
			float expectedMagnitude
		)
		{
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
			{
				if (modifier.attributeId == attributeId &&
					modifier.operation == sas::AttributeModifierOperation::Add &&
					NearlyEqual(modifier.magnitude, expectedMagnitude))
				{
					return true;
				}
			}
			return false;
		}
	}

	bool EmberSwarmAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const bool validIdentity =
			definition.abilityId == AbilityData::EmberSwarm::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::EmberSwarm &&
			HasExactAbilityTags(definition);
		const bool validLifecycle =
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Duration &&
			definition.maxCharges == 1 &&
			NearlyEqual(definition.cooldown, BaseCooldown) &&
			NearlyEqual(definition.duration, BaseDuration);

		if (!validIdentity || !validLifecycle)
		{
			if (failureReason)
			{
				*failureReason =
					"Ember Swarm requires pressed activation, duration lifetime, cooldown 14, duration 6, and maxCharges 1.";
			}
			return false;
		}

		if (definition.levelProgression.size() != RequiredProgressionStepCount)
		{
			if (failureReason)
			{
				*failureReason =
					"Ember Swarm requires fourteen progression steps through level fifteen.";
			}
			return false;
		}

		float resolvedCooldown = definition.cooldown;
		for (const AbilityLevelStep& step : definition.levelProgression)
		{
			if (step.attributeModifiers.size() != 1 ||
				!step.unlockedUpgradeIds.empty() ||
				!step.addedActions.empty() ||
				!step.addedTriggers.empty() ||
				!HasExpectedModifier(step, CommonAttributeIds::Cooldown, CooldownPerLevel))
			{
				if (failureReason)
				{
					*failureReason =
						"Ember Swarm progression may only add -0.20 Common.Cooldown per level.";
				}
				return false;
			}

			resolvedCooldown += CooldownPerLevel;
			if (resolvedCooldown <= 0.f)
			{
				if (failureReason)
				{
					*failureReason =
						"Ember Swarm progression must keep cooldown positive at every level.";
				}
				return false;
			}
		}

		return true;
	}

	float EmberSwarmAbility::ResolveActiveDuration(
		const GameAbilityBehaviorContext& context,
		float defaultDuration
	) const
	{
		(void)context;
		return std::max(0.f, std::isfinite(defaultDuration) ? defaultDuration : BaseDuration);
	}

	bool EmberSwarmAbility::Activate(GameAbilityBehaviorContext& context)
	{
		if (mActive || !dynamic_cast<SpaceShip*>(&context.owner))
		{
			return false;
		}

		World* world = context.owner.GetWorld();
		const EmberSwarmPresentationProfile* presentationProfile =
			PresentationProfileRegistry<EmberSwarmPresentationProfile>::Find(
				EmberSwarmPresentationIds::DroneBasic
			);
		if (!world || !presentationProfile)
		{
			return false;
		}

		const float resolvedDuration = context.instance.GetActiveDuration();
		if (!std::isfinite(resolvedDuration) || resolvedDuration <= 0.f)
		{
			return false;
		}

		mDrones.clear();
		for (std::size_t droneIndex = 0; droneIndex < DroneCount; ++droneIndex)
		{
			EmberDroneActor::Configuration config;
			config.droneIndex = droneIndex;
			config.orbitRadius = 80.f;
			config.targetOrbitRadius = 65.f;
			config.angularSpeed = 2.0f;
			config.travelSpeed = 800.f;
			config.pulseInterval = 0.25f;

			const weak_ptr<EmberDroneActor> droneWeak =
				world->SpawnActor<EmberDroneActor>(
					&context.owner,
					*presentationProfile,
					config
				);
			const shared_ptr<EmberDroneActor> drone = droneWeak.lock();
			if (!drone)
			{
				DestroyDrones();
				return false;
			}

			drone->SetSourceAbility(
				sas::ContentId{ context.definition.abilityId },
				context.definition.abilityTags
			);
			drone->SetLifeTime(resolvedDuration);
			drone->SetAbilityLevel(context.instance.GetLevel());
			mDrones.push_back(droneWeak);
		}

		mActive = true;
		mTargetEvaluationTimer = 0.f;
		context.abilitySystem.AddOwnedTag(AbilityData::EmberSwarm::State::Active);
		EmitEvent(context, AbilityData::EmberSwarm::Event::Started);

		EvaluateTargets(context);
		return true;
	}

	void EmberSwarmAbility::Tick(GameAbilityBehaviorContext& context, float deltaTime)
	{
		if (!mActive)
		{
			return;
		}

		mTargetEvaluationTimer += std::max(0.f, deltaTime);
		while (mTargetEvaluationTimer >= 0.25f)
		{
			mTargetEvaluationTimer -= 0.25f;
			EvaluateTargets(context);
		}
	}

	void EmberSwarmAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		DestroyDrones();
		if (!mActive)
		{
			return;
		}

		context.abilitySystem.RemoveOwnedTag(AbilityData::EmberSwarm::State::Active);
		mActive = false;
		mTargetEvaluationTimer = 0.f;
		EmitEvent(context, AbilityData::EmberSwarm::Event::Ended);
	}

	void EmberSwarmAbility::EvaluateTargets(GameAbilityBehaviorContext& context)
	{
		Actor& owner = context.owner;
		World* world = owner.GetWorld();
		if (!world)
		{
			return;
		}

		const sf::Vector2f ownerLocation = owner.GetActorLocation();
		constexpr float SearchRadius = 750.f;
		constexpr float LeashRadius = 1000.f;
		constexpr float LeashRadiusSq = LeashRadius * LeashRadius;

		// Collect live drones
		List<shared_ptr<EmberDroneActor>> liveDrones;
		for (const weak_ptr<EmberDroneActor>& droneWeak : mDrones)
		{
			if (shared_ptr<EmberDroneActor> drone = droneWeak.lock())
			{
				if (!drone->GetIsPendingDestroy())
				{
					drone->SetAbilityLevel(context.instance.GetLevel());
					liveDrones.push_back(drone);
				}
			}
		}

		if (liveDrones.empty())
		{
			return;
		}

		// 1. Query candidate targets within 750 of owner using shared target query
		const List<shared_ptr<Actor>> rawCandidates =
			targeting::FindOpposingCombatants(*world, owner, ownerLocation, SearchRadius, false);

		List<shared_ptr<Actor>> validCandidates;
		bool hasUnderCapCandidate = false;
		for (const shared_ptr<Actor>& candidate : rawCandidates)
		{
			if (!candidate || candidate->GetIsPendingDestroy())
			{
				continue;
			}
			if (const auto* combatant = dynamic_cast<const Combatant*>(candidate.get()))
			{
				if (const auto* ship = dynamic_cast<const SpaceShip*>(candidate.get()))
				{
					if (ship->GetHealthComponent().GetHealth() <= 0.f)
					{
						continue;
					}
				}
				validCandidates.push_back(candidate);
				if (GetTrueIgniteStacks(candidate.get()) < 4)
				{
					hasUnderCapCandidate = true;
				}
			}
		}

		// Track assigned drones per candidate target
		Dictionary<const Actor*, int> assignedDroneCounts;
		for (const auto& candidate : validCandidates)
		{
			assignedDroneCounts[candidate.get()] = 0;
		}

		struct DronePlan
		{
			shared_ptr<EmberDroneActor> drone;
			bool needsAssignment = false;
		};

		List<DronePlan> plans;
		plans.reserve(liveDrones.size());

		for (const auto& drone : liveDrones)
		{
			DronePlan plan;
			plan.drone = drone;

			Actor* currentTarget = drone->GetTarget();
			bool targetValid = false;

			if (currentTarget && !currentTarget->GetIsPendingDestroy())
			{
				bool isAlive = true;
				if (const auto* ship = dynamic_cast<const SpaceShip*>(currentTarget))
				{
					isAlive = ship->GetHealthComponent().GetHealth() > 0.f;
				}

				if (isAlive)
				{
					const sf::Vector2f delta = currentTarget->GetActorLocation() - ownerLocation;
					const float distSq = delta.x * delta.x + delta.y * delta.y;
					if (distSq <= LeashRadiusSq)
					{
						targetValid = true;
					}
				}
			}

			if (!targetValid)
			{
				plan.needsAssignment = true;
				drone->ClearTarget();
			}
			else
			{
				const int currentIgnite = GetTrueIgniteStacks(currentTarget);
				const bool isCapped = (currentIgnite >= 4);

				if (isCapped && hasUnderCapCandidate)
				{
					plan.needsAssignment = true;
					drone->ClearTarget();
				}
				else
				{
					plan.needsAssignment = false;
					assignedDroneCounts[currentTarget]++;
				}
			}

			plans.push_back(plan);
		}

		// Assign targets to drones needing assignment:
		// "Assignment must be current-snapshot O(3N), no persistent stale queue: prefer targets actual Ignite <4,
		//  fewer actual Ignite, fewer current/batch-reserved Ember drones, nearer owner, stable identity."
		for (auto& plan : plans)
		{
			if (!plan.needsAssignment)
			{
				continue;
			}

			if (validCandidates.empty())
			{
				plan.drone->ClearTarget();
				continue;
			}

			shared_ptr<Actor> bestCandidate = nullptr;
			int bestIgnite = 999;
			bool bestUnderCap = false;
			int bestDroneCount = 999;
			float bestDistSq = 1e30f;

			for (const auto& candidate : validCandidates)
			{
				const int candidateIgnite = GetTrueIgniteStacks(candidate.get());
				const bool candidateUnderCap = (candidateIgnite < 4);
				const int candidateDrones = assignedDroneCounts[candidate.get()];
				const sf::Vector2f delta = candidate->GetActorLocation() - ownerLocation;
				const float candidateDistSq = delta.x * delta.x + delta.y * delta.y;

				if (!bestCandidate)
				{
					bestCandidate = candidate;
					bestUnderCap = candidateUnderCap;
					bestIgnite = candidateIgnite;
					bestDroneCount = candidateDrones;
					bestDistSq = candidateDistSq;
					continue;
				}

				// 1. prefer actual Ignite < 4
				if (candidateUnderCap != bestUnderCap)
				{
					if (candidateUnderCap)
					{
						bestCandidate = candidate;
						bestUnderCap = candidateUnderCap;
						bestIgnite = candidateIgnite;
						bestDroneCount = candidateDrones;
						bestDistSq = candidateDistSq;
					}
					continue;
				}

				// 2. fewer actual Ignite
				if (candidateIgnite != bestIgnite)
				{
					if (candidateIgnite < bestIgnite)
					{
						bestCandidate = candidate;
						bestUnderCap = candidateUnderCap;
						bestIgnite = candidateIgnite;
						bestDroneCount = candidateDrones;
						bestDistSq = candidateDistSq;
					}
					continue;
				}

				// 3. fewer current/batch-reserved Ember drones
				if (candidateDrones != bestDroneCount)
				{
					if (candidateDrones < bestDroneCount)
					{
						bestCandidate = candidate;
						bestUnderCap = candidateUnderCap;
						bestIgnite = candidateIgnite;
						bestDroneCount = candidateDrones;
						bestDistSq = candidateDistSq;
					}
					continue;
				}

				// 4. nearer owner
				constexpr float DistTolerance = 0.01f;
				if (std::abs(candidateDistSq - bestDistSq) > DistTolerance)
				{
					if (candidateDistSq < bestDistSq)
					{
						bestCandidate = candidate;
						bestUnderCap = candidateUnderCap;
						bestIgnite = candidateIgnite;
						bestDroneCount = candidateDrones;
						bestDistSq = candidateDistSq;
					}
					continue;
				}

				// 5. stable identity
				if (candidate.get() < bestCandidate.get())
				{
					bestCandidate = candidate;
					bestUnderCap = candidateUnderCap;
					bestIgnite = candidateIgnite;
					bestDroneCount = candidateDrones;
					bestDistSq = candidateDistSq;
				}
			}

			if (bestCandidate)
			{
				plan.drone->SetTarget(bestCandidate);
				assignedDroneCounts[bestCandidate.get()]++;
			}
			else
			{
				plan.drone->ClearTarget();
			}
		}
	}

	void EmberSwarmAbility::EmitEvent(
		GameAbilityBehaviorContext& context,
		const GameplayTag& eventTag
	) const
	{
		sas::AbilityEvent event;
		event.eventTag = eventTag;
		event.sourceAbilityId = sas::ContentId{ context.definition.abilityId };
		event.sourceAbilityTags = context.definition.abilityTags;
		event.SetSource(&context.owner);
		event.SetTarget(&context.owner);
		context.abilitySystem.HandleGameplayEvent(event);
	}

	void EmberSwarmAbility::DestroyDrones()
	{
		for (const weak_ptr<EmberDroneActor>& droneWeak : mDrones)
		{
			if (const shared_ptr<EmberDroneActor> drone = droneWeak.lock())
			{
				drone->Destroy();
			}
		}
		mDrones.clear();
	}
}
