#pragma once

#include "framework/Actor.h"
#include "presentation/ability/cryostasis/CryostasisPresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/VertexArray.hpp>

namespace ly
{
	class CryostasisVisualActor final : public Actor
	{
	public:
		CryostasisVisualActor(
			World* world,
			Actor* owner,
			const CryostasisPresentationProfile& profile
		);

		void SetIceHealthRatio(float ratio);
		void BeginBreak();
		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;

	private:
		void UpdateVisuals();

		Actor* mOwner = nullptr;
		CryostasisPresentationProfile mProfile;
		sf::CircleShape mField;
		sf::CircleShape mShell;
		sf::CircleShape mShellOutline;
		sf::VertexArray mCracks{ sf::PrimitiveType::Lines };
		float mIceHealthRatio = 1.f;
		float mAge = 0.f;
		float mBreakAge = 0.f;
		bool mBreaking = false;
	};
}
