#include "gameConfigs/CollisionLayers.h"
#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/weapon/PrimaryWeaponHandler.h"

#include "framework/Actor.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/movement/MovementCollisionService.h"
#include "gameplay/weapon/visuals/ConeBlastVisualActor.h"

#include <algorithm>
#include <cmath>

namespace vega
{
	namespace
	{
		constexpr float RadiansToDegrees = 180.f / 3.14159265f;

		// An instant blast: every valid target whose bounds reach into the cone
		// takes exactly one hit, with no falloff by target count. Walls block
		// targets behind them, as they do for the continuous beam.
		class ConeBlastWeaponHandler final : public PrimaryWeaponHandler
		{
		public:
			PrimaryWeaponType GetType() const override
			{
				return PrimaryWeaponType::ConeBlast;
			}

			bool FireOnce(
				const PrimaryWeaponExecutionContext& context,
				PrimaryWeaponTypeRuntimeState&
			) const override
			{
				World* world = context.owner.GetWorld();
				if (!world)
				{
					return false;
				}

				vega::CollisionLayer expectedTargetLayer = vega::CollisionLayer::None;
				vega::CollisionLayer virtualProjectileLayer = vega::CollisionLayer::None;
				switch (context.owner.GetCollisionLayer())
				{
				case GameCollision::Player:
					expectedTargetLayer = GameCollision::Enemy;
					virtualProjectileLayer = GameCollision::PlayerBullet;
					break;
				case GameCollision::Enemy:
					expectedTargetLayer = GameCollision::Player | GameCollision::FriendlySummon;
					virtualProjectileLayer = GameCollision::EnemyBullet;
					break;
				default:
					break;
				}

				const WeaponMuzzleDefinition muzzle = context.definition.muzzleDefinitions.empty()
					? WeaponMuzzleDefinition{}
					: context.definition.muzzleDefinitions.front();
				const sf::Vector2f origin = context.owner.GetActorLocation() +
					context.owner.TransformLocalToWorld(muzzle.offset);
				const sf::Vector2f forward = RotationToVector(
					context.owner.GetActorRotation() + muzzle.rotationOffset - 90.f);
				const float range = std::max(0.f, sas::FindAttributeValue(
					context.attributes, CommonAttributeIds::Range, 0.f));
				const float halfAngle = std::clamp(sas::FindAttributeValue(
					context.attributes, PrimaryWeaponSchema::Cone::HalfAngleDegrees, 0.f), 0.f, 90.f);

				world->SpawnActor<ConeBlastVisualActor>(
					origin, forward, range, halfAngle,
					context.definition.presentationDefinition.pointLightDef.color);
				if (range <= 0.f || halfAngle <= 0.f || expectedTargetLayer == vega::CollisionLayer::None)
				{
					return true;
				}

				List<GameplayTag> damageTags = context.damageTags;
				// Ignite is a Thermal status: an empowered blast that carries Ignite
				// stacks also counts as Thermal so the stacks reach the target.
				if (context.shotMetadata.isEmpowered &&
					sas::FindAttributeValue(context.attributes, DamageAttributeIds::IgniteStacks, 0.f) > 0.f &&
					std::find(damageTags.begin(), damageTags.end(), DamageTypeSchema::Thermal) == damageTags.end())
				{
					damageTags.push_back(DamageTypeSchema::Thermal);
				}
				DamagePayload payload = DamageTypeSystem::BuildPayload(damageTags, context.attributes);
				ApplyShotMetadataToPayload(payload, context.shotMetadata);
				const float damage = ResolvePrimaryWeaponShotDamage(context);

				const sf::FloatRect searchBounds{ origin - sf::Vector2f{ range, range }, { range * 2.f, range * 2.f } };
				bool anyHit = false;
				for (const weak_ptr<Actor>& targetWeak : world->GetActorsInBounds(searchBounds))
				{
					if (!context.ShouldContinue()) return true;
					const shared_ptr<Actor> target = targetWeak.lock();
					if (!target || target.get() == &context.owner || target->GetIsPendingDestroy() ||
						!HasCollisionLayer(expectedTargetLayer, target->GetCollisionLayer()) ||
						!HasCollisionLayer(target->GetCollisionMask(), virtualProjectileLayer))
					{
						continue;
					}
					if (!IsInsideCone(*target, origin, forward, range, halfAngle))
					{
						continue;
					}
					const sf::Vector2f targetCenter = target->GetActorLocation();
					movement::StaticGeometrySweepHit wallHit;
					if (movement::FindFirstStaticGeometryHit(context.owner, origin, targetCenter, 0.f, wallHit, false))
					{
						continue;
					}
					ApplyCombatDamage(*target, damage, &context.owner, damageTags, payload);
					anyHit = true;
				}
				if (anyHit)
				{
					context.shotMetadata.ReportHit();
				}
				return true;
			}

		private:
			// A target counts when any part of a circle around its bounds lies
			// inside the cone and within range.
			static bool IsInsideCone(
				const Actor& target,
				const sf::Vector2f& origin,
				const sf::Vector2f& forward,
				float range,
				float halfAngleDegrees
			)
			{
				const sf::FloatRect bounds = target.GetActorGlobalBounds();
				float radius = 0.5f * std::max(bounds.size.x, bounds.size.y);
				if (radius <= 0.f)
				{
					radius = std::max(0.f, target.GetPhysicsCollisionRadius());
				}
				const sf::Vector2f toTarget = target.GetActorLocation() - origin;
				const float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);
				if (distance <= radius)
				{
					return true;
				}
				if (distance - radius > range)
				{
					return false;
				}
				const float cosine = std::clamp(
					(toTarget.x * forward.x + toTarget.y * forward.y) / distance, -1.f, 1.f);
				const float offAxisDegrees = std::acos(cosine) * RadiansToDegrees;
				const float targetHalfWidthDegrees = std::asin(std::min(1.f, radius / distance)) * RadiansToDegrees;
				return offAxisDegrees - targetHalfWidthDegrees <= halfAngleDegrees;
			}
		};
	}

	namespace PrimaryWeaponBuiltIns
	{
		unique_ptr<PrimaryWeaponHandler> CreateConeBlastWeaponHandler()
		{
			return std::make_unique<ConeBlastWeaponHandler>();
		}
	}
}
