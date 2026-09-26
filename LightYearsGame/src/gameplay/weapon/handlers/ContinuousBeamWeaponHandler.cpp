#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/weapon/PrimaryWeaponHandler.h"

#include "framework/Actor.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/movement/MovementCollisionService.h"
#include "gameplay/weapon/visuals/ContinuousBeamVisualActor.h"
#include "gameplay/targeting/SweptGeometry.h"

#include <algorithm>
#include <cmath>
#include <exception>

namespace ly
{
	namespace
	{
		struct ContinuousBeamWeaponRuntimeState final : PrimaryWeaponTypeRuntimeState
		{
			List<weak_ptr<ContinuousBeamVisualActor>> beams;
		};

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

			void BeginFire(
				const PrimaryWeaponExecutionContext& context,
				PrimaryWeaponTypeRuntimeState& state
			) const override
			{
				World* world = context.owner.GetWorld();
				if (!world)
				{
					return;
				}

				auto& beamState = static_cast<ContinuousBeamWeaponRuntimeState&>(state);
				const auto spawnBeam = [&](const WeaponMuzzleDefinition&)
				{
					if (!context.ShouldContinue()) return;
					beamState.beams.push_back(
						world->SpawnActor<ContinuousBeamVisualActor>(&context.owner)
					);
				};
				if (context.definition.muzzleDefinitions.empty())
				{
					spawnBeam(WeaponMuzzleDefinition{});
					return;
				}
				for (const WeaponMuzzleDefinition& muzzle : context.definition.muzzleDefinitions)
				{
					if (!context.ShouldContinue()) return;
					spawnBeam(muzzle);
				}
			}

			bool FireOnce(
				const PrimaryWeaponExecutionContext&,
				PrimaryWeaponTypeRuntimeState&
			) const override
			{
				return false;
			}

			void TickFire(
				const PrimaryWeaponExecutionContext& context,
				PrimaryWeaponTypeRuntimeState& state,
				float deltaTime
			) const override
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
				const float maximumDamageMultiplier = std::max(
					1.f,
					sas::FindAttributeValue(
						context.attributes,
						PrimaryWeaponSchema::Feature::Heat::DamageMultiplierAtMaxHeat,
						1.f
					)
				);
				const float damage = baseDamagePerSecond *
					(1.f + (maximumDamageMultiplier - 1.f) * heatRatio) * deltaTime;
				const DamagePayload payload =
					DamageTypeSystem::BuildPayload(context.damageTags, context.attributes);
				auto& beamState = static_cast<ContinuousBeamWeaponRuntimeState&>(state);
				Set<Actor*> damagedTargets;
				size_t beamIndex = 0;
				const auto tickBeam = [&](const WeaponMuzzleDefinition& muzzle)
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

					const sf::Vector2f start = context.owner.GetActorLocation() +
						context.owner.TransformLocalToWorld(muzzle.offset);
					const float directionRotation =
						context.owner.GetActorRotation() + muzzle.rotationOffset - 90.f;
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
					const float effectiveRange = blockedByStaticGeometry
						? range * staticHit.fraction
						: range;
					beam->UpdateBeam(
						start,
						directionRotation,
						effectiveRange,
						width,
						heatRatio,
						context.definition.presentationDefinition.pointLightDef.color
					);

					for (const weak_ptr<Actor>& targetWeak :
						context.owner.GetWorld()->GetActorsInBounds(
							targeting::swept::SegmentBounds(start, fullEnd, width * 0.5f)
						))
					{
						if (!context.ShouldContinue()) return;
						const shared_ptr<Actor> target = targetWeak.lock();
						if (!target ||
							damagedTargets.find(target.get()) != damagedTargets.end() ||
							!beam->IsValidDamageTarget(target.get()))
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
						) && (!blockedByStaticGeometry ||
							targetHitFraction + 0.0001f < staticHit.fraction))
						{
							damagedTargets.insert(target.get());
							ApplyCombatDamage(
								*target,
								damage,
								&context.owner,
								context.damageTags,
								payload
							);
							if (!context.ShouldContinue()) return;
						}
					}
				};

				if (context.definition.muzzleDefinitions.empty())
				{
					tickBeam(WeaponMuzzleDefinition{});
					return;
				}
				for (const WeaponMuzzleDefinition& muzzle : context.definition.muzzleDefinitions)
				{
					if (!context.ShouldContinue()) return;
					tickBeam(muzzle);
				}
			}

			void EndFire(
				const PrimaryWeaponExecutionContext&,
				PrimaryWeaponTypeRuntimeState& state
			) const override
			{
				auto& beamState = static_cast<ContinuousBeamWeaponRuntimeState&>(state);
				std::exception_ptr error;
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
