#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP4E2E.h"
#include "enemy/EnemyActor.h"
#include "gameplay/combat/CombatantTakeover.h"
#include "gameplay/content/EnemyFactory.h"
#include "gameplay/enemy/EnemyIds.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "gameplay/targeting/TargetRelation.h"

#include <algorithm>

namespace
{
	using namespace uncommon_p4_e2e;

	constexpr char kNeural[] = "Module.Offense.NeuralHijacker.Legendary";
	constexpr char kParasite[] = "Module.Defense.ParasiteHull.Legendary";
	constexpr char kLastStand[] = "Module.Defense.LastStand.Epic";
	constexpr float kStep = .05f;

	struct RewardCounter
	{
		int score = 0;
		void OnScore(unsigned int) { ++score; }
	};

	struct TakeoverFixture : Fixture
	{
		TakeoverFixture(Report& result, Application& application) : Fixture{ result, application }
		{
			const auto defaults = loadout.GetPlacements();
			for (const auto& placement : defaults) report.Check(loadout.Uninstall(placement.instanceId), "remove default module");
			report.Check(ship->GetModuleApplier().Apply(loadout, rules), "apply empty loadout");
		}
		int EffectiveLevel(std::uint32_t instance, int natural) const { return natural + ModuleResolver::FindZoneLevelBonus(ship->GetModuleApplier().GetResolvedEffects(), instance); }
		shared_ptr<EnemyActor> Spawn(const sf::Vector2f& offset, ControlTargetClass targetClass = ControlTargetClass::Normal)
		{
			EnemySpawnContext context;
			context.windowCullEnabled = false;
			const auto enemy = content::SpawnEnemy(*level, EnemyIds::ApproachGunnerBasic, ship->GetActorLocation() + offset, 1.f, context).lock();
			report.Check(enemy != nullptr, "spawn a real catalog enemy");
			level->TickInternal(0.f);
			report.Check(enemy->GetEnemyRuntime().IsReady(), "catalog enemy is combat ready");
			enemy->SetControlTargetClass(targetClass);
			enemy->GetShieldComponent().ChangeShield(-100000.f);
			return enemy;
		}
		void Run(float seconds) { for (float elapsed = 0.f; elapsed < seconds - 1e-4f; elapsed += kStep) level->TickInternal(kStep); }
		void Kill(Actor& target, Actor* source) { ApplyCombatDamage(target, 1000000.f, source, {}, Plain()); }
		const LegendaryNeuralHijackerRuntime& Neural() const { return ship->GetModuleApplier().GetLegendaryNeuralHijackerRuntime(); }
		const LegendaryParasiteHullRuntime& Parasite() const { return ship->GetModuleApplier().GetLegendaryParasiteHullRuntime(); }
	};

	bool Alive(const SpaceShip& ship) { return !ship.GetIsPendingDestroy() && ship.GetHealthComponent().GetHealth() > 0.f; }
	float Distance(const Actor& a, const Actor& b) { const auto d = a.GetActorLocation() - b.GetActorLocation(); return std::sqrt(d.x * d.x + d.y * d.y); }

	void NeuralChecks(Report& report, Application& application, int natural)
	{
		TakeoverFixture f{ report, application };
		const auto instance = f.Install(kNeural, 0, natural);
		const int effective = f.EffectiveLevel(instance, natural);
		const float n = static_cast<float>(effective - 1);
		const std::string input = "Neural effective L" + std::to_string(effective);
		const auto& rule = f.Neural().GetRules().front();
		report.CheckValue("ally seconds 3 + 0.4n", 3.f + .4f * n, rule.allySeconds, input);
		report.CheckValue("ally health 25% of MaxHealth", .25f, rule.allyHealthFraction, input);
		report.CheckValue("ally damage 50%", .5f, rule.allyDamageMultiplier, input);

		// Lethal player hit: the enemy switches sides instead of dying, and the kill and its rewards happen once now.
		const auto enemy = f.Spawn({ 300.f, 0.f });
		const auto other = f.Spawn({ 300.f, 400.f });
		RewardCounter rewards;
		enemy->onScoreAwarded.BindAction(&rewards, &RewardCounter::OnScore);
		const float damageBefore = enemy->GetCombatRuntime().GetOutgoingDamageMultiplier();
		const float maxHealth = enemy->GetHealthComponent().GetMaxHealth();
		f.Kill(*enemy, f.ship.get());
		report.Check(Alive(*enemy) && IsCombatantTakenOver(*enemy), "a lethal player hit hijacks a normal enemy", input);
		report.Check(rewards.score == 1 && enemy->HasGrantedKillRewards(), "rewards are granted once at the lethal hit", input);
		report.CheckValue("ally returns at 25% of its MaxHealth", maxHealth * .25f, enemy->GetHealthComponent().GetHealth(), input);
		report.CheckValue("ally deals 50% of its natural damage", damageBefore * .5f, enemy->GetCombatRuntime().GetOutgoingDamageMultiplier(), input);
		report.Check(enemy->GetCollisionLayer() == GameCollision::FriendlySummon, "ally is on the player's side", input);
		report.Check(!targeting::IsOpposingTarget(*enemy, *f.ship) && targeting::IsOpposingTarget(*enemy, *other), "ally targets enemies, never the player", input);
		const auto opposing = targeting::FindOpposingCombatants(*f.level, *enemy, 5000.f);
		report.Check(std::find(opposing.begin(), opposing.end(), other) != opposing.end() && std::find(opposing.begin(), opposing.end(), f.ship) == opposing.end(), "ally target query sees enemies only", input);
		const auto fromPlayer = targeting::FindOpposingCombatants(*f.level, *f.ship, 5000.f);
		report.Check(std::find(fromPlayer.begin(), fromPlayer.end(), enemy) == fromPlayer.end(), "the player side never targets the ally", input);

		// A kill made by the ally is a real kill but never a new hijack.
		RewardCounter otherRewards;
		other->onScoreAwarded.BindAction(&otherRewards, &RewardCounter::OnScore);
		f.Kill(*other, enemy.get());
		report.Check(!Alive(*other) && !IsCombatantTakenOver(*other) && otherRewards.score == 1, "an ally's kill rewards normally and does not chain", input);

		// Unlimited allies.
		const auto second = f.Spawn({ -300.f, 0.f });
		f.Kill(*second, f.ship.get());
		report.Check(IsCombatantTakenOver(*second) && f.Neural().GetAllyCount() == 2, "several allies at once", input);

		// Bosses and minibosses are never hijacked.
		const auto boss = f.Spawn({ 0.f, -600.f }, ControlTargetClass::Boss);
		const auto miniBoss = f.Spawn({ 0.f, 600.f }, ControlTargetClass::MiniBoss);
		f.Kill(*boss, f.ship.get());
		f.Kill(*miniBoss, f.ship.get());
		report.Check(!Alive(*boss) && !Alive(*miniBoss), "bosses and minibosses die normally", input);

		// Duration per level, then the ally dies without a second reward.
		f.Run(rule.allySeconds - .2f);
		report.Check(Alive(*enemy) && IsCombatantTakenOver(*enemy), "ally lasts its level duration", input);
		f.Run(.4f);
		report.Check(!Alive(*enemy) && rewards.score == 1, "ally dies at the end without a second reward", input);

		// Uninstall closes the remaining allies.
		const auto last = f.Spawn({ 0.f, 300.f });
		f.Kill(*last, f.ship.get());
		report.Check(IsCombatantTakenOver(*last), "hijack before uninstall", input);
		f.Remove(instance);
		report.Check(!Alive(*last) && f.Neural().GetRules().empty(), "uninstall ends the allies", input);
		const auto after = f.Spawn({ 0.f, -300.f });
		f.Kill(*after, f.ship.get());
		report.Check(!Alive(*after), "no hijack after uninstall", input);
	}

	void ParasiteChecks(Report& report, Application& application, int natural)
	{
		TakeoverFixture f{ report, application };
		const auto instance = f.Install(kParasite, 0, natural);
		const int effective = f.EffectiveLevel(instance, natural);
		const float n = static_cast<float>(effective - 1);
		const std::string input = "Parasite effective L" + std::to_string(effective);
		const auto& rule = f.Parasite().GetRules().front();
		report.CheckValue("host health bonus 20% + 10n", .2f + .1f * n, rule.borrowedHealthBonus, input);
		report.CheckValue("host damage bonus 20% + 10n", .2f + .1f * n, rule.borrowedDamageBonus, input);
		report.Check(rule.maximumSeconds == 10.f && rule.cooldownSeconds == 90.f && rule.possessionRange == 600.f, "10 s cap, 90 s cooldown, 600 range", input);
		auto& health = f.ship->GetHealthComponent();
		const float playerMax = health.GetMaxHealth();

		// The nearest eligible enemy within 600 becomes the host, with boosted health and damage.
		const auto host = f.Spawn({ 400.f, 0.f });
		const auto farther = f.Spawn({ 0.f, 550.f });
		const auto boss = f.Spawn({ 200.f, 0.f }, ControlTargetClass::Boss);
		const float hostHealth = host->GetHealthComponent().GetHealth(), hostMax = host->GetHealthComponent().GetMaxHealth();
		const float hostDamage = host->GetCombatRuntime().GetOutgoingDamageMultiplier();
		RewardCounter hostRewards;
		host->onScoreAwarded.BindAction(&hostRewards, &RewardCounter::OnScore);
		f.Kill(*f.ship, boss.get());
		report.Check(Alive(*f.ship) && f.Parasite().IsPossessing() && f.Parasite().GetHost() == host, "lethal damage takes over the nearest eligible enemy (bosses skipped)", input);
		report.CheckValue("host MaxHealth +(20 + 10n)%", hostMax * (1.f + rule.borrowedHealthBonus), host->GetHealthComponent().GetMaxHealth(), input);
		report.CheckValue("host health +(20 + 10n)%", hostHealth * (1.f + rule.borrowedHealthBonus), host->GetHealthComponent().GetHealth(), input);
		report.CheckValue("host damage +(20 + 10n)%", hostDamage * (1.f + rule.borrowedDamageBonus), host->GetCombatRuntime().GetOutgoingDamageMultiplier(), input);
		report.Check(hostRewards.score == 1, "the host's reward is granted once at the takeover", input);
		report.Check(f.ship->IsBodySuspended() && f.ship->GetCollisionLayer() == vega::CollisionLayer::None && f.ship->GetCombatRuntime().BlocksIncomingDamage(), "own ship is hidden, untargetable and protected", input);
		report.Check(host->IsExternallyControlled() && host->GetCollisionLayer() == GameCollision::FriendlySummon, "the player controls the host on the player's side", input);
		report.CheckValue("cooldown starts at 90 s", 90.f, f.Parasite().GetCooldownRemaining(), input);
		farther->Destroy(); boss->Destroy();
		f.Run(3.f);
		report.Check(Distance(*f.ship, *host) < 1.f, "the own ship (and the camera following it) rides with the host", input);
		report.Check(!f.ship->IsAfterburning() && f.ship->IsPossessing(), "Afterburner stays off while the host is steered", input);

		// The host dies: the ship returns at its position with the repair accumulated so far (3 s -> 9%).
		const sf::Vector2f hostLocation = host->GetActorLocation();
		f.Kill(*host, nullptr);
		f.level->TickInternal(0.f);
		report.Check(!f.Parasite().IsPossessing() && !f.ship->IsBodySuspended() && f.ship->GetCollisionLayer() == GameCollision::Player, "host death returns the ship", input);
		report.Check(std::abs(f.ship->GetActorLocation().x - hostLocation.x) < 1.f && std::abs(f.ship->GetActorLocation().y - hostLocation.y) < 1.f, "the ship reappears at the host's position", input);
		report.CheckValue("3 s in the host repairs 9% of MaxHealth", playerMax * .09f, health.GetHealth(), input);
		report.Check(hostRewards.score == 1, "the host's death grants no second reward", input);

		// On cooldown Parasite does nothing; with no Last Stand the ship dies.
		{
			const auto enemy = f.Spawn({ 300.f, 0.f });
			f.Kill(*f.ship, enemy.get());
			report.Check(!Alive(*f.ship) || health.GetHealth() <= 0.f, "on cooldown the lethal hit kills", input);
		}
	}

	void ParasiteTimingChecks(Report& report, Application& application)
	{
		{
			// Minimum 3% when the host dies at once, and the 10 s cap with 30%.
			TakeoverFixture f{ report, application };
			f.Install(kParasite);
			auto& health = f.ship->GetHealthComponent();
			const float playerMax = health.GetMaxHealth();
			auto host = f.Spawn({ 300.f, 0.f });
			f.Kill(*f.ship, host.get());
			report.Check(f.Parasite().IsPossessing(), "possession starts");
			f.Kill(*host, nullptr);
			f.level->TickInternal(0.f);
			report.CheckValue("an instant host death still returns 3% of MaxHealth", playerMax * .03f, health.GetHealth(), "0 s in the host");
		}
		{
			TakeoverFixture f{ report, application };
			f.Install(kParasite);
			auto& health = f.ship->GetHealthComponent();
			const float playerMax = health.GetMaxHealth();
			const auto host = f.Spawn({ 300.f, 0.f });
			f.Kill(*f.ship, host.get());
			f.Run(9.8f);
			report.Check(f.Parasite().IsPossessing() && Alive(*host), "possession lasts until the cap");
			f.Run(.4f);
			report.Check(!f.Parasite().IsPossessing() && !Alive(*host), "the host dies at 10 s");
			report.CheckValue("10 s in the host repairs 30% of MaxHealth", playerMax * .3f, health.GetHealth(), "10 s cap");
			report.Check(std::abs(90.f - 10.2f - f.Parasite().GetCooldownRemaining()) < .06f, "the cooldown keeps running during the possession", f.Parasite().GetCooldownRemaining());
			f.Run(80.2f);
			report.CheckValue("the cooldown ends after 90 s", 0.f, f.Parasite().GetCooldownRemaining(), "after 90 s");
			const auto next = f.Spawn({ 300.f, 0.f });
			f.Kill(*f.ship, next.get());
			report.Check(f.Parasite().IsPossessing(), "ready again after 90 s");
		}
		{
			// No eligible enemy within 600: normal death.
			TakeoverFixture f{ report, application };
			f.Install(kParasite);
			const auto enemy = f.Spawn({ 800.f, 0.f });
			f.Kill(*f.ship, enemy.get());
			report.Check(!f.Parasite().IsPossessing() && f.ship->GetHealthComponent().GetHealth() <= 0.f, "no host in range -> normal death");
		}
		{
			// Parasite runs before Last Stand; while it cools down Last Stand still saves the ship.
			TakeoverFixture f{ report, application };
			f.Install(kParasite);
			f.Install(kLastStand);
			int revives = 0;
			f.ship->GetModuleApplier().SetReviveSpentSink([&revives](std::uint32_t) { ++revives; });
			const auto host = f.Spawn({ 300.f, 0.f });
			f.Kill(*f.ship, host.get());
			report.Check(f.Parasite().IsPossessing() && revives == 0, "Parasite goes before Last Stand");
			f.Kill(*host, nullptr);
			f.level->TickInternal(0.f);
			const auto enemy = f.Spawn({ 300.f, 0.f });
			f.Kill(*f.ship, enemy.get());
			report.Check(!f.Parasite().IsPossessing() && revives == 1 && f.ship->GetHealthComponent().GetHealth() > 0.f, "on cooldown Last Stand applies normally");
		}
		{
			// Uninstalling during a possession returns the ship.
			TakeoverFixture f{ report, application };
			const auto instance = f.Install(kParasite);
			const auto host = f.Spawn({ 300.f, 0.f });
			f.Kill(*f.ship, host.get());
			f.Remove(instance);
			report.Check(!f.Parasite().IsPossessing() && !f.ship->IsBodySuspended() && !Alive(*host), "uninstall ends the possession and the host");
		}
	}
}

int main(int argc, char** argv)
{
	vega::EngineContext context; vega::EngineContext::Scope scope{ context }; vega::InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		report.Check(GameContentBootstrap::Register(), "register shipped game content");
		std::string title = "Legendary takeover modules E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		NeuralChecks(report, application, 1);
		NeuralChecks(report, application, 3);
		ParasiteChecks(report, application, 1);
		ParasiteChecks(report, application, 3);
		ParasiteTimingChecks(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "LightYearsLegendaryTakeoverModulesE2E"; WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
