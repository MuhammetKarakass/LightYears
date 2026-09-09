#include "gameplay/ability/aegisReaver/AegisReaverProjectileActor.h"

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/aegisReaver/AegisReaverContracts.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/projectile/ProjectileCaptureVolume.h"
#include "gameplay/projectile/ProjectileReflectionService.h"
#include "gameplay/projectile/ProjectileSweep.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "spaceShip/SpaceShip.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

namespace ly
{
	namespace
	{
		constexpr float MinimumDirectionLength = 0.001f;
		constexpr float ReturnArrivalPadding = 8.f;
		constexpr float MaximumRangeHoverDuration = 0.5f;
		constexpr float HoverSpeedMultiplier = 0.14f;
		constexpr float OutboundMinimumSpeedMultiplier = 0.12f;
		constexpr float ReturnMinimumSpeedMultiplier = 0.12f;
		constexpr float MotionCurveLogarithmStrength = 1.5f;
		constexpr float OutboundDecelerationStartSeconds = 0.75f;
		constexpr float OutboundDecelerationDuration = 0.45f;
		constexpr float ReturnAccelerationDuration = 0.45f;
		constexpr float Pi = 3.14159265358979323846f;

		const List<sas::AttributeId> AegisReaverProjectileCommonAttributes{
			CommonAttributeIds::Damage,
			CommonAttributeIds::Range,
			CollisionAttributeIds::Radius
		};

		const List<sas::AttributeId> AegisReaverProjectileAttributeRoots{
			AbilityData::AegisReaver::Actor::Projectile::Root,
			DamageAttributeIds::Root
		};

		sf::Vector2f NormalizeOrDefault(const sf::Vector2f& value)
		{
			const float length = GetVectorLength(value);
			return length > MinimumDirectionLength
				? value / length
				: sf::Vector2f{ 0.f, -1.f };
		}

		float ClampUnit(float value)
		{
			return std::clamp(value, 0.f, 1.f);
		}

		// The curve consumes a time-normalized deceleration phase, not distance.
		// Range controls when the disc turns around; it never resets this motion
		// curve when the owner moves.
		float EvaluateOutboundSpeedMultiplier(float normalizedPhaseTime)
		{
			const float progress = ClampUnit(normalizedPhaseTime);
			const float logarithmicProgress = std::log1p(
				MotionCurveLogarithmStrength * progress
			) / std::log1p(MotionCurveLogarithmStrength);
			return OutboundMinimumSpeedMultiplier +
				(1.f - OutboundMinimumSpeedMultiplier) * (1.f - logarithmicProgress);
		}

		// The return uses the inverse logarithmic shape: it starts deliberately
		// slow after the hover, then accelerates sharply as it closes on the owner.
		float EvaluateReturnSpeedMultiplier(float normalizedPhaseTime)
		{
			const float progress = ClampUnit(normalizedPhaseTime);
			const float delayedAcceleration = 1.f - std::log1p(
				MotionCurveLogarithmStrength * (1.f - progress)
			) / std::log1p(MotionCurveLogarithmStrength);
			return ReturnMinimumSpeedMultiplier +
				(1.f - ReturnMinimumSpeedMultiplier) * delayedAcceleration;
		}

		sf::Vector2f Rotate(const sf::Vector2f& direction, float radians)
		{
			const float cosine = std::cos(radians);
			const float sine = std::sin(radians);
			return {
				direction.x * cosine - direction.y * sine,
				direction.x * sine + direction.y * cosine
			};
		}

		class AegisReaverProjectileActorType final : public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::AegisReaverProjectile;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return AegisReaverProjectileAttributeRoots;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return AegisReaverProjectileCommonAttributes;
			}

			AbilityActorValidationResult ValidateDefinition(
				const AbilityActorDefinition& definition
			) const override
			{
				const AbilityActorValidationResult base =
					AbilityActorTypeHandler::ValidateDefinition(definition);
				if (!base.isValid)
				{
					return base;
				}

				for (const sas::AttributeId& attributeId : {
					CommonAttributeIds::Damage,
					CommonAttributeIds::Range,
					CollisionAttributeIds::Radius,
					AbilityData::AegisReaver::Actor::Projectile::ProjectileSpeed,
					AbilityData::AegisReaver::Actor::Projectile::ReturnSpeed,
					AbilityData::AegisReaver::Actor::Projectile::ShieldConversionRatio,
					AbilityData::AegisReaver::Actor::Projectile::ShieldStealRatio
				})
				{
					if (!sas::FindAttribute(definition.attributes, attributeId))
					{
						return { false, "Aegis Reaver projectile is missing required attribute '" +
							std::string{ attributeId.GetName() } + "'." };
					}
				}

				const float speed = sas::FindAttributeValue(
					definition.attributes,
					AbilityData::AegisReaver::Actor::Projectile::ProjectileSpeed
				);
				const float returnSpeed = sas::FindAttributeValue(
					definition.attributes,
					AbilityData::AegisReaver::Actor::Projectile::ReturnSpeed
				);
				const float range = sas::FindAttributeValue(
					definition.attributes,
					CommonAttributeIds::Range
				);
				const float conversion = sas::FindAttributeValue(
					definition.attributes,
					AbilityData::AegisReaver::Actor::Projectile::ShieldConversionRatio
				);
				const float steal = sas::FindAttributeValue(
					definition.attributes,
					AbilityData::AegisReaver::Actor::Projectile::ShieldStealRatio
				);
				return speed > 0.f && returnSpeed > 0.f && range > 0.f &&
					conversion >= 0.f && steal >= 0.f && steal <= 1.f &&
					definition.presentationProfileId.IsValid() &&
					PresentationProfileRegistry<AegisReaverPresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					) != nullptr
					? AbilityActorValidationResult{ true, {} }
					: AbilityActorValidationResult{
						false,
						"Aegis Reaver requires valid travel, shield, collision, and presentation values."
					};
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const AegisReaverPresentationProfile* profile =
					PresentationProfileRegistry<AegisReaverPresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<AegisReaverProjectileActor>(&context.owner, *profile)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	AegisReaverProjectileActor::AegisReaverProjectileActor(
		World* world,
		Actor* owner,
		const AegisReaverPresentationProfile& presentationProfile
	)
		: AbilityWorldActor{ world, owner },
		  mPresentationProfile{ presentationProfile }
	{
		// Aegis owns swept movement itself so outgoing range and owner-seeking
		// return remain deterministic after a reflection or Relay conversion.
		SetAbilityPhysicsEnabled(false);
	}

	void AegisReaverProjectileActor::BeginPlay()
	{
		AbilityWorldActor::BeginPlay();
		if (mFlightState)
		{
			return;
		}

		auto* owner = dynamic_cast<SpaceShip*>(GetOwnerActor());
		if (!owner)
		{
			Destroy();
			return;
		}

		const float committedShield = std::max(0.f, owner->GetShieldComponent().GetShield());
		owner->GetShieldComponent().ChangeShield(-committedShield);
		mReturnShieldPayload = committedShield;
		SetDamage(GetDamage() + committedShield * mShieldConversionRatio);
		mFlightState = std::make_shared<AegisReaverFlightState>(*owner);
		mFlightState->RegisterProjectile();
		mFlightRegistered = true;
		UpdateVelocityAndRotation(GetActorForwardDirection(), mProjectileSpeed);
	}

	void AegisReaverProjectileActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mProjectileSpeed = std::max(0.f, sas::FindAttributeValue(
			attributes,
			AbilityData::AegisReaver::Actor::Projectile::ProjectileSpeed
		));
		mReturnSpeed = std::max(0.f, sas::FindAttributeValue(
			attributes,
			AbilityData::AegisReaver::Actor::Projectile::ReturnSpeed
		));
		mMaximumRange = std::max(0.f, sas::FindAttributeValue(
			attributes,
			CommonAttributeIds::Range
		));
		mShieldConversionRatio = std::max(0.f, sas::FindAttributeValue(
			attributes,
			AbilityData::AegisReaver::Actor::Projectile::ShieldConversionRatio
		));
		mShieldStealRatio = std::clamp(sas::FindAttributeValue(
			attributes,
			AbilityData::AegisReaver::Actor::Projectile::ShieldStealRatio
		), 0.f, 1.f);
	}

	void AegisReaverProjectileActor::Tick(float deltaTime)
	{
		if (IsInPortalTransit())
		{
			AbilityWorldActor::Tick(deltaTime);
			return;
		}

		const float safeDeltaTime = std::max(0.f, deltaTime);
		mSpinDegrees = std::fmod(
			mSpinDegrees + mPresentationProfile.visual.spinDegreesPerSecond * safeDeltaTime,
			360.f
		);
		if (!GetIsPendingDestroy())
		{
			if (mPhase == Phase::Outbound)
			{
				MoveOutbound(safeDeltaTime);
			}
			else if (mPhase == Phase::HoveringAtMaximumRange)
			{
				TickMaximumRangeHover(safeDeltaTime);
			}
			else
			{
				MoveReturning(safeDeltaTime);
			}
		}
		AbilityWorldActor::Tick(safeDeltaTime);
	}

	void AegisReaverProjectileActor::Destroy()
	{
		ReportFlightTerminal(mPhase == Phase::Returning &&
			GetReturnOwner() &&
			GetVectorLength(GetReturnOwner()->GetActorLocation() - GetActorLocation()) <=
				GetPhysicsCollisionRadius() + GetReturnOwner()->GetPhysicsCollisionRadius() +
				ReturnArrivalPadding);
		AbilityWorldActor::Destroy();
	}

	void AegisReaverProjectileActor::MoveOutbound(float deltaTime)
	{
		const float remainingRange = std::max(0.f, mMaximumRange - mOutboundTravelDistance);
		if (remainingRange <= 0.f || mProjectileSpeed <= 0.f || deltaTime <= 0.f)
		{
			BeginMaximumRangeHover();
			return;
		}

		const float normalizedProgress = ClampUnit(
			(mOutboundElapsed - OutboundDecelerationStartSeconds) /
				OutboundDecelerationDuration
		);
		const float currentSpeed = mProjectileSpeed *
			EvaluateOutboundSpeedMultiplier(normalizedProgress);
		const sf::Vector2f direction = NormalizeOrDefault(GetVelocity());
		UpdateVelocityAndRotation(direction, currentSpeed);
		const float travelDistance = std::min(currentSpeed * deltaTime, remainingRange);
		const sf::Vector2f start = GetActorLocation();
		const sf::Vector2f end = start + direction * travelDistance;
		if (!ProcessOutboundContacts(start, end, travelDistance))
		{
			return;
		}
		SetActorLocation(end);
		mOutboundTravelDistance += travelDistance;
		mOutboundElapsed += deltaTime;
		if (mOutboundTravelDistance >= mMaximumRange - 0.001f)
		{
			BeginMaximumRangeHover();
		}
	}

	void AegisReaverProjectileActor::BeginMaximumRangeHover()
	{
		if (mPhase != Phase::Outbound)
		{
			return;
		}
		mPhase = Phase::HoveringAtMaximumRange;
		mMaximumRangeHoverElapsed = 0.f;
		mMaximumRangeHoverStartDirection = NormalizeOrDefault(GetVelocity());
	}

	void AegisReaverProjectileActor::TickMaximumRangeHover(float deltaTime)
	{
		mMaximumRangeHoverElapsed += std::max(0.f, deltaTime);
		const float normalizedProgress = ClampUnit(
			mMaximumRangeHoverElapsed / MaximumRangeHoverDuration
		);
		// Do not freeze at range. A low-speed logarithmic half turn keeps the disc
		// visually alive while holding it close to the maximum-range point.
		const float turnProgress = std::log1p(
			MotionCurveLogarithmStrength * normalizedProgress
		) / std::log1p(MotionCurveLogarithmStrength);
		const sf::Vector2f direction = NormalizeOrDefault(Rotate(
			mMaximumRangeHoverStartDirection,
			Pi * turnProgress
		));
		const float hoverSpeed = mProjectileSpeed * HoverSpeedMultiplier;
		UpdateVelocityAndRotation(direction, hoverSpeed);
		SetActorLocation(GetActorLocation() + direction * hoverSpeed * deltaTime);
		if (normalizedProgress >= 1.f)
		{
			BeginReturning();
		}
	}

	void AegisReaverProjectileActor::MoveReturning(float deltaTime)
	{
		SpaceShip* owner = GetReturnOwner();
		if (!owner || owner->GetIsPendingDestroy())
		{
			Destroy();
			return;
		}

		const sf::Vector2f toOwner = owner->GetActorLocation() - GetActorLocation();
		const float arrivalDistance = GetPhysicsCollisionRadius() +
			owner->GetPhysicsCollisionRadius() + ReturnArrivalPadding;
		const float distance = GetVectorLength(toOwner);
		if (distance <= arrivalDistance)
		{
			ReportFlightTerminal(true);
			AbilityWorldActor::Destroy();
			return;
		}

		const float returnProgress = ClampUnit(
			mReturnElapsed / ReturnAccelerationDuration
		);
		const float currentSpeed = mReturnSpeed *
			EvaluateReturnSpeedMultiplier(returnProgress);
		// Aim a short time ahead of a moving owner. Combined with swept arrival
		// below, this prevents a fast-moving ship from permanently outrunning a
		// returning disc that is already directly in front of it.
		const float leadTime = std::clamp(
			distance / std::max(currentSpeed, MinimumDirectionLength),
			0.f,
			0.35f
		);
		const sf::Vector2f direction = NormalizeOrDefault(
			toOwner + owner->GetVelocity() * leadTime
		);
		UpdateVelocityAndRotation(direction, currentSpeed);
		const float travelDistance = std::min(
			std::max(0.f, currentSpeed * deltaTime),
			std::max(0.f, distance - arrivalDistance)
		);
		const sf::Vector2f start = GetActorLocation();
		const sf::Vector2f end = start + direction * travelDistance;
		for (const projectile::SweptContact& contact : projectile::FindSweptContacts(
			*this,
			start,
			end,
			GetPhysicsCollisionRadius()
		))
		{
			if (auto* target = dynamic_cast<SpaceShip*>(contact.actor))
			{
				TryHitShip(*target);
			}
		}
		SetActorLocation(end);
		mReturnElapsed += deltaTime;
		if (GetVectorLength(owner->GetActorLocation() - end) <=
			arrivalDistance + MinimumDirectionLength)
		{
			ReportFlightTerminal(true);
			AbilityWorldActor::Destroy();
		}
	}

	bool AegisReaverProjectileActor::ProcessOutboundContacts(
		const sf::Vector2f& start,
		const sf::Vector2f& end,
		float travelledDistance
	)
	{
		for (const projectile::SweptContact& contact : projectile::FindSweptContacts(
			*this,
			start,
			end,
			GetPhysicsCollisionRadius()
		))
		{
			if (!contact.actor)
			{
				continue;
			}
			if (auto* captureVolume = dynamic_cast<ProjectileCaptureVolume*>(contact.actor))
			{
				captureVolume->TryCaptureProjectile(*this);
				if (GetIsPendingDestroy())
				{
					return false;
				}
			}
			if (contact.hasSurfaceNormal &&
				ProjectileReflectionService::TryReflectFromSurface(
					*this,
					*contact.actor,
					{ contact.impactLocation, contact.surfaceNormal }
				))
			{
				SetActorLocation(contact.impactLocation);
				mOutboundTravelDistance += travelledDistance * contact.fraction;
				return false;
			}
			if (contact.actor->GetCollisionLayer() == CollisionLayer::Environment)
			{
				BeginReturning();
				return false;
			}
			if (auto* target = dynamic_cast<SpaceShip*>(contact.actor))
			{
				TryHitShip(*target);
			}
		}
		return true;
	}

	void AegisReaverProjectileActor::TryHitShip(SpaceShip& target)
	{
		SpaceShip* returnOwner = GetReturnOwner();
		if (!mFlightState || &target == returnOwner || !IsValidAbilityTarget(&target) ||
			!mFlightState->TryRegisterTargetHit(target, mPhase == Phase::Returning))
		{
			return;
		}

		// Steal reads the shield before damage so the amount represents the target's
		// current protection, exactly as the design specifies. A Relay only weakens
		// steals earned after the conversion; carried shield was already split.
		const float availableShield = std::max(0.f, target.GetShieldComponent().GetShield());
		const float stolenShield = std::min(
			availableShield,
			availableShield * mShieldStealRatio * mPostPrismShieldStealMultiplier
		);
		target.GetShieldComponent().ChangeShield(-stolenShield);
		mReturnShieldPayload += stolenShield;

		ApplyCombatDamage(
			target,
			GetDamage(),
			GetOwnerActor(),
			GetDamageTags(),
			GetDamagePayload(),
			GetSourceAbilityId(),
			GetSourceAbilityTags(),
			DamageDeliveryType::Projectile,
			this
		);
	}

	void AegisReaverProjectileActor::BeginReturning()
	{
		if (mPhase == Phase::Returning)
		{
			return;
		}
		mPhase = Phase::Returning;
		mReturnElapsed = 0.f;
	}

	void AegisReaverProjectileActor::UpdateVelocityAndRotation(
		const sf::Vector2f& direction,
		float speed
	)
	{
		const sf::Vector2f normalized = NormalizeOrDefault(direction);
		SetVelocity(normalized * std::max(0.f, speed));
		SetActorRotation(std::atan2(normalized.y, normalized.x) * 57.2957795131f + 90.f);
	}

	SpaceShip* AegisReaverProjectileActor::GetReturnOwner() const
	{
		return mFlightState ? mFlightState->GetOwner() :
			dynamic_cast<SpaceShip*>(GetOwnerActor());
	}

	void AegisReaverProjectileActor::ReportFlightTerminal(bool returned)
	{
		if (!mFlightState || !mFlightRegistered || mFlightTerminalReported)
		{
			return;
		}
		mFlightTerminalReported = true;
		if (returned)
		{
			mFlightState->ReportProjectileReturned(mReturnShieldPayload);
		}
		else
		{
			mFlightState->ReportProjectileLost();
		}
	}

	void AegisReaverProjectileActor::OnActorBeginOverlap(Actor* otherActor)
	{
		if (!otherActor || GetIsPendingDestroy())
		{
			return;
		}
		if (auto* captureVolume = dynamic_cast<ProjectileCaptureVolume*>(otherActor))
		{
			captureVolume->TryCaptureProjectile(*this);
		}
	}

	bool AegisReaverProjectileActor::TryReflectProjectile(
		const ProjectileReflectionRequest& request
	)
	{
		if (!CanBeReflected() || GetVectorLength(request.returnDirection) <=
			MinimumDirectionLength)
		{
			return false;
		}

		// A wall changes Aegis' vector, not its allegiance. It must still return
		// shield to the owner that committed it, so this deliberately does not call
		// AbilityWorldActor::ApplyReflectionOwnership.
		SetDamage(std::max(0.f, GetDamage() * request.damageMultiplier));
		UpdateVelocityAndRotation(request.returnDirection, mProjectileSpeed);
		return true;
	}

	weak_ptr<AbilityWorldActor> AegisReaverProjectileActor::SpawnRelayClone(
		const ProjectileRelayCloneRequest& request
	) const
	{
		World* world = GetWorld();
		Actor* owner = GetOwnerActor();
		if (!world || !owner || !mFlightState)
		{
			return {};
		}

		weak_ptr<AegisReaverProjectileActor> clone =
			world->SpawnActor<AegisReaverProjectileActor>(owner, mPresentationProfile);
		if (const shared_ptr<AegisReaverProjectileActor> spawned = clone.lock())
		{
			const int safeCloneCount = std::max(1, request.cloneCount);
			spawned->ConfigureFromAttributes(request.snapshot.damageAttributes);
			spawned->ConfigureRelayClone(request);
			spawned->mFlightState = mFlightState;
			spawned->mFlightState->RegisterProjectile();
			spawned->mFlightRegistered = true;
			// Relay Prism is a new launch point, not a continuation of the source
			// projectile. Every clone receives Aegis' complete outbound distance and
			// its own fresh speed curve before it performs the normal hover/return.
			spawned->mPhase = Phase::Outbound;
			spawned->mOutboundTravelDistance = 0.f;
			spawned->mOutboundElapsed = 0.f;
			spawned->mReturnElapsed = 0.f;
			spawned->mMaximumRangeHoverElapsed = 0.f;
			spawned->mReturnShieldPayload = mReturnShieldPayload /
				static_cast<float>(safeCloneCount);
			spawned->mPostPrismShieldStealMultiplier =
				mPostPrismShieldStealMultiplier * std::clamp(request.transferRatio, 0.f, 1.f);
			// Prism's converted damage is a total payload. Split it across the
			// children so conversion creates coverage and trajectories, not a hidden
			// fourfold Aegis damage multiplier.
			spawned->SetDamage(request.damage / static_cast<float>(safeCloneCount));
			spawned->SetSourceAbilityInstance(GetSourceAbilityInstance());
			if (spawned->mPhase == Phase::Outbound)
			{
				spawned->UpdateVelocityAndRotation(request.direction, spawned->mProjectileSpeed);
			}
			else if (SpaceShip* returnOwner = spawned->GetReturnOwner())
			{
				spawned->UpdateVelocityAndRotation(
					returnOwner->GetActorLocation() - spawned->GetActorLocation(),
					spawned->mReturnSpeed
				);
			}
			spawned->SetLifeTime(0.f);
		}
		return clone;
	}

	void AegisReaverProjectileActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy() || IsInPortalTransit())
		{
			return;
		}
		const AegisReaverVisualDefinition& visual = mPresentationProfile.visual;
		const sf::Vector2f location = GetActorLocation();

		sf::CircleShape glow{ visual.glowRadius };
		glow.setOrigin({ visual.glowRadius, visual.glowRadius });
		glow.setPosition(location);
		glow.setFillColor(visual.glowColor);

		sf::CircleShape disc{ visual.radius, 6 };
		disc.setOrigin({ visual.radius, visual.radius });
		disc.setPosition(location);
		disc.setRotation(sf::degrees(mSpinDegrees));
		disc.setFillColor(visual.coreColor);
		disc.setOutlineColor(visual.edgeColor);
		disc.setOutlineThickness(2.f);

		sf::ConvexShape trail;
		trail.setPointCount(3);
		trail.setPoint(0, { 0.f, 0.f });
		trail.setPoint(1, { -visual.trailLength, visual.trailWidth });
		trail.setPoint(2, { -visual.trailLength, -visual.trailWidth });
		trail.setPosition(location);
		trail.setRotation(sf::degrees(GetActorRotation() - 90.f));
		trail.setFillColor(visual.glowColor);

		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(trail, additive);
		window.draw(glow, additive);
		window.draw(disc);
	}

	bool RegisterAegisReaverProjectileActorType()
	{
		return AbilityActorRegistry::RegisterHandler(
			std::make_unique<AegisReaverProjectileActorType>()
		);
	}
}
