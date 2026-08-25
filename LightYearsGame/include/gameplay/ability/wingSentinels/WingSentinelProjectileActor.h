#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/wingSentinels/WingSentinelsPresentationProfile.h"

namespace ly
{
	class WingSentinelProjectileActor final : public AbilityWorldActor
	{
	public:
		WingSentinelProjectileActor(World* world, Actor* owner, const WingSentinelsPresentationProfile& profile);
		void BeginPlay() override;
		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void OnActorBeginOverlap(Actor* otherActor) override;
		void ConfigureFromAttributes(const sas::GameplayAttributeList& attributes) override;
		bool IsProjectileActor() const override { return true; }
		bool CanBeReflected() const override { return true; }
		bool TryReflectProjectile(const ProjectileReflectionRequest& request) override;

	private:
		void Move(float deltaTime);
		WingSentinelsPresentationProfile mProfile;
		float mSpeed = 0.f;
		float mRange = 0.f;
		float mTravelDistance = 0.f;
	};

	bool RegisterWingSentinelProjectileActorType();
}
