#include "editor/EditorCommands.h"
#include <framework/World.h>
#include <framework/scene/SceneEntityComponent.h>
#include <cstdlib>
#include <typeinfo>
#ifdef __GNUG__
#include <cxxabi.h>
#endif

namespace vega
{
	void EditorContext::ApplyCamera()
	{
		if (world && camera && world->GetViewOverride()) world->SetViewOverride(camera->GetView());
	}

	bool EditorContext::SetEditorCamera(bool enabled)
	{
		if (!world || !camera) return false;
		if (world->IsEditMode()) enabled = true;
		if (enabled)
		{
			if (!world->GetViewOverride()) camera->InitFrom(world->GetWorldView());
			world->SetViewOverride(camera->GetView());
		}
		else world->SetViewOverride(std::nullopt);
		return true;
	}

	void EditorContext::SyncCamera()
	{
		if (!world || !camera) return;
		const bool paused = world->IsPaused();
		if (!cameraPaused || *cameraPaused != paused) SetEditorCamera(paused);
		cameraPaused = paused;
		ApplyCamera();
	}

	bool EditorContext::FocusSelection()
	{
		const auto actor = selection.Get();
		if (!actor || actor->GetWorld() != world || !SetEditorCamera(true)) return false;
		camera->Focus(*selection.Bounds());
		ApplyCamera();
		return true;
	}

	std::string ActorTypeName(const vega::Actor& actor)
	{
		std::string name = typeid(actor).name();
#ifdef __GNUG__
		int status = 0;
		std::unique_ptr<char, decltype(&std::free)> demangled{
			abi::__cxa_demangle(name.c_str(), nullptr, nullptr, &status), &std::free };
		if (status == 0 && demangled) name = demangled.get();
#endif
		// MSVC RTTI is demangled but includes the class/struct keyword.
		if (name.compare(0, 6, "class ") == 0) name.erase(0, 6);
		if (name.compare(0, 7, "struct ") == 0) name.erase(0, 7);
		const auto scope = name.rfind("::");
		if (scope != std::string::npos) name.erase(0, scope + 2);
		return name;
	}

	void EditorContext::RefreshActorCache() const
	{
		if (!world)
		{
			actorsByID.clear();
			actorsByGuid.clear();
			cachedWorldID.reset();
			return;
		}
		const auto count = world->GetActorCountIncludingPending();
		if (cachedWorldID == world->GetUniqueID() && cachedActorCount == count) return;
		actorsByID.clear();
		actorsByGuid.clear();
		cachedWorldID = world->GetUniqueID();
		cachedActorCount = count;
		for (const auto& weak : world->GetActorsByTypeIncludingPending<vega::Actor>())
		{
			if (const auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy() && actor->GetWorld() == world)
			{
				actorsByID.emplace(actor->GetUniqueID(), actor);
				if (const auto* identity = actor->FindComponent<vega::SceneEntityComponent>())
					actorsByGuid.emplace(identity->guid, actor);
			}
		}
	}

	std::shared_ptr<vega::Actor> EditorContext::Resolve(unsigned int id) const
	{
		RefreshActorCache();
		auto entry = actorsByID.find(id);
		// A remove/spawn pair can leave the count unchanged between lookups.
		if (entry == actorsByID.end() && world)
		{
			ResetActorCache();
			RefreshActorCache();
			entry = actorsByID.find(id);
		}
		const auto actor = entry != actorsByID.end() ? entry->second.lock() : nullptr;
		return actor && actor->GetWorld() == world && !actor->GetIsPendingDestroy() ? actor : nullptr;
	}

	std::shared_ptr<vega::Actor> EditorContext::ResolveGuid(const std::string& guid) const
	{
		if (!world || guid.empty()) return {};
		RefreshActorCache();
		const auto find = [&]() -> std::shared_ptr<vega::Actor>
		{
			const auto entry = actorsByGuid.find(guid);
			const auto actor = entry != actorsByGuid.end() ? entry->second.lock() : nullptr;
			if (!actor || actor->GetWorld() != world || actor->GetIsPendingDestroy()) return {};
			const auto* identity = actor->FindComponent<vega::SceneEntityComponent>();
			return identity && identity->guid == guid ? actor : nullptr;
		};
		if (auto actor = find()) return actor;
		// Identity components can be attached after spawn, including before the next frame.
		ResetActorCache();
		RefreshActorCache();
		return find();
	}

	std::shared_ptr<vega::Actor> EditorContext::Resolve(unsigned int id, std::string& guid) const
	{
		// Once captured, GUID is authoritative: a missing entity must not hit an unrelated runtime ID.
		if (!guid.empty()) return ResolveGuid(guid);
		const auto actor = Resolve(id);
		if (const auto* identity = actor ? actor->FindComponent<vega::SceneEntityComponent>() : nullptr) guid = identity->guid;
		return actor;
	}

}
