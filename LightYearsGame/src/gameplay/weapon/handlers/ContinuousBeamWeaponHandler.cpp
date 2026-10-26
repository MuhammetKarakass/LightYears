#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/weapon/PrimaryWeaponHandler.h"

#include "framework/Actor.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameConfigs/combat/CombatTick.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/module/rules/legendary/LegendaryMeltdownBarrelRuntime.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/movement/MovementCollisionService.h"
#include "gameplay/weapon/visuals/ContinuousBeamVisualActor.h"
#include "gameplay/targeting/SweptGeometry.h"
#include "gameplay/time/PeriodicTickAccumulator.h"

#include <algorithm>
#include <cmath>
#include <exception>

namespace vega
{
	namespace
	{
		// Beam damage is integrated every frame (DPS * heat multiplier * dt) per target
		// but delivered once per global combat tick, so a beam hits on the same cadence
		// as every other periodic damage source without changing total damage.
		struct PendingBeamHit
		{
			unsigned int targetId{ 0 };
			weak_ptr<Actor> target;
			float damage{ 0.f };
		};

		struct ContinuousBeamWeaponRuntimeState final : PrimaryWeaponTypeRuntimeState
		{
			List<weak_ptr<ContinuousBeamVisualActor>> beams;
			List<PendingBeamHit> pendingHits;
			// Damage accumulated between combat ticks belongs to the last 0.25s step.
			shared_ptr<DamageHitGroup> deliveryHitGroup;
			float tickAccumulator{ 0.f };
			unsigned int splitPulseCount = 0;
			// Tags of the current heat region; pending damage is delivered with them.
			List<GameplayTag> damageTags;
		};

		struct BeamHeatDamage
		{
			float multiplier = 1.f;
			List<GameplayTag> damageTags;
		};

		// Linear DamageMultiplierAtMaxHeat model, or the discrete heat region the
		// current heat ratio falls into.
		BeamHeatDamage ResolveBeamHeatDamage(const PrimaryWeaponExecutionContext& context, float heatRatio)
		{
			BeamHeatDamage result{ 1.f, context.damageTags };
			const List<HeatDamageRegionDefinition>& regions = context.definition.heatDamageRegions;
			if (regions.empty())
			{
				const float maximumDamageMultiplier = std::max(1.f, sas::FindAttributeValue(
					context.attributes, PrimaryWeaponSchema::Feature::Heat::DamageMultiplierAtMaxHeat, 1.f));
				result.multiplier = 1.f + (maximumDamageMultiplier - 1.f) * heatRatio;
				return result;
			}
			const float heatPercentage = heatRatio * 100.f;
			const HeatDamageRegionDefinition* region = &regions.back();
			for (const HeatDamageRegionDefinition& candidate : regions)
			{
				if (heatPercentage < candidate.upToHeatPercentage)
				{
					region = &candidate;
					break;
				}
			}
			result.multiplier = region->damageMultiplier;
			if (!region->damageTags.empty())
			{
				result.damageTags = region->damageTags;
			}
			return result;
		}

		void AddPendingBeamDamage(
			ContinuousBeamWeaponRuntimeState& beamState,
			const shared_ptr<Actor>& target,
			float damage
		)
		{
			for (PendingBeamHit& hit : beamState.pendingHits)
			{
				if (hit.targetId == target->GetUniqueID())
				{
					hit.damage += damage;
					return;
				}
			}
			beamState.pendingHits.push_back(PendingBeamHit{ target->GetUniqueID(), target, damage });
		}

		void DeliverPendingBeamDamage(const PrimaryWeaponExecutionContext& context, ContinuousBeamWeaponRuntimeState& beamState, const DamagePayload& payload, bool newHitGroup = true)
		{
			auto deliveryPayload = payload;
			deliveryPayload.primaryWeaponId = sas::ContentId{ context.definition.weaponId };
			if (newHitGroup || !beamState.deliveryHitGroup)
				beamState.deliveryHitGroup = std::make_shared<DamageHitGroup>(DamageHitGroup::Kind::PrimaryShot);
			deliveryPayload.hitGroup = beamState.deliveryHitGroup;
			const List<GameplayTag> damageTags = beamState.damageTags.empty()
				? context.damageTags
				: beamState.damageTags;
			List<PendingBeamHit> hits;
			hits.swap(beamState.pendingHits);
			for (const PendingBeamHit& hit : hits)
			{
				if (!context.ShouldContinue()) return;
				const shared_ptr<Actor> target = hit.target.lock();
				if (!target || target->GetIsPendingDestroy() || hit.damage <= 0.f)
				{
					continue;
				}
				ApplyCombatDamage(
					*target,
					hit.damage,
					&context.owner,
					damageTags,
					deliveryPayload
				);
			}
		}

		class ContinuousBeamWeaponHandler final : public PrimaryWeaponHandler
		{
		public:
			PrimaryWeaponType GetType() const override
			{
				return PrimaryWeaponType::BeamContinuous;
			}

			unique_ptr<PrimaryWeaponTypeRuntimeState> CreateRuntimeState() const override
			{
				return std::make_unique<ContinuousBeamWeaponRuntimeState>();
			}

			void BeginFire(const PrimaryWeaponExecutionContext& context, PrimaryWeaponTypeRuntimeState& state) const override
			{
				World* world = context.owner.GetWorld();
				if (!world)
				{
					return;
				}

				auto& beamState = static_cast<ContinuousBeamWeaponRuntimeState&>(state);
				for (const auto& emission : ResolvePrimaryWeaponEmissions(context))
				{
					if (!context.ShouldContinue()) return;
					beamState.beams.push_back(world->SpawnActor<ContinuousBeamVisualActor>(&context.owner));
				}
			}

			bool FireOnce(const PrimaryWeaponExecutionContext&, PrimaryWeaponTypeRuntimeState&) const override
			{
				return false;
			}

			void TickFire(const PrimaryWeaponExecutionContext& context, PrimaryWeaponTypeRuntimeState& state, float deltaTime) const override
			{
				if (deltaTime <= 0.f || !context.owner.GetWorld())
				{
					return;
				}

				const float range = std::max(0.f, sas::FindAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Beam::Delivery::Range,
					0.f
				));
				const float width = std::max(0.f, sas::FindAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Beam::Delivery::Width,
					0.f
				));
				const float baseDamagePerSecond = std::max(0.f, sas::FindAttributeValue(
					context.attributes,
					CommonAttributeIds::Damage,
					0.f
				));
				if (range <= 0.f || width <= 0.f || baseDamagePerSecond <= 0.f)
				{
					return;
				}

				const float heatCapacity = std::max(0.f, sas::FindAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Feature::Heat::Capacity,
					0.f
				));
				const float heat = context.runtime
					? context.runtime->GetFeatureValue(
						PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
					)
					: 0.f;
				const float heatRatio = heatCapacity > 0.f
					? std::clamp(heat / heatCapacity, 0.f, 1.f)
					: 0.f;
				const BeamHeatDamage heatDamage = ResolveBeamHeatDamage(context, heatRatio);
				const float damage = baseDamagePerSecond * heatDamage.multiplier * deltaTime;
				if (const auto* combatant = dynamic_cast<const Combatant*>(&context.owner))
					if (auto* meltdown = combatant->GetAbilitySystemComponent().GetMeltdownBarrelRuntime()) meltdown->RecordBeam(context, heatDamage.multiplier, deltaTime);
				const DamagePayload payload =
					DamageTypeSystem::BuildPayload(heatDamage.damageTags, context.attributes);
				auto& beamState = static_cast<ContinuousBeamWeaponRuntimeState&>(state);
				beamState.damageTags = heatDamage.damageTags;
				if (!context.mountProfile.splitFire) beamState.splitPulseCount = 0;
				if (context.runtime) context.runtime->splitFireShot = context.mountProfile.splitFire && (beamState.splitPulseCount + 1) % 4 == 0;
				const auto emissions = ResolvePrimaryWeaponEmissions(context);
				while (beamState.beams.size() > emissions.size())
				{
					if (const auto beam = beamState.beams.back().lock()) beam->Destroy();
					beamState.beams.pop_back();
				}
				while (beamState.beams.size() < emissions.size()) beamState.beams.push_back(context.owner.GetWorld()->SpawnActor<ContinuousBeamVisualActor>(&context.owner));
				Set<Actor*> damagedTargets[2];
				size_t beamIndex = 0;
				const auto tickBeam = [&](const PrimaryWeaponEmission& emission)
				{
					if (!context.ShouldContinue()) return;
					if (beamIndex >= beamState.beams.size())
					{
						return;
					}
					const shared_ptr<ContinuousBeamVisualActor> beam =
						beamState.beams[beamIndex++].lock();
					if (!beam || beam->GetIsPendingDestroy())
					{
						return;
					}

					const sf::Vector2f start = emission.origin;
					const float directionRotation = emission.rotation - 90.f;
					const sf::Vector2f direction = RotationToVector(directionRotation);
					const sf::Vector2f fullEnd = start + direction * range;
					movement::StaticGeometrySweepHit staticHit;
					const bool blockedByStaticGeometry = movement::FindFirstStaticGeometryHit(
						context.owner,
						start,
						fullEnd,
						width * 0.5f,
						staticHit,
						false
					);
					// The beam does not pierce: it stops at whichever comes first, static
					// geometry or the nearest valid target along its direction.
					float blockingFraction = blockedByStaticGeometry ? staticHit.fraction : 1.f;
					shared_ptr<Actor> firstTarget;
					float firstTargetFraction = 1.f;

					// The physics broadphase works in scaled, rounded coordinates, so a
					// target touching the beam's edge can fall just outside it. Widen the
					// candidate query slightly; SegmentIntersectsExpandedBounds below
					// remains the exact hit rule.
					constexpr float CandidateQueryMargin = 1.f;
					for (const weak_ptr<Actor>& targetWeak :
						context.owner.GetWorld()->GetActorsInBounds(
							targeting::swept::SegmentBounds(start, fullEnd, width * 0.5f + CandidateQueryMargin)
						))
					{
						if (!context.ShouldContinue()) return;
						const shared_ptr<Actor> target = targetWeak.lock();
						if (!target || !beam->IsValidDamageTarget(target.get()))
						{
							continue;
						}

						float targetHitFraction = 0.f;
						if (targeting::swept::SegmentIntersectsExpandedBounds(
							start,
							fullEnd,
							target->GetActorGlobalBounds(),
							width * 0.5f,
							targetHitFraction
						) && targetHitFraction + 0.0001f < blockingFraction)
						{
							blockingFraction = targetHitFraction;
							firstTarget = target;
							firstTargetFraction = targetHitFraction;
						}
					}

					float visibleFraction = blockingFraction;
					if (firstTarget)
					{
						// The edge of the beam touches first; draw the core up to the
						// target's bounds when the centre line reaches them.
						float centreFraction = 0.f;
						if (targeting::swept::SegmentIntersectsExpandedBounds(
							start,
							fullEnd,
							firstTarget->GetActorGlobalBounds(),
							0.f,
							centreFraction
						))
						{
							visibleFraction = std::max(firstTargetFraction, centreFraction);
							if (blockedByStaticGeometry)
							{
								visibleFraction = std::min(visibleFraction, staticHit.fraction);
							}
						}
					}
					beam->UpdateBeam(
						start,
						directionRotation,
						range * visibleFraction,
						width,
						heatRatio,
						context.definition.presentationDefinition.pointLightDef.color
					);

					// Converging muzzles still deal one beam per side; Broadside has two independent sides.
					const std::size_t side = context.mountProfile.broadside ? (beamIndex - 1) % 2 : 0;
					if (firstTarget && damagedTargets[side].insert(firstTarget.get()).second)
					{
						AddPendingBeamDamage(beamState, firstTarget, damage * emission.damageMultiplier);
					}
				};

				for (const auto& emission : emissions)
				{
					if (!context.ShouldContinue()) return;
					tickBeam(emission);
				}

				if (time::ConsumePeriodicTicks(
					beamState.tickAccumulator,
					deltaTime,
					CombatTick::Interval) > 0)
				{
					// Pending damage is delivered in one hit, so drop any tick debt.
					beamState.tickAccumulator = std::fmod(beamState.tickAccumulator, CombatTick::Interval);
					DeliverPendingBeamDamage(context, beamState, payload);
					if (context.mountProfile.splitFire) beamState.splitPulseCount = (beamState.splitPulseCount + 1) % 4;
				}
			}

			void EndFire(const PrimaryWeaponExecutionContext& context, PrimaryWeaponTypeRuntimeState& state) const override
			{
				auto& beamState = static_cast<ContinuousBeamWeaponRuntimeState&>(state);
				std::exception_ptr error;
				// Damage earned since the last tick must not be lost when firing stops.
				try
				{
					if (!beamState.pendingHits.empty() && context.owner.GetWorld())
					{
						DeliverPendingBeamDamage(
							context,
							beamState,
							DamageTypeSystem::BuildPayload(
								beamState.damageTags.empty() ? context.damageTags : beamState.damageTags,
								context.attributes),
							false
						);
					}
				}
				catch (...) { error = std::current_exception(); }
				beamState.pendingHits.clear();
				beamState.deliveryHitGroup.reset();
				beamState.tickAccumulator = 0.f;
				for (auto beam = beamState.beams.begin(); beam != beamState.beams.end();)
				{
					const shared_ptr<ContinuousBeamVisualActor> lockedBeam = beam->lock();
					if (!lockedBeam)
					{
						beam = beamState.beams.erase(beam);
						continue;
					}
					try { lockedBeam->Destroy(); }
					catch (...) { if (!error) error = std::current_exception(); }
					if (!lockedBeam->GetIsPendingDestroy())
					{
						++beam;
						continue;
					}
					beam = beamState.beams.erase(beam);
				}
				if (error) std::rethrow_exception(error);
			}
		};
	}

	namespace PrimaryWeaponBuiltIns
	{
		unique_ptr<PrimaryWeaponHandler> CreateContinuousBeamWeaponHandler()
		{
			return std::make_unique<ContinuousBeamWeaponHandler>();
		}
	}
}
