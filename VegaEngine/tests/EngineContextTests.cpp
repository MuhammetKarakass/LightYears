#include "framework/EngineContext.h"
#include "framework/Application.h"
#include "framework/World.h"
#include "framework/Actor.h"
#include "framework/AssetManager.h"
#include "framework/AudioManager.h"
#include "framework/EngineAssets.h"
#include "framework/reflection/Reflection.h"
#include "framework/scene/PrefabLibrary.h"
#include "framework/scene/Scene.h"
#include "framework/scene/SceneEntityComponent.h"
#include "framework/scene/SceneSettings.h"
#include "framework/content/ContentSchema.h"
#include "widget/UIStyle.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace
{
	class ContextActor : public vega::Actor
	{
	public:
		explicit ContextActor(vega::World* world) : Actor{ world } {}
	};
	class AdoptedActor : public vega::Actor
	{
	public:
		explicit AdoptedActor(vega::World* world) : Actor{ world } {}
	};
	void Check(bool condition, const char* message)
	{
		if (!condition) throw std::runtime_error(message);
	}
	void CheckApplicationAdoption(const std::filesystem::path& root)
	{
		auto& current = vega::EngineContext::Current();
		const auto assetRoot = root.generic_string() + "/";
		vega::AssetManager::GetAssetManager().SetAssetRootDirectory(assetRoot);
		vega::reflect::Class<AdoptedActor, vega::Actor>{ "AdoptedActor" }.Spawnable();
		const auto* registeredType = current.Types().Find("AdoptedActor");
		Check(registeredType != nullptr, "Pre-application registration failed");
		{
			auto older = std::make_unique<vega::Application>(sf::Vector2u{ 32, 32 }, 32, "Adopt current context", sf::Style::None);
			older->GetRenderWindow().setVisible(false);
			Check(&older->GetEngineContext() == &current, "Default Application did not adopt the current context");
			Check(older->GetEngineContext().Assets().GetAssetRootDirectory() == assetRoot, "Default Application lost the configured asset root");
			Check(older->GetEngineContext().Types().Find("AdoptedActor") == registeredType, "Default Application lost a registered type");
			vega::Application newer{ { 32, 32 }, 32, "Adopt same context", sf::Style::None };
			newer.GetRenderWindow().setVisible(false);
			// The newer application's binding must survive removal of its predecessor.
			older.reset();
			Check(&vega::EngineContext::Current() == &current && &newer.GetEngineContext() == &current, "Out-of-order application destruction lost the adopted context");
			{
				vega::EngineContext isolated;
				vega::Application explicitHost{ { 32, 32 }, 32, "Explicit isolated context", sf::Style::None, &isolated };
				explicitHost.GetRenderWindow().setVisible(false);
				Check(&explicitHost.GetEngineContext() == &isolated && isolated.Assets().GetAssetRootDirectory().empty() &&
					!isolated.Types().Find("AdoptedActor"), "Explicit Application context did not isolate services");
				vega::Application adoptedHost{ { 32, 32 }, 32, "Adopt innermost context", sf::Style::None };
				adoptedHost.GetRenderWindow().setVisible(false);
				Check(&adoptedHost.GetEngineContext() == &isolated, "Default Application did not adopt the innermost application binding");
			}
			Check(&vega::EngineContext::Current() == &current, "Explicit application binding did not restore the adopting host");
		}
		Check(&vega::EngineContext::Current() == &current && current.Types().Find("AdoptedActor") == registeredType &&
			current.Assets().GetAssetRootDirectory() == assetRoot, "Application destruction destroyed or cleared adopted services");
	}
	void WriteProject(const std::filesystem::path& root, float rotation, sf::Color color)
	{
		std::filesystem::create_directories(root / "prefabs");
		std::ofstream{ root / "prefabs/shared.prefab.json" } << nlohmann::json{
			{ "format", "vega.prefab" }, { "version", 1 }, { "name", "Shared" },
			{ "base", { { "type", "ContextActor" } } }, { "properties", { { "Rotation", rotation } } }
		}.dump();
		Check(sf::Image{ { 1, 1 }, color }.saveToFile(root / "shared.png"), "Cannot write isolated texture");
	}
}

int main()
{
	try
	{
		const std::filesystem::path root{ ENGINE_CONTEXT_TEST_ROOT };
		CheckApplicationAdoption(root / "legacy");
		vega::EngineContext first, second;
		vega::EngineContext::Scope scope{ first };
		Check(vega::SceneSettingsSchema{}.Fields().empty(), "Standalone schemas must start empty");
		Check(first.Prefabs().Root() == std::filesystem::current_path(), "Default prefab root changed");
		CheckApplicationAdoption(root / "bound");
		WriteProject(root / "first", 10.f, sf::Color::Red);
		WriteProject(root / "second", 20.f, sf::Color::Blue);
		first.Assets().SetAssetRootDirectory((root / "first").generic_string() + "/");
		second.Assets().SetAssetRootDirectory((root / "second").generic_string() + "/");
		Check(first.Prefabs().Root() != second.Prefabs().Root(), "Asset roots leaked into another prefab library");
		vega::reflect::Class<ContextActor, vega::Actor>{ "ContextActor", first.Types() }.Spawnable();
		Check(!second.Types().Find("ContextActor"), "Type registration leaked");
		vega::reflect::Class<ContextActor, vega::Actor>{ "ContextActor", second.Types() }.Spawnable();
		Check(first.Types().Find("Actor") != second.Types().Find("Actor"), "Engine descriptors shared");
		const auto* actorType = first.Types().Find("Actor");
		const auto* rotationProperty = actorType->FindProperty("Rotation");
		const auto typeCount = first.Types().RegisteredTypes().size();
		vega::reflect::RegisterEngineTypes(first.Types());
		Check(first.Types().RegisteredTypes().size() == typeCount && first.Types().Find("Actor") == actorType &&
			actorType->FindProperty("Rotation") == rotationProperty, "Repeated core registration invalidated descriptors");
		Check(first.Types().Find("ContextActor")->base == first.Types().Find("Actor"), "First base resolved in another context");
		Check(second.Types().Find("ContextActor")->base == second.Types().Find("Actor"), "Second base resolved in another context");
		int firstFactories = 0, secondFactories = 0;
		vega::SceneSerializer::RegisterSpawnFactory("ContextActor", [&](vega::World& world, const nlohmann::json&)
		{
			++firstFactories;
			return world.SpawnActor<ContextActor>().lock();
		}, first);
		vega::SceneSerializer::RegisterSpawnFactory("ContextActor", [&](vega::World& world, const nlohmann::json&)
		{
			++secondFactories;
			return world.SpawnActor<ContextActor>().lock();
		}, second);
		vega::World firstWorld{ first }, secondWorld{ second };
		auto secondBounds = *second.SceneSettings().Find("bounds");
		secondBounds.max = 1;
		second.SceneSettings().Register(std::move(secondBounds));
		{
			vega::EngineContext::Scope nested{ second };
			Check(&vega::reflect::TypeRegistry::Get() == &second.Types() && &vega::PrefabLibrary::Get() == &second.Prefabs(), "Nested compatibility binding failed");
			Check(&vega::AssetManager::GetAssetManager() == &second.Assets() && &vega::EngineAssets::Get() == &second.Defaults(), "Asset shims failed");
			const nlohmann::json scene{
				{ "format", "vega.scene" }, { "version", 1 }, { "settings", { { "bounds", { 0, 0, 10, 20 } } } },
				{ "entities", nlohmann::json::array({ { { "prefab", "prefabs/shared.prefab.json" }, { "guid", "instance" } } }) }
			};
			Check(vega::SceneSerializer::Load(firstWorld, scene).spawned == 1, "Explicit first world used ambient services");
			Check(firstWorld.GetSceneBounds() && firstWorld.GetSceneBounds()->size == sf::Vector2f{ 10.f, 20.f }, "Bounds used ambient schema");
			Check(secondFactories == 0, "Prefab validation used the ambient context's factory");
			const auto firstLoads = firstFactories;
			Check(vega::SceneSerializer::Load(secondWorld, scene).spawned == 1, "Second scene failed");
			Check(firstFactories == firstLoads, "Second prefab validation used the first context's factory");
			const auto actor = firstWorld.GetActorsByTypeIncludingPending<ContextActor>().front().lock();
			Check(actor->GetActorRotation() == 10.f, "First prefab used second root");
			Check(vega::SceneSerializer::SaveEntity(*actor, true)["properties"]["Rotation"] == 10.f, "Saving used ambient registry");
			Check(secondWorld.GetActorsByTypeIncludingPending<ContextActor>().front().lock()->GetActorRotation() == 20.f, "Second prefab used first cache");
		}
		Check(firstFactories > 0 && secondFactories > 0, "Scene factory hooks were not exercised");
		Check(&vega::reflect::TypeRegistry::Get() == &first.Types(), "Compatibility scope did not restore its predecessor");
		{
			auto older = std::make_unique<vega::EngineContext::Scope>(first);
			auto newer = std::make_unique<vega::EngineContext::Scope>(second);
			older.reset();
			Check(&vega::EngineContext::Current() == &second, "Removing an older binding displaced a live host");
			newer.reset();
			Check(&vega::EngineContext::Current() == &first, "A removed binding was restored");
		}
		const auto secondResolves = second.Prefabs().ResolveCount();
		first.Prefabs().Reload();
		Check(second.Prefabs().Resolve("prefabs/shared.prefab.json") && second.Prefabs().ResolveCount() == secondResolves, "Reload cleared another context's cache");
		const auto red = first.Assets().LoadTexture("shared.png"), blue = second.Assets().LoadTexture("shared.png");
		Check(red && blue && red != blue && red->copyToImage().getPixel({ 0, 0 }) == sf::Color::Red && blue->copyToImage().getPixel({ 0, 0 }) == sf::Color::Blue, "Asset cache/root isolation failed");
		first.Defaults().buttonSound = "first.wav";
		Check(second.Defaults().buttonSound.empty(), "Engine defaults shared");
		first.Style().fonts[0] = "first.ttf";
		Check(second.Style().fonts[0].empty(), "UI style shared");
		first.ContentSchemas().tags.push_back("first");
		Check(second.ContentSchemas().tags.empty(), "Content schemas shared");
		first.SceneSettings().Register({ "first", "First", "Test", vega::SceneSettingType::String, "" });
		Check(!second.SceneSettings().Find("first") && second.SceneSettings().Find("bounds"), "Scene schemas shared or missing core defaults");
		Check(!first.HasAudio() && !second.HasAudio(), "Constructing a context eagerly initialized audio");
		first.Audio().SetMasterVolume(17.f);
		Check(!second.HasAudio() && second.Audio().GetMasterVolume() == 100.f, "Audio state shared");
		first.ShutdownAudio();
		Check(second.HasAudio(), "Audio shutdown affected another context");
		Check(!first.Scripts() && !second.Scripts(), "Engine-only host initialized scripting");
		// Exercise the deprecated shutdown contract, rather than using it as fixture cleanup.
		vega::AssetManager::ShutdownAssetManager();
		Check(first.Assets().GetAssetRootDirectory().empty() && !second.Assets().GetAssetRootDirectory().empty(), "Legacy asset shutdown changed roots in another context");
		std::cout << "EngineContext services and compatibility scopes are isolated\n";
	}
	catch (const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
