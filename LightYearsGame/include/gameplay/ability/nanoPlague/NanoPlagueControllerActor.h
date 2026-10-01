#pragma once

#include "framework/Actor.h"
#include "gameplay/ability/nanoPlague/NanoPlagueControllerRegistryActor.h"
#include "gameplay/damage/DamageContext.h"
#include "presentation/ability/nanoPlague/NanoPlaguePresentationProfile.h"
#include <cstdint>
#include <map>

namespace ly
{
	// A persistent controller keeps infection alive after the instant ability has
	// ended. One controller is shared per owner, which is what makes refresh
	// semantics reliable even when future invocation systems overlap casts.
	class NanoPlagueControllerActor final : public Actor
	{
	public:
		struct Settings
		{
			float baseTickDamage = 1.f;
			float energyPowerTickScale = 0.05f;
			float duration = 4.f;
			float spreadRadius = 300.f;
			int baseSpreadTargetCount = 1;
			sas::ContentId sourceAbilityId;
			List<GameplayTag> sourceAbilityTags;
		};

		NanoPlagueControllerActor(
			World* world,
			Actor* owner,
			const NanoPlaguePresentationProfile& profile
		);
		~NanoPlagueControllerActor() override;

		static shared_ptr<NanoPlagueControllerActor> FindOrCreate(
			World& world,
			Actor& owner,
			const NanoPlaguePresentationProfile& profile
		);

		bool ApplyOrRefreshInfection(
			Actor& target,
			int generation,
			const Settings& settings,
			const sf::Vector2f* visualOrigin = nullptr
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void Destroy() override;

	private:
		friend struct AuditFixesE2EAccess;
		struct Infection
		{
			std::uint64_t id = 0;
			std::uint64_t revision = 0;
			weak_ptr<Actor> target;
			DelegateHandle damageSubscription;
			std::uint64_t generation = 0;
			float remainingDuration = 0.f;
			float tickAccumulator = 0.f;
			int ticksApplied = 0;
			bool pendingSpread = false;
			sf::Vector2f deathLocation{};
		};

		struct Pulse
		{
			sf::Vector2f origin{};
			weak_ptr<Actor> target;
			float elapsed = 0.f;
			float duration = 0.2f;
			bool isSpread = false;
		};

		shared_ptr<Actor> GetOwnerActor() const;
		bool ApplyOrRefreshInfectionInternal(
			Actor& target,
			std::uint64_t generation,
			const Settings& settings,
			const sf::Vector2f* visualOrigin
		);
		void OnTargetDamageResolved(const DamageContext& context);
		void ResolvePendingSpread(Infection infection);
		void ApplyTick(Actor& target);
		Infection* FindInfection(std::uint64_t id);
		void RemoveInfectionById(std::uint64_t id);
		bool HasInfection(const Actor& target) const;
		void AddPulse(
			const sf::Vector2f& origin,
			Actor& target,
			bool isSpread
		);
		std::uint64_t ResolveSpreadTargetCount() const;

		weak_ptr<Actor> mOwner;
		NanoPlagueControllerRegistryActor::Registration mRegistryRegistration;
		NanoPlaguePresentationProfile mProfile;
		Settings mSettings;
		// Monotonic id order preserves tick ordering; erase/find are O(log N).
		std::map<std::uint64_t, Infection> mInfections;
		List<Pulse> mPulses;
		float mVisualAge = 0.f;
		bool mDestroying = false;
		bool mTicking = false;
		std::uint64_t mNextInfectionId = 1;
	};
}
