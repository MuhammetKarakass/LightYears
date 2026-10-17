#include "editor/EditorAssetField.h"
#include "editor/EditorCommands.h"
#include "editor/EditorSelection.h"
#include <framework/Actor.h>
#include <framework/EngineContext.h>
#include <framework/World.h>
#include <framework/reflection/Reflection.h>
#include <framework/scene/SceneEntityComponent.h>
#include <fstream>
#include <iostream>

namespace
{
	int failures = 0;
	void Check(bool value, const char* message) { if (!value) { ++failures; std::cerr << message << '\n'; } }
	void Write(const std::filesystem::path& path, const std::string& text)
	{
		std::filesystem::create_directories(path.parent_path());
		std::ofstream{ path, std::ios::binary } << text;
	}

	class AssetProbeActor : public ly::Actor
	{
	public:
		explicit AssetProbeActor(ly::World* world) : Actor{ world } {}
		std::string icon;
	};
}

int main()
{
	ly::EngineContext engineContext;
	ly::EngineContext::Scope contextScope{ engineContext };
	using namespace vega;
	using ly::reflect::AssetKind;
	const auto root = std::filesystem::path{ VEGA_ASSET_FIELD_TEST_ROOT };
	if (!root.is_absolute() || root.filename() != "asset-field-test") return 1;
	std::filesystem::remove_all(root);
	Write(root / "textures/icon.png", "image");
	Write(root / "textures/nested/other.PNG", "image");
	Write(root / "sounds/click.wav", "sound");
	Write(root / "scripts/start.lua", "return {}\n");
	Write(root / ".vega/Trash/hidden.png", "hidden");

	// Listing is what the picker offers: accepted kind only, sorted, recursive, hidden folders skipped.
	Check((ListAssetChoices(root, AssetKind::Texture) == std::vector<std::string>{ "textures/icon.png", "textures/nested/other.PNG" }),
		"Texture choices must be sorted, recursive and skip hidden folders");
	Check((ListAssetChoices(root, AssetKind::Script) == std::vector<std::string>{ "scripts/start.lua" }), "Script choices must list only .lua files");
	Check(ListAssetChoices({}, AssetKind::Texture).empty(), "A missing asset root must offer no choices");

	ly::reflect::RegisterEngineTypes();
	ly::reflect::Class<AssetProbeActor, ly::Actor>("AssetFieldProbe").Spawnable()
		.Property("Icon", [](const AssetProbeActor& a) { return a.icon; }, [](AssetProbeActor& a, const std::string& value) { a.icon = value; })
		.Asset(AssetKind::Texture)
		.Property("Preview", [](const AssetProbeActor& a) { return a.icon; }).Asset(AssetKind::Texture);

	ly::World world{ nullptr };
	auto actor = world.SpawnActor<AssetProbeActor>().lock();
	actor->AddComponent<ly::SceneEntityComponent>("probe", "Probe");
	EditorSelection selection;
	EditorContext context{ &world, selection };
	CommandHistory history{ context };
	const auto target = ResolvePropertyTarget(*actor, {});
	const auto* icon = target.type ? target.type->FindProperty("Icon") : nullptr;
	const auto* preview = target.type ? target.type->FindProperty("Preview") : nullptr;
	Check(icon && icon->assetKind == AssetKind::Texture && icon->kind == ly::reflect::PropertyKind::String, "Probe must expose a Texture asset property");
	if (!icon || !preview) return 1;

	// An accepted drop assigns through the undo history, and undo and redo restore the value.
	std::string error;
	Check(AssignAssetProperty(context, history, *actor, target, *icon, root, "textures/icon.png", error) && actor->icon == "textures/icon.png",
		"An accepted texture must be assigned");
	Check(history.UndoCount() == 1, "An accepted asset assignment must enter the undo history");
	Check(history.Undo() && actor->icon.empty(), "Asset assignment must undo to the previous value");
	Check(history.Redo() && actor->icon == "textures/icon.png", "Asset assignment must redo");

	// Refused drops leave the value and history untouched and say why.
	const auto depth = history.UndoCount();
	struct Refusal { const char* path; const char* reason; };
	const Refusal refusals[] = {
		{ "sounds/click.wav", "Texture" }, { "scripts/start.lua", "Texture" }, { "textures", "Texture" }, { "textures/missing.png", "Not a file" },
		{ "../outside.png", "Not a file" }, { "C:/outside.png", "Not a file" },
	};
	for (const auto& refusal : refusals)
	{
		error.clear();
		const bool accepted = AssignAssetProperty(context, history, *actor, target, *icon, root, refusal.path, error);
		Check(!accepted && actor->icon == "textures/icon.png" && history.UndoCount() == depth, (std::string{ "Refused asset must leave the property unchanged: " } + refusal.path).c_str());
		Check(error.find(refusal.reason) != std::string::npos, (std::string{ "Refusal must explain the failure: " } + refusal.path).c_str());
	}

	// Read-only asset properties show the path in the Inspector and can never be written.
	error.clear();
	Check(!AssignAssetProperty(context, history, *actor, target, *preview, root, "textures/icon.png", error) && error.find("read-only") != std::string::npos,
		"A read-only asset property must refuse assignment");

	if (failures) return 1;
	std::cout << "Editor asset field tests passed\n";
	return 0;
}
