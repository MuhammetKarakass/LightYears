#include "level/authoring/CampaignSetupActor.h"
#include "enemy/VanguardStage.h"
#include "enemy/TwinBladeStage.h"
#include "enemy/HexagonStage.h"
#include "enemy/ChaosStage.h"
#include "gameplay/WaitStage.h"
#include <cmath>
#include <limits>

namespace vega
{
	VEGA_REFLECT_STRUCT(CampaignStageEntry)
	{
		VEGA_FIELD("kind", kind).Labels({{static_cast<int>(CampaignStageKind::Wait),"Wait"},{static_cast<int>(CampaignStageKind::Vanguard),"Vanguard"},
			{static_cast<int>(CampaignStageKind::TwinBlade),"TwinBlade"},{static_cast<int>(CampaignStageKind::Hexagon),"Hexagon"},{static_cast<int>(CampaignStageKind::Chaos),"Chaos"}});
		VEGA_FIELD("duration", duration).Tooltip("Wait seconds. Other kinds require zero and finish through their C++ rules.");
		VEGA_FIELD("configuration", configuration).Tooltip("Zero-based index into the list matching kind; Wait requires -1.");
	}
	VEGA_REFLECT_STRUCT(VanguardStageSettings)
	{
		VEGA_FIELD("minSpawnInterval", minSpawnInterval);
		VEGA_FIELD("maxSpawnInterval", maxSpawnInterval);
		VEGA_FIELD("spawnDistanceToEdge", spawnDistanceToEdge);
		VEGA_FIELD("switchInterval", switchInterval);
		VEGA_FIELD("rows", rows);
		VEGA_FIELD("perRow", perRow);
	}
	VEGA_REFLECT_STRUCT(TwinBladeStageSettings)
	{
		VEGA_FIELD("minSpawnInterval", minSpawnInterval);
		VEGA_FIELD("maxSpawnInterval", maxSpawnInterval);
		VEGA_FIELD("spawnCount", spawnCount);
	}
	VEGA_REFLECT_STRUCT(HexagonStageSettings)
	{
		VEGA_FIELD("spawnInterval", spawnInterval);
		VEGA_FIELD("spawnGroups", spawnGroups);
	}
	VEGA_REFLECT_STRUCT(ChaosStageSettings)
	{
		VEGA_FIELD("spawnInterval", spawnInterval);
		VEGA_FIELD("spawnIntervalDecrement", spawnIntervalDecrement);
		VEGA_FIELD("survivalDuration", survivalDuration);
		VEGA_FIELD("spawnMinDistanceTop", spawnMinDistanceTop);
	}
	VEGA_REFLECT(CampaignSetupActor, Actor)
	{
		VEGA_SPAWNABLE();
		VEGA_CATEGORY("Campaign configuration");
		// Each list is a top-level property. No reflected struct contains a list.
		type.Property("Stages", [](const Self& actor) { return actor.plan.stages; }, [](Self& actor, const auto& value) { actor.plan.stages = value; });
		type.Property("Vanguard", [](const Self& actor) { return actor.plan.vanguard; }, [](Self& actor, const auto& value) { actor.plan.vanguard = value; });
		type.Property("TwinBlade", [](const Self& actor) { return actor.plan.twinBlade; }, [](Self& actor, const auto& value) { actor.plan.twinBlade = value; });
		type.Property("Hexagon", [](const Self& actor) { return actor.plan.hexagon; }, [](Self& actor, const auto& value) { actor.plan.hexagon = value; });
		type.Property("Chaos", [](const Self& actor) { return actor.plan.chaos; }, [](Self& actor, const auto& value) { actor.plan.chaos = value; });
	}

	void CampaignPlan::Validate() const
	{
		auto require = [](bool valid, const std::string& path, const std::string& reason) { if (!valid) throw std::runtime_error("LevelOne layout: CampaignSetupActor." + path + " " + reason); };
		auto positive = [](float value) { return std::isfinite(value) && value > 0.f; };
		auto nonnegative = [](float value) { return std::isfinite(value) && value >= 0.f; };
		require(!stages.empty(), "Stages", "must be nonempty");
		int chaosStages = 0;
		for (size_t index = 0; index < stages.size(); ++index)
		{
			const auto& entry = stages[index];
			const auto path = "Stages[" + std::to_string(index) + "]";
			require(nonnegative(entry.duration), path + ".duration", "must be finite/nonnegative");
			if (entry.kind == CampaignStageKind::Wait)
			{
				require(entry.configuration == -1, path + ".configuration", "must be -1 for Wait");
				continue;
			}
			require(entry.duration == 0.f, path + ".duration", "must be zero for a rule-completed stage");
			size_t count = 0;
			switch (entry.kind)
			{
			case CampaignStageKind::Vanguard: count = vanguard.size(); break;
			case CampaignStageKind::TwinBlade: count = twinBlade.size(); break;
			case CampaignStageKind::Hexagon: count = hexagon.size(); break;
			case CampaignStageKind::Chaos: count = chaos.size(); require(++chaosStages <= 1, path + ".kind", "permits at most one Chaos stage for the existing HUD binding"); break;
			default: require(false, path + ".kind", "is unknown"); break;
			}
			require(entry.configuration >= 0 && static_cast<size_t>(entry.configuration) < count, path + ".configuration", "is outside the list selected by kind");
		}
		for (size_t index = 0; index < vanguard.size(); ++index)
		{
			const auto& value = vanguard[index];
			const auto path = "Vanguard[" + std::to_string(index) + "]";
			require(positive(value.minSpawnInterval) && positive(value.maxSpawnInterval) && value.maxSpawnInterval >= value.minSpawnInterval, path, "requires positive ordered spawn intervals");
			require(nonnegative(value.spawnDistanceToEdge) && nonnegative(value.switchInterval), path, "requires nonnegative spawnDistanceToEdge/switchInterval");
			require(value.rows > 0 && value.perRow > 0 && static_cast<int64_t>(value.rows) * value.perRow <= std::numeric_limits<int>::max(), path, "requires positive rows/perRow without int32 overflow");
		}
		for (size_t index = 0; index < twinBlade.size(); ++index)
		{
			const auto& value = twinBlade[index];
			require(positive(value.minSpawnInterval) && positive(value.maxSpawnInterval) && value.maxSpawnInterval >= value.minSpawnInterval && value.spawnCount > 0,
				"TwinBlade[" + std::to_string(index) + "]", "requires positive ordered spawn intervals and spawnCount");
		}
		for (size_t index = 0; index < hexagon.size(); ++index)
		{
			const auto& value = hexagon[index];
			require(positive(value.spawnInterval) && value.spawnGroups > 0 && value.spawnGroups <= std::numeric_limits<int>::max() / 3,
				"Hexagon[" + std::to_string(index) + "]", "requires positive spawnInterval/spawnGroups without int32 overflow");
		}
		for (size_t index = 0; index < chaos.size(); ++index)
		{
			const auto& value = chaos[index];
			// Difficulty advances from 1 to 12; the last scheduled interval is at 11.
			require(positive(value.spawnInterval) && nonnegative(value.spawnIntervalDecrement) && value.spawnInterval - 10.f * value.spawnIntervalDecrement > 0.f &&
				nonnegative(value.survivalDuration) && nonnegative(value.spawnMinDistanceTop), "Chaos[" + std::to_string(index) + "]",
				"requires positive spawn intervals through difficulty 11 and nonnegative decrement/survivalDuration/spawnMinDistanceTop");
		}
	}

	shared_ptr<GameStage> CampaignPlan::CreateStage(World& world, const CampaignStageEntry& entry) const
	{
		switch (entry.kind)
		{
		case CampaignStageKind::Wait: return std::make_shared<WaitStage>(&world, entry.duration);
		case CampaignStageKind::Vanguard: return std::make_shared<VanguardStage>(&world, vanguard.at(entry.configuration));
		case CampaignStageKind::TwinBlade: return std::make_shared<TwinBladeStage>(&world, twinBlade.at(entry.configuration));
		case CampaignStageKind::Hexagon: return std::make_shared<HexagonStage>(&world, hexagon.at(entry.configuration));
		case CampaignStageKind::Chaos: return std::make_shared<ChaosStage>(&world, chaos.at(entry.configuration));
		}
		throw std::runtime_error("LevelOne layout: unknown CampaignStageKind");
	}
}
