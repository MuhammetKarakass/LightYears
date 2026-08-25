#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/projectile/ProjectileReflectionSurface.h"
#include "presentation/ability/crystalBarricade/CrystalBarricadePresentationProfile.h"

#include <unordered_map>

namespace ly
{
	class CrystalBarricadeActor final
		: public AbilityWorldActor,
		  public ProjectileReflectionSurface
	{
	public:
		CrystalBarricadeActor(World* world, Actor* owner, const CrystalBarricadeWallPresentationProfile& profile);

		void BeginPlay() override;
		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void OnActorBeginOverlap(Actor* otherActor) override;
		void ConfigureFromAttributes(const sas::GameplayAttributeList& attributes) override;
		sf::Vector2f GetPhysicsCollisionBoxHalfExtents() const override;
		bool BuildProjectileReflectionResponse(
			const Actor& incomingProjectile,
			ProjectileReflectionSurfaceResponse& outResponse
		) const override;

	private:
		bool CanDamageContactTarget(const Actor& actor) const;
		void TryApplyContactDamage(Actor& actor);
		void ApplyPeriodicContactDamage();
		float ResolveOwnerAttribute(const sas::AttributeId& id) const;

		CrystalBarricadeWallPresentationProfile mProfile;
		float mLength = 300.f;
		float mThickness = 22.f;
		float mRemainingDuration = 6.f;
		float mBreakTimeRemaining = 0.f;
		float mContactDamage = 20.f;
		float mContactInterval = 0.50f;
		float mRicochetMultiplier = 0.80f;
		float mSameSurfaceLockDuration = 0.12f;
		std::unordered_map<uint64_t, float> mNextContactDamageTime;
	};

	bool RegisterCrystalBarricadeWallActorType();
}
