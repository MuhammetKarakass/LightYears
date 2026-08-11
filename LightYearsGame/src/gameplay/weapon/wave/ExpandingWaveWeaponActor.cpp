#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/weapon/wave/ExpandingWaveWeaponActor.h"

#include "framework/World.h"
#include "gameplay/combat/Combatant.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ly
{
	namespace
	{
		constexpr size_t WavePointCount = 15;

		float DistanceSquaredToSegment(
			const sf::Vector2f& point,
			const sf::Vector2f& segmentStart,
			const sf::Vector2f& segmentEnd
		)
		{
			const sf::Vector2f segment = segmentEnd - segmentStart;
			const float lengthSquared =
				segment.x * segment.x + segment.y * segment.y;
			if (lengthSquared <= 0.0001f)
			{
				const sf::Vector2f delta = point - segmentStart;
				return delta.x * delta.x + delta.y * delta.y;
			}

			const sf::Vector2f toPoint = point - segmentStart;
			const float projection = std::clamp(
				(toPoint.x * segment.x + toPoint.y * segment.y) / lengthSquared,
				0.f,
				1.f
			);
			const sf::Vector2f closest = segmentStart + segment * projection;
			const sf::Vector2f delta = point - closest;
			return delta.x * delta.x + delta.y * delta.y;
		}

		sf::Color WithAlpha(const sf::Color& color, float alpha)
		{
			sf::Color result = color;
			result.a = static_cast<std::uint8_t>(
				std::clamp(alpha, 0.f, 1.f) * 255.f
			);
			return result;
		}
	}

	ExpandingWaveWeaponActor::ExpandingWaveWeaponActor(
		World* world,
		Actor* owner,
		const WeaponPresentationDefinition& presentation,
		const sas::GameplayAttributeList& attributes,
		const List<GameplayTag>& damageTags
	)
		: AbilityWorldActor(world, owner)
		, mSpeed(std::max(0.f, sas::FindAttributeValue(
			attributes,
			PrimaryWeaponSchema::Wave::Delivery::Speed,
			0.f
		)))
		, mMaxTravelDistance(std::max(0.f, sas::FindAttributeValue(
			attributes,
			CommonAttributeIds::Range,
			0.f
		)))
		, mInitialWidth(std::max(1.f, sas::FindAttributeValue(
			attributes,
			PrimaryWeaponSchema::Wave::Delivery::InitialWidth,
			1.f
		)))
		, mMaximumWidth(std::max(mInitialWidth, sas::FindAttributeValue(
			attributes,
			PrimaryWeaponSchema::Wave::Delivery::MaximumWidth,
			mInitialWidth
		)))
		, mCurrentWidth(mInitialWidth)
		, mThickness(std::max(1.f, sas::FindAttributeValue(
			attributes,
			PrimaryWeaponSchema::Wave::Delivery::Thickness,
			1.f
		)))
		, mColor(presentation.pointLightDef.color)
	{
		SetRenderLayer(RenderLayer::Projectile);
		SetDamage(std::max(0.f, sas::FindAttributeValue(
			attributes,
			CommonAttributeIds::Damage,
			0.f
		)));
		SetDamageAttributes(attributes);
		SetDamageTags(damageTags);
		SetAbilityPhysicsEnabled(false);
		ConfigureCollisionFromOwner();
		if (mSpeed > 0.f && mMaxTravelDistance > 0.f)
		{
			SetLifeTime(mMaxTravelDistance / mSpeed + 0.25f);
		}
		mWaveStrip.resize(WavePointCount * 2);
		RebuildGeometry();
	}

	void ExpandingWaveWeaponActor::Tick(float deltaTime)
	{
		AbilityWorldActor::Tick(deltaTime);
		if (GetIsPendingDestroy())
		{
			return;
		}

		const float safeDeltaTime = std::max(0.f, deltaTime);
		const sf::Vector2f segmentStart = GetActorLocation();
		const sf::Vector2f movement =
			GetActorForwardDirection() * mSpeed * safeDeltaTime;
		const sf::Vector2f segmentEnd = segmentStart + movement;
		SetVelocity(GetActorForwardDirection() * mSpeed);
		SetActorLocation(segmentEnd);
		mTravelDistance += std::abs(mSpeed) * safeDeltaTime;

		const float progress = mMaxTravelDistance > 0.f
			? std::clamp(mTravelDistance / mMaxTravelDistance, 0.f, 1.f)
			: 1.f;
		mCurrentWidth =
			mInitialWidth + (mMaximumWidth - mInitialWidth) * progress;
		ApplyHits(segmentStart, segmentEnd);
		RebuildGeometry();

		if (mMaxTravelDistance <= 0.f ||
			mTravelDistance >= mMaxTravelDistance)
		{
			Destroy();
		}
	}

	void ExpandingWaveWeaponActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		sf::RenderStates additiveStates;
		additiveStates.blendMode = sf::BlendAdd;
		window.draw(mWaveStrip, additiveStates);
	}

	void ExpandingWaveWeaponActor::ApplyHits(
		const sf::Vector2f& segmentStart,
		const sf::Vector2f& segmentEnd
	)
	{
		World* world = GetWorld();
		const float hitRadius = mCurrentWidth * 0.5f;
		const float hitRadiusSquared = hitRadius * hitRadius;
		if (!world || GetDamage() <= 0.f || hitRadiusSquared <= 0.f)
		{
			return;
		}

		for (const weak_ptr<Actor>& targetWeak :
			world->GetActorsByType<Actor>())
		{
			const shared_ptr<Actor> target = targetWeak.lock();
			if (!target ||
				mHitTargets.find(target.get()) != mHitTargets.end() ||
				!IsValidAbilityTarget(target.get()))
			{
				continue;
			}
			if (DistanceSquaredToSegment(
				target->GetActorLocation(),
				segmentStart,
				segmentEnd
			) > hitRadiusSquared)
			{
				continue;
			}

			mHitTargets.insert(target.get());
			ApplyCombatDamage(
				*target,
				GetDamage(),
				GetOwnerActor(),
				GetDamageTags(),
				GetDamagePayload()
			);
		}
	}

	void ExpandingWaveWeaponActor::RebuildGeometry()
	{
		const sf::Vector2f direction = GetActorForwardDirection();
		const sf::Vector2f right{ -direction.y, direction.x };
		const sf::Vector2f center = GetActorLocation();
		const float halfWidth = mCurrentWidth * 0.5f;
		const float halfThickness = mThickness * 0.5f;
		const float progress = mMaxTravelDistance > 0.f
			? std::clamp(mTravelDistance / mMaxTravelDistance, 0.f, 1.f)
			: 1.f;
		const float fade = 1.f - progress * 0.7f;

		for (size_t index = 0; index < WavePointCount; ++index)
		{
			const float normalized =
				static_cast<float>(index) /
				static_cast<float>(WavePointCount - 1);
			const float lateral = normalized * 2.f - 1.f;
			const float bow = (1.f - lateral * lateral) *
				std::min(42.f, mCurrentWidth * 0.18f);
			const sf::Vector2f ribbonCenter =
				center + right * (lateral * halfWidth) + direction * bow;

			mWaveStrip[index * 2].position =
				ribbonCenter + direction * halfThickness;
			mWaveStrip[index * 2 + 1].position =
				ribbonCenter - direction * halfThickness;
			mWaveStrip[index * 2].color =
				WithAlpha(mColor, 0.75f * fade);
			mWaveStrip[index * 2 + 1].color =
				WithAlpha(sf::Color{ 225, 250, 255, 255 }, 0.28f * fade);
		}
	}
}
