#pragma once

#include "abilities/AbilityTypes.h"
#include "abilities/AbilityHandle.h"
#include "framework/Core.h"
#include "framework/GameplayTag.h"
#include <SFML/System/Vector2.hpp>
#include <array>
#include <algorithm>
#include <optional>
#include <random>

namespace vega
{
	class Actor;
	class GameAbility;
	class LightYearsAbilitySystemComponent;

	struct AbilityActivationModifiers { float mainEffectMultiplier = 1.f; float cooldownMultiplier = 1.f; };
	struct AbilityTimingSpec
	{
		float durationSeconds = 0.f, windowCenterFraction = 0.f, bigWindowSeconds = 0.f, smallWindowSeconds = 0.f;
		AbilityActivationModifiers small, big, miss;
		float feedbackSeconds = 0.f;
	};
	struct AbilityAutocastSpec { float recentDamageSeconds = 0.f, defaultRange = 0.f; };
	struct AbilityRandomPowerSpec { float minimum = 1.f, maximum = 1.f, doubleRollChancePerLuck = 0.f, feedbackSeconds = 0.f; };
	struct AbilityAimOverride { sf::Vector2f direction, targetPoint; };
	struct AbilityTimingView
	{
		bool open = false;
		float progress = 0.f, bigStart = 0.f, bigEnd = 0.f, smallStart = 0.f, smallEnd = 0.f;
		std::string feedback;
		bool good = false;
		bool operator==(const AbilityTimingView& other) const { return open == other.open && progress == other.progress && bigStart == other.bigStart && bigEnd == other.bigEnd && smallStart == other.smallStart && smallEnd == other.smallEnd && feedback == other.feedback && good == other.good; }
	};

	// Scoped to one component and one synchronous behavior/action call. Nested calls restore the caller's aim.
	class AbilityAimScope
	{
	public:
		AbilityAimScope(LightYearsAbilitySystemComponent& owner, const std::optional<AbilityAimOverride>& aim);
		~AbilityAimScope();
		AbilityAimScope(const AbilityAimScope&) = delete;
		AbilityAimScope& operator=(const AbilityAimScope&) = delete;
	private:
		LightYearsAbilitySystemComponent& mOwner;
		std::optional<AbilityAimOverride> mPrevious;
	};
	sf::Vector2f ResolveAbilityAimPoint(const Actor& owner);
	sf::Vector2f ResolveAbilityAimDirection(const Actor& owner);

	// Ship-owned input/activation mechanics; no dependency on module contracts or content identities.
	class AbilityActivationPolicy
	{
	public:
		explicit AbilityActivationPolicy(LightYearsAbilitySystemComponent& owner) : mOwner{ owner } {}
		void ConfigureTiming(sas::AbilitySlot slot, std::uint32_t source, std::optional<AbilityTimingSpec> spec);
		void ConfigureAutocast(sas::AbilitySlot slot, std::uint32_t source, std::optional<AbilityAutocastSpec> spec);
		void ConfigureRandomPower(sas::AbilitySlot slot, std::optional<AbilityRandomPowerSpec> spec);
		void ConfigureDamageType(sas::AbilitySlot slot, const GameplayTag& type);
		bool HandleInput(GameAbility& ability, bool held);
		void Tick(float deltaTime);
		AbilityActivationModifiers PrepareActivation(GameAbility& ability);
		void CommitActivation(GameAbility& ability);
		GameplayTag GetDamageType(sas::AbilitySlot slot) const;
		AbilityTimingView GetView(sas::AbilitySlot slot, bool second = false) const;
		void ObserveDamage() { mDamageAge = 0.f; }
		void AdvanceDamageTime(float deltaTime) { mDamageAge += std::max(0.f, deltaTime); }
		void SetRandomSeed(std::uint32_t seed) { mRandom.seed(seed); }
		void Clear();
		bool HasConfiguration(sas::AbilitySlot slot) const;
		float ResolveAutocastRange(GameAbility& ability, float fallback) const;
	private:
		struct TimingState
		{
			sas::AbilityHandle handle;
			bool held = false, open = false;
			float elapsed = 0.f, feedbackRemaining = 0.f;
			std::optional<AbilityActivationModifiers> pending;
			std::string feedback;
			bool good = false;
		};
		struct AutocastState { sas::AbilityHandle handle; bool charging = false; float elapsed = 0.f; };
		bool CanAutomate(bool checkActionLock = true) const;
		bool StartTimed(GameAbility& ability, TimingState& state, const AbilityTimingSpec& spec);
		std::size_t BankIndex(const GameAbility& ability) const;
		LightYearsAbilitySystemComponent& mOwner;
		std::array<std::optional<AbilityTimingSpec>, 4> mTiming;
		std::array<std::optional<AbilityAutocastSpec>, 4> mAutocast;
		std::array<std::optional<AbilityRandomPowerSpec>, 4> mPower;
		std::array<GameplayTag, 4> mDamageTypes;
		std::array<std::uint32_t, 4> mTimingSources{}, mAutocastSources{};
		std::array<TimingState, 8> mTimingStates;
		std::array<AutocastState, 8> mAutocastStates;
		std::array<float, 8> mRolledPower{};
		float mDamageAge = 1000000.f;
		std::mt19937 mRandom{ std::random_device{}() };
	};
}
