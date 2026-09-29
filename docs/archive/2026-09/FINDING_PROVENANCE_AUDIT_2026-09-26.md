# Sekiz bulgunun köken denetimi — 2026-09-26

## Sonuç

Önceki rapordaki sekiz başlığın yedisinin temel hata/risk mekanizması eski Git kaydında da bulunuyor. R5 karma: cleanup döngülerinin ilk hatada kesilmesi eski, fakat eksik temizliğe rağmen effect kaydını silme ve ability'yi bitmiş sayma davranışı benim düzeltme paketlerimdeki değişikliklerle oluşmuş. Dolayısıyla bütününü “eskiden vardı” diye savunmak doğru değil. Ayrıca R2'deki **rehash iddiası yanlış**; geri çekildi.

Bu, sekizinin de normal oyunda tetiklendiği anlamına gelmez. Kaynakta koşullu hata yolları ile shipped tetikleyici/runtime kanıtı ayrı tutulmalı. Önceki P1/P2 etiketleri tetiklenince olası etkiyi anlatabilir; mevcut oyundaki sıklık veya acil ürün önceliği kanıtı değildir.

## Karşılaştırma temeli ve sahiplik sınırı

- Eski kaynak: `ce14e9308c9d9825c7648c7217661a3d3fbbaafe`, 2026-09-22 21:58:36 +0300, `Test Waves`. Bu commit adı kapsam dışı wave kodunun incelendiği anlamına gelmez; yalnız ilgili sekiz bulgunun dosyaları okundu.
- Güncel kaynak: bu denetim sırasındaki commit edilmemiş çalışma ağacı.
- Kullanılan işlemler: `git show HEAD:<path>`, `git diff HEAD -- <paths>`, mevcut kaynak ve doğrudan çağıranların okunması. İndeks güncel kaynak satırlarıyla tam eşleşmediği için graph keşfi kaynakla kontrol edildi.
- HEAD, her yamamın hemen öncesine alınmış ayrı bir snapshot değildir. Çalışma ağacında başka işler de vardır; bütün HEAD farkını bana ait saymıyorum. Burada bana atfedilen operation/cleanup/activation değişiklikleri önceki konuşmadaki uygulamam, [lifecycle düzeltme kaydı](LIFECYCLE_FIXES_2026-09-26.md) ve [kök düzeltme kaydı](LIFECYCLE_ROOT_FIXES_2026-09-26.md) ile eşleşen değişikliklerdir. Commit edilmemiş her hunk için bağımsız yazar/timestamp kanıtı yoktur.
- Runtime/test dosyası değiştirilmedi; build/test çalıştırılmadı. Bu denetim köken ve statik çağrı sırası denetimidir, runtime yeniden üretimi değildir.

## Tek tek denetim

| ID | Eski kaynak | Benim değişikliğim / güncel fark | Güncel hüküm |
|---|---|---|---|
| R1 | ApplyEffect, AddModifier bildirimi boyunca effect spec/state referansı tutuyor; binding handle'ı bildirimden sonra kaydediyor. Tek effect removal düğümü silebiliyor. | Component operation sınırı/Clear koruması eklendi; bu sınır tek effect removal'ını ertelemiyor. Binding gövdesi değişmedi. | **Eski koşullu açık; önceki incelememde kaçırdım.** Yeni guard bunu çözmüyor. |
| R2 | RemoveModifier, Recalculate callback'inden sonra reverse-map iterator'ını siliyor. | AttributeSystem.cpp HEAD ile aynı; bu dosyada düzeltmem yok. | **Eski koşullu açık.** Aynı iterator'ın kaydının silinmesi/Clear geçerli; **ekleme→rehash iddiası yanlış.** |
| R3 | Tick execution vektörünü dolaşıyor; Cancel aktif ability'yi hemen bitiriyor; SetLevel önce End çağırıyor. Executor dönüşte action state'ine erişiyor. | Instance-operation ve activation guard eklendi; Cancel yalnız activation sırasında erteleniyor. Tick içi action yaşamı korunmadı. | **Eski koşullu açık; koruma kapsamını eksik değerlendirdim.** |
| R4 | RefreshScopedConfiguration doğrudan definition'ı yeniden kuruyor; action'lar definition vektöründeki spec adresini tutuyor. | Refresh RunInstanceOperation içine alındı; tanım/action sahipliği değişmedi. | **Eski, kullanımı henüz doğrulanmayan genişletme riski.** Scope API'sinin shipped çağıranı bulunmadı. |
| R5 | Modifier/tag/action cleanup döngüleri ilk throw'da duruyor. Effect erase/pop ve ability EndActivation'a ulaşılamıyor. | Üst düzey cleanup adımlarını catch ile devam ettirdim; iç döngüleri kaynak başına tamamlatmadım. Effect erase artık throw'a rağmen çalışıyor; ability end state'i de tamamlanıyor. | **Eski eksiklik + benim değişikliğimin yeni başarısızlık sonucu.** R5'i salt eski hata diye sınıflandıramam. |
| R6 | Player, LevelUpAbility sonrası daha önce aldığı ability pointer'ını okuyor. Eski Clear callback sırasında hemen silebiliyor; level bildirimi de önceden var. | Clear component çıkışına ertelendi, fakat Player'ın callback sonrası pointer kullanımı değişmedi. | **Eski lifetime/işlem açığı; yeni sınırı dış tüketiciye kadar tamamlamamışım.** Hata zamanı değişti, ilk kez yaratıldığı kanıtlanmadı. |
| R7 | ProcessIncomingDamage üyeye stack context yazıp normal yolda nullptr yapıyor; event kuyruğu ödünç context tutuyor. | Combat Clear/CompleteClear değişti; ProcessIncomingDamage ve Queue/Dispatch gövdeleri değişmedi. | **Eski koşullu exception/nested çağrı riski.** Günlük oyunda tetiklendiği doğrulanmadı. |
| R8 | One-shot timer callback'ten sonra expired oluyor; throw olursa expired olmuyor. | Repeating timer catch-up değişti. Callback one-shot else koluna taşındı ama callback→expiry sırası aynı kaldı. | **Eski koşullu exception/retry riski.** Yeni repeating düzeltmesinin oluşturduğu hata değil. |

### R1 — Effect apply/remove

[GameplayEffectBindings.cpp](../SpaceAbilitySystem/src/effects/GameplayEffectBindings.cpp) HEAD ile değişmemiştir. `AddModifier` callback'inden dönünce `state.appliedModifierHandles.push_back(handle)` yapılır. [ApplyEffect](../SpaceAbilitySystem/include/effects/GameplayEffectRuntimeSystem.h) ilgili normal apply/refresh gövdeleri de HEAD'de aynı sıradadır; başa mIsClearing kontrolü eklenmiştir. Collection eski sürümde de list tabanlıdır; storage türünü son düzeltmemde list'e çevirmiş değilim. Tek remove hem eski hem yeni sürümde node erase yapar.

Kanıt koşulu: callback uygulanmakta olan effect'i bulup kaldırır. Böyle bir shipped attribute dinleyicisi gösterilemedi; keyfi callback desteği açısından açık yol olarak kalır. “Şu anda oyunda rastgele effect çökmesi yaşanıyor” sonucu çıkarılmaz.

### R2 — Yanlış alt iddianın geri çekilmesi

[AttributeSystem.h](../SpaceAbilitySystem/include/attributes/AttributeSystem.h) `mHandleToAttribute` için `ly::Map` kullanır; [Core.h](../LightYearsEngine/include/framework/Core.h) satır 289 bunu `std::map` olarak tanımlar. Ekleme bu map'te rehash yapmaz ve mevcut iterator'ı geçersizleştirmez. Önceki raporda türü doğrulamadan unordered-map davranışı atfetmişim. Bu gerçek bir inceleme hatasıdır.

Kalan yol: onAttributeChanged içinde aynı handle RemoveModifier edilirse iç çağrı ilgili map düğümünü siler; dış çağrı `erase(foundAttribute)` ile silinmiş iterator'ı kullanır. Callback'in AttributeSystem::Clear çağrısı da map'i boşaltır. [RemoveModifier](../SpaceAbilitySystem/src/attributes/AttributeSystem.cpp) 205–220 HEAD ile aynıdır. Doğrudan okunan SpaceShip attribute dinleyicileri movement refresh yapıyor; bu tetikleyiciyi onlara atfetmiyorum.

### R3 — Tick ile activation aynı koruma değil

Eski GameplayAbilityInstance::Cancel zaten aktif instance'a hemen EndAbility uyguluyordu. Eski GameAbilityActionExecutor::TickAction `ExecuteAction` dönüşünde `action.spec->interval` ve repeated state'e erişiyordu; bu dosya ve [AbilityExecution.h](../SpaceAbilitySystem/include/abilities/AbilityExecution.h) HEAD ile değişmedi. Yeni activation iptal ertelemesi BeginExecution esnasındaki iptali iyileştiriyor, Tick execution'ın kendi vektörünü korumuyor.

En dar kanıt yolu: WhileActive action'ın callback'inden aynı instance'a doğrudan Cancel; EndExecution vektörü temizler, dış action yürütmesi referansı kullanmaya devam eder. Bütün SetLevel girişlerinin aynı biçimde davranacağını varsaymaya gerek yok; doğrudan instance yolu yeterlidir. Bu tur shipped callback zinciri veya crash üretilmedi.

### R4 — Pointer bozulması her refresh'te zorunlu değil

Eski RefreshScopedConfiguration da aktiflik kontrolü olmadan RebuildDefinitionForLevel çağırır. Yeni wrapper aynı gövdeyi çalıştırır. Eklenmiş progression action'larının silinmesi/yer değiştirmesi veya vektör kapasitesinin büyümesi halinde tutulan spec adresleri güvensizdir; her refresh'in mutlaka yeniden tahsis yaptığı söylenemez. Çağıran bulunmadığı için bunu bugün kullanılan feature'ın kesin arızası gibi sunmak doğru değildi; API genişletme riski olarak korunur.

### R5 — Benim değişikliğime ait yeni sonuç

Eski RemoveEffectInternal sırası:

```text
removal-in-progress'e ekle → tags kaldır → modifiers kaldır → removing bildir
→ effect erase → removal-in-progress'ten çıkar → removed bildir
```

İlk modifier callback'i throw ederse eski kod effect kaydını silmez ve removal-in-progress kaydını da çıkarmaz; effect kısmen temizlenmiş, tekrar kaldırılması engellenmiş halde takılır. Eski sürüm sağlıklı değildir.

Benim yeni sıram:

```text
RAII removal kaydı + effect snapshot
→ tags grubunu dene → modifiers grubunu dene → removing bildirimi dene
→ effect erase → diğer bildirimleri dene → ilk hatayı yeniden fırlat
```

Ancak modifiers grubunun içindeki foreach ilk throw'da durur. İkinci modifier hâlâ owner üzerindeyken effect silinir; snapshot da çıkışta gider. **Eksik cleanup'a rağmen sahip kaydının silinmesi benim değişikliğimin eklediği somut sonuçtur.** Düzeltmem yarımdır; dış catch yeterli sanılmıştır. Bunun sağlıklı eski yolu bozduğunu değil, zaten hatalı exception yoluna farklı ve sahipsiz kaynak bırakan bir durum eklediğini söylüyorum.

Action tarafında eski EndExecution exception'ı EndActivation'ı atlatıp instance'ı aktif bırakırdı. Yeni EndAbility EndExecution hatasını toplar, `mExecutionStarted=false` yapar ve EndActivation'ı tamamlar. İç action cleanup döngüsü hâlâ yarıda kalabildiğinden, kaynaklar kapanmadan instance bitmiş sayılabilir; sonraki Cancel aktiflik kontrolünden geri döner. Bu da eski eksikliğe yeni tamamlanma semantiği eklediğim yerdir.

### R6 — Player tüketicisi eski, erteleme sınırı yeni

[TryPurchaseAbilityLevel](../LightYearsGame/src/player/Player.cpp) gövdesi değişmemiştir: LevelUpAbility true dönerse scrap düşülür ve eski pointer'dan level okunur. Eski component LevelUp doğrudan runtime'a gider; eski Clear anında runtime koleksiyonunu silebilir. Yeni operation koruması silinmeyi dış component çıkışına taşır. Her iki durumda da Player pointer'ı dönüşte geçersiz olabilir. Yeni guard burada bir lifetime hatasını Player'a kadar tamamen çözmemiştir; önceki güvenlik iddiam eksiktir.

Level-change callback exception'ında seviye değişmiş ama ödeme gerçekleşmemiş olması da eski SetLevel sırasından gelir. onAbilityLevelChanged için taranan üretim .cpp kaynaklarında dinleyici bulunmadı; ortak onAbilityChanged yolu da bulunduğu için bu arama tek başına bütün tetikleyicilerin yokluğunu kanıtlamaz.

### R7/R8 — Eski defensive hata yolları

Combat context pointer'ının atama/reset/queue sırası diff'te değişmiyor. Timer one-shot callback/expiry sırası eski kaynakta açıkça aynı. Exception'dan sonra işlem sürdürülmesi veya aynı CombatRuntime'a nested işlem koşulları sağlanmadan günlük oyun hatası iddiası kurulamaz. Bu iki başlığı “oyunda kesin yaşanan yeni kusur” gibi okumaya yol açan toplu sekiz-hata sunumu geri çekilir; koşullu kaynak riskleri korunur.

## Önceki rapora yapılan düzeltmeler

1. R2 rehash gerekçesi kaldırıldı; std::map gerçeği ve geçerli dar tetikleyiciler yazıldı.
2. R5'e benim cleanup değişikliğimle oluşan yeni failure-state açıkça eklendi.
3. Sekiz başlık, sekiz runtime'da doğrulanmış oyun hatası olarak kabul edilmedi. Koşullar ve shipped kanıt yokluğu açık bırakıldı.
4. Köken incelemesi bütün proje denetimi veya bu sekiz riskin onarımı sayılmaz.

## Yeniden kontrol için komutlar

```powershell
git show ce14e9308c9d9825c7648c7217661a3d3fbbaafe:SpaceAbilitySystem/include/effects/GameplayEffectRuntimeSystem.h
git diff ce14e9308c9d9825c7648c7217661a3d3fbbaafe -- SpaceAbilitySystem/include/effects/GameplayEffectRuntimeSystem.h SpaceAbilitySystem/include/abilities/GameplayAbilityInstance.h
git diff ce14e9308c9d9825c7648c7217661a3d3fbbaafe -- SpaceAbilitySystem/src/attributes/AttributeSystem.cpp SpaceAbilitySystem/src/effects/GameplayEffectBindings.cpp SpaceAbilitySystem/include/abilities/AbilityExecution.h LightYearsGame/src/gameplay/ability/GameAbilityActionExecutor.cpp
git diff ce14e9308c9d9825c7648c7217661a3d3fbbaafe -- LightYearsGame/src/player/Player.cpp LightYearsGame/src/gameplay/combat/CombatRuntime.cpp LightYearsEngine/src/framework/TimerManager.cpp LightYearsGame/src/gameplay/ability/GameAbility.cpp
```

Üçüncü komut denetim anında boş fark verdi. Bu dört dosyanın ilgili mekanizmalarını son yamaların oluşturmadığının doğrudan kanıtıdır. Diğer dosyalarda yalnız ilgili gövdeler/hunk'lar karşılaştırılmıştır; bütün dosyanın değişmediği iddia edilmez.
