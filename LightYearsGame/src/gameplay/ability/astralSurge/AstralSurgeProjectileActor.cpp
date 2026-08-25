#include "gameplay/ability/astralSurge/AstralSurgeProjectileActor.h"

#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/astralSurge/AstralSurgeContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/projectile/ProjectileCaptureVolume.h"
#include "gameplay/targeting/SweptGeometry.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "framework/World.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>

namespace ly
{
	namespace
	{
		constexpr float MaximumMovementSubstep = 12.f;
		// Astral Surge has no gameplay range. This is only a distant safety
		// boundary so a projectile cannot survive forever due to a numerical or
		// world-state failure; it is intentionally far outside normal gameplay.
		constexpr float TechnicalBoundaryMargin = 100000.f;
		constexpr float RelayDeliveryLifetimeMarginSeconds = 0.05f;

		const List<sas::AttributeId> AstralSurgeProjectileCommonAttributes{
			CommonAttributeIds::Damage,
			CommonAttributeIds::PierceDamageLoss,
			AreaAttributeIds::Width
		};

		const List<sas::AttributeId> AstralSurgeProjectileAttributeRoots{
			AbilityData::AstralSurge::Actor::Projectile::Root,
			DamageAttributeIds::Root
		};

		sf::Vector2f NormalizeOrDefault(const sf::Vector2f& direction)
		{
			const float length = std::sqrt(
				direction.x * direction.x + direction.y * direction.y
			);
			return length > 0.001f
				? direction / length
				: sf::Vector2f{ 0.f, -1.f };
		}

		sf::Color WithAlpha(const sf::Color& color, float multiplier)
		{
			sf::Color result = color;
			result.a = static_cast<std::uint8_t>(std::clamp(
				static_cast<float>(color.a) * std::clamp(multiplier, 0.f, 1.f),
				0.f,
				255.f
			));
			return result;
		}

		class AstralSurgeProjectileActorType final : public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::AstralSurgeProjectile;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return AstralSurgeProjectileAttributeRoots;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return AstralSurgeProjectileCommonAttributes;
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
					CommonAttributeIds::Damage,
					CommonAttributeIds::PierceDamageLoss,
					AreaAttributeIds::Width,
					AbilityData::AstralSurge::Actor::Projectile::ProjectileSpeed,
					AbilityData::AstralSurge::Actor::Projectile::MinimumDamageMultiplier
				})
				{
					if (!sas::FindAttribute(definition.attributes, required))
					{
						return {
							false,
							"Astral Surge projectile is missing required attribute '" +
								std::string{ required.GetName() } + "'."
						};
					}
				}

				const float speed = sas::FindAttributeValue(
					definition.attributes,
					AbilityData::AstralSurge::Actor::Projectile::ProjectileSpeed
				);
				const float width = sas::FindAttributeValue(
					definition.attributes,
					AreaAttributeIds::Width
				);
				const float pierceLoss = sas::FindAttributeValue(
					definition.attributes,
					CommonAttributeIds::PierceDamageLoss
				);
				const float minimumMultiplier = sas::FindAttributeValue(
					definition.attributes,
					AbilityData::AstralSurge::Actor::Projectile::MinimumDamageMultiplier
				);
				if (speed <= 0.f || width <= 0.f || pierceLoss < 0.f ||
					pierceLoss >= 1.f || minimumMultiplier < 0.f ||
					minimumMultiplier > 1.f)
				{
					return {
						false,
						"Astral Surge projectile requires positive speed/width, PierceDamageLoss in [0, 1), and a minimum damage multiplier in [0, 1]."
					};
				}
				if (!definition.presentationProfileId.IsValid() ||
					PresentationProfileRegistry<AstralSurgePresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					) == nullptr)
				{
					return { false, "Astral Surge projectile requires a registered presentation profile." };
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const AstralSurgePresentationProfile* profile =
					PresentationProfileRegistry<AstralSurgePresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<AstralSurgeProjectileActor>(
						&context.owner,
						*profile
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	AstralSurgeProjectileActor::AstralSurgeProjectileActor(
		World* world,
		Actor* owner,
		const AstralSurgePresentationProfile& presentationProfile
	)
		: AbilityWorldActor(world, owner),
		mPresentationProfile{ presentationProfile }
	{
		SetRenderLayer(RenderLayer::Projectile);
	}

	void AstralSurgeProjectileActor::BeginPlay()
	{
		AbilityWorldActor::BeginPlay();
		SetVelocity(mLaunchVelocity);
	}

	void AstralSurgeProjectileActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mProjectileSpeed = std::max(
			0.f,
			sas::FindAttributeValue(
				attributes,
				AbilityData::AstralSurge::Actor::Projectile::ProjectileSpeed,
				mProjectileSpeed
			)
		);
		mProjectileWidth = std::max(
			0.f,
			sas::FindAttributeValue(
				attributes,
				AreaAttributeIds::Width,
				mProjectileWidth
			)
		);
		mPierceDamageLoss = std::clamp(
			sas::FindAttributeValue(
				attributes,
				CommonAttributeIds::PierceDamageLoss,
				mPierceDamageLoss
			),
			0.f,
			0.999f
		);
		mMinimumDamageMultiplier = std::clamp(
			sas::FindAttributeValue(
				attributes,
				AbilityData::AstralSurge::Actor::Projectile::MinimumDamageMultiplier,
				mMinimumDamageMultiplier
			),
			0.f,
			1.f
		);
		// The wide hit band is evaluated explicitly in ApplySweptHits. Keep the
		// physics shape tiny: it must still touch portal/relay capture volumes,
		// but must never turn the launch point into a large circular area hit.
		SetAbilityCollisionRadius(1.f);
	}

	void AstralSurgeProjectileActor::Tick(float deltaTime)
	{
		if (IsInPortalTransit())
		{
			AbilityWorldActor::Tick(deltaTime);
			return;
		}

		const float safeDeltaTime = std::max(0.f, deltaTime);
		for (ImpactPulse& pulse : mImpactPulses)
		{
			pulse.age += safeDeltaTime;
		}
		mImpactPulses.erase(
			std::remove_if(
				mImpactPulses.begin(),
				mImpactPulses.end(),
				[&](const ImpactPulse& pulse)
				{
					return pulse.age >= mPresentationProfile.projectile.impactDuration;
				}
			),
			mImpactPulses.end()
		);

		if (!GetIsPendingDestroy())
		{
			Move(safeDeltaTime);
			if (IsOutsideTechnicalBounds())
			{
				Destroy();
			}
		}
		AbilityWorldActor::Tick(safeDeltaTime);
	}

	void AstralSurgeProjectileActor::Move(float deltaTime)
	{
		if (mProjectileSpeed <= 0.f || deltaTime <= 0.f)
		{
			return;
		}

		if (GetVectorLength(mLaunchVelocity) <= 0.001f)
		{
			mLaunchVelocity = NormalizeOrDefault(GetActorForwardDirection()) * mProjectileSpeed;
		}
		SetVelocity(mLaunchVelocity);

		const float distance = mProjectileSpeed * deltaTime;
		const int substeps = std::clamp(
			static_cast<int>(std::ceil(distance / MaximumMovementSubstep)),
			1,
			32
		);
		const float stepTime = deltaTime / static_cast<float>(substeps);
		for (int step = 0; step < substeps && !GetIsPendingDestroy(); ++step)
		{
			const sf::Vector2f previousLocation = GetActorLocation();
			AddActorLocationOffset(mLaunchVelocity * stepTime);
			ApplySweptHits(previousLocation, GetActorLocation());
		}
	}

	void AstralSurgeProjectileActor::ApplySweptHits(
		const sf::Vector2f& startLocation,
		const sf::Vector2f& endLocation
	)
	{
		World* world = GetWorld();
		if (!world || GetIsPendingDestroy())
		{
			return;
		}

		// Area.Width is the full transverse width of this forward-moving wave.
		// Swept geometry owns broad combat detection; physical collision remains
		// intentionally narrow for generic projectile interactions.
		const float collisionRadius = std::max(0.f, mProjectileWidth * 0.5f);
		for (const weak_ptr<Actor>& actorWeak : world->GetActorsInBounds(
			targeting::swept::SegmentBounds(
				startLocation,
				endLocation,
				collisionRadius
			)
		))
		{
			const shared_ptr<Actor> candidate = actorWeak.lock();
			if (!candidate || candidate.get() == this)
			{
				continue;
			}

			if (auto* captureVolume = dynamic_cast<ProjectileCaptureVolume*>(candidate.get());
				captureVolume &&
				targeting::swept::SegmentIntersectsExpandedBounds(
					startLocation,
					endLocation,
					candidate->GetActorGlobalBounds(),
					collisionRadius
				) && captureVolume->TryCaptureProjectile(*this))
			{
				return;
			}

			if (!IsValidAbilityTarget(candidate.get()) ||
				!targeting::swept::SegmentIntersectsExpandedBounds(
					startLocation,
					endLocation,
					candidate->GetActorGlobalBounds(),
					collisionRadius
				))
			{
				continue;
			}
			TryHitTarget(candidate.get());
		}
	}

	void AstralSurgeProjectileActor::ApplyPiercingHit(Actor& target)
	{
		// The spec uses a linear multiplier by hit index, not exponential decay:
		// 1.00, 0.95, 0.90 ... with a configurable floor at 0.40.
		const std::size_t hitIndex = mHitTargets.empty() ? 0 : mHitTargets.size() - 1;
		const float multiplier = std::max(
			mMinimumDamageMultiplier,
			1.f - static_cast<float>(hitIndex) * mPierceDamageLoss
		);
		const float damage = GetDamage() * multiplier;
		ApplyCombatDamage(
			target,
			damage,
			GetOwnerActor(),
			GetDamageTags(),
			GetDamagePayload(),
			GetSourceAbilityId(),
			GetSourceAbilityTags(),
			DamageDeliveryType::Projectile,
			this
		);
		mImpactPulses.push_back(ImpactPulse{ target.GetActorLocation(), 0.f });
	}

	void AstralSurgeProjectileActor::OnActorBeginOverlap(Actor* otherActor)
	{
		if (GetIsPendingDestroy() || IsInPortalTransit())
		{
			return;
		}
		if (TryReflectOnOverlap(otherActor))
		{
			return;
		}
		if (auto* captureVolume = dynamic_cast<ProjectileCaptureVolume*>(otherActor);
			captureVolume && captureVolume->TryCaptureProjectile(*this))
		{
			return;
		}
		AbilityWorldActor::OnActorBeginOverlap(otherActor);
		TryHitTarget(otherActor);
	}

	bool AstralSurgeProjectileActor::TryReflectProjectile(
		const ProjectileReflectionRequest& request
	)
	{
		return ApplyBallisticReflection(request, mProjectileSpeed, mLaunchVelocity);
	}

	bool AstralSurgeProjectileActor::TryHitTarget(Actor* otherActor)
	{
		if (GetIsPendingDestroy() || !otherActor ||
			!IsValidAbilityTarget(otherActor) ||
			!mHitTargets.insert(otherActor).second)
		{
			return false;
		}
		ApplyPiercingHit(*otherActor);
		return true;
	}

	bool AstralSurgeProjectileActor::IsOutsideTechnicalBounds() const
	{
		World* world = GetWorld();
		if (!world || !world->GetApplication())
		{
			// Isolated gameplay tests may use a World without an Application. Such
			// worlds have no screen boundary to query, so only the live game applies
			// this technical cleanup rule.
			return false;
		}
		const sf::Vector2u windowSize = world->GetWindowSize();
		const sf::Vector2f location = GetActorLocation();
		return location.x < -TechnicalBoundaryMargin ||
			location.y < -TechnicalBoundaryMargin ||
			location.x > static_cast<float>(windowSize.x) + TechnicalBoundaryMargin ||
			location.y > static_cast<float>(windowSize.y) + TechnicalBoundaryMargin;
	}

	void AstralSurgeProjectileActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}
		AbilityWorldActor::Render(window);
		DrawFlight(window);
		DrawImpacts(window);
	}

	void AstralSurgeProjectileActor::DrawFlight(sf::RenderWindow& window) const
	{
		const AstralSurgeProjectileVisualDefinition& visual =
			mPresentationProfile.projectile;
		const sf::Vector2f location = GetActorLocation();
		const sf::Vector2f forward = NormalizeOrDefault(mLaunchVelocity);
		const sf::Vector2f side{ -forward.y, forward.x };
		const float pulse = 0.82f + 0.18f * std::sin(
			GetAge() * std::max(0.f, visual.pulseSpeed)
		);

		sf::RenderStates additiveStates;
		additiveStates.blendMode = sf::BlendAdd;

		// This is a semi-annular mesh, not a stretched sprite: Astral Surge is a
		// broad crescent wave travelling forward, with no missile body or trail.
		constexpr int ArcSegments = 20;
		const auto drawCrescent = [&](float outerRadius, float thickness, const sf::Color& color)
		{
			const float safeOuterRadius = std::max(1.f, outerRadius);
			const float innerRadius = std::max(0.f, safeOuterRadius - std::max(1.f, thickness));
			const float halfArcRadians = std::clamp(
				visual.crescentArcDegrees,
				10.f,
				180.f
			) * 0.5f * 0.0174532925199f;
			sf::VertexArray mesh{ sf::PrimitiveType::Triangles };
			for (int segment = 0; segment < ArcSegments; ++segment)
			{
				const float startRatio = static_cast<float>(segment) / ArcSegments;
				const float endRatio = static_cast<float>(segment + 1) / ArcSegments;
				const auto pointOnArc = [&](float angle, float radius)
				{
					return location + forward * (std::cos(angle) * radius) +
						side * (std::sin(angle) * radius);
				};
				const float startAngle = -halfArcRadians + startRatio * halfArcRadians * 2.f;
				const float endAngle = -halfArcRadians + endRatio * halfArcRadians * 2.f;
				const sf::Vector2f outerStart = pointOnArc(startAngle, safeOuterRadius);
				const sf::Vector2f outerEnd = pointOnArc(endAngle, safeOuterRadius);
				const sf::Vector2f innerStart = pointOnArc(startAngle, innerRadius);
				const sf::Vector2f innerEnd = pointOnArc(endAngle, innerRadius);
				mesh.append(sf::Vertex{ outerStart, color });
				mesh.append(sf::Vertex{ outerEnd, color });
				mesh.append(sf::Vertex{ innerEnd, color });
				mesh.append(sf::Vertex{ outerStart, color });
				mesh.append(sf::Vertex{ innerEnd, color });
				mesh.append(sf::Vertex{ innerStart, color });
			}
			window.draw(mesh, additiveStates);
		};

		drawCrescent(
			visual.crescentRadius + visual.glowThickness * 0.35f,
			visual.glowThickness,
			WithAlpha(visual.glowColor, pulse * 0.65f)
		);
		drawCrescent(
			visual.crescentRadius,
			visual.crescentThickness,
			WithAlpha(visual.outerColor, pulse)
		);
		drawCrescent(
			visual.crescentRadius - visual.crescentThickness * 0.18f,
			std::max(2.f, visual.crescentThickness * 0.28f),
			visual.coreColor
		);
	}

	void AstralSurgeProjectileActor::DrawImpacts(sf::RenderWindow& window) const
	{
		const AstralSurgeProjectileVisualDefinition& visual =
			mPresentationProfile.projectile;
		const float duration = std::max(0.001f, visual.impactDuration);
		sf::RenderStates additiveStates;
		additiveStates.blendMode = sf::BlendAdd;
		for (const ImpactPulse& pulse : mImpactPulses)
		{
			const float progress = std::clamp(pulse.age / duration, 0.f, 1.f);
			const float radius = std::max(1.f, visual.impactRadius);
			sf::CircleShape flash(radius, 32);
			flash.setOrigin({ radius, radius });
			flash.setPosition(pulse.location);
			flash.setScale({ 0.35f + progress * 0.65f, 0.35f + progress * 0.65f });
			flash.setFillColor(WithAlpha(visual.impactColor, 1.f - progress));
			window.draw(flash, additiveStates);
		}
	}

	weak_ptr<AbilityWorldActor> AstralSurgeProjectileActor::SpawnRelayClone(
		const ProjectileRelayCloneRequest& request
	) const
	{
		World* world = GetWorld();
		Actor* owner = GetOwnerActor();
		if (!world || !owner)
		{
			return {};
		}

		weak_ptr<AstralSurgeProjectileActor> clone =
			world->SpawnActor<AstralSurgeProjectileActor>(
				owner,
				mPresentationProfile
			);
		if (const shared_ptr<AstralSurgeProjectileActor> spawned = clone.lock())
		{
			spawned->ConfigureFromAttributes(request.snapshot.damageAttributes);
			spawned->ConfigureRelayClone(request);
			spawned->SetDamage(request.damage);
			spawned->SetLifeTime(request.snapshot.remainingLifetime);
			spawned->mLaunchVelocity = NormalizeOrDefault(request.direction) *
				spawned->mProjectileSpeed;
			spawned->SetVelocity(spawned->mLaunchVelocity);
		}
		return clone;
	}

	bool RegisterAstralSurgeProjectileActorType()
	{
		return AbilityActorRegistry::RegisterHandler(
			std::make_unique<AstralSurgeProjectileActorType>()
		);
	}
}
