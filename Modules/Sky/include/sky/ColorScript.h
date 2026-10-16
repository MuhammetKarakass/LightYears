#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "sky/ColorRamp.h"
#include "sky/ReloadReport.h"

namespace vega::sky
{
	// Named ramps plus named roles (background, ambient, combatWarm, player, enemy, ...) that point at ramps.
	class ColorScript
	{
	public:
		static bool LoadFromFile(const std::filesystem::path& file, ColorScript& out, std::string& error);

		const std::string& Name() const { return mName; }
		// Missing names return a loud magenta ramp so a broken script is visible instead of silently gray.
		const ColorRamp& Ramp(const std::string& rampName) const;
		const ColorRamp& Role(const std::string& roleName) const;
		bool HasRole(const std::string& roleName) const { return mRoles.find(roleName) != mRoles.end(); }
		// Every ramp stop of the script, deduplicated, capped at maxColors. Used as the world dither palette.
		// Ramps whose name starts with skipPrefix are left out (the "bg_" ramps stay out of the ship dither).
		std::vector<sf::Color> Palette(size_t maxColors, const std::string& skipPrefix = std::string()) const;

	private:
		std::string mName;
		std::map<std::string, ColorRamp> mRamps;
		std::map<std::string, std::string> mRoles;
	};

	class ColorScriptLibrary
	{
	public:
		// Loads every *.json in dir. Fails when no script loads.
		bool LoadDirectory(const std::filesystem::path& dir, std::string& error);
		// Reloads changed files (all when force). A broken file keeps its previous script.
		void Reload(bool force, ReloadReport& report);

		const ColorScript& Active() const;
		bool SetActive(const std::string& name);
		void CycleActive(int step = 1);
		std::vector<std::string> Names() const;

	private:
		struct Entry
		{
			std::filesystem::path file;
			ColorScript script;
		};

		std::filesystem::path mDirectory;
		std::vector<Entry> mEntries;
		std::map<std::filesystem::path, std::filesystem::file_time_type> mSeenStamps;
		size_t mActive = 0;
	};
}
