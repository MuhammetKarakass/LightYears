#pragma once

#include "gameConfigs/CollisionLayers.h"
#include "UncommonModuleP5E2E.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/content/ModuleSideFeatureLoader.h"
#include "gameplay/module/generated/ModuleGuidedProjectileActor.h"
#include "gameplay/module/generated/ModuleRicochetProjectileActor.h"
#include "gameplay/module/generated/ModuleMineActor.h"
#include "gameplay/module/generated/ModuleDelayedExplosionActor.h"
#include "gameplay/movement/MovementInfluenceService.h"
#include "presentation/ability/PresentationProfileRegistry.h"

namespace uncommon_p6_e2e
{
	using namespace uncommon_p5_e2e;
	template<typename T> List<shared_ptr<T>> Actors(Fixture& f)
	{
		List<shared_ptr<T>> actors;
		for (const auto& weak : f.level->GetActorsByTypeIncludingPending<T>()) if (const auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy()) actors.push_back(actor);
		return actors;
	}
	struct DamageObserver
	{
		SpaceShip& ship;
		DelegateHandle handle;
		int normal = 0;
		int generated = 0;
		bool allSuppressed = true;
		explicit DamageObserver(SpaceShip& owner) : ship{ owner }, handle{ ship.GetCombatRuntime().onDamageDealt.BindAction(this, &DamageObserver::OnDamage) } {}
		~DamageObserver() { ship.GetCombatRuntime().onDamageDealt.UnbindAction(handle); }
		void OnDamage(const DamageContext& context)
		{
			if (context.payload.moduleTriggerPolicy == DamageModuleTriggerPolicy::Suppressed)
			{
				++generated;
				allSuppressed = allSuppressed && context.payload.skipModuleDamageRules && !context.wasCritical;
			}
			else ++normal;
		}
	};
	inline shared_ptr<PrimaryWeaponProjectileActor> PrimaryHit(Fixture& f, SpaceShip& target, const PrimaryWeaponShotMetadata& metadata, float damage = 100.f, bool finishRicochets = true)
	{
		const auto* weapon = content::WeaponContentCatalog::FindById("Weapon.Projectile.FighterRapidLaser.Basic");
		f.report.Check(weapon != nullptr, "P6 primary delivery uses shipped weapon content");
		const auto projectile = f.level->SpawnActor<PrimaryWeaponProjectileActor>(f.ship.get(), weapon->presentationDefinition, weapon->attributes).lock();
		f.report.Check(projectile != nullptr, "P6 spawns a real Primary projectile");
		projectile->SetDamage(damage);
		projectile->SetDamageTags({});
		projectile->SetDamagePayload(Plain());
		projectile->SetShotMetadata(metadata);
		target.SetCollisionMask(ly::GameCollision::PlayerBullet);
		for (const auto& enemy : Actors<SpaceShip>(f)) if (enemy->GetCollisionLayer() == ly::GameCollision::Enemy) enemy->SetCollisionMask(ly::GameCollision::PlayerBullet);
		projectile->OnActorBeginOverlap(&target);
		if (finishRicochets)
			for (int step = 0; step < 200; ++step)
				for (const auto& bounce : Actors<ModuleRicochetProjectileActor>(f)) if (bounce->HasSelectedTarget()) bounce->Tick(.01f);
		return projectile;
	}
	inline GameAbility& CastOffense(Fixture& f)
	{
		auto* ability = f.ship->GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::Ability3);
		f.report.Check(ability != nullptr, "P6 uses a real shipped Offense ability");
		if (ability->IsActive()) ability->Cancel(sas::AbilityEndReason::Completed);
		ability->Tick(100.f);
		f.report.Check(ability->TryActivate(), "P6 activates Offense through the real ASC");
		f.report.Check(ability->GetDamageHitGroup() != nullptr, "successful use creates a shared damage budget");
		if (ability->IsActive()) ability->Cancel(sas::AbilityEndReason::Completed);
		return *ability;
	}
	inline float OffenseHit(Fixture& f, SpaceShip& target, GameAbility& ability, float damage = 100.f, std::shared_ptr<DamageHitGroup> group = {})
	{
		auto payload = Plain(); payload.hitGroup = group ? group : ability.GetDamageHitGroup();
		return Hit(target, f.ship.get(), damage, payload, ability.GetDefinition().abilityId, ability.GetDefinition().abilityTags);
	}
	inline void CastMovement(Fixture& f)
	{
		auto& abilities = f.ship->GetAbilitySystemComponent();
		auto* ability = abilities.GetAbilityById("Ability.Movement.Dash.Basic");
		if (!ability)
		{
			if (auto* current = abilities.GetAbility(sas::AbilitySlot::Ability2)) f.report.Check(abilities.RemoveAbility(current->GetHandle()), "P6 replaces only the fixture's E ability");
			const auto* authored = content::AbilityContentCatalog::FindById("Ability.Movement.Dash.Basic");
			f.report.Check(authored != nullptr, "P6 uses shipped Dash content");
			auto definition = *authored; definition.slot = sas::AbilitySlot::Ability2;
			f.report.Check(abilities.GrantAbility(definition).IsValid(), "P6 grants real Movement ability to the fixture");
			ability = abilities.GetAbilityById("Ability.Movement.Dash.Basic");
		}
		if (ability->IsActive()) ability->Cancel(sas::AbilityEndReason::Completed);
		ability->Tick(100.f);
		f.report.Check(ability->TryActivate(), "P6 starts mine window through real Movement activation");
		if (ability->IsActive()) ability->Cancel(sas::AbilityEndReason::Completed);
	}
	inline void Move(Fixture& f, float distance, float seconds = .1f)
	{
		const auto source = f.ship->GetUniqueID();
		f.report.Check(movement::MovementInfluenceService::SetForcedMovement(*f.ship, { source, { distance / seconds, 0.f }, 1 }), "P6 movement travels through the real component");
		f.ship->GetMovementComponent().Tick(seconds);
		f.ship->GetModuleApplier().Tick(seconds);
		movement::MovementInfluenceService::RemoveSource(*f.ship, source);
	}
	void RicochetChecks(Report& report, Application& application);
	void BalanceChecks(Report& report, Application& application);
	void ActorChecks(Report& report, Application& application);
	void MineChecks(Report& report, Application& application);
	void DeathBurstChecks(Report& report, Application& application);
	void SizingChecks(Report& report, Application& application);
	void ContentChecks(Report& report, const std::filesystem::path& artifactRoot);
}
