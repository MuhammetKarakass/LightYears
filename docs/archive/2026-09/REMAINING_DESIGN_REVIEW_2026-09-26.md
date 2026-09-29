# Kalan tasarım ve kaynak-kod riskleri — 2026-09-26

**Köken denetimi düzeltmesi:** [Eski kaynak → değişiklik → güncel davranış denetimi](FINDING_PROVENANCE_AUDIT_2026-09-26.md) bu raporu sınırlar. R1/R2/R3/R4/R6/R7/R8'in mekanizmaları eski commit'te de vardır. R5 eski cleanup eksikliği ile benim yamamın eklediği yeni başarısızlık sonucunu birlikte içerir. R2 rehash gerekçesi yanlıştı ve kaldırıldı. Sekiz başlık, runtime'da doğrulanmış sekiz oyun hatası değildir.

## Hüküm ve kanıt sınırı

Kalan açıklar var. Önceki activation/loadout/Nano düzeltmeleri bütün callback yaşam döngüsü problemlerini kapatmıyor. Aşağıdaki sekiz başlık güncel üretim kodundaki sıralama ve sahiplik üzerinden çıkarıldı; bu tur runtime yeniden üretimi, build, test veya sanitizer çalıştırılmadı. Koşullu kod yolu, normal oyunda gözlenmiş çökme anlamına gelmez. Eski E2E sonuçları bu bulguların karşı kanıtı değildir.

Kapsam dışı: Null Pulse, wave/encounter, basit AI. Graph keşfi kullanıldı; indeksin bazı sembol/satırları güncel olmadığından kanıt için çalışma ağacındaki kaynak okundu. Önceden var olan dirty değişiklikler korundu.

## Bulgular

### R1 — P1: Effect uygulanırken kendi kaydı silinebiliyor

Kaynaklar: [ApplyEffect](../SpaceAbilitySystem/include/effects/GameplayEffectRuntimeSystem.h), özellikle 129–132, 191–206, 241–245; [modifier binding](../SpaceAbilitySystem/src/effects/GameplayEffectBindings.cpp), 25–40; [component remove](../SpaceAbilitySystem/src/AbilitySystemComponent.cpp), 374–390.

`ApplyGameplayEffectModifiers` effect'in spec/state referanslarını tutuyor. `AttributeSystem::AddModifier` attribute bildirimini handle geri dönmeden yayımlıyor. Dinleyici uygulanmakta olan effect'i ID/snapshot üzerinden bulup kaldırırsa `RemoveEffectInternal` list düğümünü hemen siliyor. Binding geri döndüğünde `state.appliedModifierHandles.push_back(handle)` artık yaşamayan kayda yazıyor. Yeni modifier handle'ı removal snapshot'ına henüz kaydedilmediğinden modifier ayrıca sahipsiz kalabilir. Refresh kolunda da callback sonrası kontrol, tags/modifiers işlemlerinin tümü bittikten sonra geliyor; geç kalıyor.

Component `RunOperation` yalnız toplu Clear'ı erteliyor; tek effect kaldırma yolunu rezerve etmiyor. `std::list` ekleme sırasında adres korur, silinme sırasında korumaz. Koşul: attribute/effect callback'inin mevcut effect'i kaldırması. Bunu yapan shipped dinleyici bu tur doğrulanmadı; API yolu açık.

Tasarım yönü: effect başına apply/refresh/remove işlem sahipliği; dış bildirimden önce modifier sahipliğini commit etme; callback boyunca silinmeyi erteleme veya güvenli işlem kaydı kullanma. Yalnız son satıra Find kontrolü eklemek binding içindeki yazmayı korumaz.

### R2 — P1: Attribute modifier kaldırma callback boyunca iterator saklıyor

Kaynak: [AttributeSystem::RemoveModifier](../SpaceAbilitySystem/src/attributes/AttributeSystem.cpp), 205–220.

Reverse-map iterator'ı bulunuyor, modifier siliniyor, `Recalculate` ile kullanıcı callback'i çalışıyor; ardından eski iterator ile reverse-map erase yapılıyor. Callback aynı handle'ı yeniden kaldırırsa iç çağrı reverse-map kaydını siler; dış çağrı geçersiz iterator kullanır. Attribute Clear da iterator'ı geçersizleştirir. **Düzeltme:** Bu reverse-map `ly::Map`, yani `std::map` türündedir; önceki “modifier ekleme ile rehash” gerekçesi yanlıştı. Callback exception'ında reverse-map temizliği hiç yapılmaz.

Koşul: attribute-change callback'inin bu map'i değiştirmesi; sıradan callbacksiz remove kusurlu sayılmıyor. Tasarım yönü: reverse-index ve modifier kaydını bildirimden önce tutarlı duruma getirmek; callback sonrasına iterator taşımamak. Effect katmanındaki koruma bunu tek başına çözmez; AttributeSystem doğrudan da kullanılıyor.

### R3 — P1: Tick sırasındaki Cancel/SetLevel, yürütülen action'ı yok edebiliyor

Kaynaklar: [GameplayAbilityInstance](../SpaceAbilitySystem/include/abilities/GameplayAbilityInstance.h), Tick/Cancel/SetLevel; [execution döngüsü](../SpaceAbilitySystem/include/abilities/AbilityExecution.h), 48–84; [GameAbility](../LightYearsGame/src/gameplay/ability/GameAbility.cpp), 405–445; [executor](../LightYearsGame/src/gameplay/ability/GameAbilityActionExecutor.cpp), TickAction.

Aktivasyon esnasında Cancel erteleniyor, ancak TickExecution esnasında hemen EndAbility çalışıyor. Action callback'i aynı ability'yi Cancel eder veya level değiştirirse EndExecution `execution.actions` vektörünü temizliyor. Dış Tick hâlâ bu vektörü dolaşıyor. Repeated action kolu `ExecuteAction` dönüşünde `action.spec->interval` ve `repeated` durumuna tekrar erişiyor; weapon kolu da SimulateActiveFire sonrasında action state'ine yazıyor. Böylece hem iterator hem action-state ömrü ihlal edilebilir. GameAbility ayrıca action yürütmesinden sonra aktiflik tekrar kontrolü olmadan behavior Tick çağırıyor.

Instance operation guard instance'ın silinmesini engeller, kendi action vektörünün temizlenmesini engellemez. Koşul: Tick action/hasar/event callback'inden aynı instance'a Cancel veya SetLevel. Mevcut oyunda bu tam callback zinciri yeniden üretilmedi.

Tasarım yönü: activation'a özel bayrak yerine execution kapsamını da kapsayan mutasyon sınırı; iptal/yeniden konfigürasyonu güvenli sınırda uygulama ve iptal sonrası kalan action/behavior işini durdurma.

### R4 — P2: Scoped konfigürasyon yenilemesi aktif action tanımlarının ömrünü ihlal ediyor

Kaynaklar: [RefreshScopedAbilityRules](../LightYearsGame/src/gameplay/ability/LightYearsAbilitySystemComponent.cpp), 411–449 ve 508; [RefreshScopedConfiguration/RebuildDefinitionForLevel](../LightYearsGame/src/gameplay/ability/GameAbility.cpp), 624 ve 700; [ActiveAbilityAction](../SpaceAbilitySystem/include/abilities/AbilityExecution.h), 100.

Action'lar `mDefinition.actions` elemanlarına raw spec pointer saklıyor. Scoped rule ekleme/kaldırma aktif ability'yi sonlandırmadan definition'ı base kopyayla yeniden atıyor ve progression action'larını ekliyor. Silinen progression elemanları veya vektör yeniden tahsisi saklanan pointer'ı geçersizleştirebilir. Yeniden tahsis oluşmasa bile çalışan action listesi yeni tanımla yeniden kurulmadığından yeni/kaldırılmış action davranışı tutarsız kalır.

Bu API'nin shipped üretim çağıranı bulunmadı; geliştirme/genişletme riski olarak sınıflandırıldı. Her scope değişiminin mutlaka pointer bozduğu iddia edilmiyor. Tasarım yönü: aktif execution için değişmez tanım snapshot'ı veya açık durdur/yeniden-kur sözleşmesi. R3'ün yeniden giriş sorunundan ayrı olarak, kareler arasında yapılan scope değişimi de etkilenir.

### R5 — P1: Cleanup exception'ı kalan kaynakların bırakılmasını atlıyor

Köken notu: İç cleanup döngüsünün ilk throw'da kesilmesi eskiden de vardı. Ancak yeni dış catch/continue mantığı, eksik temizliğe rağmen effect kaydını siliyor ve ability end state'ini tamamlıyor. Bu yeni başarısızlık sonucu benim düzeltmemle eklendi; eski sürümde effect removal-in-progress'te/ability aktif durumda takılabiliyordu. Ayrıntı köken denetimindedir.

Kaynaklar: [effect cleanup](../SpaceAbilitySystem/include/effects/GameplayEffectRuntimeSystem.h), 635–648; [binding döngüleri](../SpaceAbilitySystem/src/effects/GameplayEffectBindings.cpp), 43–53 ve RemoveGameplayEffectTags; [action end](../SpaceAbilitySystem/include/abilities/AbilityExecution.h), 66–84.

Effect removal bütün modifier döngüsünü tek try/catch ile sarıyor. İlk modifier kaldırılırken attribute callback'i throw ederse kalan modifier'lar kaldırılmıyor; buna rağmen effect düğümü siliniyor. Kalan buff/debuff modifier'ları owner üzerinde kalır ve sahip effect artık yoktur. Tag döngüsü de aynı yapıda. Tam component Clear sonunda attributes/tags temizlendiği için bu sonuç özellikle tek effect kaldırma yolunda önemlidir.

Action cleanup da ilk `endActive` exception'ında sonraki action'ları ve OnEnd adımlarını atlıyor. Dış EndAbility diğer üst düzey adımları denese bile bu iç döngüye devam etmiyor. Kaynakta doğrulanmış exception yolu; shipped callback'in normal oyunda throw ettiği kanıtlanmadı.

Tasarım yönü: cleanup borcunu tek tek kaynaklarda tutmak; bir kaynak hata verse de kalanları bırakmak, ardından ilk hatayı yeniden fırlatmak. Dış katmandaki catch, iç koleksiyonun eksiksiz temizlendiği anlamına gelmez.

### R6 — P1: Oyuncu upgrade işlemi component sınırından sonra eski ability pointer'ını okuyor

Kaynaklar: [Player::TryPurchaseAbilityLevel](../LightYearsGame/src/player/Player.cpp), 165–175; [LevelUpAbility/ExecuteOperation](../SpaceAbilitySystem/src/AbilitySystemComponent.cpp), 106–112 ve 470–485.

Player önce `GameAbility*` alıyor, `LevelUpAbility(slot)` çağırıyor, sonra scrap düşüp `ability->GetLevel()` okuyor. Level-change bildirimi Clear isterse Clear component'in dış işlem çıkışında gerçekten çalışır. LevelUp sonucu true kalabilir, ama Player'a dönüldüğünde eski ability silinmiştir. Sonraki dereference geçersizdir. Level-change callback'inin exception vermesi de seviye commit edilmişken ödeme/kayıt adımlarını atlayabilir.

Koşul: upgrade bildiriminin Clear/exception üretmesi; böyle bir shipped dinleyici doğrulanmadı. Bu, loadout live-view düzeltmesinin kapsamadığı başka bir tüketici. Tasarım yönü: upgrade işleminin değer sonuçları ve ödeme/purchased-level commit'i için tek tutarlılık sınırı; mutation öncesi alınan pointer'ı callback'li API dönüşünden sonra kullanmamak.

### R7 — P2: Combat damage context'i stack/exception güvenli değil

Kaynak: [CombatRuntime](../LightYearsGame/src/gameplay/combat/CombatRuntime.cpp), 210–248 ve 314–340.

ProcessIncomingDamage geçici context adresini üyeye yazıyor; normal dönüşte nullptr yapıyor, scope guard ile eski değeri geri yüklemiyor. İç effect işleme exception verirse üye context ömrü bittikten sonra dangling kalabilir. QueueEffectEvent context'i kopyalamadan event'e bağlıyor; dispatch öncesi exception olursa kuyrukta da ödünç context kalabilir. İç içe damage işlenmesinde iç çağrı dış context'i geri yüklemek yerine nullptr yapar.

Koşul: işlem exception'ı veya aynı CombatRuntime'a nested damage. Tam shipped zincir bu tur kanıtlanmadı. Tasarım yönü: RAII ile önceki context'i geri yükleme; kuyrukta stack context yerine gerekli verilerin sahibi olan snapshot; başarısız işlemde kuyruk sınırının açık olması.

### R8 — P2: Tek seferlik timer hata sonrasında yeniden çalışabiliyor

Kaynak: [Timer::TickTimer](../LightYearsEngine/src/framework/TimerManager.cpp), 203–206; UpdateTimer catch yolu.

One-shot callback, `SetExpired()` çağrısından önce çalışıyor. Callback throw ederse expiry yapılmaz. UpdateTimer catch'i flush edip hatayı yeniden fırlatır; timer expired olmadığı için tutulur. Üst katman hatayı yakalayıp oyunu sürdürürse sonraki update aynı işi yeniden çalıştırır. Callback'in throw etmediği normal akış etkilenmez; yakalanmayan exception ile süreç sonlanırsa tekrar yoktur.

Tasarım yönü: one-shot tüketimini callback'ten önce commit etmek; yeniden deneme gerekiyorsa bunun ayrı ve açık politika olması. Ayrıca UpdateTimer yeniden girişe kapalı değil; bunun shipped çağıranı doğrulanmadığından ayrı kesin hata sayısına eklenmedi.

## Ortak kök nedenler ve öncelik

1. **Callback boyunca ödünç veri taşıma:** R1/R2/R3/R4/R6. Component yaşamı, iç kayıt ve tanım yaşamı, dış tüketicinin işlem sonrası pointer yaşamı ayrı garantilerdir. Tek guard bunların hepsini sağlamıyor.
2. **Kısmi cleanup'ın tamamlanmış sayılması:** R5 ve R8. Exception dışarı taşınırken kaynak/one-shot sahipliği kesin kapanmalı.
3. **Geçici olay verisinin sahipliği:** R7. Context'in stack ömrü, kuyruk ömrü ve nested işlemler uyumlu değil.

Öncelik R1/R2/R3/R5/R6; sonra R4/R7/R8. Bunlar uygulanmış çözüm değil, kaynak incelemesinin tasarım gereksinimleridir. Balance veya public API değişikliği bu raporla yetkilendirilmiş sayılmaz.

## İnceleme kapsamı

Derin okunan bağlantılar: SAS component/instance/action/effect/attribute, GameAbility ve action executor, scoped rules, CombatRuntime, timer, lifecycle dispatcher, Player upgrade; weapon action/runtime ve continuous beam bağlantıları. World tick, portal runtime actor, PlayerManager, HUD bağlantı/destructor, weapon catalog çözümleme ve reflection direct-world guard da odaklı kontrol edildi; bu alt kesitlerde ayrıca kanıtlanmış yeni bulgu eklenmedi.

LifecycleDispatcher snapshot semantiği header'da açıkça tanımlanıyor; snapshot sonrası unregister'ın anında etkili olmamasını tek başına bug saymadım. Alıcı ömrü için RAII politikasıyla uyum ayrıca tasarım incelemesi gerektirir; bu tur somut dangling alıcı zinciri doğrulanmadı.

**Bu rapor bütün repository'nin satır satır tamamlanmış denetimi değildir.** Tüm ability aileleri/evolve kombinasyonları, bütün weapon feature kombinasyonları, tüm content parser/validation yolları, renderer/audio/assets/input ve tam portal/progression akışları bu tur eksiksiz incelenmedi. Bu yüzden “başka hata yok” veya “tüm proje temiz” hükmü verilemez. Yukarıdaki liste doğrulanmış kod yollarının konsolide sonucudur; kapsam dışı sistemler hakkında hüküm içermez.

Runtime kaynakları değiştirilmedi. Bu tur kanıtı yalnız kaynak incelemesidir; build/test sonucu yoktur.
