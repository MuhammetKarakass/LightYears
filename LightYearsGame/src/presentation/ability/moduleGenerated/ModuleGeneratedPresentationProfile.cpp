#include "presentation/ability/moduleGenerated/ModuleGeneratedPresentationProfile.h"
#include "presentation/ability/PresentationProfileRegistry.h"

namespace ly
{
	bool RegisterModuleGeneratedPresentationProfiles()
	{
		static const bool registered = PresentationProfileRegistry<ModuleGuidedPresentationProfile>::Register({ ModuleGeneratedPresentationIds::Guided }) &&
			PresentationProfileRegistry<ModuleMinePresentationProfile>::Register({ ModuleGeneratedPresentationIds::Mine }) &&
			PresentationProfileRegistry<ModuleExplosionPresentationProfile>::Register({ ModuleGeneratedPresentationIds::Explosion }) &&
			PresentationProfileRegistry<ModuleExplosionPresentationProfile>::Register({ ModuleGeneratedPresentationIds::ThermalExplosion, { 255, 150, 60, 190 } });
		return registered;
	}
}
