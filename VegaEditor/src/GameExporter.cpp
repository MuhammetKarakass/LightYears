#include "editor/GameExporter.h"
#include <framework/ProjectSettings.h>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;

namespace vega
{
	std::future<GameExportResult> ExportGameAsync(GameExportLayout layout, GameExportRequest request)
	{
		// No references to editor members: shutdown may wait for this independent job.
		return std::async(std::launch::async, [layout = std::move(layout), request = std::move(request)]()
		{
			return ExportGame(layout, request);
		});
	}

	namespace
	{
		// Windows folder names are case-insensitive, so "Build" and "build" must compare as the same folder.
		std::string ComponentKey(const fs::path& component)
		{
			std::string text = component.u8string();
			std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return text;
		}

		fs::path Canonical(const fs::path& path)
		{
			// weakly_canonical resolves links and dot segments without requiring the folder to exist yet.
			fs::path result = fs::weakly_canonical(fs::absolute(path));
			while (!result.has_filename() && result.has_parent_path() && result != result.root_path())
			{
				result = result.parent_path();
			}
			return result;
		}

		// Component-wise comparison, so "D:\Vega2" is not treated as inside "D:\Vega". An empty root contains nothing.
		bool IsSameOrInside(const fs::path& path, const fs::path& root)
		{
			if (root.empty()) return false;
			auto candidate = path.begin();
			for (auto rootPart = root.begin(); rootPart != root.end(); ++rootPart, ++candidate)
			{
				if (candidate == path.end() || ComponentKey(*candidate) != ComponentKey(*rootPart)) return false;
			}
			return true;
		}

		bool Overlaps(const fs::path& first, const fs::path& second)
		{
			return IsSameOrInside(first, second) || IsSameOrInside(second, first);
		}

		// Manifest entries are relative and cannot climb out of the export folder. A tampered manifest must never
		// make the export delete files outside the folder.
		bool IsSafeRelative(const fs::path& relative)
		{
			if (relative.empty() || relative.is_absolute() || relative.has_root_directory()) return false;
			for (const auto& part : relative)
			{
				if (part.empty() || part == "..") return false;
			}
			return true;
		}

		std::string Lower(std::string text)
		{
			std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return text;
		}

		void WriteManifest(const fs::path& outputDir, const std::vector<std::string>& files)
		{
			std::ofstream out{ outputDir / kExportManifestName, std::ios::binary | std::ios::trunc };
			for (const auto& file : files) out << file << '\n';
			out.flush();
			if (!out) throw std::runtime_error("Cannot write " + std::string{ kExportManifestName });
		}

		std::vector<std::string> ReadManifest(const fs::path& outputDir)
		{
			std::ifstream in{ outputDir / kExportManifestName };
			std::vector<std::string> files;
			for (std::string line; std::getline(in, line);)
			{
				if (!line.empty() && line.back() == '\r') line.pop_back();
				if (!line.empty()) files.push_back(line);
			}
			return files;
		}

		// Removes exactly the files an earlier export recorded, then the folders that only held them. The manifest is
		// removed last, so a failure part-way leaves it in place to describe what is still there.
		void ClearPreviousExport(const fs::path& outputDir)
		{
			if (!fs::is_regular_file(outputDir / kExportManifestName)) return;
			std::set<std::string> parents;
			for (const auto& entry : ReadManifest(outputDir))
			{
				const fs::path relative = fs::u8path(entry);
				if (!IsSafeRelative(relative))
				{
					throw std::runtime_error("Export manifest lists a path outside the output folder: " + entry);
				}
				std::error_code error;
				const fs::path file = outputDir / relative;
				if (fs::is_regular_file(file, error) && !fs::remove(file, error) && error)
				{
					throw std::runtime_error("Cannot remove previous export file " + entry + ": " + error.message());
				}
				for (fs::path parent = relative.parent_path(); !parent.empty(); parent = parent.parent_path())
				{
					parents.insert(parent.generic_u8string());
				}
			}
			// Children sort after their parents, so reverse order removes the deepest folders first.
			for (auto folder = parents.rbegin(); folder != parents.rend(); ++folder)
			{
				std::error_code error;
				fs::remove(outputDir / fs::u8path(*folder), error);  // fails harmlessly while the folder is not empty
			}
			std::error_code error;
			fs::remove(outputDir / kExportManifestName, error);
			if (error) throw std::runtime_error("Cannot remove the export manifest: " + error.message());
		}

		struct CopyState
		{
			fs::path outputDir;
			std::vector<std::string> files;
			std::uintmax_t bytes = 0;
		};

		void CopyFileInto(CopyState& state, const fs::path& source, const fs::path& relative)
		{
			const fs::path destination = state.outputDir / relative;
			// A stray file at a destination belongs to the user. Refusing keeps it intact.
			std::error_code error;
			if (fs::exists(destination, error) || error)
			{
				throw std::runtime_error("Refusing to replace " + relative.generic_u8string() + ", which the export did not create");
			}
			fs::create_directories(destination.parent_path());
			fs::copy_file(source, destination);
			state.bytes += fs::file_size(destination);
			state.files.push_back(relative.generic_u8string());
		}

		void CopyTree(CopyState& state, const fs::path& source, const fs::path& relativeRoot)
		{
			for (auto it = fs::recursive_directory_iterator{ source }; it != fs::recursive_directory_iterator{}; ++it)
			{
				// Workspace recovery, Trash and tooling files belong to the editor. In
				// particular, a live session lock is deliberately not shareable for copy.
				if (ComponentKey(it->path().filename()) == ".vega")
				{
					if (it->is_directory()) it.disable_recursion_pending();
					continue;
				}
				if (!it->is_regular_file()) continue;
				CopyFileInto(state, it->path(), relativeRoot / it->path().lexically_relative(source));
			}
		}
	}

	GameExportResult ExportGame(const GameExportLayout& layout, const GameExportRequest& request)
	{
		GameExportResult result;
		CopyState state;
		fs::path outputDir;
		std::string executableName;
		bool copyStarted = false;
		try
		{
			// Layouts also come from tools and tests; validate package destinations
			// before copying or clearing an earlier export.
			auto contract = vega::ProjectSettings{}.ToJson();
			contract["runtime"] = { { "executable", layout.runtimeExecutable }, { "assetDirectory", layout.assetDirectory },
				{ "visualDirectory", layout.visualDirectory }, { "log", layout.gameLogName } };
			vega::ProjectSettings::Parse(contract);
			result.configuration = request.configuration;
			if (result.configuration == GameExportConfiguration::Automatic)
			{
				result.configuration = !layout.releaseGameBuildDir.empty() &&
					fs::is_regular_file(layout.releaseGameBuildDir / layout.runtimeExecutable)
					? GameExportConfiguration::Release : GameExportConfiguration::Debug;
			}
			const bool release = result.configuration == GameExportConfiguration::Release;
			if (!release) result.warning = "not portable: reads assets from the source tree";
			const fs::path buildDir = release ? layout.releaseGameBuildDir : layout.gameBuildDir;
			if (request.outputDir.empty()) throw std::runtime_error("No output folder");
			if (buildDir.empty() || !fs::is_regular_file(buildDir / layout.runtimeExecutable))
			{
				throw std::runtime_error("Game build not found: " + (buildDir / layout.runtimeExecutable).u8string());
			}
			if (!fs::is_directory(buildDir / layout.assetDirectory))
			{
				throw std::runtime_error("Game build has no assets folder: " + buildDir.u8string());
			}
			const fs::path assetRoot = layout.assetRoot.empty() ? buildDir / layout.assetDirectory : layout.assetRoot;
			vega::ProjectSettings project;
			if (fs::exists(assetRoot / "vega.project.json")) project = vega::ProjectSettings::Load(assetRoot / "vega.project.json");
			executableName = project.ExecutableName();
			const fs::path visualAssets = layout.visualAssetsDir.empty() ? fs::path{} : release ? buildDir / layout.visualDirectory : layout.visualAssetsDir;
			if (!visualAssets.empty() && !fs::is_directory(visualAssets))
			{
				throw std::runtime_error("Visual assets folder not found: " + visualAssets.u8string());
			}

			const fs::path gameDir = Canonical(buildDir);
			const fs::path visualDir = visualAssets.empty() ? fs::path{} : Canonical(visualAssets);
			const fs::path sourceRoot = layout.sourceTreeRoot.empty() ? fs::path{} : Canonical(layout.sourceTreeRoot);
			const fs::path generatedRoot = layout.generatedRoot.empty() ? fs::path{} : Canonical(layout.generatedRoot);
			outputDir = Canonical(request.outputDir);
			state.outputDir = outputDir;
			// Copying a folder into itself, or writing over the inputs, would corrupt both sides.
			if (Overlaps(outputDir, gameDir)) throw std::runtime_error("The export folder overlaps the game build folder");
			if (Overlaps(outputDir, Canonical(assetRoot))) throw std::runtime_error("The export folder overlaps the authoring assets");
			if (!visualDir.empty() && Overlaps(outputDir, visualDir)) throw std::runtime_error("The export folder overlaps the visual assets folder");
			// The repository stays read-only for exports. Generated output is allowed under the build tree.
			if (Overlaps(outputDir, sourceRoot) && !IsSameOrInside(outputDir, generatedRoot))
			{
				throw std::runtime_error("The export folder is inside the source tree; export under the build folder instead");
			}

			std::error_code error;
			if (fs::exists(outputDir, error))
			{
				if (!fs::is_directory(outputDir)) throw std::runtime_error("The export target is not a folder");
				if (!fs::is_empty(outputDir))
				{
					if (!request.overwrite) throw std::runtime_error("The output folder is not empty; enable Overwrite to replace a previous export");
					ClearPreviousExport(outputDir);
				}
			}
			else
			{
				fs::create_directories(outputDir);
			}

			// The game executable and every DLL beside it, in name order so the manifest is stable.
			std::vector<fs::path> runtimeFiles;
			for (const auto& entry : fs::directory_iterator{ gameDir })
			{
				if (!entry.is_regular_file()) continue;
				const fs::path name = entry.path().filename();
				if (name == layout.runtimeExecutable || Lower(name.extension().u8string()) == ".dll") runtimeFiles.push_back(name);
			}
			std::sort(runtimeFiles.begin(), runtimeFiles.end());

			copyStarted = true;
			for (const auto& name : runtimeFiles) CopyFileInto(state, gameDir / name, name == layout.runtimeExecutable ? fs::u8path(executableName) : name);
			// Both layouts are kept: the game reads assets/ and visual/ relative to its working directory.
			CopyTree(state, assetRoot, layout.assetDirectory);
			if (!visualDir.empty()) CopyTree(state, visualDir, layout.visualDirectory);
			WriteManifest(outputDir, state.files);
			result.success = true;
			result.filesCopied = state.files.size();
			result.bytes = state.bytes;
		}
		catch (const std::exception& error)
		{
			result.success = false;
			result.error = error.what();
			// Record what this run copied, so the next overwrite can remove it. A failure before copying must not
			// touch the manifest, which may still describe a previous export that has not been cleared.
			if (copyStarted && !state.files.empty())
			{
				try
				{
					WriteManifest(outputDir, state.files);
				}
				catch (const std::exception&)
				{
				}
			}
			return result;
		}

		if (request.runSmokeTest)
		{
			// A log that was already there belongs to the user, so only a log the smoke run created is claimed.
			std::error_code logError;
			const bool logExisted = fs::exists(outputDir / layout.gameLogName, logError);
			GameSmokeTestResult smoke = RunGameSmokeTest(outputDir, 60000, layout.assetDirectory);
			if (smoke.passed && result.configuration == GameExportConfiguration::Release)
			{
				const auto portability = CheckGameLogPortability(outputDir / layout.gameLogName, layout.sourceTreeRoot);
				if (!portability.passed) smoke = portability;
				else smoke.detail += ", " + portability.detail;
			}
			result.smokeTestRan = true;
			result.smokeTestPassed = smoke.passed;
			result.smokeTestDetail = smoke.detail;
			// The capture and the game's log are output too. Listing them lets the next overwrite remove them.
			try
			{
				if (fs::is_regular_file(outputDir / kExportSmokeCaptureName))
				{
					state.files.push_back(kExportSmokeCaptureName);
				}
				if (!logExisted && fs::is_regular_file(outputDir / layout.gameLogName))
				{
					state.files.push_back(layout.gameLogName);
				}
				WriteManifest(outputDir, state.files);
			}
			catch (const std::exception& error)
			{
				result.success = false;
				result.error = error.what();
			}
		}
		return result;
	}

	GameSmokeTestResult RunGameSmokeTest(const fs::path& gameDir, unsigned int timeoutMs, const std::string& assetDirectory)
	{
		GameSmokeTestResult result;
		vega::ProjectSettings project;
		try
		{
			if (fs::exists(gameDir / assetDirectory / "vega.project.json")) project = vega::ProjectSettings::Load(gameDir / assetDirectory / "vega.project.json");
		}
		catch (const std::exception& error) { result.detail = error.what(); return result; }
		const fs::path executable = gameDir / fs::u8path(project.ExecutableName());
		const fs::path capture = gameDir / kExportSmokeCaptureName;
		// A capture left by an earlier run must not satisfy this one.
		std::error_code error;
		fs::remove(capture, error);

		std::wstring commandLine = L"\"" + executable.wstring() + L"\" --visual-capture \"" + capture.wstring() + L"\" --capture-frames 120 --asset-root \"" + (gameDir / assetDirectory).wstring() + L"\"";
		const std::wstring applicationPath = executable.wstring();
		const std::wstring workingDirectory = gameDir.wstring();
		STARTUPINFOW startup{};
		startup.cb = sizeof(startup);
		PROCESS_INFORMATION process{};
		// Working directory is the export folder, which is where the game looks for assets/ and visual/.
		if (!CreateProcessW(applicationPath.c_str(), commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr,
			workingDirectory.c_str(), &startup, &process))
		{
			result.detail = "Cannot start " + executable.filename().string() + " (Windows error " + std::to_string(GetLastError()) + ")";
			return result;
		}
		CloseHandle(process.hThread);

		const DWORD waited = WaitForSingleObject(process.hProcess, timeoutMs);
		if (waited != WAIT_OBJECT_0)
		{
			TerminateProcess(process.hProcess, 1);
			WaitForSingleObject(process.hProcess, 5000);
			CloseHandle(process.hProcess);
			result.detail = waited == WAIT_TIMEOUT
				? "Timed out after " + std::to_string(timeoutMs / 1000) + " s; the game was terminated"
				: "Cannot wait for the game; it was terminated";
			return result;
		}
		DWORD exitCode = 1;
		GetExitCodeProcess(process.hProcess, &exitCode);
		CloseHandle(process.hProcess);
		if (exitCode != 0)
		{
			result.detail = "The game exited with code " + std::to_string(exitCode);
			return result;
		}

		std::error_code sizeError;
		const bool captured = fs::is_regular_file(capture, sizeError);
		const std::uintmax_t size = captured ? fs::file_size(capture, sizeError) : 0;
		if (sizeError || size == 0)
		{
			result.detail = std::string{ "The game exited with code 0 but wrote no non-empty " } + kExportSmokeCaptureName;
			return result;
		}
		result.passed = true;
		result.detail = "exit code 0, capture " + std::to_string(size) + " bytes";
		return result;
	}

	GameSmokeTestResult CheckGameLogPortability(const fs::path& log, const fs::path& sourceTreeRoot)
	{
		std::ifstream input{ log };
		if (!input) return { false, "Cannot read game log for portability check" };
		if (sourceTreeRoot.empty()) return { false, "No source tree root for portability check" };
		// Runtime logs can contain either slash style and arbitrary drive/folder case.
		auto normalize = [](std::string value)
		{
			std::replace(value.begin(), value.end(), '\\', '/');
			return Lower(std::move(value));
		};
		const std::string root = normalize(Canonical(sourceTreeRoot).generic_u8string());
		const std::string exportedRoot = normalize(Canonical(log.parent_path()).generic_u8string()) + "/";
		std::size_t lineNumber = 0;
		for (std::string line; std::getline(input, line);)
		{
			++lineNumber;
			auto normalized = normalize(line);
			// Debug logs append their compiled source location. That identifies
			// diagnostic code, not a resource loaded from the developer's tree.
			static const std::regex sourceLocation{ R"( \([^()\r\n]+\.(cpp|cc|cxx|h|hpp|inl):[0-9]+\)$)" };
			normalized = std::regex_replace(normalized, sourceLocation, "");
			// Integration exports live under build/ inside the repository. Their own copied assets are valid;
			// only references outside the export to the original source tree indicate a portability leak.
			for (auto position = normalized.find(exportedRoot); position != std::string::npos;
				position = normalized.find(exportedRoot, position + 9))
			{
				normalized.replace(position, exportedRoot.size(), "<export>/");
			}
			for (auto position = normalized.find(root); position != std::string::npos;
				position = normalized.find(root, position + root.size()))
			{
				// A sibling such as Vega-ui is outside a configured Vega root.
				const auto end = position + root.size();
				const auto pathCharacter = [](unsigned char c)
				{
					return std::isalnum(c) || c == '_' || c == '-' || c == '.' || c == '/' || c == ':';
				};
				if ((position == 0 || !pathCharacter(static_cast<unsigned char>(normalized[position - 1]))) &&
					(end == normalized.size() || normalized[end] == '/' || !pathCharacter(static_cast<unsigned char>(normalized[end]))))
					return { false, "Source-tree path in game log at line " + std::to_string(lineNumber) + ": " + line };
			}
		}
		if (input.bad()) return { false, "Cannot read game log for portability check" };
		if (lineNumber == 0) return { false, "Empty game log for portability check" };
		return { true, "no source-tree paths in game log" };
	}

	std::string DescribeGameExport(const GameExportResult& result)
	{
		if (!result.success) return "Export failed: " + result.error;
		std::ostringstream text;
		text << "Exported " << (result.configuration == GameExportConfiguration::Release ? "Release" : "Debug")
			<< ": " << result.filesCopied << " files, " << std::fixed << std::setprecision(1)
			<< static_cast<double>(result.bytes) / (1024.0 * 1024.0) << " MB";
		if (!result.warning.empty()) text << ". Warning: " << result.warning;
		if (!result.smokeTestRan) text << ". Smoke test not run";
		else if (result.smokeTestPassed) text << ". Smoke test OK (" << result.smokeTestDetail << ")";
		else text << ". Smoke test FAIL: " << result.smokeTestDetail;
		return text.str();
	}
}
