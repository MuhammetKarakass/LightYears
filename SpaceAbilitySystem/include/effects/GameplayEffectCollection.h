#pragma once

#include "effects/GameplayEffectHandle.h"

#include <cstddef>
#include <iterator>
#include <list>
#include <optional>
#include <utility>
#include <vector>

namespace sas
{
	template <typename ActiveEffect>
	class GameplayEffectCollection
	{
	public:
		// Active effects are handed to behavior and presentation callbacks.
		// Stable node storage prevents an effect insertion from invalidating the
		// callback's current ActiveEffect reference.
		using Container = std::list<ActiveEffect>;

		GameplayEffectHandle AllocateHandle()
		{
			GameplayEffectHandle handle;
			do
			{
				handle = GameplayEffectHandle{ mNextHandleId++ };
			}
			while (Find(handle) != nullptr);
			return handle;
		}

		ActiveEffect& Emplace()
		{
			mEffects.emplace_back();
			return mEffects.back();
		}

		ActiveEffect* Find(GameplayEffectHandle handle)
		{
			for (ActiveEffect& effect : mEffects)
			{
				if (effect.handle == handle)
				{
					return &effect;
				}
			}
			return nullptr;
		}

		const ActiveEffect* Find(GameplayEffectHandle handle) const
		{
			for (const ActiveEffect& effect : mEffects)
			{
				if (effect.handle == handle)
				{
					return &effect;
				}
			}
			return nullptr;
		}

		std::optional<std::size_t> FindIndex(GameplayEffectHandle handle) const
		{
			std::size_t index = 0;
			for (const ActiveEffect& effect : mEffects)
			{
				if (effect.handle == handle)
				{
					return index;
				}
				++index;
			}
			return std::nullopt;
		}

		template <typename Predicate>
		ActiveEffect* FindFirst(Predicate&& predicate)
		{
			for (ActiveEffect& effect : mEffects)
			{
				if (predicate(effect))
				{
					return &effect;
				}
			}
			return nullptr;
		}

		template <typename Predicate>
		const ActiveEffect* FindFirst(Predicate&& predicate) const
		{
			for (const ActiveEffect& effect : mEffects)
			{
				if (predicate(effect))
				{
					return &effect;
				}
			}
			return nullptr;
		}

		bool EraseAt(std::size_t index)
		{
			if (index >= mEffects.size())
			{
				return false;
			}
			auto iterator = mEffects.begin();
			std::advance(iterator, static_cast<std::ptrdiff_t>(index));
			mEffects.erase(iterator);
			return true;
		}

		ActiveEffect* At(std::size_t index)
		{
			if (index >= mEffects.size())
			{
				return nullptr;
			}
			auto iterator = mEffects.begin();
			std::advance(iterator, static_cast<std::ptrdiff_t>(index));
			return &*iterator;
		}

		const ActiveEffect* At(std::size_t index) const
		{
			if (index >= mEffects.size())
			{
				return nullptr;
			}
			auto iterator = mEffects.begin();
			std::advance(iterator, static_cast<std::ptrdiff_t>(index));
			return &*iterator;
		}

		Container& GetAll() { return mEffects; }
		const Container& GetAll() const { return mEffects; }

		void Reset()
		{
			mEffects.clear();
			mNextHandleId = 1;
		}

	private:
		Container mEffects;
		unsigned int mNextHandleId = 1;
	};
}
