#pragma once

#include "framework/Actor.h"
#include "presentation/ability/glacialPressure/GlacialPressurePresentationProfile.h"

#include <SFML/Graphics/VertexArray.hpp>

namespace ly
{
	// Feature-local telegraph for the five radial cone bands. It follows the
	// owner during focus and uses the owner's current forward direction, so the
	// player can reposition/rotate before the blast snapshot is taken.
	class GlacialPressureTelegraphActor final : public Actor
	{
	public:
		GlacialPressureTelegraphActor(
			World* world,
			Actor* owner,
			const GlacialPressurePresentationProfile& profile
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void SetExternalProgress(float normalizedProgress);
		void Complete(float normalizedProgress = 1.f);
		bool IsInCompletionFeedback() const;

	private:
		void BuildGeometry();

		Actor* mOwner = nullptr;
		GlacialPressurePresentationProfile mProfile;
		sf::VertexArray mBands{ sf::PrimitiveType::Triangles };
		sf::VertexArray mSeparators{ sf::PrimitiveType::Lines };
		float mAge = 0.f;
		float mProgress = 0.f;
		float mCompletionProgress = 1.f;
		float mCompletionAge = 0.f;
		bool mCompletionFeedback = false;
	};
}
