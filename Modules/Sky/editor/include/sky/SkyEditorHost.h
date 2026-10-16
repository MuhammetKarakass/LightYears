#pragma once

#include <sky/VisualAssets.h>
#include <sky/Backdrop.h>
#include <editor/EditorCommands.h>

namespace vega::sky
{
	struct SkyGeometry
	{
		sf::Vector2f planetAt, bandDrift;
		float planetRadius = 0.f, planetBaseScale = 1.f;
		MilkyWaySettings band;
		bool mirrored = false, hasPlanet = false;
	};
	struct SkyPreview
	{
		std::string id, name;
	};

	// The module edits opaque sky IDs. Games supply presentation context and may
	// map those IDs to their own locations; no route or game types cross this seam.
	class SkyEditorHost
	{
	public:
		virtual ~SkyEditorHost() = default;
		virtual VisualAssets& EditorAssets() = 0;
		virtual const std::vector<SkyPreview>& Skies() const = 0;
		virtual void SelectSky(std::size_t index) = 0;
		virtual std::string SelectedSky() const = 0;
		virtual std::size_t SceneSelection(const EditorContext&) const = 0;
		virtual void UseInScene(CommandHistory&, std::size_t index) = 0;
		virtual void SetSkyAuthoring(bool enabled) = 0;
		virtual SkyGeometry Geometry(sf::Vector2u output) const = 0;
		virtual SkyPalette Palette() const = 0;
		virtual bool RenderSky(sf::RenderTexture&, std::size_t index, const ly::World&, sf::Vector2u size) = 0;
		virtual std::filesystem::path DocumentFile() const { return "deepsky.json"; }
	};
}
