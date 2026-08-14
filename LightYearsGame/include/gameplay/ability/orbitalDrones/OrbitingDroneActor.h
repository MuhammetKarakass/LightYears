#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/orbitalDrones/OrbitalDronesPresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>

#include <cstddef>

namespace ly
{
	// A single reusable member of an Orbital Drones formation. The ability
	// behavior owns spawning and lifetime policy; this actor owns orbit motion,
	// contact throttling, shared damage delivery, and its local presentation.
	class OrbitingDroneActor final : public AbilityWorldActor
	{
	public:
		struct OrbitConfiguration
		{
			float radius = 200.f;
			// Runtime uses radians per second to keep the motion math unambiguous.
			float angularSpeedRadiansPerSecond = 2.5f;
			// The behavior supplies one stable value per drone in the formation.
			float phaseOffsetRadians = 0.f;
		};

		OrbitingDroneActor(
			World* world,
			Actor* owner,
			const OrbitalDronesPresentationProfile& presentationProfile,
			OrbitConfiguration orbit = {}
		);

		void BeginPlay() override;
		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void OnActorBeginOverlap(Actor* otherActor) override;
		void ConfigureFromAttributes(
			const sas::GameplayAttributeList& attributes
		) override;

		// These setters are the narrow runtime contract used by the future
		// ability behavior/spawner. They keep orbital tuning out of shared actor
		// code and allow the same actor to serve different drone variants.
		void SetOrbitConfiguration(const OrbitConfiguration& configuration);
		void SetOrbitConfiguration(
			float radius,
			float angularSpeedRadiansPerSecond,
			float phaseOffsetRadians
		);
		void SetContactRadius(float radius);
		void SetSameTargetHitCooldown(float cooldownSeconds);

		// AbilityWorldActor normally derives this payload from damage attributes.
		// The explicit override lets a behavior/spawner pass a prebuilt payload
		// while retaining the same shared combat delivery path.
		void SetDamagePayload(const DamagePayload& payload);
		void ClearDamagePayloadOverride();

		const OrbitConfiguration& GetOrbitConfiguration() const
		{
			return mOrbit;
		}
		float GetContactRadius() const { return mContactRadius; }
		float GetSameTargetHitCooldown() const { return mSameTargetHitCooldown; }
		float GetOrbitAngleRadians() const { return mOrbitAngleRadians; }

		// Uniform formation spacing is kept here so all callers calculate phase
		// offsets consistently without adding a global targeting/presentation API.
		static float CalculateFormationPhase(
			std::size_t droneIndex,
			std::size_t droneCount,
			float formationPhaseRadians = 0.f
		);

	private:
		struct TargetHitCooldown
		{
			weak_ptr<Actor> target;
			float nextAllowedAt = 0.f;
		};

		void UpdateOrbit(float deltaTime);
		void ProcessContactCandidates();
		void TryDamageTarget(Actor* target);
		bool IsEligibleCombatant(const Actor* target) const;
		bool IsTouchingTarget(const Actor& target) const;
		void PruneHitCooldowns();
		float ResolveExpiryFade() const;
		void UpdatePrimitiveGeometry();

		weak_ptr<Actor> mOwnerActor;
		OrbitalDronesPresentationProfile mPresentationProfile;
		OrbitConfiguration mOrbit;
		sf::CircleShape mGlow;
		sf::CircleShape mBody;
		sf::CircleShape mCore;
		// Actor addresses become invalid as soon as the World releases them. Keep
		// a stable runtime identity for lookup and a weak handle for safe pruning.
		Dictionary<unsigned int, TargetHitCooldown> mNextHitAllowedAt;
		DamagePayload mDamagePayloadOverride;
		float mOrbitAngleRadians = 0.f;
		float mVisualAge = 0.f;
		float mContactClock = 0.f;
		float mContactRadius = 12.f;
		float mSameTargetHitCooldown = 0.5f;
		bool mHasDamagePayloadOverride = false;
	};
}
