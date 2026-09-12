#pragma once

#include "framework/Actor.h"
#include "gameplay/damage/DamageContext.h"
#include "presentation/ability/nanoPlague/NanoPlaguePresentationProfile.h"

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
			float baseTickDamage = 4.f;
			float energyPowerTickScale = 0.03f;
			float duration = 3.f;
			float tickInterval = 0.25f;
			float spreadRadius = 300.f;
			int baseSpreadTargetCount = 1;
			int maximumSpreadTargetCount = 5;
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

	private:
		struct Infection
		{
			weak_ptr<Actor> target;
			int generation = 0;
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

		Actor* GetOwnerActor() const;
		void OnTargetDamageResolved(const DamageContext& context);
		void ResolvePendingSpread(const Infection& infection);
		void ApplyTick(Infection& infection, Actor& target);
		void RemoveInfection(std::size_t index);
		bool HasInfection(const Actor& target) const;
		void AddPulse(
			const sf::Vector2f& origin,
			Actor& target,
			bool isSpread
		);
		int ResolveSpreadTargetCount() const;

		weak_ptr<Actor> mOwner;
		Actor* mUnmanagedOwner = nullptr;
		NanoPlaguePresentationProfile mProfile;
		Settings mSettings;
		List<Infection> mInfections;
		List<Pulse> mPulses;
		float mVisualAge = 0.f;
	};
}
