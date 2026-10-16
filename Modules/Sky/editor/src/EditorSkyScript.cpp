#include "editor/EditorSkyEditor.h"
#include <framework/World.h>
#include <framework/scene/SceneSettings.h>
#include <sstream>
#include <imgui.h>
#include <cmath>

namespace vega
{
	bool EditorSkyEditor::Execute(const EditorScriptCommand& command, EditorContext& scene, std::string& detail)
	{
		if (!mPipeline || !mDocument || mSuspended) { detail = "Sky pipeline unavailable in this mode"; return false; }
		bool success = false;
		try
		{
			using Op = ScriptOperation;
			if (command.operation == Op::SkyOpen)
			{
				mSceneSkyIndex = mPipeline->SceneSelection(scene);
				success = Open(command.text, true);
				if (success) mFocusRequested = true;
			}
			else if (!mDocument->document.IsOpen() && command.operation != Op::CaptureSky) detail = "sky-open must run first";
			else if (command.operation == Op::SkySet) success = mDocument->Set(command.text, Json::parse(command.values.at(0)));
			else if (command.operation == Op::SkyDrag)
			{
				mGeometry = mPipeline->Geometry(mOutput);
				success = Drag(command.text, {command.location.x / mOutput.x, command.location.y / mOutput.y});
			}
			else if (command.operation == Op::SkySave) success = mDocument->Save();
			else if (command.operation == Op::ExpectSky)
			{
				const auto value = mDocument->Value(command.text); const auto expected = Json::parse(command.values.at(0));
				const auto equals = [&](const auto& self, const Json& a, const Json& b) -> bool
				{
					if (a.is_number() && b.is_number()) return std::abs(a.get<double>() - b.get<double>()) <= 1e-5;
					if (a.is_array() && b.is_array()) { if (a.size() != b.size()) return false; for (std::size_t i = 0; i < a.size(); ++i) if (!self(self, a[i], b[i])) return false; return true; }
					return a == b;
				};
				success = equals(equals, value, expected); detail = "sky " + command.text + " = " + value.dump();
			}
			else if (command.operation == Op::CaptureSky && scene.world)
			{
				if (mDocument->document.IsOpen()) ApplyPreview(true);
				else if (scene.sceneSettings)
				{
					const auto stop = mPipeline->SceneSelection(scene);
					mSkyIndex = std::min(stop, mPipeline->Skies().size() - 1);
				}
				const std::filesystem::path path{command.text};
				if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
				sf::RenderTexture target;
				success = RenderSky(target, mSkyIndex, *scene.world, {960, 540}) && target.getTexture().copyToImage().saveToFile(path);
				std::ostringstream timing;
				timing << "capture " << command.text << "; parse=" << mDocument->parseMs << "ms apply=" << mApplyMs << "ms render=" << mRenderMs << "ms";
				detail = timing.str();
			}
			else if (command.operation == Op::Undo) success = mDocument->history.Undo();
			else if (command.operation == Op::Redo) success = mDocument->history.Redo();
			if (success)
			{
				if (mDocument->document.IsOpen()) { mDocument->Refresh(); CacheRows(); }
				if (detail.empty()) detail = "sky " + mDocument->selected;
			}
			else if (detail.empty()) detail = mDocument->error;
		}
		catch (const std::exception& error) { detail = error.what(); }
		return success;
	}
}
