#pragma once
#include <string>

namespace vega
{
	// Preserve the shipped collection's identifiers. This is a game convention,
	// independent of the Sky module's opaque asset IDs and collection format.
	inline const std::string& SkyIdForRouteStop(const std::string& routeStopId)
	{
		return routeStopId;
	}
}
