#pragma once

#include <array>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "visual/ReloadReport.h"

namespace vega::visual
{
	// What a ship port shows. The game maps its own ability categories onto this; the library knows nothing else.
	enum class AbilityCategory
	{
		Primary,
		Offense,
		Defense,
		Movement,
		Utility
	};

	// Palette role holding the ramp of a category's light (roles portPrimary, portOffense, ...).
	const char* PortRoleName(AbilityCategory category);

	// How a part is colored: a stop of a ramp of the active color script. The ship definition says which script role
	// each of these roles samples. Using ramp stops keeps every hull color on the dither palette.
	enum class PartRole
	{
		Hull,
		Plate,
		Dark,
		Accent,
		Trim,
		Glass
	};

	// One polygon in local ship units (nose toward -y, 1 unit = 1 sprite pixel of the reference sprite).
	// Mirrored parts are expanded at load: the left copy is the mirror image and carries the "_L" name suffix.
	struct ShipPart
	{
		std::string name;
		std::vector<sf::Vector2f> points;
		PartRole role = PartRole::Hull;
		// Added to the role's base ramp stop (negative darkens).
		int shade = 0;
		// Draw a dark outline over neighbors first, leaving a gap between this part and the parts under it.
		bool gap = false;
		bool crackable = true;
		// Both copies of a mirrored part are paired: the lit side keeps its shade, the other side is one step darker.
		bool paired = false;
		bool mirrored = false;
		unsigned seed = 0;
		// Damage stage (1..3) from which the part is missing; 0 keeps it forever.
		int detachStage = 0;
		sf::Vector2f centroid;
	};

	// Panel line following the hull; hidden with its part (by name).
	struct ShipLine
	{
		std::string part;
		sf::Vector2f a;
		sf::Vector2f b;
		PartRole role = PartRole::Dark;
	};

	// Small accent light: a lit pixel cluster in the hull plus a tiny energy glow.
	struct ShipLight
	{
		sf::Vector2f position;
		float size = 2.f;
		PartRole role = PartRole::Trim;
		// Hz; 0 = steady
		float blink = 0.f;
	};

	struct ShipEngineMount
	{
		sf::Vector2f position;
		float size = 8.f;
	};

	struct ShipPortMount
	{
		sf::Vector2f position;
		float size = 6.f;
	};

	// Ports 0..3 are the four ability slots in input order.
	constexpr int ShipPortCount = 4;

	struct ShipVisualDefinition
	{
		// Opaque key, unique inside the library.
		std::string id;
		// Opaque external ids that resolve to this definition (the game lists its ship content ids here).
		std::vector<std::string> bindings;
		// Width and height of the drawn parts in local units (computed at load). A caller fitting the visual to another
		// footprint, such as a sprite, divides by it.
		sf::Vector2f size{100.f, 100.f};
		// Extra multiplier on top of the caller's scale.
		float scale = 1.f;
		// Palette roles sampled by part roles.
		std::string hullRole = "hull";
		std::string plateRole = "hull";
		std::string accentRole = "player";
		std::string glassRole = "player";
		std::vector<ShipPart> parts;
		std::vector<ShipLine> lines;
		std::vector<ShipLight> lights;
		std::vector<ShipEngineMount> engines;
		std::array<ShipPortMount, ShipPortCount> ports;
		// Shield bubble semi-axes: x across the ship, y along it.
		sf::Vector2f shieldRadii{60.f, 70.f};
		// Local origin of the visible smoke at the heaviest damage stage when no part gives a better spot.
		sf::Vector2f smokeOrigin{0.f, 10.f};
	};

	// Ship definitions from <dir>/*.json, one ship per file. A missing directory is an empty library.
	// Reload keeps the previous definition when a file fails to parse.
	class ShipVisualLibrary
	{
	public:
		// Parse failures land in report.errors; they never fail the load.
		void LoadDirectory(const std::filesystem::path& dir, ReloadReport& report);
		void Reload(bool force, ReloadReport& report);

		// Pointers stay valid until the next Reload; look the definition up again each frame.
		const ShipVisualDefinition* Find(const std::string& id) const;
		const ShipVisualDefinition* FindByBinding(const std::string& externalId) const;
		std::vector<std::string> Ids() const;

		static bool ParseFile(const std::filesystem::path& file, ShipVisualDefinition& out, std::string& error);

	private:
		struct Entry
		{
			std::filesystem::path file;
			ShipVisualDefinition definition;
		};

		std::filesystem::path mDirectory;
		std::vector<Entry> mEntries;
		std::map<std::filesystem::path, std::filesystem::file_time_type> mSeenStamps;
	};
}
