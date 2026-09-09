#pragma once

#include "framework/Actor.h"
#include "presentation/ability/blastback/BlastbackPresentationProfile.h"

#include <SFML/Graphics/VertexArray.hpp>

namespace ly
{
	// The recoil is separate from the ability lifetime so cooldown begins at the
	// blast frame. This actor owns only the remaining movement/action locks.
	class BlastbackBurstActor final : public Actor
	{
	public:
		BlastbackBurstActor(
			World* world,
			Actor* owner,
			const sf::Vector2f& origin,
			const sf::Vector2f& forward,
			const BlastbackPresentationProfile& profile,
			float recoilLockDuration
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void Destroy() override;

	private:
		void BuildGeometry(float alpha);
		void ReleaseOwnerState();

		Actor* mOwner = nullptr;
		BlastbackPresentationProfile mProfile;
		sf::Vector2f mOrigin{ 0.f, 0.f };
		sf::Vector2f mForward{ 1.f, 0.f };
		sf::VertexArray mGeometry{ sf::PrimitiveType::Triangles };
		float mRecoilLockDuration = 0.f;
		float mAge = 0.f;
		bool mReleasedOwnerState = false;
	};
}
