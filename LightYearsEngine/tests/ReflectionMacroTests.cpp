#include "framework/EngineContext.h"
#include "framework/Actor.h"
#include "framework/reflection/Reflection.h"
#include <algorithm>
#include <iostream>
#include <type_traits>

namespace
{
	int failures = 0;
	void Check(bool condition, const char* message)
	{
		if (!condition) { std::cerr << message << '\n'; ++failures; }
	}

	enum class BeaconMode { Steady = 2, Pulse = 7 };
	class BeaconComponent : public ly::ActorComponent
	{
		VEGA_CLASS(BeaconComponent)
	public:
		BeaconComponent(ly::Actor& owner, int count, float gain)
			: ActorComponent{ owner }, count{ count }, gain{ gain } {}
		int count;
		float gain;
	};
	class ExampleBeacon : public ly::Actor
	{
		VEGA_CLASS(ExampleBeacon)
	public:
		explicit ExampleBeacon(ly::World* world = nullptr) : Actor{ world } {}
		float GetBrightness() const { return brightness; }
		void SetBrightness(float value, bool record = true)
		{
			brightness = std::clamp(value, 0.f, 1.f);
			recorded = record;
		}
		bool IsLit() const { return brightness > 0.f; }
		BeaconMode GetMode() const { return mode; }
		void SetMode(BeaconMode value) { mode = value; }
		int GetMask() const { return mask; }
		void SetMask(int value) { mask = value; }
		// The macro must select the integer overload rather than take an ambiguous member pointer.
		void SetMask(const std::string&) {}
		bool recorded = false;
	private:
		float brightness = 0.5f;
		BeaconMode mode = BeaconMode::Steady;
		int mask = 0;
	};
	class DerivedBeacon : public ExampleBeacon
	{
		VEGA_CLASS(DerivedBeacon)
	public:
		using ExampleBeacon::ExampleBeacon;
	};
	static_assert(!std::is_convertible_v<ly::reflect::TypeTag<DerivedBeacon>, ly::reflect::TypeTag<ExampleBeacon>>);
	class RootExample
	{
		VEGA_CLASS(RootExample)
	public:
		virtual ~RootExample() = default;
		bool IsReady() const { return true; }
	};

	VEGA_REFLECT_ROOT(RootExample)
	{
		VEGA_READONLY("Ready", IsReady);
	}
	VEGA_REFLECT(BeaconComponent, ly::ActorComponent)
	{
		VEGA_ADDABLE(17, 0.25f);
	}
	VEGA_REFLECT(ExampleBeacon, ly::Actor)
	{
		VEGA_SPAWNABLE();
		VEGA_REQUIRES(BeaconComponent);
		VEGA_CATEGORY("Beacon");
		VEGA_PROPERTY("Brightness", GetBrightness, SetBrightness).Range(0.f, 1.f).Speed(0.05f).Tooltip("Light output");
		VEGA_READONLY("Lit", IsLit);
		VEGA_PROPERTY("Mode", GetMode, SetMode).Labels({ { 2, "Steady" }, { 7, "Pulse" } }).Category("Appearance");
		VEGA_PROPERTY("Mask", GetMask, SetMask).Flags({ { 1, "Near" }, { 2, "Far" } });
		VEGA_CATEGORY("Status");
		type.Property("Recorded", [](const Self& o) { return o.recorded; });
	}
	VEGA_REFLECT(DerivedBeacon, ExampleBeacon)
	{
		VEGA_SPAWNABLE([](ly::World& world) { return world.SpawnActor<Self>().lock(); });
	}
	class RemovableComponent : public ly::ActorComponent
	{
		VEGA_CLASS(RemovableComponent)
	public:
		using ActorComponent::ActorComponent;
	};
	VEGA_REFLECT(RemovableComponent, ly::ActorComponent)
	{
		VEGA_REMOVABLE();
	}

	void RegisterExamples()
	{
		VEGA_REGISTER(RootExample);
		VEGA_REGISTER(BeaconComponent);
		VEGA_REGISTER(ExampleBeacon);
		VEGA_REGISTER(DerivedBeacon);
		VEGA_REGISTER(RemovableComponent);
	}
}

int main()
{
	ly::EngineContext engineContext;
	ly::EngineContext::Scope contextScope{ engineContext };
	using namespace ly::reflect;
	RegisterEngineTypes();
	RegisterExamples();
	auto& registry = TypeRegistry::Get();
	const auto* type = registry.Find("ExampleBeacon");
	const auto* derived = registry.Find("DerivedBeacon");
	Check(type && type == registry.Find(typeid(ExampleBeacon)), "Macro must register the exact type token as its name");
	Check(derived && derived->base == type && type->base == registry.Find("Actor"), "Explicit base-first macro registration must preserve base chain");
	Check(type->root == TypeRoot::Actor && type->spawnable && type->create && derived->create, "Actor root/spawn factories missing");
	Check(registry.Find("RootExample")->root == TypeRoot::Other && !registry.Find("RootExample")->base, "Root macro must use void base");
	Check(derived->RequiresComponent("BeaconComponent") && type->requiredComponents == std::vector<std::string>{ "BeaconComponent" }, "Macro component requirement must be inherited");
	const auto* brightness = type->FindProperty("Brightness");
	const auto* lit = type->FindProperty("Lit");
	const auto* mode = type->FindProperty("Mode");
	Check(brightness->kind == PropertyKind::Float && lit->kind == PropertyKind::Bool && mode->kind == PropertyKind::Enum, "Macro property kinds changed");
	Check(brightness->min == 0.f && brightness->max == 1.f && brightness->speed == 0.05f && brightness->tooltip == "Light output", "Macro fluent metadata missing");
	Check(brightness->category == "Beacon" && lit->category == "Beacon" && mode->category == "Appearance" &&
		type->FindProperty("Mask")->category == "Beacon" && type->FindProperty("Recorded")->category == "Status", "Default category must be sticky with per-property overrides");
	Check(!lit->setter && mode->enumLabels == EnumLabels{ { 2, "Steady" }, { 7, "Pulse" } }, "Readonly/enum labels missing");
	DerivedBeacon beacon;
	ly::Actor* root = &beacon;
	Check(brightness->SetValue(root, 3.f) && beacon.GetBrightness() == 1.f && beacon.recorded, "SetValue must call the clamping setter with its default argument");
	Check(brightness->SetValue(root, -2.f) && beacon.GetBrightness() == 0.f && !std::get<bool>(lit->getter(root)), "Setter lower clamp or readonly getter failed");
	Check(!lit->SetValue(root, true), "Readonly macro must reject writes");
	Check(mode->SetValue(root, 7) && beacon.GetMode() == BeaconMode::Pulse, "Macro enum setter failed");
	const auto* mask = type->FindProperty("Mask");
	Check(mask->kind == PropertyKind::Flags && mask->SetValue(root, 3) && beacon.GetMask() == 3, "Flags/overloaded setter failed");
	Check(registry.FindMostDerived(beacon) == derived, "Macro derived registration must participate in discovery");
	const auto* componentType = registry.Find("BeaconComponent");
	Check(componentType->root == TypeRoot::Component && componentType->addComponent && componentType->removable, "Addable component metadata missing");
	auto& component = dynamic_cast<BeaconComponent&>(componentType->addComponent(beacon));
	Check(component.count == 17 && component.gain == 0.25f && &component.GetOwner() == &beacon, "Addable factory must forward constructor arguments");
	Check(registry.Find("RemovableComponent")->removable && !registry.Find("RemovableComponent")->addComponent, "Removable macro must not imply addable");
	ly::World world{ nullptr };
	Check(type->create(world) && derived->create(world), "Both spawnable macro forms must create actors");
	if (failures) return 1;
	std::cout << "Reflection macro tests passed\n";
	return 0;
}
