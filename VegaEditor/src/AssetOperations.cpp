#include "editor/AssetOperations.h"
#include <framework/content/ContentSchema.h>
#include <algorithm>
#include <cctype>
#include <fstream>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#else
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace vega
{
	namespace
	{
		struct OperationLock
		{
#ifdef _WIN32
			HANDLE file;
			explicit OperationLock(const std::filesystem::path& path) : file{ CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr) } {}
			bool Valid() const { return file != INVALID_HANDLE_VALUE; }
			~OperationLock() { if (Valid()) CloseHandle(file); }
#else
			int file;
			explicit OperationLock(const std::filesystem::path& path) : file{ open(path.c_str(), O_CREAT | O_RDWR, 0600) }
			{
				if (file >= 0 && flock(file, LOCK_EX | LOCK_NB) != 0) { close(file); file = -1; }
			}
			bool Valid() const { return file >= 0; }
			~OperationLock() { if (Valid()) close(file); }
#endif
		};
		std::string Read(const std::filesystem::path& path)
		{
			std::ifstream input{ path, std::ios::binary };
			if (!input) throw std::runtime_error("Cannot read " + path.generic_string());
			return { std::istreambuf_iterator<char>{ input }, {} };
		}
		std::string Key(const std::filesystem::path& path)
		{
			auto key = path.lexically_normal().generic_string();
#ifdef _WIN32
			std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
#endif
			return key;
		}
		bool AssetPath(const std::filesystem::path& path)
		{
			if (path.empty() || path.is_absolute() || path.has_root_name() || path.has_root_directory()) return false;
			for (const auto& part : path) if (part == ".." || Key(part) == ".vega") return false;
			return path.lexically_normal() != ".";
		}
		bool NoLinks(const std::filesystem::path& root, const std::filesystem::path& path)
		{
			auto current = root;
			for (const auto& part : path.lexically_relative(root))
			{
				current /= part;
				if (std::filesystem::is_symlink(std::filesystem::symlink_status(current))) return false;
			}
			return true;
		}
	}

	bool AssetMovePlan::Contains(const std::filesystem::path& parent, const std::filesystem::path& path)
	{
		const auto prefix = Key(parent), candidate = Key(path);
		return candidate == prefix || (candidate.size() > prefix.size() && candidate.compare(0, prefix.size(), prefix) == 0 && candidate[prefix.size()] == '/');
	}

	std::filesystem::path AssetMovePlan::Remap(const std::filesystem::path& path, const std::filesystem::path& from, const std::filesystem::path& to)
	{
		if (!Contains(from, path)) return path;
		const auto suffix = path.generic_string().substr(from.generic_string().size());
		return std::filesystem::u8path(to.generic_string() + suffix);
	}

	bool AssetMovePlan::Prepare(const std::filesystem::path& root, const std::filesystem::path& from,
		const std::filesystem::path& to, const std::filesystem::path& project)
	{
		mReady = false;
		mDocuments.clear(); mReferences.clear(); mError.clear();
		try
		{
			if (!AssetPath(from) || !AssetPath(to)) throw std::runtime_error("Asset paths must stay inside the asset root and outside .vega");
			JsonDocument resolver{ root };
			mRoot = resolver.Root();
			mFrom = from.lexically_normal(); mTo = to.lexically_normal();
			for (const auto& [type, schema] : ly::content::ContentSchemaRegistry::Get().Types())
				if (Contains(mFrom, std::filesystem::u8path(schema.file)))
					throw std::runtime_error("The game module owns the fixed catalog path " + schema.file + "; move its references instead");
			std::filesystem::path source, target;
			if (!resolver.Resolve(mFrom, source) || !resolver.Resolve(mTo, target) || !NoLinks(mRoot, mRoot / mFrom) || !NoLinks(mRoot, mRoot / mTo))
				throw std::runtime_error("Linked or escaping asset paths cannot be moved");
			if (!std::filesystem::exists(source)) throw std::runtime_error("Source asset does not exist");
			if (std::filesystem::exists(target)) throw std::runtime_error("Destination already exists (case-only renames are not supported)");
			if (!std::filesystem::is_directory(target.parent_path())) throw std::runtime_error("Destination folder does not exist");
			if (Contains(mFrom, mTo)) throw std::runtime_error("Cannot move a folder into itself");
			std::vector<std::filesystem::path> files;
			for (auto it = std::filesystem::recursive_directory_iterator{ mRoot }; it != std::filesystem::recursive_directory_iterator{}; ++it)
			{
				if (Key(it->path().filename()) == ".vega") { if (it->is_directory()) it.disable_recursion_pending(); continue; }
				if (it->is_symlink()) throw std::runtime_error("Remove linked assets before reference-safe file operations");
				if (it->is_regular_file() && Key(it->path().extension()) == ".json") files.push_back(it->path());
			}
			const auto projectPath = project.empty() ? std::filesystem::path{} : std::filesystem::weakly_canonical(project);
			if (!projectPath.empty() && !Contains(mRoot, projectPath) && std::filesystem::exists(projectPath)) files.push_back(projectPath);
			std::sort(files.begin(), files.end());
			for (const auto& file : files)
			{
				const bool external = !Contains(mRoot, file);
				JsonDocument document{ external ? file.parent_path() : mRoot };
				if (!document.Open(external ? file.filename() : file.lexically_relative(mRoot)) || document.ReadOnly())
					throw std::runtime_error("Cannot safely inspect " + file.generic_string() + ": " + document.Error() + document.ReadOnlyReason());
				auto after = document.Data();
				const auto start = mReferences.size();
				const auto visit = [&](auto&& self, Json& value, const std::string& pointer) -> void
				{
					if (value.is_string())
					{
						const auto before = value.get<std::string>();
						auto spelling = before;
						std::replace(spelling.begin(), spelling.end(), '\\', '/');
						const auto path = std::filesystem::u8path(spelling);
						std::filesystem::path changed = path;
						if (external && pointer != "/startupScene")
						{
							// Editor path settings resolve against the project file, not the asset root.
							if (pointer == "/editor/font" || pointer == "/editor/visualAssets")
							{
								const auto absolute = std::filesystem::weakly_canonical(file.parent_path() / path);
								if (Contains(mRoot / mFrom, absolute)) changed = Remap(absolute, mRoot / mFrom, mRoot / mTo).lexically_relative(file.parent_path());
							}
						}
						else if (AssetPath(path) && Contains(mFrom, path.lexically_normal())) changed = Remap(path.lexically_normal(), mFrom, mTo);
						if (changed != path)
						{
							value = changed.generic_string();
							mReferences.push_back({ file, pointer, before, value.get<std::string>() });
						}
					}
					else if (value.is_structured()) for (auto it = value.begin(); it != value.end(); ++it)
						self(self, it.value(), pointer + '/' + (value.is_array() ? std::to_string(it - value.begin()) : JsonDocument::EscapePointer(it.key())));
				};
				visit(visit, after, "");
				const auto before = document.Serialize();
				if (mReferences.size() != start)
				{
					document.Replace(after);
				}
				mDocuments.push_back({ file, Remap(file, mRoot / mFrom, mRoot / mTo), before, document.Serialize() });
			}
			mReady = true;
			return true;
		}
		catch (const std::exception& error) { mError = error.what(); return false; }
	}

	bool AssetMovePlan::Current(bool forward) const
	{
		if (!mReady) return false;
		try
		{
			JsonDocument resolver{ mRoot };
			if (resolver.Root() != mRoot) return false;
			std::filesystem::path source, target;
			if (!resolver.Resolve(forward ? mFrom : mTo, source) || !resolver.Resolve(forward ? mTo : mFrom, target)) return false;
			if (!NoLinks(mRoot, mRoot / mFrom) || !NoLinks(mRoot, mRoot / mTo) || !std::filesystem::exists(source) || std::filesystem::exists(target)) return false;
			for (const auto& document : mDocuments)
			{
				const auto& path = forward ? document.beforePath : document.afterPath;
				if (std::filesystem::weakly_canonical(path) != path || Read(path) != (forward ? document.before : document.after)) return false;
			}
			std::size_t count = 0;
			for (auto it = std::filesystem::recursive_directory_iterator{ mRoot }; it != std::filesystem::recursive_directory_iterator{}; ++it)
			{
				if (Key(it->path().filename()) == ".vega") { if (it->is_directory()) it.disable_recursion_pending(); continue; }
				if (it->is_symlink()) return false;
				if (it->is_regular_file() && Key(it->path().extension()) == ".json") ++count;
			}
			const auto internal = std::count_if(mDocuments.begin(), mDocuments.end(), [&](const auto& document) { return Contains(mRoot, document.beforePath); });
			if (count != static_cast<std::size_t>(internal)) return false;
			return true;
		}
		catch (const std::exception&) { return false; }
	}

	bool AssetMovePlan::Apply(bool forward)
	{
		if (!Current(forward)) { mError = "Assets changed since preview; build a new move preview"; return false; }
		const auto source = mRoot / (forward ? mFrom : mTo), target = mRoot / (forward ? mTo : mFrom);
		JsonDocument resolver{ mRoot };
		std::filesystem::path directory;
		if (!resolver.Resolve(".vega/AssetTransactions", directory)) { mError = "Asset transaction directory escapes the asset root"; return false; }
		std::error_code directoryError;
		std::filesystem::create_directories(directory, directoryError);
		if (directoryError) { mError = directoryError.message(); return false; }
		OperationLock lock{ directory / "operation.lock" };
		if (!lock.Valid()) { mError = "Another editor is changing assets"; return false; }
		const auto journalPath = directory / "pending.json";
		if (std::filesystem::exists(journalPath)) { mError = "An interrupted asset move needs recovery; restart the editor"; return false; }
		Json journal{ { "format", "vega.asset-transaction" }, { "version", 1 }, { "from", (forward ? mFrom : mTo).generic_string() },
			{ "to", (forward ? mTo : mFrom).generic_string() }, { "committed", false }, { "documents", Json::array() } };
		for (const auto& document : mDocuments)
		{
			journal["documents"].push_back({ { "beforePath", (forward ? document.beforePath : document.afterPath).generic_string() },
				{ "afterPath", (forward ? document.afterPath : document.beforePath).generic_string() },
				{ "before", forward ? document.before : document.after }, { "after", forward ? document.after : document.before } });
		}
		if (!AtomicWriteText(journalPath, journal.dump(1, '\t') + "\n", mError)) return false;
		std::size_t written = 0;
		try
		{
			std::filesystem::rename(source, target);
			for (const auto& document : mDocuments)
			{
				if (document.before == document.after) { ++written; continue; }
				const auto& path = forward ? document.afterPath : document.beforePath;
				const auto& before = forward ? document.before : document.after;
				if (Read(path) != before) throw std::runtime_error("Reference file changed during move: " + path.generic_string());
				if (!AtomicWriteText(path, forward ? document.after : document.before, mError)) throw std::runtime_error(mError);
				++written;
			}
			journal["committed"] = true;
			if (!AtomicWriteText(journalPath, journal.dump(1, '\t') + "\n", mError)) throw std::runtime_error(mError);
			std::error_code cleanup;
			std::filesystem::remove(journalPath, cleanup);
			mError.clear();
			return true;
		}
		catch (const std::exception& error)
		{
			mError = error.what();
			// Roll back only bytes still owned by this operation. Never replace a new
			// external edit while trying to undo a failed filesystem operation.
			try
			{
				while (written)
				{
					const auto& document = mDocuments[--written];
					if (document.before == document.after) continue;
					const auto& path = forward ? document.afterPath : document.beforePath;
					std::string rollbackError;
					if (Read(path) != (forward ? document.after : document.before) ||
						!AtomicWriteText(path, forward ? document.before : document.after, rollbackError))
						throw std::runtime_error("Reference rollback blocked: " + path.generic_string());
				}
				if (std::filesystem::exists(target) && !std::filesystem::exists(source)) std::filesystem::rename(target, source);
				std::filesystem::remove(journalPath);
			}
			catch (const std::exception& rollback) { mError += "; " + std::string{ rollback.what() }; }
			return false;
		}
	}

	bool AssetMovePlan::RecoverPending(const std::filesystem::path& root, const std::filesystem::path& project, std::string& error)
	{
		error.clear();
		try
		{
			JsonDocument resolver{ root };
			std::filesystem::path directory;
			if (!resolver.Resolve(".vega/AssetTransactions", directory)) throw std::runtime_error("Asset transaction directory escapes the asset root");
			const auto journalPath = directory / "pending.json";
			if (!std::filesystem::exists(journalPath)) return true;
			OperationLock lock{ directory / "operation.lock" };
			if (!lock.Valid()) return true; // A live editor owns this transaction.
			if (std::filesystem::is_symlink(journalPath)) throw std::runtime_error("Linked asset journal refused");
			const auto journal = Json::parse(Read(journalPath));
			if (journal.at("format") != "vega.asset-transaction" || journal.at("version") != 1) throw std::runtime_error("Invalid asset transaction journal");
			if (journal.at("committed") == true) { std::filesystem::remove(journalPath); return true; }
			const auto from = std::filesystem::u8path(journal.at("from").get<std::string>()), to = std::filesystem::u8path(journal.at("to").get<std::string>());
			std::filesystem::path source, target;
			if (!AssetPath(from) || !AssetPath(to) || !resolver.Resolve(from, source) || !resolver.Resolve(to, target) ||
				!NoLinks(resolver.Root(), resolver.Root() / from) || !NoLinks(resolver.Root(), resolver.Root() / to)) throw std::runtime_error("Unsafe asset transaction paths");
			const bool moved = std::filesystem::exists(target) && !std::filesystem::exists(source);
			if (!moved && (!std::filesystem::exists(source) || std::filesystem::exists(target))) throw std::runtime_error("Asset recovery conflicts with current files");
			const auto selectedProject = project.empty() ? std::filesystem::path{} : std::filesystem::weakly_canonical(project);
			std::vector<std::pair<std::filesystem::path, std::string>> restores;
			for (const auto& document : journal.at("documents"))
			{
				const auto beforePath = std::filesystem::u8path(document.at("beforePath").get<std::string>());
				const auto afterPath = std::filesystem::u8path(document.at("afterPath").get<std::string>());
				const bool external = !Contains(resolver.Root(), beforePath);
				if ((external && beforePath != selectedProject) || afterPath != Remap(beforePath, source, target)) throw std::runtime_error("Unsafe asset reference journal path");
				const auto path = moved ? afterPath : beforePath;
				if (std::filesystem::weakly_canonical(path) != path || (!external && !NoLinks(resolver.Root(), path))) throw std::runtime_error("Asset recovery path changed");
				const auto current = Read(path), before = document.at("before").get<std::string>(), after = document.at("after").get<std::string>();
				if (current != before && current != after) throw std::runtime_error("Asset recovery preserves an external edit in " + path.generic_string());
				if (current != before) restores.emplace_back(path, before);
			}
			for (const auto& [path, text] : restores) if (!AtomicWriteText(path, text, error)) throw std::runtime_error(error);
			if (moved) std::filesystem::rename(target, source);
			std::filesystem::remove(journalPath);
			return true;
		}
		catch (const std::exception& exception) { error = "Interrupted asset move: " + std::string{ exception.what() }; return false; }
	}
}
