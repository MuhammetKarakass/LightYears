#pragma once

#include <memory>

namespace vega::script { class ScriptRuntime; }

namespace vega
{
	class AssetManager;
	class AudioManager;
	struct EngineAssets;
	class PrefabLibrary;
	class SceneSettingsSchema;
	struct SceneServices;
	struct UIStyle;
	namespace reflect { class TypeRegistry; }
	namespace content { class ContentSchemaRegistry; }

	// Host-owned services. Objects retain a context pointer; only legacy
	// entry points consult Current(). No engine dependency on the scripting target.
	class EngineContext
	{
	public:
		EngineContext();
		~EngineContext();
		EngineContext(const EngineContext&) = delete;
		EngineContext& operator=(const EngineContext&) = delete;

		reflect::TypeRegistry& Types() const { return *mTypes; }
		PrefabLibrary& Prefabs() const { return *mPrefabs; }
		AssetManager& Assets() const { return *mAssets; }
		EngineAssets& Defaults() const { return *mDefaults; }
		SceneSettingsSchema& SceneSettings() const { return *mSceneSettings; }
		content::ContentSchemaRegistry& ContentSchemas() const { return *mContentSchemas; }
		SceneServices& SceneSerialization() const { return *mSceneServices; }
		UIStyle& Style();
		AudioManager& Audio();
		bool HasAudio() const { return mAudio != nullptr; }
		void ShutdownAudio();
		// The optional module installs its interpreter; its deleter stays module-side.
		std::shared_ptr<vega::script::ScriptRuntime>& Scripts() { return mScripts; }

		// Deprecated compatibility boundary for old static accessors and ownerless
		// constructors. Applications adopt the innermost Scope by default, or the
		// function-local static legacy context when no scope is bound.
		static EngineContext& Current();
		class Scope
		{
		public:
			explicit Scope(EngineContext& context);
			~Scope();
			Scope(const Scope&) = delete;
			Scope& operator=(const Scope&) = delete;
		private:
			friend class EngineContext;
			EngineContext* mContext;
			Scope* mPrevious;
			Scope* mNext = nullptr;
		};

	private:
		std::unique_ptr<reflect::TypeRegistry> mTypes;
		std::unique_ptr<PrefabLibrary> mPrefabs;
		std::unique_ptr<EngineAssets> mDefaults;
		std::unique_ptr<AssetManager> mAssets;
		std::unique_ptr<SceneSettingsSchema> mSceneSettings;
		std::unique_ptr<content::ContentSchemaRegistry> mContentSchemas;
		std::unique_ptr<SceneServices> mSceneServices;
		std::unique_ptr<UIStyle> mStyle;
		std::unique_ptr<AudioManager> mAudio;
		std::shared_ptr<vega::script::ScriptRuntime> mScripts;
	};
}
