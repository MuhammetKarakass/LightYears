# Düzeltmelerin kaynak denetimi — 2026-09-26

**Sonraki uygulama:** Bu denetimin dört bulgusu bağlantılı çağrı/cleanup yollarıyla ele alındı. Tasarım kararları, 51 runtime assertion ve son 6/6 E2E kanıtı [yaşam döngüsü düzeltme raporunda](LIFECYCLE_FIXES_2026-09-26.md). Aşağıdaki metin düzeltme öncesi tespittir.

## Hüküm ve sınır

Önceki “yedi bulguda iş kalmadı” hükmü fazla geniştir. İnceleme güncel üretim koduna dayanır; bu tur build/test çalıştırılmadı, runtime veya test kaynağı değiştirilmedi. Kapsam son yedi düzeltmenin sahipleri ve doğrudan entegrasyon yollarıdır; yeni bir bütün-repo denetimi değildir. Aşağıdaki koşullu callback yollarının mevcut shipped dinleyiciler tarafından tetiklendiği doğrulanmadı; kaynakta mümkün olmaları ile oyunda gözlenmiş çökme ayrıdır.

## 1. P1 — Tick callback'inden Clear çalışan ability'yi silebiliyor

[AbilityRuntimeSystem.h](../SpaceAbilitySystem/include/abilities/AbilityRuntimeSystem.h), `Tick` ve `Clear`: mutation derinliği yalnız grant/rebind/remove sırasında artırılıyor. Tick doğrudan `mCallbacks.tick(*ability, deltaTime)` çağırıyor. Tick'in ürettiği `onAbilityActivated` veya `onAbilityChanged` dinleyicisi component `Clear()` çağırırsa derinlik sıfır olduğundan collection hemen temizleniyor.

[GameplayAbilityInstance.h](../SpaceAbilitySystem/include/abilities/GameplayAbilityInstance.h), `TryActivate` aktivasyon bildiriminden sonra `mDefinition.lifetimePolicy` okuyor; `Tick` de aktivasyon dönüşünden sonra runtime state'e erişiyor. Böylece callback'in sildiği instance üzerinde çalışmaya devam ediliyor: use-after-free yolu açık. Grant callback'i için eklenen erteleme bu yolu kapsamıyor. Mevcut shipped tetikleyici doğrulanmadı; bu yeni yaratılmış hata iddiası değil, lifecycle korumasının eksik sınırıdır.

Gerekli düzeltme: yaşayan instance'a girilen yürütme/bildirim sınırını korumak; Clear yanında aktif instance'ı remove/replace eden yolları da aynı yaşam süresi kuralıyla değerlendirmek. Yalnız Tick'e Clear guard eklemek tüm mutation türlerini çözmez.

## 2. P1 — Component Clear bütünü exception ve erteleme bakımından korunmuyor

[SAS component Clear](../SpaceAbilitySystem/src/AbilitySystemComponent.cpp) önce ability runtime'ı, sonra effects/tags/attributes'ı temizliyor. Runtime Clear cancel callback hatalarını toplayıp ability collection'ı temizledikten sonra yeniden fırlatıyor. Aktif ability'nin `onAbilityEnded` dinleyicisi hata fırlatırsa üst Clear kalan effect/tag/attribute temizliğine ulaşamıyor. [Game component Clear](../LightYearsGame/src/gameplay/ability/LightYearsAbilitySystemComponent.cpp) sonundaki primary-weapon override temizliği de atlanıyor. Runtime'ın exception garantisi component'in tamamına yayılmamış.

Ters sıradaki sorun da mevcut: grant/rebind callback'i component Clear çağırdığında yalnız ability temizliği erteleniyor; effect/tag/attribute temizliği hemen gerçekleşiyor. Dış mutation sonunda ability cancel/ended callback'leri daha sonra çalışıyor. Bu callback bir owned tag/effect eklerse sonrasında ikinci component temizliği yok; Clear sonrası artık state kalabiliyor. Bu tetikleyiciler koşulludur; shipped dinleyici kanıtı yok.

Gerekli düzeltme: teardown isteğini component seviyesinde tek işlem olarak yönetmek; bütün alt sistemlerin cleanup'ını tamamladıktan sonra ilk exception'ı yaymak ve teardown boyunca yeniden state ekleme politikasını açıkça uygulamak.

## 3. P2 — Dönen gövdesiz kutunun spatial hücreleri eski kalıyor

[World bounds](../LightYearsEngine/src/framework/World.cpp), `GetManualSpatialBounds` artık kutuları actor/local rotation ile hesaplıyor. Ancak [Actor::SetActorRotation](../LightYearsEngine/src/framework/Actor.cpp) yalnız sprite ve Box2D transform güncelliyor. `UpdatePhysicsTransform` gövdesiz aktörde hiçbir şey yapmıyor; manual index refresh yok. World sorgusu sadece sorgu hücrelerindeki adayları dolaştığından yeni dönen bölgeye hiç aday gelmeyebiliyor.

Somut koşul: gövdesiz, radius=0, merkezi (128,128), yarı boyutu (400,10) olan kutu indekslendikten sonra yer değiştirmeden 90 derece döndürülür. Eski kayıt y=0 hücresindedir; yeni kutunun y≈450 bölgesindeki dar sorgu y=1 hücresine gider ve aktörü bulamaz. Başka bir spatial-refresh tetiklenene kadar beam/movement duvarı atlayabilir. Sabit rotasyonla ilk kayıt bu hatanın koşulu değildir.

Gerekli düzeltme: rotasyon değişiminde manual spatial kaydını da yenilemek. Başlangıç rotasyonuyla kayıt testi değil, kayıt sonrası yalnız rotasyon değişimiyle hücre aşan akışı doğrulamak.

## 4. P1, koşullu — Nano Plague Tick hâlâ vector referansını callback boyunca tutuyor

[NanoPlagueControllerActor.cpp](../LightYearsGame/src/gameplay/ability/nanoPlague/NanoPlagueControllerActor.cpp), Tick: `Infection& infection = mInfections[index]` referansı `ApplyTick` boyunca korunuyor; sonrasında `++infection.ticksApplied` yapılıyor. ApplyTick gerçek damage akışını çağırıyor; [CombatRuntime](../LightYearsGame/src/gameplay/combat/CombatRuntime.cpp) senkron gameplay event ve `onDamageResolved` yayımlıyor.

Bu dinleyicilerden biri aynı controller'ın public `ApplyOrRefreshInfection` API'siyle başka bir hedef ekler ve vector yeniden ayrılırsa dönüşte infection referansı geçersizdir. `mDestroying` kontrolü ekleme/reallocation'ı tespit etmez. Eski spread-origin dangling pointer'ı düzeltilmiştir; burada farklı, callback yeniden girişine bağlı bir referans yolu vardır. Bunu yapan shipped dinleyici tespit edilmedi; normal mevcut akışın her tick'te çöktüğü iddia edilmiyor.

Gerekli düzeltme: callback sonrası enfeksiyonu kararlı kimlikle tekrar bulmak ve refresh/removal/reapply semantiğini belirlemek; callback boyunca vector eleman referansı tutmamak.

## Kaynakta doğrulanan düzeltmeler

- Spread-origin artık değer snapshot'ı; yeni ekleme öncesi origin kopyası var.
- Enfeksiyon damage handle'ı tutuluyor; refresh yeniden bağlanmıyor; remove/destroy/destructor unbind ediyor.
- Can/kalkan pozitif excess'i yeni max üstünde mutlak koruyor; ledger reconcile çağrısı var.
- Static sweep `foundHit` ile fraction=1 temasını ayırıyor; ilk kayıt için kutu broadphase bounds'u mevcut.
- Reflection direct/swept registry girişlerinde same-World/pending kontrolleri mevcut; lookup actor listesi yerine World-owned typed weak service slot'u kullanıyor. Receiver taraması ayrı; FPS ölçümü yapılmadı.
- Grant/rebind/remove rezervasyon unwind'ı ve loadout'un exception sonrası runtime'dan eşitlenmesi mevcut. Bu kazanımlar yukarıdaki lifecycle açıklarını kapatmıyor.
- Root AGENTS.md'de zorunlu Flash workflow bölümü yok.

Önceki başarılı build/E2E kayıtları tarihsel kanıttır; yukarıdaki farklı tetikleme koşullarının doğrulaması değildir.
