#pragma once
#include <sky/VisualAssets.h>
#include "visual/ColorScript.h"
#include "visual/DeepSky.h"
#include "visual/PipelineSettings.h"
#include "visual/ShaderLibrary.h"
#include "visual/ShipVisual.h"
namespace vega::visual
{
	using vega::sky::ReloadReport;
	// Game art is loaded on top of the optional sky module, never by it.
	class VisualAssets : public vega::sky::VisualAssets
	{
	public:
		bool Load(const std::filesystem::path& root, std::string& error);
		ReloadReport Reload(bool force);
		ReloadReport Poll(float deltaSeconds, float intervalSeconds = .5f);
		ShipVisualLibrary& Ships() { return mShips; }
		const ShipVisualLibrary& Ships() const { return mShips; }
		const ReloadReport& LoadReport() const { return mReport; }
	private:
		ShipVisualLibrary mShips;
		ReloadReport mReport;
		float mTimer = 0.f;
	};
}
