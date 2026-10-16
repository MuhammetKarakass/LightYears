#pragma once
#include "editor/JsonDocument.h"
#include "editor/EditorCommands.h"
#include <framework/content/ContentSchema.h>
#include <map>

namespace vega
{
	struct ContentReference
	{
		std::string type, id, path, targetType, targetId;
	};
	struct ContentEntry
	{
		std::string type, id, searchText, category;
		std::size_t usedBy = 0;
	};
	class ContentWorkspace
	{
	public:
		bool Load(const std::filesystem::path& root);
		bool Loaded() const { return !mDocuments.empty(); }
		bool Dirty() const;
		bool Save();
		bool Revert();
		std::map<std::filesystem::path, std::string> DirtyDocuments() const;
		bool Recover(const std::filesystem::path& path, const std::string& text);
		bool Open(const std::string& type, const std::string& id);
		bool Set(CommandHistory&, const std::string& fieldPath, const Json&, std::uint64_t group = 0);
		bool New(CommandHistory&, const std::string& type, const std::string& source, const std::string& id);
		bool Rename(CommandHistory&, const std::string& type, const std::string& oldId, const std::string& newId);
		bool Delete(CommandHistory&, const std::string& type, const std::string& id);
		const Json* Definition(const std::string& type, const std::string& id) const;
		const Json* Selected() const { return Definition(selectedType, selectedId); }
		const ly::content::ContentField* Field(const std::string& type, const std::string& pattern) const;
		static std::string Pointer(const std::string& fieldPath);
		const std::vector<ContentEntry>& Entries();
		const std::vector<ContentReference>& References();
		const std::vector<ly::content::ContentIssue>& Validate();
		std::vector<std::string> Options(const ly::content::ContentField&) const;
		ly::content::ContentDocuments Snapshot() const;
		bool Apply(const ly::content::ContentDocuments&);
		std::uint64_t Generation() const { return mGeneration; }
		std::uint64_t Revision() const { return mRevision; }
		std::string selectedType, selectedId, error;
	private:
		void Rebuild();
		bool Commit(CommandHistory&, ly::content::ContentDocuments after, const std::string& name, std::uint64_t group = 0,
			const std::string& afterType = {}, const std::string& afterId = {});
		std::map<std::string, JsonDocument> mDocuments;
		std::vector<ContentEntry> mEntries;
		std::vector<ContentReference> mReferences;
		std::vector<ly::content::ContentIssue> mIssues;
		std::vector<std::string> mTags, mAttributes;
		std::uint64_t mGeneration = 0, mRevision = 0, mIndexRevision = ~0ull, mValidationRevision = ~0ull;
	};
}
