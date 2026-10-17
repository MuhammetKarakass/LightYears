#include "framework/EngineContext.h"
#include "framework/scene/Scene.h"
#include "framework/scene/SceneEntityComponent.h"
#include "framework/reflection/Reflection.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "framework/SpriteComponent.h"
#include "framework/LightComponent.h"
#include <iostream>
#include <set>

namespace
{
	using Json = nlohmann::json;
	int failures = 0;
	void Check(bool condition, const char* message)
	{
		if (!condition) { std::cerr << message << '\n'; ++failures; }
	}
	enum class Mode { First = 2, Second = 7, Unlabelled = 13 };
	class SceneTestComponent : public vega::ActorComponent
	{
	public:
		using ActorComponent::ActorComponent;
		float amount = 8.f;
	};
	class SceneTestActor : public vega::Actor
	{
	public:
		explicit SceneTestActor(vega::World* world) : Actor{ world }
		{
			AddComponent<SceneTestComponent>();
			AddComponent<SceneTestComponent>().amount = 31.f;
		}
		int count = 4;
		bool enabled = true;
		float number = 3.5f;
		sf::Color tint{ 12, 34, 56, 78 };
		sf::Vector2f point{ 8.f, -4.f };
		std::string text = "scene";
		Mode mode = Mode::Second;
		int flags = 5;
	};
	class ReorderedActor : public vega::Actor
	{
	public:
		explicit ReorderedActor(vega::World* world) : Actor{ world }
		{
			AddComponent<vega::SpriteComponent>();
			AddComponent<vega::LightComponent>();
		}
	};
	class FactoryActor : public vega::Actor
	{
	public:
		FactoryActor(vega::World* world, int value) : Actor{ world }, argument{ value } {}
		int argument;
	};

	Json Entity(const std::string& type)
	{
		return { { "guid", vega::SceneEntityComponent::GenerateGuid() }, { "name", "Test" }, { "type", type },
			{ "properties", Json::object() }, { "components", Json::array() }, { "scripts", Json::array() } };
	}
	Json Scene(Json entities)
	{
		return { { "format", "vega.scene" }, { "version", 1 }, { "settings", Json::object() }, { "entities", std::move(entities) } };
	}
}

int main()
{
	vega::EngineContext engineContext;
	vega::EngineContext::Scope contextScope{ engineContext };
	using namespace vega;
	using namespace vega::reflect;
	RegisterEngineTypes();
	Class<SceneTestComponent, ActorComponent>("SceneTestComponent").Addable()
		.Property("Amount", [](const SceneTestComponent& c) { return c.amount; }, [](SceneTestComponent& c, float v) { c.amount = v; });
	Class<SceneTestActor, Actor>("SceneTestActor").Spawnable()
		.Property("Count", [](const SceneTestActor& a) { return a.count; }, [](SceneTestActor& a, int v) { a.count = v; })
		.Property("Enabled", [](const SceneTestActor& a) { return a.enabled; }, [](SceneTestActor& a, bool v) { a.enabled = v; })
		.Property("Number", [](const SceneTestActor& a) { return a.number; }, [](SceneTestActor& a, float v) { a.number = v; })
		.Property("Tint", [](const SceneTestActor& a) { return a.tint; }, [](SceneTestActor& a, sf::Color v) { a.tint = v; })
		.Property("Point", [](const SceneTestActor& a) { return a.point; }, [](SceneTestActor& a, sf::Vector2f v) { a.point = v; })
		.Property("Text", [](const SceneTestActor& a) { return a.text; }, [](SceneTestActor& a, const std::string& v) { a.text = v; })
		.Property("Mode", [](const SceneTestActor& a) { return a.mode; }, [](SceneTestActor& a, Mode v) { a.mode = v; })
		.Labels({ { 2, "First" }, { 7, "Second" } });
	Class<SceneTestActor, Actor>("SceneTestActor")
		.Property("Flags", [](const SceneTestActor& a) { return a.flags; }, [](SceneTestActor& a, int v) { a.flags = v; })
		.Flags({ { 1, "Player" }, { 4, "Enemy" } });
	Class<FactoryActor, Actor>("FactoryActor").Spawnable([](World& world) { return world.SpawnActor<FactoryActor>(42).lock(); });
	World source{ nullptr };
	auto actor = source.SpawnActor<SceneTestActor>().lock();
	auto& identity = actor->AddComponent<SceneEntityComponent>("", "Named actor");
	actor->SetActorLocation({ 100.f, 200.f });
	source.SpawnActor<Actor>();
	const Json settings{ { "routeStop", 3 }, { "nested", { { "free", "game data" } } } };
	const auto saved = SceneSerializer::Save(source, settings);
	Check(saved["entities"].size() == 1, "Only scene entities may be saved");
	const auto& data = saved["entities"][0];
	Check(SceneSerializer::SaveEntity(*actor) == data, "Entity-only save must preserve the full scene representation");
	Check(SceneSerializer::SaveEntity(*source.SpawnActor<Actor>().lock()).is_null(), "Runtime actors have no scene representation");
	Check(data["name"] == "Named actor" && data["guid"] == identity.guid, "Scene identity was not saved");
	Check(data["properties"]["Mode"] == "Second" && data["properties"]["RenderLayer"] == "World", "Enums must encode labels");
	Check(data["properties"]["Flags"] == Json::array({ "Player", "Enemy" }), "Flags must encode a label array");
	for (const auto& value : { Json(5), Json("Player | Enemy"), Json::array({ "Player", "Enemy" }) })
	{
		auto flagsEntity = Entity("SceneTestActor");
		flagsEntity["properties"]["Flags"] = value;
		World flagsWorld{ nullptr };
		const auto flagsResult = SceneSerializer::Load(flagsWorld, Scene(Json::array({ flagsEntity })));
		Check(flagsResult.warnings.empty() && flagsWorld.GetActorsByTypeIncludingPending<SceneTestActor>()[0].lock()->flags == 5,
			"Flags array, integer and old enum label must load identically");
	}
	for (const auto& value : { Json(0), Json("None"), Json::array() })
	{
		auto flagsEntity = Entity("SceneTestActor");
		flagsEntity["properties"]["Flags"] = value;
		World flagsWorld{ nullptr };
		Check(SceneSerializer::Load(flagsWorld, Scene(Json::array({ flagsEntity }))).warnings.empty() &&
			flagsWorld.GetActorsByTypeIncludingPending<SceneTestActor>()[0].lock()->flags == 0, "Empty flags must load");
	}
	for (const auto& value : { Json(2), Json(-1), Json("Player | Missing"), Json::array({ "Missing" }), Json::array({ 1 }), Json(1.5) })
	{
		auto flagsEntity = Entity("SceneTestActor");
		flagsEntity["properties"]["Flags"] = value;
		World flagsWorld{ nullptr };
		Check(SceneSerializer::Load(flagsWorld, Scene(Json::array({ flagsEntity }))).warnings.size() == 1 &&
			flagsWorld.GetActorsByTypeIncludingPending<SceneTestActor>()[0].lock()->flags == 5, "Invalid flags must warn and preserve defaults");
	}
	Check(data["properties"]["Point"] == Json::array({ 8, -4 }) && data["properties"]["Tint"] == Json::array({ 12, 34, 56, 78 }), "Vector/color encoding is incorrect");
	Check(!data["properties"].contains("Velocity"), "Read-only properties must not be saved");
	Check(data["components"].size() == 2 && data["components"][0]["properties"]["Amount"] == 8 &&
		data["components"][1]["properties"]["Amount"] == 31, "Repeated default components must preserve their occurrence order");
	const auto* identityType = TypeRegistry::Get().Find("SceneEntityComponent");
	Check(identityType->FindProperty("Name")->setter && !identityType->FindProperty("GUID")->setter, "Identity reflection permissions are incorrect");
	std::set<std::string> guids;
	for (int i = 0; i < 1000; ++i)
	{
		const auto guid = SceneEntityComponent::GenerateGuid();
		Check(guid.size() == 32 && guid.find_first_not_of("0123456789abcdef") == std::string::npos && guids.insert(guid).second, "GUID must be unique 128-bit hex");
	}
	World loaded{ nullptr };
	const auto result = SceneSerializer::Load(loaded, saved);
	Check(result.spawned == 1 && result.warnings.empty(), "Supported scene must load without warnings");
	Check(loaded.GetActorsByType<Actor>().empty(), "Scene factories must leave actors pending");
	Check(SceneSerializer::Save(loaded, settings) == saved, "Save(Load(scene)) must preserve supported data exactly");
	loaded.TickInternal(0.f);
	Check(SceneSerializer::Save(loaded, settings) == saved, "Promoting actors must preserve scene identity and properties");
	Check(loaded.GetActorByType<SceneTestActor>().lock()->GetUniqueID() != actor->GetUniqueID(), "Run IDs should differ across loads");
	actor->mode = Mode::Unlabelled;
	const auto unlabelled = SceneSerializer::Save(source, settings);
	Check(unlabelled["entities"][0]["properties"]["Mode"] == 13, "Unlabelled enum must save as an integer without throwing");
	World enumWorld{ nullptr };
	const auto enumResult = SceneSerializer::Load(enumWorld, unlabelled);
	// Loading must not apply a value outside the labelled set; it warns and keeps the type's default instead.
	Check(enumResult.warnings.size() == 1 && enumResult.warnings[0].find("Mode") != std::string::npos &&
		enumWorld.GetActorsByTypeIncludingPending<SceneTestActor>()[0].lock()->mode != Mode::Unlabelled,
		"An unlabelled enum integer must be rejected with a warning on load");
	actor->mode = Mode::Second;
	Class<ReorderedActor, Actor>("ReorderedActor").Spawnable();
	World reorderWorld{ nullptr };
	const auto reordered = reorderWorld.SpawnActor<ReorderedActor>().lock();
	reordered->AddComponent<SceneEntityComponent>();
	reordered->RemoveComponent(*reordered->FindComponent<SpriteComponent>());
	reordered->AddComponent<SpriteComponent>().SetScale({ 3.f, 4.f });
	const auto reorderedScene = SceneSerializer::Save(reorderWorld);
	World reorderRoundTrip{ nullptr };
	Check(SceneSerializer::Load(reorderRoundTrip, reorderedScene).warnings.empty() && SceneSerializer::Save(reorderRoundTrip) == reorderedScene,
		"Remove-and-readd edits must save in stable constructor order");
	// Compare constructor inventories by count, retaining occurrence properties.
	World edited{ nullptr };
	const auto editedActor = edited.SpawnActor<SceneTestActor>().lock();
	editedActor->AddComponent<SceneEntityComponent>();
	const auto* spriteInfo = TypeRegistry::Get().Find("SpriteComponent");
	Check(spriteInfo->addComponent && spriteInfo->removable, "Addable reflection must supply factory and removal policy");
	spriteInfo->addComponent(*editedActor);
	Check(editedActor->RemoveComponent(*editedActor->FindComponent<SceneTestComponent>()), "Default component removal failed");
	const auto editedScene = SceneSerializer::Save(edited);
	Check(editedScene["entities"][0]["removed"] == Json::array({ "SceneTestComponent" }), "Missing constructor occurrence must save as removed");
	Check(editedScene["entities"][0]["components"][1]["added"] == true, "Non-constructor component must save added marker");
	World editedRoundTrip{ nullptr };
	Check(SceneSerializer::Load(editedRoundTrip, editedScene).warnings.empty() && SceneSerializer::Save(editedRoundTrip) == editedScene,
		"Added and removed components must round trip exactly");
	const auto addedActor = editedRoundTrip.GetActorsByTypeIncludingPending<SceneTestActor>()[0].lock();
	addedActor->AddComponent<SceneTestComponent>().amount = 72.f;
	addedActor->AddComponent<SceneTestComponent>().amount = 99.f;
	const auto duplicates = SceneSerializer::Save(editedRoundTrip);
	World duplicateWorld{ nullptr };
	Check(SceneSerializer::Load(duplicateWorld, duplicates).warnings.empty() && SceneSerializer::Save(duplicateWorld) == duplicates,
		"Repeated added components must preserve occurrence properties");
	Class<SceneTestActor, Actor>("SceneTestActor").RequiresComponent("SceneTestComponent");
	Check(!editedActor->RemoveComponent(*editedActor->FindComponent<SceneTestComponent>()), "Required component removal must be refused by Actor");
	auto refused = Entity("SceneTestActor");
	refused["removed"] = Json::array({ "SceneTestComponent", "MissingType", "SceneEntityComponent" });
	refused["components"] = Json::array({ { { "type", "MissingAddable" }, { "added", true } },
		{ { "type", "ActorComponent" }, { "added", true } } });
	World refusalWorld{ nullptr };
	const auto refusalResult = SceneSerializer::Load(refusalWorld, Scene(Json::array({ refused })));
	Check(refusalResult.spawned == 1 && refusalResult.warnings.size() == 5,
		"Required/unknown removals and unknown/non-addable additions must warn and continue");
	Check(refusalWorld.GetActorsByTypeIncludingPending<SceneTestActor>()[0].lock()->FindComponent<SceneTestComponent>() != nullptr,
		"Required constructor component must survive malformed scene removal");
	Class<SceneTestActor, Actor>("SceneTestActor")
		.Property("Throwing", [](const SceneTestActor& a) { return a.count; },
			[](SceneTestActor&, int) { throw std::runtime_error("scene setter refused"); });
	auto throwingEntity = Entity("SceneTestActor");
	throwingEntity["properties"] = { { "Throwing", 99 }, { "Number", 12.f } };
	World throwingWorld{ nullptr };
	const auto throwingResult = SceneSerializer::Load(throwingWorld, Scene(Json::array({ throwingEntity })));
	Check(throwingResult.warnings.size() == 1 && throwingResult.warnings[0].find("scene setter refused") != std::string::npos &&
		throwingWorld.GetActorsByTypeIncludingPending<SceneTestActor>()[0].lock()->number == 12.f, "Scene must report setter reasons and continue loading");
	Check(SceneSerializer::Load(loaded, saved).spawned == 0, "Duplicate entity GUIDs must be rejected");

	World errors{ nullptr };
	auto entity = Entity("SceneTestActor");
	entity["properties"] = { { "Unknown", 5 }, { "Velocity", Json::array({ 1, 2 }) }, { "Number", "bad" },
		{ "Count", 2147483648LL }, { "Tint", Json::array({ 0, 0, 0, 256 }) }, { "Mode", "Missing" }, { "Point", Json::array({ 1 }) } };
	entity["components"].push_back({ { "type", "SpriteComponent" }, { "properties", Json::object() } });
	entity["components"].push_back({ { "type", "SceneTestComponent" }, { "properties", { { "Unknown", true }, { "Amount", 17 } } } });
	auto bad = Scene(Json::array({ Entity("MissingType"), Entity("Actor"), entity, Entity("FactoryActor") }));
	const auto badResult = SceneSerializer::Load(errors, bad);
	Check(badResult.spawned == 2 && badResult.warnings.size() == 11, "Unknown/read-only/invalid properties and non-spawnable types must warn and continue");
	const auto errorActor = errors.GetActorsByTypeIncludingPending<SceneTestActor>()[0].lock();
	Check(!errorActor->FindComponent<SpriteComponent>(), "Load must not create arbitrary components");
	Check(errorActor->FindComponent<SceneTestComponent>()->amount == 17.f, "Invalid properties must not prevent valid properties from loading");
	Check(errors.GetActorsByTypeIncludingPending<FactoryActor>()[0].lock()->argument == 42, "Custom spawn factory was not used");
	for (const auto& invalid : { Json{}, Json::array(), Json{ { "version", 2 } }, Json{ { "format", 42 } } })
		Check(!SceneSerializer::Load(errors, invalid).warnings.empty(), "Malformed documents must warn without throwing");
	errorActor->Destroy();
	Check(SceneSerializer::Save(errors)["entities"].size() == 1, "Destroyed entities must not be saved");
	return failures ? 1 : 0;
}
