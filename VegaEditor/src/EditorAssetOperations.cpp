#include "editor/EditorApplication.h"
#include "editor/EditorPrefabs.h"
#include <framework/scene/PrefabLibrary.h>
#include <imgui.h>
#include <cstdio>

namespace vega
{
	namespace
	{
		class MoveAsset final : public EditorCommand
		{
		public:
			using Refresh = std::function<void(const std::filesystem::path&, const std::filesystem::path&)>;
			MoveAsset(AssetMovePlan plan, std::function<bool()> ready, Refresh refresh)
				: mPlan{ std::move(plan) }, mReady{ std::move(ready) }, mRefresh{ std::move(refresh) } {}
			bool Execute(EditorContext& context) override
			{
				if (!mReady() || !mPlan.Apply()) { context.sceneError = mPlan.Error().empty() ? "Save dirty documents and close the prefab before moving assets" : mPlan.Error(); return false; }
				mRefresh(mPlan.From(), mPlan.To());
				return true;
			}
			void Undo(EditorContext& context) override
			{
				TryUndo(context);
			}
			bool TryUndo(EditorContext& context) override
			{
				if (!mReady() || !mPlan.Apply(false)) { context.sceneError = "Cannot undo asset move: save dirty documents and check for external file changes. " + mPlan.Error(); return false; }
				mRefresh(mPlan.To(), mPlan.From());
				return true;
			}
			bool ChangesScene() const override { return false; }
			std::string Name() const override { return "Move asset and references"; }
		private:
			AssetMovePlan mPlan;
			std::function<bool()> mReady;
			Refresh mRefresh;
		};
		class DeleteAsset final : public EditorCommand
		{
		public:
			DeleteAsset(AssetMovePlan plan, std::filesystem::path source, std::filesystem::path trash,
				std::function<bool()> ready, std::function<void()> refresh)
				: mPlan{ std::move(plan) }, mSource{ std::move(source) }, mTrash{ std::move(trash) }, mReady{ std::move(ready) }, mRefresh{ std::move(refresh) } {}
			bool Execute(EditorContext& context) override
			{
				if (!mReady() || !mPlan.Current(true)) { context.sceneError = "Assets changed since delete preview"; return false; }
				try
				{
					if (!SafePaths() || std::filesystem::exists(mTrash)) throw std::runtime_error("Delete paths changed since preview");
					std::filesystem::rename(mSource, mTrash);
					mRefresh();
					return true;
				}
				catch (const std::exception& error) { context.sceneError = error.what(); return false; }
			}
			void Undo(EditorContext& context) override
			{
				TryUndo(context);
			}
			bool TryUndo(EditorContext& context) override
			{
				try
				{
					if (!mReady()) throw std::runtime_error("Save dirty documents before restoring an asset");
					if (!SafePaths() || std::filesystem::exists(mSource)) throw std::runtime_error("Restore paths changed or the original filename is occupied");
					std::filesystem::rename(mTrash, mSource);
					mRefresh();
					return true;
				}
				catch (const std::exception& error) { context.sceneError = error.what(); return false; }
			}
			bool ChangesScene() const override { return false; }
			std::string Name() const override { return "Delete asset (move to Trash)"; }
		private:
			bool SafePaths() const
			{
				JsonDocument resolver{ mPlan.Root() };
				std::filesystem::path source, trash;
				return resolver.Root() == mPlan.Root() && resolver.Resolve(mSource.lexically_relative(mPlan.Root()), source) && source == mSource &&
					resolver.Resolve(mTrash.lexically_relative(mPlan.Root()), trash) && trash == mTrash;
			}
			AssetMovePlan mPlan;
			std::filesystem::path mSource, mTrash;
			std::function<bool()> mReady;
			std::function<void()> mRefresh;
		};
	}

	bool EditorApplication::AssetsCanChange() const
	{
		return !mContext.playMode && !mContext.transitioning && !SceneDirty() && !mPrefabWorld &&
			!mDocument.Dirty() && !mContentAuthoring.Dirty() && !mExtensions.Dirty() && mAssetRecoveryError.empty();
	}

	void EditorApplication::RefreshAfterAssetMove(const std::filesystem::path& from, const std::filesystem::path& to)
	{
		const auto root = mDocument.Root();
		const auto document = mDocument.IsOpen() ? AssetMovePlan::Remap(mDocument.Path(), root / from, root / to) : std::filesystem::path{};
		mContentDirectory = AssetMovePlan::Remap(mContentDirectory, from, to);
		mSelectedAsset = AssetMovePlan::Remap(mSelectedAsset, from, to);
		mScenePath = AssetMovePlan::Remap(mScenePath, root / from, root / to);
		mProject.file = AssetMovePlan::Remap(mProject.file, root / from, root / to);
		GetEngineContext().Prefabs().Reload();
		if (!document.empty() && !mDocument.Open(document.lexically_relative(root))) mContentError = mDocument.Error();
		mContentAuthoring.ReloadIfLoaded(root);
		if (!mScenePath.empty()) OpenScene(mScenePath);
		try { if (std::filesystem::exists(mProject.file)) mProject.settings = ly::ProjectSettings::Load(mProject.file); }
		catch (const std::exception& error) { mProjectError = error.what(); }
		for (auto& [path, thumbnail] : mThumbnails) mRetiredThumbnails.push_back(std::move(thumbnail.texture));
		mThumbnails.clear();
		RefreshContent();
	}

	void EditorApplication::RequestAssetMove(const std::filesystem::path& from, const std::filesystem::path& to)
	{
		mContentError.clear();
		mAssetMove.reset();
		if (!AssetsCanChange()) { mContentError = "Save dirty documents, close the prefab and stop Play before changing assets"; return; }
		auto plan = std::make_unique<AssetMovePlan>(GetEngineContext());
		if (!plan->Prepare(mDocument.Root(), from, to, mProject.file)) { mContentError = plan->Error(); return; }
		mAssetMove = std::move(plan);
		mAssetPreview = true;
	}

	bool EditorApplication::ExecuteAssetCommand(const EditorScriptCommand& command, std::string& detail)
	{
		if (command.operation == "expect-asset")
		{
			std::filesystem::path path;
			const bool safe = mDocument.Resolve(std::filesystem::u8path(command.text), path);
			const bool exists = safe && std::filesystem::exists(path);
			detail = safe ? path.generic_u8string() : "Path escapes the asset root";
			return safe && exists == (command.values[0] == "true");
		}
		const auto from = std::filesystem::u8path(command.text);
		if (command.operation == "asset-delete")
		{
			AssetMovePlan plan{ GetEngineContext() };
			if (!plan.Prepare(mDocument.Root(), from, ".__vega_delete_preview__", mProject.file)) { detail = plan.Error(); return false; }
			const bool success = ExecuteAssetDelete(plan);
			detail = success ? "Asset moved to Trash" : mContentError;
			return success;
		}
		auto to = std::filesystem::u8path(command.values[0]);
		if (command.operation == "asset-rename")
		{
			if (to.has_parent_path() || to.has_root_name()) { detail = "Rename expects a filename"; return false; }
			to = from.parent_path() / to;
		}
		if (!AssetsCanChange()) { detail = "Save dirty documents, close the prefab and stop Play before changing assets"; return false; }
		AssetMovePlan plan{ GetEngineContext() };
		if (!plan.Prepare(mDocument.Root(), from, to, mProject.file)) { detail = plan.Error(); return false; }
		detail = std::to_string(plan.References().size()) + " references updated";
		return mHistory.Execute(std::make_unique<MoveAsset>(std::move(plan), [this]() { return AssetsCanChange(); },
			[this](const auto& previous, const auto& next) { RefreshAfterAssetMove(previous, next); }));
	}

	bool EditorApplication::ExecuteAssetDelete(const AssetMovePlan& plan)
	{
		const auto root = mDocument.Root(), source = root / plan.From();
		if (!AssetsCanChange()) { mContentError = "Save dirty documents and close the prefab before deleting assets"; return false; }
		if (AssetMovePlan::Contains(source, mScenePath) || AssetMovePlan::Contains(source, mProject.file)) { mContentError = "Cannot delete the open scene or project"; return false; }
		for (const auto& ref : plan.References()) if (!AssetMovePlan::Contains(source, ref.file))
		{ mContentError = "Cannot delete an asset referenced by " + ref.file.generic_u8string() + " " + ref.pointer; return false; }
		std::filesystem::path trash;
		const auto name = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
		if (!mDocument.Resolve(std::filesystem::path{ ".vega/Trash" } / name, trash)) { mContentError = "Trash escapes the asset root"; return false; }
		std::error_code error;
		std::filesystem::create_directories(trash, error);
		if (error) { mContentError = error.message(); return false; }
		if (mDocument.IsOpen() && AssetMovePlan::Contains(source, mDocument.Path())) mDocument.Close();
		return mHistory.Execute(std::make_unique<DeleteAsset>(plan, source, trash / source.filename(), [this]() { return AssetsCanChange(); }, [this]() { RefreshContent(); }));
	}

	void EditorApplication::AssetDropTarget(const std::filesystem::path& folder)
	{
		if (!mContext.playMode && ImGui::BeginDragDropTarget())
		{
			if (const auto* payload = ImGui::AcceptDragDropPayload("VEGA_ASSET"))
			{
				const auto source = std::filesystem::u8path(static_cast<const char*>(payload->Data));
				mAssetDelete = false;
				RequestAssetMove(source, folder / source.filename());
			}
			ImGui::EndDragDropTarget();
		}
	}

	void EditorApplication::AssetItemActions(const std::filesystem::path& relative, bool folder)
	{
		if (ImGui::IsItemClicked()) mSelectedAsset = relative;
		if (!mContext.playMode && ImGui::BeginDragDropSource())
		{
			const auto path = relative.generic_u8string();
			ImGui::SetDragDropPayload("VEGA_ASSET", path.c_str(), path.size() + 1);
			ImGui::TextUnformatted(path.c_str());
			ImGui::EndDragDropSource();
		}
		if (folder) AssetDropTarget(relative);
		const bool renameKey = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
			!ImGui::GetIO().WantTextInput && mSelectedAsset == relative && ImGui::IsKeyPressed(ImGuiKey_F2, false);
		bool rename = renameKey, move = false, remove = false;
		if (ImGui::BeginPopupContextItem("Asset actions"))
		{
			mSelectedAsset = relative;
			rename = ImGui::MenuItem("Rename...", "F2");
			move = ImGui::MenuItem("Move to folder...");
			remove = ImGui::MenuItem("Delete...");
			if (IsPrefabPath(relative) && ImGui::MenuItem("Create Variant...", nullptr, false, !mContext.playMode))
			{
				mVariantParent = relative.generic_u8string();
				std::snprintf(mPrefabName, sizeof(mPrefabName), "%s Variant", relative.stem().stem().u8string().c_str());
				mCreatePrefabDialog = true;
			}
			ImGui::EndPopup();
		}
		if (rename || move)
		{
			mAssetDialogSource = relative;
			mAssetRename = rename;
			mAssetDelete = false;
			std::snprintf(mAssetPathInput, sizeof(mAssetPathInput), "%s", (rename ? relative.filename() : relative.parent_path()).generic_u8string().c_str());
			mAssetPathDialog = true;
		}
		if (remove)
		{
			mAssetDelete = true;
			RequestAssetMove(relative, ".__vega_delete_preview__");
		}
	}

	void EditorApplication::BuildAssetDialogs()
	{
		if (mAssetPathDialog) { mAssetPathDialog = false; ImGui::OpenPopup("Asset destination"); }
		if (ImGui::BeginPopupModal("Asset destination", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextUnformatted(mAssetRename ? "New filename" : "Destination folder (asset-relative)");
			ImGui::InputText("##destination", mAssetPathInput, sizeof(mAssetPathInput));
			if (ImGui::Button("Preview"))
			{
				const auto input = std::filesystem::u8path(mAssetPathInput);
				if (mAssetRename && (input.has_parent_path() || input.has_root_name())) mContentError = "Rename expects a filename";
				else
				{
					RequestAssetMove(mAssetDialogSource, mAssetRename ? mAssetDialogSource.parent_path() / input : input / mAssetDialogSource.filename());
					if (mAssetMove) ImGui::CloseCurrentPopup();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
			if (!mContentError.empty()) ImGui::TextWrapped("%s", mContentError.c_str());
			ImGui::EndPopup();
		}
		if (mAssetPreview) { mAssetPreview = false; ImGui::OpenPopup("Review asset operation"); }
		ImGui::SetNextWindowSize({ 720.f, 420.f }, ImGuiCond_Appearing);
		if (!ImGui::BeginPopupModal("Review asset operation", nullptr)) return;
		if (mAssetMove)
		{
			const auto root = mDocument.Root();
			const auto& refs = mAssetMove->References();
			bool blocked = false;
			if (mAssetDelete)
			{
				blocked = AssetMovePlan::Contains(root / mAssetMove->From(), mScenePath) || AssetMovePlan::Contains(root / mAssetMove->From(), mProject.file);
				for (const auto& ref : refs) if (!AssetMovePlan::Contains(root / mAssetMove->From(), ref.file)) blocked = true;
			}
			ImGui::TextWrapped("%s %s%s%s", mAssetDelete ? "Delete" : "Move", mAssetMove->From().generic_u8string().c_str(),
				mAssetDelete ? "" : " -> ", mAssetDelete ? "" : mAssetMove->To().generic_u8string().c_str());
			ImGui::TextWrapped("%s", mAssetDelete ? "Deleted assets go to .vega/Trash and can be restored with Undo. Referenced assets and the open scene/project cannot be deleted." : "The following JSON references will change in one undo step:");
			ImGui::BeginChild("References", { 0.f, -90.f });
			for (const auto& ref : refs)
				ImGui::TextWrapped("%s %s\n%s -> %s", ref.file.lexically_relative(root).generic_u8string().c_str(), ref.pointer.c_str(), ref.before.c_str(), ref.after.c_str());
			if (refs.empty()) ImGui::TextDisabled("No JSON references found.");
			ImGui::EndChild();
			ImGui::BeginDisabled(blocked || !AssetsCanChange());
			if (ImGui::Button(mAssetDelete ? "Confirm delete" : "Move and update references"))
			{
				bool success = false;
				if (mAssetDelete) success = ExecuteAssetDelete(*mAssetMove);
				else success = mHistory.Execute(std::make_unique<MoveAsset>(*mAssetMove, [this]() { return AssetsCanChange(); }, [this](const auto& from, const auto& to) { RefreshAfterAssetMove(from, to); }));
				if (success) { mAssetMove.reset(); ImGui::CloseCurrentPopup(); }
				else mContentError = mContext.sceneError;
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) { mAssetMove.reset(); ImGui::CloseCurrentPopup(); }
			if (blocked) ImGui::TextWrapped("Delete refused: this asset is referenced or is the open scene/project.");
			if (!mContentError.empty()) ImGui::TextWrapped("%s", mContentError.c_str());
		}
		ImGui::EndPopup();
	}
}
