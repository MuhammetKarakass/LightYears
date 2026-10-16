#pragma once
#include "visual/AsteroidField.h"
#include "visual/Galaxy.h"
#include "visual/OrbitSky.h"
#include "visual/ShaderLibrary.h"
#include <sky/DeepSky.h>
namespace ly::visual
{
	using vega::sky::SkyFieldType;
	using vega::sky::SkyField;
	using vega::sky::DeepSkyFields;
	using vega::sky::SkyRingRadiusGap;
	using vega::sky::FindSkyField;
	using vega::sky::ValidateSkyField;
	using vega::sky::DeepSkyKind;
	using vega::sky::DeepSkyObject;
	using vega::sky::DeepSkyBand;
	using vega::sky::DeepSkyPlanet;
	using vega::sky::DeepSkyNebulaLayer;
	using vega::sky::DeepSkyHostStar;
	using vega::sky::DeepSkyPaint;
	using vega::sky::SkyAsset;
	using vega::sky::DeepSkyCatalog;
	using vega::sky::DeepSkyLayer;
	using DeepSkyStop = vega::sky::SkyAsset;
}
