#include "framework/EngineContext.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "framework/reflection/AssetKind.h"
#include "framework/reflection/Reflection.h"
#include "framework/scene/Scene.h"
#include "framework/scene/SceneEntityComponent.h"
#include <iostream>
#include <stdexcept>

namespace
{
	using Json = nlohmann::json;
	using vega::reflect::AssetKind;
	int failures = 0;
	void Check(bool condition, const char* message)
	{
		if (!condition) { std::cerr << message << '\n'; ++failures; }
	}

	// Asset-typed and plain twins hold the same value, so their saved properties must match exactly.
	class AssetActor : public vega::Actor
	{
		VEGA_CLASS(AssetActor)
	public:
		explicit AssetActor(vega::World* world = nullptr) : Actor{ world } {}
		const std::string& GetIcon() const { return icon; }
		const std::string& GetSound() const { return sound; }
		void SetSound(const std::string& value) { sound = value; }
		std::string GetPreview() const { return icon; }
	private:
		std::string icon = "textures/icon.png";
		std::string sound = "sounds/click.wav";
	};

	VEGA_REFLECT(AssetActor, vega::Actor)
	{
		VEGA_SPAWNABLE();
		VEGA_FIELD("Icon", icon).Asset(AssetKind::Texture);
		VEGA_PROPERTY("Sound", GetSound, SetSound).Asset(AssetKind::Sound);
		VEGA_READONLY("Preview", GetPreview).Asset(AssetKind::Texture);
	}

	class PlainActor : public vega::Actor
	{
		VEGA_CLASS(PlainActor)
	public:
		explicit PlainActor(vega::World* world = nullptr) : Actor{ world } {}
		const std::string& GetIcon() const { return icon; }
	private:
		std::string icon = "textures/icon.png";
	};

	VEGA_REFLECT(PlainActor, vega::Actor)
	{
		VEGA_SPAWNABLE();
		VEGA_FIELD("Icon", icon);
	}

	struct PlainValue { int count = 3; std::string text = "x"; };

	Json Entity(const std::string& type, const std::string& icon)
	{
		return { { "guid", vega::SceneEntityComponent::GenerateGuid() }, { "name", "Asset" }, { "type", type },
			{ "properties", { { "Icon", icon } } }, { "components", Json::array() }, { "scripts", Json::array() } };
	}
	Json Scene(Json entities)
	{
		return { { "format", "vega.scene" }, { "version", 1 }, { "settings", Json::object() }, { "entities", std::move(entities) } };
	}

	void TestAcceptsAsset()
	{
		using std::filesystem::path;
		struct Case { AssetKind kind; const char* name; bool accepted; };
		const Case cases[] = {
			{ AssetKind::Texture, "hero.png", true }, { AssetKind::Texture, "HERO.PNG", true }, { AssetKind::Texture, "art/hero.JPEG", true },
			{ AssetKind::Texture, "ship.tga", true }, { AssetKind::Texture, "ship.gif", false }, { AssetKind::Texture, "hero.png.txt", false },
			{ AssetKind::Sound, "shot.wav", true }, { AssetKind::Sound, "music.OGG", true }, { AssetKind::Sound, "a.flac", true },
			{ AssetKind::Sound, "a.mp3", true }, { AssetKind::Sound, "a.aac", false }, { AssetKind::Sound, "shot.png", false },
			{ AssetKind::Font, "ui.ttf", true }, { AssetKind::Font, "ui.OTF", true }, { AssetKind::Font, "ui.woff", false },
			{ AssetKind::Prefab, "enemy.prefab.json", true }, { AssetKind::Prefab, "ENEMY.Prefab.JSON", true },
			{ AssetKind::Prefab, "packs/enemy.prefab.json", true }, { AssetKind::Prefab, "enemy.json", false },
			{ AssetKind::Prefab, ".prefab.json", false }, { AssetKind::Prefab, "level.scene.json", false },
			{ AssetKind::Scene, "level.scene.json", true }, { AssetKind::Scene, "LEVEL.SCENE.JSON", true },
			{ AssetKind::Scene, "level.json", false }, { AssetKind::Scene, ".scene.json", false }, { AssetKind::Scene, "enemy.prefab.json", false },
			{ AssetKind::Script, "start.lua", true }, { AssetKind::Script, "START.LUA", true }, { AssetKind::Script, "start.lua.bak", false },
			{ AssetKind::Any, "anything.bin", true }, { AssetKind::Any, "noextension", true }, { AssetKind::Any, "", false },
			{ AssetKind::None, "hero.png", false },
		};
		for (const auto& item : cases)
		{
			const bool actual = vega::reflect::AcceptsAsset(item.kind, path{ item.name });
			if (actual != item.accepted)
			{
				std::cerr << "AcceptsAsset(" << vega::reflect::AssetKindName(item.kind) << ", " << item.name << ") must be " << item.accepted << '\n';
				++failures;
			}
		}
	}
}

int main()
{
	vega::EngineContext engineContext;
	vega::EngineContext::Scope contextScope{ engineContext };
	using namespace vega::reflect;
	using vega::SceneSerializer;
	using vega::World;
	RegisterEngineTypes();
	VEGA_REGISTER(AssetActor);
	VEGA_REGISTER(PlainActor);
	TestAcceptsAsset();

	auto& registry = engineContext.Types();
	const auto* asset = registry.Find("AssetActor");
	const auto* plain = registry.Find("PlainActor");
	Check(asset && plain, "Asset and plain actors must register");
	if (asset)
	{
		Check(asset->FindProperty("Icon")->assetKind == AssetKind::Texture && asset->FindProperty("Icon")->kind == PropertyKind::String,
			"VEGA_FIELD .Asset() must be visible on its PropertyInfo as a String");
		Check(asset->FindProperty("Sound")->assetKind == AssetKind::Sound, "VEGA_PROPERTY .Asset() must be visible on its PropertyInfo");
		Check(asset->FindProperty("Preview")->assetKind == AssetKind::Texture && !asset->FindProperty("Preview")->setter,
			"VEGA_READONLY .Asset() must be visible and keep the property read-only");
	}
	if (plain) Check(plain->FindProperty("Icon")->assetKind == AssetKind::None, "Plain string properties must have no asset kind");

	// Registration refuses kinds that cannot describe a path, so the Inspector never sees them.
	bool refusedNonString = false, refusedNone = false;
	try
	{
		Class<PlainValue>("AssetKindProbe", registry).Property("Count", [](const PlainValue& v) { return v.count; },
			[](PlainValue& v, int value) { v.count = value; }).Asset(AssetKind::Texture);
	}
	catch (const std::logic_error&) { refusedNonString = true; }
	try
	{
		Class<PlainValue>("AssetKindProbeNone", registry).Property("Text", [](const PlainValue& v) { return v.text; }).Asset(AssetKind::None);
	}
	catch (const std::logic_error&) { refusedNone = true; }
	Check(refusedNonString, ".Asset() on an int property must throw at registration");
	Check(refusedNone, ".Asset(None) must throw at registration");

	// The asset kind is editor metadata: the saved value must match a plain string property exactly.
	World assetWorld{ nullptr };
	auto assetActor = assetWorld.SpawnActor<AssetActor>().lock();
	assetActor->AddComponent<vega::SceneEntityComponent>("", "Asset");
	World plainWorld{ nullptr };
	auto plainActor = plainWorld.SpawnActor<PlainActor>().lock();
	plainActor->AddComponent<vega::SceneEntityComponent>("", "Plain");
	const auto assetSaved = SceneSerializer::Save(assetWorld);
	const auto plainSaved = SceneSerializer::Save(plainWorld);
	const auto& assetProperties = assetSaved["entities"][0]["properties"];
	const auto& plainProperties = plainSaved["entities"][0]["properties"];
	Check(assetProperties.at("Icon").dump() == plainProperties.at("Icon").dump(), "Asset-typed property must serialize byte-identically to a plain string");
	Check(assetProperties.at("Icon").is_string() && assetProperties.at("Icon") == "textures/icon.png", "Asset path must save as a plain string");
	Check(!assetProperties.contains("Preview"), "Read-only asset properties must not be saved");

	World loaded{ nullptr };
	const auto result = SceneSerializer::Load(loaded, assetSaved);
	const auto restored = loaded.GetActorsByTypeIncludingPending<AssetActor>();
	Check(result.warnings.empty() && restored.size() == 1 && restored[0].lock()->GetIcon() == "textures/icon.png",
		"Asset-typed property must survive a scene round trip");

	auto authored = Entity("AssetActor", "art/hero.png");
	authored["properties"]["Sound"] = "sounds/shot.ogg";
	World authoredWorld{ nullptr };
	const auto authoredResult = SceneSerializer::Load(authoredWorld, Scene(Json::array({ authored })));
	const auto authoredActors = authoredWorld.GetActorsByTypeIncludingPending<AssetActor>();
	Check(authoredResult.warnings.empty() && authoredActors.size() == 1 && authoredActors[0].lock()->GetIcon() == "art/hero.png" &&
		authoredActors[0].lock()->GetSound() == "sounds/shot.ogg", "Authored asset paths must load as plain strings");

	if (failures) return 1;
	std::cout << "Reflection asset tests passed\n";
	return 0;
}
