#include "gameplay/ability/lanceDrive/LanceDriveActor.h"

#include "attributes/AttributeSystem.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameplay/ability/lanceDrive/LanceDriveContracts.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/movement/MovementInfluenceService.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "gameplay/targeting/SweptGeometry.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "spaceShip/SpaceShip.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ly
{
	namespace
	{
		// Lance is an outlined inverted-V: its two arms are physical walls while
		// the space between them deliberately remains open.
		constexpr std::size_t WallArmCount = 2u;

		float FindValue(
			const sas::GameplayAttributeList& attributes,
			const sas::AttributeId& id,
			float fallback
		)
		{
			return sas::FindAttributeValue(attributes, id, fallback);
		}

		sf::Vector2f NormalizeOrDefault(const sf::Vector2f& value)
		{
			const float length = GetVectorLength(value);
			return length > 0.001f ? value / length : sf::Vector2f{ 0.f, -1.f };
		}

		sf::Color WithAlpha(const sf::Color& color, float multiplier)
		{
			return sf::Color{
				color.r,
				color.g,
				color.b,
				static_cast<std::uint8_t>(std::clamp(
					static_cast<float>(color.a) * multiplier,
					0.f,
					255.f
				))
			};
		}

		float DistanceSquaredToSegment(
			const sf::Vector2f& point,
			const sf::Vector2f& start,
			const sf::Vector2f& end
		)
		{
			const sf::Vector2f segment = end - start;
			const float lengthSquared = segment.x * segment.x + segment.y * segment.y;
			if (lengthSquared <= 0.0001f)
			{
				const sf::Vector2f delta = point - start;
				return delta.x * delta.x + delta.y * delta.y;
			}
			const sf::Vector2f toPoint = point - start;
			const float projection = std::clamp(
				(toPoint.x * segment.x + toPoint.y * segment.y) / lengthSquared,
				0.f,
				1.f
			);
			const sf::Vector2f closest = start + segment * projection;
			const sf::Vector2f delta = point - closest;
			return delta.x * delta.x + delta.y * delta.y;
		}

		class LanceDriveActorTypeHandler final : public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::LanceDrive;
			}

			AbilityActorValidationResult ValidateDefinition(
				const AbilityActorDefinition& definition
			) const override
			{
				const AbilityActorValidationResult baseResult =
					AbilityActorTypeHandler::ValidateDefinition(definition);
				if (!baseResult.isValid)
				{
					return baseResult;
				}
				if (!definition.presentationProfileId.IsValid() ||
					PresentationProfileRegistry<LanceDrivePresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					) == nullptr)
				{
					return { false, "Lance Drive needs a valid typed presentation profile." };
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const LanceDrivePresentationProfile* profile =
					PresentationProfileRegistry<LanceDrivePresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<LanceDriveActor>(&context.owner, *profile)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	LanceDriveActor::LanceDriveActor(
		World* world,
		Actor* owner,
		const LanceDrivePresentationProfile& presentationProfile
	)
		: AbilityWorldActor(world, owner)
		, mPresentationProfile(presentationProfile)
	{
		SetRenderLayer(RenderLayer::GroundDecal);
		// Both visible arms own solid fixtures; gameplay also resolves contacts
		// when this owner-following wall moves into a stationary ship.
		SetPhysicsBodyType(PhysicsBodyType::Static);
		SetAbilityPhysicsEnabled(true);
		SetCollisionLayer(CollisionLayer::Environment);
		SetCollisionMask(CollisionLayer::Enemy);
		SetDamageTags({ DamageTypeSchema::Kinetic });
		UpdateGeometry();
	}

	void LanceDriveActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mBaseDamage = std::max(0.f, FindValue(
			attributes, AbilityData::LanceDrive::Attribute::BaseDamage, 10.f
		));
		mSpeedDamageConversion = std::max(0.f, FindValue(
			attributes, AbilityData::LanceDrive::Attribute::SpeedDamageConversion, 0.20f
		));
		mEnergyMaxReference = std::max(0.f, FindValue(
			attributes, AbilityData::LanceDrive::Attribute::EnergyMaxReference, 50.f
		));
		mEnergyMaxConversionPerPoint = std::max(0.f, FindValue(
			attributes, AbilityData::LanceDrive::Attribute::EnergyMaxConversionPerPoint, 0.0001f
		));
		mSameTargetHitCooldown = std::max(0.01f, FindValue(
			attributes, AbilityData::LanceDrive::Attribute::SameTargetHitCooldown, 0.75f
		));
		mLength = std::max(1.f, FindValue(
			attributes, AbilityData::LanceDrive::Attribute::Length, 146.25f
		));
		mEdgeThickness = std::max(1.f, FindValue(
			attributes, AbilityData::LanceDrive::Attribute::EdgeThickness, 6.f
		));
		mOpeningAngleDegrees = std::clamp(FindValue(
			attributes, AbilityData::LanceDrive::Attribute::OpeningAngleDegrees, 50.f
		), 1.f, 179.f);
		mLateralKnockback = std::max(0.f, FindValue(
			attributes, AbilityData::LanceDrive::Attribute::LateralKnockback, 75.f
		));
	}

	std::size_t LanceDriveActor::GetPhysicsCollisionBoxCount() const
	{
		return WallArmCount;
	}

	PhysicsCollisionBox LanceDriveActor::GetPhysicsCollisionBox(std::size_t index) const
	{
		if (index >= GetPhysicsCollisionBoxCount())
		{
			return {};
		}

		const float halfOpeningRadians = DegreesToRadians(mOpeningAngleDegrees * 0.5f);
		const float halfBaseWidth = mLength * std::tan(halfOpeningRadians);
		const float halfArmLength = 0.5f * std::sqrt(
			mLength * mLength + halfBaseWidth * halfBaseWidth
		);
		const float side = index == 0u ? 1.f : -1.f;

		// The local +X axis is aligned to each visible arm. Local -Y is forward,
		// so the arm centres sit halfway from the rear endpoints to the forward tip.
		// Using the same geometry for the fixture and movement sweep keeps visual,
		// query, and collision behaviour identical.
		return PhysicsCollisionBox{
			{ halfArmLength, mEdgeThickness * 0.5f },
			{ side * halfBaseWidth * 0.5f, -(16.f + mLength * 0.5f) },
			std::atan2(mLength, side * halfBaseWidth) * 57.2957795131f
		};
	}

	void LanceDriveActor::ResolveEdgeEndpoints(
		sf::Vector2f& tip,
		sf::Vector2f& leftEnd,
		sf::Vector2f& rightEnd
	) const
	{
		const Actor* owner = GetOwnerActor();
		if (!owner)
		{
			tip = {};
			leftEnd = {};
			rightEnd = {};
			return;
		}
		const sf::Vector2f forward = NormalizeOrDefault(owner->GetActorForwardDirection());
		const sf::Vector2f right{ -forward.y, forward.x };
		const sf::Vector2f baseCenter = owner->GetActorLocation() + forward * 16.f;
		// The apex is in front of the ship and the two ends open back toward the
		// owner. This is an inverted V (∧), not the previous forward-opening V.
		tip = baseCenter + forward * mLength;
		const float halfAngle = DegreesToRadians(mOpeningAngleDegrees * 0.5f);
		const float halfBaseWidth = mLength * std::tan(halfAngle);
		leftEnd = baseCenter + right * halfBaseWidth;
		rightEnd = baseCenter - right * halfBaseWidth;
	}

	void LanceDriveActor::UpdateGeometry()
	{
		if (Actor* owner = GetOwnerActor())
		{
			SetActorLocation(owner->GetActorLocation());
			const sf::Vector2f forward = NormalizeOrDefault(owner->GetActorForwardDirection());
			SetActorRotation(std::atan2(forward.y, forward.x) * 57.2957795131f + 90.f);
		}
	}

	movement::MovementInfluenceSourceId LanceDriveActor::GetMovementSourceId() const
	{
		return static_cast<movement::MovementInfluenceSourceId>(
			reinterpret_cast<std::uintptr_t>(this)
		);
	}

	bool LanceDriveActor::IsInsideEdge(
		const sf::Vector2f& point,
		float targetRadius
	) const
	{
		sf::Vector2f tip;
		sf::Vector2f leftEnd;
		sf::Vector2f rightEnd;
		ResolveEdgeEndpoints(tip, leftEnd, rightEnd);
		const float contactDistance =
			std::max(0.f, targetRadius) + mEdgeThickness * 0.5f;
		const float contactDistanceSquared = contactDistance * contactDistance;
		return DistanceSquaredToSegment(point, tip, leftEnd) <= contactDistanceSquared ||
			DistanceSquaredToSegment(point, tip, rightEnd) <= contactDistanceSquared;
	}

	bool LanceDriveActor::ConsumesFrontalContact(
		const Actor& source,
		const Actor& target
	) const
	{
		const Actor* owner = GetOwnerActor();
		if (!owner || (&source != owner && &target != owner))
		{
			return false;
		}
		const Actor& other = &source == owner ? target : source;
		const SpaceShip* ship = dynamic_cast<const SpaceShip*>(&other);
		if (!ship || ship->GetIsPendingDestroy() || ship->GetHealthComponent().GetHealth() <= 0.f)
		{
			return false;
		}
		const auto bounds = ship->GetActorGlobalBounds();
		const float radius = std::max(bounds.size.x, bounds.size.y) * 0.5f;
		return IsInsideEdge(ship->GetActorLocation(), radius);
	}

	void LanceDriveActor::ApplyLanceHit(Actor& target, float speed)
	{
		Actor* owner = GetOwnerActor();
		if (!owner)
		{
			return;
		}
		float energyMax = 0.f;
		if (const auto* combatant = dynamic_cast<const Combatant*>(owner))
		{
			energyMax = std::max(0.f, combatant->GetAbilitySystemComponent().GetAttributes()
				.GetCurrentValue(OwnerAttributeIds::EnergyMax));
		}
		const float conversion = mSpeedDamageConversion +
			std::max(0.f, energyMax - mEnergyMaxReference) * mEnergyMaxConversionPerPoint;
		const float damage = mBaseDamage + std::max(0.f, speed) * conversion;

		ApplyCombatDamage(
			target,
			damage,
			owner,
			GetDamageTags(),
			GetDamagePayload(),
			GetSourceAbilityId(),
			GetSourceAbilityTags(),
			DamageDeliveryType::Contact,
			this
		);

	}

	bool LanceDriveActor::ResolveWallContact(Actor& target, float radius,
		const sf::Vector2f& previousPosition, float previousRotation, float deltaTime)
	{
		bool contacted = false;
		const auto rotate = [](sf::Vector2f value, float degrees)
		{
			const float angle = DegreesToRadians(degrees);
			return sf::Vector2f{ std::cos(angle) * value.x - std::sin(angle) * value.y,
				std::sin(angle) * value.x + std::cos(angle) * value.y };
		};
		for (std::size_t index = 0; index < GetPhysicsCollisionBoxCount(); ++index)
		{
			const PhysicsCollisionBox box = GetPhysicsCollisionBox(index);
			const sf::Vector2f center = GetActorLocation() + rotate(box.localCenter, GetActorRotation());
			const sf::Vector2f oldCenter = previousPosition + rotate(box.localCenter, previousRotation);
			const float angle = GetActorRotation() + box.localRotationDegrees;
			const sf::Vector2f local = rotate(target.GetActorLocation() - center, -angle);
			// Relative motion catches a moving wall crossing a stationary target.
			const sf::Vector2f oldLocal = rotate(target.GetActorLocation() - oldCenter,
				-(previousRotation + box.localRotationDegrees));
			const sf::Vector2f extent = box.halfExtents + sf::Vector2f{ radius, radius };
			float fraction = 1.f;
			sf::Vector2f normal;
			const bool swept = targeting::swept::SegmentIntersectsExpandedOrientedBox(
				oldLocal, local, {}, box.halfExtents, 0.f, radius, fraction, normal);
			const bool inside = std::abs(local.x) <= extent.x && std::abs(local.y) <= extent.y;
			if (!swept && !inside) continue;
			if (GetVectorLength(normal) < 0.5f)
			{
				// Initial overlaps need a separation normal even with zero velocity.
				normal = extent.x - std::abs(local.x) < extent.y - std::abs(local.y)
					? sf::Vector2f{ local.x >= 0.f ? 1.f : -1.f, 0.f }
					: sf::Vector2f{ 0.f, oldLocal.y >= 0.f ? 1.f : -1.f };
			}
			const float support = std::abs(normal.x) * extent.x + std::abs(normal.y) * extent.y;
			const float depth = support - (local.x * normal.x + local.y * normal.y);
			if (depth < -0.01f) continue;
			const sf::Vector2f worldNormal = rotate(normal, angle);
			// Positional separation is mandatory: an impulse alone leaves an
			// overlapping ship trapped by its own next movement sweep.
			target.AddActorLocationOffset(worldNormal * (std::max(0.f, depth) + 0.5f));
			const sf::Vector2f wallVelocity = deltaTime > 0.0001f
				? (center - oldCenter) / deltaTime : sf::Vector2f{};
			const sf::Vector2f relativeVelocity = target.GetVelocity() - wallVelocity;
			const float approach = std::max(0.f,
				-(relativeVelocity.x * worldNormal.x + relativeVelocity.y * worldNormal.y));
			const auto lastHit = mNextHitTime.find(&target);
			if (!contacted && (lastHit == mNextHitTime.end() || lastHit->second <= mElapsed))
			{
				movement::MovementInfluenceService::ApplyImpulse(target,
					{ worldNormal * (approach * 1.35f + mLateralKnockback), 0.15f });
			}
			contacted = true;
		}
		return contacted;
	}

	void LanceDriveActor::Tick(float deltaTime)
	{
		Actor* owner = GetOwnerActor();
		if (!owner || owner->GetIsPendingDestroy())
		{
			Destroy();
			return;
		}

		AbilityWorldActor::Tick(deltaTime);
		mElapsed += std::max(0.f, deltaTime);
		const sf::Vector2f previousPosition = GetActorLocation();
		const float previousRotation = GetActorRotation();
		UpdateGeometry();

		const auto* ownerCombatant = dynamic_cast<const Combatant*>(owner);
		if (!ownerCombatant || !ownerCombatant->GetAbilitySystemComponent().HasOwnedTag(
			AbilityData::LanceDrive::State::Active
		))
		{
			Destroy();
			return;
		}
		if (SpaceShip* ownerShip = dynamic_cast<SpaceShip*>(owner))
		{
			// Lance accelerates continuously in the ship's current forward direction.
			// Rotation therefore changes the travel direction while the shared
			// movement component still owns caps, damping, afterburner, and collision.
			movement::MovementInfluenceService::SetAccelerationSource(
				*ownerShip,
				movement::AccelerationSourceRequest{
					GetMovementSourceId(),
					ownerShip->GetActorForwardDirection() *
						ownerShip->GetMovementComponent().ResolveForwardThrust()
				}
			);
		}
		World* world = GetWorld();
		if (!world)
		{
			return;
		}
		const float speed = GetVectorLength(owner->GetVelocity());
		for (const shared_ptr<Actor>& candidate : targeting::FindOpposingCombatants(
			*world,
			*owner,
			owner->GetActorLocation(),
			mLength / std::cos(DegreesToRadians(mOpeningAngleDegrees * 0.5f)) +
				128.f + GetVectorLength(GetActorLocation() - previousPosition),
			false
		))
		{
			SpaceShip* target = candidate ? dynamic_cast<SpaceShip*>(candidate.get()) : nullptr;
			if (!target || target->GetIsPendingDestroy() ||
				target->GetHealthComponent().GetHealth() <= 0.f)
			{
				continue;
			}
			const auto bounds = target->GetActorGlobalBounds();
			const float radius = std::max(bounds.size.x, bounds.size.y) * 0.5f;
			if (!ResolveWallContact(*target, radius, previousPosition, previousRotation, deltaTime))
			{
				continue;
			}
			const auto found = mNextHitTime.find(target);
			if (found != mNextHitTime.end() && found->second > mElapsed)
			{
				continue;
			}
			ApplyLanceHit(*target, speed);
			mNextHitTime[target] = mElapsed + mSameTargetHitCooldown;
		}
	}

	void LanceDriveActor::Destroy()
	{
		if (Actor* owner = GetOwnerActor())
		{
			movement::MovementInfluenceService::RemoveSource(*owner, GetMovementSourceId());
		}
		// The wall owns all of its fixtures through its one physics body, so there
		// are no feature-created child actors left to clean up here.
		AbilityWorldActor::Destroy();
	}

	void LanceDriveActor::Render(sf::RenderWindow& window)
	{
		if (!IsRenderEnabled())
		{
			return;
		}
		sf::Vector2f tip;
		sf::Vector2f leftEnd;
		sf::Vector2f rightEnd;
		ResolveEdgeEndpoints(tip, leftEnd, rightEnd);
		const float pulse = 0.88f + 0.12f * std::sin(
			mElapsed * mPresentationProfile.visual.pulseSpeed
		);
		const sf::Color edge = WithAlpha(
			mPresentationProfile.visual.edgeColor,
			pulse
		);
		sf::VertexArray lines(sf::PrimitiveType::Lines, 4);
		lines[0] = sf::Vertex{ tip, edge };
		lines[1] = sf::Vertex{ leftEnd, edge };
		lines[2] = sf::Vertex{ tip, edge };
		lines[3] = sf::Vertex{ rightEnd, edge };
		window.draw(lines);
	}

	bool RegisterLanceDriveActorType()
	{
		static const bool registered = AbilityActorRegistry::RegisterHandler(
			std::make_unique<LanceDriveActorTypeHandler>()
		);
		return registered;
	}
}
