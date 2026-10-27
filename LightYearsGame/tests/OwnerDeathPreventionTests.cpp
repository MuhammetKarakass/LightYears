#include "framework/EngineContext.h"
#include "framework/World.h"
#include "gameFramework/LightYearsAssets.h"
#include "gameplay/combat/LethalHitInterceptors.h"
#include "spaceShip/SpaceShip.h"

#include <iostream>
#include <utility>

namespace
{
	using namespace vega;

	bool Expect(bool condition, const char* message)
	{
		if (!condition) std::cerr << "FAILED: " << message << '\n';
		return condition;
	}

	class TestShip final : public SpaceShip
	{
	public:
		TestShip(World* world, const ShipDefinition& definition) : SpaceShip{ world, definition } {}
		using SpaceShip::ResolveLethalDamage;
	};

	bool TestPriorityAndFirstResult(SpaceShip& ship)
	{
		LethalHitInterceptorList list;
		List<int> calls;
		auto last = list.Register([&](SpaceShip&, const DamageContext&) { calls.push_back(30); return LethalDamageResolution::Prevented; }, 30);
		auto middle = list.Register([&](SpaceShip&, const DamageContext&) { calls.push_back(20); return LethalDamageResolution::ConvertedKill; }, 20);
		auto first = list.Register([&](SpaceShip&, const DamageContext&) { calls.push_back(10); return LethalDamageResolution::None; }, 10);
		if (!Expect(list.Resolve(ship, {}) == LethalDamageResolution::ConvertedKill && calls == List<int>{ 10, 20 }, "priority is independent of registration order; first non-None wins")) return false;
		middle.Reset();
		calls.clear();
		if (!Expect(list.Resolve(ship, {}) == LethalDamageResolution::Prevented && calls == List<int>{ 10, 30 }, "reset removes exactly its own entry")) return false;
		last.Reset();
		calls.clear();
		return Expect(list.Resolve(ship, {}) == LethalDamageResolution::None && calls == List<int>{ 10 }, "all None results fall through");
	}

	bool TestTokenLifetime(SpaceShip& ship)
	{
		LethalHitInterceptorList::Registration expired;
		{
			LethalHitInterceptorList list;
			expired = list.Register([](SpaceShip&, const DamageContext&) { return LethalDamageResolution::Prevented; });
			if (!Expect(expired.IsValid(), "registration is valid while list state lives")) return false;
		}
		if (!Expect(!expired.IsValid(), "token safely observes expired list state")) return false;
		expired.Reset();
		expired.Reset();

		LethalHitInterceptorList list;
		List<int> calls;
		auto first = list.Register([&](SpaceShip&, const DamageContext&) { calls.push_back(1); return LethalDamageResolution::None; });
		{
			auto removed = list.Register([&](SpaceShip&, const DamageContext&) { calls.push_back(2); return LethalDamageResolution::None; });
			auto moved = std::move(removed);
			if (!Expect(!removed.IsValid() && moved.IsValid(), "moving transfers token ownership")) return false;
		}
		auto replacement = list.Register([&](SpaceShip&, const DamageContext&) { calls.push_back(3); return LethalDamageResolution::None; });
		auto moved = std::move(first);
		first.Reset();
		if (!Expect(list.Resolve(ship, {}) == LethalDamageResolution::None && calls == List<int>{ 1, 3 }, "destruction removes one entry; stale token cannot remove a newer registration; equal priorities keep order")) return false;
		moved = std::move(replacement);
		calls.clear();
		return Expect(list.Resolve(ship, {}) == LethalDamageResolution::None && calls == List<int>{ 3 }, "move assignment removes the previous entry");
	}

	bool TestRemovalDuringResolution(SpaceShip& ship)
	{
		LethalHitInterceptorList list;
		int removedCalls = 0;
		LethalHitInterceptorList::Registration removed;
		auto first = list.Register([&](SpaceShip&, const DamageContext&) { removed.Reset(); return LethalDamageResolution::None; }, 10);
		removed = list.Register([&](SpaceShip&, const DamageContext&) { ++removedCalls; return LethalDamageResolution::Prevented; }, 20);
		return Expect(list.Resolve(ship, {}) == LethalDamageResolution::None && removedCalls == 0, "reset during resolution does not call a removed receiver from the snapshot");
	}

	bool TestOwnerBeforeSource(TestShip& target, TestShip& source)
	{
		DamageContext context;
		context.source = &source;
		context.target = &target;
		List<int> calls;
		LethalDamageResolution ownerResult = LethalDamageResolution::Prevented;
		// Even an earlier priority on the source cannot precede the target's own list.
		auto capture = source.GetCombatRuntime().GetLethalHitInterceptors().Register([&](SpaceShip& actualTarget, const DamageContext& actualContext)
		{
			calls.push_back(&actualTarget == &target && actualContext.source == &source ? 2 : -2);
			return LethalDamageResolution::ConvertedKill;
		}, -100);
		auto prevent = target.GetCombatRuntime().GetOwnerDeathPreventions().Register([&](SpaceShip&, const DamageContext&) { calls.push_back(1); return ownerResult; }, 100);
		if (!Expect(target.ResolveLethalDamage(context) == LethalDamageResolution::Prevented && calls == List<int>{ 1 }, "owner prevention runs first and stops source capture")) return false;
		ownerResult = LethalDamageResolution::None;
		calls.clear();
		if (!Expect(target.ResolveLethalDamage(context) == LethalDamageResolution::ConvertedKill && calls == List<int>{ 1, 2 }, "source list runs only after owner list declines")) return false;
		context.source = nullptr;
		calls.clear();
		if (!Expect(target.ResolveLethalDamage(context) == LethalDamageResolution::None && calls == List<int>{ 1 }, "owner list also runs without a source")) return false;
		ownerResult = LethalDamageResolution::ConvertedKill;
		return Expect(target.ResolveLethalDamage(context) == LethalDamageResolution::ConvertedKill, "owner list preserves any non-None resolution");
	}
}

int main()
{
	vega::EngineContext context;
	vega::EngineContext::Scope scope{ context };
	vega::InstallVegaEngineAssets();
	vega::World world{ context };
	ShipDefinition definition{ "", 100.f, {}, 0.f, 0.f, 0, {}, {} };
	TestShip target{ &world, definition };
	TestShip source{ &world, definition };
	if (!TestPriorityAndFirstResult(target) || !TestTokenLifetime(target) || !TestRemovalDuringResolution(target) || !TestOwnerBeforeSource(target, source)) return 1;
	std::cout << "Owner death prevention tests passed.\n";
	return 0;
}
