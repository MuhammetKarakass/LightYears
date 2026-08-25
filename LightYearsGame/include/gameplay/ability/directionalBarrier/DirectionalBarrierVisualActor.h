#pragma once

#include "framework/Actor.h"
#include "presentation/ability/directionalBarrier/DirectionalBarrierPresentationProfile.h"

#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/VertexArray.hpp>

namespace ly
{
	class DirectionalBarrierVisualActor final : public Actor
	{
	public:
		DirectionalBarrierVisualActor(
			World* world,
			Actor* owner,
			const DirectionalBarrierPresentationProfile& profile
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;

	private:
		void UpdateVisuals();

		Actor* mOwner = nullptr;
		DirectionalBarrierPresentationProfile mProfile;
		sf::ConvexShape mOuterPanel;
		sf::VertexArray mSideEdges{ sf::PrimitiveType::Lines, 4 };
		float mAge = 0.f;
	};
}
