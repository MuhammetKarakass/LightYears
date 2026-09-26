# Uygulama planı — üretim kodu incelemesi, 2026-09-26

> Bu metin düzeltme öncesi kaynak bulgularıdır. Sonraki uygulamanın kontrat ve doğrulama durumu: [yedi bulgunun düzeltmesi](IMPLEMENTATION_FIXES_2026-09-26.md). Aşağıdaki geçmiş bulgular güncel kodda hâlâ açıkmış gibi okunmamalıdır.

## Sonuç ve kapsam

Ana değişiklikler uygulanmış; tamamının doğru olduğu söylenemez. Bu değerlendirme test sonucuna değil, güncel üretim kodundaki veri akışına, çağıranlara ve yaşam döngüsüne dayanır. Bu tur build/test çalıştırılmadı, üretim kodu değiştirilmedi. Null Pulse, wave/encounter ve basit AI hariç tutuldu. İnceleme plan paketlerinin üretim sahiplerini ve ilgili entegrasyon yollarını kapsar; bütün projenin her dalının hatasızlığı iddiası değildir.

## Açık bulgular

### 1. P1 — Nano Plague spread sırasında geçersiz bellek okuması

`Tick` içindeki `Infection&`, `mInfections[index]` elemanıdır. `ResolvePendingSpread(const Infection&)` bu elemanın `deathLocation` adresini `ApplyOrRefreshInfection` metoduna verir. Metot yeni hedef için aynı listeye `push_back` yapar, ardından `*visualOrigin` okur. `List` bir `std::vector` alias'ıdır. Kapasite büyüdüğünde kaynak eleman taşınır: adres geçersizleşir ve hemen ardından dereference edilir. Birden fazla spread hedefinde sonraki iterasyon da eski `infection` referansını kullanır.

Bu, kapasite büyümesine bağlı tanımsız davranıştır; yanlış görsel konumu veya çökme üretebilir. Registry taşımasının yapıldığını inkâr etmez; controller içinde kalan ayrı bir kusurdur. Çözüm sınırı: yayılma konumunu/değerlerini liste mutasyonundan önce bağımsız değer olarak almak; liste elemanı referansını mutasyon boyunca taşımamak.

Kaynak: [controller](../LightYearsGame/src/gameplay/ability/nanoPlague/NanoPlagueControllerActor.cpp), satır 132–141, 165–169, 292–324; [List alias](../LightYearsEngine/include/framework/Core.h), satır 286.

### 2. P1 — C2 mutlak geçici excess kararı uygulanmamış

`SetMaxHealth`/`SetMaxShield`, varsayılan yüzde korumasız yolda eski normal payı ve eski excess'i toplar. 100 max üzerinde 140 can varken max 120 olunca toplam 140 kalır, gerçek excess 40'tan 20'ye düşer. `Reconcile` ledger'ı da küçültür. Max 150 olunca eski excess bütünüyle normal kapasiteye emilir. SpaceShip'in sonraki normal regen çağrısı, zaten max üstündeyken kaybolan excess'i geri koymaz.

Normal heal'in overcap'i silmesi düzeltilmiştir; max artışı ayrı kusurdur. Planın mutlak excess koruma kararı ile verdiği formül de çelişiyordu: sorumluluk yalnız uygulayıcıya ait değildir. Önce bu kararın tek anlamı sabitlenmeli, sonra hesap ve ledger aynı semantiğe getirilmelidir.

Kaynak: [HealthComponent](../LightYearsGame/src/gameplay/HealthComponent.cpp), satır 24–42; [ShieldComponent](../LightYearsGame/src/gameplay/ShieldComponent.cpp), satır 14–36.

### 3. P2 — C5 transaction callback sınırını kapsamıyor

Rebind/grant, collection commit'inden sonra cancel/changed/granted callback'lerini çağırır; mutation rezervasyonları ancak bunlardan sonra elle bırakılır. Callback exception'ında rezervasyonlar kalabilir; runtime değişmişken game loadout map commit'ine ulaşılamaz. Callback içinden `Clear()` ise rezervasyonları dikkate almadan collection'ı boşaltabilir; dış işlem artık var olmayan handle için başarı bildirir.

Normal validation/ret yollarının atomikliği düzeltilmiş. Açık olan exception ve yeniden giriş sözleşmesidir; mevcut shipped callback'in bunu yaptığı iddia edilmiyor. RAII yalnız kilit temizliğini çözer; callback sırasında Clear ve katmanlar arası commit için de açık politika gerekir.

Kaynak: [AbilityRuntimeSystem](../SpaceAbilitySystem/include/abilities/AbilityRuntimeSystem.h), satır 154–175, 322–327, 397–413; [loadout](../LightYearsGame/src/gameplay/ability/loadout/AbilityLoadoutManager.cpp).

### 4. P2 — Nano Plague enfeksiyon abonelikleri birikiyor

Yeni enfeksiyon her seferinde hedefin `onDamageResolved` olayına bağlanıyor; dönen handle saklanmıyor. `RemoveInfection` yalnız tag'i ve liste elemanını kaldırıyor. Aynı controller başka enfeksiyonlar nedeniyle yaşamaya devam ederken aynı hedef expire→reinfect döngüsüne girerse aynı receiver için birden çok callback kalıyor. Delegate `AddCallback` tekilleştirmiyor. Weak receiver use-after-free'yi önler, yaşayan receiver'ın yinelenen aboneliklerini önlemez. Hasar başına yinelenen enfeksiyon taramaları ve controller ömrü boyunca artan maliyet oluşur; katlanmış hasar iddiası değildir.

Kaynak: [controller](../LightYearsGame/src/gameplay/ability/nanoPlague/NanoPlagueControllerActor.cpp), satır 136–139 ve 362–378; [Delegate](../LightYearsEngine/include/framework/Delegate.h), satır 444–459. Çözüm: enfeksiyonun sahip olduğu abonelik handle'ını kaldırmada/destruction'da çözmek veya hedef başına tek sahipli abonelik.

### 5. P2 — L5d direct reflection world izolasyonunu zorlamıyor

Direct servis defender World'ündeki registry'yi bulur fakat projectile World'ü ile karşılaştırmaz. Registry ve Return Protocol receiver yolu da bu farkı reddetmez. Dolayısıyla public API çapraz-world çift verilirse world-owned registry ile beklenen izolasyonu garanti etmez. Normal aynı-world collision'da gözlenmiş olay değil, kaynakta eksik giriş invariant'ıdır.

Kaynak: [service](../LightYearsGame/src/gameplay/projectile/ProjectileReflectionService.cpp), satır 140–153; [registry](../LightYearsGame/src/gameplay/projectile/ProjectileReflectionRegistryActor.cpp), satır 89–102. Çözüm: servis/registry sınırında world ve pending-destroy geçerliliği.

### 6. P2 — L5d registry araması mermi sıcak yolunda bütün World'ü tarıyor

Her `FindRegistryActor` çağrısı `GetActorsByTypeIncludingPending` ile aktif/pending actor'ları RTTI üzerinden tarayıp sonuç vektörü oluşturur. Gravity Anomaly hareket adımında swept reflection çağırır. P projectile ve N actor için sadece registry bulma O(P×N) olabilir; registry olmayan sahnede de tarama vardır. Bu ölçülmüş FPS düşüşü değil, kaynakta görünen ölçeklenme gerilemesidir. Güvenli world-owned doğrudan erişim gerekir; process-global raw World map'e dönülmemeli.

Kaynak: [service](../LightYearsGame/src/gameplay/projectile/ProjectileReflectionService.cpp), satır 271; [World](../LightYearsEngine/include/framework/World.h), satır 141; [projectile](../LightYearsGame/src/gameplay/ability/gravityAnomaly/GravityAnomalyProjectileActor.cpp), satır 340.

### 7. P2 — D1 beam menzilinin tam ucunda duvar eşitlik kuralı eksik

Static sweep yalnız `fraction < earliestHit.fraction` kabul eder ve `fraction >= 1` için false döner. Duvarla ilk temas tam segment sonunda (`t=1`) ise “duvar yok” sonucudur. Beam handler bu durumda hedef-wall sıralama filtresini atlar; aynı uç noktada hedef kesişimi varsa hedef hasar alabilir. Bu, normal duvar kesmesinin tümden bozuk olması değil, “eşit mesafede duvar kazanır” kararının uç noktadaki istisnasıdır. Temas varlığı ile fraction=1 sentinel'i ayrılmalı; movement davranışı sessizce değiştirilmemeli.

Kaynak: [sweep](../LightYearsGame/src/gameplay/movement/MovementCollisionService.cpp), satır 104–114; [beam](../LightYearsGame/src/gameplay/weapon/handlers/ContinuousBeamWeaponHandler.cpp), satır 155–206.

## Paket bazlı kaynak hükmü

| Paket | Üretim kodundan sonuç |
|---|---|
| T0a/T0b | Test altyapısı paketleri; bu kaynak incelemesinde ürün doğruluğuna kanıt sayılmadı. |
| C1 | Exact match gerçek eşitliğe geçirilmiş; amaçla uyumlu. |
| C2 | Heal düzeltmesi mevcut, max/excess kararı açık. |
| C3 | Bariyer dış→dış segmentin ilk çember kökünü ve projectile radius'unu kullanıyor; eski kaçış koşulu kaldırılmış. |
| C4 | PlayerManager before-destroy/created olayları ile HUD teardown/rebind akışı kurulmuş. |
| C5 | Başarısız validation'da slot kaybı düzeltilmiş; callback sınırı açık. |
| L1 | Actor-ID bazlı per-query tekilleştirme ve erken çıkış korunmuş. Set tahsisleri nedeniyle allocation-free olarak yorumlanmamalı. |
| L2 | İki hedef cooldown'ı unique ID+expiry ve prune kullanıyor; eski adres tekrar kullanımı kusuru giderilmiş. |
| L3 | deque ile Player adres kararlılığı; gemi delegate handle'ında destructor/rebind cleanup mevcut. |
| L4 | Chain Lightning weak World ve cast'e ait timer sahibi/handle iptali kullanıyor. |
| L5a | Closed Circuit world-owned registry, weak değer ve generation korumalı token mevcut. |
| L5b | Nano Plague registry taşıması mevcut; controller'ın yayılma/abonelik kusurları ayrıca açık. |
| L5c | Portal runtime state world-owned, reentry kimliği unique ID. |
| L5d | World-owned registry ve RAII token mevcut; direct world guard ve lookup maliyeti açık. |
| D1 | Normal statik duvar clipping'i mevcut; t=1 eşitlik sınırı açık. |
| D2.1 | Test organizasyonu; üretim kodu doğruluk hükmüne dahil değil. |
| D2.2 | Energy Spear family-local registration pilotu ve ortak gameplay/CMake bağlantısı mevcut; tüm aile migrasyonu vaat edilmemiş. |
| D2.3 | JSON parser ayrı modüle taşınmış; loader orchestration sahibi olarak kalıyor. |
| D2.4 | Eski behavior leaf tag temizliği ve Relay Prism canonical alias mevcut. |
| D2.5 | Scheduler değiştirilmemiş; doğrusal timer dolaşımı sürüyor. Kod okumasıyla performans eşiği aşıldı/aşılmadı denemez. |
| D2.6 | HUD canonical CombatRuntime damage event'ini dinliyor; eski SpaceShip bridge kaldırılmış. |

Öncelik: Nano Plague bellek güvenliği, C2 karar/formül uyumu, ardından callback/abonelik/izolasyon sınırları. Uyumu görülen paketleri yeniden yazmak gerekmiyor. Önceki rapordaki build/E2E sonuçları bu kaynak hükmünün gerekçesi değildir.
