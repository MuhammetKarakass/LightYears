#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameConfigs/combat/WeaponStructs.h"

namespace ly
{
	// Gameplay actor for a travelling wave. Damage is applied once per target
	// using a swept, widening collision test; rendering is only its presentation.
	class ExpandingWaveWeaponActor final : public AbilityWorldActor
	{
	public:
		ExpandingWaveWeaponActor(
			World* world,
			Actor* owner,
			const WeaponPresentationDefinition& presentation,
			const GameplayAttributeList& attributes,
			const List<GameplayTag>& damageTags
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;

	private:
		void ApplyHits(
			const sf::Vector2f& segmentStart,
			const sf::Vector2f& segmentEnd
		);
		void RebuildGeometry();

		float mSpeed = 0.f;
		float mMaxTravelDistance = 0.f;
		float mTravelDistance = 0.f;
		float mInitialWidth = 1.f;
		float mMaximumWidth = 1.f;
		float mCurrentWidth = 1.f;
		float mThickness = 1.f;
		sf::Color mColor;
		sf::VertexArray mWaveStrip{ sf::PrimitiveType::TriangleStrip };
		Set<Actor*> mHitTargets;
	};
}
