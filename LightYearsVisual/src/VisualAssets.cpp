#include "visual/VisualAssets.h"
namespace ly::visual
{
	bool VisualAssets::Load(const std::filesystem::path& root, std::string& error)
	{
		if (!vega::sky::VisualAssets::Load(root, error)) return false;
		mReport = vega::sky::VisualAssets::LoadReport();
		mShips.LoadDirectory(root / "ships", mReport);
		return true;
	}
	ReloadReport VisualAssets::Reload(bool force)
	{
		auto report = vega::sky::VisualAssets::Reload(force);
		mShips.Reload(force, report);
		return report;
	}
	ReloadReport VisualAssets::Poll(float dt, float interval)
	{
		mTimer += dt;
		if (mTimer < interval) return {};
		mTimer = 0.f;
		return Reload(false);
	}
}