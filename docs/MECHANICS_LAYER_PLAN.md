# Ortak mekanik katmanı — yeniden düzenleme planı

2026-10-27. Kullanıcı isteği: "davranışlar mekanik olarak ortak bir pencerede olmalı; modüller bunları kullanacak, sayıları ve değerleri modüller belirleyecek, yani kullanıcı olacak." Tasarım ilkeleri ve tasarım kalıplarıyla yapılır. Bu belge planın sahibidir; uygulama durumu her aşama sonunda buraya işlenir.

## 1. Hedef

**Mekanik** (ne olur) ile **kullanıcı** (ne zaman, hangi sayıyla) ayrılır.

- Mekanik katmanı modül, yetenek veya düşman bilmez. "300 yarıçapta 120 Thermal hasar", "MaxShield'in %10'unu doldur", "hasarın %30'unu 3 sn'ye yay" gibi işleri yapar; sayıları dışarıdan alır.
- Modül katmanı yalnız şunu söyler: **hangi olayda** (tetik), **hangi koşulda** (koşul), **hangi mekaniği** (komut), **hangi sayılarla** (seviye, bölge, gemi seviyesi ve yan özellikten çözülmüş değerler). Bir modül kendi mekaniğini yazmaz.
- Aynı mekaniği yarın bir yetenek, düşman veya Unique de aynı yoldan kullanabilir.

Davranış değişmez. Bu bir yeniden düzenlemedir: her aşamadan sonra oyun aynı sayılarla aynı şeyi yapar ve mevcut E2E testleri bunun kanıtıdır.

## 2. Bugünkü durum (2026-10-27 kod okuması)

| Katman | Durum |
|---|---|
| Veriyle çalışan genel kurallar | Common/Uncommon/Rare'in çoğu JSON'da ortak kural türlerini kullanır (tetikli buff, olay aksiyonu, hasar kuralı, owner burst, bölge etkisi, aile statı). Mekanik, kural türüne aittir, tek modüle değil. Ama bu türler `gameplay/module/` içinde ve modüle göre kurulu; yetenek veya düşman kullanamaz. `ModuleRulePackage` 35'ten fazla kural listesi taşır. |
| Ortak alana çıkmış mekanikler | `damage/` (statü stack'i, tepkime, armor kırma, parça dağıtımı), `control/` (stun), `movement/` (zamanlı çekme, etki/yavaşlatma servisi), `projectile/` (Phase Slip geçişi, yansıma), `combat/` (ölümcül vuruş yakalayıcıları, ele geçirme), `ShieldComponent` (Battery rezervi), `module/ModuleTriggerService` (Unique tetik servisi). |
| Modüle gömülü mekanikler | `module/generated/` aktörleri (gecikmeli patlama, mayın, güdümlü mermi, sekme, halka patlama, için için yanan alan, Speed Lance mızrağı) ve kaynağa bağlı hasar yardımcısı; `module/rules/ModuleDeferredDamageRuntime` (`SpaceShip` bile buradan çağırır); Last Stand dirilişi ve Parasite Hull kaçışı (`PlayerSpaceShip::ResolveLethalDamage` → `ShipModuleApplier::TryPreventOwnerDeath`, öncelik sabit kodlu; ölen taraf için ortak liste yok. Mevcut `LethalHitInterceptorList` yalnız vuran tarafındır ve oyuncu gemisi ona bakmaz); Shield Rupture kalkan kaybı geçmişi; `ModuleReactiveRuntime` içindeki 14 olay aksiyonu (kalkan/can doldurma, zamanlı iyileşme, tavan üstü kalkan, bekleme iadeleri, yakına Cryo, Gold); Epic runtime'ların içindeki genel parçalar (can bedeli, statü döngüsü, can eşiği, yön/hız bonusu, sıralı zincir). |
| Tetik kaynakları | Üç ayrı dinleyici var: `ModuleTriggerRuntime` (yetenek olayı + kalkan), `ModuleReactiveRuntime` (öldürme, kritik, isabet, hasar), Legendary/Epic runtime'larının kendi kancaları. Unique'in "zorla tetikleme" desteği bu dağınıklık yüzünden 11 kural türünde kurulamadı (bkz. [UNIQUE_MODULES_IMPLEMENTATION.md](UNIQUE_MODULES_IMPLEMENTATION.md)). |

## 3. Hedef mimari

Modülün eski cümlesi aynen mimariye dönüşür: **tetik → koşul → mekanik ← sayılar**.

```
 Savaş olayları (Observer)         Koşul (Specification)       Mekanik (Command)
 CombatEventStream  ──────────────▶ MechanicCondition ────────▶ IMechanic<Spec>::Execute(ctx, spec)
        ▲                                                            ▲
        │ olay üretir                                                │ çözülmüş sayılar (Spec)
 yetenek, silah, hasar hattı,                         Kullanıcılar: modül bağlayıcısı (bugün),
 kalkan, can, öldürme                                 yetenek, düşman, Unique (yarın)
```

### 3.1 Kalıplar ve nerede kullanıldıkları

| Kalıp | Nerede | Neden |
|---|---|---|
| **Observer** | `CombatEventStream`: gemiye ait tek olay akışı (yetenek kullanımı, isabet, kritik, öldürme, hasar alma/verme, kalkan kırılması, Afterburner başlangıcı). Abonelik RAII token ile. | Üç ayrı dinleyici tek kaynağa iner. Unique tetik servisi, modüller ve ileride yetenekler aynı olayları görür. |
| **Command** | `IMechanic<Spec>`: her mekanik tek bir `Execute(const MechanicContext&, const Spec&)`. Örnek: `RestoreShield`, `HealOverTime`, `TemporaryShield`, `ReduceCooldowns`, `AreaBurst`, `RingBurst`, `SpawnMine`, `SpawnGuidedProjectile`, `Ricochet`, `LingeringField`, `DeferDamage`, `TimedModifier`, `ApplyStatus`, `PullTargets`, `Stun`. | Mekanik tek iş yapar ve kim çağırırsa çağırsın aynı davranır. "Zorla tetikleme" yalnızca komutu yeniden çalıştırmak olur. |
| **Typed Registry + Factory** | `MechanicRegistry<Spec>`: mekanik kimliği → komut. JSON yükleyici kural türünü tipli `Spec` yapısına çevirir. | AGENTS kuralı: `std::any` veya her şeyi taşıyan tek struct yok; `PresentationProfileRegistry<T>` ile aynı tipli yaklaşım. |
| **Specification** | `MechanicCondition`: birleşebilen koşullar (And/Or/Not): hedef canı ≤ %X, sahip canı ≤ %X, hedefte statü var/yok, kalkan dolu, boş slot sayısı, Threat ≥ N. Bugünkü `ModuleDamageCondition` sıralaması bunlara çevrilir. | Koşullar modülden ayrılır; yetenekler de kullanır. |
| **Strategy** | Sayı çözümü modül tarafında kalır: seviye tablosu, büyüme türü (doğrusal/bileşik/basamaklı), bölge bonusu, Amplifier, gemi seviyesi `n`, yan özellik. Mekanik seviye görmez, yalnız sonucu (`Spec`) alır. | "Sayıları modül belirler" ilkesi burada tek yere bağlanır. |
| **Decorator** | Güç çarpanı: `ScaledSpec` / `PowerMultiplier` (Echo %50, Cascade, Four Corners, Ultimate ×1,6). Her `Spec` "ana büyüklük" alanlarını işaretler; ölçekleme tek yerden yapılır. | Unique turunda her kural türü için ayrı ayrı yazılan "%50 ne demek" kuralı tek yere iner. |
| **Facade / Adapter** | `ModuleMechanicBinder`: bir yerleşimin çözülmüş kurallarını `Binding{trigger, condition, mechanic, spec, instanceId}` listesine çevirir ve RAII ile kaydeder. `ShipModuleApplier` yalnız bu cepheyi çağırır. | Modül katmanı ince bir istemci olur; 600+ satırlık applier ve 750 satırlık reactive runtime küçülür. |
| **Mediator** | Mevcut `ModuleTriggerService` olay akışının üstüne oturur: doğal/zorla ayrımı, olay başına bir kez kuralı, Unique bağlantıları. | Unique davranışı değişmez, desteklenmeyen tür sayısı azalır. |

### 3.2 İlkeler

- **SRP:** her mekanik tek iş; olay, koşul, mekanik ve sayı çözümü ayrı sınıflarda.
- **OCP:** yeni mekanik = yeni komut + kayıt satırı. Dev `switch`'e satır eklenmez (bugünkü `ModuleEventActionKind` switch'i gibi).
- **DIP:** modül katmanı mekanik arayüzlerine bağımlıdır; mekanik katmanı modülü bilmez (`gameplay/module` include edemez, CMake/kod incelemesinde kontrol edilir).
- **Tek doğru kaynak:** sayılar JSON'da, kimlikler `GameplayTag`/`ContentId` olarak (IDENTITY_AND_CONTRACT_RULES). Mekanik varsayılan sayı taşımaz.
- **Sahiplik/yaşam:** AGENTS "Registry and lifetime rules" aynen: `GetUniqueID` anahtarı, `weak_ptr`, RAII kayıt token'ı, nesil korumalı silme, dünya/gemi sahipli servis; süreç geneli kayıt yok.
- **Motor sınırı:** her şey oyun deposunda. Zamanlı stat buff'ları için motordaki `GameplayEffect` (süreli etki) kullanılabiliyorsa kullanılır; motor değişikliği gerekirse "engine request" yazılır, `D:\Vega`'ya dokunulmaz.

### 3.3 Klasör yerleşimi

Mekanikler alanlarına göre mevcut klasörlere gider; yeni çekirdek tek klasörde toplanır.

| Yer | İçerik |
|---|---|
| `gameplay/mechanics/` (yeni) | `CombatEventStream`, `MechanicContext`, `IMechanic<Spec>`, `MechanicRegistry`, `MechanicCondition`, güç çarpanı yardımcısı |
| `gameplay/damage/` | alan/halka patlama hasarı, ertelenmiş hasar, kalkan kaybı geçmişi, kaynaktan bağımsız üretilmiş hasar |
| `gameplay/combat/actors/` (yeni alt klasör) | mayın, güdümlü mermi, sekme mermisi, gecikmeli patlama, halka patlama, kalıcı alan, mızrak aktörleri |
| `gameplay/combat/` | diriliş, ölümcül vuruş listesine bağlanır |
| `gameplay/resource/` veya `HealthComponent`/`ShieldComponent` | zamanlı iyileşme, tavan üstü geçici kalkan, kalkan/can/Afterburner doldurma |
| `gameplay/ability/` | bekleme iadesi ve azaltma mekanikleri (yetenek bileşeninin ortak API'si) |
| `gameplay/module/` | yalnız: içerik, yerleşim, sayı çözümü, bağlayıcı, Unique politikası, modüle özgü kimlik parçaları |

## 4. Aşamalar

Her aşama ayrı bir worktree/dalda yapılır, kendi başına derlenir ve testleri geçer, ayrı commit olur. Davranış değişikliği yoktur; bir test sayısı değişirse aşama durur ve nedeni raporlanır. Doğrulama AGENTS "Focused verification" kuralına göre yalnız etkilenen E2E'lerdir.

| Aşama | İş | Kabul ölçütü | Odaklı testler |
|---|---|---|---|
| **M0 — Çekirdek iskelet** | `gameplay/mechanics/`: olay akışı, bağlam, tipli kayıt, koşul arayüzü, güç çarpanı. Henüz hiçbir modül taşınmaz. | Oyun derlenir; yeni çekirdek için birim/E2E testi. | yeni `MechanicsCoreTests` |
| **M1 — Tek olay akışı** | `ModuleTriggerRuntime`, `ModuleReactiveRuntime` ve `ModuleTriggerService` olaylarını `CombatEventStream`'den alır; eski üç ayrı kanca kalkar. | Tetik sırası ve "olay başına bir kez" kuralı aynı. | Uncommon, RareOwnerBurst, RareRunCounter, RareSideFeatures, UniqueModules, EpicUtilityMovement |
| **M2 — Olay aksiyonları komut olur** | 14 `ModuleEventActionKind` → komutlar (`RestoreShield`, `TemporaryShield`, `HealOverTime`, `RestoreAfterburner`, bekleme azaltma/iadeleri, `ApplyStatusNearby`, `AwardGold`). Zamanlı iyileşme ve geçici kalkan can/kalkan bileşenlerine taşınır. Switch kalkar. | Aynı sayılar; Unique zorla tetikleme bu aksiyonlarda değişmeden çalışır. | Uncommon, ModuleDefenseLayer, RareShieldRules, ModuleSidePools, UniqueModules |
| **M3 — Savaş aktörleri** | `module/generated/` aktörleri `combat/actors/` altına, tipli `Spec` ile; hasar yardımcısı kaynaktan bağımsız olur (kaynak: modül/yetenek/düşman etiketi). "Modül ürünü başka modülü tetiklemez" bayrağı olay akışında genelleşir. | Patlama/mayın/mermi davranışı ve hasarları aynı. | RareDyingStarMechanics, ModuleReleaseValve, ModuleSpeedLance, RareOwnerBurst, RareStaticDamageRules, Uncommon |
| **M4 — Hasar hattı mekanikleri** | Ertelenmiş hasar `damage/` altına; Shield Rupture kalkan kaybı geçmişi `damage/` servisine; ölen taraf için ortak bir ölümü önleme listesi kurulur; Parasite Hull ve Last Stand oraya kaydolur, öncelik açık yazılır, her gemi önce kendi listesine sonra vuranınkine bakar. | Ölüm/diriliş sırası, Parasite → Last Stand önceliği aynı. | EpicDefense, RareShieldBatteryRupture, LegendaryTakeoverModules, ModuleSidePools (Deferred) |
| **M5 — Koşullar** | `ModuleDamageCondition` ve can eşiği kuralları `MechanicCondition` spesifikasyonlarına çevrilir. | Koşullu hasarlar aynı. | RareDamageConditions, RareRatioRules, RareStatusRules, Uncommon |
| **M6 — Epic/Legendary inceltme** | Epic/Legendary runtime'larındaki genel parçalar komut/koşul olarak çıkar (can bedeli, statü döngüsü, can eşiği hızlanması, yön/hız bonusu, sıralı zincir sayacı). Modüle özgü kimlik ince bir politika sınıfında kalır. | Her Epic/Legendary testi aynı. | EpicOffense, EpicDefense, EpicUtilityMovement, EpicPrimary, EpicStat, Legendary E2E'leri (yalnız dokunulanlar) |
| **M7 — Unique tamamlama ve kapanış** | Tüm komutlar zorlanabilir hale gelince Unique'in desteklemediği türler yeniden değerlendirilir (gerçek isabet isteyenler hariç). Eski kural türlerinin modül-özel kopyaları silinir; AGENTS'a "yeni mekanik `gameplay/mechanics` sözleşmesiyle yazılır" kuralı eklenir. | Unique destek tablosu güncellenir; ölü kod yok. | UniqueModules ve M1–M6'da dokunulanlar |

Sıra bağımlılığı: M0 → M1 → (M2, M3, M4, M5 paralel yapılabilir, farklı dosyalar) → M6 → M7.

## 5. Riskler ve önlemler

- **Büyük diff, gizli davranış değişikliği.** Her aşama küçük tutulur; mevcut E2E'ler davranışın kilididir. Sayısal fark çıkarsa aşama geri alınır, test "düzeltilmez".
- **Olay sırası.** Bugün bazı tepkiler "vuruştan bir kare sonra" işlenir (Reaction Core gibi). Olay akışı sıra ve gecikme bilgisini korur; M1'de bu ayrıca test edilir.
- **Performans.** Olay başına sanal çağrı ve abonelik listesi; savaş yoğunluğunda ölçülür (fps kök nedeni dersleri: gereksiz kopya ve kilitten kaçın).
- **Tek seferde her şeyi genelleştirme isteği.** Yalnız bugün en az iki kullanıcısı olan veya açıkça paylaşılabilir mekanikler çıkarılır; tek kullanımlık kimlik parçaları modülde kalır.

## 6. Karar (2026-10-27)

Kullanıcı Claude'un önerisini kabul etti: planın tamamı şimdi yapılmaz. İkinci bir kullanıcısı (yetenek, düşman) olmayan mekaniği taşımak bugün kazanç getirmiyor ve asıl darboğaz oyun döngüsü (para, Threat, ödül). Şimdi yalnız iki küçük iş Sol'a verildi:

1. **Ölümü önleme listesi (M4'ün bir parçası):** ölen taraf için ortak, RAII kayıtlı, öncelikli liste; Parasite Hull ve Last Stand bu listeye kaydolur; `SpaceShip::ResolveLethalDamage` önce kendi listesini, sonra vuranın listesini çözer, oyuncu gemisi bu temel yolu kullanır.
2. **Ertelenmiş hasar (M4'ün bir parçası):** `module/rules/ModuleDeferredDamageRuntime` → `damage/` altında modülden bağımsız mekanik; sayı yapısı damage tarafında, modül yalnız sayıları verir.

Geri kalan aşamalar bu belgede bekler. Bir mekaniği ikinci bir kullanıcı istediğinde (örneğin bir düşman mayın bırakmak isterse) o parça bu planın kalıplarıyla ortak alana taşınır. Yeni mekanikler zaten ortak alanda yazılır (kullanıcı kuralı, 2026-10-27).

MECH-SLIM implementation and verification are tracked in [MECH_SLIM.md](MECH_SLIM.md). This work covers only the two decisions above; the remaining M4 work and other stages stay deferred.

## 7. Açık kararlar (tam plan yeniden açılırsa)

1. Uygulayıcı: aşama başına Sol (ayrı worktree) mi, Claude mı?
2. Zamanlı stat buff'ları motordaki `GameplayEffect` ile mi, oyunun mevcut tetikli buff çalışma zamanı genelleştirilerek mi yapılsın? Öneri: önce mevcut çalışma zamanını taşı, motor etkisine geçişi ayrı karar yap.
3. Yetenekler bu turda mekanik katmanına geçirilmez; yalnız aynı katmanı kullanabilecek hale gelir. Yetenek tarafının taşınması ayrı plan.
