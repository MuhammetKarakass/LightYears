#include <gas/GasContentSchemas.h>
#include <iostream>

int main()
{
	using namespace ly::content;
	sas::RegisterContentSchemas();
	auto& registry = ContentSchemaRegistry::Get();
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
	sas::RegisterContentSchemas();
	if (registry.Find("effect")->file != "my-effects.json") return 4;
	std::cout << "Generic GAS schemas and game override preservation passed\n";
	return 0;
}
