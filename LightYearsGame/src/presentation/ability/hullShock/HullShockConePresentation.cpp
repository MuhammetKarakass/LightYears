#include "presentation/ability/hullShock/HullShockConePresentation.h"
#include "presentation/ability/hullShock/HullShockPresentationIds.h"
#include "presentation/ability/hullShock/HullShockPresentationProfile.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <algorithm>
#include <cmath>

namespace vega
{
	bool RegisterHullShockConePresentationProfiles()
	{
		static const bool registered = []
		{
			const auto* base = PresentationProfileRegistry<HullShockPresentationProfile>::Find(HullShockPresentationIds::FocusBasic);
			if (!base) return false;
			return PresentationProfileRegistry<HullShockConePresentationProfile>::Register({ sas::ContentId{ HullShockPresentationIds::DirectedDischarge }, base->focusTelegraph });
		}();
		return registered;
	}
	HullShockConeVisualActor::HullShockConeVisualActor(World* world, Actor* owner, const sf::Vector2f& direction, float halfAngleRadians, float radius) : Actor{ world }, mOwner{ owner ? owner->GetWeakActor() : weak_ptr<Actor>{} }, mDirection{ direction }, mHalfAngle{ halfAngleRadians }, mRadius{ radius }
	{
		if (const auto* profile = PresentationProfileRegistry<HullShockConePresentationProfile>::Find(HullShockPresentationIds::DirectedDischarge)) mProfile = *profile;
		if (owner) SetActorLocation(owner->GetActorLocation());
		SetRenderLayer(RenderLayer::GroundDecal);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
	}
	void HullShockConeVisualActor::Complete()
	{
		if (const auto owner = mOwner.lock()) SetActorLocation(owner->GetActorLocation());
		mComplete = true;
		mAge = 0.f;
		mOwner.reset();
	}
	void HullShockConeVisualActor::Tick(float deltaTime)
	{
		if (!mComplete)
		{
			const auto owner = mOwner.lock();
			if (!owner || owner->GetIsPendingDestroy()) { Destroy(); return; }
			SetActorLocation(owner->GetActorLocation());
		}
		else if ((mAge += std::max(0.f, deltaTime)) >= mProfile.style.completionFeedbackDuration) Destroy();
	}
	void HullShockConeVisualActor::Render(sf::RenderTarget& target)
	{
		if (!IsRenderEnabled()) return;
		constexpr int Segments = 48;
		const auto& style = mProfile.style;
		auto fill = mComplete ? style.completionFillColor : style.fillColor;
		auto outline = mComplete ? style.completionOutlineColor : style.outlineColor;
		float scale = 1.f;
		if (mComplete)
		{
			const float progress = std::clamp(mAge / std::max(.001f, style.completionFeedbackDuration), 0.f, 1.f);
			scale = style.completionStartScale + progress * (style.completionEndScale - style.completionStartScale);
			fill.a = static_cast<std::uint8_t>(fill.a * (1.f - progress));
			outline.a = static_cast<std::uint8_t>(outline.a * (1.f - progress));
		}
		const auto origin = GetActorLocation();
		const float angle = std::atan2(mDirection.y, mDirection.x);
		sf::VertexArray fan{ sf::PrimitiveType::TriangleFan, Segments + 2 };
		sf::VertexArray boundary{ sf::PrimitiveType::LineStrip, Segments + 3 };
		fan[0] = { origin, fill }; boundary[0] = { origin, outline };
		for (int i = 0; i <= Segments; ++i)
		{
			const float a = angle - mHalfAngle + 2.f * mHalfAngle * static_cast<float>(i) / Segments;
			const auto point = origin + sf::Vector2f{ std::cos(a), std::sin(a) } * mRadius * scale;
			fan[i + 1] = { point, fill }; boundary[i + 1] = { point, outline };
		}
		boundary[Segments + 2] = { origin, outline };
		target.draw(fan); target.draw(boundary);
	}
}
