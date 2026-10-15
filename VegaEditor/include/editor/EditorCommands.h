#pragma once

#include "editor/EditorSelection.h"
#include <framework/Actor.h>
#include <cstdint>
#include <string>
#include <variant>

namespace vega
{
	struct EditorContext
	{
		ly::World* world = nullptr;
		EditorSelection& selection;
		std::shared_ptr<ly::Actor> Resolve(unsigned int id) const;
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
		std::string UndoName() const;
		std::string RedoName() const;
		void Clear();
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
		void Undo(EditorContext&) override {}
		std::string Name() const override { return "Destroy actor #" + std::to_string(mID); }
		bool IsUndoable() const override { return false; }
	private:
		unsigned int mID;
	};
}
