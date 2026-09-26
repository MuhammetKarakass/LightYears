#include "gameplay/portal/PortalTransferService.h"

#include "framework/Actor.h"
#include "framework/World.h"
#include "gameplay/portal/PortalTransferParticipant.h"
#include "gameplay/portal/PortalTransferRuntimeActor.h"
#include "gameplay/targeting/SweptGeometry.h"
#include "PortalTransferRuntimeState.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace ly
{
	namespace
	{
		using Endpoint = PortalTransferRuntimeState::Endpoint;
		using Transit = PortalTransferRuntimeState::Transit;
		using ReentryLock = PortalTransferRuntimeState::ReentryLock;
		using Pair = PortalTransferRuntimeState::Pair;

		weak_ptr<Actor> MakeWeakActor(Actor& actor)
		{
			const shared_ptr<Object> object = actor.GetWeakPtr().lock();
			return object
				? std::dynamic_pointer_cast<Actor>(object)
				: weak_ptr<Actor>{};
		}

		sf::Vector2f ResolveEndpointLocation(const Endpoint& endpoint)
		{
			if (const shared_ptr<Actor> actor = endpoint.actor.lock())
			{
				return actor->GetActorLocation();
			}
			return endpoint.location;
		}

		bool IsInsidePortal(
			const PortalTransferParticipant& participant,
			const sf::Vector2f& participantLocation,
			const sf::Vector2f& portalLocation,
			float portalRadius
		)
		{
			const sf::Vector2f delta = participantLocation - portalLocation;
			const float effectiveRadius = std::max(
				1.f,
				portalRadius + std::max(0.f, participant.GetPortalTransferRadius())
			);
			return delta.x * delta.x + delta.y * delta.y <=
				effectiveRadius * effectiveRadius;
		}

		bool IsLocked(const Pair& pair, unsigned int actorId)
		{
			return pair.reentryLocks.find(actorId) != pair.reentryLocks.end();
		}

		void UpdateEndpointLocations(Pair& pair)
		{
			pair.first.location = ResolveEndpointLocation(pair.first);
			pair.second.location = ResolveEndpointLocation(pair.second);
		}

		void CompleteTransit(Pair& pair, Transit& transit)
		{
			const shared_ptr<Actor> actor = transit.actor.lock();
			if (!actor)
			{
				return;
			}

			PortalTransferParticipant* participant =
				dynamic_cast<PortalTransferParticipant*>(actor.get());
			if (!participant)
			{
				return;
			}

			const Endpoint& exit = transit.enteredFirst
				? pair.second
				: pair.first;
			participant->CompletePortalTransit(exit.location);
			pair.reentryLocks[actor->GetUniqueID()] = ReentryLock{
				MakeWeakActor(*actor),
				true,
				0.f
			};
		}

		void UpdateReentryLocks(Pair& pair, float deltaTime)
		{
			for (auto lock = pair.reentryLocks.begin();
				lock != pair.reentryLocks.end();)
			{
				const shared_ptr<Actor> actor = lock->second.actor.lock();
				PortalTransferParticipant* participant = actor
					? dynamic_cast<PortalTransferParticipant*>(actor.get())
					: nullptr;
				if (!actor || !participant || actor->GetIsPendingDestroy())
				{
					lock = pair.reentryLocks.erase(lock);
					continue;
				}

				ReentryLock& value = lock->second;
				if (value.waitingForExit)
				{
					const sf::Vector2f location = actor->GetActorLocation();
					const bool insideEitherPortal = IsInsidePortal(
						*participant,
						location,
						pair.first.location,
						pair.radius
					) || IsInsidePortal(
						*participant,
						location,
						pair.second.location,
						pair.radius
					);
					if (insideEitherPortal)
					{
						++lock;
						continue;
					}

					value.waitingForExit = false;
					value.cooldownRemaining = pair.reentryCooldown;
					// The clock starts at the exact exit frame. Do not consume this
					// frame's delta time, otherwise a one-second cooldown is shorter
					// at low frame rates.
					if (value.cooldownRemaining <= 0.f)
					{
						lock = pair.reentryLocks.erase(lock);
					}
					else
					{
						++lock;
					}
					continue;
				}

				value.cooldownRemaining = std::max(
					0.f,
					value.cooldownRemaining - std::max(0.f, deltaTime)
				);
				if (value.cooldownRemaining <= 0.f)
				{
					lock = pair.reentryLocks.erase(lock);
				}
				else
				{
					++lock;
				}
			}
		}

		void ProcessPair(World& world, Pair& pair, float deltaTime)
		{
			UpdateEndpointLocations(pair);

			UpdateReentryLocks(pair, deltaTime);

			for (auto transit = pair.transits.begin();
				transit != pair.transits.end();)
			{
				transit->remaining -= std::max(0.f, deltaTime);
				if (transit->remaining <= 0.f)
				{
					CompleteTransit(pair, *transit);
					transit = pair.transits.erase(transit);
				}
				else
				{
					++transit;
				}
			}

			if (!pair.acceptingEntries)
			{
				return;
			}

			const float queryRadius = pair.radius;
			const sf::FloatRect firstBounds = targeting::swept::RadiusBounds(
				pair.first.location,
				queryRadius
			);
			const sf::FloatRect secondBounds = targeting::swept::RadiusBounds(
				pair.second.location,
				queryRadius
			);
			List<weak_ptr<Actor>> candidates = world.GetActorsInBounds(firstBounds);
			const List<weak_ptr<Actor>> secondCandidates = world.GetActorsInBounds(secondBounds);
			candidates.insert(
				candidates.end(),
				secondCandidates.begin(),
				secondCandidates.end()
			);
			Set<Actor*> seenCandidates;
			for (const weak_ptr<Actor>& actorWeak : candidates)
			{
				const shared_ptr<Actor> actor = actorWeak.lock();
				if (!actor || actor->GetIsPendingDestroy() ||
					!seenCandidates.insert(actor.get()).second)
				{
					continue;
				}

				PortalTransferParticipant* participant =
					dynamic_cast<PortalTransferParticipant*>(actor.get());
				if (!participant || !participant->CanEnterPortalTransfer() ||
					participant->IsInPortalTransit() ||
					IsLocked(pair, actor->GetUniqueID()))
				{
					continue;
				}

				const sf::Vector2f location = actor->GetActorLocation();
				bool enteredFirst = IsInsidePortal(
					*participant,
					location,
					pair.first.location,
					pair.radius
				);
				if (!enteredFirst && !IsInsidePortal(
					*participant,
					location,
					pair.second.location,
					pair.radius
				))
				{
					continue;
				}

				participant->BeginPortalTransit();
				pair.transits.push_back(Transit{
					MakeWeakActor(*actor),
					enteredFirst,
					pair.transferDuration
				});
			}
		}
	}

	PortalTransferService::PairId PortalTransferService::CreatePair(
		World& world,
		Actor& firstPortal,
		Actor& secondPortal,
		float portalRadius,
		float transferDuration,
		float reentryCooldown
	)
	{
		const shared_ptr<PortalTransferRuntimeActor> runtimeActor =
			FindOrCreateRuntimeActor(world);
		if (!runtimeActor || !runtimeActor->mRuntimeState)
		{
			return 0;
		}

		PortalTransferRuntimeState& state = *runtimeActor->mRuntimeState;
		const PairId pairId = state.nextPairId++;
		state.pairs.push_back(Pair{
			pairId,
			Endpoint{ MakeWeakActor(firstPortal), firstPortal.GetActorLocation() },
			Endpoint{ MakeWeakActor(secondPortal), secondPortal.GetActorLocation() },
			std::max(1.f, portalRadius),
			std::max(0.01f, transferDuration),
			std::max(0.f, reentryCooldown),
			true,
			{},
			{}
		});
		return pairId;
	}

	void PortalTransferService::ClosePair(World& world, PairId pairId)
	{
		const shared_ptr<PortalTransferRuntimeActor> runtimeActor = FindRuntimeActor(world);
		if (!runtimeActor || !runtimeActor->mRuntimeState)
		{
			return;
		}

		PortalTransferRuntimeState& state = *runtimeActor->mRuntimeState;
		for (Pair& pair : state.pairs)
		{
			if (pair.id == pairId)
			{
				UpdateEndpointLocations(pair);
				pair.acceptingEntries = false;
				return;
			}
		}
	}

	void PortalTransferService::Tick(World& world, float deltaTime)
	{
		const shared_ptr<PortalTransferRuntimeActor> runtimeActor = FindRuntimeActor(world);
		if (!runtimeActor || !runtimeActor->mRuntimeState)
		{
			return;
		}

		PortalTransferRuntimeState& state = *runtimeActor->mRuntimeState;
		for (Pair& pair : state.pairs)
		{
			ProcessPair(world, pair, deltaTime);
		}

		state.pairs.erase(
			std::remove_if(
				state.pairs.begin(),
				state.pairs.end(),
				[](const Pair& pair)
				{
					return !pair.acceptingEntries && pair.transits.empty();
				}
			),
			state.pairs.end()
		);
	}

	void PortalTransferService::EnsureRuntimeActor(World& world)
	{
		(void)FindOrCreateRuntimeActor(world);
	}

	void PortalTransferService::ResetWorld(World& world)
	{
		if (const shared_ptr<PortalTransferRuntimeActor> runtimeActor = FindRuntimeActor(world))
		{
			runtimeActor->mRuntimeState =
				std::make_unique<PortalTransferRuntimeState>();
		}
	}

	shared_ptr<PortalTransferRuntimeActor> PortalTransferService::FindRuntimeActor(World& world)
	{
		for (const weak_ptr<PortalTransferRuntimeActor>& runtimeActorWeak :
			world.GetActorsByTypeIncludingPending<PortalTransferRuntimeActor>())
		{
			const shared_ptr<PortalTransferRuntimeActor> runtimeActor = runtimeActorWeak.lock();
			if (runtimeActor && !runtimeActor->GetIsPendingDestroy() && runtimeActor->mRuntimeState)
			{
				return runtimeActor;
			}
		}
		return {};
	}

	shared_ptr<PortalTransferRuntimeActor> PortalTransferService::FindOrCreateRuntimeActor(World& world)
	{
		if (const shared_ptr<PortalTransferRuntimeActor> runtimeActor = FindRuntimeActor(world))
		{
			return runtimeActor;
		}
		return world.SpawnActor<PortalTransferRuntimeActor>().lock();
	}
}
