#pragma once

#include "editor/EditorSelection.h"
#include "editor/EditorCamera.h"
#include "editor/GizmoMath.h"
#include <framework/Actor.h>
#include <framework/reflection/Reflection.h>
#include <cstdint>
#include <string>
#include <variant>
#include <nlohmann/json.hpp>
#include <functional>

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
		std::optional<std::size_t> componentIndex;
	};
	ReflectedTarget ResolvePropertyTarget(ly::Actor& actor, std::optional<std::size_t> componentIndex);
	ReflectedTarget FindPropertyTarget(ly::Actor& actor, const std::string& targetName);

	class SetScriptPropertyCommand final : public EditorCommand
	{
	public:
		SetScriptPropertyCommand(unsigned int id, std::size_t componentIndex, std::string property,
			ly::reflect::PropertyValue value, std::uint64_t group = 0);
		bool Execute(EditorContext&) override;
		void Undo(EditorContext&) override;
		std::string Name() const override { return "Set script " + mProperty; }
		bool MergeWith(const EditorCommand&) override;
	private:
		unsigned int mID;
		std::string mGuid;
		std::size_t mComponentIndex;
		std::string mProperty;
		ly::reflect::PropertyValue mAfter;
		std::optional<ly::reflect::PropertyValue> mBeforeOverride;
		std::uint64_t mMergeGroup;
		bool mExecuted = false;
	};

	class SetPropertyCommand final : public EditorCommand
	{
	public:
		SetPropertyCommand(unsigned int id, std::optional<std::size_t> componentIndex,
			std::string property, ly::reflect::PropertyValue after, std::uint64_t group = 0);
		bool Execute(EditorContext&) override;
		void Undo(EditorContext&) override;
		std::string Name() const override;
		bool MergeWith(const EditorCommand&) override;
	private:
		unsigned int mID;
		std::string mGuid;
		std::optional<std::size_t> mComponentIndex;
		std::string mProperty;
		ly::reflect::PropertyValue mBefore;
		ly::reflect::PropertyValue mAfter;
		std::uint64_t mMergeGroup;
		bool mExecuted = false;
	};

	class ActorPropertyCommand : public EditorCommand
	{
	public:
		bool Execute(EditorContext&) override;
		void Undo(EditorContext&) override;
		std::string Name() const override;
		bool MergeWith(const EditorCommand&) override;

	protected:
		enum class Property { Location, Rotation, Layer, Enabled };
		using Value = std::variant<sf::Vector2f, float, ly::RenderLayer, bool>;
		ActorPropertyCommand(unsigned int id, Property property, Value value, std::uint64_t group = 0);

	private:
		void Apply(ly::Actor& actor, const Value& value) const;
		unsigned int mID;
		std::string mGuid;
		Property mProperty;
		Value mBefore;
		Value mAfter;
		std::uint64_t mMergeGroup;
		bool mExecuted = false;
		std::string mActorName;
	};

	class SetActorLocation final : public ActorPropertyCommand
	{
	public:
		SetActorLocation(unsigned int id, sf::Vector2f value, std::uint64_t group = 0)
			: ActorPropertyCommand(id, Property::Location, value, group) {}
	};
	class SetActorRotation final : public ActorPropertyCommand
	{
	public:
		SetActorRotation(unsigned int id, float value, std::uint64_t group = 0)
			: ActorPropertyCommand(id, Property::Rotation, value, group) {}
	};
	class SetRenderLayer final : public ActorPropertyCommand
	{
	public:
		SetRenderLayer(unsigned int id, ly::RenderLayer value)
			: ActorPropertyCommand(id, Property::Layer, value) {}
	};
	class SetRenderEnabled final : public ActorPropertyCommand
	{
	public:
		SetRenderEnabled(unsigned int id, bool value)
			: ActorPropertyCommand(id, Property::Enabled, value) {}
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
