#pragma once

#include <nlohmann/json.hpp>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace vega
{
	class EngineContext;
	struct PrefabResult
	{
		nlohmann::json entity;
		std::vector<std::string> warnings;
		explicit operator bool() const { return entity.is_object(); }
	};

	class PrefabLibrary
	{
	public:
		explicit PrefabLibrary(std::filesystem::path root);
		PrefabLibrary(EngineContext& context, std::filesystem::path root);
		// Deprecated: forwards to the current EngineContext's library. Its root
		// follows that context's AssetManager unless explicitly set.
		static PrefabLibrary& Get();
		void SetRoot(std::filesystem::path root);
		const std::filesystem::path& Root() const { return mRoot; }
		void Reload();
		PrefabResult Resolve(const std::string& path);
		// Uncached top-level realizations, for profiling and cache regression tests.
		std::uint64_t ResolveCount() const { return mResolveCount; }
		nlohmann::json Load(const std::string& path, std::vector<std::string>& warnings);
		static nlohmann::json Merge(nlohmann::json base, const nlohmann::json& overrides);
		static nlohmann::json Diff(const nlohmann::json& entity, const nlohmann::json& baseline);
		static nlohmann::json Document(nlohmann::json entity, const std::string& name);
		static void LocalizeReferences(nlohmann::json& entity);
	private:
		EngineContext* mContext;
		// Scene realization can re-enter the public resolver on this same library.
		std::set<std::filesystem::path> mResolveChain, mResolveDependencies;
		PrefabResult Resolve(const std::string& path, std::set<std::filesystem::path>& chain, std::set<std::filesystem::path>& dependencies);
		struct Cached
		{
			std::filesystem::file_time_type modified;
			std::uintmax_t size;
			nlohmann::json document;
		};
		std::filesystem::path mRoot;
		std::map<std::filesystem::path, Cached> mCache;
		struct Resolved
		{
			PrefabResult result;
			std::map<std::filesystem::path, std::pair<std::filesystem::file_time_type, std::uintmax_t>> dependencies;
		};
		std::map<std::filesystem::path, Resolved> mResolved;
		std::uint64_t mResolveCount = 0;
	};
}
