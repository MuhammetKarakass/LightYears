#include "gameplay/ability/shieldGraft/ShieldGraftVisualActor.h"

#include "framework/MathUtility.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ly
{
	namespace
	{
		float Saturate(float value)
		{
			return std::clamp(value, 0.f, 1.f);
		}

		sf::Color WithAlpha(const sf::Color& color, float multiplier)
		{
			sf::Color result = color;
			result.a = static_cast<std::uint8_t>(std::clamp(
				static_cast<float>(color.a) * Saturate(multiplier),
				0.f,
				255.f
			));
			return result;
		}
	}

	ShieldGraftVisualActor::ShieldGraftVisualActor(
		World* world,
		Actor* owner,
		const ShieldGraftPresentationProfile& profile
	)
		: Actor(world),
		  mOwner(owner),
		  mProfile(profile),
		  mContractingRing(1.f, 60),
		  mInnerGlow(1.f, 40)
	{
		SetRenderLayer(RenderLayer::WorldVfx);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		SetEnablePhysics(false);

		mContractingRing.setFillColor(sf::Color::Transparent);

		if (mOwner)
		{
			SetActorLocation(mOwner->GetActorLocation());
			mOwner->onActorDestroyed.BindAction(GetWeakPtr(), &ShieldGraftVisualActor::OnOwnerDestroyed);
		}
		UpdateVisuals();
	}

	void ShieldGraftVisualActor::OnOwnerDestroyed(Actor* destroyedActor)
	{
		(void)destroyedActor;
		Destroy();
	}

	void ShieldGraftVisualActor::Tick(float deltaTime)
	{
		if (!mOwner || mOwner->GetIsPendingDestroy())
		{
			Destroy();
			return;
		}

		SetActorLocation(mOwner->GetActorLocation());

		mAge += std::max(0.f, deltaTime);
		if (mAge >= std::max(0.001f, mProfile.visualDuration))
		{
			Destroy();
			return;
		}

		UpdateVisuals();
	}

	void ShieldGraftVisualActor::UpdateVisuals()
	{
		const float duration = std::max(0.001f, mProfile.visualDuration);
		const float t = Saturate(mAge / duration);

		// Outer-to-inner convergence
		const float currentRadius = std::max(
			1.f,
			mProfile.startRadius + (mProfile.endRadius - mProfile.startRadius) * t
		);

		const sf::Color currentColor = ly::LerpColor(
			mProfile.outerRingColor,
			mProfile.innerFlashColor,
			t
		);
		const float alphaFade = 1.f - (t * t);

		mContractingRing.setRadius(currentRadius);
		mContractingRing.setOrigin(sf::Vector2f{ currentRadius, currentRadius });
		mContractingRing.setOutlineThickness(std::max(1.f, mProfile.ringThickness * (1.f - 0.3f * t)));
		mContractingRing.setOutlineColor(WithAlpha(currentColor, alphaFade));
		mContractingRing.setPosition(GetActorLocation());

		const float innerRadius = std::max(1.f, currentRadius * 0.5f);
		mInnerGlow.setRadius(innerRadius);
		mInnerGlow.setOrigin(sf::Vector2f{ innerRadius, innerRadius });
		mInnerGlow.setFillColor(WithAlpha(mProfile.innerFlashColor, alphaFade * t * 0.4f));
		mInnerGlow.setPosition(GetActorLocation());
	}

	void ShieldGraftVisualActor::Render(sf::RenderWindow& window)
	{
		window.draw(mContractingRing);
		window.draw(mInnerGlow);
	}
}
