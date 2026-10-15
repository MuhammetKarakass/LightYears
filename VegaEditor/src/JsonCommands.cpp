#include "editor/JsonCommands.h"

namespace vega
{
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
