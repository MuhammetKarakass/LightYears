#pragma once

#include "editor/JsonDocument.h"
#include "editor/JsonCommands.h"
#include <visual/DeepSky.h>
#include <chrono>

namespace vega
{
	// Owns one deepsky.json. Selection does not alter the document or scene.
	class SkyDocument
	{
	public:
		explicit SkyDocument(const std::filesystem::path& root);
		bool Open();
		bool Reopen();
		bool Select(const std::string& id, bool preset = false);
		bool Set(const std::string& path, const Json& value, std::uint64_t group = 0);
		bool Reset(const std::string& path);
		bool Block(const std::string& path, const Json& value, const std::string& name = "Paste sky block");
		bool RemoveBlock(const std::string& path);
		bool DuplicateTo(const std::string& stop);
		bool StorePreset(const std::string& name);
		bool Save();
		bool Refresh();
		Json Effective() const;
		Json Value(const std::string& path) const;
		const Json* Authored(const std::string& path = "") const;
		const ly::visual::DeepSkyCatalog& Parsed() const { return mParsed; }
		const ly::visual::DeepSkyCatalog& SavedParsed() const { return mSavedParsed; }
		const Json& Saved() const { return mSaved; }
		JsonDocument document;
		EditorSelection selection;
		EditorContext context{nullptr, selection};
		CommandHistory history{context};
		std::string selected, error;
		bool preset = false;
		double parseMs = 0;
		static std::string Normalize(const std::string& path);
		static bool IsHandle(const std::string& handle);
	private:
		bool Commit(Json candidate, const std::string& name, std::uint64_t group = 0);
		std::string Base() const;
		Json& Editable(Json& root) const;
		ly::visual::DeepSkyCatalog mParsed, mSavedParsed;
		Json mSaved;
		std::uint64_t mParsedRevision = 0;
	};
}
