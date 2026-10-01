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
- Enerji bloğu: Fighter'ın bloğunun aynısı (`baseEnergyPower` gemiye göre değişir; Breacher'da `baseAfterburnerSpeedMultiplier 1.65`). Tasarım dokümanı diğer gemiler için kalkan/afterburner sayısı vermiyor → bkz. **K3**.

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
| Aegis | **480** | **600** | **200** | **240** | **380** | **4.8** |
| Cryo Controller | 520 | 610 | 210 | 310 | 425 | 5.5 |

Aegis için tasarımda hareket tablosu yok; kalın değerler **öneridir** (en yavaş ve en hantal gövde, "mid-range anchor") → bkz. **K2**.

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

### Adım 4 — Gemi seçim arayüzü

Karar **K1**'e bağlı. Önerilen (A):

1. `GameApplication` açılışta `MainMenuLevel` yükler (bugün `ArenaTestLevel`).
2. `MainMenuHUD`: başlık altına gemi seçici. `MenuLayout` / `StackPanel` ile yatay ya da dikey 6 düğme (gemi adları: Fighter, Breacher, Interceptor, Conductor, Aegis, Cryo Controller), altında seçili geminin rolü + L1 HP/AP/EP özeti tek satır metin. Seçili düğme görsel olarak işaretli.
3. Düğme tıklaması `PlayerManager::SetSelectedShipId(...)` çağırır. "Start" → mevcut akış (`MainMenuLevel::StartGame` → `ArenaTestLevel`).
4. Game Over / Pause'daki "Main Menu" akışı zaten `MainMenuLevel`'e dönüyor; oradan gemi değiştirilip yeniden başlanabilir.
5. UI kuralları: yeni MVVM/Presenter tabanına ve `docs/UI_MIGRATION_PLAN.md`'deki menü katmanı desenine uy; statik menü olduğu için `Menu` katmanı yeterli.

Alternatif (B), hızlı test için: `ArenaTestLevel`'de F1–F6 tuşları gemiyi seçip `OnRestartLevel()` çağırır. A seçilse bile B geliştirici kısayolu olarak eklenebilir.

**Kabul:** Build exit 0; `LightYearsHUDMigrationMenus` geçer (menü düzeni testleri yeni düğmeler için güncellenir). Oyun içi görsel kontrol kullanıcıya bırakılır.

### Adım 5 — Testler

1. **`ContentLoaderTests.cpp`** (mevcut, Fighter kontrolünün yanına):
   - 6 oyuncu gemisi yüklenir, `GetPlayerShipIds()` sırası Bölüm 1.1 ile aynı.
   - Her gemi için `health`, `baseOwnerAttributes` (AP/Armor/Luck/AttackSpeed), `energy.baseEnergyPower`, `naturalGrowth` ve hareket değerleri tablolarla birebir.
   - Her `primaryWeaponId` `WeaponContentCatalog`'da çözülür.
2. **Yeni `tests/PlayerShipSelectionTests.cpp`** (yeni hedef `LightYearsPlayerShipSelectionTests`; `LightYearsGasLiteTestObjects`'e bağlanır, CMake'teki test listesine ve `/INCREMENTAL:NO /DEBUG:FASTLINK` döngüsüne eklenir):
   - Seçim yokken oyuncu Fighter doğurur.
   - Her gemi için: seçim → yeni oyuncu → doğan geminin owner nitelikleri L1 değerlerine eşit; 1 seviye XP sonrası L2 değerlerine eşit (ör. Breacher L2 HP 350, AP 47, EP 34).
   - `PlayerManager::Reset()` sonrası seçim korunur.
   - Bilinmeyen ID → Fighter'a düşer, çökme yok.
3. Mevcut test listesi: `GameplayTagSchemaTests` geçerli ID örnekleri (gerek yoksa dokunma).

**Kullanıcı kuralı:** `GasLiteCore` gibi büyük testlerin eski Fighter beklentileri (AP 35, HP +75 gibi) kırılırsa **düzeltmeye kaynak harcama**; listele ve raporla. Doğrulama değişikliğe doğrudan dokunan hedefli testlerle sınırlı.

### Adım 6 — Doküman ve kapanış

1. `docs/Light_Years_Ships_Primary_Weapons_Combined.md` başındaki durum notuna "Faz 1 (gemiler) uygulandı, tarih/commit" ekle. Aegis hareket değerleri ve K2/K3 kararları hangi değerle kapandıysa dokümana işle.
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
ctest -j1 -R "LightYearsContent|LightYearsPlayerLevelTestProgressionE2E|LightYearsPlayerShipSelection|LightYearsHUDMigration|LightYearsVitalsHUD"
```

- Log'ları **`.tmp/logs/`** altına yaz, `build/` köküne değil.
- Testleri `-j1` ile koş: `LightYearsContinuousBeamWallE2E` paralel koşuda bilinen biçimde dalgalı.
- Her adım sonunda commit (branch: `codex/luna-callback-lifetime-checkpoint`); push yok.

---

## 4. Bilinen riskler

1. **Interceptor AttackSpeed ↔ mevcut Dual silahı.** Dual bugün `AuthoredScaling` modunda `FireRate += AttackSpeed × 1.0` kuralı taşıyor. AS rating olduğu için (1.0 = %1), Interceptor L1'de Dual **12 → 13 atış/sn**, L10'da **22 atış/sn** olur. Tasarım: `7 × (1 + %AS)`. Faz 2'de Dual baştan yazılacak. Faz 1 için bkz. **K4**.
2. **Motor ışıkları.** `engineMounts` JSON'dan okunmuyor; yeni gövdeler Fighter'ın motor ışığı konumlarını kullanır ve yanlış yerde görünebilir. Faz 1'de kabul; gerekirse ayrı iş olarak `ShipLoader`'a `engineMounts` desteği eklenir.
3. **Mevcut silahlar tasarımdan farklı** (ör. Heat Laser'ın AP katsayısı 0.75, Shotgun pellet'li). Faz 1'de denge ölçümü anlamlı değil; gemi seçimi ve stat doğruluğu test edilir, DPS değil.
4. **Açılış değişikliği** (K1-A): oyun artık doğrudan arenaya girmez, menüden başlar.

---

## 5. Kullanıcı kararları (uygulamadan önce)

| # | Karar | Öneri |
|---|---|---|
| **K1** | Gemi seçimi nereden yapılır? | **A**: açılışta ana menü + 6 düğmelik seçici. B (F1–F6 kısayolu) geliştirici aracı olarak ek. |
| **K2** | Aegis hareket değerleri (tasarımda yok) | Bölüm 1.3'teki kalın öneriler: 480 / 600 / 200 / 240 / 380 / 4.8 |
| **K3** | Fighter dışındaki gemilerin kalkan/afterburner değerleri (tasarımda yok) | Fighter'ın enerji bloğu; yalnız Breacher afterburner hız çarpanı 1.65. EP farkı kalkan/afterburner'ı zaten gemiye göre ölçekler. |
| **K4** | Faz 1'de Interceptor'ın Dual atış hızı | Dual'ın `AttackSpeed` katsayısını geçici olarak **1.0 → 0.12** yap (12/sn tabanın %1'i = AS rating anlamı). Faz 2'de silah yeniden yazılınca kalkar. |
| **K5** | Yer tutucu sprite'lar | Bölüm 1.4'teki atamalar; sonra değiştirilebilir. |
