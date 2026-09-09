#include "gameplay/ability/blastback/BlastbackBurstActor.h"

#include "gameplay/combat/Combatant.h"
#include "gameplay/tags/GameplayTags.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		constexpr float Pi = 3.14159265358979323846f;

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

	}

	BlastbackBurstActor::BlastbackBurstActor(
		World* world,
		Actor* owner,
		const sf::Vector2f& origin,
		const sf::Vector2f& forward,
		const BlastbackPresentationProfile& profile,
		float recoilLockDuration
	)
		: Actor{ world }
		, mOwner{ owner }
		, mProfile{ profile }
		, mOrigin{ origin }
		, mForward{ forward }
		, mRecoilLockDuration{ std::max(0.01f, recoilLockDuration) }
	{
		SetRenderLayer(RenderLayer::GroundDecal);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		SetEnablePhysics(false);
		SetActorLocation(origin);
		BuildGeometry(1.f);
	}

	void BlastbackBurstActor::Tick(float deltaTime)
	{
		if (!mOwner || mOwner->GetIsPendingDestroy())
		{
			Destroy();
			return;
		}

		mAge += std::max(0.f, deltaTime);
		const float visualProgress = std::clamp(
			mAge / std::max(0.01f, mProfile.burstLifetime),
			0.f,
			1.f
		);
		BuildGeometry(1.f - visualProgress);
		if (mAge >= mRecoilLockDuration)
		{
			Destroy();
			return;
		}
		Actor::Tick(deltaTime);
	}

	void BlastbackBurstActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(mGeometry, additive);
	}

	void BlastbackBurstActor::Destroy()
	{
		ReleaseOwnerState();
		Actor::Destroy();
	}

	void BlastbackBurstActor::BuildGeometry(float alpha)
	{
		mGeometry.clear();
		const float forwardAngle = std::atan2(mForward.y, mForward.x);
		const float halfAngle = mProfile.halfAngleDegrees * Pi / 180.f;
		const float range = std::max(1.f, mProfile.range) * (0.85f + 0.15f * alpha);
		const sf::Color color = WithAlpha(mProfile.burstColor, alpha);
		const float innerRadius = std::min(
			std::max(0.f, mProfile.emitterInnerRadius),
			range
		);
		constexpr int ArcSegments = 20;
		for (int index = 0; index < ArcSegments; ++index)
		{
			const float startRatio = static_cast<float>(index) / ArcSegments;
			const float endRatio = static_cast<float>(index + 1) / ArcSegments;
			const float startAngle = forwardAngle - halfAngle + startRatio * halfAngle * 2.f;
			const float endAngle = forwardAngle - halfAngle + endRatio * halfAngle * 2.f;
			const sf::Vector2f innerStart =
				mOrigin + sf::Vector2f{ std::cos(startAngle) * innerRadius, std::sin(startAngle) * innerRadius };
			const sf::Vector2f innerEnd =
				mOrigin + sf::Vector2f{ std::cos(endAngle) * innerRadius, std::sin(endAngle) * innerRadius };
			const sf::Vector2f outerStart =
				mOrigin + sf::Vector2f{ std::cos(startAngle) * range, std::sin(startAngle) * range };
			const sf::Vector2f outerEnd =
				mOrigin + sf::Vector2f{ std::cos(endAngle) * range, std::sin(endAngle) * range };
			// Each strip connects the fixed emitter arc to the outer arc, producing
			// the annular-sector silhouette instead of a pointed cone.
			mGeometry.append(sf::Vertex{ outerStart, color });
			mGeometry.append(sf::Vertex{ outerEnd, color });
			mGeometry.append(sf::Vertex{ innerEnd, color });
			mGeometry.append(sf::Vertex{ outerStart, color });
			mGeometry.append(sf::Vertex{ innerEnd, color });
			mGeometry.append(sf::Vertex{ innerStart, color });
		}
	}

	void BlastbackBurstActor::ReleaseOwnerState()
	{
		if (mReleasedOwnerState || !mOwner)
		{
			return;
		}
		if (Combatant* combatant = dynamic_cast<Combatant*>(mOwner))
		{
			// Primary fire was intentionally released at the blast frame. The
			// remaining locks only protect the initial recoil-commit window. The
			// impulse itself keeps decaying naturally after this actor is gone.
			combatant->GetAbilitySystemComponent().RemoveOwnedTag(
				GameplayTags::State::ActionLock::AbilityActivation
			);
			combatant->GetAbilitySystemComponent().RemoveOwnedTag(
				GameplayTags::State::ActionLock::MovementInput
			);
		}
		mReleasedOwnerState = true;
	}
}
