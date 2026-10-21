#include "presentation/visual/ProceduralBackdrop.h"
#include "gameplay/galaxy/GalaxyRoute.h"
#include "level/GameSceneSettings.h"
#include "BackdropTestAccess.h"
#include <framework/EngineContext.h>
#include <framework/World.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
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
	constexpr float DegToRad = 0.01745329252f;

	const Json& BodyOf(const Json& trace, std::size_t index)
	{
		for (const auto& submission : trace)
		{
			if (submission["index"].get<std::size_t>() == index && submission["pass"] == "body") return submission;
		}
		throw std::runtime_error("Missing body submission for planet index " + std::to_string(index));
	}

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
	{		std::filesystem::create_directories(root);
		const auto route = LoadRoute();
		Capture capture;
		// Shafts are screen-wide by design; the region invariant below is about placement, so it runs with them off.
		capture.assets.Settings().lightShafts.enabled = false;
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

	Json AuthoredExtra(float x, float y, float scale, const std::string& kind, float rotation, const Json& drift = Json())
	{
		Json planet{{"at", {x, y}}, {"scale", scale}, {"kind", kind}, {"rotationDegreesPerMinute", rotation}};
		if (!drift.is_null()) planet["driftPerMinute"] = drift;
		return Json{{"planet", std::move(planet)}};
	}

	Json AttachedExtra(float x, float y, const std::string& kind)
	{
		Json extra = AuthoredExtra(x, y, 0.12f, kind, -18.f);
		extra["orbit"] = {{"moon", true}, {"ring", {{"inner", 1.25}, {"outer", 1.9}, {"opacity", 0.6}}}};
		return extra;
	}

	bool Identical(const sf::Image& a, const sf::Image& b)
	{
		return a.getSize() == b.getSize() && std::equal(a.getPixelsPtr(), a.getPixelsPtr() + static_cast<std::size_t>(a.getSize().x) * a.getSize().y * 4, b.getPixelsPtr());
	}

	struct Box { float x0, y0, x1, y1; };

	Box ExtraBox(const Json& planet)
	{
		const float x = planet.at("center")[0].get<float>(), y = planet.at("center")[1].get<float>();
		const float reach = std::max(planet.at("radius").get<float>() * 3.2f, 120.f);
		return {x - reach, y - reach, x + reach, y + reach};
	}

	std::size_t ChangedOutside(const sf::Image& before, const sf::Image& after, const std::vector<Box>& boxes)
	{
		std::size_t changed = 0;
		for (unsigned y = 0; y < Size.y; ++y) for (unsigned x = 0; x < Size.x; ++x)
		{
			if (before.getPixel({x, y}) == after.getPixel({x, y})) continue;
			const bool inside = std::any_of(boxes.begin(), boxes.end(), [&](const Box& box) { return x >= box.x0 && x <= box.x1 && y >= box.y0 && y <= box.y1; });
			if (!inside) ++changed;
		}
		return changed;
	}

	std::size_t ChangedInside(const sf::Image& before, const sf::Image& after, const Box& box)
	{
		std::size_t changed = 0;
		for (unsigned y = 0; y < Size.y; ++y) for (unsigned x = 0; x < Size.x; ++x)
		{
			if (x < box.x0 || x > box.x1 || y < box.y0 || y > box.y1) continue;
			if (before.getPixel({x, y}) != after.getPixel({x, y})) ++changed;
		}
		return changed;
	}

	struct Shot { sf::Image image; Json trace; Json owner; };

	int EditorGeometry(const std::filesystem::path& root)
	{
		std::filesystem::create_directories(root);
		Capture capture;
		const auto route = LoadRoute();
		Json evidence = Json::array();
		struct GeometryShot { vega::ProceduralBackdrop::SkyGeometry geometry; Json trace, owner; };
		for (bool mirrored : {false, true})
		{
			const auto stop = std::find_if(route.Stops().begin(), route.Stops().end(), [&](const auto& candidate)
			{
				vega::ProceduralBackdrop backdrop;
				backdrop.SetRouteStop(candidate);
				return !candidate.planets.empty() && backdrop.EditorGeometry(capture.assets, Size).mirrored == mirrored;
			});
			Require(stop != route.Stops().end(), "Missing mirrored/unmirrored route fixture");
			const auto shoot = [&](const Json& sky, float time, bool shifted)
			{
				std::string error;
				Require(capture.assets.PreviewDeepSky(Json{{"stops", {{stop->id, sky}}}}, error), error);
				vega::ProceduralBackdrop backdrop;
				backdrop.SetRouteStop(*stop);
				capture.Frame(backdrop, 0.f, false);
				Json trace = Json::array();
				Observe(backdrop, trace);
				capture.Frame(backdrop, time, shifted);
				return GeometryShot{backdrop.EditorGeometry(capture.assets, Size), std::move(trace), Owner(backdrop)};
			};
			for (const std::string scene : {"ordinary", "disk", "horizon"}) for (bool legacy : {false, true})
			{
				Json owner{{"planet", {{"at", {0.12f, 0.18f}}, {"scale", 0.8f}}}};
				if (scene != "ordinary") owner["orbit"] = {{"kind", "terran"}, {"framing", scene}, {"diskRadius", 0.24f}, {"degreesPerMinute", 9.f}};
				Json single = legacy ? owner : Json{{"planets", Json::array({owner})}};
				Json multiple = single;
				if (legacy) multiple["planets"] = Json::array();
				multiple["planets"].push_back(Json{{"planet", {{"at", {0.22f, 0.31f}}, {"scale", 0.45f}, {"kind", "ice"}, {"driftPerMinute", {0.03f, -0.02f}}}}});
				// An unauthored orbit position must remain at the framing origin, not an absolute screen fraction.
				multiple["planets"].push_back(Json{{"planet", {{"scale", 0.7f}, {"kind", "gasGiant"}}}});
				if (scene != "ordinary") multiple["planets"].back()["orbit"] = {{"framing", "disk"}, {"diskRadius", 0.17f}};
				for (float time : {0.f, 37.f}) for (bool shifted : {false, true}) for (bool hidden : {false, true})
				{
					const auto control = shoot(single, time, shifted);
					multiple["planets"].back()["planet"]["hide"] = hidden;
					const auto drawn = shoot(multiple, time, shifted);
					const auto& g = drawn.geometry;
					const auto& before = control.geometry;
					Require(g.planetAt == before.planetAt && g.bandDrift == before.bandDrift && g.planetRadius == before.planetRadius && g.planetBaseScale == before.planetBaseScale && g.band == before.band && g.mirrored == before.mirrored && g.hasPlanet == before.hasPlanet, "Extras changed an existing EditorGeometry field");
					Require(drawn.owner == control.owner, "Extras changed rendered entry 0");
					Require(!legacy || before.planets.empty(), "Legacy-only stop must leave per-entry geometry empty");
					Require(g.planets.size() == 3 && g.mirrored == mirrored, "Incorrect resolved count or mirroring");
					Json visible = multiple;
					visible["planets"].back()["planet"]["hide"] = false;
					const auto revealed = hidden ? shoot(visible, time, shifted) : drawn;
					Json unit = visible;
					for (auto& entry : unit["planets"]) entry["planet"]["scale"] = 1.f;
					if (legacy) unit["planet"]["scale"] = 1.f;
					const auto unscaled = shoot(unit, time, shifted);
					for (std::size_t i = 0; i < 3; ++i)
					{
						const auto& entry = g.planets[i];
						// Orbit owner uses its last rendered owner state; all other bodies have submission records.
						const Json body = i == 0 && scene != "ordinary" ? revealed.owner : BodyOf(revealed.trace, i).at("planet");
						const Json base = i == 0 && scene != "ordinary" ? unscaled.owner : BodyOf(unscaled.trace, i).at("planet");
						const float dx = std::abs(entry.at.x * Size.x - body.at("center")[0].get<float>());
						const float dy = std::abs(entry.at.y * Size.y - body.at("center")[1].get<float>());
						const float dr = std::abs(entry.radius * Size.y - body.at("radius").get<float>());
						const float db = std::abs(entry.baseScale * capture.assets.Settings().planet.radius * Size.y - base.at("radius").get<float>());
						Require(entry.index == i && entry.hasBody && entry.visible == !(hidden && i == 2), "Wrong entry identity/body/visibility");
						Require(dx < 1.f && dy < 1.f && dr < 1.f && db < 1.f, "Editor geometry differs from rendered body by at least a pixel");
						if (hidden && i == 2) Require(std::none_of(drawn.trace.begin(), drawn.trace.end(), [](const auto& item) { return item.at("index") == 2; }), "Hidden body submitted");
						evidence.push_back({{"stop", stop->id}, {"scene", scene}, {"legacyOwner", legacy}, {"time", time}, {"shifted", shifted}, {"hidden", hidden}, {"index", i}, {"centerErrorPixels", {dx, dy}}, {"radiusErrorPixels", dr}, {"baseRadiusErrorPixels", db}, {"at", V2(entry.at)}, {"radius", entry.radius}, {"baseScale", entry.baseScale}, {"drawn", body}});
					}
				}
			}
		}
		std::ofstream{root / "editor-geometry.json"} << evidence.dump(2) << '\n';
		std::cout << "Editor geometry agrees with rendered bodies: " << evidence.size() << " entry checks\n";
		return 0;
	}

	int PlanetData(const std::filesystem::path& root)
	{
		std::filesystem::create_directories(root);
		Capture capture;
		// Light shafts are screen-wide by design (a backlit extra planet's atmosphere casts rays across the sky), so the
		// placement invariant below, "extra bodies change only their own region", is checked with shafts off.
		capture.assets.Settings().lightShafts.enabled = false;
		const auto route = LoadRoute();
		const auto findStop = [&](const std::string& id) -> const vega::RouteStop&
		{
			const auto found = std::find_if(route.Stops().begin(), route.Stops().end(), [&](const auto& stop) { return stop.id == id; });
			Require(found != route.Stops().end(), "Missing route stop " + id);
			return *found;
		};
		std::ifstream input{Source / "LightYearsVisual/assets/deepsky.json"};
		Json catalog; input >> catalog;
		const std::string normalId = "hd-168443", orbitId = "hd-160691";
		Require(catalog.at("stops").contains(normalId) && catalog.at("stops").contains(orbitId), "Missing sky fixture stops");
		const Json normal = catalog.at("stops").at(normalId);
		const Json orbit = catalog.at("stops").at(orbitId);
		const auto install = [&](const std::string& id, const Json& stop)
		{
			std::string error;
			const Json document{{"stops", {{id, stop}}}};
			Require(capture.assets.PreviewDeepSky(document, error), error);
		};
		const auto shoot = [&](const std::string& id, const Json& stop, float time)
		{
			install(id, stop);
			vega::ProceduralBackdrop backdrop;
			backdrop.SetRouteStop(findStop(id));
			capture.Frame(backdrop, 0.f, false);
			Json trace = Json::array();
			Observe(backdrop, trace);
			Shot shot{capture.Frame(backdrop, time, false), std::move(trace), Owner(backdrop)};
			return shot;
		};

		Json single = normal;
		single["planets"] = Json::array({Json::object()});
		Json three = normal;
		three["planets"] = Json::array({Json::object(), AttachedExtra(0.15f, 0.1f, "ice"), AttachedExtra(0.5f, 0.1f, "gasGiant"), AttachedExtra(0.85f, 0.1f, "lava")});
		Json evidence{{"stop", normalId}, {"entryZeroParity", Json::array()}, {"threeBodies", Json::array()}};
		for (float time : {0.f, 30.f, 120.f})
		{
			const auto control = shoot(normalId, normal, time);
			const auto one = shoot(normalId, single, time);
			Require(Identical(control.image, one.image), "Explicit entry 0 changed the generated single-planet composition");
			Require(control.trace == one.trace && control.owner == one.owner, "Explicit entry 0 parameters/state differ from the generated path");
			const auto bodies = shoot(normalId, three, time);
			Require(bodies.trace.size() == 18, "Entry 0 plus three ring/moon bodies must submit 18 passes");
			for (std::size_t i = 0; i < 3; ++i) Require(bodies.trace[i] == one.trace[i], "Additional bodies changed entry 0 submissions");
			Require(bodies.owner == one.owner, "Additional bodies changed entry 0 owner state");
			std::vector<Box> boxes;
			for (std::size_t i = 0; i < 3; ++i)
			{
				const auto& extra = bodies.trace[3 + i * 5];
				const char* order[] = {"back-ring", "back-moon", "body", "front-ring", "front-moon"};
				for (std::size_t pass = 0; pass < 5; ++pass)
				{
					Require(extra["index"].get<std::size_t>() == i + 1 && bodies.trace[3 + i * 5 + pass]["pass"] == order[pass], "Additional body layering/order differs");
				}
				Require(bodies.trace[3 + i * 5 + 2]["planet"]["kind"] == static_cast<int>(i == 0 ? vega::visual::PlanetKind::Ice : (i == 1 ? vega::visual::PlanetKind::GasGiant : vega::visual::PlanetKind::Lava)), "Authored extra kind not applied");
				boxes.push_back(ExtraBox(bodies.trace[3 + i * 5 + 2]["planet"]));
			}
			const auto outside = ChangedOutside(one.image, bodies.image, boxes);
			Require(outside == 0, "Additional bodies changed pixels outside their regions");
			const auto name = "t" + std::to_string(static_cast<int>(time));
			Require(one.image.saveToFile(root / (name + "-single.png")) && bodies.image.saveToFile(root / (name + "-three.png")), "Cannot save planets data evidence");
			evidence["entryZeroParity"].push_back({{"timeSeconds", time}, {"pixels", PixelHash(one.image)}, {"identical", true}});
			evidence["threeBodies"].push_back({{"timeSeconds", time}, {"changedOutside", outside}, {"owner", bodies.owner}, {"submissions", bodies.trace.size()}});
		}

		// Authored kind, signed rotation, drift and mirroring; drift clamps just outside the output.
		Json motion = normal;
		motion["planets"] = Json::array({Json::object(), AuthoredExtra(0.2f, 0.3f, 0.2f, "ice", 30.f, Json::array({0.02f, 0.01f}))});
		const auto moved = shoot(normalId, motion, 60.f);
		Require(moved.trace.size() == 4, "One bare extra body must submit one body pass");
		const auto& body = BodyOf(moved.trace, 1)["planet"];
		Require(body["kind"] == static_cast<int>(vega::visual::PlanetKind::Ice), "Authored extra kind not applied");
		Require(std::abs(body["rotation"].get<float>() - 30.f * DegToRad) < 1e-4f, "Authored signed rotation is not degrees per minute");
		const bool mirrored = moved.owner["center"][0].get<float>() < Size.x / 2.f;
		const float expectedX = (mirrored ? 0.8f : 0.2f) * Size.x + 0.02f * Size.x;
		const float expectedY = 0.3f * Size.y + 0.01f * Size.y;
		Require(std::abs(body["center"][0].get<float>() - expectedX) < 0.01f && std::abs(body["center"][1].get<float>() - expectedY) < 0.01f, "Extra at/drift placement or mirroring is wrong");
		Json negative = normal;
		negative["planets"] = Json::array({Json::object(), AuthoredExtra(0.2f, 0.3f, 0.2f, "ice", -30.f)});
		const auto reverse = shoot(normalId, negative, 60.f);
		Require(std::abs(BodyOf(reverse.trace, 1)["planet"]["rotation"].get<float>() + 30.f * DegToRad) < 1e-4f, "Negative authored rotation is not applied");
		const auto clamped = shoot(normalId, motion, 6000.f);
		const auto& clampedCenter = BodyOf(clamped.trace, 1)["planet"]["center"];
		Require(std::abs(clampedCenter[0].get<float>() - 1.25f * Size.x) < 0.01f && std::abs(clampedCenter[1].get<float>() - 1.25f * Size.y) < 0.01f, "Authored drift is not clamped near the screen");
		evidence["motion"] = {{"mirrored", mirrored}, {"center", body["center"]}, {"rotation", body["rotation"]}, {"clamped", clampedCenter}};

		// Two times and both signs: the authored rotation changes the surface and reverses its phase.
		Json spin = normal;
		spin["planets"] = Json::array({Json::object(), AuthoredExtra(0.5f, 0.3f, 0.2f, "ice", 30.f)});
		const auto spinEarly = shoot(normalId, spin, 30.f);
		const auto spinLate = shoot(normalId, spin, 60.f);
		const Box spinBox = ExtraBox(BodyOf(spinEarly.trace, 1)["planet"]);
		Require(ChangedInside(spinEarly.image, spinLate.image, spinBox) > 100, "Authored rotation does not change the surface over time");
		Json spinBack = normal;
		spinBack["planets"] = Json::array({Json::object(), AuthoredExtra(0.5f, 0.3f, 0.2f, "ice", -30.f)});
		const auto spinReverse = shoot(normalId, spinBack, 30.f);
		Require(ChangedInside(spinEarly.image, spinReverse.image, spinBox) > 100, "Reversing the authored rotation sign does not reverse the phase");
		const auto driftEarly = shoot(normalId, motion, 30.f);
		Require(std::abs(BodyOf(driftEarly.trace, 1)["planet"]["center"][0].get<float>() - (BodyOf(moved.trace, 1)["planet"]["center"][0].get<float>() - 0.02f * Size.x * 0.5f)) < 0.01f, "Authored drift does not advance linearly over time");
		evidence["spin"] = {{"timeChangedPixels", ChangedInside(spinEarly.image, spinLate.image, spinBox)}, {"signChangedPixels", ChangedInside(spinEarly.image, spinReverse.image, spinBox)}};

		// Hidden extras submit nothing; the parser caps the effective list at eight.
		Json hidden = normal;
		Json middle = AuthoredExtra(0.15f, 0.12f, 0.1f, "terran", 0.f);
		middle["planet"]["hide"] = true;
		hidden["planets"] = Json::array({Json::object(), AuthoredExtra(0.5f, 0.12f, 0.1f, "desert", 0.f), middle, AuthoredExtra(0.5f, 0.85f, 0.1f, "ocean", 0.f)});
		const auto hiding = shoot(normalId, hidden, 15.f);
		Require(hiding.trace.size() == 5, "Hidden extra body must not submit");
		Require(std::none_of(hiding.trace.begin(), hiding.trace.end(), [](const Json& item) { return item["index"].get<std::size_t>() == 2; }), "Hidden extra still submitted passes");
		Json eight = normal;
		eight["planets"] = Json::array();
		for (int i = 0; i < 8; ++i) eight["planets"].push_back(Json::object());
		install(normalId, eight);
		Json nine = normal;
		nine["planets"] = Json::array();
		for (int i = 0; i < 9; ++i) nine["planets"].push_back(Json::object());
		std::string capError;
		Require(!capture.assets.PreviewDeepSky(Json{{"stops", {{normalId, nine}}}}, capError), "Nine effective planets must be rejected");
		Require(capError.find("eight") != std::string::npos, "Planet cap diagnostic does not name the limit");
		evidence["hideAndCap"] = {{"hiddenSubmissions", hiding.trace.size()}, {"capError", capError}};

		// Orbit scene: additional entries inherit the scene, suppression and shared owner state.
		Json orbitExtras = orbit;
		orbitExtras["planets"] = Json::array({AttachedExtra(0.15f, 0.25f, "ice"), AuthoredExtra(0.8f, 0.2f, 0.1f, "gasGiant", 12.f)});
		const auto orbitControl = shoot(orbitId, orbit, 30.f);
		const auto orbitBodies = shoot(orbitId, orbitExtras, 30.f);
		const std::size_t ownerPasses = orbitControl.trace.size();
		Require(orbitBodies.trace.size() == ownerPasses + 6, "Orbit owner plus a ring/moon extra and a bare extra must submit owner + 5 + 1 passes");
		for (std::size_t i = 0; i < ownerPasses; ++i) Require(orbitBodies.trace[i] == orbitControl.trace[i], "Extras changed the orbit owner submissions");
		Require(orbitBodies.owner == orbitControl.owner, "Extras changed the orbit owner state");
		std::vector<Box> orbitBoxes{ExtraBox(BodyOf(orbitBodies.trace, 1)["planet"]), ExtraBox(BodyOf(orbitBodies.trace, 2)["planet"])};
		Require(ChangedOutside(orbitControl.image, orbitBodies.image, orbitBoxes) == 0, "Orbit extras changed pixels outside their regions");

		Json suppressed = orbitExtras;
		for (const auto& item : catalog.at("stops").items())
		{
			if (!suppressed.contains("nebula") && item.value().contains("nebula")) suppressed["nebula"] = item.value().at("nebula");
			if (!suppressed.contains("skyPaint") && item.value().contains("skyPaint")) suppressed["skyPaint"] = item.value().at("skyPaint");
			if (!suppressed.contains("asteroids") && item.value().contains("asteroids")) suppressed["asteroids"] = item.value().at("asteroids");
		}
		Json suppressedObject = Json::object();
		suppressedObject["kind"] = "emission";
		suppressedObject["at"] = Json::array({0.5f, 0.5f});
		suppressedObject["size"] = 0.5f;
		suppressed["objects"] = Json::array({suppressedObject});
		const auto suppressedShot = shoot(orbitId, suppressed, 30.f);
		Require(Identical(orbitBodies.image, suppressedShot.image), "Orbit extras let objects/nebula/skyPaint/asteroids render");

		// A legacy orbit owner still ignores planet.at/scale/hide.
		Json legacyHidden = orbit;
		legacyHidden["planet"] = {{"at", {0.3, 0.2}}, {"scale", 2.f}, {"hide", true}};
		const auto legacyShot = shoot(orbitId, legacyHidden, 30.f);
		Require(Identical(orbitControl.image, legacyShot.image), "Legacy orbit owner stopped ignoring at/scale/hide");

		// An explicit orbit owner applies its authored at/scale and still owns the scene.
		Json explicitOrbit = orbit;
		Json orbitSettings = explicitOrbit.at("orbit");
		explicitOrbit.erase("orbit");
		Json explicitOwner = Json::object();
		explicitOwner["planet"] = Json{{"at", Json::array({0.1f, 0.2f})}, {"scale", 0.5f}};
		explicitOwner["orbit"] = orbitSettings;
		explicitOrbit["planets"] = Json::array({explicitOwner});
		const auto explicitShot = shoot(orbitId, explicitOrbit, 30.f);
		const auto& legacyCenter = orbitControl.owner.at("center");
		const auto& explicitCenter = explicitShot.owner.at("center");
		const float dx = explicitCenter[0].get<float>() - legacyCenter[0].get<float>();
		const float dy = explicitCenter[1].get<float>() - legacyCenter[1].get<float>();
		Require(std::abs(std::abs(dx) - 0.1f * Size.x) < 0.01f && std::abs(dy - 0.2f * Size.y) < 0.01f, "Explicit orbit owner at offset is not a framing-centre offset");
		Require(std::abs(explicitShot.owner.at("radius").get<float>() - orbitControl.owner.at("radius").get<float>() * 0.5f) < 0.01f, "Explicit orbit owner scale not applied to geometry");
		evidence["orbit"] = {{"submissions", orbitBodies.trace.size()}, {"changedOutside", 0}, {"owner", orbitControl.owner}, {"explicitOffset", {dx, dy}}, {"legacyIgnored", true}};

		std::ofstream{root / "planets-data.json"} << evidence.dump(2) << '\n';
		std::cout << "Planet data path: entry 0 parity, three bodies, motion, hide, cap, orbit scene and suppression passed\n";
		return 0;
	}
}

int main(int argc, char** argv)
{
	try
	{
		if (argc == 4 && std::string{argv[1]} == "--compare") return Compare(argv[2], argv[3]);
		if (argc == 3 && std::string{argv[1]} == "--two-body") return TwoBodies(argv[2]);
		if (argc == 3 && std::string{argv[1]} == "--planets-data") return PlanetData(argv[2]);
		if (argc == 3 && std::string{argv[1]} == "--editor-geometry") return EditorGeometry(argv[2]);
		Require(argc == 3 || argc == 6, "Usage: LightYearsBackdropCapture --capture OUTPUT [STOP SECONDS fixed|shifted]");
		Require(std::string{argv[1]} == "--capture", "Unknown capture command");
		return CaptureStops(argv[2], argc == 6 ? argv[3] : "", argc == 6 ? std::stof(argv[4]) : 0.f, argc == 6 && std::string{argv[5]} == "shifted");
	}
	catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
