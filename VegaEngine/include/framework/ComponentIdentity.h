#pragma once

#include <cstddef>
#include <optional>
#include <string>

namespace vega
{
	class Actor;
	class ActorComponent;
	// Other component types may be inserted ahead of this one without changing its address.
	struct ComponentIdentity
	{
		std::string typeName;
		std::size_t occurrence = 0;
		bool operator==(const ComponentIdentity& other) const
		{
			return typeName == other.typeName && occurrence == other.occurrence;
		}
		bool operator!=(const ComponentIdentity& other) const { return !(*this == other); }
	};
	std::optional<ComponentIdentity> IdentifyComponent(const Actor& actor, const ActorComponent& target);
	ActorComponent* FindComponentByTypeOccurrence(Actor& actor, const ComponentIdentity& identity);
}
