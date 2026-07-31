#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

namespace sas
{
	template <
		typename Key,
		typename Product,
		typename Hash = std::hash<Key>
	>
	class FactoryRegistry
	{
	public:
		using Factory = std::function<std::unique_ptr<Product>()>;

		bool Register(const Key& key, Factory factory)
		{
			if (!factory)
			{
				return false;
			}
			return mFactories.emplace(key, std::move(factory)).second;
		}

		bool IsRegistered(const Key& key) const
		{
			return mFactories.find(key) != mFactories.end();
		}

		std::unique_ptr<Product> Create(const Key& key) const
		{
			const auto found = mFactories.find(key);
			return found != mFactories.end() ? found->second() : nullptr;
		}

		void Clear()
		{
			mFactories.clear();
		}

	private:
		std::unordered_map<Key, Factory, Hash> mFactories;
	};
}
