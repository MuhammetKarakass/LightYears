#pragma once

#include <filesystem>
#include <memory>

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
