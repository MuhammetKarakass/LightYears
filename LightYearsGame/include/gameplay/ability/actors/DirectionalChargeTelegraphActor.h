#pragma once

#include "framework/Actor.h"
#include "presentation/ability/common/DirectionalChargeTelegraphVisualDefinition.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

namespace ly
{
	// Reusable directional charge preview. It follows an optional actor while
	// its owner may update the aim direction during the charge.
	class DirectionalChargeTelegraphActor final : public Actor
	{
	public:
		struct SpawnParams
		{
			sf::Vector2f location{};
			sf::Vector2f direction{ 0.f, -1.f };
			float minimumLength = 1.f;
			float maximumLength = 1.f;
			float duration = 0.f;
			float maximumLengthChargeThreshold = 0.90f;
			DirectionalChargeTelegraphVisualDefinition visual;
			Actor* targetActor = nullptr;
		};

		DirectionalChargeTelegraphActor(World* world, const SpawnParams& params);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void SetDirection(const sf::Vector2f& direction);
		void SetExternalProgress(float normalizedProgress);
		void Complete(float normalizedProgress = 1.f);
		bool IsInCompletionFeedback() const;

	private:
		void UpdateVisuals();
		void UpdateGeometry(float length, float scale);

		DirectionalChargeTelegraphVisualDefinition mDefinition;
		sf::RectangleShape mGlow;
		sf::RectangleShape mOuter;
		sf::RectangleShape mCore;
		sf::CircleShape mEndpoint;
		sf::CircleShape mEndpointRing;
		float mMinimumLength = 1.f;
		float mMaximumLength = 1.f;
		float mDuration = 0.f;
		float mMaximumLengthChargeThreshold = 0.90f;
		float mAge = 0.f;
		float mProgress = 0.f;
		float mCompletionProgress = 1.f;
		float mCompletionAge = 0.f;
		sf::Vector2f mDirection{ 0.f, -1.f };
		weak_ptr<Actor> mTargetActor;
		bool mCompletionFeedback = false;
	};
}
