# Lifecycle paketi kabul incelemesi — 2026-09-26

> Sonraki durum: Aşağıdaki üç tespit [kök sözleşme düzeltmesiyle](LIFECYCLE_ROOT_FIXES_2026-09-26.md) ele alındı; güncel kaynak ve E2E kanıtı o rapordadır. Bu belge düzeltme öncesi kabul denetimini korur.

Sonuç: **kısmen**. Önceki dört bulgunun doğrudan tetikleme yollarındaki korumalar kaynakta mevcut; koşulsuz tamamlanma kabulü uygun değil. Bu tur üretim/test kodu değiştirilmedi, build/test çalıştırılmadı. Önceki E2E sonuçları tarihsel kanıttır. Kapsam son paketin owner/çağıran/callback bağlantılarıdır; yeni bir bütün-repo denetimi değildir. Null Pulse, wave/encounter ve basit AI incelenmedi.

## Doğrulanan kazanımlar

- Component operation derinliği boyunca Clear erteleniyor; instance execution derinliği çalışan instance sırasında grant/rebind/remove'u reddediyor. Tick/input/level/event ziyaretlerinde runtime rezervasyonu var. Echo, bildirimleri kapalı olsa da execution sınırını koruyor.
- Ability, game state, effects, tags, attributes ve Combat owner completion cleanup aşamaları ilk exception sonrasında da deneniyor. Başlamış/aktif instance'ın EndContent hatası EndExecution ve state kapanışını atlatmıyor.
- Nano Tick callback üzerinden vector büyüdükten sonra eski referansı kullanmıyor; id ile yeniden buluyor, refresh revision kontrolü sayaçları koruyor.
- Actor::SetActorRotation artık World spatial kaydını yeniliyor.

## Kalan bulgular

### 1. P1 — Aktivasyon tamamlanmadan exception: davranış ve instance durumu ayrışıyor

[GameplayAbilityInstance.h](../SpaceAbilitySystem/include/abilities/GameplayAbilityInstance.h), satır 97–102: ActivateContent, BeginActivation'dan önce çalışıyor. [GameAbility.cpp](../LightYearsGame/src/gameplay/ability/GameAbility.cpp), satır 368–374: behavior Activate başarılı olduktan sonra lifecycle event yayınlanıyor. Bu event dinleyicisi exception fırlatırsa instance hâlâ inactive; Cancel (satır 128–136) ve EndAbility inactive instance'ın EndContent'ini çalıştırmıyor.

Somut shipped behavior: [ReturnProtocolAbility.cpp](../LightYearsGame/src/gameplay/ability/returnProtocol/ReturnProtocolAbility.cpp) satır 101–118, önce mActive=true, tag ve visual actor oluşturuyor, ardından Started event'i yayınlıyor. Started veya sonraki lifecycle Activated dinleyicisi hata verirse runtime inactive iken behavior aktif kalabilir; tekrar Activate mActive nedeniyle reddedilir. Sonradan Clear kayıt token'ını destructor ile kaldırır ama End çalışmadığından visual Destroy edilmez. [Visual Tick](../LightYearsGame/src/gameplay/ability/returnProtocol/ReturnProtocolVisualActor.cpp) yalnız owner yok/pending ise kendini siler; yaşayan owner üzerinde görsel kalır.

Koşul: ilgili event dinleyicisi exception fırlatmalı. Mevcut shipped dinleyicilerde bu exception tetikleyicisi doğrulanmadı; kaynak yolu kesin, oyun sırasında gerçekleşme iddiası yok. Önceki onAbilityActivated fault senaryosu BeginActivation sonrasıdır ve bu boşluğu kapatmaz. Bu, keyfi behavior içi rollback talebi değil; başarılı behavior Activate ile ortak runtime aktivasyon commit'i arasındaki yayınlama sınırıdır.

Tasarım gereksinimi: başlayan content'in cleanup sorumluluğu aktivasyon tamamlanmadan da temsil edilmeli; event exception'ında End/abort yolu çalışmalı. Sadece instance'ı erken active yapmak diğer lifecycle semantiğini incelemeden güvenli çözüm sayılmaz.

### 2. P2 — İç içe event/loadout işleminde ertelenmiş Clear sonrası hayalet loadout

[AbilitySystemComponent.h](../SpaceAbilitySystem/include/AbilitySystemComponent.h), satır 224–232 dış HandleGameplayEvent için operation açıyor. Broadcast dinleyicisinden EquipAbility yapılabilir; bu noktada instance execution derinliği sıfırdır. Grant callback'i Clear isterse Clear dış event dönüşünü bekler. İç Grant satır 83–84'te handle'ı hâlâ bulur ve başarılı döner; [AbilityLoadoutManager.cpp](../LightYearsGame/src/gameplay/ability/loadout/AbilityLoadoutManager.cpp) satır 125–126 henüz temizlenmemiş runtime'ı loadout'a kopyalayıp true döner. Dış event bitince runtime temizlenir.

Manager constructor'ında clear aboneliği yok; [GetLoadout](../LightYearsGame/include/gameplay/ability/loadout/AbilityLoadoutManager.h) doğrudan önbelleği döndürüyor. Sonuç: runtime boşken loadout eski ability'yi gösterebilir. Düz/top-level Equip→Clear yolu düzeltilmiş; nested operation yolu aynı garantiyi taşımıyor.

Koşul: dış event callback'i equip, grant callback'i Clear çağırmalı. Shipped listener kombinasyonu doğrulanmadı; normal public API'lerle erişilen koşullu açık. Ham mutable storage gerekmiyor.

Tasarım gereksinimi: loadout gözlemi canonical runtime'ın gerçek commit/clear sınırına bağlanmalı veya nested mutation politikası açıkça reddetmeli. Yalnız handle varlığı kontrolü yeterli değil.

### 3. P2 — Nano normal Tick artık O(N²)

[NanoPlagueControllerActor.cpp](../LightYearsGame/src/gameplay/ability/nanoPlague/NanoPlagueControllerActor.cpp) satır 164–168 her id için FindInfection çağırıyor; satır 213–217 her seferinde vector başından doğrusal arıyor. Enfeksiyonlar değişmese ve o kare hiç hasar uygulanmasa bile toplam N(N+1)/2 kimlik karşılaştırması var. Damage sonrası yeniden aramalar ve silmeler ek maliyet. Güvenlik düzeltmesinin getirdiği yapısal regresyondur; FPS etkisi ölçülmedi.

Tasarım gereksinimi: callback güvenli id/revision yaklaşımını koruyarak id→kayıt erişimini indekslemek veya kararlı kayıt depolaması kullanmak; sıra/timing davranışını değiştirmeden doğrulamak.

## Kabul sınırı

Doğrudan dört eski senaryo için iyileşme var; geniş “callback/exception güvenli, tutarlı ve uygun tasarımla tamamlandı” hükmü yukarıdaki iki doğruluk açığı kapanmadan verilmemeli. Performans maddesi bellek güvenliği açığı değildir. Bu bulgular statik kaynak akışıyla doğrulandı; yeni runtime reproducer veya ölçüm yapılmadı.
