#pragma once

#include <filesystem>
#include <memory>
#include <cstddef>
#include <cstdint>

namespace vega::script
{
	class ScriptComponent;
	// One interpreter on the game/editor thread; components own isolated environments.
	class ScriptRuntime
	{
	public:
		static ScriptRuntime& Get();
		~ScriptRuntime();
		void SetHotReload(bool enabled);
		bool HotReload() const;
		void SetInstructionBudget(std::uint64_t instructions);
		void SetMemoryLimit(std::size_t bytes);
		const std::filesystem::path& Root() const;
	private:
		ScriptRuntime();
		struct Impl;
		std::unique_ptr<Impl> mImpl;
		friend class ScriptComponent;
		friend void SetScriptRoot(const std::filesystem::path& root);
	};
	void SetScriptRoot(const std::filesystem::path& root);
	void RegisterScriptTypes();
}
