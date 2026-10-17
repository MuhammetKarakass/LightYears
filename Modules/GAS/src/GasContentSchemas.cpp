#include <gas/GasContentSchemas.h>
#include <framework/EngineContext.h>
#include <algorithm>

namespace sas
{
	ly::content::ContentField MakeContentField(const std::string& path, const char* example, bool curve)
	{
		using namespace ly::content;
		ContentField field;
		field.path = path;
		field.defaultValue = AuthoringJson::parse(example);
		const auto key = path.substr(path.find_last_of('/') + 1);
		const auto& value = field.defaultValue;
		field.kind = value.is_boolean() ? FieldKind::Boolean : value.is_number() ? FieldKind::Number :
			value.is_object() ? FieldKind::Object : value.is_array() ? FieldKind::List : FieldKind::String;
		field.category = path.find("/progression") == 0 ? "Progression" : "Definition";
		field.tooltip = "Authored " + path + ". Validated by the registered content validator.";
		if (key == "attributeId" || key == "sourceAttributeId" || key == "targetAttributeId" ||
			(key == "id" && path.find("/attributes/*/id") != std::string::npos)) field.kind = FieldKind::Attribute;
		if (key == "effectId") field.referenceType = "effect";
		if (key == "abilityId" || key == "baseId") field.referenceType = "ability";
		if (!field.referenceType.empty()) field.kind = FieldKind::Reference;
		if (value.is_array() && (key.find("Tags") != std::string::npos || key == "tags")) field.kind = FieldKind::TagList;
		if (curve || (value.is_array() && (key.find("ByLevel") != std::string::npos || key == "values" || key == "magnitude"))) field.kind = FieldKind::Curve;
		if (key == "operation") field.options = {"Add", "Multiply", "Override"};
		if (key == "durationPolicy") field.options = {"Instant", "Duration", "Infinite"};
		if (key == "stackingPolicy") field.options = {"None", "RefreshDuration", "Stack"};
		if (key == "stackLifetimePolicy") field.options = {"None", "DecayAfterDuration"};
		if (!field.options.empty()) field.kind = FieldKind::Enum;
		if (key == "duration" || key == "cooldown" || key.find("Seconds") != std::string::npos || key == "chargeUseInterval") { field.minimum = 0.; field.unit = "s"; }
		if (key == "maxCharges" || key == "maxStacks" || key == "levelCount") field.minimum = 0.;
		return field;
	}

	void RegisterContentSchemas(ly::EngineContext& context)
	{
		using namespace ly::content;
		auto& registry = context.ContentSchemas();
		for (const auto* type : {"ability", "effect"})
		{
			if (registry.Find(type)) continue;
			ContentTypeSchema schema;
			schema.type = type;
			schema.collection = schema.type == "ability" ? "abilities" : "effects";
			schema.file = "content/data/" + schema.collection + ".json";
			const auto add = [&](const char* path, const char* example) { schema.fields.push_back(MakeContentField(path, example)); };
			add("/id", "\"\""); add("/duration", "0.0");
			if (schema.type == "ability")
			{
				add("/cooldown", "0.0"); add("/maxCharges", "0"); add("/baseId", "\"\"");
				add("/effectSpecs", "[]"); add("/effectSpecs/*", "{}"); add("/effectSpecs/*/effectId", "\"\"");
			}
			else
			{
				add("/durationPolicy", "\"Instant\""); add("/stackingPolicy", "\"None\""); add("/maxStacks", "1");
				add("/modifiers", "[]"); add("/modifiers/*", "{}"); add("/modifiers/*/attributeId", "\"\"");
				add("/modifiers/*/operation", "\"Add\""); add("/modifiers/*/magnitude", "0.0");
			}
			registry.Register(std::move(schema));
		}
	}
}
