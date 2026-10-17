#include "framework/EngineContext.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "framework/reflection/Reflection.h"
#include "framework/scene/Scene.h"
#include "framework/scene/SceneEntityComponent.h"
#include <iostream>
#include <limits>
#include <vector>

namespace
{
	using Json = nlohmann::json;
	int failures = 0;
	void Check(bool condition, const char* message)
	{
		if (!condition) { std::cerr << message << '\n'; ++failures; }
	}

	enum class FieldMode { Idle = 1, Active = 4 };
	class FieldActor : public vega::Actor
	{
		VEGA_CLASS(FieldActor)
	public:
		explicit FieldActor(vega::World* world = nullptr) : Actor{ world } {}
		float GetGain() const { return gain; }
		int GetCount() const { return count; }
		bool GetEnabled() const { return enabled; }
		const std::string& GetLabel() const { return label; }
		sf::Vector2f GetOffset() const { return offset; }
		FieldMode GetMode() const { return mode; }
		const std::vector<int>& GetValues() const { return values; }
		float GetRatio() const { return ratio; }
		float GetLevel() const { return level; }
		float GetPlainGain() const { return plainGain; }
		void SetPlainGain(float value) { plainGain = value; }
		// Public so the hook can be a member pointer while the data stays private.
		void OnRatioChanged() { ++ratioNotifications; ratioSeen = ratio; }
		int ratioNotifications = 0;
		float ratioSeen = -1.f;
		int levelNotifications = 0;
		float levelSeen = -1.f;
	private:
		float gain = 2.f;
		int count = 4;
		bool enabled = true;
		std::string label = "field";
		sf::Vector2f offset{ 1.f, 2.f };
		FieldMode mode = FieldMode::Idle;
		std::vector<int> values{ 1, 2 };
		float ratio = 0.5f;
		float level = 0.f;
		int locked = 9;
		float plainGain = 0.f;
	};

	VEGA_REFLECT(FieldActor, vega::Actor)
	{
		VEGA_SPAWNABLE();
		VEGA_FIELD("Gain", gain).Range(0.f, 10.f);
		VEGA_FIELD("Count", count).Range(0.f, 10.f);
		VEGA_FIELD("Enabled", enabled);
		VEGA_FIELD("Label", label);
		VEGA_FIELD("Offset", offset);
		VEGA_FIELD("Mode", mode).Labels({ { 1, "Idle" }, { 4, "Active" } });
		VEGA_FIELD("Values", values);
		VEGA_FIELD_NOTIFY("Ratio", ratio, OnRatioChanged);
		// A lambda hook works where the macro's member-pointer form does not fit.
		type.Field("Level", &Self::level, [](Self& actor) { ++actor.levelNotifications; actor.levelSeen = actor.GetLevel(); });
		VEGA_FIELD_READONLY("Locked", locked);
		// Range on a hand-written property keeps its old behaviour: the setter decides what is stored.
		VEGA_PROPERTY("Plain Gain", GetPlainGain, SetPlainGain).Range(0.f, 10.f);
	}

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
	using namespace vega::reflect;
	using vega::SceneSerializer;
	using vega::World;
	RegisterEngineTypes();
	VEGA_REGISTER(FieldActor);
	auto& registry = TypeRegistry::Get();
	const auto* type = registry.Find("FieldActor");
	Check(type && type->base == registry.Find("Actor") && type->spawnable, "Field actor must register with its Actor base");
	Check(type->FindProperty("Gain")->kind == PropertyKind::Float && type->FindProperty("Count")->kind == PropertyKind::Int &&
		type->FindProperty("Enabled")->kind == PropertyKind::Bool && type->FindProperty("Label")->kind == PropertyKind::String &&
		type->FindProperty("Offset")->kind == PropertyKind::Vector2 && type->FindProperty("Mode")->kind == PropertyKind::Enum &&
		type->FindProperty("Values")->kind == PropertyKind::List && type->FindProperty("Values")->elementKind == PropertyKind::Int,
		"Field kinds must follow the member types");

	FieldActor actor;
	vega::Actor* root = &actor;
	const auto* gain = type->FindProperty("Gain");
	const auto* count = type->FindProperty("Count");
	const auto* enabled = type->FindProperty("Enabled");
	const auto* label = type->FindProperty("Label");
	const auto* offset = type->FindProperty("Offset");
	const auto* mode = type->FindProperty("Mode");
	const auto* values = type->FindProperty("Values");
	const auto* ratio = type->FindProperty("Ratio");
	const auto* level = type->FindProperty("Level");
	const auto* locked = type->FindProperty("Locked");
	const auto* plainGain = type->FindProperty("Plain Gain");

	Check(gain->SetValue(root, 7.5f) && actor.GetGain() == 7.5f && std::get<float>(gain->getter(root)) == 7.5f, "Float field must write its private member");
	Check(!gain->SetValue(root, 3) && actor.GetGain() == 7.5f, "Field must refuse a wrong value kind without mutation");
	Check(count->SetValue(root, 6) && actor.GetCount() == 6 && std::get<int>(count->getter(root)) == 6, "Int field must write its private member");
	Check(enabled->SetValue(root, false) && !actor.GetEnabled() && std::get<bool>(enabled->getter(root)) == false, "Bool field must write its private member");
	Check(label->SetValue(root, std::string{ "changed" }) && actor.GetLabel() == "changed", "String field must write its private member");
	Check(offset->SetValue(root, sf::Vector2f{ 3.f, -4.f }) && actor.GetOffset() == sf::Vector2f{ 3.f, -4.f }, "Vector field must write its private member");
	Check(values->SetValue(root, PropertyList{ 3, 5, 8 }) && actor.GetValues() == std::vector<int>{ 3, 5, 8 }, "Int list field must write its private member");
	Check(!values->SetValue(root, PropertyList{ 1.5f }) && actor.GetValues() == std::vector<int>{ 3, 5, 8 }, "List field must refuse wrong element kinds");
	Check(mode->SetValue(root, 4) && actor.GetMode() == FieldMode::Active && std::get<int>(mode->getter(root)) == 4, "Enum field must write its private member");
	Check(!mode->SetValue(root, 2) && actor.GetMode() == FieldMode::Active, "Labelled enum field must refuse unlisted values");

	Check(gain->SetValue(root, 15.f) && actor.GetGain() == 10.f, "Float field Range must clamp values above the maximum");
	Check(gain->SetValue(root, -3.f) && actor.GetGain() == 0.f, "Float field Range must clamp values below the minimum");
	Check(gain->SetValue(root, 4.f) && actor.GetGain() == 4.f, "In-range float values must be stored unchanged");
	Check(count->SetValue(root, 15) && actor.GetCount() == 10, "Int field Range must clamp values above the maximum");
	Check(count->SetValue(root, -3) && actor.GetCount() == 0, "Int field Range must clamp values below the minimum");
	Check(count->SetValue(root, 7) && actor.GetCount() == 7, "In-range int values must be stored unchanged");
	Check(plainGain->SetValue(root, 15.f) && actor.GetPlainGain() == 15.f, "A property with Range must not gain clamping");
	Check(ratio->SetValue(root, 50.f) && actor.GetRatio() == 50.f, "A field without Range must not clamp");

	actor.ratioNotifications = 0;
	Check(ratio->SetValue(root, 0.25f) && actor.ratioNotifications == 1 && actor.ratioSeen == 0.25f,
		"Member hook must run exactly once, after the value is stored");
	Check(!ratio->SetValue(root, 3) && actor.ratioNotifications == 1, "A refused write must not run the hook");
	Check(level->SetValue(root, 0.75f) && actor.levelNotifications == 1 && actor.levelSeen == 0.75f,
		"Lambda hook must run exactly once, after the value is stored");
	Check(!level->SetValue(root, std::numeric_limits<float>::infinity()) && actor.levelNotifications == 1,
		"A non-finite write must not run the lambda hook");

	Check(!locked->setter && !locked->SetValue(root, 1) && std::get<int>(locked->getter(root)) == 9, "Read-only field must reject writes");

	// Scene save and load must use the same registered fields as Inspector and Lua writes.
	World source{ nullptr };
	auto spawned = source.SpawnActor<FieldActor>().lock();
	// Only actors with a scene identity are saved, as in the scene serializer tests.
	spawned->AddComponent<vega::SceneEntityComponent>("", "Field actor");
	Check(gain->SetValue(spawned.get(), 3.5f) && count->SetValue(spawned.get(), 6) && enabled->SetValue(spawned.get(), false) &&
		label->SetValue(spawned.get(), std::string{ "scene text" }) && offset->SetValue(spawned.get(), sf::Vector2f{ 8.f, -4.f }) &&
		mode->SetValue(spawned.get(), 4) && values->SetValue(spawned.get(), PropertyList{ 3, 5, 8 }), "Scene fixture values must be writable");
	const auto saved = SceneSerializer::Save(source);
	Check(saved["entities"].size() == 1, "Only scene entities may be saved");
	const auto& savedProperties = saved["entities"][0]["properties"];
	Check(savedProperties["Gain"] == 3.5f && savedProperties.at("Mode") == Json("Active") &&
		savedProperties["Values"] == Json::array({ 3, 5, 8 }) && !savedProperties.contains("Locked"),
		"Fields must save their values and read-only fields must be omitted");
	World loaded{ nullptr };
	const auto result = SceneSerializer::Load(loaded, saved);
	const auto restored = loaded.GetActorsByTypeIncludingPending<FieldActor>()[0].lock();
	Check(result.warnings.empty() && restored && restored->GetGain() == 3.5f && restored->GetCount() == 6 && !restored->GetEnabled() &&
		restored->GetLabel() == "scene text" && restored->GetOffset() == sf::Vector2f{ 8.f, -4.f } && restored->GetMode() == FieldMode::Active &&
		restored->GetValues() == std::vector<int>{ 3, 5, 8 }, "Scene round trip must keep field values");

	auto clampEntity = Entity("FieldActor");
	clampEntity["properties"] = { { "Count", 15 }, { "Gain", -3.f } };
	World clampWorld{ nullptr };
	const auto clampResult = SceneSerializer::Load(clampWorld, Scene(Json::array({ clampEntity })));
	const auto clamped = clampWorld.GetActorsByTypeIncludingPending<FieldActor>()[0].lock();
	Check(clampResult.warnings.empty() && clamped && clamped->GetCount() == 10 && clamped->GetGain() == 0.f,
		"Scene loads must clamp fields to their Range");

	if (failures) return 1;
	std::cout << "Reflection field tests passed\n";
	return 0;
}
