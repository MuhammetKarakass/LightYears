#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "../internal/PrimaryWeaponBuiltIns.h"

#include "framework/Actor.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/weapon/visuals/ContinuousBeamVisualActor.h"

#include <algorithm>
#include <cmath>

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

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return PrimaryWeaponBuiltIns::BeamDeliveryAttributeRoots();
			}

			PrimaryWeaponValidationResult ValidateDefinition(
				const PrimaryWeaponDefinition& definition
			) const override
			{
				for (const sas::AttributeId& required : {
					CommonAttributeIds::Damage,
					PrimaryWeaponSchema::Beam::Delivery::Range,
					PrimaryWeaponSchema::Beam::Delivery::Width
				})
				{
					const PrimaryWeaponValidationResult result =
						PrimaryWeaponBuiltIns::RequireAttribute(
							definition,
							required,
							"Continuous beam weapon"
						);
					if (!result.isValid)
					{
						return result;
					}
				}

				const float range = PrimaryWeaponBuiltIns::FindDefinitionAttribute(
					definition,
					PrimaryWeaponSchema::Beam::Delivery::Range
				)->baseValue;
				const float width = PrimaryWeaponBuiltIns::FindDefinitionAttribute(
					definition,
					PrimaryWeaponSchema::Beam::Delivery::Width
				)->baseValue;
				return range > 0.f && width > 0.f
					? PrimaryWeaponValidationResult{ true, {} }
					: PrimaryWeaponValidationResult{
						false,
						"Continuous beam range and width must be greater than zero."
					};
			}

			unique_ptr<PrimaryWeaponTypeRuntimeState> CreateRuntimeState() const override
			{
				return std::make_unique<ContinuousBeamWeaponRuntimeState>();
			}

			bool UsesIntervalFire() const override
			{
				return false;
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
					spawnBeam(muzzle);
				}
			}

			void FireOnce(
				const PrimaryWeaponExecutionContext&,
				PrimaryWeaponTypeRuntimeState&
			) const override
			{
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
				size_t beamIndex = 0;
				const auto tickBeam = [&](const WeaponMuzzleDefinition& muzzle)
				{
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
					beam->UpdateBeam(
						start,
						directionRotation,
						range,
						width,
						heatRatio,
						context.definition.presentationDefinition.pointLightDef.color
					);

					for (const weak_ptr<Actor>& targetWeak :
						context.owner.GetWorld()->GetActorsByType<Actor>())
					{
						const shared_ptr<Actor> target = targetWeak.lock();
						if (!target || !beam->IsValidDamageTarget(target.get()))
						{
							continue;
						}

						const sf::Vector2f toTarget = target->GetActorLocation() - start;
						const float forwardDistance =
							toTarget.x * direction.x + toTarget.y * direction.y;
						const float lateralDistance =
							std::abs(toTarget.x * direction.y - toTarget.y * direction.x);
						if (forwardDistance >= 0.f && forwardDistance <= range &&
							lateralDistance <= width * 0.5f)
						{
							ApplyCombatDamage(
								*target,
								damage,
								&context.owner,
								context.damageTags,
								payload
							);
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
					tickBeam(muzzle);
				}
			}

			void EndFire(
				const PrimaryWeaponExecutionContext&,
				PrimaryWeaponTypeRuntimeState& state
			) const override
			{
				auto& beamState = static_cast<ContinuousBeamWeaponRuntimeState&>(state);
				for (const weak_ptr<ContinuousBeamVisualActor>& beamWeak : beamState.beams)
				{
					if (const shared_ptr<ContinuousBeamVisualActor> beam = beamWeak.lock())
					{
						beam->Destroy();
					}
				}
				beamState.beams.clear();
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
