#pragma once

#include "framework/Actor.h"
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace ly
{
	class ModuleFirstHitHistory final
	{
		struct Entry { std::weak_ptr<const Actor> target; std::uint64_t generation; bool committed = false; };
		struct State { std::unordered_map<std::uint64_t, Entry> targets; std::uint64_t nextGeneration = 1; };
	public:
		class Claim final
		{
		public:
			Claim(std::weak_ptr<State> state, std::uint64_t id, std::uint64_t generation) : mState{ std::move(state) }, mId{ id }, mGeneration{ generation } {}
			~Claim()
			{
				if (const auto state = mState.lock())
				{
					const auto found = state->targets.find(mId);
					if (found != state->targets.end() && found->second.generation == mGeneration && !found->second.committed) state->targets.erase(found);
				}
			}
			Claim(const Claim&) = delete;
			Claim& operator=(const Claim&) = delete;
			void Commit() const
			{
				if (const auto state = mState.lock())
				{
					const auto found = state->targets.find(mId);
					if (found != state->targets.end() && found->second.generation == mGeneration) found->second.committed = true;
				}
			}
		private:
			std::weak_ptr<State> mState;
			std::uint64_t mId;
			std::uint64_t mGeneration;
		};
		std::shared_ptr<Claim> TryClaim(const Actor& target) const
		{
			const auto lifetime = target.GetWeakActor().lock();
			if (!lifetime || target.GetIsPendingDestroy() || mState->targets.count(target.GetUniqueID())) return {};
			const auto generation = mState->nextGeneration++;
			auto claim = std::make_shared<Claim>(mState, target.GetUniqueID(), generation);
			mState->targets.emplace(target.GetUniqueID(), Entry{ lifetime, generation });
			return claim;
		}
		void Prune() const
		{
			for (auto it = mState->targets.begin(); it != mState->targets.end();)
				if (it->second.target.expired()) it = mState->targets.erase(it); else ++it;
		}
		std::size_t GetTargetCount() const { Prune(); return mState->targets.size(); }
		void Clear() { mState = std::make_shared<State>(); }
	private:
		std::shared_ptr<State> mState = std::make_shared<State>();
	};
}
