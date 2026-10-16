#pragma once

#include <framework/Application.h>
#include <framework/World.h>
#include <functional>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

namespace vega
{
	class EditorExtensions;

	struct ActorDisplayInfo
	{
		bool spawnMarker = false;
		std::string label;
	};

	// One instance belongs to one application. Games install their existing
	// reflection, content, settings and scripting hooks from RegisterTypes/Initialize.
	class GameModule
	{
	public:
		virtual ~GameModule() = default;
		virtual const char* Name() const = 0;
		virtual void RegisterTypes() = 0;
		virtual void ConfigureProject(const std::filesystem::path&) {}
		virtual void Initialize(ly::Application& application, bool editor) = 0;
		virtual std::shared_ptr<ly::World> CreateLevel(ly::Application& application,
			const nlohmann::json& scene, bool edit) = 0;
		virtual void BeginPlay(ly::World&) {}
		virtual void EndPlay(ly::World&) {}
		virtual std::shared_ptr<ly::Actor> PlayerActor(ly::World&) { return {}; }
		virtual ActorDisplayInfo DisplayInfo(const ly::Actor&) const { return {}; }
		virtual void RegisterEditorExtensions(EditorExtensions&) {}
		virtual void ApplySceneSettings(const nlohmann::json&) {}
		virtual std::string SceneBackdropState() const { return {}; }
		virtual void Tick(float) {}
		virtual bool HandleEvent(const sf::Event&) { return false; }
		virtual void AfterRender() {}
		virtual int ExitCode() const { return 0; }
	};
}
