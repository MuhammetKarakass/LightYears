#include "editor/EditorCommands.h"
#include <framework/World.h>
#include <typeinfo>
#include <algorithm>

namespace vega
{
	CommandHistory::CommandHistory(EditorContext& context, std::size_t maxDepth)
		: mContext{ context }, mMaxDepth{ maxDepth } {}

	bool CommandHistory::Execute(std::unique_ptr<EditorCommand> command)
	{
		PruneInvalidCommands();
		if (!command || mContext.transitioning || (mContext.playMode && command->IsUndoable() && command->ChangesScene())) return false;
		if (!command->Execute(mContext))
		{
			ly::debug::Log::Write(ly::debug::LogLevel::Error, ly::debug::LogChannel::Editor, LY_SOURCE_LOCATION,
				command->Name() + ": " + (!mContext.propertyError.empty() ? mContext.propertyError : mContext.sceneError));
			return false;
		}
		if (command->ChangesScene()) mContext.SceneChanged();
		const bool hadRedo = CanRedo();
		mRedo.clear();
		if (!command->IsUndoable() || mMaxDepth == 0)
		{
			mMergeAllowed = false;
			return true;
		}
		// Never merge across an undo branch: its initial value belongs to a different edit.
		if (mMergeAllowed && !hadRedo && !mUndo.empty() && mUndo.back()->MergeWith(*command)) return true;
		mMergeAllowed = true;
		mUndo.push_back(std::move(command));
		if (mUndo.size() > mMaxDepth) mUndo.erase(mUndo.begin());
		return true;
	}

	bool CommandHistory::Undo()
	{
		PruneInvalidCommands();
		if (mContext.playMode || mContext.transitioning || !CanUndo()) return false;
		mMergeAllowed = false;
		if (!mUndo.back()->TryUndo(mContext)) return false;
		if (mUndo.back()->ChangesScene()) mContext.SceneChanged();
		mRedo.push_back(std::move(mUndo.back()));
		mUndo.pop_back();
		return true;
	}

	bool CommandHistory::Redo()
	{
		PruneInvalidCommands();
		if (mContext.playMode || mContext.transitioning || !CanRedo() || !mRedo.back()->Execute(mContext)) return false;
		if (mRedo.back()->ChangesScene()) mContext.SceneChanged();
		mMergeAllowed = false;
		mUndo.push_back(std::move(mRedo.back()));
		mRedo.pop_back();
		return true;
	}

	std::string CommandHistory::UndoName() const { return CanUndo() ? mUndo.back()->Name() : ""; }
	std::string CommandHistory::RedoName() const { return CanRedo() ? mRedo.back()->Name() : ""; }
	void CommandHistory::PruneInvalidCommands()
	{
		// Reload/close invalidates JSON history without discarding unrelated actor edits.
		const auto prune = [&](auto& stack)
		{
			const auto size = stack.size();
			stack.erase(std::remove_if(stack.begin(), stack.end(), [&](const auto& command)
			{
				return !command->IsValid(mContext);
			}), stack.end());
			if (stack.size() != size) mMergeAllowed = false;
		};
		prune(mUndo);
		prune(mRedo);
	}

	void CommandHistory::Clear()
	{
		mUndo.clear();
		mRedo.clear();
		mMergeAllowed = false;
	}

	bool SetPaused::Execute(EditorContext& context)
	{
		if (!context.world) return false;
		context.world->SetPaused(mPaused);
		context.SetEditorCamera(mPaused);
		context.cameraPaused = mPaused;
		return true;
	}

	bool CompoundCommand::Execute(EditorContext& context)
	{
		if (mCommands.empty() || context.playMode || context.transitioning) return false;
		const auto before = context.selection;
		EditorSelection created;
		std::size_t applied = 0;
		for (auto& command : mCommands)
		{
			if (!command->Execute(context))
			{
				const auto error = context.propertyError;
				while (applied > 0) mCommands[--applied]->Undo(context);
				context.selection.Restore(before);
				context.selection.Rebind([&](const std::string& guid) { return context.ResolveGuid(guid); });
				context.propertyError = error;
				return false;
			}
			++applied;
			if (mSelectCreated) created.Add(context.selection.Get());
		}
		if (!mExecuted)
		{
			mBefore = before;
			if (mSelectCreated) context.selection.Restore(created);
			mAfter = context.selection;
			mExecuted = true;
		}
		else context.selection.Restore(mAfter);
		context.selection.Rebind([&](const std::string& guid) { return context.ResolveGuid(guid); });
		return true;
	}

	void CompoundCommand::Undo(EditorContext& context)
	{
		for (auto it = mCommands.rbegin(); it != mCommands.rend(); ++it) (*it)->Undo(context);
		context.selection.Restore(mBefore);
		context.selection.Rebind([&](const std::string& guid) { return context.ResolveGuid(guid); });
	}

	bool CompoundCommand::MergeWith(const EditorCommand& command)
	{
		const auto* next = dynamic_cast<const CompoundCommand*>(&command);
		if (!next || !mGroup || next->mGroup != mGroup || next->mName != mName || next->mCommands.size() != mCommands.size()) return false;
		for (std::size_t i = 0; i < mCommands.size(); ++i)
			if (mCommands[i]->Name() != next->mCommands[i]->Name() || typeid(*mCommands[i]) != typeid(*next->mCommands[i])) return false;
		for (std::size_t i = 0; i < mCommands.size(); ++i)
			if (!mCommands[i]->MergeWith(*next->mCommands[i])) return false;
		mAfter = next->mAfter;
		return true;
	}

	bool CompoundCommand::IsValid(const EditorContext& context) const
	{
		return std::all_of(mCommands.begin(), mCommands.end(), [&](const auto& command) { return command->IsValid(context); });
	}

}
