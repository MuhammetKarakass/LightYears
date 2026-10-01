# Gemi Kadrosu Uygulama Planı (Faz 1 — Gemiler)

**Tarih:** 2026-10-01 · **Branch:** `codex/luna-callback-lifetime-checkpoint` · **Kaynak tasarım:** [Light_Years_Ships_Primary_Weapons_Combined.md](Light_Years_Ships_Primary_Weapons_Combined.md)

Bu plan, tasarım dokümanındaki altı gemiyi oyuna getirir. **Silah davranışları bu fazın kapsamı dışındadır** (Faz 2). Her gemi şimdilik `weapons.json`'daki mevcut silahını kullanır; silahlar sonra tasarıma göre yeniden yazılacak.

Uygulayıcılar için: Sol orkestrasyonu yürütür, her adımı bir Luna uygular, derler, test eder ve commitler. Adımlar sırayla yapılır; bir adımın build ve testi geçmeden sonrakine geçilmez. `orchestrator.py` kullanılmaz.

---

## 0. Mevcut durum (kaynakta doğrulandı)

| Konu | Durum | Kaynak |
|---|---|---|
| Oyuncu gemileri | `ships.json`'da yalnız `Ship.Player.Fighter.Basic` var; diğer 3 kayıt düşman gövdesi | `LightYearsGame/assets/content/data/ships.json` |
| Oyuncu gemisi seçimi | Yok. Fighter sabit kodlu | `ShipContentCatalog::GetPlayerFighterDefinition()` (`src/gameplay/content/ShipContentCatalog.cpp:194`) |
| Gemi doğurma | `Player::SpawnSpaceShip` → `world->SpawnActor<PlayerSpaceShip>()` (varsayılan argüman Fighter) | `src/player/Player.cpp:47`, `include/player/PlayerSpaceShip.h:16-19` |
| Seviye büyümesi | `Player` ilk doğuşta Fighter'ın `progressionDefinition`'ını kullanır | `src/player/Player.cpp:57-62` |
| Oyuncu oluşturma | `PlayerRespawnSystem` → `PlayerManager::CreateNewPlayer()`; restart'ta `PlayerManager::Reset()` oyuncuları siler | `src/player/PlayerRespawnSystem.cpp:62-70`, `src/level/GameLevel.cpp:174-177` |
| Açılış | `GameApplication` doğrudan `ArenaTestLevel` yükler; ana menü açılışta gösterilmez | `src/gameFramework/GameApplication.cpp:30` |
| JSON şablonu | Her gemi C++ `ShipData::Ship_Player_Fighter` şablonundan kopyalanıp JSON alanlarıyla ezilir. **`engineMounts` JSON'dan okunmaz**, tüm gemiler Fighter'ın motor ışıklarını alır | `src/gameplay/content/ShipLoader.cpp:116-167` |
| İzinli temel nitelikler | `baseOwnerAttributes`: AttackPower, Armor, Luck, AttackSpeed, CriticalChance, CriticalDamage, AbilityHaste. **MaxHealth** `health` alanından, **EnergyPower** `energy.baseEnergyPower`'dan gelir | `include/gameplay/progression/ShipProgression.h:56-64`, `src/gameplay/ship/ShipRuntime.cpp:16-27` |
| İzinli büyüme nitelikleri | MaxHealth, AttackPower, EnergyPower, Armor, Luck, AttackSpeed, CriticalChance, AbilityHaste, MoveSpeedHorizontal/Vertical | `ShipProgression.h:45-54` |
| AttackSpeed birimi | Rating; `1.0` = **%1**. Silah çarpanı `1 + AS/100` (yalnız `OwnerAttackSpeedPercentage` modundaki silahlarda) | `PrimaryWeaponExecutionSystem.cpp:803-813` |
| Fighter'ın bugünkü verisi | AP 35, MaxHealth +75/seviye, Armor tabanı yok | `ships.json`, `include/gameConfigs/ship/ShipConfig.h:12-69` |

---

## 1. Hedef veriler

Ortak değerler (tüm oyuncu gemileri, Fighter'dan kopya):
- `speed: [350, 350]`, `collisionDamage: 25`, `score: 0`, `shipXPReward: 0`
- `progression.baseXP: 6`, `xpExponent: 0`, `xpPerLevel: 1` (öldürme eşiği 6, 7, 8… kararı korunur)
- Hareket: `linearDamping 0.70`, `inputResponsiveness 12`, `mouseAimDeadZone 32`, `horizontalRatingToThrust 60`, `verticalRatingToThrust 60`, `ratingToMaxSpeed 20`, `movementRatingDiminishingScale 20`
- Enerji bloğu: gemiye göre, bkz. Bölüm 1.3b (`baseEnergyPower` Bölüm 1.1'den).

### 1.1 Stat tablosu

| Gemi (`id`) | `health` | AP | EP (`baseEnergyPower`) | Armor | Luck | AttackSpeed |
|---|---:|---:|---:|---:|---:|---:|
| `Ship.Player.Fighter.Basic` | 250 | 39 | 35 | 2 | 0 | — |
| `Ship.Player.Breacher.Basic` | 290 | 45 | 34 | 3 | 0 | — |
| `Ship.Player.Interceptor.Basic` | 200 | 41 | 28 | 1 | 1 | 1 (=%1) |
| `Ship.Player.Conductor.Basic` | 220 | 27 | 41 | 1 | 2 | — |
| `Ship.Player.Aegis.Basic` | 270 | 25 | 43 | 4 | 0 | — |
| `Ship.Player.CryoController.Basic` | 250 | 27 | 40 | 2 | 1 | — |

`baseOwnerAttributes` içine yalnız **sıfırdan farklı** AP / Armor / Luck / AttackSpeed yazılır.

### 1.2 Seviye başı büyüme (`progression.naturalGrowth`)

| Gemi | MaxHealth | AttackPower | EnergyPower | Armor | Luck | AttackSpeed |
|---|---:|---:|---:|---:|---:|---:|
| Fighter | 40 | 3 | 2 | 2 | 1 | — |
| Breacher | 60 | 2 | — | 2 | 1 | — |
| Interceptor | 30 | 3 | 1 | 1 | 1 | 1 |
| Conductor | 40 | 1 | 3 | 1 | 2 | — |
| Aegis | 50 | 1 | 3 | 3 | 1 | — |
| Cryo Controller | 40 | 1 | 2 | 2 | 1 | — |

"—" = kayıt yazılmaz (0 büyüme). Breacher'ın EP büyümesi tasarımda +0.

### 1.3 Hareket

| Gemi | maxSpeed | forwardThrust | reverseThrust | strafeThrust | angularTurnSpeed | angularTurnResponsiveness |
|---|---:|---:|---:|---:|---:|---:|
| Fighter | 520 | 650 | 190 | 270 | 400 | 5.0 |
| Breacher | 490 | 750 | 220 | 245 | 450 | 6.0 |
| Interceptor | 590 | 720 | 240 | 340 | 480 | 6.5 |
| Conductor | 505 | 620 | 200 | 285 | 410 | 5.3 |
| Aegis | 480 | 600 | 200 | 240 | 380 | 4.8 |
| Cryo Controller | 520 | 610 | 210 | 310 | 425 | 5.5 |

Aegis için tasarımda hareket tablosu yoktu; değerler kullanıcı onayıyla (K2, 2026-10-01) en yavaş ve en hantal gövde olarak belirlendi.

### 1.3b Enerji / kalkan / afterburner

**Kullanıcı kararı (K3, 2026-10-01):** Kalkan ve afterburner artık **EP'den türemez**. İkisi de HP gibi geminin kendi sistemidir: L1 taban değer + seviye başı sabit artış. EP yalnız yetenek/enerji silahı gücüdür; savunmaya katkısı aktif kalkan yetenekleri (ShieldHarvest, ShieldGraft vb.) üzerinden olur.

Gerekçe: EP zaten 55 yeteneğin 14'ünü ölçekliyor (29 kural; AP 11 yetenek / 20 kural). Pasif kalkan + afterburner da EP'den gelirse EP her gemi için tek doğru stat olur. Ayrıca bugünkü formülde EP her değiştiğinde (buff/debuff) kalkan tavanı oynuyor ve kalkan seviyeyle neredeyse büyümüyordu (Fighter L1→L20: 150→188).

Yeni formül:

```
MaxKalkan(L)       = baseMaxShield + maxShieldPerLevel × (L − 1)
ABkapasite(L)      = baseAfterburnerCapacity + afterburnerCapacityPerLevel × (L − 1)
Kalkan regen/sn    = MaxKalkan / shieldFullRechargeDuration          (değişmez)
AB regen/sn        = ABkapasite / afterburnerFullRechargeDuration    (değişmez)
```

`L` = gemi seviyesi, sınırsız. Kalkan seviye başı aralığı kullanıcı kararıyla **en az 10, en fazla 30**.

| Gemi | `baseMaxShield` | `maxShieldPerLevel` | `shieldFullRechargeDuration` | `baseShieldRechargeDelay` | `baseAfterburnerCapacity` | `afterburnerCapacityPerLevel` | `afterburnerFullRechargeDuration` | `baseAfterburnerRechargeDelay` | `baseAfterburnerSpeedMultiplier` | `baseAfterburnerAccelerationMultiplier` | `baseAfterburnerManeuverabilityMultiplier` | `baseAfterburnerEnergyDrainPerSecond` |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Fighter | 140 | 20 | 6.0 | 6.0 | 100 | 3 | 6.0 | 1.5 | 1.55 | 1.80 | 0.90 | 33 |
| Breacher | 110 | 10 | 6.0 | 6.0 | 110 | 2 | 5.0 | 1.2 | 1.65 | 2.00 | 0.90 | 33 |
| Interceptor | 110 | 15 | 6.0 | 6.0 | 120 | 4 | 5.0 | 1.0 | 1.60 | 2.00 | 1.00 | 30 |
| Conductor | 130 | 20 | 5.0 | 5.0 | 95 | 3 | 6.0 | 1.5 | 1.50 | 1.80 | 0.90 | 33 |
| Aegis | 160 | 30 | 7.0 | 4.5 | 75 | 2 | 7.0 | 2.0 | 1.45 | 1.60 | 0.85 | 36 |
| Cryo Controller | 130 | 20 | 6.0 | 5.5 | 95 | 3 | 6.0 | 1.5 | 1.50 | 1.80 | 0.90 | 33 |

Tüm gemilerde `baseAfterburnerRampUpDuration 0.22`, `baseAfterburnerRampDownDuration 0.45` (Fighter ile aynı). `shieldAffinity` / `afterburnerAffinity` alanları **kalkar** (bkz. Adım 2b).

Sonuç (hesaplanmış; "dayanıklılık" = HP × (1 + Armor/100) + kalkan; Armor yalnız gövdeye uygulanır, `CombatRuntime::ApplyHullDamageMitigation`):

| Gemi | Kalkan L1 → L10 → L20 | L20 kalkan/HP | Kalkan regen/sn L20 | Dayanıklılık (Fighter = 100) L1 / L10 / L20 | AB kapasitesi L1 → L10 → L20 | Boost süresi L1 → L20 |
|---|---|---:|---:|---|---|---|
| Fighter | 140 → 320 → 520 | %51 | 87 | 100 / 100 / 100 | 100 → 127 → 157 | 3,0 → 4,8 sn |
| Breacher | 110 → 200 → 300 | %21 | 50 | 103 / 114 / 120 | 110 → 128 → 148 | 3,3 → 4,5 sn |
| Interceptor | 110 → 245 → 395 | %51 | 66 | 79 / 72 / 68 | 120 → 156 → 196 | 4,0 → 6,5 sn |
| Conductor | 130 → 310 → 510 | %52 | 102 | 89 / 90 / 87 | 95 → 122 → 152 | 2,9 → 4,6 sn |
| Aegis | 160 → 430 → 730 | %60 | 104 | 112 / 131 / 139 | 75 → 93 → 113 | 2,1 → 3,1 sn |
| Cryo Controller | 130 → 310 → 510 | %50 | 85 | 97 / 99 / 99 | 95 → 122 → 152 | 2,9 → 4,6 sn |

### 1.4 Silah ve görsel

| Gemi | `primaryWeaponId` (mevcut kayıt) | `texturePath` (yer tutucu) |
|---|---|---|
| Fighter | `Weapon.Projectile.FighterRapidLaser.Basic` | `SpaceShooterRedux/PNG/playerShip1_blue.png` (şablondan) |
| Breacher | `Weapon.Projectile.RapidShotgun.Basic` | `SpaceShooterRedux/PNG/playerShip2_orange.png` |
| Interceptor | `Weapon.Projectile.DualKineticBlaster.Basic` | `SpaceShooterRedux/PNG/playerShip3_red.png` |
| Conductor | `Weapon.Arc.ElectricLauncher.Basic` | `SpaceShooterRedux/PNG/playerShip3_blue.png` |
| Aegis | `Weapon.Beam.ContinuousHeatLaser.Basic` | `SpaceShooterRedux/PNG/playerShip2_green.png` |
| Cryo Controller | `Weapon.Wave.CryoProjector.Basic` | `SpaceShooterRedux/PNG/playerShip2_blue.png` |

Sprite'ların hepsi `assets/SpaceShooterRedux/PNG/` altında mevcut.

---

## 2. Adımlar

### Adım 1 — Fighter verisini tasarıma getir

**Dosyalar:** `LightYearsGame/assets/content/data/ships.json`, `LightYearsGame/include/gameConfigs/ship/ShipConfig.h`

1. `ships.json` → Fighter:
   - `baseOwnerAttributes`: `Owner.AttackPower` **35 → 39**, yeni `Owner.Armor` **2** (Luck 0, yazılmaz).
   - `naturalGrowth`: `Owner.MaxHealth` **75 → 40**. Diğer büyümeler (AP 3, EP 2, Armor 2, Luck 1) aynı.
   - Hareket, enerji, silah, XP alanları değişmez.
2. `ShipConfig.h` → `Ship_Player_Fighter` C++ yedeğini JSON ile aynı yap: AP 39, Armor 2 tabanı, MaxHealth büyümesi 40. Ayrıca `linearDamping` yedekte `0.36f`, JSON'da `0.70` — yedeği **0.70** yap. (Yedek, JSON yüklenemezse ve testlerde kullanılır; JSON ile ayrışmamalı.)
3. Uyarı: 2026-10-01'de yanlışlıkla eklenen **Armor 400 / Luck 177** değerleri geri alınmıştı. Doğru değer **Armor 2, Luck 0**.

**Kabul:** Build exit 0. `LightYearsContent` ve `LightYearsPlayerLevelTestProgressionE2E` geçer. Fighter L1: HP 250, AP 39, EP 35, Armor 2, Luck 0; L2: HP 290, AP 42, EP 37, Armor 4, Luck 1.

### Adım 2 — Beş yeni gemiyi `ships.json`'a ekle

**Dosya:** `LightYearsGame/assets/content/data/ships.json`

1. Fighter kaydının hemen altına, Bölüm 1'deki değerlerle sırasıyla Breacher, Interceptor, Conductor, Aegis, Cryo Controller kayıtlarını ekle. Kayıt şablonu olarak Fighter'ı kopyala, yalnız tablolardaki alanları değiştir.
2. ID'ler tam olarak Bölüm 1.1'deki gibi. `ContentIdSchema::ValidateShipId` formatına uyduğunu doğrula.
3. Interceptor: `baseOwnerAttributes`'a `{"attributeId":"Owner.AttackSpeed","baseValue":1.0}`, `naturalGrowth`'a `{"attributeId":"Owner.AttackSpeed","perLevel":1.0}`.
4. Sıra önemli: oyuncu gemileri dosyada düşman gövdelerinden önce ve bu sırada kalmalı (seçim listesi bu sırayı kullanacak).

**Kabul:** Build exit 0; `LightYearsContent` geçer (katalog 6 oyuncu + 3 düşman gemisini yükler, her `primaryWeaponId` çözülür).

### Adım 2b — Kalkan/afterburner'ı EP'den ayır, seviye başı büyüme ekle

**Dosyalar:** `include/gameConfigs/ship/ShipStructs.h`, `include/gameConfigs/ship/ShipConfig.h`, `src/gameplay/content/ShipLoader.cpp`, `src/gameplay/content/ShipContentCatalog.cpp`, `include/gameplay/ship/ShipRuntime.h`, `src/gameplay/ship/ShipRuntime.cpp`, `src/enemy/DummyEnemy.cpp`, `src/player/Player.cpp`, `assets/content/data/ships.json`

1. **`ShipEnergyAttributes`:**
   - `shieldAffinity` ve `afterburnerAffinity` alanlarını **sil**.
   - Yeni alanlar: `float maxShieldPerLevel = 0.f;` ve `float afterburnerCapacityPerLevel = 0.f;`. Varsayılan 0, yani düşman gemileri ve eski içerik değişmeden çalışır.
2. **`ShipLoader::ParseEnergy`:** affinity okumayı kaldır. Yeni iki alanı `object.value("maxShieldPerLevel", 0.f)` / `object.value("afterburnerCapacityPerLevel", 0.f)` ile oku.
3. **`ShipContentCatalog::IsValidShipDefinition`:** affinity kontrollerini ve "toplam = 1" kuralını kaldır. Yeni iki alan için `IsFiniteNonNegative` ekle.
4. **`ShipRuntime`:**
   - `RecalculateAttributes` içindeki EP reaktör terimini (`ReactorBudgetPerEnergyPower`, `reactorBudget × affinity`) **kaldır**. Kalkan = `baseMaxShield + (seviye − 1) × maxShieldPerLevel + mevcut shield contribution'lar`; AB = `baseAfterburnerCapacity + (seviye − 1) × afterburnerCapacityPerLevel`.
   - Yeni `void SetShipLevel(int level)` (1'den küçükse 1'e kenetle); değişince `RecalculateAttributes()`. `int mShipLevel = 1`. `Clear()` 1'e döndürür.
   - EP değişince yeniden hesaplama artık gerekmez: `OnOwnerAttributeChanged` yalnız `MaxHealth` için tetiklenir (HealthRegen hâlâ MaxHealth'ten türüyor).
   - Düşmanların `SetBaseMaxShieldContribution` / `GetAuthoredBaseMaxShield` yolu (`EnemyRuntime.cpp:327-375`) **aynen kalır**; düşman seviye kalkanı oradan gelmeye devam eder.
5. **`Player`:** gemi doğduktan sonra ve `OnShipLevelChanged`'de geminin `ShipRuntime`'ına `SetShipLevel(mShipProgression.GetLevel())` çağır. Respawn'da da çağrıldığından emin ol (yeni gemi L1'den değil oyuncunun seviyesinden başlar).
6. **`DummyEnemy.cpp:15-16`:** affinity atamalarını kaldır (alan yok).
7. **`ships.json`:** altı oyuncu gemisinin `energy` bloğunu Bölüm 1.3b'deki tabloyla yaz; tüm kayıtlardan (düşman gövdeleri dahil) `shieldAffinity` / `afterburnerAffinity` anahtarlarını sil.
8. **`ShipConfig.h`:** Fighter yedeğinin enerji bloğu JSON ile aynı olsun (140 / +20, AB 100 / +3; affinity'siz kurucu sırası).
9. EP'nin başka okuyucuları (yetenekler, `SpaceShip.cpp:187`, `CombatRuntime.cpp:111`) **değişmez**.

**Kabul:** Build exit 0. Hedefli test: L1 Fighter kalkan 140 / AB 100; L2'de 160 / 103; L10'da 320 / 127. EP'yi geçici modifier ile değiştirmek kalkan ve AB tavanını **değiştirmez**. Düşman kalkan büyümesi testleri (`EnemyCombatFoundation` içindekiler) geçer. `GasLiteCoreTests.cpp` içinde affinity veya "EP kalkanı artırır" varsayan eski kontroller kırılırsa **düzeltilmez, listelenir** (kullanıcı kuralı).

### Adım 3 — Gemi seçimini oyuncu akışına bağla (UI'sız)

**Dosyalar:** `include/gameplay/content/ShipContentCatalog.h`, `src/gameplay/content/ShipContentCatalog.cpp`, `include/player/PlayerManager.h`, `src/player/PlayerManager.cpp`, `include/player/Player.h`, `src/player/Player.cpp`

1. **ShipContentCatalog:**
   - `static List<std::string> GetPlayerShipIds();` → yüklü tanımlardan `Ship.Player.` ile başlayanlar, dosya sırasıyla.
   - `static const ShipDefinition& GetPlayerShipDefinition(const std::string& shipId);` → bulunursa onu, bulunmazsa `LY_GAME_ERROR` ile Fighter'ı döndürür.
   - `GetPlayerFighterDefinition()` korunur (testler kullanıyor), içi `GetPlayerShipDefinition("Ship.Player.Fighter.Basic")` olur.
2. **PlayerManager:** `SetSelectedShipId(std::string)` / `GetSelectedShipId()`; varsayılan `"Ship.Player.Fighter.Basic"`. **`Reset()` seçimi silmez** (yalnız oyuncuları siler) — böylece restart'ta seçim korunur.
3. **Player:**
   - Yeni alan `std::string mShipId`. `PlayerManager::CreateNewPlayer()` yeni oyuncuya o anki seçimi verir (ör. `Player` kurucusuna parametre ya da `SetShipId` — mevcut stile uygun olanı seç).
   - `SpawnSpaceShip`: `world->SpawnActor<PlayerSpaceShip>(ShipContentCatalog::GetPlayerShipDefinition(mShipId))`.
   - `mShipProgression.Configure(...)`: aynı gemi tanımının `progressionDefinition`'ı.
   - Bir koşu (run) boyunca gemi değişmez; respawn aynı gemiyi doğurur. Seçim değişikliği bir sonraki yeni `Player`'da (restart / yeni oyun) geçerli olur.
4. `PlayerSpaceShip` kurucusundaki varsayılan argüman (Fighter) **kalır**; testler argümansız kuruyor.
5. `ArenaTestLevel`'deki dummy düşman Fighter tanımını kullanmaya devam eder; dokunma.

**Kabul:** Build exit 0. Yeni hedefli test (Adım 5) seçilen geminin doğduğunu ve büyümenin o gemiden geldiğini doğrular. Seçim değiştirilmezse davranış bugünküyle aynıdır (Fighter).

### Adım 4 — F1–F6 ile gemi değiştirme (seçim ekranı yok)

**Kullanıcı kararı (K1, 2026-10-01):** Şimdilik seçim ekranı yapılmaz. Gemi yalnız F tuşlarıyla değişir. Açılış (`GameApplication` → `ArenaTestLevel`) ve ana menü **değişmez**.

**Dosyalar:** `include/level/ArenaTestLevel.h`, `src/level/ArenaTestLevel.cpp` (gerekirse `GameLevel::DispatchEvent` deseni örnek alınır)

1. `ArenaTestLevel::DispatchEvent` içinde `KeyPressed`:
   - **F1** Fighter, **F2** Breacher, **F3** Interceptor, **F4** Conductor, **F5** Aegis, **F6** Cryo Controller.
   - Eşleme sabit bir tablodan gelir; ID'ler Bölüm 1.1'deki gibi. Tuş sırası `GetPlayerShipIds()` sırasıyla aynı.
2. Tuşa basılınca: `PlayerManager::SetSelectedShipId(id)` → mevcut `OnRestartLevel()` akışı. Restart oyuncuyu sıfırlar (`PlayerManager::Reset()`), yeni oyuncu seçili gemiyle doğar. Seviye/XP/satın alınmış yetenek seviyeleri sıfırlanır; bu bilinçli (yeni koşu).
3. Kısıtlar:
   - Game Over ekranı açıkken ve pause'dayken F tuşları yok sayılır (Escape kuralıyla aynı mantık).
   - Zaten seçili gemiye basılırsa yine restart eder (hızlı sıfırlama olarak da işe yarar). İstenmezse no-op yapılır — uygulayıcı mevcut kodda hangisi daha sade ise onu seçer ve rapora yazar.
   - Seçim değişince log: `LY_GAME_LOG` ile "Player ship selected: <id>".
4. Ekranda geçici bilgi: mevcut notification HUD'u kullanılabiliyorsa "Gemi: Breacher" bildirimi gösterilir; yeni widget yazılmaz.

**Kabul:** Build exit 0. Hedefli test (Adım 5) F tuşu → seçim → restart → doğru gemi zincirini doğrular. Oyun içi görsel kontrol kullanıcıya bırakılır.

### Adım 5 — Testler

1. **`ContentLoaderTests.cpp`** (mevcut, Fighter kontrolünün yanına):
   - 6 oyuncu gemisi yüklenir, `GetPlayerShipIds()` sırası Bölüm 1.1 ile aynı.
   - Her gemi için `health`, `baseOwnerAttributes` (AP/Armor/Luck/AttackSpeed), `energy.baseEnergyPower`, enerji bloğu (Bölüm 1.3b, `maxShieldPerLevel` / `afterburnerCapacityPerLevel` dahil), `naturalGrowth` ve hareket değerleri tablolarla birebir.
   - JSON'da `shieldAffinity` / `afterburnerAffinity` anahtarı kalmamış.
   - Her `primaryWeaponId` `WeaponContentCatalog`'da çözülür.
2. **Yeni `tests/PlayerShipSelectionTests.cpp`** (F1–F6 eşlemesinin doğru ID'yi seçtiği ve restart sonrası o geminin doğduğu da test edilir) (yeni hedef `LightYearsPlayerShipSelectionTests`; `LightYearsGasLiteTestObjects`'e bağlanır, CMake'teki test listesine ve `/INCREMENTAL:NO /DEBUG:FASTLINK` döngüsüne eklenir):
   - Seçim yokken oyuncu Fighter doğurur.
   - Her gemi için: seçim → yeni oyuncu → doğan geminin owner nitelikleri, max kalkanı ve AB kapasitesi L1 değerlerine eşit; 1 seviye XP sonrası L2 değerlerine eşit (ör. Breacher L2 HP 350, AP 47, EP 34, kalkan 120, AB 112).
   - Seviye atlamış oyuncu ölüp respawn olunca yeni geminin kalkan/AB'si oyuncunun seviyesine göre (L1'e değil).
   - `PlayerManager::Reset()` sonrası seçim korunur.
   - Bilinmeyen ID → Fighter'a düşer, çökme yok.
3. Mevcut test listesi: `GameplayTagSchemaTests` geçerli ID örnekleri (gerek yoksa dokunma).

**Kullanıcı kuralı:** `GasLiteCore` gibi büyük testlerin eski Fighter beklentileri (AP 35, HP +75 gibi) kırılırsa **düzeltmeye kaynak harcama**; listele ve raporla. Doğrulama değişikliğe doğrudan dokunan hedefli testlerle sınırlı.

### Adım 6 — Doküman ve kapanış

1. `docs/Light_Years_Ships_Primary_Weapons_Combined.md` başındaki durum notuna "Faz 1 (gemiler) uygulandı, tarih/commit" ekle. (K2/K3 kararları ve kalkan/AB tablosu dokümana plan aşamasında zaten işlendi.)
2. `docs/PROJECT_DOCUMENTATION.md`'de gemi/oyuncu bölümü varsa güncelle.
3. Bu plan dosyasının sonuna "Uygulama sonucu" bölümü: her adımın commit'i, build ve test komutları/sonuçları.

---

## 3. Build ve test komutları

```bat
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
set VSLANG=1033
cd /d D:\LightYears
cmake -S . -B build
cmake --build build
cd build
ctest -j1 -R "LightYearsContent|LightYearsPlayerLevelTestProgressionE2E|LightYearsPlayerShipSelection|LightYearsVitalsHUD"
```

- Log'ları **`.tmp/logs/`** altına yaz, `build/` köküne değil.
- Testleri `-j1` ile koş: `LightYearsContinuousBeamWallE2E` paralel koşuda bilinen biçimde dalgalı.
- Her adım sonunda commit (branch: `codex/luna-callback-lifetime-checkpoint`); push yok.

---

## 4. Bilinen riskler

1. **Interceptor AttackSpeed ↔ mevcut Dual silahı.** Dual bugün `AuthoredScaling` modunda `FireRate += AttackSpeed × 1.0` kuralı taşıyor. AS rating olduğu için (1.0 = %1), Interceptor L1'de Dual **12 → 13 atış/sn**, L10'da **22 atış/sn** olur. Tasarım: `7 × (1 + %AS)`. Faz 2'de Dual baştan yazılacak. Faz 1 için bkz. **K4**.
2. **Motor ışıkları.** `engineMounts` JSON'dan okunmuyor; yeni gövdeler Fighter'ın motor ışığı konumlarını kullanır ve yanlış yerde görünebilir. Faz 1'de kabul; gerekirse ayrı iş olarak `ShipLoader`'a `engineMounts` desteği eklenir.
3. **Mevcut silahlar tasarımdan farklı** (ör. Heat Laser'ın AP katsayısı 0.75, Shotgun pellet'li). Faz 1'de denge ölçümü anlamlı değil; gemi seçimi ve stat doğruluğu test edilir, DPS değil.
4. **F tuşu = restart.** Gemi değiştirmek koşuyu sıfırlar; seviye ve yetenek seviyeleri kaybolur. Test amaçlı kabul edildi.

---

## 5. Kullanıcı kararları (uygulamadan önce)

| # | Karar | Öneri |
|---|---|---|
| **K1** | Gemi seçimi nereden yapılır? | ✅ **Karar verildi:** yalnız F1–F6, seçim ekranı yok (Adım 4). |
| **K2** | Aegis hareket değerleri (tasarımda yok) | ✅ **Karar verildi:** hantal; 480 / 600 / 200 / 240 / 380 / 4.8 |
| **K3** | Kalkan/afterburner modeli ve değerleri | ✅ **Karar verildi:** EP'den ayrıldı; L1 kalkan Fighter 140, Breacher 110, Interceptor 110, Conductor 130, Aegis 160, Cryo 130; seviye başı 10–30; AB seviye başı +2…+4 (Bölüm 1.3b, Adım 2b). |
| **K4** | Faz 1'de Interceptor'ın Dual atış hızı | Dual'ın `AttackSpeed` katsayısını geçici olarak **1.0 → 0.12** yap (12/sn tabanın %1'i = AS rating anlamı). Faz 2'de silah yeniden yazılınca kalkar. |
| **K5** | Yer tutucu sprite'lar | Bölüm 1.4'teki atamalar; sonra değiştirilebilir. |
