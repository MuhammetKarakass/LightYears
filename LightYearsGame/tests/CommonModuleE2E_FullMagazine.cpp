#include "gameConfigs/CollisionLayers.h"
#include "CommonModuleE2ETestSupport.h"

namespace common_module_e2e
{
	void RunFullMagazineChecks(Report& report, vega::Application& application)
	{
		using namespace vega;
		const ShipDefinition& shipDefinition = content::ShipContentCatalog::GetPlayerShipDefinition("Ship.Player.Fighter.Basic");
		const ModuleRulesContext rules{
			*shipDefinition.schematic,
			[](const sas::ContentId& moduleId) { return content::ModuleContentCatalog::FindById(moduleId); },
			content::ModuleContentCatalog::GetZoneTable()
		};
		auto level = std::make_shared<HeadlessModuleLevel>(&application);
		level->BeginPlayInternal();
		const shared_ptr<PlayerSpaceShip> source = level->SpawnActor<PlayerSpaceShip>(shipDefinition).lock();
		const shared_ptr<PlayerSpaceShip> target = level->SpawnActor<PlayerSpaceShip>(shipDefinition).lock();
		report.Check(source && target, "Full Magazine fixture uses real player ship owners");
		level->TickInternal(0.f);
		source->SetActorLocation({ -50000.f, -50000.f });
		target->SetActorLocation({ -40000.f, -50000.f });
		source->SetInvulnerability(false);
		target->SetInvulnerability(false);
		target->SetCollisionLayer(vega::GameCollision::Enemy);
		target->SetCollisionMask(vega::GameCollision::PlayerBullet);
		target->GetHealthComponent().SetMaxHealth(5000.f, true);

		ModuleLoadout loadout;
		report.Check(loadout.SeedDefaults(rules), "Full Magazine fixture seeds the real default loadout");
		const std::uint32_t fullMagazineId = AddAndInstall(report, loadout, rules, "Module.Primary.FullMagazine.Basic");
		report.Check(source->GetModuleApplier().Apply(loadout, rules), "Full Magazine applies to the real source combat runtime");
		const ModuleDefinition* module = FindModule(rules, "Module.Primary.FullMagazine.Basic");
		report.Check(module && !module->sizeVariants.empty() && module->sizeVariants.front().damageRules.size() == 1,
			"catalog resolves one typed FullMagazineShot rule");
		const ModuleDamageRule& rule = module->sizeVariants.front().damageRules.front();
		report.Check(rule.condition == ModuleDamageCondition::FullMagazineShot,
			"Full Magazine loader preserves its typed shot condition");
		report.Check(rule.openingShotCount == 3, "Full Magazine loads a three-shot opening window");
		report.CheckValue("Full Magazine resolves its authored 1.3 multiplier", 1.3f,
			source->GetCombatRuntime().ResolveFullMagazineShotMultiplier(0), "M11 installed on the real source");
		report.CheckValue("generic outgoing rules exclude FullMagazineShot", 1.f,
			source->GetCombatRuntime().ResolveOutgoingDamagePlan(*target, { GameplayTags::Ability::Primary }).damageMultiplier,
			"the full-magazine multiplier is snapshotted before impact");
		const ModulePlacement installedModule = FindInstalled(loadout, fullMagazineId);
		const std::uint32_t duplicateId = loadout.AddToInventory(module->moduleId);
		report.Check(loadout.Install(rules, duplicateId, installedModule.origin, installedModule.rotationQuarterTurns) ==
			ModulePlacementFailure::DuplicateModule && loadout.Remove(duplicateId),
			"loadout rejects a second installed copy of Full Magazine");

		const PrimaryWeaponDefinition* fighterCatalog = content::WeaponContentCatalog::FindById("Weapon.Projectile.FighterRapidLaser.Basic");
		report.Check(fighterCatalog && fighterCatalog->magazine.has_value(), "Fighter laser catalog weapon has a magazine");
		const PrimaryWeaponDefinition fighter = *fighterCatalog;
		sas::GameplayAttributeList fighterAttributes = fighter.attributes;
		PrimaryWeaponExecutionContext fighterContext{ *source, fighter, fighterAttributes };
		fighterContext.damageTags = fighter.damageTags;
		bool canContinue = true;
		fighterContext.shouldContinue = [&canContinue]() { return canContinue; };
		PrimaryWeaponRuntimeState fighterRuntime;
		report.Check(PrimaryWeaponExecutionSystem::InitializeRuntime(fighter, fighterRuntime).isValid,
			"Fighter laser initializes its real magazine runtime");
		PrimaryWeaponExecutionSystem::BeginFire(fighterContext, fighterRuntime);
		const int fullCapacity = fighter.magazine->capacity;
		canContinue = false;
		report.Check(!PrimaryWeaponExecutionSystem::FireOnce(fighterContext, fighterRuntime) &&
			fighterRuntime.magazineState.roundsRemaining == fullCapacity && fighterRuntime.magazineState.shotsFired == 0,
			"failed fire leaves a full magazine eligible for the next shot");
		canContinue = true;

		const float fighterBaseDamage = sas::FindAttributeValue(fighterAttributes, CommonAttributeIds::Damage, 0.f);
		auto projectileCount = [&level]()
		{
			return level->GetActorsByType<PrimaryWeaponProjectileActor>().size();
		};
		auto latestProjectile = [&level]() -> shared_ptr<PrimaryWeaponProjectileActor>
		{
			const auto projectiles = level->GetActorsByType<PrimaryWeaponProjectileActor>();
			return projectiles.empty() ? shared_ptr<PrimaryWeaponProjectileActor>{} : projectiles.back().lock();
		};
		auto firstProjectile = shared_ptr<PrimaryWeaponProjectileActor>{};
		report.Check(PrimaryWeaponExecutionSystem::FireOnce(fighterContext, fighterRuntime),
			"full-magazine Fighter shot fires through PrimaryWeaponExecutionSystem");
		level->TickInternal(0.f);
		firstProjectile = latestProjectile();
		report.Check(firstProjectile != nullptr, "full-magazine shot spawns a real delayed projectile");
		report.CheckValue("first full-magazine projectile snapshots x1.3", 1.3f,
			firstProjectile->GetShotMetadata().shotDamageMultiplier, "48 of 48 rounds before FireOnce debit");
		report.CheckValue("first full-magazine shot deals 30 percent more damage", fighterBaseDamage * 1.3f,
			firstProjectile->GetDamage(), "projectile damage captured at spawn");
		report.Check(fighterRuntime.magazineState.roundsRemaining == fullCapacity - 1,
			"successful first shot spends ammo after taking its snapshot");

		const std::size_t beforeSecondShot = projectileCount();
		report.Check(PrimaryWeaponExecutionSystem::FireOnce(fighterContext, fighterRuntime),
			"second Fighter shot fires after the full-magazine shot");
		level->TickInternal(0.f);
		const shared_ptr<PrimaryWeaponProjectileActor> secondProjectile = latestProjectile();
		report.Check(projectileCount() == beforeSecondShot + 1 && secondProjectile,
			"second Fighter shot creates one projectile");
		report.CheckValue("second opening shot keeps its bonus", 1.3f,
			secondProjectile->GetShotMetadata().shotDamageMultiplier, "47 of 48 rounds before FireOnce debit");
		report.CheckValue("second opening shot deals boosted damage", fighterBaseDamage * 1.3f,
			secondProjectile->GetDamage(), "one round has already been spent");
		report.CheckValue("delayed first projectile keeps its original snapshot", fighterBaseDamage * 1.3f,
			firstProjectile->GetDamage(), "second shot was fired before first projectile impact");
		PrimaryWeaponExecutionSystem::EndFire(fighterContext, fighterRuntime);
		PrimaryWeaponExecutionSystem::BeginFire(fighterContext, fighterRuntime);
		for (int shot = 3; shot <= 4; ++shot)
		{
			report.Check(PrimaryWeaponExecutionSystem::FireOnce(fighterContext, fighterRuntime), "opening-window boundary Fighter shot fires");
			level->TickInternal(0.f);
			const auto projectile = latestProjectile();
			report.Check(projectile != nullptr, "opening-window boundary spawns a real projectile");
			const float expected = shot == 3 ? 1.3f : 1.f;
			report.CheckValue("Fighter third boosted and fourth ordinary shot", expected,
				projectile->GetShotMetadata().shotDamageMultiplier, "shot " + std::to_string(shot) + "; releasing fire did not reset the window");
			report.CheckValue("Fighter boundary damage matches its shot snapshot", fighterBaseDamage * expected,
				projectile->GetDamage(), "shot " + std::to_string(shot));
		}

		bool drained = true;
		for (int shot = 0; shot < fullCapacity - 4; ++shot)
		{
			if (!PrimaryWeaponExecutionSystem::FireOnce(fighterContext, fighterRuntime))
			{
				drained = false;
				break;
			}
			level->TickInternal(0.f);
		}
		report.Check(drained && fighterRuntime.magazineState.roundsRemaining == 0 &&
			fighterRuntime.magazineState.reloadRemaining > 0.0,
			"real successful FireOnce calls drain the magazine and start reload");
		report.Check(!PrimaryWeaponExecutionSystem::FireOnce(fighterContext, fighterRuntime) &&
			fighterRuntime.magazineState.roundsRemaining == 0,
			"empty magazine rejects fire without spending extra ammo");
		PrimaryWeaponExecutionSystem::AdvanceMagazineReload(fighter, fighterRuntime,
			static_cast<float>(fighterRuntime.magazineState.reloadRemaining) + 0.01f);
		report.Check(fighterRuntime.magazineState.roundsRemaining == fullCapacity &&
			fighterRuntime.magazineState.reloadRemaining == 0.0 && fighterRuntime.magazineState.shotsFired == 0,
			"completed reload restores the full magazine");
		report.Check(PrimaryWeaponExecutionSystem::FireOnce(fighterContext, fighterRuntime),
			"first shot after reload fires through the same runtime");
		level->TickInternal(0.f);
		const shared_ptr<PrimaryWeaponProjectileActor> reloadedProjectile = latestProjectile();
		report.CheckValue("reload makes the next shot full-magazine eligible again", 1.3f,
			reloadedProjectile->GetShotMetadata().shotDamageMultiplier, "rounds restored to magazine capacity");
		for (int shot = 2; shot <= 4; ++shot)
		{
			report.Check(PrimaryWeaponExecutionSystem::FireOnce(fighterContext, fighterRuntime), "reloaded opening-window shot fires");
			level->TickInternal(0.f);
			report.CheckValue("reload restores exactly three boosted shots", shot <= 3 ? 1.3f : 1.f,
				latestProjectile()->GetShotMetadata().shotDamageMultiplier, "reloaded shot " + std::to_string(shot));
		}

		const PrimaryWeaponDefinition* dualCatalog = content::WeaponContentCatalog::FindById("Weapon.Projectile.DualKineticBlaster.Basic");
		report.Check(dualCatalog && dualCatalog->magazine.has_value() && dualCatalog->muzzleDefinitions.size() == 2,
			"Dual Kinetic Blaster keeps its shipped two-muzzle magazine configuration");
		const PrimaryWeaponDefinition dual = *dualCatalog;
		sas::GameplayAttributeList dualAttributes = dual.attributes;
		PrimaryWeaponExecutionContext dualContext{ *source, dual, dualAttributes };
		dualContext.damageTags = dual.damageTags;
		PrimaryWeaponRuntimeState dualRuntime;
		report.Check(PrimaryWeaponExecutionSystem::InitializeRuntime(dual, dualRuntime).isValid,
			"Dual Kinetic Blaster initializes its real magazine runtime");
		PrimaryWeaponExecutionSystem::BeginFire(dualContext, dualRuntime);
		const std::size_t beforeDualShot = projectileCount();
		report.Check(PrimaryWeaponExecutionSystem::FireOnce(dualContext, dualRuntime) &&
			dualRuntime.magazineState.roundsRemaining == dual.magazine->capacity - 2,
			"full-magazine multi-muzzle shot fires and spends one round per muzzle");
		level->TickInternal(0.f);
		const auto allProjectilesAfterDual = level->GetActorsByType<PrimaryWeaponProjectileActor>();
		report.Check(allProjectilesAfterDual.size() == beforeDualShot + 2,
			"multi-muzzle shot creates both real projectiles");
		const float dualBaseDamage = sas::FindAttributeValue(dualAttributes, CommonAttributeIds::Damage, 0.f);
		for (std::size_t index = beforeDualShot; index < allProjectilesAfterDual.size(); ++index)
		{
			const shared_ptr<PrimaryWeaponProjectileActor> projectile = allProjectilesAfterDual[index].lock();
			report.Check(projectile != nullptr, "multi-muzzle projectile remains owned by the world");
			report.CheckValue("both muzzles share the full-magazine shot snapshot", 1.3f,
				projectile->GetShotMetadata().shotDamageMultiplier, "same FireOnce metadata across muzzles");
			report.CheckValue("both muzzles apply the multiplier once", dualBaseDamage * 1.3f,
				projectile->GetDamage(), "shared per-shot metadata");
		}

		for (int shot = 2; shot <= 4; ++shot)
		{
			const std::size_t beforeShot = projectileCount();
			report.Check(PrimaryWeaponExecutionSystem::FireOnce(dualContext, dualRuntime), "Dual opening-window boundary fires");
			level->TickInternal(0.f);
			const auto projectiles = level->GetActorsByType<PrimaryWeaponProjectileActor>();
			report.Check(projectiles.size() == beforeShot + 2 && dualRuntime.magazineState.shotsFired == shot,
				"two muzzles consume one opening-window shot, not two");
			for (std::size_t index = beforeShot; index < projectiles.size(); ++index)
				report.CheckValue("Dual first three volleys boosted and fourth ordinary", shot <= 3 ? 1.3f : 1.f,
					projectiles[index].lock()->GetShotMetadata().shotDamageMultiplier, "volley " + std::to_string(shot));
		}
		const PrimaryWeaponDefinition* shotgunCatalog = content::WeaponContentCatalog::FindById("Weapon.Projectile.EnemyTwinBladeScatter.Basic");
		report.Check(shotgunCatalog && !shotgunCatalog->magazine.has_value(),
			"shipped shotgun remains a no-magazine weapon");
		PrimaryWeaponDefinition magazineShotgun = *shotgunCatalog;
		magazineShotgun.magazine = PrimaryWeaponMagazineDefinition{ 3, 1.f };
		sas::GameplayAttributeList shotgunAttributes = magazineShotgun.attributes;
		PrimaryWeaponExecutionContext shotgunContext{ *source, magazineShotgun, shotgunAttributes };
		shotgunContext.damageTags = magazineShotgun.damageTags;
		PrimaryWeaponRuntimeState shotgunRuntime;
		report.Check(PrimaryWeaponExecutionSystem::InitializeRuntime(magazineShotgun, shotgunRuntime).isValid,
			"shotgun test copy initializes with an explicit magazine");
		PrimaryWeaponExecutionSystem::BeginFire(shotgunContext, shotgunRuntime);
		const std::size_t beforeShotgun = projectileCount();
		report.Check(PrimaryWeaponExecutionSystem::FireOnce(shotgunContext, shotgunRuntime),
			"magazine-backed shotgun test copy fires through its shipped falloff handler");
		level->TickInternal(0.f);
		const auto allProjectilesAfterShotgun = level->GetActorsByType<PrimaryWeaponProjectileActor>();
		report.Check(allProjectilesAfterShotgun.size() == beforeShotgun + 3,
			"shotgun volley creates its three real pellets");
		const float shotgunBaseDamage = sas::FindAttributeValue(shotgunAttributes, CommonAttributeIds::Damage, 0.f);
		for (std::size_t index = beforeShotgun; index < allProjectilesAfterShotgun.size(); ++index)
		{
			const shared_ptr<PrimaryWeaponProjectileActor> projectile = allProjectilesAfterShotgun[index].lock();
			report.Check(projectile != nullptr, "shotgun pellet remains owned by the world");
			report.CheckValue("shotgun pellets carry the full-magazine shot metadata", 1.3f,
				projectile->GetShotMetadata().shotDamageMultiplier, "shotgun FireOnce begins with 3 of 3 rounds");
		}
		DamageCapture shotgunDamage;
		const DelegateHandle shotgunDamageHandle = source->GetCombatRuntime().onDamageDealt.BindAction(
			&shotgunDamage, &DamageCapture::OnDamage);
		const shared_ptr<PrimaryWeaponProjectileActor> firstShotgunPellet = allProjectilesAfterShotgun[beforeShotgun].lock();
		const shared_ptr<PrimaryWeaponProjectileActor> secondShotgunPellet = allProjectilesAfterShotgun[beforeShotgun + 1].lock();
		report.Check(firstShotgunPellet && secondShotgunPellet, "two shotgun pellets remain alive before delayed impacts");
		firstShotgunPellet->OnActorBeginOverlap(target.get());
		report.Check(shotgunDamage.records.size() == 1, "first shared-group pellet applies one damage event",
			Json{ { "damageEvents", shotgunDamage.records.size() } });
		secondShotgunPellet->OnActorBeginOverlap(target.get());
		source->GetCombatRuntime().onDamageDealt.UnbindAction(shotgunDamageHandle);
		report.Check(shotgunDamage.records.size() == 2,
			"shared shotgun impact group applies two pellet impacts to one target",
			Json{ { "damageEvents", shotgunDamage.records.size() } });
		report.CheckValue("shotgun falloff begins from the 1.3x full-magazine damage once", shotgunBaseDamage * 1.3f,
			shotgunDamage.records[0].originalDamage, "first target pellet, no falloff");
		report.CheckValue("shotgun falloff applies after the captured multiplier", shotgunBaseDamage * 1.3f * 0.9f,
			shotgunDamage.records[1].originalDamage, "second target pellet, 10 percent falloff");
		level->TickInternal(0.f);

		const PrimaryWeaponDefinition noMagazineShotgun = *shotgunCatalog;
		sas::GameplayAttributeList noMagazineAttributes = noMagazineShotgun.attributes;
		PrimaryWeaponExecutionContext noMagazineContext{ *source, noMagazineShotgun, noMagazineAttributes };
		noMagazineContext.damageTags = noMagazineShotgun.damageTags;
		PrimaryWeaponRuntimeState noMagazineRuntime;
		report.Check(PrimaryWeaponExecutionSystem::InitializeRuntime(noMagazineShotgun, noMagazineRuntime).isValid,
			"shipped no-magazine shotgun initializes unchanged");
		PrimaryWeaponExecutionSystem::BeginFire(noMagazineContext, noMagazineRuntime);
		const std::size_t beforeNoMagazineShot = projectileCount();
		report.Check(PrimaryWeaponExecutionSystem::FireOnce(noMagazineContext, noMagazineRuntime),
			"shipped no-magazine shotgun fires while Full Magazine is installed");
		level->TickInternal(0.f);
		const auto noMagazineProjectiles = level->GetActorsByType<PrimaryWeaponProjectileActor>();
		report.Check(noMagazineProjectiles.size() == beforeNoMagazineShot + 3,
			"no-magazine shotgun creates its usual pellet count");
		for (std::size_t index = beforeNoMagazineShot; index < noMagazineProjectiles.size(); ++index)
		{
			const shared_ptr<PrimaryWeaponProjectileActor> projectile = noMagazineProjectiles[index].lock();
			report.Check(projectile != nullptr, "no-magazine shotgun pellet remains owned by the world");
			report.CheckValue("no-magazine weapon receives no Full Magazine multiplier", 1.f,
				projectile->GetShotMetadata().shotDamageMultiplier, "unmodified shipped weapon has no magazine");
			report.CheckValue("no-magazine shotgun keeps its base damage", shotgunBaseDamage,
				projectile->GetDamage(), "Full Magazine eligibility requires a magazine definition");
		}

		const PrimaryWeaponDefinition* coneCatalog = content::WeaponContentCatalog::FindById("Weapon.Cone.RapidShotgun.Basic");
		report.Check(coneCatalog && coneCatalog->magazine.has_value(),
			"the shipped Cone Rapid Shotgun is the third magazine-backed player weapon");
		const PrimaryWeaponDefinition cone = *coneCatalog;
		sas::GameplayAttributeList coneAttributes = cone.attributes;
		PrimaryWeaponExecutionContext coneContext{ *source, cone, coneAttributes };
		coneContext.damageTags = cone.damageTags;
		PrimaryWeaponRuntimeState coneRuntime;
		report.Check(PrimaryWeaponExecutionSystem::InitializeRuntime(cone, coneRuntime).isValid,
			"Cone Rapid Shotgun initializes its shipped magazine runtime");
		PrimaryWeaponExecutionSystem::BeginFire(coneContext, coneRuntime);
		source->SetActorLocation({ 0.f, 0.f });
		target->SetActorLocation({ 0.f, -250.f });
		DamageCapture coneDamage;
		const DelegateHandle coneDamageHandle = source->GetCombatRuntime().onDamageDealt.BindAction(
			&coneDamage, &DamageCapture::OnDamage);
		report.Check(PrimaryWeaponExecutionSystem::FireOnce(coneContext, coneRuntime) &&
			coneRuntime.magazineState.roundsRemaining == cone.magazine->capacity - 1,
			"first Cone Rapid Shotgun shot fires and spends one round from full");
		report.Check(coneDamage.records.size() == 1,
			"Cone Rapid Shotgun reaches a real enemy-layer target");
		const float coneBaseDamage = sas::FindAttributeValue(coneAttributes, CommonAttributeIds::Damage, 0.f);
		report.CheckValue("Cone Rapid Shotgun full-magazine damage is multiplied once", coneBaseDamage * 1.3f,
			coneDamage.records.back().originalDamage, "first instant cone blast from a full magazine");
		coneDamage.records.clear();
		report.Check(PrimaryWeaponExecutionSystem::FireOnce(coneContext, coneRuntime) &&
			coneRuntime.magazineState.roundsRemaining == cone.magazine->capacity - 2,
			"next Cone Rapid Shotgun shot spends its next round");
		report.Check(coneDamage.records.size() == 1,
			"second Cone Rapid Shotgun shot reaches the same real target");
		report.CheckValue("Cone Rapid Shotgun second opening shot remains boosted", coneBaseDamage * 1.3f,
			coneDamage.records.back().originalDamage, "second shot begins below magazine capacity");
		for (int shot = 3; shot <= 4; ++shot)
		{
			coneDamage.records.clear();
			report.Check(PrimaryWeaponExecutionSystem::FireOnce(coneContext, coneRuntime) && coneDamage.records.size() == 1,
				"Cone opening-window boundary reaches the real target");
			report.CheckValue("Cone third boosted and fourth ordinary shot", coneBaseDamage * (shot == 3 ? 1.3f : 1.f),
				coneDamage.records.back().originalDamage, "cone shot " + std::to_string(shot));
		}
		source->GetCombatRuntime().onDamageDealt.UnbindAction(coneDamageHandle);

		report.Check(loadout.Uninstall(fullMagazineId) && source->GetModuleApplier().Apply(loadout, rules),
			"uninstalling Full Magazine clears its runtime multiplier");
		report.CheckValue("removed Full Magazine no longer affects future shots", 1.f,
			source->GetCombatRuntime().ResolveFullMagazineShotMultiplier(0), "module instance returned to inventory");
		report.Check(firstProjectile->GetShotMetadata().shotDamageMultiplier == 1.3f,
			"uninstall preserves an already-created shot's captured metadata");
		PrimaryWeaponRuntimeState removalRuntime;
		report.Check(PrimaryWeaponExecutionSystem::InitializeRuntime(fighter, removalRuntime).isValid,
			"post-removal fixture starts inside a fresh opening-shot window");
		PrimaryWeaponExecutionSystem::BeginFire(fighterContext, removalRuntime);
		const std::size_t beforeAfterRemoval = projectileCount();
		report.Check(PrimaryWeaponExecutionSystem::FireOnce(fighterContext, removalRuntime),
			"Fighter fires again after Full Magazine is removed");
		level->TickInternal(0.f);
		const shared_ptr<PrimaryWeaponProjectileActor> afterRemovalProjectile = latestProjectile();
		report.Check(projectileCount() == beforeAfterRemoval + 1 && afterRemovalProjectile,
			"post-removal Fighter shot creates one projectile");
		report.CheckValue("removed module does not affect new-shot metadata", 1.f,
			afterRemovalProjectile->GetShotMetadata().shotDamageMultiplier, "Full Magazine registration cleared");
		report.CheckValue("removed module leaves new-shot damage at base", fighterBaseDamage,
			afterRemovalProjectile->GetDamage(), "Full Magazine registration cleared");
		DamageCapture delayedImpact;
		const DelegateHandle delayedImpactHandle = source->GetCombatRuntime().onDamageDealt.BindAction(
			&delayedImpact, &DamageCapture::OnDamage);
		firstProjectile->OnActorBeginOverlap(target.get());
		source->GetCombatRuntime().onDamageDealt.UnbindAction(delayedImpactHandle);
		report.Check(delayedImpact.records.size() == 1,
			"captured projectile applies one real impact after later shots, reload, and module removal");
		report.CheckValue("delayed impact keeps the captured full-magazine damage once", std::ceil(fighterBaseDamage * 1.3f),
			delayedImpact.records.front().originalDamage, "first projectile impacts after module removal");
	}


}
