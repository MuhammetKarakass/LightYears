#pragma once

#include <framework/ActorComponent.h>
#include <framework/reflection/Reflection.h>
#include <map>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

namespace vega::script
{
	// Script values reuse reflection's small value type; only float/bool/string/Vector2 are accepted.
	using ScriptProperties = std::map<std::string, vega::reflect::PropertyValue>;
	class ScriptComponent final : public vega::ActorComponent
	{
		VEGA_CLASS(ScriptComponent)
	public:
		ScriptComponent(vega::Actor& owner, std::string path);
		~ScriptComponent() override;
		void BeginPlay() override;
		void TickComponent(float deltaTime) override;
		void EndPlay() override;
		bool Reload() noexcept;
		// Reads declarations without on_create/on_destroy; scene loading can
		// validate overrides while the actor is still pending.
		bool Prepare() noexcept;
		const std::string& ScriptPath() const;
		const std::string& LastError() const;
		const ScriptProperties& Properties() const;
		const ScriptProperties& Overrides() const;
		// Stages overrides before BeginPlay without executing the script. Loading
		// validates names/types against its declarations before on_create runs.
		bool SetOverrides(const ScriptProperties& values) noexcept;
		bool SetProperty(const std::string& name, const vega::reflect::PropertyValue& value) noexcept;
		bool ClearOverride(const std::string& name) noexcept;
		bool QueueMessage(const std::string& name, const nlohmann::json& data);
	private:
		void OnOverlapBegin(vega::Actor* other) override;
		void OnOverlapEnd(vega::Actor* other) override;
		void OnHit(vega::Actor* other) override;
		struct Impl;
		std::unique_ptr<Impl> mImpl;
	};
}
