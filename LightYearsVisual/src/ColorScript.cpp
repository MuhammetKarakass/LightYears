#include "visual/ColorScript.h"

#include <algorithm>

#include <nlohmann/json.hpp>

#include "FileUtil.h"

namespace ly::visual
{
	namespace
	{
		const ColorRamp& MissingRamp()
		{
			static const ColorRamp ramp({{0.f, sf::Color(255, 0, 255)}, {1.f, sf::Color(255, 120, 255)}});
			return ramp;
		}

		const ColorScript& EmptyScript()
		{
			static const ColorScript script;
			return script;
		}

		bool ParseHexColor(const std::string& text, sf::Color& out)
		{
			if ((text.size() != 7 && text.size() != 9) || text[0] != '#')
			{
				return false;
			}
			unsigned value = 0;
			for (size_t i = 1; i < text.size(); ++i)
			{
				char c = text[i];
				unsigned digit = 0;
				if (c >= '0' && c <= '9') digit = c - '0';
				else if (c >= 'a' && c <= 'f') digit = 10 + c - 'a';
				else if (c >= 'A' && c <= 'F') digit = 10 + c - 'A';
				else return false;
				value = value * 16 + digit;
			}
			if (text.size() == 7)
			{
				out = sf::Color((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
			}
			else
			{
				out = sf::Color((value >> 24) & 0xFF, (value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
			}
			return true;
		}

		// A ramp is an array of "#RRGGBB" strings (evenly spaced) and/or {"t": 0..1, "color": "#RRGGBB"} objects.
		bool ParseRamp(const nlohmann::json& json, ColorRamp& out, std::string& error)
		{
			if (!json.is_array() || json.size() < 2)
			{
				error = "a ramp needs an array of at least two stops";
				return false;
			}
			std::vector<ColorStop> stops;
			for (size_t i = 0; i < json.size(); ++i)
			{
				const nlohmann::json& item = json[i];
				ColorStop stop;
				stop.t = static_cast<float>(i) / static_cast<float>(json.size() - 1);
				std::string text;
				if (item.is_string())
				{
					text = item.get<std::string>();
				}
				else if (item.is_object() && item.contains("color") && item["color"].is_string())
				{
					text = item["color"].get<std::string>();
					if (item.contains("t"))
					{
						stop.t = item["t"].get<float>();
					}
				}
				if (!ParseHexColor(text, stop.color))
				{
					error = "stop " + std::to_string(i) + " has no valid #RRGGBB color";
					return false;
				}
				stops.push_back(stop);
			}
			out = ColorRamp(std::move(stops));
			return true;
		}
	}

	bool ColorScript::LoadFromFile(const std::filesystem::path& file, ColorScript& out, std::string& error)
	{
		std::string text;
		if (!ReadTextFile(file, text))
		{
			error = "cannot read " + file.string();
			return false;
		}
		try
		{
			nlohmann::json json = nlohmann::json::parse(text);
			ColorScript script;
			script.mName = json.value("name", file.stem().string());
			if (!json.contains("ramps") || !json["ramps"].is_object() || json["ramps"].empty())
			{
				error = "missing \"ramps\" object";
				return false;
			}
			for (const auto& [name, rampJson] : json["ramps"].items())
			{
				ColorRamp ramp;
				std::string rampError;
				if (!ParseRamp(rampJson, ramp, rampError))
				{
					error = "ramp \"" + name + "\": " + rampError;
					return false;
				}
				script.mRamps[name] = std::move(ramp);
			}
			if (json.contains("roles"))
			{
				for (const auto& [role, rampName] : json["roles"].items())
				{
					std::string target = rampName.get<std::string>();
					if (script.mRamps.find(target) == script.mRamps.end())
					{
						error = "role \"" + role + "\" points at unknown ramp \"" + target + "\"";
						return false;
					}
					script.mRoles[role] = target;
				}
			}
			out = std::move(script);
			return true;
		}
		catch (const nlohmann::json::exception& e)
		{
			error = e.what();
			return false;
		}
	}

	const ColorRamp& ColorScript::Ramp(const std::string& rampName) const
	{
		auto it = mRamps.find(rampName);
		return it == mRamps.end() ? MissingRamp() : it->second;
	}

	const ColorRamp& ColorScript::Role(const std::string& roleName) const
	{
		auto it = mRoles.find(roleName);
		return it == mRoles.end() ? MissingRamp() : Ramp(it->second);
	}

	std::vector<sf::Color> ColorScript::Palette(size_t maxColors) const
	{
		std::vector<sf::Color> colors;
		for (const auto& [name, ramp] : mRamps)
		{
			for (const ColorStop& stop : ramp.Stops())
			{
				if (std::find(colors.begin(), colors.end(), stop.color) == colors.end())
				{
					colors.push_back(stop.color);
				}
			}
		}
		if (colors.size() > maxColors)
		{
			std::vector<sf::Color> reduced;
			for (size_t i = 0; i < maxColors; ++i)
			{
				reduced.push_back(colors[i * colors.size() / maxColors]);
			}
			colors = std::move(reduced);
		}
		return colors;
	}

	bool ColorScriptLibrary::LoadDirectory(const std::filesystem::path& dir, std::string& error)
	{
		mDirectory = dir;
		mEntries.clear();
		mSeenStamps.clear();
		ReloadReport report;
		Reload(true, report);
		if (mEntries.empty())
		{
			error = "no color script loaded from " + dir.string();
			for (const std::string& e : report.errors)
			{
				error += "\n  " + e;
			}
			return false;
		}
		mActive = 0;
		return true;
	}

	void ColorScriptLibrary::Reload(bool force, ReloadReport& report)
	{
		std::string activeName = mEntries.empty() ? std::string() : mEntries[mActive].script.Name();
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
			ColorScript script;
			std::string error;
			if (!ColorScript::LoadFromFile(file, script, error))
			{
				report.errors.push_back("palette " + file.filename().string() + ": " + error);
				continue;
			}
			auto entry = std::find_if(mEntries.begin(), mEntries.end(), [&](const Entry& e) { return e.file == file; });
			if (entry == mEntries.end())
			{
				mEntries.push_back({file, std::move(script)});
			}
			else
			{
				entry->script = std::move(script);
			}
			report.reloaded.push_back("palette " + file.filename().string());
		}

		mEntries.erase(std::remove_if(mEntries.begin(), mEntries.end(), [&](const Entry& e) {
			return std::find(files.begin(), files.end(), e.file) == files.end();
		}), mEntries.end());
		std::sort(mEntries.begin(), mEntries.end(), [](const Entry& a, const Entry& b) { return a.file < b.file; });

		mActive = 0;
		for (size_t i = 0; i < mEntries.size(); ++i)
		{
			if (mEntries[i].script.Name() == activeName)
			{
				mActive = i;
			}
		}
	}

	const ColorScript& ColorScriptLibrary::Active() const
	{
		return mEntries.empty() ? EmptyScript() : mEntries[mActive].script;
	}

	bool ColorScriptLibrary::SetActive(const std::string& name)
	{
		for (size_t i = 0; i < mEntries.size(); ++i)
		{
			if (mEntries[i].script.Name() == name)
			{
				mActive = i;
				return true;
			}
		}
		return false;
	}

	void ColorScriptLibrary::CycleActive(int step)
	{
		if (mEntries.empty())
		{
			return;
		}
		int count = static_cast<int>(mEntries.size());
		mActive = static_cast<size_t>(((static_cast<int>(mActive) + step) % count + count) % count);
	}

	std::vector<std::string> ColorScriptLibrary::Names() const
	{
		std::vector<std::string> names;
		for (const Entry& entry : mEntries)
		{
			names.push_back(entry.script.Name());
		}
		return names;
	}
}
