#include "visual/ShipVisual.h"

#include <algorithm>

#include <nlohmann/json.hpp>

#include "FileUtil.h"

namespace ly::visual
{
	namespace
	{
		using Json = nlohmann::json;

		sf::Vector2f ReadVec(const Json& json)
		{
			if (!json.is_array() || json.size() < 2 || !json[0].is_number() || !json[1].is_number())
			{
				throw std::runtime_error("expected a [x, y] pair");
			}
			return {json[0].get<float>(), json[1].get<float>()};
		}

		PartRole ReadRole(const Json& object, const char* key, PartRole fallback)
		{
			if (!object.contains(key))
			{
				return fallback;
			}
			const std::string name = object[key].get<std::string>();
			if (name == "hull") return PartRole::Hull;
			if (name == "plate") return PartRole::Plate;
			if (name == "dark") return PartRole::Dark;
			if (name == "accent") return PartRole::Accent;
			if (name == "trim") return PartRole::Trim;
			if (name == "glass") return PartRole::Glass;
			throw std::runtime_error("unknown part role \"" + name + "\"");
		}

		unsigned HashName(const std::string& name)
		{
			unsigned hash = 2166136261u;
			for (char c : name)
			{
				hash = (hash ^ static_cast<unsigned char>(c)) * 16777619u;
			}
			return hash;
		}

		sf::Vector2f Mirror(sf::Vector2f p)
		{
			return {-p.x, p.y};
		}

		sf::Vector2f Centroid(const std::vector<sf::Vector2f>& points)
		{
			sf::Vector2f sum;
			for (sf::Vector2f p : points)
			{
				sum += p;
			}
			return points.empty() ? sf::Vector2f() : sum / static_cast<float>(points.size());
		}

		void ParseParts(const Json& json, ShipVisualDefinition& out)
		{
			for (const Json& item : json)
			{
				ShipPart part;
				part.name = item.at("name").get<std::string>();
				for (const Json& p : item.at("points"))
				{
					part.points.push_back(ReadVec(p));
				}
				if (part.points.size() < 3)
				{
					throw std::runtime_error("part \"" + part.name + "\" needs at least 3 points");
				}
				part.role = ReadRole(item, "role", PartRole::Hull);
				part.shade = item.value("shade", 0);
				part.gap = item.value("gap", false);
				part.crackable = item.value("crack", true);
				part.seed = item.value("seed", HashName(part.name));
				int detachRight = 0;
				int detachLeft = 0;
				if (item.contains("detach"))
				{
					const Json& detach = item["detach"];
					detachRight = detach.is_array() ? detach.at(0).get<int>() : detach.get<int>();
					detachLeft = detach.is_array() ? detach.at(1).get<int>() : detachRight;
				}
				part.detachStage = std::clamp(detachRight, 0, 3);
				part.centroid = Centroid(part.points);
				out.parts.push_back(part);
				if (item.value("mirror", false))
				{
					out.parts.back().paired = true;
					ShipPart left = part;
					left.paired = true;
					left.name += "_L";
					left.mirrored = true;
					left.seed = part.seed * 2654435761u + 977u;
					left.detachStage = std::clamp(detachLeft, 0, 3);
					left.points.clear();
					for (auto it = part.points.rbegin(); it != part.points.rend(); ++it)
					{
						left.points.push_back(Mirror(*it));
					}
					left.centroid = Mirror(part.centroid);
					out.parts.push_back(left);
				}
			}
		}

		void ParseLines(const Json& json, ShipVisualDefinition& out)
		{
			for (const Json& item : json)
			{
				ShipLine line;
				line.part = item.value("part", std::string());
				line.a = ReadVec(item.at("from"));
				line.b = ReadVec(item.at("to"));
				line.role = ReadRole(item, "role", PartRole::Dark);
				out.lines.push_back(line);
				if (item.value("mirror", false))
				{
					ShipLine left = line;
					left.a = Mirror(line.a);
					left.b = Mirror(line.b);
					if (!left.part.empty())
					{
						left.part += "_L";
					}
					out.lines.push_back(left);
				}
			}
		}

		void ParseLights(const Json& json, ShipVisualDefinition& out)
		{
			for (const Json& item : json)
			{
				ShipLight light;
				light.position = ReadVec(item.at("pos"));
				light.size = item.value("size", light.size);
				light.role = ReadRole(item, "role", PartRole::Trim);
				light.blink = item.value("blink", 0.f);
				out.lights.push_back(light);
				if (item.value("mirror", false))
				{
					light.position = Mirror(light.position);
					out.lights.push_back(light);
				}
			}
		}

		void ParseEngines(const Json& json, ShipVisualDefinition& out)
		{
			for (const Json& item : json)
			{
				ShipEngineMount mount;
				mount.position = ReadVec(item.at("pos"));
				mount.size = item.value("size", mount.size);
				out.engines.push_back(mount);
				if (item.value("mirror", false))
				{
					mount.position = Mirror(mount.position);
					out.engines.push_back(mount);
				}
			}
		}

		void ParseDefinition(const Json& json, ShipVisualDefinition& out)
		{
			out.id = json.at("id").get<std::string>();
			if (out.id.empty())
			{
				throw std::runtime_error("empty ship id");
			}
			if (json.contains("bindings"))
			{
				out.bindings = json["bindings"].get<std::vector<std::string>>();
			}
			out.scale = std::clamp(json.value("scale", out.scale), 0.1f, 10.f);
			if (json.contains("roles"))
			{
				const Json& roles = json["roles"];
				out.hullRole = roles.value("hull", out.hullRole);
				out.plateRole = roles.value("plate", out.hullRole);
				out.accentRole = roles.value("accent", out.accentRole);
				out.glassRole = roles.value("glass", out.glassRole);
			}
			if (json.contains("shield"))
			{
				out.shieldRadii = ReadVec(json["shield"]);
			}
			if (json.contains("smoke"))
			{
				out.smokeOrigin = ReadVec(json["smoke"]);
			}
			ParseParts(json.at("parts"), out);
			sf::Vector2f lo(1e9f, 1e9f);
			sf::Vector2f hi(-1e9f, -1e9f);
			for (const ShipPart& part : out.parts)
			{
				for (sf::Vector2f p : part.points)
				{
					lo = {std::min(lo.x, p.x), std::min(lo.y, p.y)};
					hi = {std::max(hi.x, p.x), std::max(hi.y, p.y)};
				}
			}
			out.size = {std::max(1.f, hi.x - lo.x), std::max(1.f, hi.y - lo.y)};
			if (json.contains("lines")) ParseLines(json["lines"], out);
			if (json.contains("lights")) ParseLights(json["lights"], out);
			if (json.contains("engines")) ParseEngines(json["engines"], out);
			if (out.engines.empty())
			{
				throw std::runtime_error("a ship needs at least one engine mount");
			}
			const Json& ports = json.at("ports");
			if (!ports.is_array() || ports.size() != ShipPortCount)
			{
				throw std::runtime_error("\"ports\" needs exactly 4 entries (ability slots Q, E, F, R)");
			}
			for (int i = 0; i < ShipPortCount; ++i)
			{
				out.ports[i].position = ReadVec(ports[i].at("pos"));
				out.ports[i].size = ports[i].value("size", out.ports[i].size);
			}
		}
	}

	const char* PortRoleName(AbilityCategory category)
	{
		switch (category)
		{
		case AbilityCategory::Primary: return "portPrimary";
		case AbilityCategory::Offense: return "portOffense";
		case AbilityCategory::Defense: return "portDefense";
		case AbilityCategory::Movement: return "portMovement";
		case AbilityCategory::Utility: return "portUtility";
		}
		return "portUtility";
	}

	bool ShipVisualLibrary::ParseFile(const std::filesystem::path& file, ShipVisualDefinition& out, std::string& error)
	{
		std::string text;
		if (!ReadTextFile(file, text))
		{
			error = "cannot read " + file.string();
			return false;
		}
		try
		{
			ShipVisualDefinition definition;
			ParseDefinition(Json::parse(text), definition);
			out = std::move(definition);
			return true;
		}
		catch (const std::exception& e)
		{
			error = e.what();
			return false;
		}
	}

	void ShipVisualLibrary::LoadDirectory(const std::filesystem::path& dir, ReloadReport& report)
	{
		mDirectory = dir;
		mEntries.clear();
		mSeenStamps.clear();
		Reload(true, report);
	}

	void ShipVisualLibrary::Reload(bool force, ReloadReport& report)
	{
		std::vector<std::filesystem::path> files;
		std::error_code ec;
		for (const auto& item : std::filesystem::directory_iterator(mDirectory, ec))
		{
			if (item.is_regular_file() && item.path().extension() == ".json")
			{
				files.push_back(item.path());
			}
		}
		std::sort(files.begin(), files.end());

		for (const auto& file : files)
		{
			auto stamp = FileStamp(file);
			auto seen = mSeenStamps.find(file);
			if (!force && seen != mSeenStamps.end() && seen->second == stamp)
			{
				continue;
			}
			mSeenStamps[file] = stamp;
			ShipVisualDefinition definition;
			std::string error;
			if (!ParseFile(file, definition, error))
			{
				report.errors.push_back("ship " + file.filename().string() + ": " + error);
				continue;
			}
			auto entry = std::find_if(mEntries.begin(), mEntries.end(), [&](const Entry& e) { return e.file == file; });
			if (entry == mEntries.end())
			{
				mEntries.push_back({file, std::move(definition)});
			}
			else
			{
				entry->definition = std::move(definition);
			}
			report.reloaded.push_back("ship " + file.filename().string());
		}

		mEntries.erase(std::remove_if(mEntries.begin(), mEntries.end(), [&](const Entry& e) {
			return std::find(files.begin(), files.end(), e.file) == files.end();
		}), mEntries.end());
		std::sort(mEntries.begin(), mEntries.end(), [](const Entry& a, const Entry& b) { return a.file < b.file; });
	}

	const ShipVisualDefinition* ShipVisualLibrary::Find(const std::string& id) const
	{
		for (const Entry& entry : mEntries)
		{
			if (entry.definition.id == id)
			{
				return &entry.definition;
			}
		}
		return nullptr;
	}

	const ShipVisualDefinition* ShipVisualLibrary::FindByBinding(const std::string& externalId) const
	{
		for (const Entry& entry : mEntries)
		{
			const auto& bindings = entry.definition.bindings;
			if (std::find(bindings.begin(), bindings.end(), externalId) != bindings.end())
			{
				return &entry.definition;
			}
		}
		return nullptr;
	}

	std::vector<std::string> ShipVisualLibrary::Ids() const
	{
		std::vector<std::string> ids;
		for (const Entry& entry : mEntries)
		{
			ids.push_back(entry.definition.id);
		}
		return ids;
	}
}
