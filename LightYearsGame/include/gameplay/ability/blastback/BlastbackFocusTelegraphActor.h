#pragma once

#include "framework/Actor.h"
#include "presentation/ability/blastback/BlastbackPresentationProfile.h"

#include <SFML/Graphics/VertexArray.hpp>

namespace ly
{
	// Visual-only preview. It follows the owner while focusing, but it has no
	// collision or gameplay authority; the ability snapshots targets on release.
	class BlastbackFocusTelegraphActor final : public Actor
	{
	public:
		BlastbackFocusTelegraphActor(
			World* world,
			Actor* owner,
			const BlastbackPresentationProfile& profile
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void SetProgress(float normalizedProgress);

	private:
		void BuildGeometry();

		Actor* mOwner = nullptr;
		BlastbackPresentationProfile mProfile;
		sf::VertexArray mGeometry{ sf::PrimitiveType::Triangles };
		float mProgress = 0.f;
		float mAge = 0.f;
	};
}
