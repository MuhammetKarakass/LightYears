#pragma once

#include "attributes/AttributeSystem.h"

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
			const sas::GameplayAttributeList& attributes,
			const List<GameplayTag>& damageTags
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		// Applies the firing shot's policy (empowered status stacks, critical)
		// and lets the wave report its first hit to the weapon.
		void SetShotMetadata(const PrimaryWeaponShotMetadata& metadata);

	private:
		void ApplyHits(
			const sf::Vector2f& segmentStart,
			const sf::Vector2f& segmentEnd,
			float startWidth,
			float endWidth
		);
		void RebuildGeometry();
		void AwardFrostMeter(bool targetWasChilled);

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
		PrimaryWeaponShotMetadata mShotMetadata;
		bool mShotHitReported = false;
		float mFullStackDamageMultiplier = 1.f;
		// Frost Meter points this wave already awarded (each condition once).
		bool mAwardedMultiTarget = false;
		bool mAwardedChilledTarget = false;
	};
}
