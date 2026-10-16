#include "framework/EngineContext.h"
#include "framework/reflection/Reflection.h"
#include "reflection/GameReflection.h"
#include "script/ScriptRuntime.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

namespace
{
	nlohmann::json DumpRegistry()
	{
		nlohmann::json types = nlohmann::json::array();
		for (const auto* type : ly::reflect::TypeRegistry::Get().RegisteredTypes())
		{
			nlohmann::json properties = nlohmann::json::array();
			for (const auto& property : type->properties)
			{
				properties.push_back({
					{ "name", property.name }, { "kind", static_cast<int>(property.kind) },
					{ "hasSetter", static_cast<bool>(property.setter) },
					{ "min", property.min ? nlohmann::json(*property.min) : nlohmann::json(nullptr) },
					{ "max", property.max ? nlohmann::json(*property.max) : nlohmann::json(nullptr) },
					{ "speed", property.speed }, { "tooltip", property.tooltip },
					{ "category", property.category }, { "labels", property.enumLabels },
					{ "elementKind", property.kind == ly::reflect::PropertyKind::List ? nlohmann::json(static_cast<int>(property.elementKind)) : nlohmann::json(nullptr) },
					{ "requiredType", property.requiredType }
				});
			}
			types.push_back({
				{ "name", type->name }, { "base", type->base ? type->base->name : "" },
				{ "root", static_cast<int>(type->root) }, { "spawnable", type->spawnable },
				{ "addable", static_cast<bool>(type->addComponent) }, { "removable", type->removable },
				{ "requiredComponents", type->requiredComponents }, { "properties", properties }
			});
		}
		return types;
	}
}

int main(int argc, char** argv)
{
	ly::EngineContext engineContext;
	ly::EngineContext::Scope contextScope{ engineContext };
	ly::RegisterGameTypes();
	vega::script::RegisterScriptTypes();
	const auto actual = DumpRegistry();
	// Explicit capture is used once before a registration refactor; ordinary tests never rewrite the oracle.
	if (argc == 3 && std::string{ argv[1] } == "--dump")
	{
		std::ofstream output{ argv[2] };
		output << actual.dump(2) << '\n';
		return output ? 0 : 1;
	}
	std::ifstream input{ REFLECTION_GOLDEN_PATH };
	if (!input) { std::cerr << "Cannot open reflection golden\n"; return 1; }
	const auto expected = nlohmann::json::parse(input);
	if (actual != expected)
	{
		std::cerr << "Reflection registry differs from pre-macro golden:\n"
			<< nlohmann::json::diff(expected, actual).dump(2) << '\n';
		return 1;
	}
	// Repeated module registration must preserve order, descriptors and custom hooks.
	ly::RegisterGameTypes();
	vega::script::RegisterScriptTypes();
	if (DumpRegistry() != actual) return 1;
	std::cout << "Full reflection registry matches pre-macro golden (" << actual.size() << " types)\n";
	return 0;
}
