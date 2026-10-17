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
#include <unordered_set>
#include <framework/debug/LogBuffer.h>

namespace vega
{
	struct EditorScriptCommand;
	class EditorExtensions;
	class JsonDocument;
	struct EditorContext
	{
		vega::World* world = nullptr;
		EditorSelection& selection;
		EditorCamera* camera = nullptr;
		std::optional<bool> cameraPaused;
		JsonDocument* document = nullptr;
		EditorViewSettings viewSettings;
		bool playMode = false;
		bool transitioning = false;
		bool relativeTransforms = false;
		std::string attachmentRoot;
		std::function<bool(const std::string&, const std::string&)> sceneAction;
		std::function<bool(const std::string&, const std::string&)> prefabAction;
		std::function<bool()> sceneDirty;
		std::function<void()> contentChanged;
		std::string sceneError;
		nlohmann::json* sceneSettings = nullptr;
		std::function<void(const nlohmann::json&)> applySettings;
		std::function<std::string()> backdropState;
		std::string settingErrorKey, settingError;
		vega::debug::LogBuffer* console = nullptr;
		std::function<void()> clearConsole;
		std::function<bool(const std::string&, bool)> audioPreference;
		std::function<void(bool)> diagnosticsVisible;
		std::function<bool(const EditorScriptCommand&, std::string&)> contentCommand;
		std::function<bool(const EditorScriptCommand&, std::string&)> autosaveCommand;
		std::function<bool(const EditorScriptCommand&, std::string&)> assetCommand;
		EditorExtensions* extensions = nullptr;
		std::string propertyError;
		unsigned int propertyErrorActor = 0;
		std::optional<vega::ComponentIdentity> propertyErrorComponent;
		std::string propertyErrorName;
		// Monotonic edit revisions deliberately keep undo-to-save dirty until Save/Reload.
		std::uint64_t sceneRevision = 0;
		std::optional<unsigned int> prefabOverrideActor;
		std::uint64_t prefabOverrideRevision = 0;
		std::unordered_map<const void*, std::unordered_set<std::string>> prefabOverrides;
		vega::EngineContext& Engine() const { return world ? world->GetEngineContext() : *services; }
		// Set by the host, also used while scene transitions have no world.
		vega::EngineContext* services = &vega::EngineContext::Current();
		void SceneChanged() { ++sceneRevision; selection.Rebind([this](const std::string& guid) { return ResolveGuid(guid); }); }
		void ResetActorCache() const { cachedWorldID.reset(); }
		void RefreshActorCache() const;
		mutable std::optional<unsigned int> cachedWorldID;
		mutable std::size_t cachedActorCount = 0;
		mutable std::unordered_map<unsigned int, std::weak_ptr<vega::Actor>> actorsByID;
		mutable std::unordered_map<std::string, std::weak_ptr<vega::Actor>> actorsByGuid;
		void SyncCamera();
		bool SetEditorCamera(bool enabled);
		bool FocusSelection();
		void ApplyCamera();
		std::shared_ptr<vega::Actor> Resolve(unsigned int id) const;
		std::shared_ptr<vega::Actor> Resolve(unsigned int id, std::string& guid) const;
		std::shared_ptr<vega::Actor> ResolveGuid(const std::string& guid) const;
	};

	std::string ActorTypeName(const vega::Actor& actor);

	class EditorCommand
	{
	public:
		virtual ~EditorCommand() = default;
		virtual bool Execute(EditorContext&) = 0;
		virtual void Undo(EditorContext&) = 0;
		virtual bool TryUndo(EditorContext& context) { Undo(context); return true; }
		virtual std::string Name() const = 0;
		virtual bool MergeWith(const EditorCommand&) { return false; }
		virtual bool IsUndoable() const { return true; }
		virtual bool ChangesScene() const { return IsUndoable(); }
		virtual bool IsValid(const EditorContext&) const { return true; }
	};

	// A gesture is atomic: failed children roll back, undo runs in reverse order.
	class CompoundCommand final : public EditorCommand
	{
	public:
		explicit CompoundCommand(std::string name, std::uint64_t group = 0, bool selectCreated = false)
			: mName{ std::move(name) }, mGroup{ group }, mSelectCreated{ selectCreated } {}
		void Add(std::unique_ptr<EditorCommand> command) { mCommands.push_back(std::move(command)); }
		bool Execute(EditorContext&) override;
		void Undo(EditorContext&) override;
		bool MergeWith(const EditorCommand&) override;
		bool IsValid(const EditorContext&) const override;
		std::string Name() const override { return mName; }
	private:
		std::string mName;
		std::uint64_t mGroup;
		bool mSelectCreated;
		bool mExecuted = false;
		EditorSelection mBefore, mAfter;
		std::vector<std::unique_ptr<EditorCommand>> mCommands;
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
		const vega::reflect::TypeInfo* type = nullptr;
		std::optional<vega::ComponentIdentity> componentIdentity;
	};
	ReflectedTarget ResolvePropertyTarget(vega::Actor& actor, std::optional<vega::ComponentIdentity> componentIdentity);
	ReflectedTarget FindPropertyTarget(vega::Actor& actor, const std::string& targetName);

	// A leaf inside a reflected struct: "Name.Field" for a struct property, "Name[i].Field" for an element of a struct list.
	// Edits still write the whole top-level property through SetPropertyCommand, so validation and undo see one value.
	struct StructLeafPath
	{
		const vega::reflect::PropertyInfo* property = nullptr;
		std::optional<std::size_t> index;
		const vega::reflect::PropertyInfo* leaf = nullptr;
		std::string field;
	};
	bool ResolveStructLeafPath(const ReflectedTarget& target, const std::string& path, StructLeafPath& resolved, std::string& error);
	// Points at the stored value of the addressed field inside a copy of the top-level value; nullptr with error on failure.
	vega::reflect::LeafValue* FindStructLeaf(const StructLeafPath& path, vega::reflect::PropertyValue& value, std::string& error);

	class SetSceneSettingCommand final : public EditorCommand
	{
	public:
		SetSceneSettingCommand(std::string key, std::optional<nlohmann::json> value, std::uint64_t group = 0)
			: mKey{ std::move(key) }, mAfter{ std::move(value) }, mGroup{ group } {}
		bool Execute(EditorContext&) override;
		void Undo(EditorContext&) override;
		bool MergeWith(const EditorCommand&) override;
		std::string Name() const override { return (mAfter ? "Set " : "Reset ") + mKey; }
	private:
		std::string mKey;
		std::optional<nlohmann::json> mBefore, mAfter;
		std::uint64_t mGroup;
		bool mExecuted = false;
	};

	class SetScriptPropertyCommand final : public EditorCommand
	{
	public:
		SetScriptPropertyCommand(unsigned int id, vega::ComponentIdentity componentIdentity, std::string property,
			vega::reflect::PropertyValue value, std::uint64_t group = 0);
		bool Execute(EditorContext&) override;
		void Undo(EditorContext&) override;
		std::string Name() const override { return "Set script " + mProperty; }
		bool MergeWith(const EditorCommand&) override;
	private:
		unsigned int mID;
		std::string mGuid;
		vega::ComponentIdentity mComponentIdentity;
		std::string mProperty;
		vega::reflect::PropertyValue mAfter;
		std::optional<vega::reflect::PropertyValue> mBeforeOverride;
		std::uint64_t mMergeGroup;
		bool mExecuted = false;
	};

	class SetPropertyCommand final : public EditorCommand
	{
	public:
		SetPropertyCommand(unsigned int id, std::optional<vega::ComponentIdentity> componentIdentity,
			std::string property, vega::reflect::PropertyValue after, std::uint64_t group = 0, bool relative = false);
		bool Execute(EditorContext&) override;
		void Undo(EditorContext&) override;
		std::string Name() const override;
		bool MergeWith(const EditorCommand&) override;
	private:
		unsigned int mID;
		std::string mGuid;
		std::optional<vega::ComponentIdentity> mComponentIdentity;
		std::string mProperty;
		std::string mActorName;
		bool mRelative = false;
		vega::reflect::PropertyValue mBefore;
		vega::reflect::PropertyValue mAfter;
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
	class ReparentActor final : public EditorCommand
	{
	public:
		ReparentActor(std::string child, std::string parent) : mChild{ std::move(child) }, mParent{ std::move(parent) } {}
		bool Execute(EditorContext&) override;
		void Undo(EditorContext&) override;
		std::string Name() const override { return mParent.empty() ? "Detach" : "Parent"; }
	private:
		std::string mChild, mParent, mBeforeParent;
		nlohmann::json mPrefabStates;
		sf::Vector2f mBeforeLocation{};
		float mBeforeRotation = 0.f;
		bool mExecuted = false;
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
		ComponentCommand(unsigned int id, vega::ComponentIdentity component, bool adding)
			: mID{ id }, mComponent{ std::move(component) }, mAdding{ adding } {}
	private:
		bool Restore(EditorContext&, const nlohmann::json&);
		unsigned int mID;
		std::string mGuid;
		vega::ComponentIdentity mComponent;
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
		RemoveComponentCommand(unsigned int id, vega::ComponentIdentity component) : ComponentCommand{ id, std::move(component), false } {}
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

	// Whole-entity edits keep GUID-based history valid across prefab refreshes.
	bool RestoreSceneEntity(EditorContext&, const nlohmann::json&);
	class ReplaceEntities final : public EditorCommand
	{
	public:
		ReplaceEntities(nlohmann::json before, nlohmann::json after, std::string name)
			: mBefore(std::move(before)), mAfter(std::move(after)), mName{ std::move(name) } {}
		bool Execute(EditorContext& context) override;
		void Undo(EditorContext& context) override;
		std::string Name() const override { return mName; }
	private:
		nlohmann::json mBefore, mAfter;
		std::string mName;
	};

	bool CreateSceneEntity(EditorContext&, CommandHistory&, const std::string& type);
	bool DuplicateSceneEntity(EditorContext&, CommandHistory&);
	bool RenameSceneEntity(EditorContext&, CommandHistory&, const std::string& name, std::uint64_t group = 0);
	struct SelectionTransform
	{
		unsigned int id;
		sf::Vector2f location;
		float rotation;
	};
	std::vector<SelectionTransform> CaptureSelectionTransform(const EditorSelection&);
	bool TransformSelection(EditorContext&, CommandHistory&, const std::vector<SelectionTransform>&,
		sf::Vector2f pivot, sf::Vector2f delta, std::optional<float> angle = {}, std::uint64_t group = 0);
	bool DeleteSelection(EditorContext&, CommandHistory&);
	bool CopySelection(EditorContext&);
	bool PasteSelection(EditorContext&, CommandHistory&, sf::Vector2f centre);
	void SelectRectangle(EditorContext&, sf::Vector2f first, sf::Vector2f last, bool add = false, bool remove = false);

	struct CommonProperty
	{
		std::string section;
		const vega::reflect::PropertyInfo* property;
		std::vector<std::optional<vega::ComponentIdentity>> targets;
		vega::reflect::PropertyValue value;
		bool mixed = false;
	};
	// Values/descriptors only; never JSON or object pointers. Rebuilt after a scene/selection edit.
	class MultiInspectorCache
	{
	public:
		const std::vector<CommonProperty>& Get(EditorContext&);
	private:
		std::optional<std::uint64_t> mSceneRevision, mSelectionRevision;
		std::optional<unsigned int> mWorld;
		std::vector<CommonProperty> mProperties;
	};
	bool SetCommonProperty(EditorContext&, CommandHistory&, const CommonProperty&,
		const vega::reflect::PropertyValue&, std::uint64_t group = 0);

}
