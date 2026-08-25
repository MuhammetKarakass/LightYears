#pragma once

#include "attributes/AttributeSystem.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/voidGate/VoidGatePresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>

namespace ly
{
	class VoidGatePortalActor final : public AbilityWorldActor
	{
	public:
		VoidGatePortalActor(
			World* world,
			Actor* owner,
			const VoidGatePresentationProfile& presentationProfile
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void ConfigureFromAttributes(
			const sas::GameplayAttributeList& attributes
		) override;

		float GetPortalRadius() const { return mPortalRadius; }
		// Portal A is a non-functional placement preview until portal B exists.
		void SetActivationAlpha(float alpha);

	private:
		void ConfigureGeometry();

		VoidGatePresentationProfile mPresentationProfile;
		sf::CircleShape mOuterRing;
		sf::CircleShape mInnerRing;
		sf::CircleShape mGlow;
		float mPortalRadius = 70.f;
		float mVisualAge = 0.f;
		float mActivationAlpha = 1.f;
	};

	bool RegisterVoidGatePortalActorType();
}
