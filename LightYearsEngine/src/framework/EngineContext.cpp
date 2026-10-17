#include "framework/EngineContext.h"
#include "framework/AssetManager.h"
#include "framework/AudioManager.h"
#include "framework/AudioOutput.h"
#include "framework/EngineAssets.h"
#include "framework/reflection/Reflection.h"
#include "framework/scene/PrefabLibrary.h"
#include "framework/scene/SceneSettings.h"
#include "framework/content/ContentSchema.h"
#include "framework/scene/Scene.h"
#include "widget/UIStyle.h"
#include <filesystem>

namespace ly
{
	namespace
	{
		thread_local EngineContext::Scope* currentScope = nullptr;
	}

	EngineContext::EngineContext()
		: mTypes{ new reflect::TypeRegistry }, mPrefabs{ new PrefabLibrary{ *this, std::filesystem::current_path() } },
		mDefaults{ new EngineAssets }, mAssets{ new AssetManager{ *this } },
		mSceneSettings{ new SceneSettingsSchema }, mContentSchemas{ new content::ContentSchemaRegistry },
		mSceneServices{ new SceneServices }
	{
		mSceneSettings->RegisterEngineSettings();
		// Custom test hosts bypass EntryPoint, and components can play without an AudioManager.
		AudioOutput::Initialize();
		reflect::RegisterEngineTypes(*mTypes);
	}

	EngineContext::~EngineContext()
	{
		mScripts.reset();
		ShutdownAudio();
		mAssets->Shutdown();
	}

	AudioManager& EngineContext::Audio()
	{
		if (!mAudio) mAudio.reset(new AudioManager{ *this });
		return *mAudio;
	}

	UIStyle& EngineContext::Style()
	{
		if (!mStyle) mStyle = std::make_unique<UIStyle>(UIStyle::MakeDefault(*this));
		return *mStyle;
	}

	void EngineContext::ShutdownAudio()
	{
		if (mAudio) mAudio->Shutdown();
		mAudio.reset();
	}

	EngineContext& EngineContext::Current()
	{
		if (currentScope) return *currentScope->mContext;
		static EngineContext legacy;
		return legacy;
	}

	EngineContext::Scope::Scope(EngineContext& context) : mContext{ &context }, mPrevious{ currentScope }
	{
		if (mPrevious) mPrevious->mNext = this;
		currentScope = this;
	}

	EngineContext::Scope::~Scope()
	{
		// Applications can close in a different order from their construction.
		// Unlink in O(1), so a surviving host never restores a destroyed binding.
		if (mNext) mNext->mPrevious = mPrevious;
		else currentScope = mPrevious;
		if (mPrevious) mPrevious->mNext = mNext;
	}

	EngineAssets& EngineAssets::Get() { return EngineContext::Current().Defaults(); }
	content::ContentSchemaRegistry& content::ContentSchemaRegistry::Get() { return EngineContext::Current().ContentSchemas(); }
}
