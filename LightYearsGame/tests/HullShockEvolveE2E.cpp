#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"
#include "gameplay/ability/hullShock/HullShockContracts.h"
#include "gameplay/ability/hullShock/HullShockDischarge.h"
#include "gameplay/ability/aegisReaver/AegisReaverFlightState.h"
#include "gameplay/ability/aegisReaver/AegisReaverProjectileActor.h"
#include "gameplay/ability/temporalConvergence/TemporalConvergenceFieldActor.h"
#include "gameplay/targeting/SectorGeometry.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "presentation/ability/hullShock/HullShockConePresentation.h"
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Image.hpp>

namespace
{
	using namespace vega;
	using common_module_e2e::Report;
	using common_module_e2e::HeadlessModuleLevel;
	using H = AbilityData::HullShock::Evolve;
	constexpr auto Q = sas::AbilitySlot::Ability1;
	constexpr const char* HullId = AbilityData::HullShock::AbilityId::Basic;
	void Equal(Report& report, const std::string& name, float expected, float actual)
	{
		report.CheckValue(name, expected, actual, "real Player / ship / ASC");
	}
	template<class T> shared_ptr<T> Latest(World& world)
	{
		shared_ptr<T> found;
		for (const auto& weak : world.GetActorsByTypeIncludingPending<T>())
			if (const auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy() && (!found || actor->GetUniqueID() > found->GetUniqueID())) found = actor;
		return found;
	}
	struct Fixture
	{
		Report& report;
		Player& player;
		shared_ptr<HeadlessModuleLevel> level;
		shared_ptr<PlayerSpaceShip> ship;
		Fixture(Report& result, Application& app) : report{ result }, player{ PlayerManager::GetPlayerManager().CreateNewPlayer() }, level{ std::make_shared<HeadlessModuleLevel>(&app) }
		{
			ship = player.SpawnSpaceShip(level.get()).lock();
			report.Check(ship != nullptr, "spawn real Player ship");
			level->BeginPlayInternal(); level->TickInternal(0.f);
			ship->SetInvulnerability(false); ship->SetUseScreenClamp(false); ship->SetEnablePhysics(false);
			ship->SetActorLocation({ 10000.f, 10000.f }); ship->SetActorRotation(0.f);
			ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 0.f);
			ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::EnergyPower, 100.f);
			player.AwardScrap(1000000);
			Equip(HullId);
		}
		~Fixture() { PlayerManager::GetPlayerManager().Reset(); }
		LightYearsAbilitySystemComponent& ASC() { return ship->GetAbilitySystemComponent(); }
		GameAbility& Ability() { return *ASC().GetAbility(Q); }
		void Equip(const char* id)
		{
			if (auto* current = ASC().GetAbility(Q)) report.Check(ASC().RemoveAbility(current->GetHandle()), "replace test Q");
			if (auto* current = ASC().GetAbilityById(id)) report.Check(ASC().RemoveAbility(current->GetHandle()), "remove duplicate family binding");
			report.Check(ASC().GrantAbility(*content::AbilityContentCatalog::FindById(id), Q).IsValid(), "grant shipped ability in fixture Q");
		}
		void Level(int targetLevel)
		{
			while (Ability().GetLevel() < targetLevel) report.Check(player.TryPurchaseAbilityLevel(Q), "purchase real ability level");
		}
		void Choose(const char* first, const char* second = nullptr)
		{
			Level(8); report.Check(player.TryChooseAbilityEvolve(Q, first), "Player chooses first HullShock evolve at 8");
			if (second) { Level(15); report.Check(player.TryChooseAbilityEvolve(Q, second), "Player chooses second HullShock evolve at 15"); }
		}
		shared_ptr<SpaceShip> Enemy(sf::Vector2f offset)
		{
			const auto enemy = level->SpawnActor<SpaceShip>(content::ShipContentCatalog::GetPlayerFighterDefinition()).lock();
			level->TickInternal(0.f);
			enemy->SetEnablePhysics(false); enemy->SetCollisionLayer(GameCollision::Enemy); enemy->SetCollisionMask(GameCollision::Player | GameCollision::PlayerBullet);
			enemy->SetActorLocation(ship->GetActorLocation() + offset);
			enemy->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 0.f);
			enemy->GetShieldComponent().ChangeShield(-100000.f);
			enemy->GetHealthComponent().SetMaxHealth(10000.f); enemy->GetHealthComponent().Regenerate(10000.f);
			return enemy;
		}
		void Start(sf::Vector2f aim = { 1.f, 0.f })
		{
			Ability().Tick(100.f);
			Ability().SetActivationAim(AbilityAimOverride{ aim, ship->GetActorLocation() + aim * 1000.f });
			report.Check(Ability().TryActivateWithInput(true), "activate HullShock through real ASC instance");
		}
		void Release(float charge)
		{
			if (charge > 0.f) Ability().Tick(charge);
			if (Ability().IsActive()) { Ability().SetInputHeld(false); Ability().Tick(0.f); }
		}
		float FullDamage(bool directed) const
		{
			const auto& asc = ship->GetAbilitySystemComponent();
			const float base = 30.f + 4.f * (asc.GetAbility(Q)->GetLevel() - 1) + ship->GetHealthComponent().GetMaxHealth() * .3f;
			return directed ? (base + asc.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower) * .30f) * 1.35f : base;
		}
	};
	struct HitRecorder
	{
		weak_ptr<SpaceShip> target;
		DelegateHandle handle;
		List<float> raw;
		List<float> applied;
		List<List<GameplayTag>> tags;
		explicit HitRecorder(const shared_ptr<SpaceShip>& ship) : target{ ship }, handle{ ship->GetCombatRuntime().onDamageResolved.BindAction(this, &HitRecorder::Hit) } {}
		~HitRecorder() { if (const auto ship = target.lock()) ship->GetCombatRuntime().onDamageResolved.UnbindAction(handle); }
		void Hit(const DamageContext& context) { raw.push_back(context.originalDamage); applied.push_back(context.appliedDamage); tags.push_back(context.damageTags); }
	};
	struct UseRecorder
	{
		weak_ptr<PlayerSpaceShip> ship;
		AbilityLifecycleObserverHandle handle;
		int activations = 0;
		explicit UseRecorder(const shared_ptr<PlayerSpaceShip>& owner) : ship{ owner }
		{
			AbilityLifecycleObserverFilter filter; filter.eventTag = GameplayTags::Event::Ability::Activated;
			handle = owner->GetAbilitySystemComponent().RegisterAbilityLifecycleObserver(filter, [this](const sas::AbilityLifecycleEvent&) { ++activations; });
		}
		~UseRecorder() { if (const auto owner = ship.lock()) owner->GetAbilitySystemComponent().UnregisterAbilityLifecycleObserver(handle); }
	};
	int Electric(SpaceShip& ship) { return uncommon_p5_e2e::Stacks(ship, DamageStatusEffectIds::ElectricEffectId); }
	void SelectionAndBase(Report& report, Application& app)
	{
		Fixture f{ report, app };
		const auto* definition = content::AbilityContentCatalog::FindById(HullId);
		report.Check(definition && definition->evolveContent && definition->evolveContent->options.size() == 3 && definition->evolveContent->combinations.size() == 10, "all shipped HullShock evolve combinations load");
		f.Level(7); report.Check(!f.player.TryChooseAbilityEvolve(Q, H::DirectedDischarge), "no evolve selection before level 8");
		f.Level(8); report.Check(f.Ability().GetPendingEvolve().options.size() == 3, "level 8 offers three evolves");
		const auto front = f.Enemy({ 250.f, 0.f }), back = f.Enemy({ -250.f, 0.f }), far = f.Enemy({ 650.f, 0.f });
		HitRecorder a{ front }, b{ back }, c{ far };
		const float damage = f.FullDamage(false);
		f.Start(); f.Release(0.f);
		Equal(report, "base minimum-charge damage stays at 25 percent", damage * .25f, a.applied.back());
		Equal(report, "base remains circular", a.applied.back(), b.applied.back());
		report.Check(c.raw.empty() && Electric(*front) == 1, "base minimum radius and one Electric stack unchanged");
		report.Check(f.player.TryChooseAbilityEvolve(Q, H::DirectedDischarge), "first evolve selected through Player");
		report.Check(!f.player.TryChooseAbilityEvolve(Q, H::ReactiveCapacitor), "second tier cannot be selected early");
		f.Level(15); report.Check(f.Ability().GetPendingEvolve().options.size() == 2, "level 15 offers remaining two evolves");
		report.Check(f.player.TryChooseAbilityEvolve(Q, H::AftershockResonance) && !f.player.TryChooseAbilityEvolve(Q, H::ReactiveCapacitor), "two evolves close third option for this run");
	}
	void Directed(Report& report, Application& app)
	{
		for (const float charge : { 0.f, 1.25f, 2.f })
		{
			Fixture f{ report, app }; f.Choose(H::DirectedDischarge);
			const float radius = charge == 0.f ? 400.f : 800.f;
			const auto inside = f.Enemy({ radius - 70.f, 0.f }), outside = f.Enemy({ radius + 120.f, 0.f }), behind = f.Enemy({ -200.f, 0.f }), side = f.Enemy({ 0.f, 200.f });
			HitRecorder a{ inside }, b{ outside }, c{ behind }, d{ side };
			f.Start(); f.Release(charge);
			report.Check(a.raw.size() == 1 && b.raw.empty() && c.raw.empty() && d.raw.empty(), "directed sector hits inside, misses behind/side/outside at each charge");
			Equal(report, "directed damage includes fixed EP coefficient and 1.35 multiplier", f.FullDamage(true) * (.25f + .75f * charge / 2.f), a.applied.front());
			report.Check(Electric(*inside) == (charge == 0.f ? 1 : charge < 1.8f ? 3 : 4), "Directed preserves Electric charge thresholds");
			if (charge == 2.f)
			{
				const auto visual = Latest<HullShockConeVisualActor>(*f.level);
				sf::RenderTexture texture{ { 512u, 512u } }; sf::View view; view.setCenter(f.ship->GetActorLocation()); view.setSize({ 1900.f, 1900.f }); texture.setView(view);
				texture.clear(sf::Color{ 8, 12, 22 }); visual->Render(texture); texture.display();
				report.Check(texture.getTexture().copyToImage().saveToFile(std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "outputs/hullshock-cone.png"), "render cone impact presentation artifact");
			}
		}
		Fixture f{ report, app }; f.Choose(H::DirectedDischarge); const auto enemy = f.Enemy({ -300.f, 0.f }); HitRecorder damage{ enemy };
		f.Ability().SetActivationAim(std::nullopt);
		f.ASC().GetActivationPolicy().ConfigureAutocast(Q, 1234, AbilityAutocastSpec{ 3.f, 1000.f });
		f.ASC().GetActivationPolicy().Tick(.01f);
		report.Check(f.Ability().IsActive() && f.Ability().GetActivationAim() && f.Ability().GetActivationAim()->direction.x < -.9f, "Autocast obtains opposing aim even when ship faces away");
		f.Release(2.f); report.Check(damage.raw.size() == 1, "Directed uses real Autocast aim");
	}
	void Capacitor(Report& report, Application& app)
	{
		for (const auto* ending : { "release", "empty", "cancel", "stun", "cap" })
		{
			Fixture f{ report, app }; f.Choose(H::ReactiveCapacitor);
			if (std::string{ ending } != "empty") f.Enemy({ 100.f, 0.f });
			auto& shield = f.ship->GetShieldComponent();
			shield.ChangeShield(-shield.GetShield()); shield.ChangeShield(shield.GetMaxShield());
			const float incoming = std::string{ ending } == "cap" ? 200.f : 20.f;
			f.Start(); const float before = shield.GetShield();
			uncommon_p4_e2e::Hit(*f.ship, nullptr, incoming, uncommon_p4_e2e::Plain());
			Equal(report, "Capacitor reduces incoming damage during charge including shield absorption", incoming * .70f, before - shield.GetShield());
			const float taken = incoming * .70f;
			// Restore normal shield as a test resource correction so the gain must become overshield.
			shield.ChangeShield(shield.GetMaxShield());
			if (std::string{ ending } == "cancel") f.Ability().Cancel(sas::AbilityEndReason::Interrupted);
			else if (std::string{ ending } == "stun") { f.ASC().AddOwnedTag(GameplayTags::State::Effect::Control::Stunned); f.Ability().Tick(.01f); f.ASC().RemoveOwnedTag(GameplayTags::State::Effect::Control::Stunned); }
			else f.Release(1.f);
			const bool gain = std::string{ ending } == "release" || std::string{ ending } == "cap";
			const float expected = gain ? std::min(taken * .60f, f.ship->GetHealthComponent().GetMaxHealth() * .20f) : 0.f;
			Equal(report, std::string{ "Capacitor gain condition: " } + ending, expected, shield.GetOvershield());
			shield.TickTemporaryOvershields(5.f); Equal(report, "Capacitor uses shared five-second hold", expected, shield.GetOvershield());
			shield.TickTemporaryOvershields(.01f); Equal(report, "Capacitor uses shared 100/s decay", std::max(0.f, expected - 1.f), shield.GetOvershield());
			const float resource = shield.GetShield(); uncommon_p4_e2e::Hit(*f.ship, nullptr, 10.f, uncommon_p4_e2e::Plain());
			Equal(report, "damage reduction removed on every charge end path", 10.f, resource - shield.GetShield());
		}
		Fixture f{ report, app }; f.Choose(H::ReactiveCapacitor); f.Enemy({ 100.f, 0.f });
		auto& shield = f.ship->GetShieldComponent(); shield.ChangeShield(-100000.f);
		f.Start(); const float health = f.ship->GetHealthComponent().GetHealth(); uncommon_p4_e2e::Hit(*f.ship, nullptr, 20.f, uncommon_p4_e2e::Plain());
		Equal(report, "Capacitor also reduces real hull damage", 14.f, health - f.ship->GetHealthComponent().GetHealth());
		f.Release(0.f); Equal(report, "recorded hull damage converts to missing normal shield first", 8.4f, shield.GetShield());
	}
	void Aftershock(Report& report, Application& app, const char* first, const char* second)
	{
		Fixture f{ report, app }; f.Choose(first, second);
		const bool directed = f.Ability().HasEvolve(H::DirectedDischarge), capacitor = f.Ability().HasEvolve(H::ReactiveCapacitor);
		const auto firstTarget = f.Enemy({ 150.f, 0.f }); HitRecorder firstDamage{ firstTarget };
		UseRecorder uses{ f.ship };
		f.Start(); if (capacitor) uncommon_p4_e2e::Hit(*f.ship, nullptr, 20.f, uncommon_p4_e2e::Plain()); f.Release(.8f);
		const auto actor = Latest<HullShockAftershockActor>(*f.level);
		report.Check(actor && !f.Ability().IsActive(), "aftershock survives the ended ability");
		const auto history = f.ASC().GetAbilityUseHistory().GetRecords().size();
		const float shieldBefore = f.ship->GetShieldComponent().GetShield();
		f.ship->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ 2000.f, 0.f }); f.ship->SetActorRotation(90.f);
		const auto aim = AbilityActorSpawner::ResolveDirection(*f.ship, sas::AbilityDirectionPolicy::MouseWorld);
		const float chargedRadius = (directed ? 400.f : 300.f) * (1.f + std::log1p(2.f * (.8f / 1.25f)) / std::log(3.f));
		const auto current = f.Enemy(aim * (chargedRadius - 70.f)), behind = f.Enemy(aim * -200.f), outside = f.Enemy(aim * (chargedRadius + 120.f));
		HitRecorder currentDamage{ current }, behindDamage{ behind }, outsideDamage{ outside };
		actor->Tick(.799f); report.Check(currentDamage.raw.empty(), "aftershock waits the full 0.8 seconds");
		actor->Tick(.001f); report.Check(currentDamage.raw.size() == 1 && outsideDamage.raw.empty(), "aftershock uses moved ship position and charged radius");
		Equal(report, "aftershock raw damage is 45 percent of first shock", firstDamage.raw.front() * .45f, currentDamage.raw.front());
		report.Check(Electric(*current) == 1, "aftershock applies exactly one Electric stack");
		report.Check(directed ? behindDamage.raw.empty() : behindDamage.raw.size() == 1, "aftershock combination uses current cone direction or circle");
		report.Check(f.ASC().GetAbilityUseHistory().GetRecords().size() == history && f.ASC().GetActiveAbilityInvocationCount() == 0, "aftershock creates no ability-use or Echo history record/invocation");
		report.Check(uses.activations == 1, "aftershock cannot advance ability-use counters or Spark/Unique activation triggers");
		Equal(report, "aftershock cannot give Capacitor shield again", shieldBefore, f.ship->GetShieldComponent().GetShield());
		actor->Tick(1.f); report.Check(currentDamage.raw.size() == 1, "aftershock fires only once");
	}
	void LifetimeAndCapture(Report& report, Application& app)
	{
		Fixture f{ report, app }; f.Choose(H::AftershockResonance); f.Enemy({ 100.f, 0.f }); f.Start(); f.Release(2.f);
		const auto actor = Latest<HullShockAftershockActor>(*f.level); report.Check(actor != nullptr, "capture test obtains real delayed actor");
		report.Check(f.ASC().RemoveAbility(f.Ability().GetHandle()), "remove source ability before delayed tick");
		const auto target = f.Enemy({ 500.f, 0.f }); HitRecorder damage{ target };
		actor->Tick(.8f); report.Check(damage.raw.size() == 1, "captured delay, damage fraction, stacks and full radius survive source removal");
		Equal(report, "actor retains evolved numeric setting outside callback scope", .45f, actor->FindCapturedNumericSetting(AbilityData::HullShock::Setting::AftershockDamageFraction).value_or(-1.f));
		f.Equip(HullId); f.Start(); f.Ability().Cancel(sas::AbilityEndReason::Interrupted);
		// Directly exercise a weak-owner expiry as well as the pending-destroy path.
		HullShockDischarge spec; spec.radius = 600.f; spec.damage = 100.f; spec.abilityId = sas::ContentId{ HullId };
		const auto delayed = f.level->SpawnActor<HullShockAftershockActor>(f.ship.get(), spec, actor->GetCapturedAbilityConfiguration()).lock();
		f.ship->Destroy(); delayed->Tick(1.f); report.Check(delayed->GetIsPendingDestroy(), "delayed shock safely cancels when ship is destroyed");
	}
	void CombinedCapacitorAndConversion(Report& report, Application& app)
	{
		{
			Fixture f{ report, app }; f.Choose(H::DirectedDischarge, H::ReactiveCapacitor);
			const auto front = f.Enemy({ 200.f, 0.f }), back = f.Enemy({ -200.f, 0.f }); HitRecorder a{ front }, b{ back };
			f.Start(); const float before = f.ship->GetShieldComponent().GetShield(); uncommon_p4_e2e::Hit(*f.ship, nullptr, 20.f, uncommon_p4_e2e::Plain());
			Equal(report, "Directed plus Capacitor retains charge reduction", 14.f, before - f.ship->GetShieldComponent().GetShield());
			f.Release(2.f); report.Check(a.raw.size() == 1 && b.raw.empty(), "Directed plus Capacitor retains cone targeting");
			Equal(report, "Directed plus Capacitor retains EP scaling", f.FullDamage(true), a.raw.front());
			Equal(report, "Directed plus Capacitor returns recorded damage", before - 14.f + 8.4f, f.ship->GetShieldComponent().GetShield());
		}
		{
			Fixture f{ report, app }; f.Choose(H::AftershockResonance);
			// Element Core's binding owns this shared activation damage-type override.
			f.ASC().GetActivationPolicy().ConfigureDamageType(Q, DamageTypeSchema::Thermal);
			const auto enemy = f.Enemy({ 100.f, 0.f }); HitRecorder damage{ enemy };
			f.Start(); f.Release(0.f); const auto actor = Latest<HullShockAftershockActor>(*f.level);
			actor->Tick(.8f);
			report.Check(damage.raw.size() == 2 && damage.tags[0] == List<GameplayTag>{ DamageTypeSchema::Thermal } && damage.tags[1] == damage.tags[0], "aftershock retains Element Core conversion through the on-hit pipeline");
		}
	}
	void SharedRule(Report& report, Application& app)
	{
		Fixture f{ report, app }; ShieldComponent shield{ *f.ship, 60.f, 100.f };
		Equal(report, "shared grant fills normal shield then excess", 80.f, shield.GrantShield("A", 80.f)); Equal(report, "excess portion is tracked", 40.f, shield.GetOvershield());
		shield.TickTemporaryOvershields(5.f); Equal(report, "hold includes exactly five seconds", 140.f, shield.GetShield());
		shield.TickTemporaryOvershields(.01f); Equal(report, "continuous decay is 100 per second", 139.f, shield.GetShield());
		shield.GrantShield("A", 20.f); Equal(report, "same source keeps larger remaining value without stacking", 139.f, shield.GetShield());
		shield.TickTemporaryOvershields(5.f); Equal(report, "same source regrant restarts hold", 139.f, shield.GetShield());
		shield.GrantShield("A", 60.f); Equal(report, "same source larger grant refreshes to maximum", 160.f, shield.GetShield());
		shield.GrantShield("B", 30.f); Equal(report, "different sources stack", 190.f, shield.GetShield());
		shield.TickTemporaryOvershields(5.1f); Equal(report, "each source decays independently at the common rate", 170.f, shield.GetShield());
		shield.AbsorbDamage(75.f, 1.f, 0.f); Equal(report, "damage spends shared excess before normal shield", 95.f, shield.GetShield());
		shield.TickTemporaryOvershields(100.f); Equal(report, "spent entries cannot remove normal shield later", 95.f, shield.GetShield());
		ShieldComponent zeroCap{ *f.ship, 0.f, 0.f }; zeroCap.GrantShield("Zero", 20.f); zeroCap.TickTemporaryOvershields(5.1f);
		Equal(report, "zero normal capacity still follows the shared grant rule", 10.f, zeroCap.GetOvershield());
		zeroCap.ChangeShield(-100.f); zeroCap.GrantShield("Tiny", .0005f); zeroCap.TickTemporaryOvershields(5.f); zeroCap.TickTemporaryOvershields(1.f);
		report.Check(zeroCap.GetShield() == 0.f, "tiny grants cannot leave permanent untracked overshield");
		report.Check(targeting::CircleOverlapsSector({ 110.f, 0.f }, 10.f, {}, { 1.f, 0.f }, 100.f, .785398f) && !targeting::CircleOverlapsSector({ -30.f, 0.f }, 10.f, {}, { 1.f, 0.f }, 100.f, .785398f), "shared sector geometry includes arc overlap and excludes rear");
	}
	void SourceRules(Report& report, Application& app)
	{
		for (const char* id : { "Ability.Defense.ShieldHarvest.Basic", "Ability.Defense.TemporalConvergence.Basic" })
		{
			Fixture f{ report, app }; f.Equip(id); const auto enemy = f.Enemy({ 100.f, 0.f });
			auto& shield = f.ship->GetShieldComponent(); shield.ChangeShield(shield.GetMaxShield());
			f.Ability().SetActivationAim(AbilityAimOverride{ { 1.f, 0.f }, f.ship->GetActorLocation() }); report.Check(f.Ability().TryActivateWithInput(true), "activate moved shield ability source");
			if (std::string{ id }.find("ShieldHarvest") != std::string::npos) f.Ability().Tick(1.5f);
			else
			{
				const auto field = Latest<TemporalConvergenceFieldActor>(*f.level); report.Check(field != nullptr, "TemporalConvergence spawns real field");
				field->Tick(2.f); field->Tick(.01f); field->Tick(.01f);
			}
			const float excess = shield.GetOvershield(); report.Check(excess > 0.f, std::string{ "source grants real overshield: " } + id);
			shield.TickTemporaryOvershields(5.f); Equal(report, std::string{ "source uses shared hold: " } + id, excess, shield.GetOvershield());
			shield.TickTemporaryOvershields(.01f); Equal(report, std::string{ "source uses shared decay: " } + id, std::max(0.f, excess - 1.f), shield.GetOvershield());
		}
		Fixture f{ report, app }; auto& shield = f.ship->GetShieldComponent();
		// The real return aggregation owner is the terminal path used by every Aegis projectile/Relay child.
		AegisReaverFlightState flight{ *f.ship }; flight.RegisterProjectile(); flight.ReportProjectileReturned(50.f);
		const float excess = shield.GetOvershield(); Equal(report, "Aegis return now grants shared excess", 50.f, excess);
		shield.TickTemporaryOvershields(5.01f); Equal(report, "Aegis is no longer permanent", 49.f, shield.GetOvershield());
		{
			Fixture recall{ report, app }; recall.Equip("Ability.Defense.TemporalRecall.Basic"); auto& resource = recall.ship->GetShieldComponent();
			resource.GrantShield("RecallSnapshot", 400.f);
			auto& history = recall.ship->GetTemporalStateHistory(); history.Reset(); history.CaptureInitial(*recall.ship);
			resource.ChangeShield(-400.f); history.AdvanceAndCapture(*recall.ship, 3.f);
			report.Check(recall.Ability().TryActivate(), "activate real Temporal Recall with an overcap history snapshot"); recall.Ability().Tick(recall.Ability().GetDefinition().duration);
			const float restoredExcess = resource.GetOvershield(); report.Check(restoredExcess > 0.f, "Temporal Recall restoration now follows fill-then-overshield");
			resource.TickTemporaryOvershields(5.f); Equal(report, "Temporal Recall restoration uses shared hold", restoredExcess, resource.GetOvershield());
			resource.TickTemporaryOvershields(.01f); Equal(report, "Temporal Recall restoration uses shared decay", restoredExcess - 1.f, resource.GetOvershield());
		}
	}
	void ModuleSourceRules(Report& report, Application& app)
	{
		for (const char* id : { "Module.Defense.LootShield.Uncommon", "Module.Defense.BarrierRecharge.Basic", "Module.Defense.OvershieldLayer.Uncommon" })
		{
			uncommon_p6_e2e::Fixture f{ report, app }; f.Install(id);
			auto& shield = f.ship->GetShieldComponent(); shield.ChangeShield(shield.GetMaxShield());
			const auto trigger = [&]
			{
				if (std::string{ id }.find("LootShield") != std::string::npos) { f.Kill(); return; }
				auto& asc = f.ship->GetAbilitySystemComponent();
				if (auto* current = asc.GetAbility(Q)) asc.RemoveAbility(current->GetHandle());
				report.Check(asc.GrantAbility(*content::AbilityContentCatalog::FindById("Ability.Defense.DirectionalBarrier.Basic"), Q).IsValid(), "grant Defense trigger for shield module");
				auto* ability = asc.GetAbility(Q); report.Check(ability->TryActivate(), "activate real module Defense trigger"); ability->Cancel(sas::AbilityEndReason::Completed);
			};
			trigger(); const float excess = shield.GetOvershield(); report.Check(excess > 0.f, std::string{ "module uses shared overcap grant: " } + id);
			shield.TickTemporaryOvershields(5.f); Equal(report, "module grants share the full five-second hold", excess, shield.GetOvershield());
			shield.TickTemporaryOvershields(.01f); Equal(report, "module grants share 100/s decay", std::max(0.f, excess - 1.f), shield.GetOvershield());
			trigger(); Equal(report, "module regrant refreshes without per-source stacking", excess, shield.GetOvershield());
		}
		uncommon_p6_e2e::Fixture f{ report, app }; f.Install("Module.Defense.LootShield.Uncommon", 2, 1, "Module.SideFeature.LootShield.OverflowRecovery");
		auto& shield = f.ship->GetShieldComponent(); shield.ChangeShield(shield.GetMaxShield());
		auto& health = f.ship->GetHealthComponent(); health.ChangeHealth(50.f - health.GetHealth()); f.Kill();
		Equal(report, "overflow recovery converts its share of the excess to health", 50.f + shield.GetMaxShield() * .03f * .5f, health.GetHealth());
		Equal(report, "only the unconverted excess becomes overshield", shield.GetMaxShield() * .03f * .5f, shield.GetOvershield());
	}
}
int main(int argc, char** argv)
{
	EngineContext context; EngineContext::Scope scope{ context }; InstallVegaEngineAssets(); Report report;
	try
	{
		context.Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		report.Check(GameContentBootstrap::Register(), "bootstrap shipped content including all evolve combinations");
		std::string title = "HullShock evolve E2E"; Application app{ { 64u, 64u }, 32u, title, sf::Style::None }; app.GetRenderWindow().setVisible(false);
		SharedRule(report, app); SelectionAndBase(report, app); Directed(report, app); Capacitor(report, app);
		Aftershock(report, app, H::AftershockResonance, nullptr);
		Aftershock(report, app, H::DirectedDischarge, H::AftershockResonance);
		Aftershock(report, app, H::ReactiveCapacitor, H::AftershockResonance);
		CombinedCapacitorAndConversion(report, app);
		LifetimeAndCapture(report, app); SourceRules(report, app); ModuleSourceRules(report, app);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	PlayerManager::GetPlayerManager().Reset();
	report.artifact["test"] = "HullShockEvolveE2E";
	common_module_e2e::WriteArtifact(argc > 1 ? argv[1] : "outputs/hullshock-evolve-e2e.json", report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
