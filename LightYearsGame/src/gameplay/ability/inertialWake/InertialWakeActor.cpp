#include "gameplay/ability/inertialWake/InertialWakeActor.h"

#include "attributes/AttributeSystem.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/inertialWake/InertialWakeContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/movement/MovementInfluenceService.h"
#include "gameplay/portal/PortalTransferParticipant.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "spaceShip/SpaceShip.h"

#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ly
{
	namespace
	{
		const List<sas::AttributeId> InertialWakeAttributeRoots{
			AbilityData::InertialWake::Actor::Wake::Root
		};
		const List<sas::AttributeId> InertialWakeCommonAttributes{
			AreaAttributeIds::Length,
			AreaAttributeIds::Width
		};

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

		class InertialWakeActorTypeHandler final : public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::InertialWake;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return InertialWakeAttributeRoots;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return InertialWakeCommonAttributes;
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
				for (const sas::AttributeId& required : {
					AreaAttributeIds::Length,
					AreaAttributeIds::Width,
					AbilityData::InertialWake::Actor::Wake::SpeedDamageConversion,
					AbilityData::InertialWake::Actor::Wake::SameTargetHitCooldown
				})
				{
					const sas::GameplayAttribute* attribute =
						sas::FindAttribute(definition.attributes, required);
					if (!attribute || attribute->baseValue <= 0.f)
					{
						return { false, "Inertial Wake needs positive geometry, conversion and hit cooldown attributes." };
					}
				}
				if (!definition.presentationProfileId.IsValid() ||
					PresentationProfileRegistry<InertialWakePresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					) == nullptr)
				{
					return { false, "Inertial Wake needs a valid typed presentation profile." };
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const InertialWakePresentationProfile* profile =
					PresentationProfileRegistry<InertialWakePresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<InertialWakeActor>(&context.owner, *profile)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	InertialWakeActor::InertialWakeActor(
		World* world,
		Actor* owner,
		const InertialWakePresentationProfile& presentationProfile
	)
		: AbilityWorldActor(world, owner)
		, mPresentationProfile(presentationProfile)
	{
		SetRenderLayer(RenderLayer::GroundDecal);
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		SetDamageTags({ DamageTypeSchema::Kinetic });
	}

	void InertialWakeActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mCurrentLength = std::max(1.f, FindValue(attributes, AreaAttributeIds::Length, 220.f));
		mBaseEdgeThickness = std::max(1.f, FindValue(attributes, AreaAttributeIds::Width, 3.f));
		mSpeedDamageConversion = std::max(0.f, FindValue(
			attributes, AbilityData::InertialWake::Actor::Wake::SpeedDamageConversion, 0.30f
		));
		mEnergyPowerReference = std::max(0.f, FindValue(
			attributes, AbilityData::InertialWake::Actor::Wake::EnergyPowerReference, 50.f
		));
		mEnergyPowerConversionPerPoint = std::max(0.f, FindValue(
			attributes, AbilityData::InertialWake::Actor::Wake::EnergyPowerConversionPerPoint, 0.0001f
		));
		mSameTargetHitCooldown = std::max(0.01f, FindValue(
			attributes, AbilityData::InertialWake::Actor::Wake::SameTargetHitCooldown, 2.f
		));
		mMinimumSpeedRatio = std::max(0.f, FindValue(
			attributes, AbilityData::InertialWake::Actor::Wake::MinimumSpeedRatio, 0.20f
		));
		mLengthPerEffectiveRatio = std::max(0.f, FindValue(
			attributes, AbilityData::InertialWake::Actor::Wake::LengthPerEffectiveRatio, 176.f
		));
		mWidthPerEffectiveRatio = std::max(0.f, FindValue(
			attributes, AbilityData::InertialWake::Actor::Wake::WidthPerEffectiveRatio, 1.5f
		));
		mOpeningAngleDegrees = std::clamp(FindValue(
			attributes, AbilityData::InertialWake::Actor::Wake::OpeningAngleDegrees, 130.f
		), 1.f, 179.f);
		mDiminishingStartRatio = std::max(0.01f, FindValue(
			attributes, AbilityData::InertialWake::Actor::Wake::DiminishingStartRatio, 1.5f
		));
		mDiminishingExcessMultiplier = std::clamp(FindValue(
			attributes, AbilityData::InertialWake::Actor::Wake::DiminishingExcessMultiplier, 0.5f
		), 0.f, 1.f);
		mBaseKnockbackSpeed = std::max(0.f, FindValue(
			attributes, AbilityData::InertialWake::Actor::Wake::BaseKnockbackSpeed, 120.f
		));
		mKnockbackPerEffectiveRatio = std::max(0.f, FindValue(
			attributes, AbilityData::InertialWake::Actor::Wake::KnockbackPerEffectiveRatio, 140.f
		));
		mBaseStunDuration = std::max(0.f, FindValue(
			attributes, AbilityData::InertialWake::Actor::Wake::BaseStunDuration, 0.25f
		));
		mStunPerEffectiveRatio = std::max(0.f, FindValue(
			attributes, AbilityData::InertialWake::Actor::Wake::StunPerEffectiveRatio, 0.20f
		));
	}

	float InertialWakeActor::ResolveEffectiveSpeedRatio(
		float speed,
		float normalTopSpeed
	) const
	{
		const float rawRatio = normalTopSpeed > 0.001f ? speed / normalTopSpeed : 0.f;
		if (rawRatio <= mDiminishingStartRatio)
		{
			return rawRatio;
		}
		// Geometry and control soften only after the configured threshold. Damage
		// deliberately uses raw speed and has no cap or diminishing rule.
		return mDiminishingStartRatio +
			(rawRatio - mDiminishingStartRatio) * mDiminishingExcessMultiplier;
	}

	void InertialWakeActor::UpdateGeometry()
	{
		Actor* owner = GetOwnerActor();
		if (!owner)
		{
			return;
		}
		SetActorLocation(owner->GetActorLocation());
		const sf::Vector2f forward = NormalizeOrDefault(owner->GetActorForwardDirection());
		SetActorRotation(std::atan2(forward.y, forward.x) * 57.2957795131f + 90.f);
	}

	bool InertialWakeActor::IsInsideWakeEdge(
		const sf::Vector2f& point,
		float length,
		float edgeThickness,
		float targetRadius
	) const
	{
		Actor* owner = GetOwnerActor();
		if (!owner || length <= 0.f || edgeThickness <= 0.f)
		{
			return false;
		}
		const sf::Vector2f forward = NormalizeOrDefault(owner->GetActorForwardDirection());
		const sf::Vector2f right{ -forward.y, forward.x };
		const sf::Vector2f tip = owner->GetActorLocation() - forward * 16.f;
		const float halfAngleRadians =
			(mOpeningAngleDegrees * 0.5f) * (3.14159265f / 180.f);
		const sf::Vector2f backward = -forward;
		const float cosHalfAngle = std::cos(halfAngleRadians);
		const float sinHalfAngle = std::sin(halfAngleRadians);
		const sf::Vector2f leftDirection =
			backward * cosHalfAngle + right * sinHalfAngle;
		const sf::Vector2f rightDirection =
			backward * cosHalfAngle - right * sinHalfAngle;
		const sf::Vector2f leftEnd = tip + leftDirection * length;
		const sf::Vector2f rightEnd = tip + rightDirection * length;

		auto DistanceSquaredToSegment = [](
			const sf::Vector2f& value,
			const sf::Vector2f& start,
			const sf::Vector2f& end
		)
		{
			const sf::Vector2f segment = end - start;
			const float segmentLengthSquared =
				segment.x * segment.x + segment.y * segment.y;
			if (segmentLengthSquared <= 0.0001f)
			{
				const sf::Vector2f delta = value - start;
				return delta.x * delta.x + delta.y * delta.y;
			}
			const sf::Vector2f toValue = value - start;
			const float projection = std::clamp(
				(toValue.x * segment.x + toValue.y * segment.y) / segmentLengthSquared,
				0.f,
				1.f
			);
			const sf::Vector2f closest = start + segment * projection;
			const sf::Vector2f delta = value - closest;
			return delta.x * delta.x + delta.y * delta.y;
		};

		// The visual line is thin, but a ship should be hit when its body touches
		// the line. Using the target's conservative radius fixes endpoint misses
		// without turning the empty interior into a damage zone.
		const float maximumDistance = edgeThickness * 0.5f + std::max(0.f, targetRadius);
		const float maximumDistanceSquared = maximumDistance * maximumDistance;
		return DistanceSquaredToSegment(point, tip, leftEnd) <= maximumDistanceSquared ||
			DistanceSquaredToSegment(point, tip, rightEnd) <= maximumDistanceSquared;
	}

	void InertialWakeActor::ApplyStun(SpaceShip& target, float duration) const
	{
		const ControlResponse response = target.ResolveControlResponse(
			GameplayTags::State::Effect::Control::Stunned
		);
		if (duration <= 0.f || response.mode == ControlResponseMode::Immune ||
			response.mode == ControlResponseMode::InterruptOnly)
		{
			return;
		}
		const sas::GameplayEffectDefinition* definition =
			EffectData::FindGameplayEffectDefinition(AbilityData::InertialWake::Effect::StunId);
		if (!definition)
		{
			return;
		}
		sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(*definition);
		spec.duration = duration * std::max(0.f, response.durationMultiplier);
		spec.maxStacks = 1;
		if (spec.duration > 0.f)
		{
			target.GetAbilitySystemComponent().ApplyGameplayEffect(
				spec,
				sas::GameplayEffectSourceContext{ GetOwnerActor(), GetSourceAbilityInstance() }
			);
		}
	}

	void InertialWakeActor::ApplyWakeHit(
		Actor& target,
		float speed,
		float effectiveRatio
	)
	{
		Actor* owner = GetOwnerActor();
		if (!owner)
		{
			return;
		}
		float energyPower = 0.f;
		if (const auto* combatantOwner = dynamic_cast<const Combatant*>(owner))
		{
			energyPower = std::max(0.f, combatantOwner->GetAbilitySystemComponent()
				.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower));
		}
		const float conversion = mSpeedDamageConversion +
			std::max(0.f, energyPower - mEnergyPowerReference) * mEnergyPowerConversionPerPoint;
		// The hit samples real velocity here, not an activation snapshot. No upper
		// limit is applied: high-speed builds retain their full kinetic payoff.
		ApplyCombatDamage(
			target,
			std::max(0.f, speed * conversion),
			owner,
			GetDamageTags(),
			GetDamagePayload(),
			GetSourceAbilityId(),
			GetSourceAbilityTags(),
			DamageDeliveryType::Area,
			this
		);

		SpaceShip* targetShip = dynamic_cast<SpaceShip*>(&target);
		if (!targetShip)
		{
			return;
		}
		const sf::Vector2f forward = NormalizeOrDefault(owner->GetActorForwardDirection());
		const sf::Vector2f right{ -forward.y, forward.x };
		const sf::Vector2f relative = target.GetActorLocation() - owner->GetActorLocation();
		const float side = relative.x * right.x + relative.y * right.y;
		const sf::Vector2f lateralDirection = side >= 0.f ? right : -right;
		movement::MovementInfluenceService::ApplyImpulse(
			*targetShip,
			movement::ImpulseRequest{
				lateralDirection * (mBaseKnockbackSpeed +
					effectiveRatio * mKnockbackPerEffectiveRatio),
				targetShip->GetMovementComponent().GetAttributes().linearDamping.currentValue
			}
		);
		ApplyStun(
			*targetShip,
			mBaseStunDuration + effectiveRatio * mStunPerEffectiveRatio
		);
	}

	void InertialWakeActor::Tick(float deltaTime)
	{
		Actor* owner = GetOwnerActor();
		if (!owner || owner->GetIsPendingDestroy())
		{
			Destroy();
			return;
		}
		if (const auto* participant = dynamic_cast<const PortalTransferParticipant*>(owner);
			participant && participant->IsInPortalTransit())
		{
			SetRenderEnabled(false);
			return;
		}
		SetRenderEnabled(true);
		AbilityWorldActor::Tick(deltaTime);
		mElapsed += std::max(0.f, deltaTime);
		if (const auto* combatant = dynamic_cast<const Combatant*>(owner);
			!combatant || !combatant->GetAbilitySystemComponent().HasOwnedTag(
				AbilityData::InertialWake::State::Active))
		{
			Destroy();
			return;
		}

		UpdateGeometry();
		const auto* ship = dynamic_cast<const SpaceShip*>(owner);
		if (!ship)
		{
			return;
		}
		const float normalTopSpeed = std::max(
			0.f,
			ship->GetMovementComponent().GetAttributes().maxSpeed.currentValue
		);
		const float speed = GetVectorLength(owner->GetVelocity());
		const float rawRatio = normalTopSpeed > 0.001f ? speed / normalTopSpeed : 0.f;
		if (rawRatio < mMinimumSpeedRatio)
		{
			mCurrentLength = 0.f;
			mCurrentWidth = 0.f;
			return;
		}
		const float effectiveRatio = ResolveEffectiveSpeedRatio(speed, normalTopSpeed);
		mCurrentLength = mLengthPerEffectiveRatio * effectiveRatio;
		mCurrentWidth = mBaseEdgeThickness + mWidthPerEffectiveRatio * effectiveRatio;
		World* world = GetWorld();
		if (!world)
		{
			return;
		}
		for (const shared_ptr<Actor>& target : targeting::FindOpposingCombatants(
			*world,
			*owner,
			owner->GetActorLocation(),
			mCurrentLength + 128.f
		))
		{
			if (!target || target->GetIsPendingDestroy())
			{
				continue;
			}
			const sf::FloatRect targetBounds = target->GetActorGlobalBounds();
			const float targetRadius = std::max(
				targetBounds.size.x,
				targetBounds.size.y
			) * 0.5f;
			if (!IsInsideWakeEdge(
				target->GetActorLocation(),
				mCurrentLength,
				mCurrentWidth,
				targetRadius
			))
			{
				continue;
			}
			const auto nextHit = mNextHitTime.find(target.get());
			if (nextHit != mNextHitTime.end() && nextHit->second > mElapsed)
			{
				continue;
			}
			ApplyWakeHit(*target, speed, effectiveRatio);
			mNextHitTime[target.get()] = mElapsed + mSameTargetHitCooldown;
		}
	}

	void InertialWakeActor::Render(sf::RenderWindow& window)
	{
		if (!IsRenderEnabled() || mCurrentLength <= 0.f || mCurrentWidth <= 0.f)
		{
			return;
		}
		Actor* owner = GetOwnerActor();
		if (!owner)
		{
			return;
		}
		const sf::Vector2f forward = NormalizeOrDefault(owner->GetActorForwardDirection());
		const sf::Vector2f right{ -forward.y, forward.x };
		const sf::Vector2f tip = owner->GetActorLocation() - forward * 16.f;
		const sf::Vector2f backward = -forward;
		const float halfAngleRadians =
			(mOpeningAngleDegrees * 0.5f) * (3.14159265f / 180.f);
		const float cosHalfAngle = std::cos(halfAngleRadians);
		const float sinHalfAngle = std::sin(halfAngleRadians);
		const sf::Vector2f leftEnd = tip + (
			backward * cosHalfAngle + right * sinHalfAngle
		) * mCurrentLength;
		const sf::Vector2f rightEnd = tip + (
			backward * cosHalfAngle - right * sinHalfAngle
		) * mCurrentLength;
		const float pulse = 0.88f + 0.12f * std::sin(mElapsed * mPresentationProfile.visual.pulseSpeed);

		// The interior remains empty. The two quads below are only the V roof
		// edges, and the gameplay query uses the same two line segments.
		sf::VertexArray outline(sf::PrimitiveType::Triangles, 12);
		const sf::Color edge = WithAlpha(mPresentationProfile.visual.edgeColor, pulse);
		auto SetThickSegment = [&outline, &edge](
			std::size_t offset,
			const sf::Vector2f& start,
			const sf::Vector2f& end,
			float thickness
		)
		{
			const sf::Vector2f segment = end - start;
			const float segmentLength = GetVectorLength(segment);
			const sf::Vector2f normal = segmentLength > 0.001f
				? sf::Vector2f{ -segment.y, segment.x } / segmentLength * (thickness * 0.5f)
				: sf::Vector2f{};
			const sf::Vector2f a = start + normal;
			const sf::Vector2f b = end + normal;
			const sf::Vector2f c = end - normal;
			const sf::Vector2f d = start - normal;
			outline[offset + 0] = sf::Vertex{ a, edge };
			outline[offset + 1] = sf::Vertex{ b, edge };
			outline[offset + 2] = sf::Vertex{ c, edge };
			outline[offset + 3] = sf::Vertex{ a, edge };
			outline[offset + 4] = sf::Vertex{ c, edge };
			outline[offset + 5] = sf::Vertex{ d, edge };
		};
		SetThickSegment(0, tip, leftEnd, mCurrentWidth);
		SetThickSegment(6, tip, rightEnd, mCurrentWidth);
		window.draw(outline);
	}

	bool RegisterInertialWakeActorType()
	{
		static const bool registered = AbilityActorRegistry::RegisterHandler(
			std::make_unique<InertialWakeActorTypeHandler>()
		);
		return registered;
	}
}
