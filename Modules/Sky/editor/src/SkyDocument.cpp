#include "editor/SkyDocument.h"
#include "editor/ContentPath.h"
#include "editor/SkyHandle.h"
#include <algorithm>

namespace vega
{
	namespace
	{
		Json Merge(Json base, const Json& overrides)
		{
			if (!overrides.is_object()) return base;
			for (auto it = overrides.begin(); it != overrides.end(); ++it)
			{
				if (it.key() == "use") continue;
				if (it->is_object()) base[it.key()] = Merge(base.value(it.key(), Json::object()), it.value());
				else if ((it.key() == "objects" || it.key() == "nebula") && base.contains(it.key()) && base[it.key()].is_array() && it->is_array())
					for (const auto& item : it.value()) base[it.key()].push_back(item);
				else base[it.key()] = it.value();
			}
			return base;
		}
		void Erase(Json& root, const std::string& pointer)
		{
			const Json::json_pointer p{pointer};
			if (root.contains(p))
			{
				auto& parent = root.at(p.parent_pointer());
				if (parent.is_object()) parent.erase(p.back());
				else parent.erase(parent.begin() + std::stoul(p.back()));
			}
		}
		void PruneEmpty(Json& root, const Json& saved, Json::json_pointer pointer)
		{
			while (pointer.to_string() != "/stops" && pointer.to_string() != "/skies" && pointer.to_string() != "/sky" && pointer.to_string() != "/presets" && !pointer.empty())
			{
				if (!root.contains(pointer) || !root.at(pointer).is_object() || !root.at(pointer).empty()) break;
				if (saved.contains(pointer) && saved.at(pointer).is_object() && saved.at(pointer).empty()) break;
				Erase(root, pointer.to_string()); pointer = pointer.parent_pointer();
			}
		}
	}

	SkyDocument::SkyDocument(const std::filesystem::path& root, std::filesystem::path file) : document{root}, mFile{std::move(file)} { context.document = &document; }
	std::string SkyDocument::Base() const { return preset ? "/presets/" + JsonDocument::EscapePointer(selected) : mSingle ? "/sky" : "/" + mCollection + "/" + JsonDocument::EscapePointer(selected); }
	std::string SkyDocument::Normalize(const std::string& path)
	{
		const auto pointer = ContentPointer(path);
		std::string result = pointer.substr(1);
		std::replace(result.begin(), result.end(), '/', '.');
		return result;
	}
	bool SkyDocument::Open()
	{
		if (document.Dirty()) { error = "Save or discard sky changes before opening"; return false; }
		if (!document.Open(mFile)) { error = document.Error(); return false; }
		if (document.ReadOnly()) { error = document.ReadOnlyReason(); return false; }
		mSingle = document.Data().contains("sky");
		mCollection = document.Data().contains("skies") ? "skies" : "stops";
		if (!Refresh()) return false;
		mSaved = document.Data(); mSavedParsed = mParsed;
		history.Clear();
		return true;
	}
	bool SkyDocument::Reopen()
	{
		if (!document.Revert()) { error = document.Error(); return false; }
		history.Clear();
		if (!Refresh()) return false;
		mSaved = document.Data(); mSavedParsed = mParsed;
		return true;
	}
	bool SkyDocument::Select(const std::string& id, bool isPreset)
	{
		if (id.empty() || (mSingle && !isPreset && id != document.Data().value("id", std::string{}))) return false;
		if (isPreset && (!document.Data().contains("presets") || !document.Data()["presets"].contains(id))) { error = "Unknown preset"; return false; }
		selected = id; preset = isPreset; error.clear(); return true;
	}
	const Json* SkyDocument::Authored(const std::string& path) const
	{
		if (path.empty()) return document.Get(Base());
		std::string pointer = ContentPointer(path);
		const auto* stop = document.Get(Base());
		if (!preset && stop && stop->is_object() && stop->contains("use"))
		{
			for (const std::string array : {"objects", "nebula"})
			{
				const auto prefix = "/" + array + "/";
				if (pointer.find(prefix) != 0) continue;
				const auto end = pointer.find('/', prefix.size());
				const auto index = std::stoul(pointer.substr(prefix.size(), end - prefix.size()));
				const auto* inherited = document.Get("/presets/" + JsonDocument::EscapePointer(stop->at("use").get<std::string>()) + '/' + array);
				const auto count = inherited && inherited->is_array() ? inherited->size() : 0;
				if (index < count) return nullptr;
				pointer.replace(prefix.size(), (end == std::string::npos ? pointer.size() : end) - prefix.size(), std::to_string(index - count));
			}
		}
		return document.Get(Base() + pointer);
	}
	Json SkyDocument::Effective() const
	{
		const auto* authored = Authored();
		if (!authored) return Json::object();
		std::string use;
		if (authored->is_string()) use = authored->get<std::string>();
		else if (authored->is_object()) use = authored->value("use", std::string{});
		Json base = Json::object();
		const auto* inherited = document.Get("/presets/" + JsonDocument::EscapePointer(use));
		if (!preset && !use.empty() && inherited) base = *inherited;
		return Merge(base, *authored);
	}
	Json SkyDocument::Value(const std::string& path) const
	{
		const auto value = Effective();
		const Json::json_pointer p{ContentPointer(path)};
		if (value.contains(p)) return value.at(p);
		const auto normalized = Normalize(path);
		const auto* parsed = preset ? mParsed.FindPreset(selected) : mParsed.Find(selected);
		if (parsed)
		{
			if (normalized == "band.tint") return parsed->band.style.tint;
			if (normalized == "band.bulge") return parsed->band.style.bulge;
			if (normalized == "band.anticenter") return parsed->band.style.anticenter;
			if (normalized.find("objects.") == 0 && normalized.size() > 5 && normalized.substr(normalized.size() - 5) == ".seed")
			{
				const auto i = std::stoul(normalized.substr(8));
				if (i < parsed->objects.size()) return parsed->objects[i].seed;
			}
		}
		const auto* field = sky::FindSkyField(normalized);
		return field ? Json(field->defaultValue) : Json{};
	}
	Json& SkyDocument::Editable(Json& root) const
	{
		auto& stop = root[Json::json_pointer{Base()}];
		if (stop.is_string()) stop = Json{{"use", stop.get<std::string>()}};
		if (stop.is_null()) stop = Json::object();
		return stop;
	}
	bool SkyDocument::Commit(Json candidate, const std::string& name, std::uint64_t group)
	{
		if (candidate == document.Data()) { error.clear(); return true; }
		if (!Refresh()) return false;
		sky::DeepSkyCatalog parsed = mParsed;
		const auto start = std::chrono::steady_clock::now();
		const Json empty = Json::object();
		const auto& presets = candidate.contains("presets") ? candidate.at("presets") : empty;
		const auto& oldPresets = document.Data().contains("presets") ? document.Data().at("presets") : empty;
		if (!mSingle && presets == oldPresets)
		{
			// Most gestures change one stop. Converting/parsing the complete ordered document
			// on every mouse sample would make a drag's cost grow with the whole route.
			Json fragment{{"presets", presets}, {"stops", Json::object()}};
			std::vector<std::string> removed;
			const auto& stops = candidate.at(mCollection);
			const auto& oldStops = document.Data().at(mCollection);
			for (auto it = stops.begin(); it != stops.end(); ++it)
				if (!oldStops.contains(it.key()) || oldStops.at(it.key()) != it.value()) fragment["stops"][it.key()] = it.value();
			for (auto it = oldStops.begin(); it != oldStops.end(); ++it) if (!stops.contains(it.key())) removed.push_back(it.key());
			if (!parsed.ApplyPatch(nlohmann::json(fragment), removed, error)) return false;
		}
		else if (!parsed.Apply(nlohmann::json(candidate), error)) return false;
		parseMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
		if (!history.Execute(std::make_unique<ReplaceJsonCommand>(document, std::move(candidate), name, group))) { error = "Sky document is read-only"; return false; }
		mParsed = std::move(parsed); mParsedRevision = document.Revision(); error.clear(); return true;
	}
	bool SkyDocument::Refresh()
	{
		if (mParsedRevision == document.Revision()) return true;
		const auto start = std::chrono::steady_clock::now();
		if (!mParsed.Apply(nlohmann::json(document.Data()), error)) return false;
		parseMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
		mParsedRevision = document.Revision(); error.clear(); return true;
	}
	bool SkyDocument::Set(const std::string& path, const Json& value, std::uint64_t group)
	{
		try
		{
			const auto normalized = Normalize(path);
			const auto* field = sky::FindSkyField(normalized);
			if (normalized == "use" && !preset)
			{
				if (!value.is_string() || !document.Get("/presets/" + JsonDocument::EscapePointer(value.get<std::string>()))) { error = "Unknown preset"; return false; }
			}
			else if (!field) { error = "Unknown sky fields are read-only"; return false; }
			else if (!(error = sky::ValidateSkyField(*field, nlohmann::json(value))).empty()) return false;
			Json candidate = document.Data();
			auto& stop = Editable(candidate);
			// Preset arrays append in the runtime parser. Editing an inherited object materializes
			// the effective sky and detaches the preset, preventing a duplicate object at preview.
			if ((normalized.find("objects.") == 0 || normalized.find("nebula.") == 0) && stop.contains("use")) stop = Effective();
			if (normalized.find("objects.") == 0)
			{
				const auto end = normalized.find('.', 8);
				const auto index = std::stoul(normalized.substr(8, end - 8));
				if (!stop.contains("objects") || index >= stop["objects"].size()) { error = "Object index does not exist"; return false; }
			}
			stop[Json::json_pointer{ContentPointer(path)}] = value;
			if (normalized.find("orbit.ring.") == 0)
			{
				const auto effective = Merge(Effective(), stop);
				const auto& ring = effective["orbit"]["ring"];
				if (ring.value("outer", 2.3f) < ring.value("inner", 1.35f) + sky::SkyRingRadiusGap - 1e-6f) { error = "Ring outer must be at least inner + 0.05"; return false; }
			}
			return Commit(std::move(candidate), "Set sky " + normalized, group);
		}
		catch (const std::exception& e) { error = e.what(); return false; }
	}
	bool SkyDocument::Reset(const std::string& path)
	{
		const auto* field = sky::FindSkyField(Normalize(path));
		if (!field && path != "use") { error = "Unknown sky fields are read-only"; return false; }
		const bool stringUse = path == "use" && Authored() && Authored()->is_string();
		if (!Authored(path) && !stringUse) return true;
		if (field && field->path == "objects.*.kind") return Set(path, Json(field->defaultValue));
		Json candidate = document.Data();
		if (stringUse) candidate[Json::json_pointer{Base()}] = Json::object();
		if (Normalize(path).find("objects.") == 0 && Editable(candidate).contains("use")) Editable(candidate) = Effective();
		Erase(candidate, Base() + ContentPointer(path));
		PruneEmpty(candidate, mSaved, Json::json_pointer{Base() + ContentPointer(path)}.parent_pointer());
		return Commit(std::move(candidate), "Reset sky " + path);
	}
	bool SkyDocument::Block(const std::string& path, const Json& value, const std::string& name)
	{
		if (path != "band" && path != "planet" && path != "orbit" && path != "orbit.ring" && path != "hostStar" && path != "grade" && path != "asteroids" && path != "skyPaint" && path != "objects" && path.find("objects.") != 0) return false;
		Json candidate = document.Data(); auto& stop = Editable(candidate);
		if (path.find("objects") == 0 && stop.contains("use")) stop = Effective();
		stop[Json::json_pointer{ContentPointer(path)}] = value;
		return Commit(std::move(candidate), name);
	}
	bool SkyDocument::RemoveBlock(const std::string& path)
	{
		Json candidate = document.Data();
		if (path.find("objects.") == 0 && Editable(candidate).contains("use")) Editable(candidate) = Effective();
		Erase(candidate, Base() + ContentPointer(path));
		return Commit(std::move(candidate), "Remove sky " + path);
	}
	bool SkyDocument::DuplicateTo(const std::string& stop)
	{
		if (mSingle || stop.empty() || stop == selected) return false;
		Json candidate = document.Data(); candidate[mCollection][stop] = Authored() ? *Authored() : Json::object();
		return Commit(std::move(candidate), "Duplicate sky to " + stop);
	}
	bool SkyDocument::StorePreset(const std::string& name)
	{
		if (name.empty()) return false;
		Json candidate = document.Data(); candidate["presets"][name] = Effective();
		return Commit(std::move(candidate), "Store sky preset " + name);
	}
	bool SkyDocument::Save()
	{
		if (!Refresh() || !document.Save()) { if (error.empty()) error = document.Error(); return false; }
		mSaved = document.Data(); mSavedParsed = mParsed; error.clear(); return true;
	}
	bool SkyDocument::IsHandle(const std::string& handle)
	{
		return IsSkyHandle(handle);
	}
}
