#pragma once

#include <cstdint>

namespace ly
{
	// This selector crosses catalog, registry, and validator translation units.
	// Values are explicit so appending a new family cannot silently reinterpret
	// stale/generated data as another behavior during an incremental build.
	enum class AbilityBehaviorType : uint16_t
	{
		Configured = 0,
		Dash = 1,
		Shield = 2,
		GravityAnomaly = 3,
		Rocket = 4,
		SunBeam = 5,
		InfernoSpray = 6,
		OverdriveCore = 7,
		NullPulse = 8,
		PhaseDrift = 9,
		ShieldHarvest = 10,
		HullShock = 11,
		OrbitalDrones = 12,
		ExecutionDrive = 13,
		RelayPrism = 14,
		EchoProtocol = 15,
		RailBurst = 16,
		EnergySpear = 17,
		MineLayer = 18,
		ScorchDrive = 19,
		IonStorm = 20,
		CrescentReaver = 21,
		DirectionalBarrier = 22,
		ChainLightning = 23,
		GlacialPressure = 24,
		Cryostasis = 25,
		VoidGate = 26,
		FrostMaelstrom = 27,
		FrozenThrong = 28,
		CombatSentry = 29,
		AstralSurge = 30,
		WingSentinels = 31,
		NanoPlague = 32,
		ReturnProtocol = 33,
		CrystalBarricade = 34
	};

	inline const char* ToString(AbilityBehaviorType type)
	{
		switch (type)
		{
		case AbilityBehaviorType::Configured: return "Configured";
		case AbilityBehaviorType::Dash: return "Dash";
		case AbilityBehaviorType::Shield: return "Shield";
		case AbilityBehaviorType::GravityAnomaly: return "GravityAnomaly";
		case AbilityBehaviorType::Rocket: return "Rocket";
		case AbilityBehaviorType::SunBeam: return "SunBeam";
		case AbilityBehaviorType::InfernoSpray: return "InfernoSpray";
		case AbilityBehaviorType::OverdriveCore: return "OverdriveCore";
		case AbilityBehaviorType::NullPulse: return "NullPulse";
		case AbilityBehaviorType::PhaseDrift: return "PhaseDrift";
		case AbilityBehaviorType::ShieldHarvest: return "ShieldHarvest";
		case AbilityBehaviorType::HullShock: return "HullShock";
		case AbilityBehaviorType::OrbitalDrones: return "OrbitalDrones";
		case AbilityBehaviorType::ExecutionDrive: return "ExecutionDrive";
		case AbilityBehaviorType::RelayPrism: return "RelayPrism";
		case AbilityBehaviorType::EchoProtocol: return "EchoProtocol";
		case AbilityBehaviorType::RailBurst: return "RailBurst";
		case AbilityBehaviorType::EnergySpear: return "EnergySpear";
		case AbilityBehaviorType::MineLayer: return "MineLayer";
		case AbilityBehaviorType::ScorchDrive: return "ScorchDrive";
		case AbilityBehaviorType::IonStorm: return "IonStorm";
		case AbilityBehaviorType::CrescentReaver: return "CrescentReaver";
		case AbilityBehaviorType::DirectionalBarrier: return "DirectionalBarrier";
		case AbilityBehaviorType::ChainLightning: return "ChainLightning";
		case AbilityBehaviorType::GlacialPressure: return "GlacialPressure";
		case AbilityBehaviorType::Cryostasis: return "Cryostasis";
		case AbilityBehaviorType::VoidGate: return "VoidGate";
		case AbilityBehaviorType::FrostMaelstrom: return "FrostMaelstrom";
		case AbilityBehaviorType::FrozenThrong: return "FrozenThrong";
		case AbilityBehaviorType::CombatSentry: return "CombatSentry";
		case AbilityBehaviorType::AstralSurge: return "AstralSurge";
		case AbilityBehaviorType::WingSentinels: return "WingSentinels";
		case AbilityBehaviorType::NanoPlague: return "NanoPlague";
		case AbilityBehaviorType::ReturnProtocol: return "ReturnProtocol";
		case AbilityBehaviorType::CrystalBarricade: return "CrystalBarricade";
		}
		return "Unknown";
	}
}
