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

	ly::ResolvedModuleTriggerBuff MakeBuff(std::uint32_t instanceId, ly::ModuleTriggerKind trigger, float movementMultiplier)
	{
		ly::ResolvedModuleTriggerBuff resolved;
		resolved.instanceId = instanceId;
		resolved.rule.trigger = trigger;
		resolved.rule.durationSeconds = 20.f;
		resolved.rule.movementSpeedMultiplier = movementMultiplier;
		return resolved;
	}

	ly::ResolvedModuleDamageRule MakeDamageRule(
		std::size_t ruleIndex,
		ly::ModuleDamageCondition condition,
		float multiplier,
		ly::ModuleTriggerKind activeWhile = ly::ModuleTriggerKind::None,
		float durationSeconds = 0.f,
		float conditionValue = 0.f,
		int openingShotCount = 1
	)
	{
		ly::ResolvedModuleDamageRule resolved;
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

	void BroadcastEvent(ly::PlayerSpaceShip& ship, const ly::GameplayTag& tag, float magnitude)
	{
		sas::AbilityEvent event;
		event.eventTag = tag;
		event.magnitude = magnitude;
		event.payloadTags.push_back(ly::GameplayTags::Damage::Type::Kinetic);
		ship.GetAbilitySystemComponent().onGameplayEvent.Broadcast(event);
	}

	void BroadcastSourceEvent(
		ly::PlayerSpaceShip& ship,
		const ly::GameplayTag& tag,
		const ly::List<ly::GameplayTag>& sourceAbilityTags,
		const ly::DamageContext* damage = nullptr
	)
	{
		sas::AbilityEvent event;
		event.eventTag = tag;
		event.magnitude = 5.f;
		event.sourceAbilityTags = sourceAbilityTags;
		event.payloadTags.push_back(ly::GameplayTags::Damage::Type::Kinetic);
		event.SetContext(damage);
		ship.GetAbilitySystemComponent().onGameplayEvent.Broadcast(event);
	}
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: CommonModuleE2ETriggerLifetimeTests <artifact-path>\n";
		return 2;
	}

	TriggerLifetimeReport report;
	const std::filesystem::path artifactPath{ argv[1] };
	try
	{
		ly::AssetManager::GetAssetManager().SetAssetRootDirectory(
			(std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/"
		);
		if (!ly::GameContentBootstrap::Register()) throw std::runtime_error{ "GameContentBootstrap::Register failed" };
		std::string windowTitle = "Common module trigger lifetime E2E";
		ly::Application application{ sf::Vector2u{ 64u, 64u }, 32u, windowTitle, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		const ShipDefinition& shipDefinition = ly::content::ShipContentCatalog::GetPlayerShipDefinition("Ship.Player.Fighter.Basic");
		auto level = std::make_shared<common_module_e2e::HeadlessModuleLevel>(&application);
		level->BeginPlayInternal();
		const ly::shared_ptr<ly::PlayerSpaceShip> source = level->SpawnActor<ly::PlayerSpaceShip>(shipDefinition).lock();
		const ly::shared_ptr<ly::PlayerSpaceShip> target = level->SpawnActor<ly::PlayerSpaceShip>(shipDefinition).lock();
		report.Check(source && target, "trigger lifetime fixture spawns real player ships");
		level->TickInternal(0.f);
		source->SetInvulnerability(false);
		target->SetInvulnerability(false);
		target->GetHealthComponent().SetMaxHealth(5000.f, true);

		const ly::List<ly::GameplayTag> primaryTags{ ly::GameplayTags::Ability::Primary };
		{
			ly::ModuleTriggerBuffRuleApplier olderOwner{ source->GetModuleTriggerRuntime() };
			ly::ModuleTriggerBuffRuleApplier currentOwner{ source->GetModuleTriggerRuntime() };
			olderOwner.Replace({ MakeBuff(9101, ly::ModuleTriggerKind::ShieldBroken, 1.05f) });

			ly::List<ly::ResolvedModuleTriggerBuff> currentBuffs{
				MakeBuff(9102, ly::ModuleTriggerKind::ShieldBroken, 1.25f),
				MakeBuff(9103, ly::ModuleTriggerKind::DamageTaken, 1.1f),
				MakeBuff(9104, ly::ModuleTriggerKind::SourceDamageDealt, 1.2f),
				MakeBuff(9105, ly::ModuleTriggerKind::AbilityActivated, 1.15f)
			};
			currentOwner.Replace(currentBuffs);
			olderOwner.Reset();

			ly::CombatRuntime::OutgoingDamageRuleRegistration damageOwner = source->GetCombatRuntime().RegisterOutgoingDamageRules();
			ly::List<ly::ResolvedModuleDamageRule> damageRules{
				MakeDamageRule(0, ly::ModuleDamageCondition::TargetHealthAtMost, 2.f,
					ly::ModuleTriggerKind::None, 0.f, 1.f),
				MakeDamageRule(1, ly::ModuleDamageCondition::FullMagazineShot, 1.3f,
					ly::ModuleTriggerKind::None, 0.f, 0.f, 3),
				MakeDamageRule(2, ly::ModuleDamageCondition::PrimaryShotOnly, 1.5f,
					ly::ModuleTriggerKind::ShieldBroken, 20.f),
				MakeDamageRule(3, ly::ModuleDamageCondition::PrimaryShotOnly, 1.4f,
					ly::ModuleTriggerKind::DamageTaken, 20.f),
				MakeDamageRule(4, ly::ModuleDamageCondition::PrimaryShotOnly, 1.6f,
					ly::ModuleTriggerKind::SourceDamageDealt, 20.f),
				MakeDamageRule(5, ly::ModuleDamageCondition::PrimaryShotOnly, 1.1f,
					ly::ModuleTriggerKind::AbilityActivated, 20.f)
			};
			damageOwner.ReplaceRules(damageRules);

			report.CheckNear("impact health condition applies at threshold one", 2.f,
				source->GetCombatRuntime().ResolveOutgoingDamagePlan(*target, primaryTags).damageMultiplier);
			report.CheckNear("full magazine fire snapshot excludes the threshold-one impact condition", 1.3f,
				source->GetCombatRuntime().ResolveFullMagazineShotMultiplier(0));

			const ly::List<std::pair<ly::GameplayTag, float>> rejectedEvents{
				{ ly::GameplayTags::Event::Owner::DamageTaken, 0.f },
				{ ly::GameplayTags::Event::Owner::DamageTaken, -1.f },
				{ ly::GameplayTags::Event::Source::DamageDealt, 0.f },
				{ ly::GameplayTags::Event::Source::DamageDealt, -1.f }
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

			BroadcastEvent(*source, ly::GameplayTags::Event::Ability::Activated, 0.f);
			report.Check(source->GetModuleTriggerRuntime().GetActiveBuffCount() == 1,
				"zero-magnitude ability activation remains a valid timed-buff event");
			report.CheckNear("zero-magnitude ability activation starts its damage window", 2.2f,
				source->GetCombatRuntime().ResolveOutgoingDamagePlan(*target, primaryTags).damageMultiplier);

			currentBuffs = { MakeBuff(9102, ly::ModuleTriggerKind::ShieldBroken, 1.25f),
				MakeBuff(9103, ly::ModuleTriggerKind::DamageTaken, 1.1f),
				MakeBuff(9104, ly::ModuleTriggerKind::SourceDamageDealt, 1.2f) };
			currentOwner.Replace(currentBuffs);
			damageRules.pop_back();
			damageOwner.ReplaceRules(damageRules);
			report.Check(source->GetModuleTriggerRuntime().GetActiveBuffCount() == 0,
				"replacing trigger rules removes the previous timed buff");

			const float shieldBeforeHit = source->GetShieldComponent().GetShield();
			ly::ApplyCombatDamage(*source, shieldBeforeHit + 2.f, nullptr, { ly::GameplayTags::Damage::Type::Kinetic });
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

			ly::ApplyCombatDamage(*target, 1.f, source.get(), { ly::GameplayTags::Damage::Type::Kinetic }, {}, {}, primaryTags);
			report.Check(source->GetModuleTriggerRuntime().GetActiveBuffCount() == 3,
				"positive real source-damage-dealt event activates its timed buff");
			report.CheckNear("positive source-damage-dealt event opens its outgoing damage window", 6.72f,
				source->GetCombatRuntime().ResolveOutgoingDamagePlan(*target, primaryTags).damageMultiplier);

			const PrimaryWeaponDefinition* fighter = ly::content::WeaponContentCatalog::FindById("Weapon.Projectile.FighterRapidLaser.Basic");
			report.Check(fighter && fighter->magazine.has_value(), "fire-time snapshot fixture uses the shipped magazine weapon");
			if (fighter && fighter->magazine)
			{
				sas::GameplayAttributeList fighterAttributes = fighter->attributes;
				ly::PrimaryWeaponExecutionContext context{ *source, *fighter, fighterAttributes };
				context.damageTags = fighter->damageTags;
				ly::PrimaryWeaponRuntimeState runtime;
				report.Check(ly::PrimaryWeaponExecutionSystem::InitializeRuntime(*fighter, runtime).isValid,
					"fire-time snapshot fixture initializes the real weapon runtime");
				ly::PrimaryWeaponExecutionSystem::BeginFire(context, runtime);
				report.Check(ly::PrimaryWeaponExecutionSystem::FireOnce(context, runtime),
					"fire-time snapshot fixture fires through PrimaryWeaponExecutionSystem");
				level->TickInternal(0.f);
				const auto projectiles = level->GetActorsByType<ly::PrimaryWeaponProjectileActor>();
				const ly::shared_ptr<ly::PrimaryWeaponProjectileActor> projectile = projectiles.empty()
					? ly::shared_ptr<ly::PrimaryWeaponProjectileActor>{} : projectiles.back().lock();
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
			ly::CombatRuntime::OutgoingDamageRuleRegistration windowOwner = source->GetCombatRuntime().RegisterOutgoingDamageRules();
			ly::ResolvedModuleDamageRule offenseWindow = MakeDamageRule(0, ly::ModuleDamageCondition::PrimaryShotOnly, 1.5f,
				ly::ModuleTriggerKind::AbilityActivated, 20.f);
			offenseWindow.rule.activeWhileEventTags = { ly::GameplayTags::Ability::Offense };
			const ly::ResolvedModuleDamageRule dealtWindow = MakeDamageRule(1, ly::ModuleDamageCondition::PrimaryShotOnly, 1.25f,
				ly::ModuleTriggerKind::SourceDamageDealt, 20.f);
			windowOwner.ReplaceRules({ offenseWindow, dealtWindow });
			const auto windowMultiplier = [&]()
				{ return source->GetCombatRuntime().ResolveOutgoingDamagePlan(*target, primaryTags).damageMultiplier; };

			BroadcastSourceEvent(*source, ly::GameplayTags::Event::Ability::Activated, primaryTags);
			report.CheckNear("tagged damage window ignores a primary-only activation", 1.f, windowMultiplier());
			BroadcastSourceEvent(*source, ly::GameplayTags::Event::Ability::Activated,
				{ ly::GameplayTags::Ability::Offense, ly::GameplayTags::Ability::Primary });
			report.CheckNear("offense damage window excludes primary activations like timed buffs", 1.f, windowMultiplier());
			BroadcastSourceEvent(*source, ly::GameplayTags::Event::Ability::Activated, { ly::GameplayTags::Ability::Offense });
			report.CheckNear("offense activation opens its tagged damage window", 1.5f, windowMultiplier());

			ly::DamageContext bonusHit;
			bonusHit.payload.skipModuleDamageRules = true;
			BroadcastSourceEvent(*source, ly::GameplayTags::Event::Source::DamageDealt, primaryTags, &bonusHit);
			report.CheckNear("module-generated damage never opens a damage window", 1.5f, windowMultiplier());
			ly::DamageContext ownHit;
			BroadcastSourceEvent(*source, ly::GameplayTags::Event::Source::DamageDealt, primaryTags, &ownHit);
			report.CheckNear("owner damage still opens the source-damage window", 1.875f, windowMultiplier());

			const ly::ResolvedModuleDamageRule threeShotOpening = MakeDamageRule(2, ly::ModuleDamageCondition::FullMagazineShot, 1.3f,
				ly::ModuleTriggerKind::None, 0.f, 0.f, 3);
			windowOwner.ReplaceRules({ offenseWindow, dealtWindow, threeShotOpening });
			report.CheckNear("fire-phase rule stays out of the impact plan", 1.875f, windowMultiplier());
			report.CheckNear("fire-phase rule applies inside its opening shots", 1.3f,
				source->GetCombatRuntime().ResolveFullMagazineShotMultiplier(2));
			report.CheckNear("fire-phase rule stops after its opening shots", 1.f,
				source->GetCombatRuntime().ResolveFullMagazineShotMultiplier(3));
		}

		{
			const ly::ModuleRulesContext rules{
				*shipDefinition.schematic,
				[](const sas::ContentId& moduleId) { return ly::content::ModuleContentCatalog::FindById(moduleId); },
				ly::content::ModuleContentCatalog::GetZoneTable()
			};
			ly::shared_ptr<ly::PlayerSpaceShip> teardownShip = level->SpawnActor<ly::PlayerSpaceShip>(shipDefinition).lock();
			level->TickInternal(0.f);
			ly::ModuleLoadout loadout;
			report.Check(teardownShip && loadout.SeedDefaults(rules), "teardown fixture spawns a ship and loads real defaults");
			common_module_e2e::Report installReport;
			common_module_e2e::AddAndInstall(installReport, loadout, rules, "Module.Defense.HullReinforcement.Basic");
			common_module_e2e::AddAndInstall(installReport, loadout, rules, "Module.Primary.FocusedBarrel.Basic");
			report.Check(teardownShip->GetModuleApplier().Apply(loadout, rules) &&
				teardownShip->GetModuleApplier().GetOwnedHandleCount() >= 2,
				"teardown fixture owns static owner and primary-weapon modifiers");
			const std::weak_ptr<ly::PlayerSpaceShip> expired = teardownShip;
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
