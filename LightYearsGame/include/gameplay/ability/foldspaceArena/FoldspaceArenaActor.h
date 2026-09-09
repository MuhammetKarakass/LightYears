#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/foldspaceArena/FoldspaceArenaPresentationProfile.h"

namespace ly
{
	// One lightweight actor owns both phases because the delivery is only a
	// non-damaging route to a fixed target. It becomes the arena at impact.
	class FoldspaceArenaActor final : public AbilityWorldActor
	{
	public:
		FoldspaceArenaActor(
			World* world,
			Actor* owner,
			const FoldspaceArenaPresentationProfile& presentationProfile
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void ConfigureFromAttributes(
			const sas::GameplayAttributeList& attributes
		) override;
		void ConfigureFromAbilityValues(
			const sas::GameplayAttributeList& values,
			float ownerEnergyMax
		);
		void SetSnapshotTarget(const sf::Vector2f& targetLocation);

	private:
		enum class Phase { Travelling, Active };

		bool IsInsideArena(const sf::Vector2f& location) const;
		sf::Vector2f FindBoundaryCrossing(
			const sf::Vector2f& inside,
			const sf::Vector2f& outside
		) const;
		sf::Vector2f ResolveOutwardNormal(const sf::Vector2f& boundaryPoint) const;
		bool IsExternalRelocation(
			const Actor& owner,
			const sf::Vector2f& previousLocation,
			float deltaTime
		) const;
		void BeginArena();
		void ApplyFormationDamage();
		void UpdateOwnerBoundary(float deltaTime);
		void RenderArenaBoundary(sf::RenderWindow& window) const;
		void RenderOppositeGhost(sf::RenderWindow& window) const;

		FoldspaceArenaPresentationProfile mPresentationProfile;
		Phase mPhase = Phase::Travelling;
		sf::Vector2f mTargetLocation{};
		sf::Vector2f mPreviousOwnerLocation{};
		float mProjectileSpeed = 1200.f;
		float mArenaWidth = 1500.f;
		float mArenaHeight = 1000.f;
		float mCornerRadius = 250.f;
		float mArenaDuration = 6.f;
		float mWrapInwardOffset = 20.f;
		float mPhaseElapsed = 0.f;
		float mVisualAge = 0.f;
		bool mTargetConfigured = false;
		bool mHasPreviousOwnerLocation = false;
	};

	bool RegisterFoldspaceArenaActorType();
}
