#pragma once

namespace sas
{
	struct AbilityHandle
	{
		unsigned int id = 0;

		bool IsValid() const { return id != 0; }
		bool operator==(const AbilityHandle& other) const { return id == other.id; }
		bool operator<(const AbilityHandle& other) const { return id < other.id; }
	};
}
