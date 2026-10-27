#pragma once

#include "framework/Core.h"

#include <cstdint>
#include <functional>
#include <memory>

namespace vega
{
	class SpaceShip;
	struct DamageContext;

	// What happens to a hit that would bring a ship's health to zero.
	enum class LethalDamageResolution
	{
		None,          // the hit kills normally
		Prevented,     // the hit deals no health damage and is not a kill (revives, escapes)
		ConvertedKill  // the hit deals no health damage but counts as a kill (the target lives on in another form)
	};

	using LethalHitInterceptor = std::function<LethalDamageResolution(SpaceShip& target, const DamageContext& context)>;

	// Shared by source-side lethal-hit interception and target-side owner death prevention.
	// Lower priorities run first; equal priorities retain registration order. The first non-None result decides.
	// Each registration is owned by an RAII token; destroying or resetting the token removes exactly that interceptor.
	class LethalHitInterceptorList final
	{
		struct Entry
		{
			std::uint64_t id = 0;
			int priority = 0;
			LethalHitInterceptor interceptor;
		};
		struct State
		{
			std::uint64_t nextId = 1;
			List<Entry> entries;
		};

	public:
		class Registration final
		{
		public:
			Registration() = default;
			~Registration() { Reset(); }
			Registration(const Registration&) = delete;
			Registration& operator=(const Registration&) = delete;
			Registration(Registration&& other) noexcept;
			Registration& operator=(Registration&& other) noexcept;
			void Reset() noexcept;
			bool IsValid() const noexcept { return mId != 0 && !mState.expired(); }

		private:
			friend class LethalHitInterceptorList;
			Registration(std::weak_ptr<State> state, std::uint64_t id) : mState{ std::move(state) }, mId{ id } {}
			std::weak_ptr<State> mState;
			std::uint64_t mId = 0;
		};

		[[nodiscard]] Registration Register(LethalHitInterceptor interceptor, int priority = 0);
		LethalDamageResolution Resolve(SpaceShip& target, const DamageContext& context) const;

	private:
		std::shared_ptr<State> mState = std::make_shared<State>();
	};
}
