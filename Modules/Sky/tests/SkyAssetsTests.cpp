#include <sky/DeepSky.h>
#include <sky/VisualAssets.h>
#include <fstream>
#include <iostream>

int main()
{
	using nlohmann::json;
	vega::sky::DeepSkyCatalog catalog;
	std::string error;
	const json sky = {{"objects", {{{"kind", "fog"}}}}, {"band", {{"width", .25}}}};
	const auto check = [&](bool value) { if (!value) std::cerr << error << '\n'; return value; };
	if (!check(catalog.Apply({{"id", "aurora"}, {"sky", sky}}, error))) return 1;
	const float seed = catalog.Find("aurora")->objects.front().seed;
	if (!check(catalog.Apply({{"skies", {{"aurora", sky}}}}, error))) return 1;
	if (catalog.Find("aurora")->objects.front().seed != seed) return 2;
	if (!check(catalog.Apply({{"stops", {{"aurora", sky}}}}, error))) return 3;
	if (catalog.Find("aurora")->objects.front().seed != seed) return 4;
	const auto version = catalog.Version();
	if (catalog.Apply({{"sky", sky}}, error) || catalog.Version() != version || !catalog.Find("aurora")) return 5;
	if (!check(catalog.Apply({{"presets", {{"soft", sky}}}, {"skies", {{"aurora", "soft"}}}}, error))) return 6;
	if (catalog.Find("aurora")->objects.front().seed != seed) return 7;
	const std::filesystem::path root{VEGA_SKY_ASSETS_TEST_ROOT};
	std::filesystem::create_directories(root);
	std::filesystem::copy(VEGA_SKY_ASSETS_SOURCE, root,
		std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
	std::filesystem::create_directories(root / "ships");
	std::ofstream{root / "ships/invalid.json"} << "not a ship definition";
	vega::sky::VisualAssets assets;
	if (!check(assets.Load(root, error, "skies/aurora.sky.json")) || !assets.LoadReport().errors.empty()) return 8;
	if (!assets.DeepSky().Find("aurora")) return 9;
	if (!check(assets.PreviewDeepSky({{"id", "temporary"}, {"sky", sky}}, error))) return 10;
	assets.EndDeepSkyPreview();
	if (!assets.DeepSky().Find("aurora") || assets.DeepSky().Find("temporary")) return 11;
	std::ofstream{root / "skies/aurora.sky.json"} << "invalid sky";
	assets.Reload(true);
	if (!assets.DeepSky().Find("aurora")) return 12;
	std::cout << "Independent sky assets, collections, legacy IDs and transactional parsing passed\n";
	return 0;
}
