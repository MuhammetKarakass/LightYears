#include "gameplay/combat/LethalHitInterceptors.h"

#include <algorithm>

namespace vega
{
	LethalHitInterceptorList::Registration::Registration(Registration&& other) noexcept : mState{ std::move(other.mState) }, mId{ other.mId }
	{
		other.mId = 0;
	}

	LethalHitInterceptorList::Registration& LethalHitInterceptorList::Registration::operator=(Registration&& other) noexcept
	{
		if (this == &other) return *this;
		Reset();
		mState = std::move(other.mState);
		mId = other.mId;
		other.mId = 0;
		return *this;
	}

	void LethalHitInterceptorList::Registration::Reset() noexcept
	{
		if (const auto state = mState.lock())
		{
			const std::uint64_t id = mId;
			state->entries.erase(std::remove_if(state->entries.begin(), state->entries.end(), [id](const Entry& entry) { return entry.id == id; }), state->entries.end());
		}
		mState.reset();
		mId = 0;
	}

	LethalHitInterceptorList::Registration LethalHitInterceptorList::Register(LethalHitInterceptor interceptor)
	{
		if (!interceptor) return {};
		const std::uint64_t id = mState->nextId++;
		mState->entries.push_back({ id, std::move(interceptor) });
		return Registration{ mState, id };
	}

	LethalDamageResolution LethalHitInterceptorList::Resolve(SpaceShip& target, const DamageContext& context) const
	{
		// A copy: an interceptor may register or remove interceptors while it runs.
		const List<Entry> entries = mState->entries;
		for (const Entry& entry : entries)
		{
			const LethalDamageResolution resolution = entry.interceptor(target, context);
			if (resolution != LethalDamageResolution::None) return resolution;
		}
		return LethalDamageResolution::None;
	}
}
