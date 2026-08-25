#include "gameplay/ability/cryostasis/CryostasisVisualActor.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ly
{
	namespace
	{
		sf::Color WithAlpha(const sf::Color& color, float multiplier)
		{
			sf::Color result = color;
			result.a = static_cast<std::uint8_t>(std::clamp(
				static_cast<float>(color.a) * std::max(0.f, multiplier), 0.f, 255.f
			));
			return result;
		}
	}

	CryostasisVisualActor::CryostasisVisualActor(
		World* world,
		Actor* owner,
		const CryostasisPresentationProfile& profile
	)
		: Actor(world), mOwner{ owner }, mProfile{ profile },
		mField{ 1.f, 80 }, mShell{ 1.f, 48 }, mShellOutline{ 1.f, 48 }
	{
		SetRenderLayer(RenderLayer::WorldVfx);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		SetEnablePhysics(false);
		mField.setFillColor(sf::Color::Transparent);
		mShellOutline.setFillColor(sf::Color::Transparent);
		UpdateVisuals();
	}

	void CryostasisVisualActor::SetIceHealthRatio(float ratio)
	{
		mIceHealthRatio = std::clamp(ratio, 0.f, 1.f);
	}

	void CryostasisVisualActor::BeginBreak()
	{
		mBreaking = true;
		mBreakAge = 0.f;
	}

	void CryostasisVisualActor::Tick(float deltaTime)
	{
		if (!mOwner || mOwner->GetIsPendingDestroy())
		{
			Destroy();
			return;
		}

		const float safeDeltaTime = std::max(0.f, deltaTime);
		mAge += safeDeltaTime;
		if (mBreaking)
		{
			mBreakAge += safeDeltaTime;
			if (mBreakAge >= std::max(0.01f, mProfile.breakDuration))
			{
				Destroy();
				return;
			}
		}
		SetActorLocation(mOwner->GetActorLocation());
		UpdateVisuals();
		Actor::Tick(deltaTime);
	}

	void CryostasisVisualActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		const sf::Vector2f location = GetActorLocation();
		mField.setPosition(location);
		mShell.setPosition(location);
		mShellOutline.setPosition(location);
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(mField, additive);
		window.draw(mShell, additive);
		window.draw(mShellOutline, additive);
		window.draw(mCracks, additive);
	}

	void CryostasisVisualActor::UpdateVisuals()
	{
		const float breakProgress = mBreaking
			? std::clamp(mBreakAge / std::max(0.01f, mProfile.breakDuration), 0.f, 1.f)
			: 0.f;
		const float pulse = 0.5f + 0.5f * std::sin(mAge * mProfile.shellPulseSpeed);
		const float fieldRadius = mProfile.fieldRadius * (1.f + breakProgress * 0.35f);
		const float shellRadius = mProfile.shellRadius * (1.f + breakProgress * 0.55f);
		mField.setRadius(std::max(1.f, fieldRadius));
		mField.setOrigin({ fieldRadius, fieldRadius });
		mField.setOutlineThickness(1.5f + 2.f * pulse);
		mField.setOutlineColor(WithAlpha(mProfile.fieldColor, (0.65f + 0.35f * pulse) * (1.f - breakProgress)));
		mShell.setRadius(std::max(1.f, shellRadius));
		mShell.setOrigin({ shellRadius, shellRadius });
		mShell.setFillColor(WithAlpha(mProfile.shellColor, (0.65f + 0.35f * pulse) * (1.f - breakProgress)));
		mShellOutline.setRadius(std::max(1.f, shellRadius + 2.f));
		mShellOutline.setOrigin({ shellRadius + 2.f, shellRadius + 2.f });
		mShellOutline.setOutlineThickness(-2.25f);
		mShellOutline.setOutlineColor(WithAlpha(mProfile.crackColor, (0.55f + 0.45f * mIceHealthRatio) * (1.f - breakProgress)));

		// As the shell weakens, six radial cracks lengthen from the hull. This is
		// state feedback, not a second collision or gameplay radius.
		const float crackFactor = std::clamp((1.f - mIceHealthRatio) + breakProgress, 0.f, 1.f);
		mCracks.resize(12);
		const sf::Vector2f center = GetActorLocation();
		for (std::size_t index = 0; index < 6; ++index)
		{
			const float angle = static_cast<float>(index) * 1.04719755f + mAge * 0.07f;
			const sf::Vector2f direction{ std::cos(angle), std::sin(angle) };
			const float inner = shellRadius * (0.24f + 0.18f * crackFactor);
			const float outer = shellRadius * (0.38f + 0.58f * crackFactor);
			mCracks[index * 2].position = center + direction * inner;
			mCracks[index * 2 + 1].position = center + direction * outer;
			mCracks[index * 2].color = WithAlpha(mProfile.crackColor, crackFactor * (1.f - breakProgress));
			mCracks[index * 2 + 1].color = WithAlpha(mProfile.crackColor, crackFactor * (1.f - breakProgress));
		}
	}
}
