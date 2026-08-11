#pragma once

#include <cctype>
#include <functional>
#include <string>
#include <utility>

namespace sas
{
	// Effect behaviors cross the JSON and runtime registry boundary. They are
	// selectors, not gameplay semantics, so they must not be represented as tags.
	struct GameplayEffectBehaviorKey
	{
		std::string name;

		GameplayEffectBehaviorKey() = default;
		explicit GameplayEffectBehaviorKey(std::string value)
			: name(std::move(value))
		{
		}

		bool IsValid() const
		{
			if (name.empty() || name.front() == '.' || name.back() == '.')
			{
				return false;
			}
			bool beginsSegment = true;
			for (const unsigned char character : name)
			{
				if (character == '.')
				{
					if (beginsSegment)
					{
						return false;
					}
					beginsSegment = true;
					continue;
				}
				if (!std::isalnum(character) && character != '_')
				{
					return false;
				}
				beginsSegment = false;
			}
			return !beginsSegment;
		}

		const std::string& ToString() const { return name; }

		friend bool operator==(
			const GameplayEffectBehaviorKey& left,
			const GameplayEffectBehaviorKey& right
		)
		{
			return left.name == right.name;
		}
	};
}

template <>
struct std::hash<sas::GameplayEffectBehaviorKey>
{
	std::size_t operator()(const sas::GameplayEffectBehaviorKey& key) const noexcept
	{
		return std::hash<std::string>{}(key.name);
	}
};
