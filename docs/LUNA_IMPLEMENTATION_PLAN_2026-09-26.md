# Luna implementasyon planı — callback, kaynak ömrü ve işlem tutarlılığı

Durum: **plan; uygulanmadı**. Hazırlayan: Codex. Hedef uygulayıcı: Luna. Bu belge worker başlatmaz. Kullanıcı uygulamaya verdiğinde aşağıdaki paketler sırayla yürütülür.

## 1. Amaç ve başarı tanımı

Amaç sekiz bulgunun satırlarını ayrı ayrı yamamak değil; callback çağrısı sırasında hangi kaydın yaşadığı, hangi değişikliğin commit edildiği ve hangi cleanup borcunun kaldığı için tutarlı sözleşmeler kurmaktır. Özellikle R5'te önceki Codex düzeltmesinin eklediği **eksik cleanup'a rağmen sahip kaydını silme/instance'ı bitmiş sayma** sonucu giderilecektir.

Başarı: aşağıdaki kabul matrisi güncel üretim koduyla geçer; aynı callback sınırına bağlı tüketiciler kaynak üzerinden incelenir; normal oyun akışı, balance, slot mapping ve presentation değişmez. Sıfır hata garantisi verilmez. Başarısız veya koşulmayan kapı varsa paket tamamlandı denmez.

Önce oku:

1. Kök `AGENTS.md`, `.agents/skills/lightyears-engineering/SKILL.md`.
2. [Köken denetimi](FINDING_PROVENANCE_AUDIT_2026-09-26.md): bu planın bulgu dayanağı.
3. [Kaynak incelemesi](REMAINING_DESIGN_REVIEW_2026-09-26.md): koşullar ve bağlantılar.
4. [Identity kuralları](IDENTITY_AND_CONTRACT_RULES.md), `docs/vault/01 - Architecture/Ownership and Lifetime.md`.
5. İlgili paketin direct owner ve en yakın çağıranları; sonra gerekirse graph keşfi. Eski indeks veya belgeyi güncel kod yerine kullanma.

Köken gerçeği: R1/R2/R3/R4/R6/R7/R8 mekanizmaları ce14e93'te de vardır. R5 eski eksiklik + Codex yamasının yeni hata sonucudur. R2 reverse-map **std::map**; rehash gerekçesi yanlıştı. Bunu düzeltmek için map türü değiştirme. Shipped tetikleyicisi kanıtlanmamış yolları gözlenmiş oyun çökmesi diye sunma.

## 2. Kapsam, yasaklar ve çalışma biçimi

- Null Pulse, wave/encounter ve basit AI inceleme/değişiklik kapsamı dışında. Bağımlılık nedeniyle derlenmeleri inceleme sayılmaz.
- Mevcut dirty çalışma ağacı korunur. `git reset --hard`, toplu restore, otomatik commit/push, eski commit'e dönüp geliştirme yok. ce14e93 yalnız tarihsel karşılaştırma içindir, implementasyon tabanı güncel ağaçtır.
- Slot/balance/JSON, normal cooldown/charge, hasar sırası, loadout live view, Nano kimlikli depolama ve önceki reflection/beam düzeltmeleri korunur.
- Yeni universal event bus, generic transaction framework, global registry, std::any veya engine Actor'a gameplay state ekleme yok.
- Aynı kaynaklarda eşzamanlı iki worker yok. Paketleri aynı Luna sırayla veya birbirinin teslimini okuyan Luna'lar sırayla uygular. Paralel implementasyon uygun değil.
- Unit test ekleme/çalıştırma yok; kullanıcının açık tercihi **gerçek runtime E2E**. `LightYearsGasLiteTests` yalnız derlenir. Sadece bir sınıfı mock'layan testi E2E diye adlandırma.
- Her paket öncesi kısa change contract yaz: amaç, izinli dosyalar, canonical owner, invariant, kanıt. Yeni dosya ancak gerekli yerel helper için; compiled source eklenirse mevcut shared CMake listesi üzerinden game/GasLite/E2E'ye bağla.
- Dosya sınırı dışındaki bir değişiklik zorunlu çıkarsa önce gerekçeyi teslim kaydında açıkla; unrelated refactor yapma. Normal gameplay/public contract değişikliği gerektiriyorsa kullanıcı kararı olmadan uygulama.

## 3. Paketler ve bağımlılıkları

| Paket | İş | Bulgular | Önkoşul |
|---|---|---|---|
| P0 | Güncel başlangıç ve sözleşme kapısı | R1–R8 | Yok |
| P1 | Attribute mutasyonu ve sahiplik bildirimi | R2, R1/R5 altyapısı | P0 |
| P2 | Effect apply/refresh/remove ve kaynak başına cleanup | R1, R5-effect | P1 |
| P3 | Ability execution, iptal, immutable action tanımı ve cleanup | R3, R4, R5-action | P2 |
| P4 | Player upgrade commit/ödeme tutarlılığı | R6 | P3 |
| P5 | Damage context ve nested event ömrü | R7 | P2, P3 |
| P6 | One-shot timer tüketimi | R8 | P0 |
| P7 | Birleşik E2E, kaynak denetimi ve teslim | R1–R8 | P1–P6 |

Uygulama sırası **P0 → P1 → P2 → P3 → P4 → P5 → P6 → P7**. Belgedeki bütün paket referansları bu tabloda tanımlıdır; A1 gibi başka bağımlılık yoktur.

## 4. Ortak hata ve callback sözleşmesi

1. Callback'ten önce canonical kayıt kendi içinde tutarlı olmalı. Bildirim boyunca iterator/reference/pointer korunacaksa bunu sağlayan açık owner/scope bulunmalı.
2. Callback'in silme isteği mantıksal iptal olarak hemen görünür; fiziksel silme aktif borrower çıkışına kadar ertelenebilir. İptal işaretinden sonra yeni hasar/action/aktivasyon bildirimi üretme.
3. Cleanup kaynak başına tamamlanır; tek bir foreach'in etrafındaki catch yeterli değildir. İlk exception saklanır, bağımsız kalan cleanup yapılır, ilk exception sonunda yeniden yayılır. Exception swallow ederek başarılı sonuç verme.
4. RAII destructor throw etmez. Hata üretebilen drain/cleanup açık finalize noktasında yapılır; destructor yalnız guard/state geri yükler.
5. İşlem callback'te Clear alırsa component, alt kayıtlar ve dış tüketiciler aynı tamamlanma sınırını kullanmalı. Guard çıkışında instance silinebiliyorsa ardından `this` üyelerine erişme.
6. Keyfi gameplay yan etkileri geri alınamaz. Hasar veya gönderilmiş event için genel rollback vaat etme. Sahip olunan modifier/tag/visual gibi cleanup borçları ile dış yan etkileri ayır.
7. Değişiklik sonrası ilk throw'un hatası korunur; cleanup sırasında ikinci throw ilkini maskelemez. Allocation failure dahil sınırsız güçlü rollback vaat edilmez; scope guard ve kaynak sahipliği yine sızdırılmamalı.

## P0 — Başlangıç ve tasarım sözleşmesi

**Çıktı:** uygulama sırasında oluşturulacak `docs/LUNA_IMPLEMENTATION_ACCEPTANCE_2026-09-26.md` içinde başlangıç bölümü. Bu dosya bu plan turunda oluşturulmuş değildir.

1. `git status --short`, `git rev-parse HEAD`, ilgili dosyaların başlangıç diff'ini ve SHA-256 değerlerini kaydet. HEAD farkını kendi değişikliğin sayma; teslimde başlangıç→son farkını ayır.
2. Yukarıdaki iki raporu ve güncel direct owner'ları karşılaştır. Artık mevcut olmayan bulguyu zorla uygulama; kapanmışsa kaynak kanıtıyla işaretle.
3. Gerçek E2E fixture girişlerini `LightYearsGame/tests/AuditFixesE2E.cpp`, `TimerManagerSceneE2E.cpp`, `ContinuousBeamWallE2E.cpp` ve `LightYearsGame/CMakeLists.txt` üzerinden doğrula. İşlem baseline'ı için mevcut E2E'leri çalıştır; eski log'u yeni kanıt diye kullanma.
4. P1–P6'daki semantik kararları bir paragrafla teyit et. Özellikle aynı effect'e nested apply, aktif definition güncellemesi ve satın alma commit sınırında belirsizlik bırakma. Aşağıdaki hedef normal shipped davranışla çatışıyorsa koddan kanıtla dur; yeni oyun davranışını kendi başına seçme.
5. Baseline failure varsa değiştirilmemiş dosyaya ait hata ile bu işin hatasını ayır; kapsamdışı encounter/unit başarısızlığını onarmaya girişme.

**Kapı:** Kullanılacak başlangıç kaydı var; bütün R başlıkları bir pakete bağlı; test isimleri mevcut CMake ile eşleşiyor.

## P1 — AttributeSystem tutarlılığı

**Owner:** `SpaceAbilitySystem/include/attributes/AttributeSystem.h`, `SpaceAbilitySystem/src/attributes/AttributeSystem.cpp`. En yakın tüketici: effect bindings. Engine `Delegate` semantiğini genel olarak değiştirme.

### Uygulama

1. RemoveModifier'da attribute ID'yi değer olarak al. Modifier kaydı ve reverse-index kaydını **Recalculate bildiriminden önce** birlikte kaldır. Callback sonrasına map iterator'ı taşıma. Aynı handle'ın ikinci kez kaldırılması no-op olmalı.
2. Recalculate'a verilen ID değerinin callback boyunca ömrü garanti olsun. Clear sonrası lookup sonuçlarını tekrar kullanma. Recalculate'ın callback sonrasında başka member/reference erişimleri olup olmadığını kontrol et.
3. AddModifier için effect sahibinin handle'ı **dış callback çalışmadan önce** kaydedebilmesini sağla. Dar iki-aşamalı dahili mutasyon/bildirim API'si veya commit edilmiş handle'ı çağırana callback öncesi yazan sonuç kanalı kullanılabilir; ortak event sistemini yeniden kurma. Seçilen biçimi P0 sözleşmesinde yaz.
4. Dış AddModifier API'sinin normal event sırasını koru. Kayıtlı olmayan attribute'u oluştururken `onAttributeRegistered` callback'inin de çalıştığını unutma: bu callback Clear/throw yaparsa eski entry/modifier referanslarıyla devam etme. Gerekli girdileri önceden değer kopyası al; implicit registration ve revision davranışını incele.
5. Handle kaydı başarılı olup bildirim throw ederse effect owner handle'ı yine bilmeli. `AddModifier` dönüşünden sonra push_back yapmak yeterli değildir. Owner ledger için gereken kapasite/yer tahsisini callback'ten önce hazırla.
6. std::map'i değiştirme; problem erase/Clear'dır. Attribute Clear içinde handle sayaçlarını yeniden kullanıma açma.

### Kabul

- Bir modifier kaldırılırken callback aynı handle'ı kaldırır: tek gerçek kaldırma, recursion/UB yok.
- Callback başka modifier ekler/kaldırır: diğer kayıtlar doğru, değer/revision tutarlı.
- Callback attribute Clear yapar: eski iterator'a erişim yok; yeni yaşamın handle'ı eskisiyle çakışmaz.
- Add bildirimi throw eder: eklenen handle kayıtlı owner tarafından temizlenebilir; kayıt hiçbir sahibin bilmediği durumda kalmaz.

Bu senaryolar gerçek World/SpaceShip attribute ve effect akışında E2E fault injection ile doğrulanır.

## P2 — Effect başına işlem ömrü ve eksiksiz cleanup

**Owner/dosyalar:** `SpaceAbilitySystem/include/effects/GameplayEffectRuntimeSystem.h`, `GameplayEffectRuntimeState.h`, `GameplayEffectCollection.h`, `GameplayEffectBindings.h`, `SpaceAbilitySystem/src/effects/GameplayEffectBindings.cpp`; component sınırı gerekirse `SpaceAbilitySystem/include/AbilitySystemComponent.h` ve `src/AbilitySystemComponent.cpp`. Game effect behavior/presentation çağıranları okunur; somut cleanup owner düzeltmesi gerekirse yalnız ilgili feature'a inilir.

### İşlem politikası

- Bir effect'in apply/refresh/cleanup'ı boyunca canlı kayıt veya bağımsız ownership ledger korunur. `std::list` tek başına silinmeye karşı koruma değildir.
- Callback aynı effect'i kaldırırsa remove-request işaretlenir; aktif mutasyon kaydı fiziksel olarak silinmez. Dış apply/refresh güvenli noktada isteği görür, yeni aşamalara başlamaz, cleanup'ı tamamlar.
- Aynı effect/stacking key'e apply veya refresh kendi mutasyonu sürerken yeniden girerse açık ret/invalid sonuç ver; recursive refresh çalıştırma. Farklı effect'lere normal nested işlem engellenmez. Ret kararı sessiz başarı değildir; bool/handle sonucunda görünür ve belgelenir.
- Deferred remove için internal bool/count semantiği **kabul edilmiş remove isteği** olarak açıkça belgelenir; callback içindeki fiziksel Find sonucu canlı kaydı gösterebilir, ancak kaydın pending-remove durumu yeni işlem başlatmayı engeller. Public void remove değişmez. Dış işlem bittiğinde pending remove kalmamalı.

### Uygulama

1. Kaydı handle ile rezerve eden küçük owner-local scope kur. Pending remove/clear ve reentry durumunu canonical effect kaydında veya tek handle tablosunda tut; paralel kopya indeksler üretme.
2. Yeni apply ile refresh'in her callback sınırını listele: BindSource, Initialize/Refresh, modifier registration/change, tag grant/remove, behavior Activated/Removing, collection notification. İç fonksiyonlardaki callback'leri de say.
3. P1 üzerinden modifier handle'ı callback öncesi effect ledger'ına girer. Grant edilen tag katkısını da gerçek refcount semantiğine göre izle. Henüz eklenmemiş tag'i rollback diye kaldırıp başka effect'in ortak tag katkısını düşürme.
4. Apply sırasında remove/Clear/throw olduğunda yalnız bu işlemin gerçekten edindiği kaynakları bırak. Normal yola ait Activated/Applied başarı bildirimi cancellation sonrasında çıkmasın. Başarı handle'ı, güvenli dış sınırda effect mevcut değilse dönmesin.
5. Refresh eski kaynakları bırakıp yenileri edinirken aynı sahiplik kuralını uygula. Hata durumunda kısmen yeniden kurulmuş effect'i çalışır sayma: tutarlı biçimde kaldır, ilk hatayı yay. Eski effect durumuna genel rollback vaat etme; bu failure politikası açık olsun.
6. RemoveModifiers/RemoveTags içindeki her kaynak için ayrı cleanup dene. Callback throw etse bile kalan modifier/tag katkıları bırakılır. İlk hata saklanır. Already-removed kaynak idempotent no-op'dur.
7. Effect düğümü ancak sahip olduğu kaynak cleanup borcu güvenle boşaldıktan sonra silinir. Behavior callback'i iç kaynak tutuyorsa End/Removing owner'ını da incele; “çağırdım”ı “temizlendi” sayma. Yönetilemeyen kaynak kalırsa sessiz orphan oluşturma, paket kapısını kapatma.
8. `mRemovalInProgress`, pending flags ve operation depth exception sonrasında yeniden kullanılabilir durumda olmalı. Eski kodun removal-in-progress'te sonsuza kadar takılması geri gelmesin.
9. Full component Clear, tek effect Remove ve effect runtime Clear ayrı ayrı doğrulansın. Tüm attributes'ı temizleyerek tek-effect leak'ini gizleme.

### Kabul

- Apply callback'inde kendini remove: geçersiz handle dönüşü, dangling erişim yok, net tag/modifier katkısı sıfır.
- Refresh callback'inde remove; stack cap/stack add ve duration-only refresh: uygulanabilir her kolda aynı garanti.
- İlk/orta/son modifier kaldırma callback'i throw: bütün effect kaynakları gider, başka effect'in modifier/tag'leri korunur; ilk hata yakalanır.
- Aynı tag'i paylaşan iki effect'ten biri yarıda apply/remove olur: diğerinin tag'i ve refcount'u korunur.
- Farklı effect ekleyen callback, aynı effect'e nested apply, Clear ve throw+Clear kombinasyonları; işlem sonrası temiz bir effect tekrar uygulanıp kaldırılabilir.

## P3 — Ability execution'ın kendi ömrü ve definition kararlılığı

**Owner/dosyalar:** `SpaceAbilitySystem/include/abilities/GameplayAbilityInstance.h`, `AbilityExecution.h`, `AbilitySystemRuntime.h`, gerektiğinde `AbilityRuntimeSystem.h`; game `GameAbility.h/.cpp`, `GameAbilityActionExecutor.h/.cpp`, `actions/FireWeaponActionRuntime.cpp`, `LightYearsAbilitySystemComponent.h/.cpp`. Son ikisinin header'ları include/gameplay/ability, source'ları src/gameplay/ability altındadır. Weapon cleanup gerekirse `PrimaryWeaponExecutionSystem` ve ilgili handler direct owner olarak ele alınır.

### A. Tick içi iptal ve mutasyon

1. Activation guard'ını kopyalayıp yeni birkaç bağımsız bool eklemek yerine instance'ın aktif callback/execution kapsamını tanımla. Begin/Tick/End ve behavior callback girişlerinde yürüyen execution'ın fiziksel action storage'ı korunmalı.
2. Cancel yürüyen action sırasında gelirse reason kaydedilir; action stack'i dönene kadar vektör temizlenmez. Clear/OwnerDestroyed en güçlü sonlandırma isteğidir; normal Cancel bunu düşüremez. Aynı öncelikte ilk istek korunur; bu politika tek owner'da yazılır.
3. İptal isteğinden sonra diğer action, behavior Tick ve duration ilerletmesi çalışmaz. Hem dış loop hem ExecuteAction/SimulateActiveFire dönüşündeki action-state erişimleri kontrol edilir; sadece dış for'a break eklemek yetmez.
4. Nested aynı-instance Tick/TryActivate reddedilir. Yürüyen callback içindeki SetLevel gibi bool mutasyonlar **false** ile reddedilir; uygulanmış gibi true dönüp daha sonra kuyrukta çalıştırma. Callback dışındaki level değişimi mevcut end-then-rebuild davranışını korur.
5. Scope finalize sırasında deferred end yalnız bir kez gerçekleşir. Component guard'ın çıkışı instance'ı yok edebileceğinden ondan sonra instance üyesi okunmaz.

### B. Scoped tanım yenilemesi

1. Aktif execution'ın action spec adresleri değişebilir `mDefinition.actions` içine işaret etmemeli. Tercih: execution'a ait immutable action-spec snapshot'ı; tek allocation ile kararlı adresler ve Begin/WhileActive/OnEnd için aynı snapshot. Kopyalama/move sonrası pointer bağlarının geçerliliğini tasarla; vector kopyasını alıp pointer'ları eski vektörde bırakma.
2. Scoped rule güncellemeleri mevcut public API'lerde korunur. Aktif execution'ın **yapısal action listesi** mevcut aktivasyon boyunca sabittir; sonraki activation güncel listeyi alır. Scope değişikliği implicit cancel/restart veya fazladan OnActivate tetiklemez.
3. Sayısal live scaling/primary-weapon configuration yenilemesinin mevcut owner'ını koru. Action topolojisi snapshot'ı, bütün attribute değerlerini aktivasyon başında dondurmak anlamına gelmez. `context.definition`, action spec, behavior context ve primary-weapon config'in hangi revision'ı okuyacağını tabloya dök; canlı sayısal güncelleme ile structural spec ömrünü ayır.
4. Scope mutation callback içinde olursa structural snapshot etkilenmez; mutable runtime/config değişimi aktif borrower'ın güvenli çıkışında birleştirilerek uygulanır. Aynı scope listesini gezen callback'te vektörü değiştirme; operation kapsamına rule store mutasyonu da girsin. En son canonical rule set'ten bir kez yeniden hesapla; Clear olursa bekleyen refresh'i iptal et.
5. Bu politika mevcut shipped davranışla çelişiyorsa P0 kapısına geri dön. Güvenlik için balance/hot-reload semantiğini sessizce değiştirme.

### C. Action cleanup — R5'in ikinci yarısı

1. End her active action cleanup'ını ayrı ayrı dener; bir EndFire hatası sonraki action'ın EndFire'ını atlatmaz. OnEnd action'ları tanımlı sırayla en fazla bir kez denenir; normal başarısızlıkta sonraki bağımsız cleanup'lar atlanmaz.
2. FireWeapon runtime owner'ında `lifecycleStarted`, persistent runtime pointer, beam/visual ve pending simulation time borçlarını incele. Sahiplik bitişini callback'ten önce güvenle ayır; callback throw sonrası nesne aktifmiş gibi kalmasın. Bir visual Destroy throw ederse kalan visual cleanup'ı da denensin.
3. `mExecutionStarted=false` veya EndActivation yapmak, iç cleanup'ın tamamlanmasının yerine geçmez. Her bağımsız kaynak ya bırakılmış ya açık bir owner'da tutuluyor olmalı. Kısmi temizliğin üstüne execution.clear yapıp borcu kaybetme.
4. İlk hata saklanır; state transition ve ended bildirimi için mevcut commit/abort ayrımı korunur. Başlamamış execution'ın OnEnd action'ı çalışmaz.

### Kabul

- İlk ve orta WhileActive action callback'inden Cancel; kalan action/behavior Tick çalışmaz, end bir kez.
- Aynı callback'ten doğrudan SetLevel, component SetAbilityLevel, nested Tick/TryActivate: ret sonucu doğru, level/charge değişmez.
- Cancel+Clear+throw kombinasyonu: ilk hata korunur, action/storage/visual borcu kalmaz.
- Scope değişiminde aktif spec adresleri kararlı; yeni activation yeni action listesini alır; OnEnd eski activation snapshot'ından tam bir kez çalışır.
- En az iki active kaynak içeren execution'da ilk EndFire/visual callback throw: ikincisi temizlenir; component yeniden kullanılabilir.
- Önceki precommit abort/committed exception/Return Protocol/Echo senaryoları gerilemez.

## P4 — Player satın alma commit'i

**Owner/dosyalar:** `LightYearsGame/include/player/Player.h`, `src/player/Player.cpp`; SAS level mutasyon girişi ve sonucu için P3'teki instance/runtime/component dosyaları. Loadout binding owner'ı değişmez.

### İşlem kararı

Satın alma doğrulaması ve level commit'i ayrılacak. **Level gerçekten commit olmadan ücret kalıcı harcanmaz. Level commit olmuşsa daha sonraki observer exception/Clear satın almayı bedava yapmaz.** Commit olmuş satın alma kaydı Player'ın respawn progression'ında korunur. Bu yalnız hatalı/reentrant sınırların politikasıdır; normal fiyat/level davranışı aynı kalır.

### Uygulama

1. Başta ability kimliği, handle, hedef level ve fiyatı değer olarak al. Callback'li API sonrasında eski `GameAbility*` veya definition reference kullanma.
2. Player'a dar purchase-in-progress reservation ekle: satın alma callback'inden yeniden satın alma açıkça reddedilir; unsigned scrap underflow/double charge engellenir. Bütün çıkışlarda RAII ile açılır.
3. Canonical level setter, external level notification'dan **önce** commit edilmiş handle/id/level sonucunu çağırana bildirebilmeli. Dar result sink/commit observer API'si kullanılabilir; normal bool wrapper'ları koru. Commit callback'i keyfi gameplay kodu değil Player'ın önceden hazırlanmış kayıt/ödeme finalize adımıdır. Mevcut SetLevel'ın önce runtime level, sonra RebuildDefinition sırasını körlemesine koruma: yeni definition/config için hata üretebilecek hazırlığı geçici değer üzerinde yap, level/config'i tek güvenli commit noktasında geçir. Eski active execution'ı sonlandırmak geri alınamaz; precommit failure için level/scrap/purchased-record garantisi ver, geçmiş execution'ı yeniden canlandırma garantisi verme.
4. Purchased-level map kaydı/kapasitesi ve hata üretebilecek hazırlıkları level değişiminden önce hazırla. Internal commit adımı kullanıcı callback'i yayınlamasın; ödeme ve purchased-level kaydı beraber tutarlı hale gelsin. Public bildirimler sonra gelsin.
5. Level commit öncesi ret/exception: reservation serbest, ödeme/kayıt değişmemiş. Commit sonrası exception: ödeme/kayıt korunur, ilk hata yayılır; Clear olmuş ability yeniden aranıp dereference edilmez. Kayıt hazırlandıysa precommit ret yolunda yalnız bu işlemin geçici hazırlığını geri al; önceki satın alma kaydını silme. OnLevelConfigurationChanged dahil callback'lerin commit öncesi/sonrası sınıfını kaynakta açıkça belirle.
6. Sadece `ability->GetLevel()` yerine `targetLevel` yazmak yeterli değil; yanlış başarı/ödeme sınırı da çözülmeli. Genel world rollback veya bütün mutasyon API'lerini yeniden tasarlama yok.

### Kabul

- Normal satın alma: tam bir level ve tam bir ödeme; normal yetersiz scrap/max level aynı.
- Precommit ret/exception: level, scrap, purchased map değişmez.
- Level bildirimi Clear/throw/throw+Clear: pointer erişim hatası yok; commit sonrası ödeme ve purchased record korunur.
- Reentrant satın alma: ikinci işlem reddedilir, duplicate charge yok.
- Clear/respawn sonrası yalnız gerçekten commit edilen satın alınmış level geri yüklenir.

## P5 — Combat context'in senkron frame ömrü

**Owner:** `LightYearsGame/include/gameplay/combat/CombatRuntime.h`, `src/gameplay/combat/CombatRuntime.cpp`. `DamageContext.h` ve SAS `AbilityEvent.h` öncelikle okunur; universal owned-context/event türü eklemek varsayılan çözüm değildir.

### Tercih edilen dar çözüm

1. ProcessIncomingDamage için stack-sahipli bir **damage dispatch frame** kullan. Önceki frame/context pointer'ını RAII ile her çıkışta geri yükle. Nested işlem kendi frame'ini kullanır; outer frame'i nullptr yapmaz.
2. Damage sırasında gelen effect behavior event'lerini o frame'in yerel buffer'ında değer olarak tut. ProcessGameplayEffectEvent tamamlanınca context hâlâ canlıyken senkron dispatch et. `SetContext` ancak dispatch'teki geçici AbilityEvent kurulurken yapılır; stack context adresi cross-frame member kuyruğuna girmez.
3. Nested frame kendi event'lerini dispatch eder, outer buffer'a dokunmaz. Outer event'lerin context içeriği aynı damage işlemine ait olur; yeni tick'e sarkmaz. Hasar işleme öncesi bir snapshot alıp remainingDamage gibi güncel alanları yanlış dondurma.
4. Damage dışındaki effect event'leri mevcut member queue'da context'siz değer payload ile kalabilir. Bir frame throw ederse kendi dispatch edilmemiş event'leri düşer; diğer frame'lerin event'leri silinmez. Keyfi hasarı rollback etme.
5. Clear bir runtime generation/epoch sınırı oluşturur: başlamış local batch Clear sonrasında eski kalan event'leri yayımlamaz. Clear ile eski frame pointer'ını kalıcı yanlış state'e geri yüklememeye dikkat et.
6. Raw Actor* içeren DamageContext'i kopyalamak tek başına lifetime çözümü değildir. Senkron dispatch boyunca gerekli actor'ları weak lock/yerel güçlü referansla doğrula; pending-destroy semantiğini koru. Uzun ömürlü global source/target cache ekleme.

### Kabul

- Outer damage A içinde nested B; B bittikten sonraki outer event yine A context'ini görür.
- Effect callback throw sonrası sonraki Tick/başka damage eski stack context'i görmez.
- Event dispatch sırasında Clear: eski batch'in geri kalanı yayınlanmaz; yeni yaşam event'i eski epoch ile karışmaz.
- Normal damage phase, status/event sırası ve source/target metadata'sı baseline ile aynı.

## P6 — One-shot timer

**Owner:** `LightYearsEngine/src/framework/TimerManager.cpp`; header ancak zorunluysa. E2E: `LightYearsGame/tests/TimerManagerSceneE2E.cpp`.

1. One-shot timer'ın tüketilmiş/expired state'ini callback'ten önce commit et. Callback throw olsa da sonraki Update aynı timer'ı yeniden çalıştırmaz. Otomatik retry ekleme.
2. Listener expired/pending ise çağrı yok. Callback boyunca listener ömrünü doğrula; timer clear/scene geçişinde var olan weak-owner sözleşmesi korunsun.
3. Repeating catch-up cap, zaman borcu ve geçerli süre davranışını değiştirme.
4. Aynı manager'a nested UpdateTimer için açık no-op guard kullan; dış update'in mIsUpdating durumunu bozup container'ı callback sürerken flush etmesin. Diğer manager'ın update'i engellenmez. Global ShutdownTimerManagers'ı callback içinde güvenli hale getirecek yeni subsystem refactor bu paketin kapsamı değildir; bu ihtiyacı kanıtsız ekleme.

**Kabul:** one-shot throw yakalandıktan sonra ikinci update count hâlâ 1; normal callback count 1; callback ClearAll/add-new-timer/nested-update senaryoları invalidation yapmaz; repeating ve scene E2E geçer.

## 5. Zorunlu E2E matrisi

Her satır bir assertion grubu; yalnız process exit 0 yeterli değildir. Gerçek World/SpaceShip/component/player/timer ve production dispatch kullan. Hatalı callback test fixture'ından bağlanabilir; bu callback'in shipped olduğuna dair iddia üretme. UB üreten eski kodu çalıştırmak zorunlu değil; eski kaynak yolunu belgelemek yeterli, yeni kodda güvenli son state kanıtlanmalı.

| ID | Paket | Girdi | Beklenen kanıt |
|---|---|---|---|
| E01 | P1 | Remove callback aynı handle remove | Bir kaldırma, tutarlı değer/index |
| E02 | P1 | Add/Remove callback Clear ve throw | Dangling yok; handle sahipliği biliniyor |
| E03 | P2 | Yeni effect apply içinde kendini remove | Invalid sonuç, sıfır kendi kaynak katkısı |
| E04 | P2 | Refresh/stack sırasında remove/Clear | Son state temiz, stale başarı yok |
| E05 | P2 | İlk/orta/son cleanup callback throw | Tüm kaynaklar temiz; ilk hata korunur |
| E06 | P2 | İki effect aynı tag/modifier attribute | Birinin cleanup'ı diğerini bozmaz |
| E07 | P2 | Same-key nested apply, other-effect apply | İlki ret; ikincisi güvenli; sonsuz recursion yok |
| E08 | P3 | WhileActive ilk/orta callback Cancel | Sonraki action/behavior yok; bir end |
| E09 | P3 | Nested SetLevel/Tick/Activate | Ret/no-op; charge/level yan etkisi yok |
| E10 | P3 | Aktifken scope action listesi değişir | Eski execution kararlı; yeni activation güncel |
| E11 | P3 | İki resource; ilk End throw | İkinci cleanup çalışır; orphan visual yok |
| E12 | P4 | Normal/precommit fail/postcommit throw | Level/ödeme/record doğru commit sınırında |
| E13 | P4 | Purchase callback Clear + nested purchase | Stale pointer/double charge yok; respawn kayıt doğru |
| E14 | P5 | Nested A/B damage | Context sequence A→B→A |
| E15 | P5 | Throw ve Clear sırasında event batch | Eski context/event sonraki tick'te yok |
| E16 | P6 | One-shot throw + ikinci update | count=1, first error doğru |
| E17 | P6 | ClearAll/add/nested Update | Container güvenli, yeni timer doğru zamanda |
| E18 | P7 | Effect callback→ability Cancel→Clear→cleanup throw | Ortak borçlar sıfır; yeniden kullanım mümkün |
| E19 | P7 | Mevcut baseline E2E regresyonları | Güncel kaynakla hepsi geçer |

### Artefakt

Her case için `case_id`, input/seed/delta, beklenen ve gerçek sonuç, callback sırası, ilgili kimlikler/handle'lar, before/after level/scrap/attribute/tag/active kayıt sayıları, hata kimliği ve pass boolean üret. Actor pointer adresini kalıcı kimlik yapma; unique ID kullan. `build/e2e-artifacts/` altında vaka gruplarını ayrı dosya/log ile tut; önceki kanıtı yanlışlıkla güncel diye kullanma. Fixture lifetime ve fault-injection aboneliklerini temizle.

## P7 — Son kabul ve teslim

### Kaynak kontrolü

1. P0 başlangıç farkıyla kendi diff'ini karşılaştır; değişen her callback sınırının çağıranlarını graph + güncel kaynakla incele.
2. Özellikle son callback'ten sonra tekrar okunan pointer/reference/iterator'ları kontrol et. E2E'nin crash vermemesi UB olmadığının tek başına kanıtı değildir.
3. R5 için effect ledger ve action resource owner'larını tek tek say: kayıt silinirken/EndActivation yapılırken sahiplik borcu kalmadığını göster. `catch (...) {}` ile susturma veya tek dış catch'e dönme yok.
4. Public normal davranış değişmiş mi, kimlik/tag/registry kaynağı çoğalmış mı, yeni cache başka bir canonical veriyi kopyalıyor mu kontrol et.
5. Güncel dokümanı yalnız yeni sözleşme kadar güncelle; eski denetimi tarihsel bırak. Tam loadout tablosu çoğaltma.

### Komutlar

Mevcut build dizini/geçerli hedefleri P0'da doğrula. PowerShell'de MSVC x64 ortamı için mevcut çalışan biçim:

```powershell
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul && set VSLANG=1033&& cmake --build build --target LightYearsGame LightYearsGasLiteTests LightYearsContinuousBeamWallE2ETests --parallel 8'
ctest --test-dir build -N -R 'E2E$'
ctest --test-dir build -R 'E2E$' --output-on-failure --output-log build/e2e-artifacts/luna-lifecycle-suite.log
ctest --test-dir build -R '^LightYearsAuditFixesE2E$' --repeat until-fail:3 --output-on-failure --output-log build/e2e-artifacts/luna-lifecycle-repeat.log
ctest --test-dir build -R '^LightYearsTimerManagerSceneE2E$' --repeat until-fail:3 --output-on-failure --output-log build/e2e-artifacts/luna-timer-repeat.log
```

Yeni E2E adı eklenirse -N çıktısında gerçekten listelendiğini ve suite'e girdiğini doğrula. Unit test çalıştıran filtresiz ctest kullanma. Hedefler değişmişse komutu kaynakla düzeltip gerçek komutu raporla; var olmayan test için “passed” yazma. Build test değildir. Sanitizer mevcut altyapıyla mümkünse ayrı E2E koşusu önerilir; çalıştırılmadıysa açıkça belirt, kurulmamış sanitizer'ı varmış sayma.

### Teslim şablonu

Uygulama kabul belgesi şunları içermeli:

- P0 başlangıç revision/diff özeti ve değiştirilen dosyalar.
- Her P paketi: yapılan değişiklik, owner, korunmuş invariant, değişen exceptional semantics.
- R1–R8 için `kapalı / kısmi / engelli` ve kaynak+E case bağlantısı.
- E01–E19 gerçek sonuç/artefaktı; koşulmayanlar açık. Önceki 6/6 sonucu yeni kanıt yerine kullanma.
- Exact build/E2E komutu, exit code, repeat sonuçları; ilk başarısız diagnostic ve yapılan düzeltme.
- Yeni hata üretmemek için kontrol edilen ilişkili tüketiciler; doğrulanamayan kapsam.
- Kalan riskler. “Tüm proje hatasız”, “sıfır hata” veya yalnız build'e dayanarak “kökten tamam” deme.

## 6. Luna'ya verilecek kısa başlangıç talimatı

> D:\LightYears\docs\LUNA_IMPLEMENTATION_PLAN_2026-09-26.md belgesini tamamen oku ve güncel kaynak üzerinde P0'dan P7'ye sırayla uygula. Önce köken denetimini oku: yedi eski mekanizma, R5'te Codex'in eklediği yeni failure-state ve R2'de geri çekilen rehash iddiası var. Paket öncesi change contract yaz; owner ve callback bağlantılarını doğrulamadan kod değiştirme. Dirty işi koru. Null Pulse/wave/encounter/basit AI, balance/slot/JSON kapsam dışı. Unit test yok; gerçek runtime E2E ve artefakt zorunlu. İlk hata korunurken tüm bağımsız cleanup borçları kapanmalı. Sonucu docs/LUNA_IMPLEMENTATION_ACCEPTANCE_2026-09-26.md içinde kaynak ve güncel kanıtla teslim et. Bir kapı başarısızken tamamlandı deme; normal gameplay/public contract çatışması varsa varsayım yapmadan bildir. Başka worker veya Flash başlatma.
