# Luna değişiklikleri sonrası kaynak incelemesi — 2026-09-27

İncelenen HEAD: `6a17ea7`; devam değişikliklerinin karşılaştırma tabanı: `5bffe04`. Başlangıç çalışma ağacı temizdi. Kullanıcının bu tur talebi analizdir; runtime/test kaynakları değiştirilmedi ve build/test çalıştırılmadı. Işın uç noktası inceleme dışında tutuldu.

**Sonuç: ışın dışında da bütün risklerin arındırıldığı söylenemez. Kaynak akışında üç koşullu kusur doğrulandı: ikisi yeni hata durumu, biri eski yolun eksik kapatılması.** Bunlar gözlenmiş shipped oyun çökmesi olarak sunulmaz. Önceki E2E geçişleri geçerlidir, fakat aşağıdaki callback varyantlarını kapsamıyor.

## İnceleme sözleşmesi

Amaç P1–P6 düzeltmelerinin owner/caller bağlantılarını ve değişikliklerin yeni hata üretip üretmediğini incelemek. Şef rapor/hafıza/Git sahibi; dört Luna max inceleme rolü salt okunur: ability execution/invocation/weapon cleanup, Player purchase/scoped refresh, component/effect/attribute cleanup, damage/timer. Graph keşfi ardından güncel kaynak okundu; graph eski isim/gövde gösterdiğinde disk kaynakları ve Git karşılaştırması esas alındı. İzinli yazma alanı bu rapor, acceptance yönlendirmesi ve companion notlarıdır. Runtime değişikliği yok.

## F1 — Yeni: etkisiz scoped-rule işlemi sonsuz yenileme üretebilir (P2)

Kaynak: [RemoveScopedAbilityRule](../LightYearsGame/src/gameplay/ability/LightYearsAbilitySystemComponent.cpp), satır 468–484; `ClearScopedAbilityRules`, 497–505; `RefreshScopedAbilityRules`, 591; `FlushDeferredScopedAbilityRules`, 627–635. Bildirim sahibi [AbilitySystemComponent.cpp](../SpaceAbilitySystem/src/AbilitySystemComponent.cpp), 190–193.

Somut tetikleme: en az bir granted ability varken `onAbilityChanged` dinleyicisi daha önce kaldırılmış/geçersiz bir rule handle için `RemoveScopedAbilityRule` çağırır. Yenileme içinde `defer=true` olur. Kod önce rule listesini kopyalayıp `mHasDeferredScopedAbilityRules=true` yapar; sonra handle bulunamadığı için false döner. Gerçek değişiklik olmadığı halde pending durum kalır. Flush döngüsü aynı listeyi tekrar uygular, yeniden bildirim yollar, dinleyici aynı etkisiz isteği üretir. Döngü sonlanmaz. Boş listeyi callback içinde `ClearScopedAbilityRules` ile temizleme de aynı sınıftadır.

`5bffe04` sürümünde bulunmayan handle/boş liste doğrudan döner ve yenileme işaretlemezdi; bu nedenle **bu regresyon yeni erteleme koduyla eklendi**. Mevcut üretim kaynaklarında bu silme metodunu kullanan söz konusu dinleyici saptanmadı; public API'nin callback kullanımında ortaya çıkabilen donma riskidir, bugün shipped oyunda gerçekleştiği iddia edilmez.

Düzeltme yönü: deferred dirty durumunu yalnız gerçek liste mutasyonunda üret; etkisiz remove/clear bildirim veya yeniden drain oluşturmasın. Gerekli kanıt: callback içinde stale-handle remove ve empty-clear; çağrı sonlanmalı ve fazladan sonsuz bildirim olmamalı. Mevcut E10 structural snapshot senaryosu bu varyant değildir.

## F2 — Kalan: Clear sonrası aynı vuruşun statü döngüsü devam edebilir (P2)

Kaynak: [CombatRuntime.cpp](../LightYearsGame/src/gameplay/combat/CombatRuntime.cpp), 321–330; [DamageTypeSystem.cpp](../LightYearsGame/src/gameplay/damage/DamageTypeSystem.cpp), 79–92 ve 379–429; [AbilitySystemComponent.cpp](../SpaceAbilitySystem/src/AbilitySystemComponent.cpp), effect Apply girişleri ve 527/532–544.

Somut tetikleme: tek vuruş birden fazla statü stack'i veya birden fazla hasar statüsü uygular. İlk `ApplyGameplayEffect` içindeki bir callback hedef `CombatRuntime.Clear()` çağırır ve exception atmaz. ASC bu tek Apply operasyonundan çıkarken Clear'ı tamamlar ve `mClearRequested` durumunu sıfırlar. Dış `ApplyCappedStackEffect` döngüsü ikinci stack'e geçer; sonraki Apply artık pending-clear görmez ve temizlenmiş hedefe aynı eski vuruştan yeniden effect ekleyebilir. Farklı statü dalları arasında da aynı sorun oluşabilir.

CombatRuntime generation kontrolü bütün `ApplyStatusEffects` dönüşünden **sonra** yapılır. Sonraki source event'lerini durdurabilir, ancak helper'ın çoktan yeniden eklediği effect'i geri alamaz. Bütün status uygulamasını kapsayan dış component operasyonu veya her alt adım arasında generation kontrolü yoktur. ASC sınırı bağımsız incelemeyle de doğrulandı.

Bu helper döngüsü ve per-call operation sınırı `5bffe04` içinde de vardı. **Yeni üretilmiş hata diye sınıflandırılmadı; P5 devam-iptal kapsamındaki kapanmamış eski yoldur.** Yeni frame/context ömrü düzeltmesini geçersiz kılmaz, ancak Clear sonrası devam için kapsamlı kapanma iddiasını sınırlar. Güncel E15, barrier break callback'inde Clear+throw ile statü aşamasına ulaşmadan çıkar; burada gereken Clear-without-throw ve statü uygulamasının içindeki callback varyantı yoktur.

Düzeltme yönü: hasar epoch/iptal sınırını status helper'ın her effect uygulamasına taşı veya bütün helper'ı uygun ortak operasyon kapsamında tut ve devam koşulunu kontrol et. Keyfi gerçekleşmiş hasarı rollback etmek bu önerinin parçası değildir.

## F3 — Yeni entegrasyon açığı: aktif silahın başarısız EndFire temizliği sonraki Tick'te denenmiyor (P2)

Kaynak: [PrimaryWeaponExecutionSystem.cpp](../LightYearsGame/src/gameplay/weapon/PrimaryWeaponExecutionSystem.cpp), 493–496, 520–529, 646–665 ve 726–773; [FireWeaponActionRuntime.cpp](../LightYearsGame/src/gameplay/ability/actions/FireWeaponActionRuntime.cpp), 159–162 ve 342–355; [GameplayAbilityInstance.h](../SpaceAbilitySystem/include/abilities/GameplayAbilityInstance.h), 54–56; [HeatWeaponFeatureHandler.cpp](../LightYearsGame/src/gameplay/weapon/features/HeatWeaponFeatureHandler.cpp), 208–218.

Somut tetikleme: basılı tutulan, aktif sürekli silah aşırı ısınarak cooldown ister. SimulateActiveFire cooldown talebini tüketir ve EndFire çağırır. Handler/visual temizliği geçici exception atarsa yeni EndFire önce `isFiring=false` yapmış, başarısız borcu pending bayraklarında tutmuştur. Çağrı exception ile çıktığından `lifecycleInterrupted` sonucu dış action'a ulaşmaz; `lifecycleStarted=true` kalır ve cooldown ataması da atlanır.

Sonraki Tick'te EnsureLifecycle bu true bayrak nedeniyle yeniden BeginFire/cleanup yapmaz. TickFire `isFiring=false` diye döner; tüketilmiş cooldown talebi de EndFire'ı yeniden çağırmaz. Instance hâlâ aktif olduğundan inactive-only RetryPendingCleanup yolu devreye girmez. Sonuç: hata yakalanıp oyun sürdürülürse ateş, input bırakılana/ability Cancel edilene kadar durmuş kalabilir; failed visual cleanup borcu da bu süre boyunca korunur fakat ilerlemez. Kaynak tamamen kaybolmuş değildir, ancak aktif Tick toparlanması eksiktir.

`812a0d0`, `isFiring=false` atamasını cleanup callback'lerinden önceye aldı ve pending cleanup durumunu ekledi. `5bffe04` bu atamayı callback'lerden sonra yapıyordu. Eski exception yolu da kusursuz değildi; burada **yeni pending-cleanup modelinin aktif overheat yoluna bağlanmaması ve yeni durmuş state** raporlanıyor. Shipped heat feature bu EndFire çağrı yolunu kullanır, fakat gerçek shipped handler'ın böyle exception attığı gözlenmiş değildir. E11 Clear/EndExecution sırasında fault enjekte eder; aktif overheat sırasında fault ve sonraki held Tick bu kanıtın dışında kalır.

Düzeltme yönü: pending EndFire borcunu aktif simülasyonun açık bir toparlanma sınırında drain et; lifecycle/cooldown kesintisini exception olsa da kaybetme. Sonraki Tick için gereken durum korunmalı, aynı cleanup ve bildirim gereksiz tekrarlanmamalı.

## Doğrulanan olumlu noktalar ve sınırlar

- Attribute modifier sahipliği ve reverse map güncellemesi callback öncesi tutarlı; effect operation/deferred removal ve retained cleanup yollarında bu incelemede ayrı somut kusur bulunmadı.
- Ability/component Clear gerçek cleanup borcu sürerken sahipleri ve bağımlılıkları tutuyor; observer-only hata bağımsız temizliği kesmiyor. Bu, önceki uygulama sırasında bulunup `cdde81a` ile giderilen entegrasyon kusurunun mevcut kaynağıdır.
- Purchase sink, typed SetLevel prepare/commit, primary-weapon configuration commit ve no-sink level yolu incelendi; F1 dışında ayrı somut purchase regresyonu bulunmadı.
- Action snapshot, callback-depth/deferred end, invocation ownership ve Return Protocol cleanup owner yolları incelendi; F3 dışındaki şüpheli OnEnd tekrar beklentisi bulguya dönüştürülmedi, çünkü planda lifecycle bildirimi tek seferliktir.

- Nested damage frame/RAII geri yükleme, frame-local event batch ve TimerManager one-shot/nested Update/listener lifetime diff'lerinde bağımsız ek somut kusur bulunmadı.

Bu tur yalnız kaynak incelemesidir; yeni bulgular için E2E çalıştırılmadı. “Somut kusur bulunmadı” incelenen yollarla sınırlıdır; gelecekte hiç hata oluşmayacağı garantisi değildir. Null Pulse, wave/encounter, basit AI ve bağımsız yeni özellikler bu karşılaştırmaya dahil değildir. Önceki 101 assertion ve 3/3 tekrar, bu üç varyantın kanıtı olarak kullanılmaz.
