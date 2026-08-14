#pragma once

#include "framework/Actor.h"
#include "presentation/ability/common/AreaTelegraphVisualDefinition.h"
#include "presentation/ability/common/AreaTelegraphVisualState.h"

#include <SFML/Graphics/CircleShape.hpp>

namespace ly
{
	// Reusable circular warning. It may remain at a fixed world location or
	// follow an actor for effects that are centered on a moving target.
	class AreaTelegraphActor final : public Actor
	{
	public:
		struct SpawnParams
		{
			sf::Vector2f location{};
			float radius = 1.f;
			float duration = 0.f;
			AreaTelegraphVisualDefinition visual;
			AreaTelegraphAnchorMode anchor = AreaTelegraphAnchorMode::FixedLocation;
			AreaTelegraphProgressDriver progress = AreaTelegraphProgressDriver::Timed;
			Actor* targetActor = nullptr;
		};

		AreaTelegraphActor(World* world, const SpawnParams& params);

		// Fixed-location mode: used by warnings such as SunBeam's predicted impact.
		AreaTelegraphActor(
			World* world,
			const sf::Vector2f& worldLocation,
			float radius,
			float lifeTime,
			const AreaTelegraphVisualDefinition& definition,
			Actor* targetActor = nullptr
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void SetExternalProgress(float normalizedProgress);
		void Complete(float normalizedProgress = 1.f);
		bool IsInCompletionFeedback() const;

		// Compatibility delegates for existing ability code and external callers.
		void SetCountdownProgress(float normalizedProgress);
		void SetCompleted();
		void SetCompleted(float normalizedProgress);
		bool IsShowingCompletionFeedback() const;

	private:
		void UpdateVisuals();

		AreaTelegraphVisualDefinition mDefinition;
		sf::CircleShape mFill;
		sf::CircleShape mCountdownRing;
		sf::CircleShape mOutline;
		float mDuration = 0.f;
		float mAge = 0.f;
		float mCountdownProgress = 0.f;
		float mCompletionProgress = 1.f;
		float mCompletionAge = 0.f;
		AreaTelegraphAnchorMode mAnchor = AreaTelegraphAnchorMode::FixedLocation;
		AreaTelegraphProgressDriver mProgressDriver = AreaTelegraphProgressDriver::Timed;
		AreaTelegraphPhase mPhase = AreaTelegraphPhase::Countdown;
		weak_ptr<Actor> mTargetActor;
	};
}
