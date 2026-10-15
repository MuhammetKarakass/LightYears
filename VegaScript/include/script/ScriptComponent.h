#pragma once

#include <framework/ActorComponent.h>
#include <framework/reflection/Reflection.h>
#include <map>
#include <memory>
#include <string>

namespace vega::script
{
	// Script values reuse reflection's small value type; only float/bool/string/Vector2 are accepted.
	using ScriptProperties = std::map<std::string, ly::reflect::PropertyValue>;
	class ScriptComponent final : public ly::ActorComponent
	{
	public:
		ScriptComponent(ly::Actor& owner, std::string path);
		~ScriptComponent() override;
		void BeginPlay() override;
		void TickComponent(float deltaTime) override;
		void EndPlay() override;
		bool Reload() noexcept;
		const std::string& ScriptPath() const;
		const std::string& LastError() const;
		const ScriptProperties& Properties() const;
		const ScriptProperties& Overrides() const;
		bool SetProperty(const std::string& name, const ly::reflect::PropertyValue& value) noexcept;
		bool ClearOverride(const std::string& name) noexcept;
	private:
		struct Impl;
		std::unique_ptr<Impl> mImpl;
	};
}
