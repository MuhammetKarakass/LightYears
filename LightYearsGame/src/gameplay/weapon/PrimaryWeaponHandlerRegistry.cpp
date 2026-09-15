#include "gameplay/weapon/PrimaryWeaponHandlerRegistry.h"

#include "internal/PrimaryWeaponBuiltIns.h"

#include <cstdlib>

namespace ly
{
	namespace
	{
		using PrimaryWeaponHandlerMap = Dictionary<
			PrimaryWeaponType,
			unique_ptr<PrimaryWeaponHandler>,
			std::hash<PrimaryWeaponType>
		>;
		using PrimaryWeaponFeatureMap = Dictionary<
			PrimaryWeaponFeatureType,
			unique_ptr<PrimaryWeaponFeatureHandler>,
			std::hash<PrimaryWeaponFeatureType>
		>;

		PrimaryWeaponHandlerMap& GetHandlers()
		{
			static PrimaryWeaponHandlerMap handlers;
			return handlers;
		}

		PrimaryWeaponFeatureMap& GetFeatures()
		{
			static PrimaryWeaponFeatureMap features;
			return features;
		}
	}

	void PrimaryWeaponHandlerRegistry::EnsureBuiltIns()
	{
		static const bool initialized = []
		{
			const auto registerHandler = [](unique_ptr<PrimaryWeaponHandler> handler)
			{
				return handler && GetHandlers().emplace(handler->GetType(), std::move(handler)).second;
			};
			const auto registerFeature = [](unique_ptr<PrimaryWeaponFeatureHandler> feature)
			{
				return feature && GetFeatures().emplace(feature->GetFeatureType(), std::move(feature)).second;
			};

			bool registered = true;
			registered &= registerHandler(PrimaryWeaponBuiltIns::CreateStandardProjectileWeaponHandler());
			registered &= registerHandler(PrimaryWeaponBuiltIns::CreateShotgunWeaponHandler());
			registered &= registerHandler(PrimaryWeaponBuiltIns::CreateElectricArcWeaponHandler());
			registered &= registerHandler(PrimaryWeaponBuiltIns::CreateContinuousBeamWeaponHandler());
			registered &= registerHandler(PrimaryWeaponBuiltIns::CreateExpandingWaveWeaponHandler());
			registered &= registerFeature(PrimaryWeaponBuiltIns::CreateHeatFeatureHandler());
			if (!registered)
			{
				std::abort();
			}
			return true;
		}();
		(void)initialized;
	}

	bool PrimaryWeaponHandlerRegistry::RegisterHandler(unique_ptr<PrimaryWeaponHandler> handler)
	{
		EnsureBuiltIns();
		if (!handler)
		{
			return false;
		}
		return GetHandlers().emplace(handler->GetType(), std::move(handler)).second;
	}

	bool PrimaryWeaponHandlerRegistry::RegisterFeature(unique_ptr<PrimaryWeaponFeatureHandler> feature)
	{
		EnsureBuiltIns();
		if (!feature)
		{
			return false;
		}
		return GetFeatures().emplace(feature->GetFeatureType(), std::move(feature)).second;
	}

	const PrimaryWeaponHandler* PrimaryWeaponHandlerRegistry::FindHandler(PrimaryWeaponType weaponType)
	{
		EnsureBuiltIns();
		auto found = GetHandlers().find(weaponType);
		return found != GetHandlers().end() ? found->second.get() : nullptr;
	}

	const PrimaryWeaponFeatureHandler* PrimaryWeaponHandlerRegistry::FindFeature(PrimaryWeaponFeatureType featureType)
	{
		EnsureBuiltIns();
		auto found = GetFeatures().find(featureType);
		return found != GetFeatures().end() ? found->second.get() : nullptr;
	}
}
