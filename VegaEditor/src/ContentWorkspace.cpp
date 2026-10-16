#include "editor/ContentWorkspace.h"
#include "editor/ContentPath.h"
#include <algorithm>
#include <cctype>
#include <set>
#include <cmath>

namespace vega
{
	namespace
	{
		using namespace ly::content;
		Json* Find(ContentDocuments& documents, const std::string& type, const std::string& id)
		{
			const auto* schema = ContentSchemaRegistry::Get().Find(type);
			if (!schema || !documents.count(type)) return nullptr;
			for (auto& item : documents.at(type).at(schema->collection))
				if (item.value("id", std::string{}) == id) return &item;
			return nullptr;
		}

		class ContentEdit final : public EditorCommand
		{
		public:
			ContentEdit(ContentWorkspace& workspace, ContentDocuments before, ContentDocuments after, std::string name, std::uint64_t group, std::string afterType, std::string afterId)
				: mWorkspace{ workspace }, mBefore{ std::move(before) }, mAfter{ std::move(after) }, mName{ std::move(name) }, mGroup{ group }, mGeneration{ workspace.Generation() },
				mBeforeType{ workspace.selectedType }, mBeforeId{ workspace.selectedId }, mAfterType{ std::move(afterType) }, mAfterId{ std::move(afterId) } {}
			bool Execute(EditorContext&) override
			{
				if (!mWorkspace.Apply(mAfter)) return false;
				mWorkspace.selectedType = mAfterType; mWorkspace.selectedId = mAfterId;
				return true;
			}
			void Undo(EditorContext&) override
			{
				mWorkspace.Apply(mBefore);
				mWorkspace.selectedType = mBeforeType; mWorkspace.selectedId = mBeforeId;
			}
			std::string Name() const override { return mName; }
			bool ChangesScene() const override { return false; }
			bool IsValid(const EditorContext&) const override { return mGeneration == mWorkspace.Generation(); }
			bool MergeWith(const EditorCommand& command) override
			{
				const auto* next = dynamic_cast<const ContentEdit*>(&command);
				if (!next || !mGroup || next->mGroup != mGroup || &next->mWorkspace != &mWorkspace || mName != next->mName) return false;
				mAfter = next->mAfter;
				return true;
			}
		private:
			ContentWorkspace& mWorkspace;
			ContentDocuments mBefore, mAfter;
			std::string mName;
			std::uint64_t mGroup, mGeneration;
			std::string mBeforeType, mBeforeId, mAfterType, mAfterId;
		};
	}

	std::string ContentWorkspace::Pointer(const std::string& fieldPath)
	{
		return ContentPointer(fieldPath);
	}

	bool ContentWorkspace::Load(const std::filesystem::path& root)
	{
		if (Dirty()) { error = "Save content changes before reloading"; return false; }
		std::map<std::string, JsonDocument> documents;
		for (const auto& [type, schema] : ContentSchemaRegistry::Get().Types())
		{
			JsonDocument document{ root };
			std::filesystem::path path;
			if (document.Resolve(schema.file, path) && !std::filesystem::exists(path)) continue;
			if (!document.Open(schema.file) || document.ReadOnly()) { error = type + ": " + document.Error() + document.ReadOnlyReason(); return false; }
			if (!document.Data().contains(schema.collection) || !document.Data().at(schema.collection).is_array()) { error = "Missing collection " + schema.collection; return false; }
			documents.emplace(type, std::move(document));
		}
		if (documents.empty()) { error = "No registered content files in asset root"; return false; }
		mDocuments = std::move(documents);
		if (!mDocuments.count(selectedType)) { selectedType = mDocuments.begin()->first; selectedId.clear(); }
		++mGeneration;
		++mRevision;
		error.clear();
		return true;
	}

	bool ContentWorkspace::Dirty() const
	{
		for (const auto& [type, document] : mDocuments) if (document.Dirty()) return true;
		return false;
	}

	bool ContentWorkspace::Open(const std::string& type, const std::string& id)
	{
		if (!Definition(type, id)) { error = "Unknown content " + type + " " + id; return false; }
		selectedType = type;
		selectedId = id;
		error.clear();
		return true;
	}

	const Json* ContentWorkspace::Definition(const std::string& type, const std::string& id) const
	{
		const auto document = mDocuments.find(type);
		const auto* schema = ContentSchemaRegistry::Get().Find(type);
		if (document == mDocuments.end() || !schema) return nullptr;
		for (const auto& item : document->second.Data().at(schema->collection))
			if (item.value("id", std::string{}) == id) return &item;
		return nullptr;
	}

	const ContentField* ContentWorkspace::Field(const std::string& type, const std::string& pattern) const
	{
		const auto* schema = ContentSchemaRegistry::Get().Find(type);
		if (schema) for (const auto& field : schema->fields) if (field.path == pattern) return &field;
		return nullptr;
	}

	ContentDocuments ContentWorkspace::Snapshot() const
	{
		ContentDocuments result;
		for (const auto& [type, document] : mDocuments) result.emplace(type, document.Data());
		return result;
	}

	bool ContentWorkspace::Apply(const ContentDocuments& documents)
	{
		for (const auto& [type, value] : documents) if (!mDocuments.count(type) || mDocuments.at(type).ReadOnly()) return false;
		for (const auto& [type, value] : documents) mDocuments.at(type).Replace(value);
		++mRevision;
		error.clear();
		return true;
	}

	bool ContentWorkspace::Commit(CommandHistory& history, ContentDocuments after, const std::string& name, std::uint64_t group, const std::string& afterType, const std::string& afterId)
	{
		auto before = Snapshot();
		if (before == after) return true;
		return history.Execute(std::make_unique<ContentEdit>(*this, std::move(before), std::move(after), name, group,
			afterType.empty() ? selectedType : afterType, afterType.empty() ? selectedId : afterId));
	}

	bool ContentWorkspace::Set(CommandHistory& history, const std::string& fieldPath, const Json& value, std::uint64_t group)
	{
		try
		{
			const auto pointer = Pointer(fieldPath);
			if (pointer == "/id") throw std::runtime_error("Use Rename ID to update references");
			auto documents = Snapshot();
			auto* item = Find(documents, selectedType, selectedId);
			if (!item) throw std::runtime_error("Open a definition first");
			const Json* cursor = item;
			std::string pattern;
			// Build a schema path by following the actual JSON containers, so numeric
			// object keys never accidentally become array wildcards.
			std::size_t start = 1;
			while (start <= pointer.size())
			{
				const auto end = pointer.find('/', start);
				const auto part = pointer.substr(start, end - start);
				const bool index = !part.empty() && std::all_of(part.begin(), part.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
				pattern += '/' + ((cursor && cursor->is_array()) || (!cursor && index) ? std::string{"*"} : part);
				if (cursor)
				{
					try { cursor = &cursor->at(Json::json_pointer{ "/" + part }); } catch (...) { cursor = nullptr; }
				}
				if (end == std::string::npos) break;
				start = end + 1;
			}
			const auto* field = Field(selectedType, pattern);
			if (!field) throw std::runtime_error("Unknown fields are preserved read-only under Other");
			if (value.is_number_float() && !std::isfinite(value.get<double>())) throw std::runtime_error("Number must be finite");
			if (field->kind == FieldKind::Number && !value.is_number()) throw std::runtime_error("Expected a number");
			if (field->kind == FieldKind::Boolean && !value.is_boolean()) throw std::runtime_error("Expected a boolean");
			if (field->kind == FieldKind::Object && !value.is_object()) throw std::runtime_error("Expected an object");
			if ((field->kind == FieldKind::List || field->kind == FieldKind::TagList) && !value.is_array()) throw std::runtime_error("Expected a list");
			if (field->kind == FieldKind::Curve && !value.is_array() && !value.is_number()) throw std::runtime_error("Expected a number or per-level list");
			if ((field->kind == FieldKind::String || field->kind == FieldKind::Enum || field->kind == FieldKind::Tag || field->kind == FieldKind::Attribute || field->kind == FieldKind::Reference) && !value.is_string()) throw std::runtime_error("Expected a string");
			(*item)[Json::json_pointer{ pointer }] = value;
			return Commit(history, std::move(documents), "Set content " + selectedId + pointer, group);
		}
		catch (const std::exception& exception) { error = exception.what(); return false; }
	}

	bool ContentWorkspace::New(CommandHistory& history, const std::string& type, const std::string& source, const std::string& id)
	{
		const auto* schema = ContentSchemaRegistry::Get().Find(type);
		if (!schema || !mDocuments.count(type)) { error = "Unknown content type"; return false; }
		if (Definition(type, id)) { error = "ID already exists"; return false; }
		if (schema->validateId && !schema->validateId(id, error)) return false;
		Json value;
		if (const auto* existing = Definition(type, source)) value = *existing;
		else if (schema->templates.count(source)) value = schema->templates.at(source);
		else { error = "Unknown template or source ID"; return false; }
		if (schema->duplicate) value = schema->duplicate(value, id);
		else value["id"] = id;
		auto after = Snapshot();
		after.at(type).at(schema->collection).push_back(std::move(value));
		if (!Commit(history, std::move(after), "New " + type, 0, type, id)) return false;
		return Open(type, id);
	}

	bool ContentWorkspace::Rename(CommandHistory& history, const std::string& type, const std::string& oldId, const std::string& newId)
	{
		const auto* schema = ContentSchemaRegistry::Get().Find(type);
		if (!schema || !Definition(type, oldId)) { error = "Unknown definition"; return false; }
		if (Definition(type, newId)) { error = "ID already exists"; return false; }
		if (schema->validateId && !schema->validateId(newId, error)) return false;
		auto after = Snapshot();
		for (const auto& ref : References())
			if (ref.targetType == type && ref.targetId == oldId) (*Find(after, ref.type, ref.id))[Json::json_pointer{ ref.path }] = newId;
		Find(after, type, oldId)->at("id") = newId;
		if (schema->validate)
		{
			const auto failure = schema->validate(after, newId);
			if (!failure.empty())
			{
				error = failure;
				return false;
			}
		}
		if (!Commit(history, std::move(after), "Rename " + type + " and references", 0, type, newId)) return false;
		return Open(type, newId);
	}

	bool ContentWorkspace::Delete(CommandHistory& history, const std::string& type, const std::string& id)
	{
		const auto* schema = ContentSchemaRegistry::Get().Find(type);
		if (!schema || !Definition(type, id)) { error = "Unknown definition"; return false; }
		for (const auto& ref : References()) if (ref.targetType == type && ref.targetId == id)
		{ error = "Still referenced by " + ref.type + " " + ref.id + " " + ref.path; return false; }
		auto after = Snapshot();
		auto& items = after.at(type).at(schema->collection);
		for (auto it = items.begin(); it != items.end(); ++it) if (it->value("id", std::string{}) == id) { items.erase(it); break; }
		return Commit(history, std::move(after), "Delete " + type, 0, selectedType, selectedType == type && selectedId == id ? std::string{} : selectedId);
	}

	void ContentWorkspace::Rebuild()
	{
		if (mIndexRevision == mRevision) return;
		mIndexRevision = mRevision;
		mEntries.clear(); mReferences.clear();
		const auto& registry = ContentSchemaRegistry::Get();
		std::set<std::string> tags(registry.tags.begin(), registry.tags.end()), attributes(registry.attributes.begin(), registry.attributes.end());
		for (const auto& [type, document] : mDocuments)
		{
			const auto& schema = *registry.Find(type);
			for (const auto& item : document.Data().at(schema.collection))
			{
				const auto id = item.value("id", std::string{});
				ContentEntry entry{ type, id, id + " " + item.value("displayName", std::string{}), item.value("category", item.value("family", std::string{})), 0 };
				std::function<void(const Json&, const std::string&, const std::string&)> visit;
				visit = [&](const Json& value, const std::string& path, const std::string& pattern)
				{
					const auto* field = Field(type, pattern);
					if (field && value.is_string())
					{
						const auto text = value.get<std::string>();
						if (field->kind == FieldKind::Tag && registry.isTag && registry.isTag(text)) { tags.insert(text); entry.searchText += " " + text; }
						if (field->kind == FieldKind::Attribute && registry.isAttribute && registry.isAttribute(text)) attributes.insert(text);
						if (field->kind == FieldKind::Reference && !text.empty()) mReferences.push_back({ type, id, path, field->referenceType, text });
					}
					if (value.is_structured()) for (auto it = value.begin(); it != value.end(); ++it)
					{
						const auto key = value.is_array() ? std::to_string(it - value.begin()) : JsonDocument::EscapePointer(it.key());
						visit(it.value(), path + '/' + key, pattern + '/' + (value.is_array() ? "*" : key));
					}
				};
				visit(item, "", "");
				entry.searchText += " " + entry.category;
				mEntries.push_back(std::move(entry));
			}
		}
		for (auto& entry : mEntries)
			for (const auto& ref : mReferences) if (ref.targetType == entry.type && ref.targetId == entry.id) ++entry.usedBy;
		mTags.assign(tags.begin(), tags.end()); mAttributes.assign(attributes.begin(), attributes.end());
	}

	const std::vector<ContentEntry>& ContentWorkspace::Entries() { Rebuild(); return mEntries; }
	const std::vector<ContentReference>& ContentWorkspace::References() { Rebuild(); return mReferences; }

	std::vector<std::string> ContentWorkspace::Options(const ContentField& field) const
	{
		if (field.kind == FieldKind::Tag) return mTags;
		if (field.kind == FieldKind::Attribute) return mAttributes;
		if (field.kind == FieldKind::Reference)
		{
			std::vector<std::string> result;
			for (const auto& entry : mEntries) if (entry.type == field.referenceType) result.push_back(entry.id);
			return result;
		}
		return field.options;
	}

	const std::vector<ContentIssue>& ContentWorkspace::Validate()
	{
		Rebuild();
		if (mValidationRevision == mRevision) return mIssues;
		mValidationRevision = mRevision;
		mIssues.clear();
		const auto documents = Snapshot();
		std::set<std::pair<std::string, std::string>> ids;
		for (const auto& entry : mEntries)
		{
			if (!ids.emplace(entry.type, entry.id).second) mIssues.push_back({ entry.type, entry.id, "/id", "Duplicate content ID", false });
			const auto* schema = ContentSchemaRegistry::Get().Find(entry.type);
			if (schema->validate)
			{
				try
				{
					const auto message = schema->validate(documents, entry.id);
					if (!message.empty()) mIssues.push_back({ entry.type, entry.id, "", message, false });
				}
				catch (const std::exception& exception) { mIssues.push_back({ entry.type, entry.id, "", exception.what(), false }); }
			}
		}
		for (const auto& ref : mReferences)
			if (!Definition(ref.targetType, ref.targetId)) mIssues.push_back({ ref.type, ref.id, ref.path, "Missing " + ref.targetType + ": " + ref.targetId, false });
		return mIssues;
	}

	bool ContentWorkspace::Save()
	{
		for (const auto& issue : Validate()) if (!issue.warning) { error = issue.id + ": " + issue.message; return false; }
		for (auto& [type, document] : mDocuments) if (document.Dirty() && !document.Save()) { error = type + ": " + document.Error(); return false; }
		error.clear();
		return true;
	}

	bool ContentWorkspace::Revert()
	{
		for (auto& [type, document] : mDocuments) if (!document.Revert()) { error = type + ": " + document.Error(); return false; }
		++mGeneration; ++mRevision;
		error.clear();
		return true;
	}
}
