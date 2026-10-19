#include "presentation/visual/ProceduralBackdrop.h"
#include "gameplay/galaxy/GalaxyRoute.h"
#include "level/GameSceneSettings.h"
#include "BackdropTestAccess.h"
#include <framework/EngineContext.h>
#include <framework/World.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace
{
	using Json = nlohmann::json;
	using Access = vega::ProceduralBackdropTestAccess;
	const std::filesystem::path Source{LIGHT_YEARS_PROJECT_SOURCE_DIR};
	constexpr sf::Vector2u Size{960, 540};

	void Require(bool condition, const std::string& message)
	{
		if (!condition) throw std::runtime_error(message);
	}

	std::string PixelHash(const sf::Image& image)
	{
		std::uint64_t hash = 14695981039346656037ULL;
		const auto count = static_cast<std::size_t>(image.getSize().x) * image.getSize().y * 4;
		for (std::size_t i = 0; i < count; ++i) { hash ^= image.getPixelsPtr()[i]; hash *= 1099511628211ULL; }
		std::ostringstream out;
		out << std::hex << std::setfill('0') << std::setw(16) << hash;
		return out.str();
	}

	vega::GalaxyRoute LoadRoute()
	{
		vega::GalaxyRoute route;
		std::string error;
		Require(route.LoadFromFile(Source / "LightYearsGame/assets/content/data/galaxy_route.json", error), error);
		route.AppendProceduralStops(vega::RouteProceduralStopCount, vega::RouteProceduralSeed);
		Require(route.AppendStopsFromFile(Source / "LightYearsGame/assets/content/data/galaxy_centre_tour.json", error), error);
		return route;
	}

	template<typename V> Json V2(const V& value) { return {value.x, value.y}; }
	template<typename V> Json V3(const V& value) { return {value.x, value.y, value.z}; }

	Json Parameters(const vega::visual::PlanetParams& p)
	{
		Json result{{"center", V2(p.center)}, {"radius", p.radius}, {"pixelScale", p.pixelScale}, {"surfacePixelScale", p.surfacePixelScale}, {"light", V3(p.light)}, {"rotation", p.rotation}, {"seed", p.seed}, {"dither", p.dither}, {"brightness", p.brightness}, {"saturation", p.saturation}, {"opacity", p.opacity}, {"lightColor", V3(p.lightColor)}, {"lightIntensity", p.lightIntensity}, {"kind", static_cast<int>(p.kind)}, {"timeSeconds", p.timeSeconds}, {"cloudAmount", p.cloudAmount}, {"atmosphereColor", V3(p.atmosphereColor)}, {"spinAxis", V3(p.spinAxis)}, {"haloWidth", p.haloWidth}, {"surfaceScale", p.surfaceScale}, {"orbitAxis", V3(p.orbitAxis)}, {"orbitAngle", p.orbitAngle}, {"ring", nullptr}, {"rayCamera", nullptr}};
		if (p.ring) result["ring"] = {{"normal", V3(p.ring->normal)}, {"inner", p.ring->inner}, {"outer", p.ring->outer}, {"opacity", p.ring->opacity}};
		if (p.rayCamera) result["rayCamera"] = {{"centerDir", V3(p.rayCamera->centerDir)}, {"distance", p.rayCamera->distance}, {"viewCenter", V2(p.rayCamera->viewCenter)}, {"flow", V2(p.rayCamera->flow)}, {"pixelsPerRadian", p.rayCamera->pixelsPerRadian}};
		result["ramp"] = Json::array();
		const auto ramp = vega::visual::PlanetRamp(p.kind);
		for (int i = 0; i < 5; ++i) { const auto c = ramp.Sample(static_cast<float>(i) / 4.f); result["ramp"].push_back({c.r, c.g, c.b, c.a}); }
		return result;
	}

	void Observe(vega::ProceduralBackdrop& backdrop, Json& trace)
	{
		Access::Observe(backdrop, [&](std::size_t index, Access::Pass pass, const auto& planet, unsigned moonSeed)
		{
			const char* names[] = {"back-ring", "back-moon", "body", "front-ring", "front-moon"};
			trace.push_back({{"index", index}, {"pass", names[static_cast<int>(pass)]}, {"planet", Parameters(planet)}, {"moonSeed", moonSeed}});
		});
	}

	Json Owner(const vega::ProceduralBackdrop& backdrop)
	{
		return {{"center", V2(Access::Center(backdrop))}, {"radius", Access::Radius(backdrop)}, {"starDirection", V2(Access::StarDirection(backdrop))}, {"shatteredMoon", Access::HasShatteredMoon(backdrop)}};
	}

	class Capture
	{
	public:
		Capture() : scope{context}, world{context}, pipeline{assets}
		{
			std::string error;
			Require(sf::Shader::isAvailable(), "Shaders unavailable");
			Require(assets.Load(Source / "LightYearsVisual/assets", error), error);
			Require(assets.LoadReport().errors.empty(), "Optional visual assets failed to load");
			Require(pipeline.Resize(Size) && target.resize(Size), "Cannot create offscreen capture targets");
		}

		sf::Image Frame(vega::ProceduralBackdrop& backdrop, float time, bool shifted)
		{
			pipeline.BeginFrame();
			backdrop.Draw(assets, pipeline, world, sf::View{shifted ? sf::Vector2f{1330.f, 330.f} : sf::Vector2f{960.f, 540.f}, {1920.f, 1080.f}}, time);
			target.clear(); pipeline.Compose(target); target.display();
			return target.getTexture().copyToImage();
		}

		vega::visual::VisualAssets assets;
	private:
		vega::EngineContext context;
		vega::EngineContext::Scope scope;
		vega::World world;
		vega::visual::VisualPipeline pipeline;
		sf::RenderTexture target;
	};

	int CaptureStops(const std::filesystem::path& root, const std::string& selected, float selectedTime, bool selectedShift)
	{
		std::filesystem::create_directories(root);
		std::ifstream input{Source / "LightYearsVisual/assets/deepsky.json"};
		Json catalog; input >> catalog;
		const auto route = LoadRoute();
		Capture capture;
		Json manifest{{"hash", "fnv1a64-decoded-rgba8-row-major"}, {"size", {Size.x, Size.y}}, {"viewSize", {1920, 1080}}, {"fixedCamera", {960, 540}}, {"shiftedCamera", {1330, 330}}, {"images", Json::array()}};
		std::size_t stops = 0;
		Json submissions = Json::array();
		for (const auto& entry : catalog.at("stops").items())
		{
			if (!selected.empty() && selected != entry.key()) continue;
			const auto found = std::find_if(route.Stops().begin(), route.Stops().end(), [&](const auto& stop) { return stop.id == entry.key(); });
			Require(found != route.Stops().end(), "Sky missing route stop: " + entry.key());
			++stops;
			// Include route-generated planets as well as authored planet/orbit blocks.
			const bool movingBody = !found->planets.empty() || entry.value().contains("planet") || entry.value().contains("orbit");
			const std::vector<float> times = selected.empty() ? (movingBody ? std::vector<float>{0.f, 30.f, 120.f} : std::vector<float>{0.f}) : std::vector<float>{selectedTime};
			for (float time : times) for (bool shifted : {false, true})
			{
				if (!selected.empty() && shifted != selectedShift) continue;
				vega::ProceduralBackdrop backdrop;
				backdrop.SetRouteStop(*found);
				capture.Frame(backdrop, 0.f, false); // Anchor the camera and start location/orbit clocks at zero.
				Json trace = Json::array();
				Observe(backdrop, trace);
				const auto image = capture.Frame(backdrop, time, shifted);
				const auto name = entry.key() + "-t" + std::to_string(static_cast<int>(time)) + (shifted ? "-shifted.png" : "-fixed.png");
				Require(image.saveToFile(root / name), "Cannot save " + name);
				sf::Image decoded;
				Require(decoded.loadFromFile(root / name), "Cannot decode " + name);
				Require(PixelHash(image) == PixelHash(decoded), "PNG round trip changed pixels");
				manifest["images"].push_back({{"file", name}, {"stop", entry.key()}, {"timeSeconds", time}, {"camera", shifted ? "shifted" : "fixed"}, {"pixels", PixelHash(decoded)}});
				submissions.push_back({{"file", name}, {"submissions", trace}, {"owner", Owner(backdrop)}});
			}
			std::cout << "Captured " << entry.key() << std::endl;
		}
		Require(stops == (selected.empty() ? 55u : 1u), "Unexpected stop count");
		manifest["stopCount"] = stops;
		std::ofstream{root / "manifest.json"} << manifest.dump(2) << '\n';
		std::ofstream{root / "body-submissions.json"} << submissions.dump(2) << '\n';
		std::cout << "Captured " << manifest["images"].size() << " images across " << stops << " stops\n";
		return 0;
	}

	int Compare(const std::filesystem::path& before, const std::filesystem::path& after)
	{
		Json first, second;
		std::ifstream{before / "manifest.json"} >> first;
		std::ifstream{after / "manifest.json"} >> second;
		Require(first.at("stopCount") == second.at("stopCount") && first.at("images").size() == second.at("images").size(), "Capture coverage differs");
		Json result{{"stopCount", first.at("stopCount")}, {"identical", 0}, {"different", Json::array()}};
		for (const auto& entry : first.at("images"))
		{
			const auto file = entry.at("file").get<std::string>();
			sf::Image a, b;
			Require(a.loadFromFile(before / file) && b.loadFromFile(after / file), "Cannot decode comparison: " + file);
			const auto left = PixelHash(a), right = PixelHash(b);
			Require(left == entry.at("pixels"), "Baseline pixels no longer match manifest: " + file);
			const auto match = std::find_if(second.at("images").begin(), second.at("images").end(), [&](const auto& item) { return item.at("file") == file; });
			Require(match != second.at("images").end() && match->at("pixels") == right, "Candidate manifest mismatch: " + file);
			const bool identical = a.getSize() == b.getSize() && std::equal(a.getPixelsPtr(), a.getPixelsPtr() + static_cast<std::size_t>(a.getSize().x) * a.getSize().y * 4, b.getPixelsPtr());
			if (identical) result["identical"] = result["identical"].get<int>() + 1;
			else result["different"].push_back({{"file", file}, {"before", left}, {"after", right}});
		}
		std::ofstream{after / "comparison.json"} << result.dump(2) << '\n';
		std::cout << result.dump(2) << '\n';
		return result["different"].empty() ? 0 : 1;
	}

	int TwoBodies(const std::filesystem::path& root)
	{
		std::filesystem::create_directories(root);
		const auto route = LoadRoute();
		Capture capture;
		const auto found = std::find_if(route.Stops().begin(), route.Stops().end(), [&](const auto& stop)
		{
			const auto* sky = capture.assets.DeepSky().Find(stop.id);
			return sky && !sky->orbit && !sky->planet.hide && !stop.planets.empty();
		});
		Require(found != route.Stops().end(), "No normal planet fixture");
		Json evidence = Json::array();
		for (float time : {0.f, 30.f, 120.f}) for (bool shifted : {false, true}) for (bool attachments : {false, true})
		{
			vega::ProceduralBackdrop single, multiple;
			single.SetRouteStop(*found); multiple.SetRouteStop(*found);
			capture.Frame(single, 0.f, false); capture.Frame(multiple, 0.f, false);
			Json singleTrace = Json::array(), multiTrace = Json::array();
			Observe(single, singleTrace); Observe(multiple, multiTrace);
			const auto one = capture.Frame(single, time, shifted);
			Require(singleTrace.size() == 3, "Entry 0 must submit back-moon/body/front-moon");
			Require(singleTrace[0]["pass"] == "back-moon" && singleTrace[1]["pass"] == "body" && singleTrace[2]["pass"] == "front-moon", "Entry 0 submission order changed");
			const auto primaryCenter = Access::Center(single);
			Access::Body extra;
			extra.center = {primaryCenter.x < Size.x / 2.f ? 740.f : 220.f, 405.f};
			extra.radius = 48.f;
			extra.kind = vega::visual::PlanetKind::Ice;
			extra.seed = 71.f;
			extra.rotationSpeed = attachments ? -0.04f : 0.06f;
			extra.moonSeed = 90210u;
			extra.moon = attachments;
			if (attachments) extra.ring = vega::visual::PlanetRing{};
			Access::SetAdditionalPlanetBodies(multiple, {extra});
			const auto two = capture.Frame(multiple, time, shifted);
			Require(multiTrace.size() == (attachments ? 8u : 4u), "Extra body submission count differs");
			for (std::size_t i = 0; i < singleTrace.size(); ++i) Require(singleTrace[i] == multiTrace[i], "Entry 0 parameters/order changed with additional body");
			Require(Owner(single) == Owner(multiple), "Extra body overwrote entry 0 center/radius/light/moon attachment");
			const auto& p0 = multiTrace[1]["planet"];
			const auto& p1 = multiTrace[attachments ? 5 : 3]["planet"];
			Require(p0["seed"] == vega::visual::PlanetParams{}.seed && p1["seed"] == extra.seed && p0["seed"] != p1["seed"], "Seeds not independent");
			Require(p0["light"] != p1["light"], "Light direction was reused from entry 0");
			Require(p1["rotation"] == time * extra.rotationSpeed, "Signed extra rotation not applied");
			Require(p1["ring"].is_null() != attachments, "Extra ring flag not applied");
			if (attachments)
			{
				const char* order[] = {"back-ring", "back-moon", "body", "front-ring", "front-moon"};
				for (std::size_t i = 0; i < 5; ++i) Require(multiTrace[i + 3]["index"] == 1 && multiTrace[i + 3]["pass"] == order[i], "Extra body layering differs");
			}
			const auto center = sf::Vector2f{p1["center"][0].get<float>(), p1["center"][1].get<float>()};
			std::size_t changedInside = 0, changedOutside = 0, ownerPixels = 0, ownerChanged = 0;
			const float ownerReach = Access::Radius(single) * 2.f;
			for (unsigned y = 0; y < Size.y; ++y) for (unsigned x = 0; x < Size.x; ++x)
			{
				const bool changed = one.getPixel({x, y}) != two.getPixel({x, y});
				const bool inside = std::abs(static_cast<float>(x) - center.x) <= 125.f && std::abs(static_cast<float>(y) - center.y) <= 125.f;
				if (changed) { if (inside) ++changedInside; else ++changedOutside; }
				if (std::abs(static_cast<float>(x) - primaryCenter.x) < ownerReach && std::abs(static_cast<float>(y) - primaryCenter.y) < ownerReach)
				{
					++ownerPixels; if (changed) ++ownerChanged;
				}
			}
			const auto name = "t" + std::to_string(static_cast<int>(time)) + (shifted ? "-shifted" : "-fixed") + (attachments ? "-ring-moon" : "-bare");
			Require(one.saveToFile(root / (name + "-one.png")) && two.saveToFile(root / (name + "-two.png")), "Cannot save two-body evidence");
			evidence.push_back({{"case", name}, {"stop", found->id}, {"changedInside", changedInside}, {"changedOutside", changedOutside}, {"ownerPixels", ownerPixels}, {"ownerChanged", ownerChanged}, {"single", singleTrace}, {"multiple", multiTrace}, {"owner", Owner(multiple)}});
			std::ofstream{root / "two-body.json"} << evidence.dump(2) << '\n';
			Require(changedInside > 1000, "Second body is not visible inside its expected region");
			Require(changedOutside == 0 && ownerPixels > 1000 && ownerChanged == 0, "Additional body changed entry 0 or pixels outside its expected region");
		}
		std::cout << "Two-body visibility, entry 0 parameters/state/pixels, lighting, seeds, signed rotation, ring/moon and layering: " << evidence.size() << " cases passed\n";
		return 0;
	}
}

int main(int argc, char** argv)
{
	try
	{
		if (argc == 4 && std::string{argv[1]} == "--compare") return Compare(argv[2], argv[3]);
		if (argc == 3 && std::string{argv[1]} == "--two-body") return TwoBodies(argv[2]);
		Require(argc == 3 || argc == 6, "Usage: LightYearsBackdropCapture --capture OUTPUT [STOP SECONDS fixed|shifted]");
		Require(std::string{argv[1]} == "--capture", "Unknown capture command");
		return CaptureStops(argv[2], argc == 6 ? argv[3] : "", argc == 6 ? std::stof(argv[4]) : 0.f, argc == 6 && std::string{argv[5]} == "shifted");
	}
	catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
