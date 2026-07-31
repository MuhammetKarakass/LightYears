#pragma once

namespace sas
{
	struct GameplayEffectHandle
	{
		unsigned int id = 0;

		bool IsValid() const { return id != 0; }
		bool operator==(const GameplayEffectHandle& other) const { return id == other.id; }
		bool operator!=(const GameplayEffectHandle& other) const { return id != other.id; }
	};
}
