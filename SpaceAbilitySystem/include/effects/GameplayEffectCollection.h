#pragma once

#include "effects/GameplayEffectHandle.h"

#include <cstddef>
#include <iterator>
#include <list>
#include <optional>
#include <unordered_map>
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
		using Iterator = typename Container::iterator;

		GameplayEffectHandle AllocateHandle()
		{
			GameplayEffectHandle handle;
			do
			{
				handle = GameplayEffectHandle{ mNextHandleId++ };
			}
			while (mEffectsByHandle.find(handle.id) != mEffectsByHandle.end());
			return handle;
		}

		ActiveEffect& Emplace(GameplayEffectHandle handle)
		{
			mEffects.emplace_back();
			Iterator iterator = std::prev(mEffects.end());
			iterator->handle = handle;
			mEffectsByHandle.emplace(handle.id, iterator);
			return *iterator;
		}

		ActiveEffect* Find(GameplayEffectHandle handle)
		{
			const auto iterator = mEffectsByHandle.find(handle.id);
			return iterator != mEffectsByHandle.end() ? &*iterator->second : nullptr;
		}

		const ActiveEffect* Find(GameplayEffectHandle handle) const
		{
			const auto iterator = mEffectsByHandle.find(handle.id);
			return iterator != mEffectsByHandle.end() ? &*iterator->second : nullptr;
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
			mEffectsByHandle.erase(iterator->handle.id);
			mEffects.erase(iterator);
			return true;
		}

		bool Erase(GameplayEffectHandle handle)
		{
			const auto index = mEffectsByHandle.find(handle.id);
			if (index == mEffectsByHandle.end())
			{
				return false;
			}
			mEffects.erase(index->second);
			mEffectsByHandle.erase(index);
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
			mEffectsByHandle.clear();
			mNextHandleId = 1;
		}

	private:
		Container mEffects;
		std::unordered_map<unsigned int, Iterator> mEffectsByHandle;
		unsigned int mNextHandleId = 1;
	};
}
