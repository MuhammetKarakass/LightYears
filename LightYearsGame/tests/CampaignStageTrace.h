#pragma once

#include "enemy/VanguardStage.h"
#include "enemy/TwinBladeStage.h"
#include "enemy/HexagonStage.h"
#include "enemy/ChaosStage.h"
#include "enemy/EnemyActor.h"
#include "framework/World.h"
#include "framework/RenderViewport.h"
#include "gameplay/GameStageSequence.h"
#include "gameplay/WaitStage.h"
#include "CampaignStageRandom.h"
#undef RandRange
#include <nlohmann/json.hpp>
#include <set>

namespace campaign_test
{
	using namespace vega;
	using Json = nlohmann::json;
	class HeadlessViewport : public RenderViewport
	{
	public:
		sf::RenderTexture target{ sf::Vector2u{1,1} };
		sf::RenderTarget& GetRenderTarget() override { return target; }
		const sf::RenderTarget& GetRenderTarget() const override { return target; }
		sf::Vector2u GetSize() const override { return {1920,1080}; }
		sf::Vector2i GetMousePixelPosition() const override { return {}; }
		sf::Vector2f MapPixelToCoords(sf::Vector2i, const sf::View&) const override { return {}; }
		sf::View GetDefaultView() const override { return sf::View{sf::FloatRect{{0.f,0.f},{1920.f,1080.f}}}; }
		bool HasInputFocus() const override { return false; }
	};
	// Frozen transcription of LevelOne::InitGameStages at 3e26ef91. Do not rewrite
	// this oracle to call the new factory or read the scene configuration.
	inline std::vector<shared_ptr<GameStage>> LegacyStages(World& world)
	{
		return { std::make_shared<WaitStage>(&world, 5.f), std::make_shared<VanguardStage>(&world),
			std::make_shared<WaitStage>(&world, 5.f), std::make_shared<TwinBladeStage>(&world),
			std::make_shared<WaitStage>(&world, 5.f), std::make_shared<HexagonStage>(&world),
			std::make_shared<WaitStage>(&world, 5.f), std::make_shared<ChaosStage>(&world) };
	}
	inline std::string Kind(GameStage& stage)
	{
		if (dynamic_cast<WaitStage*>(&stage)) return "Wait";
		if (dynamic_cast<VanguardStage*>(&stage)) return "Vanguard";
		if (dynamic_cast<TwinBladeStage*>(&stage)) return "TwinBlade";
		if (dynamic_cast<HexagonStage*>(&stage)) return "Hexagon";
		if (dynamic_cast<ChaosStage*>(&stage)) return "Chaos";
		throw std::runtime_error("Unexpected stage type");
	}
	class Trace : public Object
	{
	public:
		int step = 0;
		Json events = Json::array();
		GameStageSequence* sequence = nullptr;
		void Finished() { events.push_back({{"event","finish"},{"step",step}}); sequence->Next(); }
	};
	template<typename Factory> Json Run(Factory factory, unsigned int seed = 1741)
	{
		campaignTestRandom.seed(seed);
		HeadlessViewport viewport;
		World world{ vega::EngineContext::Current() };
		world.SetRenderViewport(&viewport);
		auto trace = std::make_shared<Trace>();
		bool done = false;
		GameStageSequence sequence{ [&](GameStage& stage)
			{
				trace->events.push_back({{"event","begin"},{"kind",Kind(stage)},{"step",trace->step}});
				stage.onStageFinished.BindAction(trace->GetWeakPtr(), &Trace::Finished);
			}, [&]() { done = true; } };
		trace->sequence = &sequence;
		for (auto& stage : factory(world)) sequence.Add(stage);
		sequence.Begin();
		Json spawns = Json::array();
		std::set<unsigned int> seen;
		// Exercise the real stage sequence, timers and enemy spawn path, without
		// actor ticks, player input, physics, a HUD or a native window.
		for (trace->step = 1; trace->step <= 10000 && !done; ++trace->step)
		{
			sequence.Tick(0.1f);
			world.GetTimerManager().UpdateTimer(0.1f);
			for (const auto& handle : world.GetActorsByTypeIncludingPending<EnemyActor>())
			{
				const auto enemy = handle.lock();
				if (seen.insert(enemy->GetUniqueID()).second) spawns.push_back({{"step",trace->step},{"id",enemy->GetEnemyId()},
					{"position",{enemy->GetActorLocation().x,enemy->GetActorLocation().y}}});
			}
		}
		if (!done) throw std::runtime_error("Campaign did not finish headlessly");
		return {{"seed",seed},{"deltaTime",0.1f},{"events",trace->events},{"spawns",spawns}};
	}
}
