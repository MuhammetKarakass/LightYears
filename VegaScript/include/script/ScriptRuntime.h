#pragma once

#include <filesystem>
#include <memory>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

namespace vega { class Actor; class World; class EngineContext; }

namespace vega::script
{
	class ScriptComponent;
	// One interpreter per context on the game/editor thread; components own isolated environments.
	class ScriptRuntime
	{
	public:
		// Deprecated no-argument shim; new hosts supply their context.
		static ScriptRuntime& Get();
		static ScriptRuntime& Get(vega::EngineContext& context);
		~ScriptRuntime();
		void SetRoot(const std::filesystem::path& root);
		void SetHotReload(bool enabled);
		bool HotReload() const;
		void SetInstructionBudget(std::uint64_t instructions);
		void SetMemoryLimit(std::size_t bytes);
		const std::filesystem::path& Root() const;
		using PlayerResolver = std::function<std::weak_ptr<vega::Actor>(vega::World&)>;
		void SetPlayerResolver(PlayerResolver resolver);
		std::shared_ptr<vega::Actor> Player(vega::World& world) const;
		// Optional device adapter, also permits deterministic headless input tests.
		using KeyReader = std::function<bool(const std::string&)>;
		void SetKeyReader(KeyReader reader);
		bool ReadKey(const std::string& key) const;
	private:
		ScriptRuntime();
		struct Impl;
		std::unique_ptr<Impl> mImpl;
		friend class ScriptComponent;
		friend void SetScriptRoot(const std::filesystem::path& root);
	};
	void SetScriptRoot(const std::filesystem::path& root);
	void RegisterScriptTypes();
	void RegisterScriptTypes(vega::EngineContext& context);
	bool GenerateLuaTypes(const std::filesystem::path& output, std::string& error) noexcept;
	bool GenerateLuaTypes(vega::EngineContext& context, const std::filesystem::path& output, std::string& error) noexcept;
}
