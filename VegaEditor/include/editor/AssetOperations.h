#pragma once

#include "editor/JsonDocument.h"

namespace vega
{
	struct AssetReferenceEdit
	{
		std::filesystem::path file;
		std::string pointer, before, after;
	};

	// A reviewed plan owns the exact bytes it read. Apply/undo refuse stale inputs,
	// use same-filesystem rename and atomic JSON writes, and roll back write failures.
	class AssetMovePlan
	{
	public:
		bool Prepare(const std::filesystem::path& root, const std::filesystem::path& from,
			const std::filesystem::path& to, const std::filesystem::path& project = {});
		bool Apply(bool forward = true);
		bool Current(bool forward) const;
		static bool RecoverPending(const std::filesystem::path& root, const std::filesystem::path& project, std::string& error);
		const std::vector<AssetReferenceEdit>& References() const { return mReferences; }
		const std::string& Error() const { return mError; }
		const std::filesystem::path& From() const { return mFrom; }
		const std::filesystem::path& Root() const { return mRoot; }
		const std::filesystem::path& To() const { return mTo; }
		static bool Contains(const std::filesystem::path& parent, const std::filesystem::path& path);
		static std::filesystem::path Remap(const std::filesystem::path& path,
			const std::filesystem::path& from, const std::filesystem::path& to);
	private:
		struct Document
		{
			std::filesystem::path beforePath, afterPath;
			std::string before, after;
		};
		std::filesystem::path mRoot, mFrom, mTo;
		std::vector<Document> mDocuments;
		std::vector<AssetReferenceEdit> mReferences;
		std::string mError;
		bool mReady = false;
	};
}
