#include "gameplay/weapon/PrimaryWeaponHandlerRegistry.h"

#include "internal/PrimaryWeaponBuiltIns.h"

namespace ly
{
	namespace
	{
		using PrimaryWeaponHandlerMap = Dictionary<
			GameplayTag,
			unique_ptr<PrimaryWeaponHandler>,
			GameplayTagHash
		>;
		using PrimaryWeaponFeatureMap = Dictionary<
			GameplayTag,
			unique_ptr<PrimaryWeaponFeatureHandler>,
			GameplayTagHash
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
			GetHandlers().emplace(
				PrimaryWeaponSchema::Projectile::Standard::TypeId,
				PrimaryWeaponBuiltIns::CreateStandardProjectileWeaponHandler()
			);
			GetHandlers().emplace(
				PrimaryWeaponSchema::Projectile::Shotgun::TypeId,
				PrimaryWeaponBuiltIns::CreateShotgunWeaponHandler()
			);
			GetHandlers().emplace(
				PrimaryWeaponSchema::Arc::Electric::TypeId,
				PrimaryWeaponBuiltIns::CreateElectricArcWeaponHandler()
			);
			GetHandlers().emplace(
				PrimaryWeaponSchema::Beam::Continuous::TypeId,
				PrimaryWeaponBuiltIns::CreateContinuousBeamWeaponHandler()
			);
			GetHandlers().emplace(
				PrimaryWeaponSchema::Wave::Expanding::TypeId,
				PrimaryWeaponBuiltIns::CreateExpandingWaveWeaponHandler()
			);
			GetFeatures().emplace(
				PrimaryWeaponSchema::Feature::Heat::FeatureId,
				PrimaryWeaponBuiltIns::CreateHeatFeatureHandler()
			);
			return true;
		}();
		(void)initialized;
	}

	bool PrimaryWeaponHandlerRegistry::RegisterHandler(
		unique_ptr<PrimaryWeaponHandler> handler
	)
	{
		EnsureBuiltIns();
		if (!handler || !handler->GetTypeTag().IsValid())
		{
			return false;
		}
		return GetHandlers().emplace(handler->GetTypeTag(), std::move(handler)).second;
	}

	bool PrimaryWeaponHandlerRegistry::RegisterFeature(
		unique_ptr<PrimaryWeaponFeatureHandler> feature
	)
	{
		EnsureBuiltIns();
		if (!feature || !feature->GetFeatureTag().IsValid())
		{
			return false;
		}
		return GetFeatures().emplace(feature->GetFeatureTag(), std::move(feature)).second;
	}

	const PrimaryWeaponHandler* PrimaryWeaponHandlerRegistry::FindHandler(
		const GameplayTag& weaponTypeTag
	)
	{
		EnsureBuiltIns();
		auto found = GetHandlers().find(weaponTypeTag);
		return found != GetHandlers().end() ? found->second.get() : nullptr;
	}

	const PrimaryWeaponFeatureHandler* PrimaryWeaponHandlerRegistry::FindFeature(
		const GameplayTag& featureTag
	)
	{
		EnsureBuiltIns();
		auto found = GetFeatures().find(featureTag);
		return found != GetFeatures().end() ? found->second.get() : nullptr;
	}
}
