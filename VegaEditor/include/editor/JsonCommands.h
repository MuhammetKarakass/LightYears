#pragma once
#include "editor/EditorCommands.h"
#include "editor/JsonDocument.h"

namespace vega
{
	class SetJsonValueCommand final : public EditorCommand
	{
	public:
		SetJsonValueCommand(std::uint64_t id, std::string pointer, Json before, Json after, std::uint64_t group = 0)
			: mID{ id }, mPointer{ std::move(pointer) }, mBefore(std::move(before)), mAfter(std::move(after)), mGroup{ group } {}
		bool Execute(EditorContext& context) override;
		void Undo(EditorContext& context) override;
		bool MergeWith(const EditorCommand& command) override;
		bool IsValid(const EditorContext& context) const override;
		bool ChangesScene() const override { return false; }
		std::string Name() const override { return "Set JSON " + mPointer; }
	private:
		std::uint64_t mID;
		std::string mPointer;
		Json mBefore;
		Json mAfter;
		std::uint64_t mGroup;
	};
}
