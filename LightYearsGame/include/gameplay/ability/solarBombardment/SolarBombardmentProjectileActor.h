#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/solarBombardment/SolarBombardmentPresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <optional>

namespace ly
{
	class AreaTelegraphActor;

	class SolarBombardmentProjectileActor final : public AbilityWorldActor
	{
	public:
		SolarBombardmentProjectileActor(
			World* world,
			Actor* owner,
			const SolarBombardmentPresentationProfile& presentationProfile,
			std::optional<sf::Vector2f> requestedTargetLocation
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void ConfigureFromAttributes(
			const sas::GameplayAttributeList& attributes
		) override;
		void OnActorBeginOverlap(Actor* otherActor) override;
		bool IsProjectileActor() const override { return true; }
		// Solar Bombardment is a delivery projectile. It resolves at its fixed
		// target point and intentionally does not become a relay clone.
		bool CanBeCapturedByRelay() const override { return false; }

	private:
		void ResolveTargetLocation();
		void SpawnTelegraphs();
		void UpdateTelegraphProgress(float normalizedProgress);
		void Detonate();
		void ApplyExplosionDamage();
		void ConfigureVisualGeometry();
		void DrawProjectile(sf::RenderWindow& window);
		void DrawExplosion(sf::RenderWindow& window);

		SolarBombardmentPresentationProfile mPresentationProfile;
		std::optional<sf::Vector2f> mRequestedTargetLocation;
		std::weak_ptr<Actor> mOwnerReference;
		std::weak_ptr<AreaTelegraphActor> mOuterTelegraph;
		std::weak_ptr<AreaTelegraphActor> mInnerTelegraph;
		sf::Vector2f mResolvedTargetLocation{};
		sf::Vector2f mFlightDirection{ 0.f, -1.f };
		sf::Vector2f mStartLocation{};
		sf::CircleShape mProjectileGlow;
		sf::CircleShape mProjectileCore;
		sf::CircleShape mExplosionOuter;
		sf::CircleShape mExplosionInner;
		sf::CircleShape mExplosionShockwave;
		sf::VertexArray mTrail;
		float mCastRange = 0.f;
		float mInnerRadius = 0.f;
		float mOuterRadius = 0.f;
		float mInnerDamageMultiplier = 1.f;
		int mInnerIgniteStacks = 0;
		int mOuterIgniteStacks = 0;
		float mMinTravelTime = 0.f;
		float mMaxTravelTime = 0.f;
		float mTargetDistance = 0.f;
		float mTravelDuration = 0.f;
		float mTravelAge = 0.f;
		float mExplosionAge = 0.f;
		float mVisualAge = 0.f;
		bool mDetonated = false;
		bool mDamageApplied = false;
	};

	bool RegisterSolarBombardmentProjectileActorType();
}
