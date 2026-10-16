#include "framework/EngineContext.h"
#include "framework/scene/SceneSettings.h"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace ly
{
	std::string SceneSettingField::Validate(const nlohmann::json& value) const
	{
		const auto number = [&](const nlohmann::json& item, bool integer, bool color)
		{
			if (!item.is_number() || (integer && !item.is_number_integer())) return false;
			const auto n = item.get<double>();
			return std::isfinite(n) && std::abs(n) <= std::numeric_limits<float>::max() && (!integer || (n >= std::numeric_limits<int>::min() && n <= std::numeric_limits<int>::max())) &&
				(!min || n >= *min) && (!max || n <= *max) && (!color || (n >= 0 && n <= 255));
		};
		bool valid = false;
		switch (type)
		{
		case SceneSettingType::Bool: valid = value.is_boolean(); break;
		case SceneSettingType::Int: valid = number(value, true, false); break;
		case SceneSettingType::Float: valid = number(value, false, false); break;
		case SceneSettingType::String:
		case SceneSettingType::CatalogId: valid = value.is_string(); break;
		case SceneSettingType::Enum: valid = value.is_string() || number(value, true, false); break;
		case SceneSettingType::Vec2:
		case SceneSettingType::Color:
		case SceneSettingType::Rect:
			if (type == SceneSettingType::Rect && value.is_null()) return {};
			valid = value.is_array() && value.size() == (type == SceneSettingType::Vec2 ? 2u : 4u);
			if (valid) for (const auto& item : value)
				valid = valid && number(item, type == SceneSettingType::Color, type == SceneSettingType::Color);
			if (valid && type == SceneSettingType::Rect)
				valid = value[2].get<double>() > 0 && value[3].get<double>() > 0;
			break;
		}
		if (!valid) return "Invalid type or value outside the allowed range";
		if (type == SceneSettingType::Enum || type == SceneSettingType::CatalogId)
		{
			if (!options) return "Options are unavailable";
			for (const auto& option : options()) if (option.value == value) return {};
			return "Value is not in the available options";
		}
		return {};
	}

	SceneSettingsSchema& SceneSettingsSchema::Get() { return EngineContext::Current().SceneSettings(); }
	void SceneSettingsSchema::RegisterEngineSettings()
	{
		SceneSettingField bounds;
		bounds.key = "bounds"; bounds.label = "Arena bounds"; bounds.category = "World";
		bounds.type = SceneSettingType::Rect; bounds.defaultValue = nullptr;
		bounds.min = -1000000; bounds.max = 1000000;
		bounds.tooltip = "Optional [x, y, width, height] in world units. Games choose how to enforce it.";
		Register(std::move(bounds));
	}
	std::optional<sf::FloatRect> ReadSceneBounds(const nlohmann::json& settings)
	{
		return ReadSceneBounds(settings, EngineContext::Current().SceneSettings());
	}
	std::optional<sf::FloatRect> ReadSceneBounds(const nlohmann::json& settings, const SceneSettingsSchema& schema)
	{
		if (!settings.is_object() || !settings.contains("bounds") || settings["bounds"].is_null()) return {};
		const auto& value = settings["bounds"];
		const auto* field = schema.Find("bounds");
		if (!field || !field->Validate(value).empty()) return {};
		return sf::FloatRect{ { value[0].get<float>(), value[1].get<float>() }, { value[2].get<float>(), value[3].get<float>() } };
	}
	void SceneSettingsSchema::Register(SceneSettingField field)
	{
		if (field.key.empty()) throw std::invalid_argument("Scene setting key is empty");
		if (field.min && field.max && *field.min > *field.max) throw std::invalid_argument("Invalid scene setting range");
		const auto error = field.Validate(field.defaultValue);
		if (!error.empty()) throw std::invalid_argument(field.key + ": " + error);
		for (auto& existing : mFields)
		{
			if (existing.key == field.key) { existing = std::move(field); ++mRevision; return; }
		}
		mFields.push_back(std::move(field));
		++mRevision;
	}
	const SceneSettingField* SceneSettingsSchema::Find(const std::string& key) const
	{
		for (const auto& field : mFields) if (field.key == key) return &field;
		return nullptr;
	}
	nlohmann::json SceneSettingsSchema::Value(const nlohmann::json& authored, const std::string& key) const
	{
		if (authored.is_object() && authored.contains(key)) return authored.at(key);
		const auto* field = Find(key);
		return field ? field->defaultValue : nlohmann::json{};
	}
	bool SceneSettingsSchema::Set(nlohmann::json& authored, const std::string& key,
		const std::optional<nlohmann::json>& value, std::string& error) const
	{
		error.clear();
		const auto* field = Find(key);
		if (!field) { error = "Unknown setting (read-only)"; return false; }
		if (!authored.is_object()) { error = "Settings must be an object"; return false; }
		if (value && !(error = field->Validate(*value)).empty()) return false;
		// Existing explicit defaults remain untouched unless the author resets them.
		if (value && (!authored.contains(key) && *value == field->defaultValue)) return true;
		if (value) authored[key] = *value;
		else authored.erase(key);
		return true;
	}
}
