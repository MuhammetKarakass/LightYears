# Uygulama planı bağımsız kontrolü — 2026-09-26

## Sonuç

Planın büyük bölümü kaynak koduna uygulanmış; fakat **tam, doğru ve güncel olarak doğrulanmış kabul edilemez**. İki mevcut E2E tekrar tekrar çöküyor. C2 kararı ile uygulama/test beklentisi arasında çelişki var. C5 callback sınırları ve L5d world izolasyonu için açık riskler kalmış. D2 timer ölçümü yüksek timer yükünü temsil etmiyor.

Kontrol, [2026-09-23 planındaki](ENGINE_GAME_AUDIT_IMPLEMENTATION_PLAN_2026-09-23.md) T0, C1–C5, L1–L5d, D1 ve D2 alt maddelerini kapsar. Null Pulse/wave/encounter/basit AI ürün davranışı yeniden denetlenmedi veya değiştirilmedi. Üretim/test/config kodu değiştirilmedi; yeni unit test yazılmadı veya çalıştırılmadı. Mevcut E2E'ler çalıştırıldı. Hafıza ve önceki tamamlanma kayıtları tarihsel iddia olarak ele alındı, kanıt yerine kullanılmadı.

## Yeniden açılması gereken noktalar

### 1. P1 — Güncel E2E doğrulaması kırmızı

`LightYearsContinuousBeamWallE2E` ve `LightYearsTimerManagerSceneE2E` ilk beşli koşuda, ayrı tekrar koşusunda ve son loglu beşli koşuda SegFault verdi. Böylece her biri üç kez başarısız oldu. Registration, loader ve damage→HUD senaryoları geçti.

Son kanıt: [CTest audit logu](../build/e2e-artifacts/implementation-audit-2026-09-26.log). Çöküşün kaynak kodu, eski binary veya ortam kaynaklı kök nedeni bu denetimde kanıtlanmadı. Bu sonuçtan “beam algoritması kesin yanlış” çıkarımı yapılmaz; **D1 ve D2.5'in bugün geçtiği söylenemez**.

Çöken iki senaryonun JSON dosyaları yenilenmedi: `continuous-beam-wall.json` 24 Eylül 02:44, `timer-manager-scene.json` 24 Eylül 18:49 tarihli kaldı. İçlerindeki `passed:true` bugünkü başarısız koşuların sonucu değildir. Artefaktlar çalışmaya özgü kimlik/tarih veya dış runner sonucuyla eşleştirilmeden okunursa yanlış yeşil rapor çıkarılabilir.

### 2. P1 — Header bağımlılık kanıtı hâlâ eksik; incremental build yeterli değil

VS geliştirici ortamında oyun ve E2E build komutu exit 0 verdi, ancak yalnız asset senkronizasyonu çalıştı; C++ nesneleri yeniden derlenmedi. Ninja `-t deps` şu dört nesnenin her birini `#deps 0 (VALID)` gösterdi:

- `LightYearsGameplayCore.dir/src/gameplay/ability/content/GameAbilityContentRegistration.cpp.obj`
- `LightYearsContinuousBeamWallE2ETests.dir/tests/ContinuousBeamWallE2E.cpp.obj`
- `LightYearsContinuousBeamWallE2ETests.dir/tests/TimerManagerSceneE2E.cpp.obj`
- `LightYearsContinuousBeamWallE2ETests.dir/src/spaceShip/SpaceShip.cpp.obj`

Bu kaynakların çok sayıda header include'u var. [CMake'teki prefix ayarı](../CMakeLists.txt) tek başına mevcut dep kayıtlarının onarıldığını göstermiyor. Eski “stale obj sorunu çözüldü” kaydını genellemek güvenli değil. Bu iki çöküşün mutlaka stale object yüzünden olduğunu da kanıtlamıyor.

Kapatma koşulu: doğru compiler dili/include-prefix ile ayrı temiz build veya kontrollü yeniden derleme; sonrasında nonzero header bağımlılıklarını ve header değişimine doğru rebuild tepkisini doğrulamak; aynı E2E'leri tekrar çalıştırmak. Bu tur build nesneleri silinmedi.

### 3. P1 — C2: max artışında mutlak excess korunmuyor

[HealthComponent::SetMaxHealth](../LightYearsGame/src/gameplay/HealthComponent.cpp) ve [ShieldComponent::SetMaxShield](../LightYearsGame/src/gameplay/ShieldComponent.cpp) normal heal'in overcap'i silmesini düzeltmiş. Fakat `preservePercent=false` ile maksimum büyüdüğünde eski normal pay + eski excess toplamı korunuyor; yeni maksimum üstündeki gerçek excess küçülüyor, ardından `Reconcile` ledger'ı küçültüyor.

Somut kaynak örneği: 100 normal + 40 geçici = 140 can; maksimumu 120 yapınca toplam 140 kalır ve excess 20'ye düşer. Maksimum 150 olunca excess sıfırlanır. [TemporaryOvercapTests](../LightYearsGame/tests/TemporaryOvercapTests.cpp) bunu açıkça bekliyor; gerçek Temporal Recall/Cryostasis aktivasyonu yerine component çağrılarıyla bu davranışı test ediyor. SpaceShip'in max artışından sonraki normal `Regenerate` çağrısı da can zaten max üstündeyse eksilen excess'i geri getirmiyor.

Planın “excess mutlak miktar olarak korunur” ifadesi ve kayıtlı kullanıcı kararıyla bu uyuşmuyor. **İlk plandaki formül de aynı çelişkiyi içeriyordu; bu yalnız uygulayıcı hatası değildir.** Karar “max artışı excess'i normal kapasiteye emer” ise belge öyle düzeltilmeli; gerçekten mutlak excess korunacaksa kaynak/test beklentileri o karara göre hizalanmalı. Şu hâliyle C2 tamamen kapalı sayılmaz.

### 4. P2 — C5: normal ret yolları atomik, callback sınırı tamamlanmamış

[AbilityRuntimeSystem](../SpaceAbilitySystem/include/abilities/AbilityRuntimeSystem.h) ve [AbilityCollection](../SpaceAbilitySystem/include/abilities/AbilityCollection.h) hedefi önceden silme kusurunu önemli ölçüde düzeltmiş: preflight ve collection replacement var; loadout map'i commit sonrasına taşınmış.

Ancak `RebindAbility`/grant replacement, mutation rezervasyonlarını elle alıp `NotifyReplacedAbility` ve değişim callback'lerinden **sonra** elle bırakıyor. Callback exception atarsa release satırları çalışmaz; runtime kilitli kalabilir ve runtime commit gerçekleştiği hâlde game loadout/inventory commit edilmemiş olabilir. Ayrıca callback içinden `Clear()` çağrısı collection'ı koşulsuz boşaltabilir; dış rebind/grant mevcut olmayan handle için başarı döndürmeye devam edebilir. `Clear()` transaction guard'ını kontrol etmiyor.

Bu, kaynakta açık callback sözleşmesi riski; shipped akışta üretildiği iddia edilmiyor. Mevcut transaction testleri ret ve bildirim sırasına bakıyor; exception veya callback→Clear senaryosu yok. Kapatma koşulu: callback exception/tekrar giriş politikasını açıkça tanımlamak, RAII cleanup ve commit tutarlılığıyla gerçek runtime E2E'de doğrulamak.

### 5. P2 — L5d: direct reflection farklı World girişini reddetmiyor

[ProjectileReflectionService::TryReflectProjectile](../LightYearsGame/src/gameplay/projectile/ProjectileReflectionService.cpp) registry'yi defender'ın World'ünden seçiyor; projectile'ın aynı World'de olduğunu kontrol etmiyor. [Registry direct yolu](../LightYearsGame/src/gameplay/projectile/ProjectileReflectionRegistryActor.cpp) da yalnız defender kimliğini kilitleyip receiver'a geçiyor. Swept yolun world filtresi direct yolda yok.

Mevcut [reflection testinin](../LightYearsGame/tests/ProjectileReflectionTests.cpp) iki World senaryosu yalnız doğru projectile–defender çiftlerini çağırıyor; çapraz çiftin reddini test etmiyor. Bu yüzden “direct world izolasyonu doğrulandı” ifadesi fazla geniş. Normal aynı-world collision akışında görülmüş hata değil; public service sınırının eksik invariant'ı. Kapatma koşulu: çapraz-world ve pending-destroy girişlerin reddi, aynı-world normal/reflection davranışının korunması.

### 6. P2 — L5d registry bulma sıcak yolda O(N) tarama ekliyor

[FindRegistryActor](../LightYearsGame/src/gameplay/projectile/ProjectileReflectionService.cpp), her sorguda [GetActorsByTypeIncludingPending](../LightYearsEngine/include/framework/World.h) çağırıyor. Bu helper tüm aktif/pending actor listesini RTTI ile tarayıp sonuç vektörü oluşturuyor. [Gravity Anomaly projectile](../LightYearsGame/src/gameplay/ability/gravityAnomaly/GravityAnomalyProjectileActor.cpp) hareket adımında swept reflection yolunu çağırıyor; ilgili P mermi ve N world actor için registry araması O(P×N) olabilir. Registry bulunmayan sahne de tam taranır.

World-owned taşıma yapılmış; fakat lookup maliyeti önceki küçük receiver tablosuna erişimden büyümüş. Ölçülmüş frame düşüşü değil, kaynakla doğrulanan ölçeklenme riski. World'e bağlı, ömrü güvenli doğrudan service erişimi veya eşdeğer cache düşünülmeli; process-global raw World haritasına dönülmemeli.

### 7. P2 — D2.5 eski ölçümü yoğun timer sahnesini kapsamıyor

24 Eylül tarihli [timer artefaktında](../build/e2e-artifacts/timer-manager-scene.json) global timer maksimumu **0**, game timer maksimumu **1**, game ortalaması yaklaşık **0.202**. 600 frame ölçülmüş olması yüksek timer sayısında ölçeklenmeyi doğrulamaz. Scheduler'ı değiştirmemek bu düşük yükte makul; “çok timer'lı sahne riski kapandı” sonucunu desteklemez. Üstelik bugünkü senaryo çalışması çöktü.

Kapatma koşulu: kapsam dışı AI/wave sistemlerine girmeden gerçek ability/weapon runtime girdileriyle temsili aktif timer yükü, timer sayısıyla birlikte ölçüm ve başarıyla yazılmış güncel artefakt. Ölçüm sonucu yine düşükse scheduler değişikliği gerekli olmayabilir.

## Bütün paketlerin durumu

“Kaynakta uygulanmış” taze E2E ile ispatlanmış demek değildir. 24 Eylül E2E tercihi nedeniyle eski unit testleri bu tur tekrar çalıştırılmadı; yeni test eklenmedi.

| Paket | Güncel kaynak/kanıt değerlendirmesi |
|---|---|
| T0a | PrimaryWeapon asset root'u source directory ile ayarlanmış. Eski test yeniden çalıştırılmadı; bugünkü genel binary/header güvenilirliği açık. |
| T0b | 9B case ayrı çağrılabilir olmuş; stack combatant yerine World-spawned shared owner kullanılmış. Güncel GasLite geçişi iddia edilmiyor. |
| C1 | `MatchesTagExact` tam `operator==` kullanıyor. Kaynak değişikliği planla uyumlu; yeni exact/schema test kaynakları mevcut. |
| C2 | Heal düzeltmesi var; max artışı–excess karar çelişkisi açık (bulgu 3). |
| C3 | Dış→dış segment için çember giriş kökü ve [0,1] kontrolü var; projectile radius kullanılıyor. Dar test kaynağı mevcut; taze E2E yok. |
| C4 | PlayerManager before-destroy/created olayları, HUD delegate handle cleanup ve unique ID rebind var. Dar test manager reset yapıyor; gerçek Arena restart E2E'si değil. |
| C5 | Collection replacement ve loadout commit sırası düzeltilmiş; callback exception/Clear riski açık (bulgu 4). |
| L1 | Physics ve World actor-ID bazlı per-query dedup var; early-exit korunmuş. Kaynakta uygulanmış. |
| L2 | Inertial Wake/Lance Drive `ActorHitCooldowns` ID+expiry kullanıyor, aktif tick'te prune var. Kaynakta uygulanmış. |
| L3 | `deque<Player>`, emplace ve Player'ın ship-destruction aboneliğini destructor/rebind sırasında kaldırması var. Kaynakta uygulanmış. |
| L4 | Chain Lightning weak World, cast timer owner/handles, ability/owner cleanup kullanıyor. Kaynakta uygulanmış; taze lifetime E2E yok. |
| L5a | Closed Circuit World-owned registry, generation-gated move-only token ve replacement var. Kaynakta uygulanmış. |
| L5b | Nano Plague World-owned registry, weak owner ve unmanaged fallback reddi var. Kaynakta uygulanmış. |
| L5c | Portal state World-owned runtime actor'a taşınmış; reentry key unique ID. Kaynakta uygulanmış. |
| L5d | World-owned kayıt ve token var; direct world guard ve sıcak-yol lookup riski açık (bulgular 5–6). |
| D1 | Static wall sweep, görsel/hasar clipping ve eşit temas kuralı kaynakta var; güncel E2E üç kez çöktü. |
| D2.1 | E2E senaryoları/artefakt hattı eklenmiş. Legacy monolitin tümü bölünmemiş; son plan bunu kapsam dışına çıkarmış. Bu, ilk planla kapsam farkıdır. |
| D2.2 | Energy Spear aile kayıt pilotu ve CMake bağlantısı var; public bootstrap/factory/profile E2E tekrar geçti. Tüm aileler taşınmış değil, zaten pilot kapsamıydı. |
| D2.3 | JSON parser ayrı kaynak modülüne taşınmış; public-load E2E tekrar geçti. |
| D2.4 | Contract header'larında eski `GameAbilityBehavior.*` literal'leri bulunmadı; Relay Prism canonical family tag kullanıyor. Umbrella ve domain-root tekrarları bilinçli bırakılmış. |
| D2.5 | Ölçüm kodu var; bugünkü E2E kırmızı, eski yük max 1 timer. Ölçeklenme iddiası sınırlı. |
| D2.6 | SpaceShip geçici hasar delegate'i kaldırılmış, HUD canonical combat event'ini dinliyor. Gerçek Arena death/respawn E2E tekrar geçti. Portal-transit ile headless movement bypass'ı testin açık sınırı. |

## Bu tur çalıştırılan doğrulama

VS 2022 x64 DevCmd altında:

```text
cmake --build build --target LightYearsGame LightYearsContinuousBeamWallE2ETests --parallel 4
```

Exit 0; yalnız runtime asset sync, fresh C++ compile değil.

```text
ctest --test-dir build -R 'E2E$' --output-on-failure --timeout 40
ctest --test-dir build -R '^(LightYearsContinuousBeamWallE2E|LightYearsTimerManagerSceneE2E)$' --output-on-failure --timeout 40 --repeat until-fail:3
ctest --test-dir build -R '^(LightYearsAbilityContentRegistrationE2E|LightYearsAbilityLoaderPublicLoadE2E|LightYearsGameHUDDamageEventE2E)$' --output-on-failure --timeout 40
ctest --test-dir build -R 'E2E$' --output-on-failure --timeout 40 --output-log build/e2e-artifacts/implementation-audit-2026-09-26.log
```

Sırasıyla 3/5, 0/2, 3/3, 3/5 geçti. `until-fail:3` ilk hatada durdu; o komutta üçer koşu yapılmış gibi sayılmadı. Final toplam senaryo durumu 3 yeşil/2 kırmızı; tüm 27 CTest çalıştırılmadı. `ctest -N` ayrıca AutoTargeting, MovementInfluence ve MovementPolicy binary'lerinin bulunmadığını gösterdi; bu, bu paketlerde kaynak hatası kanıtı değildir.

Başarılı üç artefaktın tekrar öncesi/sonrası SHA-256 değerleri eşleşti:

- Registration: `2C44BF1F1E89C8B22882115C662275F8C6A2355E8D8D500DA4A5B1E36943B1DC`
- Loader: `0FC808F7081F6938A97A310F07E24E5D7455C2CF378B39C8DEC520D21433DCFF`
- HUD: `CD11D6D6B18AF5D229EF85023BA943237AA01DAF01C71DD697E2FD5509587C90`

## Kabul önerisi

“Tüm plan tamamlandı” yerine **“ana değişiklikler uygulanmış; doğrulama/karar uyumu açık”** durumuna alınmalı. Önce build/header bağımlılıkları ve iki crash, sonra C2 kontratı; ardından C5 ve L5d sınırları kapatılmalı. Mevcut kodda uyumlu görünen diğer paketleri yeniden yazmak gerekmiyor. D2 borçlarını ölçüm/pilot sınırıyla raporlamak yeterli; blanket mimari refactor yetkisi çıkarılmamalı.
