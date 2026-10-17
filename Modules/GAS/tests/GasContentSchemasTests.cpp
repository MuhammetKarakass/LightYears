#include <gas/GasContentSchemas.h>
#include <framework/EngineContext.h>
#include <iostream>

int main()
{
	using namespace vega::content;
	vega::EngineContext context;
	vega::EngineContext unrelated;
	vega::EngineContext::Scope unrelatedScope{ unrelated };
	sas::RegisterContentSchemas(context);
	auto& registry = context.ContentSchemas();
	if (!unrelated.ContentSchemas().Types().empty()) return 5;
	const auto* effect = registry.Find("effect");
	const auto* ability = registry.Find("ability");
	if (!effect || !ability || registry.Find("ship") || registry.Find("module") || registry.Find("weapon")) return 1;
	const auto field = sas::MakeContentField("/modifiers/*/attributeId", "\"Health\"");
	if (field.kind != FieldKind::Attribute || field.defaultValue != "Health") return 2;
	const auto duration = sas::MakeContentField("/duration", "2.0");
	if (duration.minimum != 0. || duration.unit != "s") return 3;
	ContentTypeSchema custom = *effect;
	custom.file = "my-effects.json";
	registry.Register(custom);
	sas::RegisterContentSchemas(context);
	if (registry.Find("effect")->file != "my-effects.json") return 4;
	sas::RegisterContentSchemas(unrelated);
	if (!unrelated.ContentSchemas().Find("ability") || !unrelated.ContentSchemas().Find("effect") ||
		unrelated.ContentSchemas().Find("effect")->file != "content/data/effects.json") return 6;
	if (registry.Find("effect")->file != "my-effects.json") return 7;
	std::cout << "Generic GAS schemas and game override preservation passed\n";
	return 0;
}
