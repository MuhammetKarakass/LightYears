#include "gameplay/ability/actors/DirectionalChargeTelegraphActor.h"

#include "framework/MathUtility.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ly
{
	namespace
	{
		weak_ptr<Actor> MakeWeakActor(Actor* actor)
		{
			if (!actor)
			{
				return {};
			}

			const shared_ptr<Object> object = actor->GetWeakPtr().lock();
			return object
				? std::dynamic_pointer_cast<Actor>(object)
				: weak_ptr<Actor>{};
		}

		sf::Color WithAlpha(const sf::Color& color, float multiplier)
		{
			return sf::Color{
				color.r,
				color.g,
				color.b,
				static_cast<std::uint8_t>(std::clamp(
					static_cast<float>(color.a) * std::max(0.f, multiplier),
					0.f,
					255.f
				))
			};
		}

		float NormalizeDirectionComponent(float value)
		{
			return std::isfinite(value) ? value : 0.f;
		}
	}

	DirectionalChargeTelegraphActor::DirectionalChargeTelegraphActor(
		World* world,
		const SpawnParams& params
	)
		: Actor(world),
		mDefinition(params.visual),
		mGlow({ 1.f, 1.f }),
		mOuter({ 1.f, 1.f }),
		mCore({ 1.f, 1.f }),
		mEndpoint(1.f, 32),
		mEndpointRing(1.f, 32),
		mMinimumLength(std::max(1.f, params.minimumLength)),
		mMaximumLength(std::max(mMinimumLength, params.maximumLength)),
		mDuration(std::max(0.f, params.duration)),
		mMaximumLengthChargeThreshold(std::clamp(
			params.maximumLengthChargeThreshold,
			0.001f,
			1.f
		)),
		mDirection({
			NormalizeDirectionComponent(params.direction.x),
			NormalizeDirectionComponent(params.direction.y)
		}),
		mTargetActor(MakeWeakActor(params.targetActor))
	{
		if (GetVectorLength(mDirection) <= 0.001f)
		{
			mDirection = { 0.f, -1.f };
		}
		else
		{
			NormalizeVector(mDirection);
		}

		SetActorLocation(params.targetActor ? params.targetActor->GetActorLocation() : params.location);
		SetRenderLayer(RenderLayer::GroundDecal);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		UpdateVisuals();
	}

	void DirectionalChargeTelegraphActor::SetDirection(const sf::Vector2f& direction)
	{
		mDirection = {
			NormalizeDirectionComponent(direction.x),
			NormalizeDirectionComponent(direction.y)
		};
		if (GetVectorLength(mDirection) <= 0.001f)
		{
			mDirection = { 0.f, -1.f };
		}
		else
		{
			NormalizeVector(mDirection);
		}

		UpdateVisuals();
	}

	void DirectionalChargeTelegraphActor::Tick(float deltaTime)
	{
		if (mTargetActor.lock())
		{
			const shared_ptr<Actor> target = mTargetActor.lock();
			if (!target || target->GetIsPendingDestroy())
			{
				Destroy();
				return;
			}
			SetActorLocation(target->GetActorLocation());
		}

		const float safeDeltaTime = std::max(0.f, deltaTime);
		mAge += safeDeltaTime;
		if (mCompletionFeedback)
		{
			mCompletionAge += safeDeltaTime;
			if (mDefinition.completionFeedbackDuration <= 0.f ||
				mCompletionAge >= mDefinition.completionFeedbackDuration)
			{
				Destroy();
				return;
			}
			UpdateVisuals();
			Actor::Tick(deltaTime);
			return;
		}

		if (mDuration > 0.f && mAge >= mDuration)
		{
			Destroy();
			return;
		}

		UpdateVisuals();
		Actor::Tick(deltaTime);
	}

	void DirectionalChargeTelegraphActor::SetExternalProgress(float normalizedProgress)
	{
		if (GetIsPendingDestroy() || mCompletionFeedback)
		{
			return;
		}
		mProgress = std::clamp(normalizedProgress, 0.f, 1.f);
		UpdateVisuals();
	}

	void DirectionalChargeTelegraphActor::Complete(float normalizedProgress)
	{
		if (GetIsPendingDestroy() || mCompletionFeedback)
		{
			return;
		}

		mProgress = std::clamp(normalizedProgress, 0.f, 1.f);
		if (mDefinition.completionFeedbackDuration <= 0.f)
		{
			Destroy();
			return;
		}

		mCompletionFeedback = true;
		mCompletionProgress = mProgress;
		mCompletionAge = 0.f;
		UpdateVisuals();
	}

	bool DirectionalChargeTelegraphActor::IsInCompletionFeedback() const
	{
		return mCompletionFeedback && !GetIsPendingDestroy();
	}

	void DirectionalChargeTelegraphActor::UpdateVisuals()
	{
		const float progress = std::clamp(mProgress, 0.f, 1.f);
		const float lengthProgress = std::clamp(
			progress / mMaximumLengthChargeThreshold,
			0.f,
			1.f
		);
		const float length = mMinimumLength +
			(mMaximumLength - mMinimumLength) * lengthProgress;
		const float pulse = 0.85f + 0.15f * (
			std::sin(mAge * std::max(0.f, mDefinition.pulseSpeed)) + 1.f
		) * 0.5f;
		const float intensity = mDefinition.minimumAlpha +
			(mDefinition.maximumAlpha - mDefinition.minimumAlpha) * progress;
		const bool completion = mCompletionFeedback;
		const float completionProgress = completion
			? std::clamp(
				mCompletionAge / std::max(0.001f, mDefinition.completionFeedbackDuration),
				0.f,
				1.f
			)
			: 0.f;
		const float completionScale = completion
			? mDefinition.completionStartScale +
				(mDefinition.completionEndScale - mDefinition.completionStartScale) * completionProgress
			: 1.f;
		const float fade = completion ? 1.f - completionProgress : 1.f;

		UpdateGeometry(length, completionScale);
		mGlow.setFillColor(WithAlpha(mDefinition.glowColor, intensity * pulse * fade));
		mOuter.setFillColor(WithAlpha(mDefinition.outerColor, intensity * pulse * fade));
		mCore.setFillColor(WithAlpha(mDefinition.coreColor, intensity * fade));
		mEndpoint.setFillColor(WithAlpha(mDefinition.endpointColor, intensity * pulse * fade));
		mEndpointRing.setFillColor(sf::Color::Transparent);
		mEndpointRing.setOutlineColor(
			WithAlpha(mDefinition.endpointRingColor, intensity * pulse * fade)
		);
	}

	void DirectionalChargeTelegraphActor::UpdateGeometry(float length, float scale)
	{
		const float angle = RadiansToDegrees(std::atan2(mDirection.y, mDirection.x));
		const sf::Vector2f endpoint = GetActorLocation() + mDirection * length;

		mGlow.setSize({ length, mDefinition.glowWidth });
		mGlow.setOrigin({ 0.f, mDefinition.glowWidth * 0.5f });
		mGlow.setPosition(GetActorLocation());
		mGlow.setRotation(sf::degrees(angle));
		mGlow.setScale({ scale, scale });

		mOuter.setSize({ length, mDefinition.outerWidth });
		mOuter.setOrigin({ 0.f, mDefinition.outerWidth * 0.5f });
		mOuter.setPosition(GetActorLocation());
		mOuter.setRotation(sf::degrees(angle));
		mOuter.setScale({ scale, scale });

		mCore.setSize({ length, mDefinition.coreWidth });
		mCore.setOrigin({ 0.f, mDefinition.coreWidth * 0.5f });
		mCore.setPosition(GetActorLocation());
		mCore.setRotation(sf::degrees(angle));
		mCore.setScale({ scale, scale });

		mEndpoint.setRadius(mDefinition.endpointRadius);
		mEndpoint.setOrigin({ mDefinition.endpointRadius, mDefinition.endpointRadius });
		mEndpoint.setPosition(endpoint);
		mEndpoint.setScale({ scale, scale });

		const float ringRadius = mDefinition.endpointRadius + mDefinition.endpointRingThickness;
		mEndpointRing.setRadius(ringRadius);
		mEndpointRing.setOrigin({ ringRadius, ringRadius });
		mEndpointRing.setOutlineThickness(mDefinition.endpointRingThickness);
		mEndpointRing.setPosition(endpoint);
		mEndpointRing.setScale({ scale, scale });
	}

	void DirectionalChargeTelegraphActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		window.draw(mGlow, sf::RenderStates{ sf::BlendAdd });
		window.draw(mOuter, sf::RenderStates{ sf::BlendAdd });
		window.draw(mCore, sf::RenderStates{ sf::BlendAdd });
		window.draw(mEndpoint, sf::RenderStates{ sf::BlendAdd });
		window.draw(mEndpointRing, sf::RenderStates{ sf::BlendAdd });
	}
}
