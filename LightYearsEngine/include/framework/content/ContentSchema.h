#pragma once

#include <nlohmann/json.hpp>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace ly::content
{
	using AuthoringJson = nlohmann::ordered_json;
	enum class FieldKind { Number, Boolean, String, Enum, Tag, TagList, Attribute, Reference, Curve, Object, List };
	struct ContentField
	{
		// JSON pointer pattern relative to a definition; * matches one array element.
		std::string path;
		FieldKind kind = FieldKind::String;
		AuthoringJson defaultValue;
		std::string category, tooltip, unit, referenceType;
		std::vector<std::string> options;
		double minimum = -1.e30, maximum = 1.e30;
	};
	struct ContentIssue
	{
		std::string type, id, path, message;
		bool warning = false;
	};
	using ContentDocuments = std::map<std::string, AuthoringJson>;
	struct ContentTypeSchema
	{
		std::string type, file, collection;
		std::vector<ContentField> fields;
		std::map<std::string, AuthoringJson> templates;
		std::function<std::string(const ContentDocuments&, const std::string&)> validate;
		std::function<bool(const std::string&, std::string&)> validateId;
		std::function<AuthoringJson(const AuthoringJson&, const std::string&)> duplicate;
	};
	class ContentSchemaRegistry
	{
	public:
		// Deprecated: use EngineContext::ContentSchemas().
		static ContentSchemaRegistry& Get();
		void Register(ContentTypeSchema schema) { mTypes[schema.type] = std::move(schema); }
		const std::map<std::string, ContentTypeSchema>& Types() const { return mTypes; }
		const ContentTypeSchema* Find(const std::string& type) const
		{
			const auto it = mTypes.find(type);
			return it == mTypes.end() ? nullptr : &it->second;
		}
		// Domains are supplied by the game; neither editor nor engine knows game IDs.
		std::vector<std::string> tags, attributes;
		std::function<bool(const std::string&)> isTag, isAttribute;
	private:
		std::map<std::string, ContentTypeSchema> mTypes;
	};
}
