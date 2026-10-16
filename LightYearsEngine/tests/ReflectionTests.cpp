#include "framework/EngineContext.h"
#include "framework/Actor.h"
#include "framework/reflection/Reflection.h"
#include "framework/CollisionComponent.h"
#include "framework/ComponentIdentity.h"
#include <iostream>
#include <limits>

namespace
{
	int failures = 0;
	void Check(bool condition, const char* message)
	{
		if (!condition) { std::cerr << message << '\n'; ++failures; }
	}

	enum class Mode { First = 2, Second = 7 };
	struct Padding { virtual ~Padding() = default; int padding = 123; };
	class SampleActor : public Padding, public ly::Actor
	{
	public:
		SampleActor() : Actor{ nullptr } {}
		float GetNumber() const { return number; }
		void SetNumber(float value) { number = value; }
		const std::string& GetText() const { return text; }
		void SetText(const std::string& value) { text = value; }
		int GetCount() const { return count; }
		void SetCount(const int& value) { count = value; }
		const sf::Vector2f& GetPoint() const { return point; }
		void SetPoint(sf::Vector2f value) { point = value; }
		sf::Color GetTint() const { return tint; }
		void SetTint(const sf::Color& value) { tint = value; }
		Mode GetMode() const { return mode; }
		void SetMode(Mode value) { mode = value; }
	private:
		float number = 3.f;
		std::string text = "original";
		int count = 4;
		sf::Vector2f point{};
		sf::Color tint = sf::Color::White;
		Mode mode = Mode::First;
	};
	class RegisteredActor : public SampleActor {};
	class UnregisteredActor : public RegisteredActor {};
	class SampleComponent : public ly::ActorComponent
	{
	public:
		using ActorComponent::ActorComponent;
	};
	class UnregisteredComponent : public SampleComponent
	{
	public:
		using SampleComponent::SampleComponent;
	};
}

int main()
{
	ly::EngineContext engineContext;
	ly::EngineContext::Scope contextScope{ engineContext };
	using namespace ly::reflect;
	static_assert(std::is_default_constructible_v<TypeRegistry> && !std::is_copy_constructible_v<TypeRegistry> &&
		!std::is_move_constructible_v<TypeRegistry>, "Registries must be independently owned and keep descriptor addresses stable");
	RegisterEngineTypes();
	Check(KindOf(PropertyValue{true}) == PropertyKind::Bool && KindOf(PropertyValue{2}) == PropertyKind::Int &&
		KindOf(PropertyValue{2.f}) == PropertyKind::Float && KindOf(PropertyValue{sf::Vector2f{}}) == PropertyKind::Vector2 &&
		KindOf(PropertyValue{sf::Color{}}) == PropertyKind::Color && KindOf(PropertyValue{std::string{}}) == PropertyKind::String,
		"KindOf must describe all value alternatives");
	ly::Actor flagsActor{ nullptr };
	auto& collision = flagsActor.AddComponent<ly::CollisionComponent>();
	const auto* flags = TypeRegistry::Get().Find("CollisionComponent")->FindProperty("Collision Mask");
	Check(flags->kind == PropertyKind::Flags && flags->enumLabels.size() == 8, "Collision flags need eight bit labels");
	Check(flags->SetValue(&collision, 5) && std::get<int>(flags->getter(&collision)) == 5, "Flags combination failed");
	Check(!flags->SetValue(&collision, 256) && !flags->SetValue(&collision, -1) && !flags->SetValue(&collision, 5.f) &&
		std::get<int>(flags->getter(&collision)) == 5, "Flags must reject undeclared bits without mutation");
	Check(flags->SetValue(&collision, 0) && flags->SetValue(&collision, 255), "Zero/all declared flags must work");
	auto& registry = TypeRegistry::Get();
	const auto* actorType = registry.Find("Actor");
	const auto count = actorType->AllProperties().size();
	RegisterEngineTypes();
	Check(registry.Find("Actor") == actorType && actorType->AllProperties().size() == count, "Engine registration must be idempotent");
	Class<SampleActor, ly::Actor>("SampleActor")
		.Property("Number", &SampleActor::GetNumber, &SampleActor::SetNumber).Range(0.f, 100.f).Speed(0.25f).Category("Sample").Tooltip("A number")
		.Property("Text", &SampleActor::GetText, &SampleActor::SetText)
		.Property("Count", &SampleActor::GetCount, &SampleActor::SetCount)
		.Property("Point", &SampleActor::GetPoint, &SampleActor::SetPoint)
		.Property("Tint", &SampleActor::GetTint, &SampleActor::SetTint)
		.Property("Mode", &SampleActor::GetMode, &SampleActor::SetMode).Labels({ { 2, "First" }, { 7, "Second" } })
		.Property("Read Only", &SampleActor::GetNumber);
	Class<RegisteredActor, SampleActor>("RegisteredActor");
	Class<SampleComponent, ly::ActorComponent>("SampleComponent");
	UnregisteredActor actor;
	ly::Actor* root = &actor;
	const auto* type = registry.FindMostDerived(*root);
	Check(type && type->name == "RegisteredActor", "Unregistered actor must fall back to closest registered base");
	Check(registry.FindMostDerived(ly::Actor{ nullptr }) == actorType, "Root fallback failed");
	Check(type->AllProperties().front()->name == "Location" && type->AllProperties().size() == count + 7, "Properties must include base first");
	Check(!type->isInstance(nullptr) && type->isInstance(root), "Instance predicate failed");
	const auto* number = type->FindProperty("Number");
	Check(number->min == 0.f && number->max == 100.f && number->speed == 0.25f && number->category == "Sample" && number->tooltip == "A number", "Metadata lost");
	Check(number->SetValue(root, 9.f) && actor.GetNumber() == 9.f && std::get<float>(number->getter(root)) == 9.f, "Value getter/setter failed through adjusted root");
	Check(!number->SetValue(root, 9) && !number->SetValue(root, true) && actor.GetNumber() == 9.f, "Wrong variant must be refused without mutation");
	Check(!number->SetValue(root, std::numeric_limits<float>::infinity()) && !number->SetValue(nullptr, 1.f), "Nonfinite/null writes must fail");
	Check(type->FindProperty("Text")->SetValue(root, std::string{ "changed" }) && actor.GetText() == "changed", "Const-reference getter/setter failed");
	Check(type->FindProperty("Count")->SetValue(root, 8) && actor.GetCount() == 8, "Const-reference scalar setter failed");
	Check(type->FindProperty("Point")->SetValue(root, sf::Vector2f{ 2.f, 3.f }) && actor.GetPoint() == sf::Vector2f{ 2.f, 3.f }, "Const-reference vector getter failed");
	Check(type->FindProperty("Tint")->SetValue(root, sf::Color::Red) && actor.GetTint() == sf::Color::Red, "Color setter failed");
	const auto* mode = type->FindProperty("Mode");
	Check(mode->kind == PropertyKind::Enum && mode->enumLabels[1].first == 7 && mode->enumLabels[1].second == "Second", "Enum labels lost");
	Check(mode->SetValue(root, 7) && actor.GetMode() == Mode::Second && std::get<int>(mode->getter(root)) == 7, "Enum conversion failed");
	std::string enumError;
	Check(!mode->SetValue(root, 7.f) && !mode->SetValue(root, 3, &enumError) && actor.GetMode() == Mode::Second &&
		enumError.find("not one of the enum's labels") != std::string::npos,
		"A labelled enum must reject unlisted integers with a reason and keep its value");
	PropertyInfo throwing = *number;
	throwing.setter = [](void*, const PropertyValue&) { throw std::runtime_error("setter refused"); };
	std::string error;
	Check(!throwing.SetValue(root, 1.f, &error) && error == "setter refused" && actor.GetNumber() == 9.f,
		"Setter exceptions must return their reason without escaping or changing the value");
	throwing.setter = [](void*, const PropertyValue&) { throw 42; };
	Check(!throwing.SetValue(root, 1.f, &error) && error == "Unknown property setter error", "Nonstandard setter exceptions must be contained");
	Check(number->SetValue(root, 9.f, &error) && error.empty(), "Successful writes must clear old errors");
	const auto* readOnly = type->FindProperty("Read Only");
	Check(!readOnly->setter && !readOnly->SetValue(root, 12.f) && actor.GetNumber() == 9.f, "Read-only write must fail");
	Check(!type->FindProperty("Missing"), "Missing property must return null");
	auto& first = actor.AddComponent<ly::ActorComponent>();
	auto& second = actor.AddComponent<UnregisteredComponent>();
	Check(registry.FindMostDerived(second)->name == "SampleComponent", "Component fallback failed");
	Check(registry.FindMostDerived(first)->name == "ActorComponent", "Component root fallback failed");
	std::vector<const ly::ActorComponent*> visited;
	actor.ForEachComponent([&](ly::ActorComponent& component) { visited.push_back(&component); });
	Check(visited == std::vector<const ly::ActorComponent*>{ &first, &second }, "Mutable component order changed");
	visited.clear();
	static_cast<const ly::Actor&>(actor).ForEachComponent([&](const ly::ActorComponent& component) { visited.push_back(&component); });
	Check(visited == std::vector<const ly::ActorComponent*>{ &first, &second }, "Const component order changed");
	Check(registry.FindMostDerived(first)->FindProperty("Active")->SetValue(&first, false) && !first.IsActive(), "Bool property failed");
	const auto componentID = ly::IdentifyComponent(actor, second);
	ly::Actor reordered{ nullptr };
	reordered.AddComponent<ly::CollisionComponent>();
	reordered.AddComponent<ly::ActorComponent>();
	auto& sameType = reordered.AddComponent<UnregisteredComponent>();
	Check(componentID && ly::FindComponentByTypeOccurrence(reordered, *componentID) == &sameType,
		"Component identity must ignore earlier components of other types");
	auto& repeated = reordered.AddComponent<UnregisteredComponent>();
	Check(ly::FindComponentByTypeOccurrence(reordered, { "SampleComponent", 1 }) == &repeated &&
		!ly::FindComponentByTypeOccurrence(reordered, { "SampleComponent", 2 }), "Type occurrences must distinguish repeated components");
	if (failures) return 1;
	std::cout << "Reflection tests passed\n";
	return 0;
}
