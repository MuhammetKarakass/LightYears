#include "editor/JsonCommands.h"

namespace vega
{
	ReplaceJsonCommand::ReplaceJsonCommand(JsonDocument& document, Json after, std::string name, std::uint64_t group)
		: mDocument{document}, mID{document.ID()}, mGroup{group}, mBefore(document.Data()), mAfter(std::move(after)), mName{std::move(name)} {}
	bool ReplaceJsonCommand::IsValid(const EditorContext&) const { return mDocument.IsOpen() && mDocument.ID() == mID; }
	bool ReplaceJsonCommand::Execute(EditorContext& context) { return IsValid(context) && mDocument.Replace(mAfter); }
	void ReplaceJsonCommand::Undo(EditorContext& context) { if (IsValid(context)) mDocument.Replace(mBefore); }
	bool ReplaceJsonCommand::MergeWith(const EditorCommand& command)
	{
		const auto* next = dynamic_cast<const ReplaceJsonCommand*>(&command);
		if (!next || !mGroup || mGroup != next->mGroup || &mDocument != &next->mDocument || mID != next->mID || mName != next->mName) return false;
		mAfter = next->mAfter;
		return true;
	}
	bool SetJsonValueCommand::IsValid(const EditorContext& context) const
	{
		return context.document && context.document->IsOpen() && context.document->ID() == mID;
	}

	bool SetJsonValueCommand::Execute(EditorContext& context)
	{
		return IsValid(context) && context.document->Set(mPointer, mAfter);
	}

	void SetJsonValueCommand::Undo(EditorContext& context)
	{
		if (context.document && context.document->ID() == mID) context.document->Set(mPointer, mBefore);
	}

	bool SetJsonValueCommand::MergeWith(const EditorCommand& command)
	{
		const auto* next = dynamic_cast<const SetJsonValueCommand*>(&command);
		if (!next || !mGroup || mGroup != next->mGroup || mID != next->mID || mPointer != next->mPointer) return false;
		mAfter = next->mAfter;
		return true;
	}
}
