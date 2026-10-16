#pragma once

#include <nlohmann/json.hpp>
#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <cstdint>
#include <SFML/Graphics/Rect.hpp>

namespace ly
{
	class SceneSettingsSchema;
	enum class SceneSettingType { Bool, Int, Float, String, Enum, CatalogId, Vec2, Color, Rect };
	// Optional world-space [x, y, width, height]. Invalid authored values are
	// preserved by serialization but have no runtime bounds.
	std::optional<sf::FloatRect> ReadSceneBounds(const nlohmann::json& settings);
	std::optional<sf::FloatRect> ReadSceneBounds(const nlohmann::json& settings, const SceneSettingsSchema& schema);
	struct SceneSettingOption
	{
		nlohmann::json value;
		std::string label;
	};
	struct SceneSettingField
	{
		std::string key, label, category;
		SceneSettingType type = SceneSettingType::String;
		nlohmann::json defaultValue;
		std::optional<double> min, max;
		std::string tooltip;
		std::function<std::vector<SceneSettingOption>()> options;
		std::string Validate(const nlohmann::json& value) const;
	};

	// Descriptors belong to the game. Authored JSON stays sparse and retains
	// unknown keys so tools can open scenes from newer versions without data loss.
	class SceneSettingsSchema
	{
	public:
		SceneSettingsSchema() = default;
		// Deprecated: use EngineContext::SceneSettings().
		static SceneSettingsSchema& Get();
		void Register(SceneSettingField field);
		const SceneSettingField* Find(const std::string& key) const;
		const std::vector<SceneSettingField>& Fields() const { return mFields; }
		std::uint64_t Revision() const { return mRevision; }
		nlohmann::json Value(const nlohmann::json& authored, const std::string& key) const;
		bool Set(nlohmann::json& authored, const std::string& key,
			const std::optional<nlohmann::json>& value, std::string& error) const;
	private:
		friend class EngineContext;
		// Standalone schemas remain empty; only an application installs core fields.
		void RegisterEngineSettings();
		std::vector<SceneSettingField> mFields;
		std::uint64_t mRevision = 0;
	};
}
