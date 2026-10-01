# HUD Göç Planı — Eski HUD parçalarını MVVM + Presenter tabanına taşıma

Orkestratör: Sol. Uygulayıcı ve inceleyici alt ajanlar: Luna. Sol her ADIM'ı ayrı bir Luna görevine çevirir.
Taban: [UI_FOUNDATION_PLAN.md](UI_FOUNDATION_PLAN.md) (tamamlandı) ve [PROJECT_DOCUMENTATION.md](PROJECT_DOCUMENTATION.md) içindeki "UI tabanı ve vitals sunumu" bölümü.

## Amaç

`GameHUD` ve eski controller'larda kalan HUD parçalarını tek tek yeni desene taşımak, ölü kodu silmek ve sonunda `GameHUD`'ı ince bir kabuğa indirmek. Her adım tek bir parçayı bitirir: kodu taşır, eskisini siler, test eder. Kullanıcının 2026-09-30 tarihli son talimatıyla adımlar otomatik ve seri ilerler; build/test ve gerekli inceleme tamamlanınca sonraki adıma geçilir. Görsel kontroller finalde topluca kullanıcıya sunulur.

**Kapsam dışı (dokunulmaz):** Hasar sayıları, hız yazısı, FPS yazısı (`GameHUD` içindeki `TEMPORARY TEST UI`). Vitals zaten taşındı.

## Referans uygulama: Vitals

Yeni her parça bu dosyaların desenini izler. Luna işe başlamadan önce bunları okur:

- `D:\LightYears\LightYearsGame\include\presentation\hud\vitals\VitalsViewModel.h`: düz struct + `UIRevision`
- `D:\LightYears\LightYearsGame\src\presentation\hud\vitals\VitalsPresenter.cpp`: `SubscriptionSet`, `SetIfChanged`, player/ship yeniden bağlanma
- `D:\LightYears\LightYearsGame\src\presentation\hud\vitals\VitalsView.cpp`: `Panel` alt sınıfı, `UIRevisionWatcher::Consume`, `StackPanel` kompozisyonu
- `D:\LightYears\LightYearsGame\src\presentation\hud\vitals\VitalsHUDController.cpp`: HUD init sonrası `AddToLayer`, destructor'da `DestroyWidget`
- `D:\LightYears\LightYearsGame\tests\VitalsHUDTests.cpp`: headless level kurulumu, JSON artefakt raporu

## Mimari sözleşme (tüm adımlar için bağlayıcı)

1. **Model gameplay'dir.** UI, gameplay durumunun kopyasını tutmaz. ViewModel yalnız gösterilecek türetilmiş değerleri tutar.
2. **ViewModel** düz bir struct'tır. Her alan `SetIfChanged(field, value, vm.revision)` ile yazılır.
3. **ViewModel'den View'a callback yoktur.** View her `Tick`'te `UIRevisionWatcher::Consume` ile değişikliği yakalar. VM üzerinde `Delegate` tanımlanmaz.
4. **Presenter** gameplay'e bağlanır ve lifetime'ı yönetir. Abonelikler `SubscriptionSet` ile tutulur, ham `DelegateHandle` alanı tutulmaz. Guarded `Bind` delegate'i kaynak nesne sahipleniyorsa kullanılır. Kaynak shared_ptr değilse `BindUnguarded` kullanılır ve kaynak silinmeden önce `Clear` garanti edilir.
5. **View** yalnız widget kompozisyonudur ve oyun mantığı içermez. Sunuma özgü zaman durumu (fade, titreme animasyonu) View'da tutulabilir. Kullanıcı girdisi View'dan dışarı niyet delegate'i olarak çıkar.
6. View, VM'yi `shared_ptr<const VM>` ile tutar.
7. **Konumlandırma yalnız `UILayout`/`StackPanel` ile yapılır.** Yeni kodda `SetWidgetLocation`, `CenterOrigin` ve `GetWindowSize()` ile elle konum hesabı yapılmaz. Konum değişecekse layout'un `offset` alanı güncellenir.
8. **Pragmatizm:** Arkasında dinamik model verisi olmayan statik görünümler (menüler) için ViewModel veya Presenter uydurulmaz. View + niyet delegate'i yeterlidir.

## Ortak kurallar (her Luna adımı)

- Luna yalnız kendisine atanan ADIM'ı uygular. Git komutu (commit, stash, restore, checkout) çalıştırmaz. Çalışma ağacındaki ilgisiz dirty değişiklikler korunur.
- **Satır numaraları bu planın yazıldığı andaki durumu gösterir.** Önceki adımlar dosyayı değiştirmiş olabilir. Luna fonksiyon adıyla konumu doğrular, satır numarasına körü körüne güvenmez.
- C++17, SFML 3, namespace `ly`, tab girinti, çevredeki kodun adlandırma ve yorum yoğunluğu.
- Yeni game kaynakları `D:\LightYears\LightYearsGame\CMakeLists.txt` içindeki `LIGHT_YEARS_UI_HUD_SOURCES` listesine eklenir (satır ~58). Bu liste hem oyun hedefine hem test kaynaklarına bağlıdır. Silinen dosyalar hem bu listeden hem de oyun ve `LIGHT_YEARS_GAS_LITE_TEST_SOURCES` listelerindeki açık girdilerden çıkarılır.
- Engine UI kuralları için `D:\LightYears\docs\PROJECT_DOCUMENTATION.md` içindeki "Kurallar ve bilinen tuzaklar" listesine uyulur. Özellikle: layout'suz çocuk parent'ı izlemez; boyutu değişen widget `InvalidateLayout()` çağırır.
- İlgisiz refactor, denge değişikliği veya biçimlendirme yapılmaz.
- **Luna kendi adımını derler ve testlerini koşar.** Build veya test kırmızıysa adım bitmiş sayılmaz.
- **Rapor (Sol'a):** değişen ve silinen dosyalar; özet; çalıştırılan komutlar ve exit kodları; geçen/kalan testler; kullanıcının oyunda bakması gereken görsel noktalar; belirsizlikler. Çalıştırılmayan doğrulama "çalıştırılmadı" diye açıkça yazılır.

## Doğrulama komutları

```bat
cmd /c "call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && set VSLANG=1033 && cmake --build build --target <HEDEFLER> --parallel 4"
ctest --test-dir build -R "<REGEX>" --output-on-failure --timeout 90
```

`VSLANG=1033` zorunludur (bkz. `AGENTS.md`). Yeni test hedefi ekleyen adımda önce `cmake -S . -B build` çalıştırılır.

**Her adımın temel regresyon seti** (adımın kendi testlerine ek olarak):
- Hedefler: `LightYearsGame LightYearsUIFoundationTests LightYearsVitalsHUDTests LightYearsHUDMigrationTests LightYearsGasLiteTests LightYearsContinuousBeamWallE2ETests`
- ctest: `^(LightYearsUIFoundation|LightYearsVitalsHUD|LightYearsHUDMigration.*|LightYearsGameHUDDamageEventE2E|LightYearsGameHUDPlayerRestart)$`

`LightYearsHUDMigrationTests` ADIM 2'de oluşturulur. ADIM 1'de bu hedef ve regex parçası atlanır.

## Sol için orkestrasyon notları

- **Seri yürüt.** Aynı `build` dizini, `GameHUD.cpp`, CMake ve test dosyası paylaşılıyor. Aynı anda tek uygulayıcı çalışır.
- **Adımlar arasında otomatik ilerleme.** Kullanıcının son talimatı: "devam et biri bitince diğerine başla otomatik olarak". Sol her adımın build/test ve gerekli incelemesini tamamlayınca sıradaki adımı başlatır; görsel kontrol maddelerini finalde topluca iletir.
- **İnceleyici:** ADIM 2 (desenin ilk tekrarı), ADIM 6 (en büyük göç), ADIM 7 (input davranışı) ve ADIM 8 (final) sonrası salt okunur bir Luna inceleyici diff'i aşağıdaki listeye göre denetler.
- **İnceleyici kontrol listesi:** sözleşme maddeleri 1–8; silinen `GameHUD` üyelerinin başka yerde kullanılmadığı (grep ile); yeni kodda `SetWidgetLocation`/`CenterOrigin`/`GetWindowSize` yok; ham `DelegateHandle` yok; VM üzerinde `Delegate` yok; CMake listeleri tutarlı; raporlanan build/test sonuçları gerçekten çalıştırılmış.
- **Her Luna görevine verilecek bağlam:** "Mimari sözleşme", "Ortak kurallar", "Doğrulama komutları" bölümleri, ilgili ADIM ve önceki adımların kısa özeti.
- **Bilinen ortam gerçekleri:**
  - HUD'lar (`World::TickInternal` içinde) oyun duraklatılmışken de tick alır.
  - `GameLevel::Tick` ve dolayısıyla tüm `HUDController::Tick` çağrıları **duraklatılmışken çalışmaz**.
  - Overlay HUD (pause, game over) varken `World::DispatchEvent` event'i yalnız overlay HUD'a verir.

---

## ADIM 1: Boss can barını sil (ölü kod)

**Neden:** `CreateBossHealthBar`, `BossHealthUpdated` ve `RemoveBossHealthBar` projede hiçbir yerden çağrılmıyor (grep ile doğrulandı). Kullanıcı silinmesine karar verdi. İleride boss gelirse yeni sistemle sıfırdan yazılacak.

**Değişecek dosyalar**
- `D:\LightYears\LightYearsGame\include\widget\GameHUD.h`
  - Satır 37–39: `CreateBossHealthBar`, `BossHealthUpdated`, `RemoveBossHealthBar` bildirimleri silinir.
  - Satır 80–81: `mBossHealthBar`, `mBossNameText` üyeleri silinir.
  - Satır 15: `class Actor;` forward declaration yalnız `RemoveBossHealthBar` için kullanılıyorsa silinir. Luna dosyada başka kullanım olup olmadığını kontrol eder.
- `D:\LightYears\LightYearsGame\src\widget\GameHUD.cpp`: satır 317–363 arasındaki üç fonksiyon gövdesi silinir.

**Önce doğrula:** `CreateBossHealthBar|BossHealthUpdated|RemoveBossHealthBar|mBossHealthBar|mBossNameText` için `D:\LightYears\LightYearsGame` ve `D:\LightYears\LightYearsEngine` altında `src`, `include` ve `tests` klasörlerinde grep yapılır. `GameHUD` dışında sonuç çıkarsa silme yapılmaz ve Sol'a raporlanır.

**Test:** Yeni test yok (silme). Temel regresyon seti koşulur (`LightYearsHUDMigrationTests` hariç).

**Görsel kontrol (kullanıcı):** Değişiklik yok. Oyun normal açılıyor mu?

## ADIM 2: Oyun uyarıları (arena sınırı) + ortak göç test hedefi

**Mevcut durum**
- Akış: `ArenaLevel::OnArenaBoundaryWarningUpdated` (`D:\LightYears\LightYearsGame\src\level\ArenaLevel.cpp` ~309) her frame `GameLevel::BroadcastGameplayWarning` → `HUDController::ShowGameplayWarning` → `GameplayWarningHUDController` → `GameHUD::ShowGameplayWarning` çağırır. Temizleme de `ClearGameplayWarning` → `HideGameplayWarning` yoluyla gider.
- `GameHUD.cpp` satır 365–450: `ShowGameplayWarning` metni biçimler (`"%s\n%s %.2f"` countdown ile, `"%s\n%s"` countdown olmadan), `HideGameplayWarning` yalnız aynı türdeyse gizler, `UpdateGameplayWarningVisuals` titreme animasyonunu yapar.
- Animasyon formülü (birebir korunacak):
  - `pulse = (sin(t*9.5)+1)/2`
  - `flicker = (sin(t*37)+1)/2`
  - `threat = max(pulse, flicker*0.65)`
  - renk `{255, 35+threat*45, 35+threat*45, 205+threat*50}`
  - titreme `{sin(t*51)*1.8*threat, sin(t*29)*1.2*threat}`
- Stil: 24 punto, `OrbitronBlack.ttf`, metin merkezi `(W/2, 54)`. Yeni bir uyarı türü gelince animasyon zamanı sıfırlanır.
- `GameHUD.h`: satır 42–43 (Show/Hide), 47 (`UpdateGameplayWarningVisuals`), 71 (`mTopCenterText`), 86–89 (uyarı durum üyeleri). `GameHUD.cpp`: constructor satır 21 ve 25–27, `Draw` satır 46–47, `Tick` satır 70, `Init` satır 103.

**Yeni dosyalar** (`presentation/hud/warning/`)
- `include\presentation\hud\warning\GameplayWarningViewModel.h`
  ```cpp
  struct GameplayWarningViewModel
  {
  	bool visible{ false };
  	GameplayWarningType type{ GameplayWarningType::ArenaBoundary };
  	std::string text;          // biçimlenmiş, iki satır
  	std::uint32_t activation{ 0 }; // yeni tür gösterildiğinde artar; View animasyon zamanını buna göre sıfırlar
  	UIRevision revision;
  };
  std::string FormatGameplayWarningText(const GameplayWarning& warning); // mevcut snprintf mantığı birebir
  struct GameplayWarningPulse { sf::Color color; sf::Vector2f shake; };
  GameplayWarningPulse ComputeGameplayWarningPulse(float animTime);        // mevcut formül birebir, saf fonksiyon
  ```
- `include\presentation\hud\warning\GameplayWarningView.h` + `src\...\GameplayWarningView.cpp`
  - `Panel` alt sınıfı, tek `TextWidget` (24 punto, `OrbitronBlack.ttf`).
  - Layout: anchorMin = anchorMax = (0.5, 0), pivot (0.5, 0.5), offset (0, 54). Metnin merkezi eskisi gibi (W/2, 54)'te durur.
  - `Tick`: revision değiştiyse metni ve görünürlüğü uygular. `activation` değiştiyse animasyon zamanını sıfırlar. Görünürken her frame `ComputeGameplayWarningPulse` ile rengi uygular ve titremeyi `layout.offset = base + shake` ile verir.
- `GameplayWarningHUDController` yeniden yazılır. Sınıf adı ve dosya yolu değişmez (`D:\LightYears\LightYearsGame\include\presentation\hud\GameplayWarningHUDController.h`, `src\...\GameplayWarningHUDController.cpp`).
  - VM'yi (`shared_ptr`) sahiplenir.
  - `ShowGameplayWarning`: `SetIfChanged(visible,true)`, `SetIfChanged(text, Format...)`. Tür değiştiyse veya önceden görünmüyorsa `type` yazılır ve `activation` artırılır (revision bump).
  - `HideGameplayWarning(type)`: yalnız görünür ve tür eşleşiyorsa `visible=false`.
  - `Tick`: HUD init olduysa ve view yoksa `AddToLayer<GameplayWarningView>(UILayer::Hud, vm)`. Destructor'da view `DestroyWidget` edilir (Vitals controller gibi).

**GameHUD'dan silinecekler:** `ShowGameplayWarning`, `HideGameplayWarning`, `UpdateGameplayWarningVisuals`, `mTopCenterText` ve tüm kullanımları, `mHasActiveGameplayWarning`, `mActiveGameplayWarningType`, `mGameplayWarningAnimTime`, `mGameplayWarningBaseLocation`, `#include "gameplay/GameplayWarning.h"` (başka kullanım yoksa). `GameLevel`'daki broadcast yolu değişmez.

**Ortak test hedefi (bu adımda oluşturulur)**
- Yeni dosya `D:\LightYears\LightYearsGame\tests\HUDMigrationTests.cpp`: `main(argc, argv)` bayrakla alt test seçer (`--warning`, sonraki adımlarda `--notification`, `--encounter`, `--ability`, `--menus`). Her alt test bir JSON artefakt yazar (`VitalsHUDTests.cpp`'deki rapor yardımcısını kopyala). Headless kurulum `VitalsHUDTests.cpp`'den örnek alınır.
- CMake: `LightYearsVitalsHUDTests` bloğu (satır ~1509) kopyalanarak `LightYearsHUDMigrationTests` executable'ı oluşturulur. Her bayrak için ayrı `add_test`: `NAME LightYearsHUDMigrationWarning COMMAND LightYearsHUDMigrationTests --warning "${LIGHT_YEARS_E2E_ARTIFACT_DIR}/hud-migration-warning.json"`, `TIMEOUT 60`.

**`--warning` testleri**
1. `FormatGameplayWarningText`: countdown'lı ve countdown'sız metin eski biçimle birebir aynı (örnek: `"UNAUTHORIZED REGION\nRETURN IN 2.50"`).
2. `ComputeGameplayWarningPulse(0)` ve `(0.1)` eski formülün elle hesaplanan değerlerine eşit.
3. Controller: Show → `visible` ve metin doğru. Aynı türde ikinci Show `activation`'ı artırmaz, yalnız metin değişir. Farklı türde Hide gizlemez. Aynı türde Hide gizler. Hide'dan sonra Show `activation`'ı artırır.
4. View: HUD `SetViewportSize({1920,1080})` ve Tick sonrası metin merkezi yaklaşık (960, 54). Gizlenince `GetVisibility()==false`.

**Görsel kontrol (kullanıcı):** Arena sınırından çıkınca üstte kırmızı, titreyen "UNAUTHORIZED REGION / RETURN IN x.xx" yazısı eskisi gibi görünüyor ve geri dönünce kayboluyor mu?

## ADIM 3: Geçici bildirimler ve sayaç (LevelOne "SURVIVE!" ve "TIME LEFT")

**Mevcut durum**
- `D:\LightYears\LightYearsGame\src\level\LevelOne.cpp` satır 114–124 (`ConnectChaosStageToHUD`): `chaosStage` delegate'leri doğrudan `GameHUD` metotlarına bağlanıyor.
  - `onNotification` → `ShowDynamicNotification`
  - `onTotalChaosStarted` → `ShowTimer`
  - `onChaosTimerUpdated` → `UpdateTimer`
  - `onTotalChaosEnded` → `TimerFinished`
- Delegate imzaları:
  - `GameStage::onNotification`: `Delegate<const std::string&, float, float, float, const sf::Vector2f&, float, sf::Color>` (metin, fadeIn, hold, fadeOut, konum, boyut, renk). Tanım: `D:\LightYears\LightYearsEngine\include\gameplay\GameStage.h:21`.
  - `ChaosStage`: `onTotalChaosStarted(fadeIn, hold, fadeOut)`, `onChaosTimerUpdated(timeLeft)`, `onTotalChaosEnded()`. Tanım: `D:\LightYears\LightYearsGame\include\enemy\ChaosStage.h:20-22`.
- `GameHUD.cpp` satır 80–94 (`ShowDynamicNotification`): `AddWidget<TextWidget>` → `CenterOrigin`, verilen konum, fade ve lifetime. Tek gerçek çağrı `ChaosStage.cpp:233`: "SURVIVE!", 0.5/1.5/0.5, ekran merkezi, 50 punto, kırmızı.
- `GameHUD.cpp` satır 287–315 (`ShowTimer`/`UpdateTimer`/`TimerFinished`): üst ortada `"TIME LEFT: " + ceil(t)`, 20 punto, kırmızı, fade in. Merkezi y=0'da olduğu için metnin yarısı ekran dışında kalıyor (eski hata).
- `GameStage` bir `Object`'tir ve `shared_ptr` ile yönetilir. `LevelOne` zaten `chaosStage->...BindAction(GetWeakPtr()...)` kullanıyor.

**Yeni dosyalar** (`presentation/hud/notification/`)
- `NotificationViewModel.h`
  ```cpp
  struct NotificationRequest { std::string text; float fadeIn, hold, fadeOut; unsigned int size; sf::Color color; std::uint32_t id; };
  struct NotificationViewModel
  {
  	std::vector<NotificationRequest> pending; // View bir kez tüketir (id ile)
  	bool timerVisible{ false };
  	int timerSeconds{ 0 };                    // ceil(timeLeft); saniye değişmedikçe revision artmaz
  	float timerFadeIn{ 0.f };
  	std::uint32_t timerActivation{ 0 };       // yeni sayaç başlarken artar (fade in için)
  	UIRevision revision;
  };
  ```
  Bildirimler olay niteliğindedir. Presenter her isteğe artan bir `id` verir ve `pending` içinde yalnız son 8 isteği tutar (eskisini atar). View işlediği son `id`'yi kendisi saklar ve yalnız daha büyük `id`'li istekleri işler. View'dan presenter'a geri bildirim yoktur.
- `NotificationPresenter.h/.cpp` (kontrolcü içinde de olabilir, Vitals'taki gibi ayrı tutulması tercih edilir)
  - `void BindChaosStage(const shared_ptr<ChaosStage>& stage)`: `SubscriptionSet::Bind(stage, stage->onNotification, this, &...)` vb. Guarded, kaynak `stage`. Önceki bağlantılar önce `Clear` edilir.
  - Handler'lar: `OnNotification(...)` konum parametresini **yok sayar** (View ekran merkezine anchor'lar). `OnTimerStarted(fadeIn, hold, fadeOut)`, `OnTimerUpdated(t)`, `OnTimerEnded()`.
- `NotificationView.h/.cpp` (`Panel`, katman `UILayer::Hud`)
  - Bildirim: her istek için `AddChild<TextWidget>(text, OrbitronBlack, size)`. Layout Center (anchor ve pivot 0.5,0.5). `SetFillColor(color)`, `StartFadeAnimation(fadeIn, hold, fadeOut)`, `SetLifeTime(fadeIn+hold+fadeOut+1)`. Süresi dolan çocuk, Widget Tick'inde kendiliğinden silinir.
  - Sayaç: tek `TextWidget`, 20 punto, kırmızı. Layout anchor (0.5, 0), pivot (0.5, 0), offset (0, 8), yani eski yarım görünme hatası düzeltilir. `timerActivation` değişince `StartFadeAnimation(timerFadeIn, 0, 0)` uygulanır.
- `NotificationHUDController.h/.cpp`: presenter ve view'ı sahiplenir. Public `NotificationPresenter& GetPresenter()` veya doğrudan `BindChaosStage` sunar.
- `D:\LightYears\LightYearsGame\include\level\GameLevel.h` / `src\level\GameLevel.cpp`: `CreateHUDControllers` içinde `mNotificationHUD = make_shared<NotificationHUDController>(mGameHUD)` oluşturulur, `AddHUDController` ile eklenir ve `weak_ptr<NotificationHUDController> GetNotificationHUD() const` erişimcisi eklenir.
- `LevelOne::ConnectChaosStageToHUD`: dört `BindAction` satırı silinir, yerine `if (auto notifications = GetNotificationHUD().lock()) notifications->BindChaosStage(chaosStage);` gelir.

**GameHUD'dan silinecekler:** `ShowDynamicNotification`, `ShowTimer`, `UpdateTimer`, `TimerFinished`, `mTimerText`, `mCenterNotificationText`, constructor'daki yorum bloğu (satır 29–33). `mWindowSize` başka yerde (hasar sayıları) kullanılmıyorsa ADIM 8'de temizlenir, bu adımda dokunulmaz.

**Dikkat:** `HUDController::Tick` duraklatılmışken çalışmaz. Bildirimler olay geldiği anda VM'ye yazılır. View HUD Tick'inde çalıştığı için gösterim duraklatmadan etkilenmez.

**`--notification` testleri**
1. Presenter'a bir `ChaosStage` (veya test için `GameStage` alt sınıfı; `ChaosStage` constructor'ı `World*` istiyor, headless level ile kurulabilir) bağlanır. `onNotification.Broadcast(...)` → VM'de bir istek oluşur. View tick sonrası bir metin çocuğu vardır ve metni "SURVIVE!"dır.
2. Toplam süre + 1 saniye tick'lenince bildirim çocuğu silinir.
3. Sayaç: Started → görünür. `Updated(4.2)` → "TIME LEFT: 5". `Updated(4.1)` → revision artmaz. Ended → gizli.
4. Stage yok edildikten sonra presenter `Clear`'ı crash olmadan çalışır (guarded).
5. Sayaç metninin üst kenarı y ≥ 0 (yarım görünme düzeltmesi).

**Görsel kontrol (kullanıcı):** LevelOne'da zorluk 12'ye çıkınca ortada "SURVIVE!" belirip sönüyor mu? Ardından üst ortada "TIME LEFT: N" tam görünüyor ve süre bitince kayboluyor mu?

## ADIM 4: Dalga bilgisi (EncounterHUDController)

**Mevcut durum**
- `D:\LightYears\LightYearsGame\include\presentation\hud\encounter\EncounterHUDController.h` ve `src\...\EncounterHUDController.cpp` (237 satır).
- Saf `BuildEncounterHUDViewModel(snapshot)` (satır 56–85) **korunur**. Testlerde (`EnemyCombatFoundationTests.cpp` satır 1274–1306 "[TEST Encounter HUD View Model]") kullanılıyor ve doğru.
- Controller: `GameHUD::AddWidget` ile iki `TextWidget` ekliyor, `GetWindowSize()` ile elle konumluyor (dalga başlığının merkezi y=38, detayın merkezi y=72). Değişiklik tespiti için yedi `mLast*` alanı tutuyor.
- Stil: font `OrbitronBlack.ttf`, başlık 24 punto, detay 17 punto. Renkler: başlık (190,230,255), tamamlandı (140,255,190), detay (150,190,220). Boş metin gizlenir.
- Oluşturan yer: `D:\LightYears\LightYearsGame\src\level\ArenaTestLevel.cpp` satır 122–133. Snapshot provider imzası değişmez.

**Değişiklik**
- `EncounterHUDViewModel` (başlıkta tanımlı struct) **olduğu gibi kalır**. Test sözleşmesi olarak saf projeksiyon görevini sürdürür.
- Yeni `EncounterHUDPresentation` struct'ı (aynı başlığa veya `EncounterHUDPresentation.h`'a): `bool visible; std::string title; std::string detail; sf::Color titleColor; UIRevision revision;`
- `EncounterHUDController::Tick` presenter olur: provider'dan snapshot alır, `BuildEncounterHUDViewModel` çağırır ve sonucu `SetIfChanged` ile presentation'a yazar (başlık rengi `ResolveWaveTextColor`). Tüm `mLast*` alanları, `EnsureWidgets`, `UpdateWidgets`, `HideWidgets`, `RemoveWidgets`, `PositionWidgets`, `mWaveText`, `mDetailText`, `mLastWindowSize` ve `GetWindowSize` kullanımı silinir.
- Yeni `EncounterHUDView` (`Panel`): dikey `StackPanel` (spacing 8, cross Center), layout anchor (0.5, 0), pivot (0.5, 0), offset (0, 24), yani başlık merkezi yaklaşık y=38. İki `TextWidget` çocuğu. Detay boşsa gizlenir (StackPanel boşluğu kapatır). Tümü görünmüyorsa panel gizlenir.
- Controller view'ı HUD init sonrası `AddToLayer<EncounterHUDView>(UILayer::Hud, presentation)` ile ekler. Destructor'da `DestroyWidget`.

**Test taşıma**
- `D:\LightYears\LightYearsGame\tests\EnemyCombatFoundationTests.cpp` satır 1790–1884 ("[TEST Encounter HUD Presentation]" bloğu) eski `mWidgets` listesini sayıyor (`EncounterHUDTestHUD`, satır 253–267), bu yüzden taşıma sonrası kırılır.
  - Bu blok ve yalnız onun kullandığı `EncounterHUDTestHUD` sınıfı buradan silinir. `MakeWeakLevelSnapshotProvider` başka yerde kullanılmıyorsa o da taşınır.
  - Senaryolar `HUDMigrationTests.cpp` içinde `--encounter` alt testine yeniden yazılır: widget sayısı yerine view'ın görünür metin çocukları, metin içerikleri ve renkleri doğrulanır. Weak-level senaryosu için `ArenaLifecycleTestLevel` taşınamıyorsa yalnız "provider boş snapshot döndürünce view gizli" durumu test edilir.
- "[TEST Encounter HUD View Model]" bloğu (satır 1274–1306) **olduğu gibi kalır**.
- CMake: `add_test(NAME LightYearsHUDMigrationEncounter ... --encounter ...)`.

**`--encounter` testleri:** Idle → view gizli. Spawning → iki satır, "WAVE 2 / 3" ve "ENEMIES 4 • LEVEL 2". Completed → tek satır, tamamlandı rengiyle. Failed → gizli. Aynı snapshot ile 16 tick → revision sabit ve view yenileme sayacı artmaz. Controller yok edilince view ağaçtan kalkar (layer çocuk sayısı eski değerine döner).

**Görsel kontrol (kullanıcı):** ArenaTestLevel'da üst ortada "WAVE x / y" ve altında "ENEMIES n • LEVEL m" eskisi gibi mi? Dalga arasında "NEXT WAVE IN n" görünüyor mu?

## ADIM 5: Ability bar — ViewModel ve Presenter

**Mevcut durum** (`D:\LightYears\LightYearsGame\src\presentation\hud\ability\AbilityUIController.cpp`, 527 satır)
- **Slot kurulumu (`EnsureWidgetsCreated`, satır 85–216):** Oyuncunun gemisi yoksa veya pending destroy ise tüm widget'lar sıfırlanır. Gemi ID'si değiştiyse sıfırlanıp yeniden kurulur. Ability1..4 slotlarından yalnız ability'si olan **ve** `definition.iconPath` boş olmayanlar gösterilir. Kaldırılan ability'nin widget'ları silinir.
- **Durum (`RefreshFromAbilitySystem` + `UpdateWidgetVisuals`, satır 241–357):**
  - aktif → ikon alpha 1.0, "ACTIVE", accent rengi
  - cooldown → alpha 0.35, `"%.1fs"`, renk (220,170,80)
  - hazır → alpha 0.9, "READY", renk (120,255,140)
  - Tuş etiketi `AbilityInputSchema::GetLabel(slot)`, accent rengiyle.
- **Geçici stat bloğu (satır 359–460, `TEMPORARY TEST UI`):** Ship ability component delegate'leri (`onAbilityGranted`, `onAbilityRemoved`, `onAbilityChanged`, `onAbilityLevelChanged`) ham `DelegateHandle` dizisiyle bağlanıyor ve yalnız dirty işaretliyor. Metin: `ShortAbilityName` + "Lv n" + "CD xs" + en fazla 6 attribute (`AbilityActionAttributeResolver::ResolveAbilityAttributes`).
- **Yerleşim (`PositionWidgets`, satır 462–524):**
  - ikon 48 varsayılıyor, slot aralığı 130 (pitch 178), yatay ortalı
  - ikonun üst kenarı `H - 112`
  - tuş etiketi ikonun 20 px üstünde
  - durum yazısı ikonun 4 px altında
  - stat bloğu etiketin üstünde, yukarı doğru büyüyor
- Eski `D:\LightYears\LightYearsGame\include\presentation\hud\ability\AbilityUIViewModel.h` üzerinde `Delegate<sas::AbilitySlot> onSlotDataChanged` var (sözleşme 3'ü ihlal ediyor). Yalnız `AbilityUIController` kullanıyor.

**Yeni dosyalar** (`presentation/hud/ability/`)
- `AbilityBarViewModel.h`
  ```cpp
  enum class AbilitySlotState { Ready, Cooldown, Active };
  struct AbilitySlotViewData
  {
  	bool visible{ false };
  	std::string iconPath;
  	std::string inputLabel;
  	sf::Color accentColor{ sf::Color::White };
  	AbilitySlotState state{ AbilitySlotState::Ready };
  	int cooldownTenths{ 0 };   // round(remaining*10); her frame değil, 0.1 s adımında değişir
  	std::string statsText;     // TEMPORARY TEST UI
  };
  struct AbilityBarViewModel
  {
  	std::array<AbilitySlotViewData, 4> slots; // Ability1..Ability4 sırası
  	std::uint32_t shipGeneration{ 0 };         // gemi değiştiğinde artar; View ikonları yeniden kurar
  	UIRevision revision;
  };
  ```
  `AbilitySlotViewData` için `operator==` tanımlanır, böylece `SetIfChanged` slot başına çalışır.
- `AbilityBarPresenter.h/.cpp`
  - `Tick()`: player → ship çözülür (Vitals presenter deseni). Ship yoksa veya pending destroy ise tüm slotlar `visible=false`, abonelikler `Clear`.
  - Ship ID değiştiyse: `mShipSubs.Clear()`, dört delegate'e `mShipSubs.Bind(shipWeak, abilitySystem.onAbilityX, this, &...)` (guarded, kaynak ship), `shipGeneration` artırılır, stat dirty işaretlenir.
  - Her slot için poll: ability yoksa veya ikonu boşsa `visible=false`. Varsa ikon, etiket, accent ve durum yazılır. `cooldownTenths = lround(GetCooldownRemaining()*10)`.
  - Stat metni yalnız dirty iken yeniden hesaplanır. `ShortAbilityName` ve `FormatStatValue` yardımcıları eski dosyadan buraya taşınır, "TEMPORARY TEST UI" yorumu korunur.
  - Tüm yazımlar `SetIfChanged(vm.slots[i], newData, vm.revision)` ile yapılır.
- Bu adımda **eski controller'a dokunulmaz**. Presenter yalnız testlerle doğrulanır, oyuna ADIM 6'da bağlanır.

**`--ability` testleri (presenter kısmı; view testleri ADIM 6'da eklenir)**
1. Headless level + shipped oyuncu gemisi (VitalsHUDTests kurulumu). Tick sonrası ikonu olan slotlar `visible`, etiketler `AbilityInputSchema::GetLabel` ile aynı.
2. Bir slotu aktive et (`GameplayAbilityInstance::TryActivate`, `D:\LightYears\SpaceAbilitySystem\include\abilities\GameplayAbilityInstance.h:96`; ya da component üzerindeki mevcut aktivasyon yolu). Birkaç tick sonra durum Active veya Cooldown olmalı. Cooldown süresince `cooldownTenths` azalır.
3. Hiçbir şey değişmeden iki tick → revision sabit.
4. Ability seviyesi değişince (`onAbilityLevelChanged`) statsText değişir.
5. Gemi yok edilip respawn olunca `shipGeneration` artar, slotlar yeni gemiden dolar, eski geminin delegate'i tetiklenince VM değişmez.
- CMake: `add_test(NAME LightYearsHUDMigrationAbility ... --ability ...)`.

**Görsel kontrol:** Yok (oyuna henüz bağlanmadı).

## ADIM 6: Ability bar — View, controller ve eski kodun silinmesi

**Yeni dosyalar**
- `AbilityBarView.h/.cpp` (`Panel`)
  - Kök: şeffaf 1×1 `Panel`, içinde yatay `StackPanel`, spacing 28, cross align End. Layout anchor (0.5, 1), pivot (0.5, 1). Offset, eski 48px mantıksal ikon hücrelerinin merkezini ve `H-112` ikon üst kenarını koruyacak şekilde feature-local çözülür. 150 px kolon içinde sola hizalı 48 px ikonlar varsa kolon merkezini doğrudan viewport merkezine koymak ikonları 51 px sola kaydırır; bu fark layout offset/kompozisyon ile giderilir. Satır boyları mevcut fontun gerçek intrinsic ölçüsüyle doğrulanır.
  - Her slot için bir kolon: 150×200 sabit `Panel`, çocukları alt anchor/offset ile konumlanır. Değişken stat satırlarının ikon tabanını kaydırmaması için dikey StackPanel yerine feature-local sabit kolon kullanılır. Sabit kolon genişliği ve 28 aralık, eski 178 px pitch'i korur; stat metni sağa taşabilir. Kolon çocukları yukarıdan aşağı: stat metni (11 punto, (220,235,255)), tuş etiketi (14 punto), ikon (`ImageWidget`), durum metni (15 punto). Font `OrbitronBlack.ttf`.
  - Dört kolon constructor'da kurulur. Görünmeyen slotun kolonu gizlenir (StackPanel boşluğu kapatır, kalanlar yeniden ortalanır).
  - `Tick`: revision değiştiyse slot başına uygulanır. `shipGeneration` veya `iconPath` değiştiyse ikon kendi doğru texture yolu ile yeniden oluşturulur; boş VM'de placeholder ikon yüklenmez. Mevcut `ImageWidget::SetImage` eski sprite rect'ini tuttuğundan farklı boyutlu texture geçişinde feature-local yeniden oluşturma gerekir. Alpha: Active 1.0, Cooldown 0.35, Ready 0.9. Durum metni ve rengi eski kuralla aynı; cooldown metni `cooldownTenths/10` ile `"%.1fs"`.
- `AbilityBarHUDController.h/.cpp`: presenter ve view'ı sahiplenir. Desen Vitals controller ile aynı.
- `D:\LightYears\LightYearsGame\src\level\GameLevel.cpp` `CreateHUDControllers` (satır ~116): `std::make_shared<AbilityUIController>(mGameHUD)` satırı `AbilityBarHUDController` ile değiştirilir. Include güncellenir.

**Silinecek dosyalar**
- `D:\LightYears\LightYearsGame\include\presentation\hud\ability\AbilityUIController.h`
- `D:\LightYears\LightYearsGame\src\presentation\hud\ability\AbilityUIController.cpp`
- `D:\LightYears\LightYearsGame\include\presentation\hud\ability\AbilityUIViewModel.h`
- CMake'teki açık girdileri: oyun listesi satır ~903–905 ve `LIGHT_YEARS_GAS_LITE_TEST_SOURCES` içindeki `AbilityUIController.cpp`. Önce grep ile tüm referanslar bulunur.

**`--ability` testlerine eklenecek view kısmı**
1. VM'de 2 görünür slot → view'da 2 görünür kolon, kolonlar aynı taban çizgisinde (ikonların alt kenarı eşit).
2. Cooldown durumunda ikonun uygulanan alpha değeri 0.35 (`GetEffectiveAlpha`), Ready'de 0.9.
3. 1920x1080 viewport'ta eski 48px mantıksal ikon hücrelerinin orta noktası ekran ortasına göre simetrik (aynı shipped texture kullanılarak fiziksel sınır merkezi `W/2 - 24 + textureWidth/2` ile doğrulanır; shipped ikonlar 48×48 değildir); ardışık ikon pitch'i 178 ± 1 ve ikon üst kenarı eski `H-112` ± 1. Yalnız kolon kutularını ortalamak yeterli değildir.
4. Değişiklik yokken view yenileme sayacı artmaz.

**Görsel kontrol (kullanıcı):** Alt ortadaki Q/E/F/R ikonları, tuş etiketleri, READY/ACTIVE/cooldown yazıları ve üstteki geçici stat blokları eski yerlerinde mi? Cooldown'da ikon soluklaşıyor mu? Seviye atlayınca stat bloğu güncelleniyor mu? Respawn sonrası bar geri geliyor mu?

## ADIM 7: Menüler (ana menü, pause, game over) + Button hover düzeltmesi

**Önce engine düzeltmesi (zorunlu)**
- `D:\LightYears\LightYearsEngine\src\widget\Button.cpp` satır 60–79 (`MouseMoved` dalı): basılı tuş yokken **her** fare hareketinde `handled = true` dönüyor. Eski menüler her butonu ayrı ayrı çağırdığı için bu görünmüyordu. Yeni ağaçta ise dispatch ilk `true`'da durur: ilk buton bütün hover event'lerini yutar, diğer butonlar hover rengine geçemez veya hover renginde takılı kalır.
- **Düzeltme:** `MouseMoved` hover/default rengini günceller ama `handled` değerini **değiştirmez** (hover bir tüketim değildir). Press ve release davranışı aynen kalır.
- Test için `Button`'a `bool IsHovered() const` eklenir (hover rengindeyse true). Ayrıca `D:\LightYears\LightYearsEngine\tests\UIFoundationTests.cpp`'ye bir test eklenir: aynı parent'ta iki buton; A'nın üstüne gidip sonra B'nin üstüne gidince A hover değil, B hover. Butonun texture ve font yüklemesi asset kökü gerektiriyorsa bu test `HUDMigrationTests --menus` içine konur.
- **Güvenlik kontrolü:** `World::DispatchEvent` (`D:\LightYears\LightYearsEngine\src\framework\World.cpp` ~550) dönüş değeri oyunu etkiliyor mu? `Application::DispatchEvent` (`Application.cpp:166`) çağıranı incelenir. MouseMoved'ın artık `false` dönmesi oyunda bir yan etki yaratıyorsa Sol'a raporlanır.

**Menü taşıma** (ViewModel yok, sözleşme 8)
- Ortak yardımcı: `D:\LightYears\LightYearsGame\include\widget\MenuLayout.h` / `src\widget\MenuLayout.cpp`. İçerik:
  - `weak_ptr<StackPanel> BuildMenuColumn(Panel& parent, const UILayout& layout, float spacing)`
  - `weak_ptr<Button> AddMenuButton(StackPanel& column, const std::string& text, unsigned int textSize)`
- **Public API değişmez:** `onStartButtonClicked`, `onResumeButtonClicked`, `SetTitleText`, `SetScoreText` vb. `GameLevel.cpp` (satır 213–262) ve `MainMenuLevel.cpp` (satır 19–20) dokunulmadan çalışmalı.
- **`MainMenuHUD`** (`D:\LightYears\LightYearsGame\include\widget\MainMenuHUD.h`, `src\widget\MainMenuHUD.cpp`)
  - Üye widget'lar (`mTitleText`, `mStartButton`, `mQuitButton`), `Draw` override'ı ve `HandleEvent` override'ı silinir.
  - İçerik **constructor'da** `UILayer::Menu` katmanına kurulur. Katmanlar HUD constructor'ında hazırdır ve konumu layout çözer, layout bağımsız olarak çözülebilir; ancak weak callback bağlamak için `Init` override'ı korunur:
    - başlık: 40 punto, anchor (0.5, 0), pivot (0.5, 0.5), offset (0, 100)
    - buton kolonu: anchor (0.5, 0.5), pivot (0.5, 0), cross Center. İlk buton merkezi H/2'de kalacak şekilde kolon üst offset'i gerçek ilk buton intrinsic yüksekliğinin yarısı kadar yukarı alınır; spacing gerçek yükseklikten hesaplanarak eski 100 px pitch korunur
    - butonlar: Start 25 punto, Quit 20 punto
  - Buton `onButtonClicked` → mevcut `StartButtonClicked`/`QuitButtonClicked` bağlantıları constructor'da değil `Init` içinde `GetWeakPtr()` ile kurulur. Object::GetWeakPtr weak_from_this döndürür; constructor'da weak sahiplik henüz yoktur. Widget içerik constructor'da kalır, Init yalnız güvenli callback bağlama için korunur. Aynı kural Pause/GameOver için geçerlidir.
- **`PauseMenuHUD`** (`...\PauseMenuHUD.h/.cpp`)
  - İçerik constructor'da, `UILayer::Modal` katmanına kurulur.
  - Overlay: `Panel`, layout `UILayout::Stretch({50,50},{50,50})`, arka plan (60,60,60,150).
  - Başlık: 40 punto, merkezi y=100.
  - Kolon: Resume 25, Restart 25, Quit 20, Main Menu 20. İlk buton merkezi y=200'de kalır; kolon üst offset'i gerçek buton yüksekliğinin yarısına göre çözülür. 75 px pitch gerçek intrinsic yükseklikten spacing hesaplanarak korunur.
  - Eski sıra korunur: Resume, Restart, Quit, Main Menu.
  - `HandleEvent` override'ı **yalnız Escape** için kalır: önce `HUD::HandleEvent` sonucu alınır; Escape ise `onResumeButtonClicked.Broadcast()` ve true, diğer event'lerde base sonucu döndürülür. Modal katmanda görünür içerik varken HUD base tüm event'leri capture ettiği için Escape kontrolü base true sonucundan önce erken return ile atlanmamalıdır.
  - `mOverlay` (`sf::RectangleShape`) ve `Draw` override'ı silinir.
- **`GameOverHUD`** (`...\GameOverHUD.h/.cpp`)
  - Katman `UILayer::Modal`, aynı overlay.
  - Başlık: 40 punto, merkezi y=150. Skor: 30 punto, merkezi y=200.
  - Kolon: Restart 25, Main Menu 20, Quit 20 (eski sıra). İlk buton merkezi y=300'de kalır; kolon üst offset'i gerçek buton yüksekliğinin yarısına göre çözülür. 75 px pitch korunur.
  - İçerik constructor'da kurulur (MainMenu gibi). Bu sayede `GameLevel::GameFinished`'in spawn'dan hemen sonra, `Init`'ten önce çağırdığı `SetTitleText`/`SetScoreText` doğrudan ilgili `TextWidget`'ı (weak_ptr) güncelleyebilir.

**`--menus` testleri** (`HUDMigrationTests.cpp`; butonlar asset yüklediği için headless asset kökü gerekir)
1. Her menü HUD'ı oluşturulur. Pencere gerekiyorsa `EnemyCombatFoundationTests.cpp` gibi `NativeInit(application.GetRenderWindow())` kullanılır; ardından `SetViewportSize({1920,1080})` ve Tick. Butonların merkezleri, viewport'a göre hesaplanan eski konumların ±2 px içinde olmalı (ör. MainMenu Start merkezi (W/2, H/2)).
2. Buton merkezine MouseButtonPressed + MouseButtonReleased gönderilir (`sf::Event{sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {x,y}}}`) → ilgili public delegate bir kez tetiklenir.
3. Hover: A → B hareketinde yalnız B hover.
4. Pause: Escape → `onResumeButtonClicked`.
5. GameOver: `Init`'ten önce `SetTitleText("You Win!")` ve `SetScoreText(42)` → Tick sonrası metinler "You Win!" ve "Score: 42".
- CMake: `add_test(NAME LightYearsHUDMigrationMenus ... --menus ...)`.

**Görsel kontrol (kullanıcı):** Ana menü: başlık ve Start/Quit yerinde mi, hover rengi her butonda doğru mu, tıklama çalışıyor mu? Pause (Esc): gri overlay, 4 buton, Esc ile devam, Restart ve Main Menu çalışıyor mu? Game over: başlık, skor ve 3 buton doğru mu?

## ADIM 8: GameHUD son temizlik, dokümantasyon ve tam regresyon

**GameHUD inceltme** (`D:\LightYears\LightYearsGame\include\widget\GameHUD.h`, `src\widget\GameHUD.cpp`)
- Kalması gerekenler: FPS yazısı, hız yazısı, hasar sayıları (`ConnectDamageObservers`, `ShipDamageResolved`, `UpdateDamageNumberVisuals`, `mDamageNumbers`, `mObservedDamageShips`), `GameHUDDamageE2ETestAccess` friend'i.
- Kullanılmayan include'lar silinir: `Button.h`, `ValueGauge.h`, `ImageWidget.h`, `TimerManager.h`, `GameplayWarning.h`. Her biri için grep ile kullanım kontrol edilir.
- `GetWindowSize()`, `mWindowSize`, `mWindowRef`: artık hiçbir controller kullanmıyorsa (grep `GetWindowSize` ile `LightYearsGame` genelinde kontrol) silinir. Hasar sayıları kullanıyorsa gerekçesiyle kalır.
- `HandleEvent` override'ı yalnız `HUD::HandleEvent` çağırıyorsa silinir.
- FPS ve hız yazıları bu adımda da taşınmaz (`TEMPORARY TEST UI`).

**Dokümantasyon**
- `D:\LightYears\docs\PROJECT_DOCUMENTATION.md`, "UI tabanı ve vitals sunumu" bölümü:
  - Referans uygulamalar listesine warning, notification, encounter, ability bar ve menüler eklenir (her biri bir satır ve yol).
  - "Sonraki işler" paragrafı güncellenir: kalan yalnız geçici test UI'ı (FPS, hız, hasar sayıları) ve gerekirse JSON layout.
  - Menüler için "statik görünüm, ViewModel yok" kuralı (sözleşme 8) ve Button hover düzeltmesi bir cümleyle eklenir.
- Bu plan dosyasının sonuna kısa bir "Uygulama sonucu" bölümü eklenir: adım başına build/test sonuçları.

**Tam regresyon**
- Hedefler: temel set + `LightYearsPlayerLevelTestProgressionE2ETests`
- ctest: `^(LightYearsUIFoundation|LightYearsVitalsHUD|LightYearsHUDMigration.*|LightYearsGameHUDDamageEventE2E|LightYearsGameHUDPlayerRestart|LightYearsPlayerLevelTestProgressionE2E|LightYearsGasLiteCore)$`
- `LightYearsGasLiteCore` monolitik testi `EnemyCombatFoundationTests`'i içerir (encounter bloğu ADIM 4'te çıkarıldı). Bilinen dalgalı test 19 opt-in olduğu için normalde geçmesi beklenir. Kırmızı olursa hatanın bu göçle ilgili olup olmadığı raporlanır, ilgisiz bilinen dalgalanma ise öyle yazılır.
- `git diff --check` temiz.

**Görsel kontrol (kullanıcı), son tur:** Ana menü → oyun → vitals, ability bar, uyarı, dalga bilgisi → pause → game over → restart. Hepsi eskisi gibi çalışıyor mu?

## Uygulama sonucu

### ADIM 1 — 2026-09-30

Luna uygulayıcısı boss barının üç fonksiyonunu, iki üyesini ve yalnız bu API için gereken `Actor` forward declaration'ını kaldırdı. Güncel Game/Engine `src`, `include`, `tests` taramasında dış kullanım bulunmadı; silme sonrası beş sembol için eşleşme kalmadı. Yalnız `GameHUD.h/.cpp` değişti; önceki Vitals ve diğer dirty çalışma korundu. Sol diff'i ve CTest logunu inceledi. Yeni test ve commit yok.

PowerShell'den çalıştırılan build (39/39, exit 0):

```powershell
cmd /c 'call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && set VSLANG=1033 && cmake --build build --target LightYearsGame LightYearsUIFoundationTests LightYearsVitalsHUDTests LightYearsGasLiteTests LightYearsContinuousBeamWallE2ETests --parallel 4'
ctest --test-dir build -R "^(LightYearsUIFoundation|LightYearsVitalsHUD|LightYearsGameHUDDamageEventE2E|LightYearsGameHUDPlayerRestart)$" --output-on-failure --timeout 90
```

CTest exit 0, 4/4 geçti; [CTest logu](../build/Testing/Temporary/LastTest.log). `git diff --check` exit 0. İlk build denemesi shell quoting hatasıyla derlemeye başlamadan başarısız oldu; yukarıdaki düzeltilmiş komut başarılıdır. GasLiteCore monolitik suite çalıştırılmadı.

**ADIM 1 sınırı:** Görsel kontrol yapılmadı. Kullanıcının sonraki otomatik ilerleme talimatı görsel onay kapısını kaldırdı; ADIM 2 ve sonrası seri sürdürülecek. Derleme ve test geçişi görsel kabul değildir.
### ADIM 2 — 2026-09-30

Warning VM/View ve controller göçü, GameHUD warning temizliği ve ortak `LightYearsHUDMigrationTests --warning` tamamlandı. GameLevel broadcast yolu korundu. Altı hedef build exit 0 (891/891); configure exit 0. Aynı vcvars64 + VSLANG=1033 ortamında:

```powershell
cmake -S . -B build
cmake --build build --target LightYearsGame LightYearsUIFoundationTests LightYearsVitalsHUDTests LightYearsHUDMigrationTests LightYearsGasLiteTests LightYearsContinuousBeamWallE2ETests --parallel 4
ctest --test-dir build -R '^(LightYearsUIFoundation|LightYearsVitalsHUD|LightYearsHUDMigration.*|LightYearsGameHUDDamageEventE2E|LightYearsGameHUDPlayerRestart)$' --output-on-failure --timeout 90
```

Son CTest exit 0, 5/5; [warning artefaktı](../build/e2e-artifacts/hud-migration-warning.json) 13/13. [Configure](../build/ui-migration-step2-configure.log), [build](../build/ui-migration-step2-build.log), [CTest](../build/ui-migration-step2-ctest.log). İlk build testte `Application` başlığı için mutable string istedi; yerel string ile düzeltildi. İlk geometry test başarısızlığı viewport/Tick sırası ve piksel yuvarlaması nedeniyleydi; gerçek 1920x1080 gizli pencerede merkez (960,54.5), 41px yüksekliğin doğal 0.5px yuvarlaması. Formül toleransı değişmedi; yalnız geometri 0.5px pay kullanır. Test-only sonraki build exit 0. Sol scoped diff check ve güncel CTest/artefaktı okudu. Ayrı Luna kaynak incelemesi: somut bulgu yok. Görsel kontrol yapılmadı. Kullanıcının otomatik ilerleme talimatıyla ADIM 3 başlatıldı.
### ADIM 3 — 2026-09-30

Notification VM/Presenter/View/controller eklendi; GameLevel canonical controller vektörü sahibi, notification erişimi weak'tir. LevelOne Stage olaylarını presenter'a bağlar; GameHUD notification/timer API ve üyeleri kaldırıldı. Bildirim pending listesi son 8 isteği tutar, View kimlik başına bir kez işler; timer ceil/coalescing, fade ve üst offset8 uygular. Her VM alanı SetIfChanged ile yayımlanır.

ADIM 2'deki configure/build/CTest komutları aynı vcvars64 + VSLANG=1033 ortamıyla çalıştı: configure exit 0, altı hedef build exit 0 (77/77); son test dosyası düzenlemesi sonrası aynı build exit 0 (3/3). CTest exit 0, 6/6. [Bildirim artefaktı](../build/e2e-artifacts/hud-migration-notification.json) 11/11: bounded queue, ceil ve revision, View metni/gizleme/top edge, bildirim expiry, stage rebind/aktif stage önce destroy, controller teardown. [Configure](../build/ui-migration-step3-configure.log), [build](../build/ui-migration-step3-build.log), [CTest](../build/ui-migration-step3-ctest.log). İlk build eksik SFML Vector2 include tanısını verdi; düzeltildi. Sol kaynak/artefakt/CTest logunu inceledi; scoped diff check exit 0. Görsel oyun kabulü yapılmadı; ADIM 4 otomatik başlatıldı.
### ADIM 4 — 2026-09-30

Encounter controller sunum alanlarını SetIfChanged ile typed presentation'a yazar; yeni View const VM, revision ve StackPanel kullanır. Saf `BuildEncounterHUDViewModel` fonksiyonu ve `[TEST Encounter HUD View Model]` bloğu değişmedi. Eski widget-list presentation bloğu ve yalnız onun kullandığı iki yardımcı kaldırılıp ortak `--encounter` testine taşındı. Idle/Spawning/InterWaveDelay/Completed/Failed, top layout, 16 stable Tick, controller subtree cleanup ve expired weak provider kontrol edildi.

ADIM 2'deki altı hedef configure/build/CTest komutları aynı vcvars64 + VSLANG=1033 ortamında: configure/build exit 0, CTest exit 0 7/7. [Encounter artefaktı](../build/e2e-artifacts/hud-migration-encounter.json) 10/10. [Configure](../build/ui-migration-step4-configure.log), [build](../build/ui-migration-step4-build.log), [CTest](../build/ui-migration-step4-ctest.log). Scoped diff check exit 0, Sol kaynak ve sonuçları inceledi. Sınır: weak-provider senaryosu gerçek Arena level teardown fixture'ı yerine weak snapshot sahibinin süresi dolunca boş snapshot döndürmesini doğrular; gerçek level teardown burada yeniden test edilmedi. Görsel kabul yok; ADIM 5 presenter-only otomatik başladı.
### ADIM 5 — 2026-09-30

Dört slotlu typed AbilityBarViewModel ve Presenter eklendi; mevcut AbilityUIController ve GameLevel bağlantısı bu adımda değiştirilmedi. Mevcut stat biçimi/ResolveAbilityAttributes, ikon/etiket/accent ve active/cooldown politikası korunur. Abonelikler guarded SubscriptionSet, gemi erişimi weak, unique ID 0 ayrı bound bayrağıyla geçerli; respawn generation değiştirir.

ADIM 2'deki configure/build/CTest komutları aynı MSVC/VSLANG ortamıyla: configure/build exit 0, CTest exit 0 8/8. Sol güncel [ability artefaktında](../build/e2e-artifacts/hud-migration-ability.json) 15/15 doğruladı (worker'ın son metnindeki 14 sayımı eskiydi). Gerçek shipped dört slot, Frost Maelstrom aktivasyonundan cooldown 14.0→13.8s (140→138 tenths), Lv1→Lv2 stat değişimi, respawn generation1→2 ve eski gemi olayında revision sabitliği ölçüldü. Headless input+zero Tick aktivasyon yapmayınca mevcut canonical TryActivate yolu kullanıldı. [Configure](../build/ui-migration-step5-configure.log), [build](../build/ui-migration-step5-build.log), [CTest](../build/ui-migration-step5-ctest.log). Scoped diff check exit 0. Görsel değişiklik bu adımda yok; ADIM 6 otomatik başladı.

ADIM 6 ön inceleme düzeltmesi: planın 150 px kolon ortalaması, mevcut 48 px sola hizalı ikonları 51 px sola kaydıracaktı. Kabul kriteri gerçek ikonların merkez/pitch/üst kenarı üzerinden düzeltildi; uygulama feature-local layout ile eski yerleşimi koruyacak. Bu oyun/balance değişikliği değildir.
### ADIM 6 — 2026-09-30

AbilityBarView/controller eklendi, GameLevel yeni controller'a bağlandı; eski AbilityUIController h/cpp ve AbilityUIViewModel silindi, tüm source/CMake referansları temizlendi. View const VM/revision ve layout kullanır. Değişken stat yüksekliklerinin ikonları kaydırmaması için 150×200 sabit Panel kolonları ve 28px yatay spacing seçildi. Eski 48px mantıksal hücre merkezleri, 178px pitch ve H−112 ikon üst kenarı korunur; shipped texture boyutları farklıdır. TextWidget glyph offset'i eski ham metin konumlarını korur. Boş VM placeholder yüklemez; texture/generation değişiminde feature-local ikon yeniden oluşturulur. Böylece ImageWidget::SetImage'ın eski texture rect'ini tutması barı etkilemez; engine değiştirilmedi.

Configure exit 0, ADIM 2'deki altı hedef build komutu vcvars64 ardından VSLANG=1033 ile exit 0 (185/185). İlk denemede HUDMigrationTests'teki hatalı NativeTick çağrıları mevcut HUD::Tick'e düzeltildi; ikinci deneme eşzamanlı dış build sırasında GasLiteCoreTests object dosyası erişim hatası verdi; üçüncü deneme geçti. [Configure](../build/ui-migration-step6-configure.log), [ilk deneme](../build/ui-migration-step6-build-attempt1.log), [ikinci deneme](../build/ui-migration-step6-build-attempt2.log), [başarılı build](../build/ui-migration-step6-build.log).

Ortak build klasöründe dış all-target Ninja devam ettiği için root, başarılı build'in beş farklı test exe'sini ignored runtime klasörüne SHA256 before/copy/after eşitliğiyle kopyaladı. Mevcut CTest `--show-only=json-v1` manifestindeki aynı sekiz test, argüman, timeout ve working directory ile ayrı CTestTestfile oluşturuldu. `ctest --test-dir build/ui-migration-step6-runtime --output-on-failure --timeout 90` exit 0, 8/8. [Manifest](../build/ui-migration-step6-test-manifest.json), [hashler](../build/ui-migration-step6-runtime-hashes.json), [izole CTest](../build/ui-migration-step6-isolated-ctest.log). Normal build klasörü CTest'i final regresyonda tekrar alınacak. [Ability artefaktı](../build/e2e-artifacts/hud-migration-ability.json) 25/25 (15 presenter +10 view/controller): iki/dört/tek slot geometry, unequal stats, alpha, stable revision, boş→ship, 9×54→34×33 ikon değişimi/old child removal, controller teardown. Scoped diff check temiz, Luna read-only kaynak incelemesinde somut bulgu yok. Görsel kabul yapılmadı; ADIM 7 otomatik başladı.

ADIM 6 ek doğrulama: dış Ninja sona erince root normal build klasöründe ADIM 2'deki CTest regex'ini tekrar çalıştırdı: exit 0, 8/8 (39.36s). [Normal CTest](../build/ui-migration-step6-ctest.log). Böylece snapshot sonucuna ek ordinary CTest kanıtı da alındı.

### ADIM 7 — 2026-09-30

Üç statik menü constructor'da Menu/Modal katmanlarına taşındı; ViewModel eklenmedi. Weak callback'ler shared sahiplik oluştuktan sonra Init içinde bağlanır. MenuLayout ortak kolon/buton kompozisyonudur; gerçek 222×39 texture boyutundan spacing ve kolon üst offset'i hesaplanarak eski 100/75px button merkez pitch'i korunur. Başlıkların eski glyph bearing ofseti layout'a eklenir; GameOver SetTitleText/SetScoreText güncel metin glyph ofsetini yeniden uygular. Modal overlay inset 50, rengi 60/60/60/150 ve public delegates korunur. Pause Escape, HUD base'in modal capture=true sonucuyla erken atlanmaz. Button MouseMoved hover durumunu günceller ama tüketmez; press/release davranışı korunur, IsHovered test gözlemidir.

ADIM 2'deki altı hedef configure/build komutları vcvars64 ardından VSLANG=1033 ile exit 0, build 258/258; ordinary CTest aynı regex ile exit 0, 9/9 (11.38s). Ninja başlangıçta `premature end of file; recovering` uyarısı verdi, ardından build ve linkler başarılı tamamlandı. [Configure](../build/ui-migration-step7-configure.log), [build](../build/ui-migration-step7-build.log), [CTest](../build/ui-migration-step7-ctest.log). [Menü artefaktı](../build/e2e-artifacts/hud-migration-menus.json) 18/18: 1920×1080 buton merkezleri/sıra/pitch, legacy title centers, MainMenu 1280×720 layout, hover A→B, dokuz gerçek press/release public action, modal overlay/capture, Pause Escape, Init öncesi gerçek You Win! / Score: 42 metni. Kaynakta manual window/location/CenterOrigin hesabı kalmadı; public level callsites değişmedi. Root log/artefaktı okudu, Luna read-only kaynak incelemesi temiz. Oyun içi görsel kabul yapılmadı; ADIM 8 son temizlik ve tam regresyona geçilir.
### ADIM 8 — final doğrulama, 2026-10-01

GameHUD yalnız FPS, hız ve hasar sayılarını tutacak şekilde inceltildi; kullanılmayan widget/Timer/Delegate include'ları, pencere boyutu getter/member/local değişkeni ve boş HandleEvent override'ı kaldırıldı. Hasar sayılarının mapCoordsToPixel dönüşümü için mWindowRef korundu. TEMPORARY TEST UI ve damage observer unique-ID/weak-ship davranışı taşınmadı. Kanonik UI bölümü beş göç referansını, statik menü sözleşmesi8'i, constructor/Init weak callback ömrünü ve non-consuming Button hover davranışını gösterir. AbilityBarPresenter'ın mevcut generic GameAbility okumaları doğru anlatılır; eski snapshot tüketicisi iddiası kaldırıldı.

Yedi hedef build exit0 (216/216); önceki deneme kapsam dışı EnergySpearTraversalActor object derlemesinde tanı metni vermeden durdu, o kaynak değiştirilmedi. İlk log [attempt1](../build/ui-migration-step8-build-attempt1.log), başarılı [build](../build/ui-migration-step8-build.log). Son unused Delegate include temizliği başarılı build'den önce yapıldı. Luna kullanım limiti son rapor/audit turunu kesti; root güncel Markdown kaynaklarını, derleme/test zamanlarını ve UI diff/wiring/linklerini doğrudan doğruladı. Kesinti sonrası aynı yedi hedef build'i root tarafından tekrar exit0 tamamlandı (yalnız runtime asset sync, yeniden C++ derleme işi yok): [resume build](../build/ui-migration-final-resume-build.log).

PowerShell'de kullanılan tam komutlar:

```powershell
cmd /c 'call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && set VSLANG=1033 && cmake --build build --target LightYearsGame LightYearsUIFoundationTests LightYearsVitalsHUDTests LightYearsHUDMigrationTests LightYearsGasLiteTests LightYearsContinuousBeamWallE2ETests LightYearsPlayerLevelTestProgressionE2ETests --parallel 4'
ctest --test-dir build -R '^(LightYearsUIFoundation|LightYearsVitalsHUD|LightYearsHUDMigration.*|LightYearsGameHUDDamageEventE2E|LightYearsGameHUDPlayerRestart|LightYearsPlayerLevelTestProgressionE2E|LightYearsGasLiteCore)$' --output-on-failure --timeout 90
```

Final ordinary CTest exit0,11/11 (28.48s), GasLiteCore ve player progression dahil; [CTest](../build/ui-migration-step8-ctest.log). HUD artefaktları warning13/13, notification11/11, encounter10/10, ability25/25, menus18/18 (toplam77). Scoped git diff --check exit0; UI doküman linklerinin tüm hedefleri mevcut. Eski ability UI kaynak/CMake referansları kalmadı; GameplayWarningHUDController'ın geçerli Show/Hide API'leri korunur. İlgisiz ve eşzamanlı combat çalışma korundu; commit yok.

**Göçün sekiz uygulama adımı tamamlandı.** Oyun içi görsel kabul yapılmadı. Son kullanıcı kontrolü: ana menü → oyun HUD/vitals/ability/uyarı/dalga/bildirim → pause → game over → restart. Encounter weak-provider testi gerçek Arena level teardown fixture'ı değildir; compact MainMenu testi layout çözümünü kapsar, gerçek pencere resize testi değildir. Bunlar otomatik doğrulamanın sınırlarıdır.