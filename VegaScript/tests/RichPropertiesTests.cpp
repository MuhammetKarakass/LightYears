#include "framework/EngineContext.h"
#include <framework/ProjectSettings.h>
#include <framework/CollisionComponent.h>
#include <framework/Actor.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <framework/scene/PrefabLibrary.h>
#include <framework/reflection/Reflection.h>
#include <script/ScriptComponent.h>
#include <script/ScriptRuntime.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif

namespace
{
	using Json = nlohmann::json;
	using namespace vega;
	using namespace vega::reflect;
	int failures = 0;
	void Check(bool condition, const char* message)
	{
		if (!condition) { ++failures; std::cerr << message << '\n'; }
	}
	enum class Mode { A = 1, B = 3 };
	struct RichWave
	{
		VEGA_STRUCT(RichWave)
		std::string prefab;
		int count = 1;
	};
	VEGA_REFLECT_STRUCT(RichWave)
	{
		VEGA_FIELD("Prefab", prefab);
		VEGA_FIELD("Count", count).Range(1.f, 50.f);
	}
	class RichActor : public Actor
	{
		VEGA_CLASS(RichActor)
	public:
		using Actor::Actor;
		ActorRef target;
		std::vector<ActorRef> refs;
		std::vector<bool> bools;
		std::vector<int> ints;
		std::vector<float> floats;
		std::vector<sf::Vector2f> points;
		std::vector<sf::Color> colors;
		std::vector<std::string> strings;
		std::vector<Mode> modes;
		std::vector<RichWave> waves;
		ActorRef GetTarget() const { return target; }
		void SetTarget(const ActorRef& value) { target = value; }
	};
	VEGA_REFLECT(RichActor, Actor)
	{
		VEGA_SPAWNABLE();
		VEGA_ACTOR_REF("Target", GetTarget, SetTarget).RequiredType("RichActor");
		type.Property("Refs", [](const Self& a) { return a.refs; }, [](Self& a, const std::vector<ActorRef>& v) { a.refs = v; }).RequiredType("RichActor");
		type.Property("Bools", [](const Self& a) { return a.bools; }, [](Self& a, const std::vector<bool>& v) { a.bools = v; });
		type.Property("Ints", [](const Self& a) { return a.ints; }, [](Self& a, const std::vector<int>& v) { a.ints = v; });
		type.Property("Floats", [](const Self& a) { return a.floats; }, [](Self& a, const std::vector<float>& v) { a.floats = v; });
		type.Property("Points", [](const Self& a) { return a.points; }, [](Self& a, const std::vector<sf::Vector2f>& v) { a.points = v; });
		type.Property("Colors", [](const Self& a) { return a.colors; }, [](Self& a, const std::vector<sf::Color>& v) { a.colors = v; });
		type.Property("Strings", [](const Self& a) { return a.strings; }, [](Self& a, const std::vector<std::string>& v) { a.strings = v; });
		type.Property("Modes", [](const Self& a) { return a.modes; }, [](Self& a, const std::vector<Mode>& v) { a.modes = v; }).Labels({ { 1, "A" }, { 3, "B" } });
		VEGA_FIELD("Waves", waves);
	}
	Json Scene(Json entities)
	{
		if (entities.is_object()) entities = Json::array({entities});
		return { { "format", "vega.scene" }, { "version", 1 }, { "settings", Json::object() }, { "entities", entities } };
	}
	Json Entity(const std::string& guid)
	{
		return { { "guid", guid }, { "name", guid }, { "type", "RichActor" } };
	}
	std::shared_ptr<RichActor> Find(World& world, const std::string& guid)
	{
		for (const auto& weak : world.GetActorsByTypeIncludingPending<RichActor>())
			if (const auto a = weak.lock(); a && !a->GetIsPendingDestroy())
				if (a->FindComponent<SceneEntityComponent>()->guid == guid) return a;
		return {};
	}
	void Serialization()
	{
		std::cerr << "Serialization\n";
		World world{ nullptr };
		auto a = Entity("a"), b = Entity("b");
		a["properties"] = { { "Target", {{"guid", "b"}} }, { "Refs", Json::array({ {{"guid", "b"}}, {{"guid", ""}} }) },
			{ "Bools", { true, false } }, { "Ints", { -4, 6 } }, { "Floats", { 0.25, 2.5 } },
			{ "Points", { { 10, 20 }, { 30, 40 } } }, { "Colors", { { 1, 2, 3, 4 } } },
			{ "Strings", { "a", "b" } }, { "Modes", { "A", "B" } } };
		Check(SceneSerializer::Load(world, Scene({a, b})).warnings.empty(), "Forward references and every list kind must load");
		const auto actor = Find(world, "a"), target = Find(world, "b");
		Check(actor && actor->target.Resolve(world) == target, "Forward GUID must resolve");
		const auto saved = SceneSerializer::Save(world);
		World reopened{ nullptr };
		Check(SceneSerializer::Load(reopened, saved).warnings.empty(), "Rich scene must reopen without warnings");
		Check(SceneSerializer::Save(reopened) == saved, "Every scalar list must round-trip exactly");
		const auto* type = TypeRegistry::Get().Find("RichActor");
		std::string error;
		Check(!type->FindProperty("Points")->SetValue(actor.get(), PropertyList{ sf::Vector2f{ std::numeric_limits<float>::infinity(), 0 } }, &error), "Non-finite list element must fail");
		Check(actor->points.size() == 2, "Rejected list write must be atomic");
		Check(!type->FindProperty("Modes")->SetValue(actor.get(), PropertyList{ 2 }), "Unlabelled enum list element must fail");
		Check(!type->FindProperty("Ints")->SetValue(actor.get(), PropertyList{ 1.f }), "Wrong list element kind must fail");
		auto wrong = world.SpawnActor<Actor>().lock();
		Check(!type->FindProperty("Target")->SetValue(actor.get(), ActorRef{ *wrong }), "Reference type filter must reject incompatible actor");
		target->Destroy();
		Check(!actor->target.Resolve(world), "Destroyed actor reference must become null");
		auto replacement = world.SpawnActor<RichActor>().lock();
		replacement->AddComponent<SceneEntityComponent>("b", "Replacement");
		Check(actor->target.Resolve(world) == replacement, "Reference cache must recover after replacement with same GUID");
		a["properties"]["Target"]["guid"] = "missing";
		World dangling{ nullptr };
		Check(!SceneSerializer::Load(dangling, Scene({a})).warnings.empty(), "Dangling reference must warn");
		Check(!Find(dangling, "a")->target.Resolve(dangling), "Dangling reference must be null-safe");
		Check(SceneSerializer::Save(dangling)["entities"][0]["properties"]["Target"]["guid"] == "missing", "Dangling GUID must survive save");
	}
	void Prefabs(const std::filesystem::path& root)
	{
		std::cerr << "Prefabs\n";
		auto a = Entity("a"), b = Entity("b");
		a["properties"] = { { "Target", {{"guid", "b"}} }, { "Refs", Json::array({ {{"guid", "b"}}, {{"guid", "external"}} }) } };
		b["id"] = "door";
		b["properties"] = { { "Target", {{"guid", "a"}} } };
		a["children"] = Json::array({ b });
		auto copies = Json::array({a});
		SceneSerializer::RemapGuids(copies);
		const auto copyGuid = copies[0]["guid"], childGuid = copies[0]["children"][0]["guid"];
		Check(copies[0]["properties"]["Target"]["guid"] == childGuid && copies[0]["children"][0]["properties"]["Target"]["guid"] == copyGuid, "Copy must remap internal references in both directions");
		Check(copies[0]["properties"]["Refs"][1]["guid"] == "external", "Copy must preserve external list reference");
		std::filesystem::create_directories(root / "prefabs");
		const auto document = PrefabLibrary::Document(a, "Connected");
		Check(document["properties"]["Target"]["guid"] == "$prefab:/door", "Prefab must persist stable internal child address");
		std::ofstream{ root / "prefabs/connected.prefab.json" } << document.dump(2);
		std::ofstream{ root / "prefabs/variant.prefab.json" } << R"({"format":"vega.prefab","version":1,"base":{"prefab":"prefabs/connected.prefab.json"}})";
		std::ofstream{ root / "prefabs/nested.prefab.json" } << R"({"format":"vega.prefab","version":1,"base":{"type":"RichActor"},"children":[{"id":"nested","prefab":"prefabs/connected.prefab.json"}]})";
		PrefabLibrary::Get().Reload();
		World world{ nullptr };
		auto one = Json{ { "guid", "one" }, { "prefab", "prefabs/variant.prefab.json" } };
		auto two = Json{ { "guid", "two" }, { "prefab", "prefabs/nested.prefab.json" } };
		Check(SceneSerializer::Load(world, Scene({Entity("external"), one, two})).warnings.empty(), "Variants and nested references must load");
		auto first = Find(world, "one"), second = Find(world, "two/nested");
		Check(first && first->target.Resolve(world) == Find(world, "one/door"), "First placement must link its own child");
		Check(second && second->target.Resolve(world) == Find(world, "two/nested/door"), "Nested placement must rebase internal references");
		Check(first && first->refs[1].Resolve(world) == Find(world, "external"), "Prefab placement must preserve external reference");
		if (first)
		{
			const auto sparse = SceneSerializer::SaveEntity(*first);
			Check(!sparse.value("properties", Json::object()).contains("Target"), "Inherited references must not become overrides");
			first->ints = { 1, 2 };
			Check(SceneSerializer::SaveEntity(*first)["properties"]["Ints"] == Json({1,2}), "List override must be one complete array");
		}
	}
	void Lua(const std::filesystem::path& root)
	{
		World world{ nullptr };
		ProjectSettings project;
		project.collisionLayers = { { 0, "Solid", {} }, { 7, "Sensor", {} } };
		project.ApplyCollisionLayers(world.GetEngineContext());
		Check(SceneSerializer::Load(world, Scene({Entity("a"), Entity("b")})).warnings.empty(), "Lua fixture scene must load");
		Find(world, "a")->AddComponent<CollisionComponent>();
		std::ofstream{ root / "rich.lua" } << R"(
function on_start(self)
	local a, b = self.actor, world.find('b')
	local collision = a:component('CollisionComponent')
	assert(collision:set('Collision Mask', 'Solid | Sensor'))
	assert(collision:get('Collision Mask') == 129)
	assert(collision:set('Collision Layer', 'Sensor'))
	assert(collision:get('Collision Layer') == 128)
	assert(not collision:set('Collision Mask', 'Unknown'))
	assert(collision:get('Collision Mask') == 129)
	assert(a:get('Target') == nil)
	assert(a:set('Target', b))
	assert(a:get('Target'):id() == b:id())
	assert(a:set('Target', nil) and a:get('Target') == nil)
	assert(not a:set('Target', 'b'))
	assert(a:set('Bools', {true,false}))
	assert(a:set('Ints', {-2147483648,2147483647}))
	assert(not a:set('Ints', {1.5}) and not a:set('Ints', {2147483648}))
	assert(not a:set('Ints', {[1]=1,[3]=3}) and not a:set('Ints', {oops=1}))
	assert(a:set('Floats', {0.5,2.5}) and not a:set('Floats', {0/0}))
	assert(a:set('Points', {vec(10,20),{x=30,y=40}}))
	local p = a:get('Points'); assert(#p==2 and p[2].x==30 and a:set('Points',p))
	assert(a:set('Colors', {{r=1,g=2,b=3,a=4}}))
	assert(a:get('Colors')[1].a==4)
	assert(a:set('Strings', {'a','b'}))
	assert(a:set('Modes', {1,3}) and not a:set('Modes', {2}))
	assert(a:set('Refs', {b,b}))
	local refs=a:get('Refs'); refs[2]=nil
	assert(a:set('Refs',refs)); refs=a:get('Refs')
	assert(refs.n==2 and refs[1]:id()==b:id() and refs[2]==nil)
	assert(a:get('Waves').n == 0)
	assert(a:set('Waves', {{Prefab='a.prefab.json', Count=3}, {Count=7}}))
	local waves = a:get('Waves')
	assert(waves.n == 2 and waves[1].Prefab == 'a.prefab.json' and waves[2].Count == 7)
	waves[2].Count = 99
	assert(a:set('Waves', waves) and a:get('Waves')[2].Count == 50)
	assert(not a:set('Waves', {{Count='x'}}) and a:get('Waves')[2].Count == 50)
	b:destroy()
	assert(a:get('Refs')[1]==nil)
end
)";
		world.TickInternal(0.f);
		auto& script = Find(world, "a")->AddComponent<vega::script::ScriptComponent>("rich.lua");
		world.TickInternal(0.01f);
		Check(script.LastError().empty(), script.LastError().c_str());
		Check(Find(world, "a")->points.size() == 2, "Lua must write Vec2 list through reflected setter");
	Check(Find(world, "a")->waves.size() == 2 && Find(world, "a")->waves[1].count == 50, "Lua must write struct lists through reflected fields");
	}
}

int main(int argc, char** argv)
{
	vega::EngineContext engineContext;
	vega::EngineContext::Scope contextScope{ engineContext };
#ifdef _MSC_VER
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
	const auto root = std::filesystem::path{ VEGA_SCRIPT_TEST_ROOT } / (argc > 1 ? argv[1] : "reflection");
	std::filesystem::create_directories(root);
	vega::script::SetScriptRoot(root);
	vega::PrefabLibrary::Get().SetRoot(root);
	vega::script::RegisterScriptTypes();
	VEGA_REGISTER(RichWave);
	VEGA_REGISTER(RichActor);
	if (argc > 1 && std::string{ argv[1] } == "lua") Lua(root);
	else { Serialization(); Prefabs(root); }
	return failures ? 1 : 0;
}
