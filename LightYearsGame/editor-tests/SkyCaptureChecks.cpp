#include "editor/SkyDocument.h"
#include "framework/EngineContext.h"
#include <SFML/Graphics/Image.hpp>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <cmath>

namespace
{
	int failures = 0;
	void Check(bool value, const std::string& message)
	{
		if (!value) { std::cerr << message << '\n'; ++failures; }
	}
	std::string Read(const std::filesystem::path& path)
	{
		std::ifstream input{path, std::ios::binary}; return {std::istreambuf_iterator<char>{input}, {}};
	}
	void Write(const std::filesystem::path& path, const std::string& text)
	{
		std::ofstream out{path, std::ios::binary}; out << text;
	}
	// Extract the exact authored entry, including its original whitespace and numeric spelling.
	std::string Entry(const std::string& text, const std::string& id)
	{
		const auto key = text.find('"' + id + '"');
		if (key == std::string::npos) return {};
		const auto start = text.find('{', key);
		int depth = 0; bool quoted = false, escaped = false;
		for (auto i = start; i < text.size(); ++i)
		{
			const char c = text[i];
			if (quoted) { if (escaped) escaped = false; else if (c == '\\') escaped = true; else if (c == '"') quoted = false; }
			else if (c == '"') quoted = true;
			else if (c == '{') ++depth;
			else if (c == '}' && --depth == 0) return text.substr(start, i - start + 1);
		}
		return {};
	}
	std::uint64_t Checksum(const sf::Image& image)
	{
		std::uint64_t hash = 14695981039346656037ULL;
		const auto size = image.getSize();
		for (unsigned y = 0; y < 54; ++y) for (unsigned x = 0; x < 96; ++x)
		{
			unsigned sum[3]{};
			for (unsigned dy = 0; dy < 10; ++dy) for (unsigned dx = 0; dx < 10; ++dx)
			{
				const auto c = image.getPixel({std::min(size.x - 1, x * size.x / 96 + dx), std::min(size.y - 1, y * size.y / 54 + dy)});
				sum[0] += c.r; sum[1] += c.g; sum[2] += c.b;
			}
			for (auto channel : sum) { hash ^= channel / 100; hash *= 1099511628211ULL; }
		}
		return hash;
	}
	int Verify(const std::filesystem::path& root)
	{
		vega::EngineContext engineContext;
		sf::Image before, moved, edited, restored, reopened;
		sf::Image disk, preview;
		Check(disk.loadFromFile(root / "disk-baseline.png") && preview.loadFromFile(root / "unchanged-preview.png"), "Unchanged-data captures missing");
		Check(before.loadFromFile(root / "before.png") && moved.loadFromFile(root / "moved.png") && edited.loadFromFile(root / "edited.png") && restored.loadFromFile(root / "restored.png") && reopened.loadFromFile(root / "reopened.png"), "Sky captures missing");
		if (failures) return 1;
		Check(Checksum(disk) == Checksum(preview), "Opening Sky changed the existing look");
		const auto first = Checksum(before), second = Checksum(edited), undo = Checksum(restored);
		Check(first != second, "Sky edits did not alter downscaled pixels");
		Check(first == undo, "Undo did not restore the downscaled checksum");
		Check(second == Checksum(reopened), "Saved/reopened sky differs");
		std::size_t central = 0, outside = 0, band = 0;
		const auto size = before.getSize();
		for (unsigned y = 0; y < size.y; ++y) for (unsigned x = 0; x < size.x; ++x)
		{
			const sf::Vector2u at{x, y};
			if (before.getPixel(at) != moved.getPixel(at))
			{
				if (x > size.x / 5 && x < size.x * 4 / 5 && y > size.y / 10 && y < size.y * 9 / 10) ++central;
				else ++outside;
			}
			if (moved.getPixel(at) != edited.getPixel(at)) ++band;
		}
		Check(central > 1000 && central > outside, "Planet drag failed to change the expected central region");
		Check(band > 1000, "Band colour failed to change the background");
		const auto original = Read(std::filesystem::path{VEGA_SKY_SOURCE} / "deepsky.json");
		const auto saved = Read(root / "visual/deepsky.json");
		Check(!Entry(original, "gj-676-a").empty() && Entry(original, "gj-676-a") == Entry(saved, "gj-676-a"), "Unrelated stop JSON bytes changed");
		vega::SkyDocument planets{engineContext, root / "visual"};
		Check(planets.Open() && planets.Select("hd-168443"), "Cannot reopen indexed-handle fixture");
		const auto at = planets.Value("planets.0.planet.at");
		Check(at.is_array() && at.size() == 2 && std::abs(at[0].get<float>() - .22f) < 1e-5f && std::abs(at[1].get<float>() - .35f) < 1e-5f, "Second planet drag did not save planets.0.planet.at");
		sf::Image planetsBefore, planetsAfter, planetsReopened;
		Check(planetsBefore.loadFromFile(root / "planets-before.png") && planetsAfter.loadFromFile(root / "planets-after.png") && planetsReopened.loadFromFile(root / "planets-reopened.png"), "Indexed planet captures missing");
		if (!failures)
		{
			Check(Checksum(planetsBefore) != Checksum(planetsAfter), "Indexed planet drag did not change rendered pixels");
			Check(Checksum(planetsAfter) == Checksum(planetsReopened), "Indexed planet save/reopen changed pixels");
		}
		std::cout << "Sky checksums before=" << first << " edited=" << second << " undo=" << undo << "; changed planet central=" << central << " outside=" << outside << " band=" << band << '\n';
		return failures ? 1 : 0;
	}
}

int main(int argc, char** argv)
{
	vega::EngineContext engineContext;
	if (argc == 3 && std::string{argv[1]} == "--verify") return Verify(argv[2]);
	if (argc == 3 && std::string{argv[1]} == "--prepare")
	{
		// Author only the disposable harness copy, preserving unrelated stop bytes through the real document API.
		vega::SkyDocument sky{engineContext, std::filesystem::path{argv[2]} / "visual"};
		Check(sky.Open() && sky.Select("hd-168443"), "Cannot prepare indexed planet fixture");
		const vega::Json entries = vega::Json::array({vega::Json{{"planet", {{"at", {.22, .25}}, {"scale", .45}, {"kind", "ice"}}}}, vega::Json{{"planet", {{"at", {.78, .25}}, {"scale", .3}, {"kind", "lava"}}}}});
		Check(sky.Set("planet.scale", 1.f) && sky.Block("planets", entries) && sky.Save(), "Cannot save indexed planet fixture: " + sky.error);
		return failures ? 1 : 0;
	}
	const std::filesystem::path root{VEGA_SKY_TEST_ROOT};
	std::filesystem::create_directories(root);
	const std::string original = "{\r\n\t\"presets\": {\"soft\": {\"band\": {\"clouds\": 2}, \"objects\": [{\"kind\": \"fog\"}]}},\r\n\t\"stops\": {\r\n\t\t\"one\": {\"use\": \"soft\", \"future\": {\"keep\": 1.2300}},\r\n\t\t\"other\": { \"band\": { \"dust\": 1.000 }, \"future\": [\"keep me\"] }\r\n\t}\r\n}\r\n";
	Write(root / "deepsky.json", original);
	vega::SkyDocument sky{engineContext, root};
	Check(sky.Open() && sky.Select("one"), "Cannot open sky fixture: " + sky.error);
	Check(!sky.document.Dirty() && sky.document.Serialize() == original, "Opening materialized defaults or changed bytes");
	Check(sky.Value("band.clouds") == 2, "Preset did not resolve");
	Check(sky.Value("objects[0].kind") == "fog", "Inherited array missing");
	const auto revision = sky.document.Revision();
	for (const auto& field : vega::sky::DeepSkyFields()) Check(vega::sky::ValidateSkyField(field, field.defaultValue).empty(), "Invalid schema default: " + field.path);
	Check(sky.Refresh() && sky.document.Revision() == revision, "Cached read mutated sky");
	Check(!sky.Set("future.keep", 9) && !sky.Set("planet.scale", 8) && !sky.Set("band.view", "bad") && !sky.Set("planet.at", vega::Json{1,2,3}), "Invalid or unknown edits accepted");
	Check(sky.document.Revision() == revision && !sky.document.Dirty(), "Invalid edits changed revision");
	Check(sky.Set("planet.at", vega::Json{.6, .35}), "Set missing block failed");
	const auto count = sky.history.UndoCount(); const auto group = sky.history.BeginMergeGroup();
	Check(sky.Set("planet.at", vega::Json{.6,.4}, group) && sky.Set("planet.at", vega::Json{.6,.5}, group), "Gesture failed");
	Check(sky.history.UndoCount() == count + 1, "One drag made multiple undo steps");
	Check(sky.history.Undo() && sky.Value("planet.at") == vega::Json({.6,.35}), "Drag undo did not restore start");
	Check(sky.history.Redo() && sky.Value("planet.at") == vega::Json({.6,.5}), "Drag redo failed");
	Check(sky.Reset("planet.at") && !sky.Authored("planet.at"), "Reset did not remove override");
	Check(!sky.Authored("planet"), "Reset left an unintended empty block");
	Check(sky.Set("objects[0].size", .7), "Cannot edit inherited object");
	Check(!sky.Authored("use") && sky.Effective()["objects"].size() == 1 && sky.Parsed().Find("one")->objects.size() == 1, "Inherited object edit duplicated runtime objects");
	Check(sky.Reset("objects[0].kind") && sky.Value("objects.0.kind") == "emission", "Required object kind did not reset to its default");
	Check(sky.DuplicateTo("copy") && sky.Select("copy") && sky.Value("objects.0.size") == .7, "Duplicate failed");
	Check(sky.StorePreset("new") && sky.Select("one") && sky.Set("use", "new"), "Preset workflow failed");
	Check(sky.Block("orbit", vega::Json{{"framing","disk"},{"diskRadius",.3}}), "Orbit block failed");
	const auto version = sky.Parsed().Version();
	Check(!sky.Block("orbit", vega::Json{{"framing","bad"}}) && sky.Parsed().Version() == version, "Bad catalog replaced working data");
	Check(sky.Set("orbit.ring.inner", 1.5) && !sky.Set("orbit.ring.outer", 1.51) && sky.Set("orbit.ring.outer", 2.5), "Ring dependent range failed");
	Check(sky.Save(), "Save failed: " + sky.error);
	Check(Entry(Read(root / "deepsky.json"), "other") == Entry(original, "other"), "Unrelated JSON reformatted");
	vega::SkyDocument loaded{engineContext, root}; Check(loaded.Open() && loaded.Select("one") && loaded.Value("orbit.framing") == "disk", "Reopen lost values");
	Check(loaded.Authored("future") && loaded.Authored("future")->at("keep") == 1.23, "Unknown keys were lost");
	Check(loaded.Set("band.width", .4), "External conflict setup failed");
	const auto external = Read(root / "deepsky.json") + " "; Write(root / "deepsky.json", external);
	Check(!loaded.Save() && Read(root / "deepsky.json") == external, "Save overwrote external changes");
	Write(root / "deepsky.json", original); vega::SkyDocument clean{engineContext, root};
	Check(clean.Open() && clean.Select("one") && clean.Set("band.width", .4) && clean.history.Undo() && !clean.document.Dirty() && clean.document.Serialize() == original, "Undo failed byte-exact clean-state restoration");
	vega::sky::DeepSkyCatalog catalog; std::string error;
	Check(catalog.Load(std::filesystem::path{VEGA_SKY_SOURCE} / "deepsky.json", error), "Shipped catalog no longer parses: " + error);
	for (const bool single : {false, true})
	{
		const auto file = single ? "single.sky.json" : "collection.sky.json";
		const std::string bytes = single
			? "{\r\n\t\"id\": \"aurora\", \"sky\": {\"band\": {\"width\": 0.2500}, \"future\": 1.2300}\r\n}\r\n"
			: "{\r\n\t\"skies\": {\"aurora\": {\"band\": {\"width\": 0.2500}}, \"other\": {\"future\": 1.2300}}\r\n}\r\n";
		Write(root / file, bytes);
		vega::SkyDocument asset{engineContext, root, file};
		Check(asset.Open() && asset.Select("aurora"), "Cannot open independent sky asset");
		Check(asset.Set("band.width", .4) && asset.history.Undo() && asset.document.Serialize() == bytes, "Sky asset undo changed bytes");
		Check(asset.Set("band.width", .4) && asset.Save(), "Cannot save independent sky asset");
		vega::SkyDocument reopened{engineContext, root, file};
		Check(reopened.Open() && reopened.Select("aurora") && reopened.Value("band.width") == .4, "Sky asset reopen lost values");
		Check(Read(root / file).find("1.2300") != std::string::npos, "Sky asset save rewrote opaque bytes");
	}
	std::cout << "Sky document failures: " << failures << '\n';
	return failures ? 1 : 0;
}
