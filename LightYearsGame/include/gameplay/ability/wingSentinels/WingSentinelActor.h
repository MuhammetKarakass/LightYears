#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/ability/GameAbilityActionExecutor.h"
#include "presentation/ability/wingSentinels/WingSentinelsPresentationProfile.h"

namespace ly
{
	class WingSentinelActor final : public AbilityWorldActor
	{
	public:
		enum class Side { Left = -1, Right = 1 };
		struct Configuration
		{
			Side side = Side::Left;
			float sideOffset = 110.f;
			float targetingRange = 650.f;
			float baseAttackRate = 1.5f;
			LightYearsAbilitySystemComponent* abilitySystem = nullptr;
			const GameAbilityDefinition* abilityDefinition = nullptr;
			GameAbility* abilityInstance = nullptr;
		};

		WingSentinelActor(World* world, Actor* owner, const WingSentinelsPresentationProfile& profile, Configuration configuration);
		void BeginPlay() override;
		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;

	private:
		void UpdateFormation();
		shared_ptr<Actor> FindTarget() const;
		void TryFire(float deltaTime);
		float ResolveAttackRate() const;

		WingSentinelsPresentationProfile mProfile;
		Configuration mConfiguration;
		float mFireCooldown = 0.f;
		float mVisualAge = 0.f;
	};
}
