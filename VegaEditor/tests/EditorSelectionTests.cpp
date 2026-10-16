#include "framework/EngineContext.h"
#include "editor/EditorSelection.h"

#include <framework/Actor.h>
#include <framework/World.h>
#include <algorithm>
#include <iostream>

namespace
{
	int failures = 0;

	void Check(bool condition, const char* message)
	{
		if (!condition)
		{
			std::cerr << message << '\n';
			++failures;
		}
	}

	void TestSelection()
	{
		vega::EditorSelection selection;
		Check(!selection.Get(), "Selection must start empty");
		ly::World world{ nullptr };
		auto actor = world.SpawnActor<ly::Actor>().lock();
		world.TickInternal(0.f);
		Check(world.GetActorsByType<ly::Actor>().size() == 1, "Tick must promote the test actor to the live list");
		const long owners = actor.use_count();
		selection.Select(actor);
		Check(actor.use_count() == owners, "Selection must not own the actor");
		Check(selection.Get() == actor, "Select did not return the selected actor");
		selection.Clear();
		Check(!selection.Get(), "Clear did not empty selection");
		selection.Clear();
		selection.Select(actor);
		auto other = world.SpawnActor<ly::Actor>().lock();
		world.TickInternal(0.f);
		selection.Select(other);
		Check(selection.Get() == other, "Selecting another actor did not replace selection");
		selection.Select(actor);
		actor->Destroy();
		Check(actor->GetIsPendingDestroy(), "Destroy must mark the test actor pending destroy");
		Check(!selection.Get(), "A retained pending destroy actor must not be returned");
		selection.Select(actor);
		Check(!selection.Get(), "Selecting a pending destroy actor must not make it accessible");
		selection.Select(std::weak_ptr<ly::Actor>{});
		Check(!selection.Get(), "An empty weak pointer must clear the accessible selection");
	}

	void TestMultiple()
	{
		ly::World world{ nullptr };
		const auto a = world.SpawnActor<ly::Actor>().lock();
		const auto b = world.SpawnActor<ly::Actor>().lock();
		const auto c = world.SpawnActor<ly::Actor>().lock();
		vega::EditorSelection selection;
		const auto owners = a.use_count();
		selection.Add(a);
		selection.Add(b);
		selection.Add(c);
		Check(selection.Count() == 3 && selection.Get() == c, "Ordered selection must make the last actor primary");
		selection.Add(a);
		Check(selection.Count() == 3 && selection.GetAll()[0] == b && selection.Get() == a, "Adding an existing actor must preserve the set and change primary");
		Check(a.use_count() == owners, "Multiple selection must retain only weak pointers");
		selection.Toggle(a);
		Check(selection.Count() == 2 && selection.Get() == c, "Removing primary must promote the preceding actor");
		const auto revision = selection.Revision();
		selection.Remove(a);
		Check(selection.Revision() == revision, "Removing an absent actor must not invalidate the inspector cache");
		selection.Toggle(a);
		b->Destroy();
		Check(selection.Count() == 2 && !selection.Contains(b), "Destroyed secondary selections must be inaccessible");
		selection.Select(c);
		Check(selection.Count() == 1 && selection.Get() == c, "Legacy Select must replace the complete set");
	}

	void TestExpiry()
	{
		vega::EditorSelection selection;
		std::weak_ptr<ly::Actor> expired;
		{
			ly::World world{ nullptr };
			expired = world.SpawnActor<ly::Actor>();
			world.TickInternal(0.f);
			selection.Select(expired);
			Check(selection.Get() != nullptr, "Live actor was not selected before expiry");
		}
		Check(expired.expired(), "Selection kept the actor alive after its world expired");
		Check(!selection.Get(), "An expired selection must return nullptr");
	}

	void TestPicking()
	{
		using vega::PickBestActor;
		Check(!PickBestActor({ 5.f, 5.f }, {}), "An empty query must miss");
		std::vector<vega::ActorPickBounds> bounds{
			{ 9u, { { 0.f, 0.f }, { 100.f, 100.f } } },
			{ 4u, { { 2.f, 2.f }, { 10.f, 10.f } } },
			{ 2u, { { 20.f, 20.f }, { 1.f, 1.f } } }
		};
		Check(PickBestActor({ 5.f, 5.f }, bounds) == 4u, "Smallest containing bounds must win over larger or nearby bounds");
		Check(PickBestActor({ 20.5f, 20.5f }, bounds) == 2u, "A containing small actor was missed");
		Check(PickBestActor({ 50.f, 50.f }, bounds) == 9u, "A lone containing actor was missed");
		Check(!PickBestActor({ -1.f, -1.f }, bounds), "Non-containing query results must miss");
		Check(PickBestActor({ 2.f, 2.f }, bounds) == 4u, "Top-left bounds edge must be included");
		Check(PickBestActor({ 12.f, 12.f }, bounds) == 9u, "Bottom-right bounds edge must be excluded");
		bounds.push_back({ 1u, { { 2.f, 2.f }, { 10.f, 10.f } } });
		Check(PickBestActor({ 5.f, 5.f }, bounds) == 1u, "Equal-area hits must prefer the lower ID");
		std::reverse(bounds.begin(), bounds.end());
		Check(PickBestActor({ 5.f, 5.f }, bounds) == 1u, "Traversal order must not affect equal-area hits");
		Check(!PickBestActor({ 0.f, 0.f }, { { 3u, { {}, {} } } }), "Zero-size bounds must not contain a point");
		Check(PickBestActor({ -5.f, -5.f }, { { 0u, { { -10.f, -10.f }, { 10.f, 10.f } } } }) == 0u,
			"Negative coordinates and an ID of zero must remain valid");
	}
}

int main()
{
	ly::EngineContext engineContext;
	ly::EngineContext::Scope contextScope{ engineContext };
	TestSelection();
	TestExpiry();
	TestMultiple();
	TestPicking();
	if (failures != 0)
	{
		return 1;
	}
	std::cout << "Editor selection and picking tests passed\n";
	return 0;
}
