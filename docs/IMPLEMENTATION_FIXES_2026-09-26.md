# Yedi kaynak bulgusunun düzeltmesi — 2026-09-26

**Sonraki kaynak denetimi:** Koşulsuz tamamlanma hükmü geri çekildi. Tick/Clear yaşam süresi, component teardown bütünlüğü, rotasyon sonrası spatial refresh ve koşullu Nano Tick referansı açıkları için [yeniden inceleme](IMPLEMENTATION_FIXES_REVIEW_2026-09-26.md). Aşağıdaki build/E2E sonuçları kendi senaryolarının tarihsel kanıtıdır.

Kullanıcı doğrudan Codex uygulamasını istedi. AGENTS.md içindeki zorunlu Flash iş akışı kaldırıldı; bu düzeltmede worker kullanılmadı. İlgisiz kirli değişiklikler korunmuştur. Bu belge [kaynak incelemesindeki](IMPLEMENTATION_SOURCE_REVIEW_2026-09-26.md) yedi bulgunun devamıdır; eski inceleme, düzeltme öncesi durumdur.

## Uygulanan kontratlar

1. **Nano Plague yayılma:** `ResolvePendingSpread` enfeksiyon snapshot'ını değer olarak alır; `ApplyOrRefreshInfection` görsel başlangıç konumunu vector mutasyonundan önce kopyalar. Push/reallocation sonrasında eski vector elemanına pointer okunmaz.
2. **Nano Plague aboneliği:** Her enfeksiyon damage delegate handle'ını saklar. Kaldırma önce kaydı vector'den çıkarır, sonra aboneliği ve tag'i temizler. Destroy ve destructor aynı temizleme yolunu kullanır; teardown sırasında yeni enfeksiyon reddedilir. Refresh ikinci abonelik oluşturmaz.
3. **Geçici excess:** Eski excess pozitifse yeni toplam `newMax + oldExcess`; ledger'ın miktar/süreleri korunur. Excess yoksa eski normal kapasite ve yüzde davranışı değişmez. Planın eski çelişkili formülü düzeltildi.
4. **Beam sınırı:** Static sweep hit varlığını ayrı bool ile tutar. `t=1` gerçek bir temastır; menzil ve hedef filtresi bunu görür. İlk runtime koşusu ek bir neden gösterdi: fizik gövdesi olmayan kutu engeller manual spatial index'e yalnız merkezle giriyordu. World artık local offset/rotation dahil kutuların AABB birleşimini indekse alır; duvarın temas kenarı broadphase'de kaybolmaz. Movement'ın mevcut `fraction<1` güvenli mesafe davranışı korunur.
5. **Loadout callback sınırı:** Grant/rebind/remove rezervasyonları RAII ile unwind edilir. Callback'in `Clear` isteği dış mutation bitimine ertelenir; Clear yeniden giriş ve callback exception durumunda koleksiyonu temizlemeyi tamamlar. Grant/rebind dönüşte handle/slot geçerliliğini tekrar kontrol eder. Loadout yöneticisi başarısız/exception çıkışlarında canonical runtime'dan dört slotunu eşitler; kendi callback'inden yeniden equip/unequip reddedilir. Exception'lar yutulmaz: **basic exception guarantee**, post-commit rollback garantisi değil. Bellek tahsisinin başarısızlığı için bütün collection container'larını kapsayan güçlü rollback bu paketin iddiası değildir.
6. **Reflection world sınırı:** Direct servis ve registry projectile/defender/world eşitliğini ve pending-destroy durumunu doğrular; swept giriş de registry'nin kendi World'ünü doğrular.
7. **Reflection lookup:** World'de explicit typed service-actor weak slot'u kullanılır. Registry oluşturulunca pending aşamasında kaydolur; yokluk sorgusu bile actor listesi taramaz. Expired/pending servis temizlenir, yenisi açıkça yerini alır. Process-global raw World map yoktur. Karmaşıklık ortalama O(1) registry erişimidir; receiver taraması ayrı ve değişmemiştir. FPS kazancı ölçülmüş sayılmaz.

## Kaynak sahipleri

- [Nano Plague](../LightYearsGame/src/gameplay/ability/nanoPlague/NanoPlagueControllerActor.cpp)
- [Health](../LightYearsGame/src/gameplay/HealthComponent.cpp), [Shield](../LightYearsGame/src/gameplay/ShieldComponent.cpp)
- [Runtime mutation](../SpaceAbilitySystem/include/abilities/AbilityRuntimeSystem.h), [Loadout manager](../LightYearsGame/src/gameplay/ability/loadout/AbilityLoadoutManager.cpp)
- [World service slot](../LightYearsEngine/include/framework/World.h), [reflection service](../LightYearsGame/src/gameplay/projectile/ProjectileReflectionService.cpp), [registry](../LightYearsGame/src/gameplay/projectile/ProjectileReflectionRegistryActor.cpp)
- [Static sweep](../LightYearsGame/src/gameplay/movement/MovementCollisionService.cpp)

## Doğrulama

Üretim kodu üzerinden kontrat/çağrı incelemesine ek olarak [AuditFixesE2E](../LightYearsGame/tests/AuditFixesE2E.cpp) eklendi. Gerçek SpaceShip attribute→resource akışı, gerçek lethal damage→Nano spread, expire/reinfect/teardown abonelikleri, shipped ability loadout observer exception/Clear, Return Protocol→Rocket ownership ve exact endpoint static sweep kullanır. Fault injection yalnız callback hata yollarını kontrollü tetiklemek içindir. Mevcut continuous-beam E2E'ye de tam menzil ucunda duvarlı/duvarsız karşılaştırma eklendi: duvarlı hedef hasar almıyor, aynı hedef duvarsızken hasar alıyor. Yeni unit test eklenmedi/çalıştırılmadı; mevcut TemporaryOvercapTests'in eski emilme beklentileri yeni ürün kararıyla hizalandı.

VS 2022 x64 ortamında `VSLANG=1033` ile:

```text
cmake --build build --clean-first --target LightYearsGame LightYearsGasLiteTests LightYearsContinuousBeamWallE2ETests --parallel 8
cmake --build build --target LightYearsGame LightYearsGasLiteTests LightYearsContinuousBeamWallE2ETests --parallel 4
ctest --test-dir build -R 'E2E$' --output-on-failure --timeout 60 --output-log build/e2e-artifacts/audit-fixes-run.log
ctest --test-dir build -R '^(LightYearsAuditFixesE2E|LightYearsContinuousBeamWallE2E)$' --repeat until-fail:2 --output-on-failure --timeout 60 --output-log build/e2e-artifacts/audit-fixes-repeat.log
```

Temiz derleme 1101 adımı ve üç hedefi tamamladı; son kaynak düzeltmelerinden sonra incremental build de exit 0. Yeni E2E nesnesinin Ninja kaydı `#deps 317 (VALID)`: bu koşuda header bağımlılıkları izleniyor. GasLite yalnız derlendi, kullanıcının E2E tercihiyle çalıştırılmadı. Altı E2E 6/6; iki hedefli senaryo ikişer tekrar geçişi 4/4. Sınırlı `git diff --check` exit 0; üretim kodu/sahiplik/çağıran incelemesi yapıldı. Sanitizer veya manuel oynanış çalıştırılmadı. Reflection FPS kazancı ölçülmedi.

Son artefaktlar:

- [audit-fixes.json](../build/e2e-artifacts/audit-fixes.json): 22 assertion true; SHA-256 `A2743D2B06840F3CEF0BE4A3484DA23A209821F19B4754E27A0381150700ACFE`.
- [continuous-beam-wall.json](../build/e2e-artifacts/continuous-beam-wall.json): endpoint kontrolü dahil 11 assertion true; SHA-256 `4E9F2572CB7DCBCF6041220576B4AE89AEC4AB04A0924BDF0F8E36ADA6EA1FBB`.
- [tam koşu](../build/e2e-artifacts/audit-fixes-run.log), [tekrar koşusu](../build/e2e-artifacts/audit-fixes-repeat.log).

Bu yedi bulgu kaynakta giderildi ve belirtilen runtime yollarında doğrulandı. Projenin tamamının hatasızlığı veya önceki timer yük ölçümünün yeterliliği iddia edilmiyor. Commit yapılmadı.
