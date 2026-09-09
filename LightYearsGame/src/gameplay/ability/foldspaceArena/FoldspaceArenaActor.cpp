#include "gameplay/ability/foldspaceArena/FoldspaceArenaActor.h"

#include "attributes/AttributeSystem.h"
#include "framework/World.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/foldspaceArena/FoldspaceArenaContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace ly
{
	namespace
	{
		constexpr float Epsilon = 0.001f;
		constexpr int ArcSegments = 10;

		const List<sas::AttributeId> ArenaCommonAttributes{
			CommonAttributeIds::Duration,
			AreaAttributeIds::Width,
			AreaAttributeIds::Length
		};
		const List<sas::AttributeId> ArenaOwnedAttributeRoots{
			AbilityData::FoldspaceArena::Actor::Arena::AttributeRoot
		};

		float FindValue(
			const sas::GameplayAttributeList& values,
			const sas::AttributeId& id,
			float fallback
		)
		{
			return sas::FindAttributeValue(values, id, fallback);
		}

		class FoldspaceArenaActorTypeHandler final : public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::FoldspaceArena;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return ArenaCommonAttributes;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return ArenaOwnedAttributeRoots;
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
				const sas::GameplayAttribute* duration = sas::FindAttribute(
					definition.attributes, CommonAttributeIds::Duration
				);
				const auto hasPositive = [&definition](const sas::AttributeId& id)
				{
					const sas::GameplayAttribute* attribute = sas::FindAttribute(
						definition.attributes, id
					);
					return attribute && std::isfinite(attribute->baseValue) &&
						attribute->baseValue > 0.f;
				};
				if (!duration || !std::isfinite(duration->baseValue) ||
					duration->baseValue <= 0.f || !definition.presentationProfileId.IsValid() ||
					!hasPositive(AbilityData::FoldspaceArena::Actor::Arena::Width) ||
					!hasPositive(AbilityData::FoldspaceArena::Actor::Arena::Height) ||
					!hasPositive(AbilityData::FoldspaceArena::Actor::Arena::ProjectileSpeed) ||
					!hasPositive(AbilityData::FoldspaceArena::Actor::Arena::CornerRadius) ||
					!hasPositive(AbilityData::FoldspaceArena::Actor::Arena::WrapInwardOffset) ||
					!PresentationProfileRegistry<FoldspaceArenaPresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					))
				{
					return {
						false,
						"Foldspace Arena requires complete positive geometry and a registered typed presentation profile."
					};
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const auto* profile = PresentationProfileRegistry<
					FoldspaceArenaPresentationProfile
				>::Find(context.definition.presentationProfileId.ToString());
				return world && profile
					? world->SpawnActor<FoldspaceArenaActor>(&context.owner, *profile)
					: weak_ptr<AbilityWorldActor>{};
			}
		};

		void AppendArc(
			std::vector<sf::Vector2f>& points,
			const sf::Vector2f& center,
			float radius,
			float startAngleRadians,
			float endAngleRadians
		)
		{
			for (int index = 0; index <= ArcSegments; ++index)
			{
				const float alpha = static_cast<float>(index) /
					static_cast<float>(ArcSegments);
				const float angle = startAngleRadians +
					(endAngleRadians - startAngleRadians) * alpha;
				points.push_back(center + sf::Vector2f{
					std::cos(angle) * radius,
					std::sin(angle) * radius
				});
			}
		}
	}

	FoldspaceArenaActor::FoldspaceArenaActor(
		World* world,
		Actor* owner,
		const FoldspaceArenaPresentationProfile& presentationProfile
	)
		: AbilityWorldActor(world, owner)
		, mPresentationProfile(presentationProfile)
	{
		SetRenderLayer(RenderLayer::WorldVfx);
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
	}

	void FoldspaceArenaActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		SetLifeTime(std::max(
			0.01f,
			FindValue(attributes, CommonAttributeIds::Duration, GetLifeTime())
		));
		mProjectileSpeed = std::max(1.f, FindValue(
			attributes,
			AbilityData::FoldspaceArena::Actor::Arena::ProjectileSpeed,
			mProjectileSpeed
		));
		mArenaWidth = std::max(1.f, FindValue(
			attributes,
			AbilityData::FoldspaceArena::Actor::Arena::Width,
			mArenaWidth
		));
		mArenaHeight = std::max(1.f, FindValue(
			attributes,
			AbilityData::FoldspaceArena::Actor::Arena::Height,
			mArenaHeight
		));
		mCornerRadius = std::clamp(FindValue(
			attributes,
			AbilityData::FoldspaceArena::Actor::Arena::CornerRadius,
			mCornerRadius
		), 0.f, std::min(mArenaWidth, mArenaHeight) * 0.5f);
		mWrapInwardOffset = std::max(0.f, FindValue(
			attributes,
			AbilityData::FoldspaceArena::Actor::Arena::WrapInwardOffset,
			mWrapInwardOffset
		));
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
	}

	void FoldspaceArenaActor::ConfigureFromAbilityValues(
		const sas::GameplayAttributeList& values,
		float ownerEnergyMax
	)
	{
		SetDamage(std::max(0.f, FindValue(
			values, CommonAttributeIds::Damage, GetDamage()
		)));
		const float baseDuration = std::max(0.01f, FindValue(
			values, AbilityData::FoldspaceArena::Attribute::BaseArenaDuration, 6.f
		));
		const float energyReference = std::max(0.f, FindValue(
			values,
			AbilityData::FoldspaceArena::Attribute::EnergyMaxDurationReference,
			50.f
		));
		const float durationPerEnergy = std::max(0.f, FindValue(
			values,
			AbilityData::FoldspaceArena::Attribute::EnergyMaxDurationPerPoint,
			0.002f
		));
		mArenaDuration = baseDuration + std::max(0.f, ownerEnergyMax - energyReference) *
			durationPerEnergy;
		mPhase = Phase::Travelling;
		mPhaseElapsed = 0.f;
		mVisualAge = 0.f;
		mHasPreviousOwnerLocation = false;
	}

	void FoldspaceArenaActor::SetSnapshotTarget(const sf::Vector2f& targetLocation)
	{
		mTargetLocation = targetLocation;
		mTargetConfigured = true;
	}

	void FoldspaceArenaActor::Tick(float deltaTime)
	{
		AbilityWorldActor::Tick(deltaTime);
		if (GetIsPendingDestroy() || !mTargetConfigured)
		{
			return;
		}

		const float safeDeltaTime = std::max(0.f, deltaTime);
		mVisualAge += safeDeltaTime;
		mPhaseElapsed += safeDeltaTime;
		if (mPhase == Phase::Travelling)
		{
			const sf::Vector2f delta = mTargetLocation - GetActorLocation();
			const float distance = GetVectorLength(delta);
			const float step = mProjectileSpeed * safeDeltaTime;
			if (distance <= std::max(Epsilon, step))
			{
				SetActorLocation(mTargetLocation);
				BeginArena();
			}
			else
			{
				SetActorLocation(GetActorLocation() + delta / distance * step);
			}
			return;
		}

		UpdateOwnerBoundary(safeDeltaTime);
		if (!GetIsPendingDestroy() && mPhaseElapsed >= mArenaDuration)
		{
			Destroy();
		}
	}

	bool FoldspaceArenaActor::IsInsideArena(const sf::Vector2f& location) const
	{
		const float halfWidth = mArenaWidth * 0.5f;
		const float halfHeight = mArenaHeight * 0.5f;
		const float innerHalfWidth = std::max(0.f, halfWidth - mCornerRadius);
		const float innerHalfHeight = std::max(0.f, halfHeight - mCornerRadius);
		const sf::Vector2f local = location - mTargetLocation;
		const float closestX = std::clamp(local.x, -innerHalfWidth, innerHalfWidth);
		const float closestY = std::clamp(local.y, -innerHalfHeight, innerHalfHeight);
		const float deltaX = local.x - closestX;
		const float deltaY = local.y - closestY;
		return deltaX * deltaX + deltaY * deltaY <=
			mCornerRadius * mCornerRadius + Epsilon;
	}

	sf::Vector2f FoldspaceArenaActor::FindBoundaryCrossing(
		const sf::Vector2f& inside,
		const sf::Vector2f& outside
	) const
	{
		// A short binary search gives the true curved-boundary exit point even at
		// high speed; using the final outside position would distort corner wraps.
		sf::Vector2f lower = inside;
		sf::Vector2f upper = outside;
		for (int iteration = 0; iteration < 16; ++iteration)
		{
			const sf::Vector2f middle = (lower + upper) * 0.5f;
			if (IsInsideArena(middle))
			{
				lower = middle;
			}
			else
			{
				upper = middle;
			}
		}
		return lower;
	}

	sf::Vector2f FoldspaceArenaActor::ResolveOutwardNormal(
		const sf::Vector2f& boundaryPoint
	) const
	{
		const float halfWidth = mArenaWidth * 0.5f;
		const float halfHeight = mArenaHeight * 0.5f;
		const float innerHalfWidth = std::max(0.f, halfWidth - mCornerRadius);
		const float innerHalfHeight = std::max(0.f, halfHeight - mCornerRadius);
		const sf::Vector2f local = boundaryPoint - mTargetLocation;
		const sf::Vector2f closest{
			std::clamp(local.x, -innerHalfWidth, innerHalfWidth),
			std::clamp(local.y, -innerHalfHeight, innerHalfHeight)
		};
		sf::Vector2f normal = local - closest;
		const float normalLength = GetVectorLength(normal);
		if (normalLength > Epsilon)
		{
			return normal / normalLength;
		}
		return std::abs(local.x) >= std::abs(local.y)
			? sf::Vector2f{ local.x >= 0.f ? 1.f : -1.f, 0.f }
			: sf::Vector2f{ 0.f, local.y >= 0.f ? 1.f : -1.f };
	}

	bool FoldspaceArenaActor::IsExternalRelocation(
		const Actor& owner,
		const sf::Vector2f& previousLocation,
		float deltaTime
	) const
	{
		const float actualDistance = GetVectorLength(
			owner.GetActorLocation() - previousLocation
		);
		const float expectedMovement = GetVectorLength(owner.GetVelocity()) *
			std::max(0.f, deltaTime);
		// Normal drift, dash and force movement are velocity-driven. A position
		// jump larger than that envelope is a teleport and must collapse the arena
		// instead of being converted into a wrap.
		return actualDistance > expectedMovement + 48.f;
	}

	void FoldspaceArenaActor::BeginArena()
	{
		mPhase = Phase::Active;
		mPhaseElapsed = 0.f;
		if (Actor* owner = GetOwnerActor())
		{
			mPreviousOwnerLocation = owner->GetActorLocation();
			mHasPreviousOwnerLocation = true;
		}
		ApplyFormationDamage();
	}

	void FoldspaceArenaActor::ApplyFormationDamage()
	{
		World* world = GetWorld();
		Actor* owner = GetOwnerActor();
		if (!world || !owner || GetDamage() <= 0.f)
		{
			return;
		}
		const float broadPhaseRadius = std::sqrt(
			mArenaWidth * mArenaWidth + mArenaHeight * mArenaHeight
		) * 0.5f;
		for (const shared_ptr<Actor>& candidate : targeting::FindOpposingCombatants(
			*world, *owner, mTargetLocation, broadPhaseRadius
		))
		{
			if (!candidate || candidate->GetIsPendingDestroy() ||
				!IsInsideArena(candidate->GetActorLocation()))
			{
				continue;
			}
			ApplyCombatDamage(
				*candidate,
				GetDamage(),
				owner,
				GetDamageTags(),
				GetDamagePayload(),
				GetSourceAbilityId(),
				GetSourceAbilityTags(),
				DamageDeliveryType::Area,
				this
			);
		}
	}

	void FoldspaceArenaActor::UpdateOwnerBoundary(float deltaTime)
	{
		Actor* owner = GetOwnerActor();
		if (!owner || owner->GetIsPendingDestroy())
		{
			Destroy();
			return;
		}
		const sf::Vector2f currentLocation = owner->GetActorLocation();
		if (!mHasPreviousOwnerLocation)
		{
			mPreviousOwnerLocation = currentLocation;
			mHasPreviousOwnerLocation = true;
			return;
		}
		if (IsInsideArena(currentLocation))
		{
			mPreviousOwnerLocation = currentLocation;
			return;
		}
		if (IsExternalRelocation(*owner, mPreviousOwnerLocation, deltaTime))
		{
			Destroy();
			return;
		}

		const sf::Vector2f exitPoint = FindBoundaryCrossing(
			mPreviousOwnerLocation, currentLocation
		);
		const sf::Vector2f exitNormal = ResolveOutwardNormal(exitPoint);
		const sf::Vector2f oppositeBoundary = mTargetLocation * 2.f - exitPoint;
		const sf::Vector2f oppositeOutwardNormal{
			-exitNormal.x,
			-exitNormal.y
		};
		const sf::FloatRect ownerBounds = owner->GetActorGlobalBounds();
		const float ownerRadius = std::max(
			0.f,
			std::min(ownerBounds.size.x, ownerBounds.size.y) * 0.5f
		);
		const sf::Vector2f wrappedLocation = oppositeBoundary - oppositeOutwardNormal *
			(ownerRadius + mWrapInwardOffset);
		owner->SetActorLocation(wrappedLocation);
		// Velocity, rotation, aim and movement inputs are intentionally untouched.
		mPreviousOwnerLocation = wrappedLocation;
	}

	void FoldspaceArenaActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}
		if (mPhase == Phase::Travelling)
		{
			const float pulse = 0.85f + 0.15f * std::sin(mVisualAge * 9.f);
			sf::CircleShape glow(mPresentationProfile.projectileRadius * 2.f, 24);
			glow.setOrigin({ glow.getRadius(), glow.getRadius() });
			glow.setPosition(GetActorLocation());
			glow.setFillColor(mPresentationProfile.projectileGlowColor);
			glow.setScale({ pulse, pulse });
			sf::CircleShape core(mPresentationProfile.projectileRadius, 20);
			core.setOrigin({ core.getRadius(), core.getRadius() });
			core.setPosition(GetActorLocation());
			core.setFillColor(mPresentationProfile.projectileCoreColor);
			sf::RenderStates additive;
			additive.blendMode = sf::BlendAdd;
			window.draw(glow, additive);
			window.draw(core, additive);
			return;
		}
		RenderArenaBoundary(window);
		RenderOppositeGhost(window);
	}

	void FoldspaceArenaActor::RenderArenaBoundary(sf::RenderWindow& window) const
	{
		const float halfWidth = mArenaWidth * 0.5f;
		const float halfHeight = mArenaHeight * 0.5f;
		const float radius = mCornerRadius;
		const float innerHalfWidth = std::max(0.f, halfWidth - radius);
		const float innerHalfHeight = std::max(0.f, halfHeight - radius);
		std::vector<sf::Vector2f> points;
		points.reserve(ArcSegments * 4 + 8);
		const float pi = 3.1415926535f;
		AppendArc(points, mTargetLocation + sf::Vector2f{ innerHalfWidth, -innerHalfHeight },
			radius, -pi * 0.5f, 0.f);
		AppendArc(points, mTargetLocation + sf::Vector2f{ innerHalfWidth, innerHalfHeight },
			radius, 0.f, pi * 0.5f);
		AppendArc(points, mTargetLocation + sf::Vector2f{ -innerHalfWidth, innerHalfHeight },
			radius, pi * 0.5f, pi);
		AppendArc(points, mTargetLocation + sf::Vector2f{ -innerHalfWidth, -innerHalfHeight },
			radius, pi, pi * 1.5f);
		if (!points.empty())
		{
			points.push_back(points.front());
		}

		const float pulse = 1.f + mPresentationProfile.boundaryPulseAmount *
			std::sin(mVisualAge * mPresentationProfile.boundaryPulseSpeed);
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		for (std::size_t index = 1; index < points.size(); ++index)
		{
			const sf::Vector2f delta = points[index] - points[index - 1];
			const float length = GetVectorLength(delta);
			if (length <= Epsilon)
			{
				continue;
			}
			const float angle = std::atan2(delta.y, delta.x) * 180.f / pi;
			sf::RectangleShape glow({ length, mPresentationProfile.boundaryThickness * 3.f });
			glow.setOrigin({ 0.f, glow.getSize().y * 0.5f });
			glow.setPosition(points[index - 1]);
			glow.setRotation(sf::degrees(angle));
			glow.setFillColor(mPresentationProfile.boundaryGlowColor);
			glow.setScale({ 1.f, pulse });
			window.draw(glow, additive);
			sf::RectangleShape line({ length, mPresentationProfile.boundaryThickness });
			line.setOrigin({ 0.f, line.getSize().y * 0.5f });
			line.setPosition(points[index - 1]);
			line.setRotation(sf::degrees(angle));
			line.setFillColor(mPresentationProfile.boundaryColor);
			window.draw(line, additive);
		}
	}

	void FoldspaceArenaActor::RenderOppositeGhost(sf::RenderWindow& window) const
	{
		const Actor* owner = GetOwnerActor();
		if (!owner || !IsInsideArena(owner->GetActorLocation()))
		{
			return;
		}
		const sf::Vector2f local = owner->GetActorLocation() - mTargetLocation;
		const float edgeDistance = std::min(
			mArenaWidth * 0.5f - std::abs(local.x),
			mArenaHeight * 0.5f - std::abs(local.y)
		);
		if (edgeDistance > 90.f)
		{
			return;
		}
		sf::CircleShape ghost(18.f, 20);
		ghost.setOrigin({ 18.f, 18.f });
		ghost.setPosition(mTargetLocation * 2.f - owner->GetActorLocation());
		ghost.setFillColor(mPresentationProfile.ghostColor);
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(ghost, additive);
	}

	bool RegisterFoldspaceArenaActorType()
	{
		return AbilityActorRegistry::RegisterHandler(
			std::make_unique<FoldspaceArenaActorTypeHandler>()
		);
	}
}
