#include "gameFramework/LightYearsAssets.h"
#include "CommonModuleE2ETestSupport.h"

namespace
{
	using namespace common_module_e2e;

	struct TriggerLifetimeReport
	{
		Json artifact{ { "test", "CommonModuleE2ETriggerLifetime" }, { "passed", false }, { "checks", Json::array() } };

		void Check(bool passed, const std::string& name, Json actual = nullptr)
		{
			Json check{ { "name", name }, { "passed", passed } };
			if (!actual.is_null()) check["actual"] = std::move(actual);
			artifact["checks"].push_back(std::move(check));
			if (!passed) throw std::runtime_error{ name };
		}

		void CheckNear(const std::string& name, float expected, float actual)
		{
			Check(std::abs(expected - actual) < 0.001f, name, Json{ { "expected", expected }, { "actual", actual } });
		}
	};

	vega::ResolvedModuleTriggerBuff MakeBuff(std::uint32_t instanceId, vega::ModuleTriggerKind trigger, float movementMultiplier)
	{
		vega::ResolvedModuleTriggerBuff resolved;
		resolved.instanceId = instanceId;
		resolved.rule.trigger = trigger;
		resolved.rule.durationSeconds = 20.f;
		resolved.rule.movementSpeedMultiplier = movementMultiplier;
		return resolved;
	}

	vega::ResolvedModuleDamageRule MakeDamageRule(
		std::size_t ruleIndex,
		vega::ModuleDamageCondition condition,
		float multiplier,
		vega::ModuleTriggerKind activeWhile = vega::ModuleTriggerKind::None,
		float durationSeconds = 0.f,
		float conditionValue = 0.f,
		int openingShotCount = 1
	)
	{
		vega::ResolvedModuleDamageRule resolved;
		resolved.instanceId = 9200;
		resolved.ruleIndex = ruleIndex;
		resolved.rule.condition = condition;
		resolved.rule.conditionValue = conditionValue;
		resolved.rule.openingShotCount = openingShotCount;
		resolved.rule.damageMultiplier = multiplier;
		resolved.rule.activeWhileTrigger = activeWhile;
		resolved.rule.activeDurationSeconds = durationSeconds;
		return resolved;
	}

	void BroadcastEvent(vega::PlayerSpaceShip& ship, const vega::GameplayTag& tag, float magnitude)
	{
		sas::AbilityEvent event;
		event.eventTag = tag;
		event.magnitude = magnitude;
		event.payloadTags.push_back(vega::GameplayTags::Damage::Type::Kinetic);
		ship.GetAbilitySystemComponent().onGameplayEvent.Broadcast(event);
	}

	void BroadcastSourceEvent(
		vega::PlayerSpaceShip& ship,
		const vega::GameplayTag& tag,
		const vega::List<vega::GameplayTag>& sourceAbilityTags,
		const vega::DamageContext* damage = nullptr
	)
	{
		sas::AbilityEvent event;
		event.eventTag = tag;
		event.magnitude = 5.f;
		event.sourceAbilityTags = sourceAbilityTags;
		event.payloadTags.push_back(vega::GameplayTags::Damage::Type::Kinetic);
		event.SetContext(damage);
		ship.GetAbilitySystemComponent().onGameplayEvent.Broadcast(event);
	}
}

int main(int argc, char** argv)
{
	vega::InstallVegaEngineAssets();
	if (argc != 2)
	{
		std::cerr << "Usage: CommonModuleE2ETriggerLifetimeTests <artifact-path>\n";
		return 2;
	}

	TriggerLifetimeReport report;
	const std::filesystem::path artifactPath{ argv[1] };
	try
	{
		vega::AssetManager::GetAssetManager().SetAssetRootDirectory(
			(std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/"
		);
		if (!vega::GameContentBootstrap::Register()) throw std::runtime_error{ "GameContentBootstrap::Register failed" };
		std::string windowTitle = "Common module trigger lifetime E2E";
		vega::Application application{ sf::Vector2u{ 64u, 64u }, 32u, windowTitle, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		const ShipDefinition& shipDefinition = vega::content::ShipContentCatalog::GetPlayerShipDefinition("Ship.Player.Fighter.Basic");
		auto level = std::make_shared<common_module_e2e::HeadlessModuleLevel>(&application);
		level->BeginPlayInternal();
		const vega::shared_ptr<vega::PlayerSpaceShip> source = level->SpawnActor<vega::PlayerSpaceShip>(shipDefinition).lock();
		const vega::shared_ptr<vega::PlayerSpaceShip> target = level->SpawnActor<vega::PlayerSpaceShip>(shipDefinition).lock();
		report.Check(source && target, "trigger lifetime fixture spawns real player ships");
		level->TickInternal(0.f);
		source->SetInvulnerability(false);
		target->SetInvulnerability(false);
		target->GetHealthComponent().SetMaxHealth(5000.f, true);

		const vega::List<vega::GameplayTag> primaryTags{ vega::GameplayTags::Ability::Primary };
		{
			vega::ModuleTriggerBuffRuleApplier olderOwner{ source->GetModuleTriggerRuntime() };
			vega::ModuleTriggerBuffRuleApplier currentOwner{ source->GetModuleTriggerRuntime() };
			olderOwner.Replace({ MakeBuff(9101, vega::ModuleTriggerKind::ShieldBroken, 1.05f) });

			vega::List<vega::ResolvedModuleTriggerBuff> currentBuffs{
				MakeBuff(9102, vega::ModuleTriggerKind::ShieldBroken, 1.25f),
				MakeBuff(9103, vega::ModuleTriggerKind::DamageTaken, 1.1f),
				MakeBuff(9104, vega::ModuleTriggerKind::SourceDamageDealt, 1.2f),
				MakeBuff(9105, vega::ModuleTriggerKind::AbilityActivated, 1.15f)
			};
			currentOwner.Replace(currentBuffs);
			olderOwner.Reset();

			vega::CombatRuntime::OutgoingDamageRuleRegistration damageOwner = source->GetCombatRuntime().RegisterOutgoingDamageRules();
			vega::List<vega::ResolvedModuleDamageRule> damageRules{
				MakeDamageRule(0, vega::ModuleDamageCondition::TargetHealthAtMost, 2.f,
					vega::ModuleTriggerKind::None, 0.f, 1.f),
				MakeDamageRule(1, vega::ModuleDamageCondition::FullMagazineShot, 1.3f,
					vega::ModuleTriggerKind::None, 0.f, 0.f, 3),
				MakeDamageRule(2, vega::ModuleDamageCondition::PrimaryShotOnly, 1.5f,
					vega::ModuleTriggerKind::ShieldBroken, 20.f),
				MakeDamageRule(3, vega::ModuleDamageCondition::PrimaryShotOnly, 1.4f,
					vega::ModuleTriggerKind::DamageTaken, 20.f),
				MakeDamageRule(4, vega::ModuleDamageCondition::PrimaryShotOnly, 1.6f,
					vega::ModuleTriggerKind::SourceDamageDealt, 20.f),
				MakeDamageRule(5, vega::ModuleDamageCondition::PrimaryShotOnly, 1.1f,
					vega::ModuleTriggerKind::AbilityActivated, 20.f)
			};
			damageOwner.ReplaceRules(damageRules);

			report.CheckNear("impact health condition applies at threshold one", 2.f,
				source->GetCombatRuntime().ResolveOutgoingDamagePlan(*target, primaryTags).damageMultiplier);
			report.CheckNear("full magazine fire snapshot excludes the threshold-one impact condition", 1.3f,
				source->GetCombatRuntime().ResolveFullMagazineShotMultiplier(0));

			const vega::List<std::pair<vega::GameplayTag, float>> rejectedEvents{
				{ vega::GameplayTags::Event::Owner::DamageTaken, 0.f },
				{ vega::GameplayTags::Event::Owner::DamageTaken, -1.f },
				{ vega::GameplayTags::Event::Source::DamageDealt, 0.f },
				{ vega::GameplayTags::Event::Source::DamageDealt, -1.f }
			};
			for (const auto& rejected : rejectedEvents)
			{
				BroadcastEvent(*source, rejected.first, rejected.second);
				report.Check(source->GetModuleTriggerRuntime().GetActiveBuffCount() == 0,
					"nonpositive damage trigger does not activate a timed buff",
					Json{ { "event", rejected.first.name }, { "magnitude", rejected.second } });
				report.CheckNear("nonpositive damage trigger does not activate a damage window", 2.f,
					source->GetCombatRuntime().ResolveOutgoingDamagePlan(*target, primaryTags).damageMultiplier);
			}

			BroadcastEvent(*source, vega::GameplayTags::Event::Ability::Activated, 0.f);
			report.Check(source->GetModuleTriggerRuntime().GetActiveBuffCount() == 1,
				"zero-magnitude ability activation remains a valid timed-buff event");
			report.CheckNear("zero-magnitude ability activation starts its damage window", 2.2f,
				source->GetCombatRuntime().ResolveOutgoingDamagePlan(*target, primaryTags).damageMultiplier);

			currentBuffs = { MakeBuff(9102, vega::ModuleTriggerKind::ShieldBroken, 1.25f),
				MakeBuff(9103, vega::ModuleTriggerKind::DamageTaken, 1.1f),
				MakeBuff(9104, vega::ModuleTriggerKind::SourceDamageDealt, 1.2f) };
			currentOwner.Replace(currentBuffs);
			damageRules.pop_back();
			damageOwner.ReplaceRules(damageRules);
			report.Check(source->GetModuleTriggerRuntime().GetActiveBuffCount() == 0,
				"replacing trigger rules removes the previous timed buff");

			const float shieldBeforeHit = source->GetShieldComponent().GetShield();
			vega::ApplyCombatDamage(*source, shieldBeforeHit + 2.f, nullptr, { vega::GameplayTags::Damage::Type::Kinetic });
			report.Check(source->GetShieldComponent().GetShield() == 0.f,
				"real incoming damage breaks the source ship shield");
			report.Check(source->GetModuleTriggerRuntime().GetActiveBuffCount() == 2,
				"shield break activates both its timed buff and the positive damage-taken buff");
			report.CheckNear("shield-break and damage-taken buffs combine their movement multipliers", 1.375f,
				source->GetMovementSpeedMultiplier());
			report.CheckNear("shield break opens both outgoing damage windows", 4.2f,
				source->GetCombatRuntime().ResolveOutgoingDamagePlan(*target, primaryTags).damageMultiplier);
			report.CheckNear("shield-break window does not alter the full-magazine fire snapshot", 1.3f,
				source->GetCombatRuntime().ResolveFullMagazineShotMultiplier(0));

			vega::ApplyCombatDamage(*target, 1.f, source.get(), { vega::GameplayTags::Damage::Type::Kinetic }, {}, {}, primaryTags);
			report.Check(source->GetModuleTriggerRuntime().GetActiveBuffCount() == 3,
				"positive real source-damage-dealt event activates its timed buff");
			report.CheckNear("positive source-damage-dealt event opens its outgoing damage window", 6.72f,
				source->GetCombatRuntime().ResolveOutgoingDamagePlan(*target, primaryTags).damageMultiplier);

			const PrimaryWeaponDefinition* fighter = vega::content::WeaponContentCatalog::FindById("Weapon.Projectile.FighterRapidLaser.Basic");
			report.Check(fighter && fighter->magazine.has_value(), "fire-time snapshot fixture uses the shipped magazine weapon");
			if (fighter && fighter->magazine)
			{
				sas::GameplayAttributeList fighterAttributes = fighter->attributes;
				vega::PrimaryWeaponExecutionContext context{ *source, *fighter, fighterAttributes };
				context.damageTags = fighter->damageTags;
				vega::PrimaryWeaponRuntimeState runtime;
				report.Check(vega::PrimaryWeaponExecutionSystem::InitializeRuntime(*fighter, runtime).isValid,
					"fire-time snapshot fixture initializes the real weapon runtime");
				vega::PrimaryWeaponExecutionSystem::BeginFire(context, runtime);
				report.Check(vega::PrimaryWeaponExecutionSystem::FireOnce(context, runtime),
					"fire-time snapshot fixture fires through PrimaryWeaponExecutionSystem");
				level->TickInternal(0.f);
				const auto projectiles = level->GetActorsByType<vega::PrimaryWeaponProjectileActor>();
				const vega::shared_ptr<vega::PrimaryWeaponProjectileActor> projectile = projectiles.empty()
					? vega::shared_ptr<vega::PrimaryWeaponProjectileActor>{} : projectiles.back().lock();
				report.Check(projectile != nullptr, "fire-time snapshot fixture creates a real delayed projectile");
				if (projectile)
				{
					report.CheckNear("real FireOnce snapshots only the full-magazine rule", 1.3f,
						projectile->GetShotMetadata().shotDamageMultiplier);
				}
			}
		}

		report.Check(source->GetModuleTriggerRuntime().GetActiveBuffCount() == 0,
			"scope destruction removes active timed buffs");
		report.CheckNear("scope destruction removes timed movement modifiers", 1.f,
			source->GetMovementSpeedMultiplier());
		report.CheckNear("scope destruction unregisters outgoing damage rules", 1.f,
			source->GetCombatRuntime().ResolveOutgoingDamagePlan(*target, primaryTags).damageMultiplier);
		report.CheckNear("scope destruction unregisters full-magazine rules", 1.f,
			source->GetCombatRuntime().ResolveFullMagazineShotMultiplier(0));

		{
			vega::CombatRuntime::OutgoingDamageRuleRegistration windowOwner = source->GetCombatRuntime().RegisterOutgoingDamageRules();
			vega::ResolvedModuleDamageRule offenseWindow = MakeDamageRule(0, vega::ModuleDamageCondition::PrimaryShotOnly, 1.5f,
				vega::ModuleTriggerKind::AbilityActivated, 20.f);
			offenseWindow.rule.activeWhileEventTags = { vega::GameplayTags::Ability::Offense };
			const vega::ResolvedModuleDamageRule dealtWindow = MakeDamageRule(1, vega::ModuleDamageCondition::PrimaryShotOnly, 1.25f,
				vega::ModuleTriggerKind::SourceDamageDealt, 20.f);
			windowOwner.ReplaceRules({ offenseWindow, dealtWindow });
			const auto windowMultiplier = [&]()
				{ return source->GetCombatRuntime().ResolveOutgoingDamagePlan(*target, primaryTags).damageMultiplier; };

			BroadcastSourceEvent(*source, vega::GameplayTags::Event::Ability::Activated, primaryTags);
			report.CheckNear("tagged damage window ignores a primary-only activation", 1.f, windowMultiplier());
			BroadcastSourceEvent(*source, vega::GameplayTags::Event::Ability::Activated,
				{ vega::GameplayTags::Ability::Offense, vega::GameplayTags::Ability::Primary });
			report.CheckNear("offense damage window excludes primary activations like timed buffs", 1.f, windowMultiplier());
			BroadcastSourceEvent(*source, vega::GameplayTags::Event::Ability::Activated, { vega::GameplayTags::Ability::Offense });
			report.CheckNear("offense activation opens its tagged damage window", 1.5f, windowMultiplier());

			vega::DamageContext bonusHit;
			bonusHit.payload.skipModuleDamageRules = true;
			BroadcastSourceEvent(*source, vega::GameplayTags::Event::Source::DamageDealt, primaryTags, &bonusHit);
			report.CheckNear("module-generated damage never opens a damage window", 1.5f, windowMultiplier());
			vega::DamageContext ownHit;
			BroadcastSourceEvent(*source, vega::GameplayTags::Event::Source::DamageDealt, primaryTags, &ownHit);
			report.CheckNear("owner damage still opens the source-damage window", 1.875f, windowMultiplier());

			const vega::ResolvedModuleDamageRule threeShotOpening = MakeDamageRule(2, vega::ModuleDamageCondition::FullMagazineShot, 1.3f,
				vega::ModuleTriggerKind::None, 0.f, 0.f, 3);
			windowOwner.ReplaceRules({ offenseWindow, dealtWindow, threeShotOpening });
			report.CheckNear("fire-phase rule stays out of the impact plan", 1.875f, windowMultiplier());
			report.CheckNear("fire-phase rule applies inside its opening shots", 1.3f,
				source->GetCombatRuntime().ResolveFullMagazineShotMultiplier(2));
			report.CheckNear("fire-phase rule stops after its opening shots", 1.f,
				source->GetCombatRuntime().ResolveFullMagazineShotMultiplier(3));
		}

		{
			const vega::ModuleRulesContext rules{
				*shipDefinition.schematic,
				[](const sas::ContentId& moduleId) { return vega::content::ModuleContentCatalog::FindById(moduleId); },
				vega::content::ModuleContentCatalog::GetZoneTable()
			};
			vega::shared_ptr<vega::PlayerSpaceShip> teardownShip = level->SpawnActor<vega::PlayerSpaceShip>(shipDefinition).lock();
			level->TickInternal(0.f);
			vega::ModuleLoadout loadout;
			report.Check(teardownShip && loadout.SeedDefaults(rules), "teardown fixture spawns a ship and loads real defaults");
			common_module_e2e::Report installReport;
			common_module_e2e::AddAndInstall(installReport, loadout, rules, "Module.Defense.HullReinforcement.Basic");
			common_module_e2e::AddAndInstall(installReport, loadout, rules, "Module.Primary.FocusedBarrel.Basic");
			report.Check(teardownShip->GetModuleApplier().Apply(loadout, rules) &&
				teardownShip->GetModuleApplier().GetOwnedHandleCount() >= 2,
				"teardown fixture owns static owner and primary-weapon modifiers");
			const std::weak_ptr<vega::PlayerSpaceShip> expired = teardownShip;
			teardownShip->Destroy();
			teardownShip.reset();
			level->TickInternal(0.f);
			report.Check(expired.expired(), "destroying a ship with static module modifiers tears down cleanly");
		}
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception)
	{
		report.artifact["error"] = exception.what();
		std::cerr << "Common module trigger lifetime E2E failed: " << exception.what() << '\n';
	}

	try { common_module_e2e::WriteArtifact(artifactPath, report.artifact); }
	catch (const std::exception& exception)
	{
		std::cerr << "Could not write Common Module Trigger Lifetime E2E artifact: " << exception.what() << '\n';
		return 1;
	}
	std::cout << "E2E artifact: " << std::filesystem::absolute(artifactPath).string() << '\n';
	return report.artifact.value("passed", false) ? 0 : 1;
}
