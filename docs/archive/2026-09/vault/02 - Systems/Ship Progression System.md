---
type: system
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
uncommitted_areas:
  - player, ability, weapon and combat integrations
  - GasLiteCoreTests
source_files:
  - LightYearsGame/include/gameConfigs/ship/ShipStructs.h
  - LightYearsGame/include/gameplay/progression/ShipProgression.h
  - LightYearsGame/src/gameplay/progression/ShipProgression.cpp
  - LightYearsGame/src/player/Player.cpp
  - LightYearsGame/src/gameplay/ship/ShipRuntime.cpp
symbols:
  - ly::ShipProgression
  - ly::ShipProgression::AddXP
  - ly::ShipProgression::BindAttributes
  - ly::ShipRuntime::RecalculateAttributes
  - ly::Player::AwardShipXP
related:
  - "[[Ship Progression and Respawn Flow]]"
  - "[[ShipProgression]]"
  - "[[CombatRuntime]]"
  - "[[Shield System]]"
---

# Ship Progression System

## 7 Eylül 2026 kaynak kontrolü

EnemyActor::Blew → onShipXPAwarded → GameLevel::OnActorSpawned bağlaması → Player::AwardShipXP → ShipProgression::AddXP yolu kaynakta mevcut. Player::SpawnSpaceShip yeni attributes'a rebind eder. Bu bellek içi run/respawn davranışıdır; [[Save and Load]] desteği değildir.

Bu ek kaynak incelemesidir; build/test çalıştırılmadı. Aşağıdaki eski örnekler tarihsel inceleme kapsamını taşır; bu güncelleme ile çelişen ayrıntılar güncel davranış kabul edilmemeli.


## Kapsam

Bu not, player-owned ship XP/level state'i, level attribute modifier'ları, respawn'da rebind ve owner attribute'larından türetilen ship runtime değerlerini açıklar. Ability level/evolve ve weapon-specific progression ayrı sonraki aşamalardır.

Production player-ship ve progression kapsamı şu anda yalnız Fighter'dır.
Breacher, Interceptor, Conductor, Aegis ve Cryo Controller tasarım kimlikleri
henüz production `ShipDefinition`/JSON/runtime seçim profili değildir. Phase 3A
Fighter üzerinden sürdürülecek; diğer beş geminin base ve growth balance'ı daha
sonra ilgili content/selection altyapısıyla birlikte ele alınacaktır.

## Sorumluluk sınırları

| Katman | Doğrulanmış sorumluluk |
|---|---|
| `ShipDefinition::progressionDefinition` | Base XP, exponent ve kontrollü `{attributeId, perLevel}` doğal büyüme listesi |
| `Player::mShipProgression` | Run boyunca yaşayan XP/level state'i; active ship'in parçası değildir |
| `ShipProgression` | XP, level, modifier handle'ları ve currently bound owner attributes |
| `CombatRuntime::AttributeSystem` | Level bonuslarının uygulandığı owner attribute storage |
| `ShipRuntime` | Owner max health/energy değişimlerinden shield, afterburner ve regen-derived değerlerini hesaplar |
| `SpaceShip` | Attribute değişikliklerini health/shield/energy/movement component'lerine yansıtır |
| `GameLevel` / enemy | Enemy XP event'ini player'a bağlayan producer yolu |

## Config ve seviye hesabı

`ShipProgressionDefinition` üç alan taşır: `baseXP`, `xpExponent` ve isteğe bağlı `naturalGrowth`. Her büyüme girdisi bir owner `attributeId` ile doğrudan flat `perLevel` değerini eşler. Bir sonraki level için gereksinim:

`max(1, baseXP × currentLevel^xpExponent)`

`AddXP`, non-positive XP'yi reddeder; tek çağrıda birden çok level geçebilir. XP her geçişte o anki level'ın gereksinimi kadar azaltılır. Level başlangıcı `1`dir; level 1 tamamlanmış level sayısını `0` kabul eder ve growth modifier almaz.

## Level growth

Kontrollü whitelist şu owner attribute'larını kabul eder: MaxHealth, EnergyPower, AttackPower, AttackSpeed, AbilityHaste, horizontal/vertical movement speed, Armor, Luck ve CriticalChance. Her attribute için toplam bonus:

`(currentLevel - 1) × perLevel`

Listede olmayan attribute sıfır büyür. Duplicate attribute, negatif/non-finite değer ve whitelist dışındaki derived attribute content validation'da reddedilir. Movement yeni bir genel attribute değildir; `MoveSpeedHorizontal` ve `MoveSpeedVertical` ayrı girdilerdir. Eski merkezi base-growth ve multiplier modeli kaldırılmıştır.

`ShipProgression`, önce eski modifier handle'larını kaldırır, sonra her total bonus için `AttributeSystem::AddModifier(Add)` çağırır. Bu yeniden kurma yaklaşımı level-up/rebind sırasında modifier birikimini önler.

Fighter'ın eski migration compatibility growth listesi kaldırılmış ve Phase 3A.3 ile production Fighter base profili ve natural growth değerleri tanımlanmıştır:
- MaxHealth: +75 / level
- AttackPower: +3 / level
- EnergyPower: +2 / level
- Armor: +2 / level
- Luck: +1 / level

AttackSpeed, CriticalChance, AbilityHaste, MoveSpeedHorizontal ve MoveSpeedVertical sistem whitelist'inde yer almakla birlikte Fighter profilinde growth almaz.
Fighter L1 AttackPower 35 değeri progression modifier'ı değil, ship profile'daki `baseOwnerAttributes` alanı üzerinden initialize edilen base ship statıdır (L1'de progression bonusu 0'dır). Respawn veya rebind durumunda AttackPower ve progression modifier'ları kümülatif olarak birikmez.

Bu değerler mevcut Fighter production değeridir; gelecekte playtest sonucu tune edilebilir ve diğer gemiler için küresel bir standart olarak sunulmamıştır. Altı gemilik nihai bir karşılaştırma yapılmamıştır; diğer beş tasarım kimliği (Breacher, Interceptor, Conductor, Aegis, Cryo Controller) ertelenmiştir.

### Fighter Seviye Tablosu

Formül: `FinalStat = BaseStat + perLevel × (Level - 1)`

| Stat / Türetme | L1 | L5 | L10 | L15 |
|---|---:|---:|---:|---:|
| MaxHealth | 250 | 550 | 925 | 1300 |
| AttackPower | 35 | 47 | 62 | 77 |
| EnergyPower | 35 | 43 | 53 | 63 |
| Armor | 0 | 8 | 18 | 28 |
| Luck | 0 | 4 | 9 | 14 |
| AttackSpeed / Crit / Haste / MoveSpeed | 0 | 0 | 0 | 0 |
| MaxShield (`115 + EnergyPower`) | 150 | 158 | 168 | 178 |
| AfterburnerCapacity (`65 + EnergyPower`) | 100 | 108 | 118 | 128 |
| ShieldRegenPerSecond (`MaxShield / 6`) | 25.0 | ~26.3333 | 28.0 | ~29.6667 |
| AfterburnerRegenPerSecond (`AfterburnerCapacity / 6`) | ~16.6667 | 18.0 | ~19.6667 | ~21.3333 |
| Armor Damage Reduction (`Armor / (Armor + 100)`) | 0% | 8/108 (~7.407%) | 18/118 (~15.254%) | 28/128 (21.875%) |

AttackSpeed, CriticalChance ve AbilityHaste yüzde fraction'ı değil, `PercentageRatingScale = 100` ile çözülen raw rating'dir. `2.0` iki puandır; `0.02` yüzde iki değildir. Primary weapon AttackSpeed'i şu an FireRate'a additive scaling ile tüketir; bu tüketici formülü Phase 3A.1'in dışında tutulmuştur.

## XP producer ve player state

Enemy `shipXPReward` değerini taşır. `GameLevel`, enemy'nin `onShipXPAwarded` delegate'ini `Player::AwardShipXP`e bağlar; Player bunu `mShipProgression.AddXP`e geçirir. Score ve ship XP ayrı alanlardır; `ShipDefinition` varsayılan olarak explicit XP ödülü verilmezse score miktarını kullanır.

`Player` yeni ship spawn'ında progression henüz configure edilmemişse `ShipData::Ship_Player_Fighter.progressionDefinition` ile configure eder. Bu Fighter-only bağlantı mevcut kapsamın bilinçli sınırıdır; farklı player ship seçimi sonraki aşamaya ertelenmiştir.

## Respawn ve derived runtime bağlantısı

Progression, old ship destruction veya yeni spawn öncesinde `UnbindAttributes` çağırır; eski runtime pointer'ını dereference etmez. Yeni `PlayerSpaceShip` oluşunca Player, progression'ı yeni ship'in `CombatRuntime` owner attributes'ına bağlar ve toplam level bonuslarını yeniden kurar.

Level bonusu MaxHealth veya EnergyPower'i değiştirdiğinde `ShipRuntime` owner attribute callback'i ile recalculation yapar. Bu, derived max shield, shield regen, afterburner capacity/regen ve owner-facing afterburner regen değerlerini yeniler. `SpaceShip` attribute callbacks ile component/movement değerlerini refresh eder.

## Run reset ve sınırlar

`ResetForNewRun`, XP'yi `0`, level'i `1` yapar ve attribute binding'i kaldırır. `Player::ResetRunProgression` ayrıca purchased ability level kaydını ve scrap'i temizler.

- Ship progression, ability level satın alma state'ini sahiplenmez; Player'daki `mPurchasedAbilityLevels` ayrı mekanizmadır.
- Weapon progression profile ve ability evolve alanları bu sistemin config'ine dahil değildir.
- XP persistence/save-game veya UI level display entegrasyonu bu aşamada doğrulanmadı.

## Kaynak doğrulaması

- İncelenen durum: dirty worktree.
- Directly read: ShipProgression, ShipRuntime, Player spawn/reset ve GameLevel XP binding noktaları.
- Test/build bu aşamada çalıştırılmadı.
