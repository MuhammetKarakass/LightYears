#pragma once

#include "editor/EditorSelection.h"
#include "editor/EditorCamera.h"
#include "editor/GizmoMath.h"
#include <framework/Actor.h>
#include <framework/ComponentIdentity.h>
#include <framework/reflection/Reflection.h>
#include <cstdint>
#include <string>
#include <variant>
#include <nlohmann/json.hpp>
#include <functional>
#include <unordered_map>

namespace vega
{
	class JsonDocument;
	struct EditorContext
	{
		ly::World* world = nullptr;
		EditorSelection& selection;
		EditorCamera* camera = nullptr;
		std::optional<bool> cameraPaused;
		JsonDocument* document = nullptr;
		EditorViewSettings viewSettings;
		bool playMode = false;
		bool transitioning = false;
		std::function<bool(const std::string&, const std::string&)> sceneAction;
		std::function<bool()> sceneDirty;
		std::string sceneError;
		std::string propertyError;
		unsigned int propertyErrorActor = 0;
		std::optional<ly::ComponentIdentity> propertyErrorComponent;
		std::string propertyErrorName;
		// Monotonic edit revisions deliberately keep undo-to-save dirty until Save/Reload.
		std::uint64_t sceneRevision = 0;
		void SceneChanged() { ++sceneRevision; }
		void ResetActorCache() const { cachedWorldID.reset(); }
		void RefreshActorCache() const;
		mutable std::optional<unsigned int> cachedWorldID;
		mutable std::size_t cachedActorCount = 0;
		mutable std::unordered_map<unsigned int, std::weak_ptr<ly::Actor>> actorsByID;
		mutable std::unordered_map<std::string, std::weak_ptr<ly::Actor>> actorsByGuid;
		void SyncCamera();
		bool SetEditorCamera(bool enabled);
		bool FocusSelection();
		void ApplyCamera();
		std::shared_ptr<ly::Actor> Resolve(unsigned int id) const;
		std::shared_ptr<ly::Actor> Resolve(unsigned int id, std::string& guid) const;
		std::shared_ptr<ly::Actor> ResolveGuid(const std::string& guid) const;
	};

	std::string ActorTypeName(const ly::Actor& actor);

	class EditorCommand
	{
	public:
		virtual ~EditorCommand() = default;
		virtual bool Execute(EditorContext&) = 0;
		virtual void Undo(EditorContext&) = 0;
		virtual std::string Name() const = 0;
		virtual bool MergeWith(const EditorCommand&) { return false; }
		virtual bool IsUndoable() const { return true; }
		virtual bool ChangesScene() const { return IsUndoable(); }
		virtual bool IsValid(const EditorContext&) const { return true; }
	};

	class CommandHistory
	{
	public:
		explicit CommandHistory(EditorContext& context, std::size_t maxDepth = 256);
		bool Execute(std::unique_ptr<EditorCommand> command);
		bool Undo();
		bool Redo();
		bool CanUndo() const { return !mUndo.empty(); }
		bool CanRedo() const { return !mRedo.empty(); }
		std::size_t UndoCount() const { return mUndo.size(); }
		std::size_t RedoCount() const { return mRedo.size(); }
		std::string UndoName() const;
		std::string RedoName() const;
		void Clear();
		void PruneInvalidCommands();
		// Zero means a discrete edit. Each gesture gets a new token, even on the same property.
		std::uint64_t BeginMergeGroup() { return ++mNextMergeGroup; }

	private:
		EditorContext& mContext;
		std::size_t mMaxDepth;
		std::uint64_t mNextMergeGroup = 0;
		bool mMergeAllowed = false;
		std::vector<std::unique_ptr<EditorCommand>> mUndo;
		std::vector<std::unique_ptr<EditorCommand>> mRedo;
	};

	// Re-resolved for every operation; history never retains actor/component pointers.
	struct ReflectedTarget
	{
		void* object = nullptr;
		const ly::reflect::TypeInfo* type = nullptr;
		std::optional<ly::ComponentIdentity> componentIdentity;
	};
	ReflectedTarget ResolvePropertyTarget(ly::Actor& actor, std::optional<ly::ComponentIdentity> componentIdentity);
	ReflectedTarget FindPropertyTarget(ly::Actor& actor, const std::string& targetName);

	class SetScriptPropertyCommand final : public EditorCommand
	{
	public:
		SetScriptPropertyCommand(unsigned int id, ly::ComponentIdentity componentIdentity, std::string property,
			ly::reflect::PropertyValue value, std::uint64_t group = 0);
		bool Execute(EditorContext&) override;
		void Undo(EditorContext&) override;
		std::string Name() const override { return "Set script " + mProperty; }
		bool MergeWith(const EditorCommand&) override;
	private:
		unsigned int mID;
		std::string mGuid;
		ly::ComponentIdentity mComponentIdentity;
		std::string mProperty;
		ly::reflect::PropertyValue mAfter;
		std::optional<ly::reflect::PropertyValue> mBeforeOverride;
		std::uint64_t mMergeGroup;
		bool mExecuted = false;
	};

	class SetPropertyCommand final : public EditorCommand
	{
	public:
		SetPropertyCommand(unsigned int id, std::optional<ly::ComponentIdentity> componentIdentity,
			std::string property, ly::reflect::PropertyValue after, std::uint64_t group = 0);
		bool Execute(EditorContext&) override;
		void Undo(EditorContext&) override;
		std::string Name() const override;
		bool MergeWith(const EditorCommand&) override;
	private:
		unsigned int mID;
		std::string mGuid;
		std::optional<ly::ComponentIdentity> mComponentIdentity;
		std::string mProperty;
		std::string mActorName;
		ly::reflect::PropertyValue mBefore;
		ly::reflect::PropertyValue mAfter;
		std::uint64_t mMergeGroup;
		bool mExecuted = false;
	};

	class SelectActor final : public EditorCommand
	{
	public:
		explicit SelectActor(std::optional<unsigned int> id) : mID{ id } {}
		bool Execute(EditorContext&) override;
		void Undo(EditorContext&) override {}
		std::string Name() const override { return "Select actor"; }
		bool IsUndoable() const override { return false; }
	private:
		std::optional<unsigned int> mID;
	};
	class SetPaused final : public EditorCommand
	{
	public:
		explicit SetPaused(bool paused) : mPaused{ paused } {}
		bool Execute(EditorContext&) override;
		void Undo(EditorContext&) override {}
		std::string Name() const override { return mPaused ? "Pause" : "Play"; }
		bool IsUndoable() const override { return false; }
	private:
		bool mPaused;
	};
	class DestroyActor final : public EditorCommand
	{
	public:
		explicit DestroyActor(unsigned int id) : mID{ id } {}
		bool Execute(EditorContext&) override;
		void Undo(EditorContext&) override;
		std::string Name() const override { return "Destroy actor #" + std::to_string(mID); }
		bool IsUndoable() const override { return !mEntity.is_null(); }
	private:
		unsigned int mID;
		std::string mGuid;
		nlohmann::json mEntity;
	};

	class ComponentCommand : public EditorCommand
	{
	public:
		bool Execute(EditorContext&) override;
		void Undo(EditorContext&) override;
		std::string Name() const override { return std::string{ mAdding ? "Add " : "Remove " } + mComponent.typeName; }
	protected:
		ComponentCommand(unsigned int id, ly::ComponentIdentity component, bool adding)
			: mID{ id }, mComponent{ std::move(component) }, mAdding{ adding } {}
	private:
		bool Restore(EditorContext&, const nlohmann::json&);
		unsigned int mID;
		std::string mGuid;
		ly::ComponentIdentity mComponent;
		bool mAdding;
		nlohmann::json mBefore;
		nlohmann::json mAfter;
	};

	class AddComponentCommand final : public ComponentCommand
	{
	public:
		AddComponentCommand(unsigned int id, std::string type) : ComponentCommand{ id, { std::move(type), 0 }, true } {}
	};
	class RemoveComponentCommand final : public ComponentCommand
	{
	public:
		RemoveComponentCommand(unsigned int id, ly::ComponentIdentity component) : ComponentCommand{ id, std::move(component), false } {}
	};

	class CreateEntity final : public EditorCommand
	{
	public:
		explicit CreateEntity(nlohmann::json entity) : mEntity(std::move(entity)) {}
		bool Execute(EditorContext&) override;
		void Undo(EditorContext&) override;
		std::string Name() const override { return "Create " + mEntity.value("type", std::string{}); }
	private:
		nlohmann::json mEntity;
	};

	bool CreateSceneEntity(EditorContext&, CommandHistory&, const std::string& type);
	bool DuplicateSceneEntity(EditorContext&, CommandHistory&);
	bool RenameSceneEntity(EditorContext&, CommandHistory&, const std::string& name, std::uint64_t group = 0);
}
