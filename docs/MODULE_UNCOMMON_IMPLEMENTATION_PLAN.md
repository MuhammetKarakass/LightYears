# Uncommon Modüller — Sol/Luna Uygulama Planı

**Tarih:** 2026-10-06 · **Durum:** P0/P1/P2 tamamlandı; P3 runtime ve E2E uygulandı (commit edilmedi) · **Tasarım kaynağı:** [modules.md](modules.md) (Planned/Reworking bölümü), [Modül Sistemi Tasarımı](MODULE_SYSTEM_DESIGN.md), [Common uygulama planı](MODULE_COMMON_IMPLEMENTATION_PLAN.md) (§8 sadeleştirme bulguları)

Bu belge, [modules.md](modules.md)'de tasarlanan **18 Uncommon modülün** ve onları taşıyan **boyut merdiveni / yan özellik / modül seviyesi** altyapısının uygulama sözleşmesidir. Değerlerin sahibi modules.md'deki kartlar, sonra `modules.json`'dır; bu belge ikinci bir değer listesi tutmaz.

---

## 0. Başlangıç noktası ve genişletilebilirlik ilkeleri

Bu ilk sürümdür; amaç 18 modülü çalıştırmak kadar, sonraki nadirliklerin (Rare ve üstü) aynı altyapıya **veri olarak** eklenebilmesidir. Kullanıcı kararı: sistem geliştirilebilir ve sağlam olmalı.

1. **Modül başına sınıf yazılmaz.** Her modül, küçük ve yeniden kullanılan *kural parçalarının* (primitive) veri birleşimidir. Yeni modül = JSON girdisi; kod yalnız ona gerçekten yeni bir kural parçası gerekiyorsa eklenir.
2. **Ana özellik ve yan özellik aynı kural dilini kullanır.** Yan özellik, adlandırılmış bir kural paketidir; havuz yalnız yan özellik kimliklerinin listesidir.
3. **Seviye değerleri tablodur, formül değil.** Her parametre seviye başına açık değer listesi taşır (örnekte 3 adım). Seviye sayısı ve formül karara bağlanmadığından yükleyici N adımı destekler; denge değişikliği JSON düzenlemesidir.
4. **Bilinmeyenler yapılandırmadır, kod değil:** seviye sayısı, yan özellik çıkış ağırlıkları, yükseltme harcaması. Bunlar sonradan değişir; kod bunlara göre dallanmaz.
5. **Davranış değişmez kuralı:** modülsüz yol ve mevcut Common modüller birebir aynı kalır. Common E2E aynen geçmeli.
6. Tüm sayılar Common ×1,3 ölçütüyle yazılmış **önerilerdir**; onaylı denge değildir.

---

## 1. Kapsam

| Dahil | Hariç (bu plan değil) |
|---|---|
| 18 Uncommon modül (7 tek boyutlu, 11 merdivenli) | Rare ve üstü modüller (Kor Birikimi, Altın Kaplama, Zırh Dönüşümü, Kırılma Şoku, Kalıcı Etki Rezonatörü, Enerji Kuplörü) |
| Boyut merdiveni (1-2-3, 1-2, 1-3, 2-3) ve en büyük boyutta açılan yan özellik | Loot düşme tabloları ve dağılımı |
| Yan özellik havuzları (O/D/G, 15 ortak) ve modül başına 2 özel yan özellik | Yükseltme arayüzü ve harcama ekonomisi (yalnız veri/API hazır olur) |
| Modül seviyesi (gelişim) veri modeli, çözümleme ve test kancası | Gold ekonomisi (bkz. §2 engel) |
| Yeni kural parçaları (§3.3) ve E2E | Bağlantı modülleri (Röle, Ayna…) |
| Kart metni, kilitli yan özellik gösterimi | Kayıt/yükleme (kodda modül kaydı yok; bkz. §2) |

---

## 2. Başlangıç durumu (2026-10-06 kaynak anlık görüntüsü)

| Konu | Bulgu | Kaynak |
|---|---|---|
| Modül yerleşimi | `ModulePlacement`: `instanceId`, `moduleId`, `variantIndex` (boyut varyantı), `rarity`, `rolledSideStats` (yalnız **öznitelik** modifier listesi, bölge bonusundan etkilenmez). **Seviye ve yan özellik kimliği yok** | `ModuleLoadout.h` |
| Nadirlik | `ModuleRarity { Common, Rare, Epic, Legendary }`. **Uncommon yok** | `ModuleLoadout.h:14` |
| Çözümleme | `ModuleResolver::Resolve` saf fonksiyon: boyut varyantı → sahip/gemi/primary modifier, `ScopedAbilityRule`, tetik buff, hasar kuralı, cooldown kuralı | `ModuleResolver.cpp` |
| Tetikler | `AbilityActivated`, `ShieldBroken`, `DamageTaken`, `SourceDamageDealt`. **Öldürme tetiği yok**, ama `Event.Combat.KillConfirmed` olayı `CombatRuntime.cpp:738`'de üretiliyor | `ModuleDefinition.h`, `GameplayTags.h:161` |
| Hasar koşulları | `PrimaryShotOnly`, `TargetHealthAtMost`, `FullMagazineShot`. **Status sorgusu, gelen hasar kuralı, kalkan-emme kancası, kritik tetiği yok** | `ModuleDefinition.h` |
| Status | Üst stack sınırı **global** denge dosyasından (`damage_status_balance.json`) geliyor ve payload'da `clamp` ediliyor; sahip başına geçersiz kılma yok | `DamageTypeSystem.cpp:296–310` |
| Kritik | `Owner.CriticalChance` ve `Owner.CriticalDamage` (taban 1,5) öznitelikleri var | `CombatRuntime.cpp:170`, `modules.json` |
| Komşuluk | Yerleşim kuralları ve bölge var; **hücre kenarı komşuluğu sorgusu yok** | `ModulePlacementRules.*` |
| Gold | Kodda Gold ekonomisi **yok** (yalnız ability ilerlemesi için scrap formülü). Gold'a bağlı yan özellikler (G1, Av Saati'nin Av Ödülü özelliği) **engelli** | taranan başlıklar |
| Kayıt | Modül envanteri/loadout kaydı (serileştirme) **bulunamadı**; modüller çalışma zamanı verisi | `Player.*`, `ModuleLoadout.*` |
| Test hibesi | `kGrantAllModulesForTesting = true`: her varyantın bir örneği envantere verilir | `Player.cpp:21` |
| Mimari borç | Common11 sonrası bulgular B1–B7 (iki yerde `ShieldBroken`, etkisiz `FullMagazineShot` enum, şişmiş `ApplyCombatDamage`, tek sınıfta dört kural türü) | Common planı §8 |
| Çalışma ağacı | Başlangıç `git status`: `modules.json`, `ModuleDefinition.h`, `ModuleLoader.cpp`, `CombatRuntime.*`, `PrimaryWeapon*`, `ModuleScreenPresenter.cpp`, `CommonModuleE2ETests.cpp` ve görsel kütüphane dosyaları **kirli** (commit edilmemiş Common içerik revizyonu + başka işler). Bu planın hedef dosyalarıyla çakışır | `git status` |

---

## 3. Mimari

### 3.1 Veri modeli v2

- `ModuleRarity`'ye **Uncommon** eklenir (Common ile Rare arasında). Numeric değere bağımlı kod/test varsa P2 öncesi taranır.
- `ModuleDefinition` genişler: `rarity`; `sizeVariants` ile birlikte **merdiven bildirimi** (hangi varyantlar var, en büyük boyut); `levelCount` (varsayılan 3); `sideFeatures` (özel yan özellik kimlikleri) ve `sharedPools` (kullanılan ortak havuz öğeleri).
- `ModulePlacement` genişler: `level` (varsayılan 1) ve `sideFeatureId` (boş = yok / tek boyutlu modül). `rolledSideStats` korunur ama yeni sistem onu kullanmaz (eski öznitelik yan stat yolu; ayrı karar).
- Yeni içerik dosyası **`module_side_features.json`**: yan özellik tanımları (kural paketi + seviye tabloları) ve havuzlar (O/D/G). Yükleyici bilinmeyen kimlik, eksik seviye girdisi, havuzda olmayan özellik ve tek boyutlu modülde yan özellik tanımını **reddeder**.
- **Seviye tabloları:** bir kuralın sayısal alanı `[v1, v2, v3…]` listesi olabilir; çözümleme `level` ile seçer, liste kısaysa son değeri tutar (veya yükleyici reddeder; P2'de karar). Kural şablonu ile değer tablosu ayrıdır.

### 3.2 Çözümleme

`ModuleResolver::Resolve` aynı saf yapıda kalır; ek olarak:
1. Seçilen varyantın kurallarını `level`'e göre somutlaştırır.
2. `variantIndex` modülün **en büyük boyutundaysa** ve `sideFeatureId` doluysa yan özelliğin kurallarını aynı çıkışa katar (kuralın kaynak kimliği: `(instanceId, ruleIndex)`; yan özellik indeksleri ana özellikten sonra gelir ve ayrı ad uzayı alır, çakışmaz).
3. Küçük boyutta yan özellik **kilitli** kalır: etkisi yoktur, yalnız sunumda görünür.
4. Yan özellik seviyesi modül seviyesini izler (öneri: ana özelliğin yarı hızında; sayılar tablodadır).

### 3.3 Yeni kural parçaları (primitive)

Hangi modüllerin neye ihtiyaç duyduğu; tekrar edenler önce yapılır:

| Kural parçası | İhtiyaç duyanlar (ana · yan) |
|---|---|
| **Öldürme tetiği** (`KillConfirmed`) | Yağma Kalkanı, Av Saati · O3, D3 |
| **Sahip durumu koşulu** (can ≤ %X) | Sıkı Dişler · Cam Top yan özelliği (can ≤%50) |
| **Hızla ölçeklenen öznitelik** | Hareket Ritmi |
| **Yetenek kullanımı + Luck zarı + cooldown iadesi** | Şanslı Cast |
| **Hedef status sorgusu** (status'suz / Thermal+Cryo / max stack) | Fotonic Mercek, Isıl Şok Bobini |
| **Gelen hasar kuralı** (çarpan; kaynak status'u) | Sıkı Dişler, Soğuk Karşılık yan özelliği |
| **Kalkan-emme kancası** (`ShieldComponent::onShieldDamaged` → yansıma) | Yansıma Kalkanı |
| **DamageTaken → status uygulama** | Soğuk Karşılık |
| **Kritik vuruş tetiği** | Keskin Ölçek yan özelliği (Kritik Ritim) |
| **Komşuluk sorgusu** (hücre kenarı, Q/E/F/R slotu, en çok 2) | Can Köprüsü |
| **Sahip başına status geçersiz kılma** (max stack, donma eşiği, süre) | Ark Genişletici, Kırağı Kilidi |
| **Üretilen varlık** (güdümlü mermi, sekme, mayın, gecikmeli patlama) + zincirleme koruması + sınır | Yayılım Çekirdeği, Sekme Ucu, İz Mayınları |

Aynı kural parçası ana özellikte de yan özellikte de kullanılabilir.

### 3.4 Üretilen varlıklar için sert kurallar

- Üretilen mermi/patlama/sekme/mayın **başka modül kuralını tetiklemez** (`skipModuleDamageRules` benzeri bayrak; Common planı B6 ile birlikte gözden geçirilir).
- Aynı anda en çok mayın sayısı ve mermi ömrü **teknik sınır** olarak yapılandırılır (performans; denge sınırı değildir).
- Hasar sırası: modül çarpanları, sonra kritik.

---

## 4. Fazlar (Sol yönetir, Luna uygular)

**Çalışma kuralları:** Common planı §7 geçerli. Aynı anda **tek yazıcı ajan**; salt okunur denetimler paralel olabilir. Her faz bir **commit** ile kapanır; yalnız o fazın dosyaları stage edilir; başka işin kirli dosyalarına dokunulmaz. Commit yalnız kullanıcı onayıyla atılır ve **Claude atıf satırı eklenmez**. Sol her fazda bağımsız derler, diff'i okur ve E2E'yi kendisi koşar; ajan raporuna güvenilmez. Windows derleme/test komutları Common planı §5'tekiyle aynıdır (vcvars64 + `VSLANG=1033`, seri bağlama).

### P0 — Ön kontrol (Sol)
- **2026-10-06 bağımsız P0:** HEAD `bed824d`, dal `codex/luna-callback-lifetime-checkpoint`; modül hedef dosyaları temiz. Mevcut görsel dirty dosyalar ve untracked `GDD.md`, `modules.md`, bu plan korunur. vcvars64 sonrası `set "VSLANG=1033"` ile `cmake --build C:\LightYears\out\build\x64-debug --target LightYearsGame LightYearsCommonModuleE2ETests LightYearsGasLiteTests --parallel 1 --verbose` exit 0. Sandbox içinde iki deneme ilerlemeden kaldığı için durduruldu; aynı komut sandbox dışında geçti. CTest Common + MixedLoadout 2/2, Common artefaktı 358 kontrol ve `passed=true`; MixedLoadout ayrıca kontrol sayısı yazdırmıyor.
- **P2 ön taraması:** `ModuleRarity` yalnız enum, placement varsayılanı, inventory aktarımı ve bir `Rare` test atamasında kullanılıyor; sayısal cast/sıralama/serileştirme bağımlılığı bulunmadı. `Player::AddModuleToInventory` için graph ve kapsamlı metin taraması çağıran bulmadı; wrapper `ModuleLoadout::AddToInventory`'ye aktarır. Test hibesi `Player::ApplyModuleLoadout` içinden doğrudan loadout API'sini çağırır (varyant başına Common). Presenter `Player::BuildModuleRules`, loadout placement/inventory ve lookup üzerinden tanım + `variantIndex` okur; açıklamayı varyant kural değerlerinden biçimlendirir, boyutu shape'ten ve reboot durumunu applier'dan okur. Rarity/level/sideFeature alanları kart modeline taşınmıyor. Kaynaklar: `ModuleLoadout.h/.cpp`, `Player.cpp:497–530`, `ModuleScreenPresenter.cpp:113–210,338–426`.
- **P1 kapsam bağımlılığı:** `PlayerSpaceShip.cpp:17` null olabilen pointer constructor'ını çağırıyor; R1b referans API'sine geçiş bu tek çağrının değiştirilmesini gerektiriyor. Dosya ilk izinli listede olmadığı için kapsam soruldu; kullanıcı yalnız constructor çağrısının referanslarla değiştirilmesini açıkça onayladı.
- `git status`: kirli dosyaların hangisinin commit edilmemiş Common revizyonu, hangisinin başka iş olduğunu ayır. **Common revizyonunun commit edilmesi için kullanıcı kararı alınır**; uygulama o commit üzerinde başlar.
- Başlangıç doğrulaması: oyun build + `LightYearsCommonModuleE2E` (son kayıt 358 kontrol) + `LightYearsGasLiteMixedLoadout`.
- `ModuleRarity` sayısal kullanımı, `Player::AddModuleToInventory` çağıranları ve modül ekranı sunumu taranır; sonuçlar plana işlenir.
- **Çıktı:** başlangıç commit'i ve P1'in güvenle başlayabileceği onay.

### P1 — Sadeleştirme (Common planı §8 R1; Luna A)
- R1a: tek tetik hattı (`ShieldBroken` dahil, zamanlı buff ve hasar kuralı aynı yol). R1b: `ShipModuleApplier` kural türü başına küçük uygulayıcı parçalara bölünür, ham pointer seçenekleri kalkar. R1c: ek vuruş üretimi `ApplyCombatDamage`'dan ayrılır, bayrak sızıntısı gözden geçirilir. R1d: E2E dosyası modül grubuna göre bölünür.
- **Kabul:** davranış değişmez; mevcut Common E2E aynen geçer; yeni tetik/koşul eklemek için dokunulacak dosya sayısı azalır.

### P2 — Veri modeli v2, zar ve dikey dilim (Luna B)
- §3.1: `Uncommon` nadirliği, `level`, `sideFeatureId`, `ModuleDefinition` genişlemesi, `module_side_features.json` ve yükleyici doğrulaması, seviye tablosu çözümü, `ModuleResolver` birleştirmesi (§3.2).
- Saf `ModuleRoller`: tohumlu rastgele; havuzdan yan özellik seçer (ağırlık yapılandırma). `Player::AddModuleToInventory` yeni alanları taşır; test hibesi merdiven varyantlarını ve yan özellikleri kapsar.
- **Dikey dilim (yalnız mevcut kural parçaları):** Cam Top Çekirdeği (1-3), Çift Besleme Çekirdeği (3), Keskin Ölçek (yalnız A) ve yalnız öznitelik tabanlı yan özellikler (O1, O2, D1, D5, G2, G3, G4, G5).
- **Kabul:** küçük boyutta yan özellik etkisiz/kilitli, en büyük boyutta etkili; aynı modül iki farklı yan özellikle farklı sonuç verir; seviye 1→3 değerleri tablodan okunur; geçersiz JSON reddedilir; Common sonuçları değişmez.

### P3 — Öldürme tetiği ve sahip durumu (Luna B)

- **2026-10-07 uygulama:** Kullanıcının açık isteğiyle Codex doğrudan uyguladı; Luna/alt ajan kullanılmadı. Beş ana modül, Loot Shield'in Hunt Fuel/overflow dönüşümü, Hunt Clock'un her beş öldürmede iadesi, Movement Rhythm'in Advanced Thrust özelliği ve Grit'in AP/EP seçimi çalışır. O3/D3 yeni parçaları; D2/D4 ise mevcut buff/modifier parçalarıyla yalnız içerik olarak eklendi. Gold yok; Movement Rhythm'in 1,5 sn sönme önerisi için açık kullanıcı yanıtı henüz yok, içerikte etkin değil. Grit'in kalkan yenileme yan özelliği bu fazın A + AP/EP kapsamı dışında. Afterburner dahil gerçek hareketin tamamı sayılır. Modül kaynaklı hasarın öldürmesi başka modül kuralını tetiklemez.
- **Can sözleşmesi:** Modül takma/sökme/seviye değişimi mevcut can sayısını korur ve yalnız nihai yeni azami değere keser; geçici modifier birleşimleri üzerinden kesme veya bedava iyileşme yok. Yeteneklerin/seviye gelişiminin mevcut azami-can davranışı korunur.
- **Kanıt:** `LightYearsGame` ve ilgili E2E hedefleri `vcvars64` sonrası `VSLANG=1033` ile derlendi. MixedLoadout/Common/TriggerLifetime/P2/P3 5/5; P3 385, P2 185, Common 358, TriggerLifetime 41 kontrol geçti. [P3 E2E kaynağı](../LightYearsGame/tests/UncommonModuleP3E2E.cpp), [artefakt](../out/build/x64-debug/e2e-artifacts/uncommon-module-p3.json). Bu oyun içi görsel kabul değildir. P2/P3 commit onayı ayrı; sonraki uygulama fazı P4.
- `KillConfirmed` tetiği; sahip durumu koşulu; hızla ölçeklenen öznitelik; Luck zarı ve cooldown iadesi.
- **Modüller:** Yağma Kalkanı (1-2-3), Av Saati (Gold yan özelliği engelli, atlanır), Hareket Ritmi, Sıkı Dişler (A + AP/EP yan özelliği), Şanslı Cast. **Yan özellik girdileri:** O3, D3.
- **Kabul:** her modül senaryosu gerçek ASC/kalkan/can yolunda doğrulanır; öldürme olayı yalnız sahibin öldürmesinde tetikler; tak/sök sonrası handle sayısı sabit.

### P4 — Hasar hattı genişlemesi (doğrudan Codex, tamamlandı)

- **2026-10-07 uygulama kapsamı:** Kullanıcı P4'ü doğrudan Codex ile açtı. Beş ana modül, Sharp Scale'in iki özel özelliği ve Cold Retort'un Jarring Effect özelliği bu dilimde. Photonic Lens tüm hasara; Health Bridge yeteneğin kendi türünde doğrudan vuruşlara, DoT hariç; yansıma Energy Feedback dahil başka modül tetiklemez. Thermal Shock'un süre uzatması ve Cold Retort'un donma yayılımı status fazında; ölüm patlaması üretilen hasar fazında açık. Bu özel özellikler içerikte açılmadı.
- **Sahiplik:** komşuluk hücre kenarı/rotasyon üzerinden saf çözülür; slot-system eşlemesi tek `ModuleAbilitySlots.h` sahibine taşındı, mevcut ASC ve presenter onu kullanır. Yansıma, raw shield delegate yerine hasar bağlamındaki yalnız kalkan kaybını okur (bariyer emmesi ayrıdır); source/target yaşamı nested çağrı boyunca tutulur. Raw shield kırılma tetik hattı açık trigger suppression bilgisini RAII kapsamında taşır; eski Common bonus hasar tetikleri korunur.
- **Kanıt (2026-10-08):** LightYearsGame ve ilgili hedefler vcvars64 sonrası VSLANG=1033 ile exit 0. Mixed/Common/TriggerLifetime/P2/P3/P4 gerçek E2E 6/6; P4 670, P3 385, P2 185, Common 358, Lifetime 41 kontrol. İlk Cold Retort fixture sorgu sorunu giderildi. 18 korunan görsel/UI/font/GDD path hash'i aynı; diff check geçti. Stage/commit yok. [Tam rapor ve kesin komutlar](../beyin/notes/uncommon-p4-2026-10-08.md), [P4 E2E](../LightYearsGame/tests/UncommonModuleP4E2E.cpp). Görsel kabul yapılmadı.
- Hedef status sorgusu koşulları; gelen hasar kuralı kancası; kalkan-emme kancası; DamageTaken'da status uygulama; kritik tetiği; komşuluk sorgusu (`ModulePlacementRules`/`ModuleResolver`).
- **Modüller:** Fotonic Mercek (1), Isıl Şok Bobini, Yansıma Kalkanı, Soğuk Karşılık, Can Köprüsü, Keskin Ölçek'in Kritik Ritim ve Luck→kritik yan özellikleri.
- **Kabul:** modülsüz hasar akışı birebir aynı; koşullar vuruş öncesi durum üzerinden çalışır; yansıyan hasar kritik yapmaz ve modül kuralı tetiklemez.

### P5 — Status kuralı geçersiz kılma (Luna C)
- Sahip başına max stack ve donma eşiği için `DamageTypeSystem` kancası (global denge dosyası korunur, sahip değeri üstüne biner).
- **Modüller:** Ark Genişletici (3), Kırağı Kilidi (2).
- **Kabul:** modülsüz hedefte stack sınırı değişmez; modüllü sahipte 5. Electric stack ve 4 Cryo stack'te donma + bekleme çalışır; modül sökülünce davranış geri döner.

### P6 — Üretilen varlıklar (Luna D)
- Güdümlü mermi, sekme, gecikmeli patlama, mayın; zincirleme koruması ve teknik sınırlar (§3.4).
- **Modüller:** Sekme Ucu, Yayılım Çekirdeği (+ Artçı Darbe yan özelliği), İz Mayınları. Alt fazlar halinde, her biri ayrı commit.
- **Kabul:** üretilen hasar başka modülü tetiklemez; sınırlar aşılmaz; performans için sahne varlık sayısı ölçülür. Mayın penceresi, mayın hasarı ve sekme menzili modules.md'deki açıklar kapandıktan sonra.

### P7 — Sunum (Luna A)
- Kart metni seviye ve yan özelliği çözer; kilitli yan özellik gösterimi; merdiven boyutları modül ekranında. Türkçe glyph/font ve satır genişliği geçerli formatter/fontla ölçülür (Common P7 yöntemi). **Görsel kabul kullanıcıda.**

### P8 — Kapanış (Sol)
- E2E dosyaları modül grubuna göre; hedefli E2E ve oyun build bağımsız doğrulanır; modules.md statüleri (Planned → Implemented) ve bu belgenin sonuç bölümü güncellenir.

### Bağımlılık
```
P0 → P1 → P2 ─┬→ P3 → P4 ─→ P5
              └──────────────→ P6 → P7 → P8
```
P3/P4/P5 aynı dosyalara dokunduğu için seri koşar.

---

## 5. Test stratejisi

- Gerçek ASC, loadout, kalkan ve hasar yolları kullanılır (Common E2E yöntemi); mock davranışı kanıt sayılmaz.
- Her kural parçası için: tetiklenir, tetiklenmez, süre/yenileme, sökünce temizlik, 10 tur tak/sök sonrası handle sabit.
- Her merdivenli modül için: küçük boyutta yan özellik yok, büyük boyutta var, farklı yan özellikle farklı sonuç, seviye tablosu.
- Üretilen varlık modülleri için ayrıca: zincirleme yok ve sınır.
- Yeni E2E dosyaları modül grubuna göre bölünür (tek 1000+ satırlık dosya olmaz).

---

## 6. Riskler ve açık kararlar

| Konu | Durum |
|---|---|
| Kirli çalışma ağacı | P0'da çözülür; Common revizyonu commit edilmeden başlanmaz. |
| Gold ekonomisi yok | G1 ve Av Saati'nin Av Ödülü yan özelliği engelli; Gold sistemi ayrı kapsamda tasarlanana dek tanımlı ama pasif ya da havuzdan çıkarılır. |
| Yan özellik çıkış ağırlıkları | Yapılandırma; varsayılan eşit ağırlık. |
| Yükseltme harcaması ve arayüzü | Kapsam dışı; yalnız `level` ve `sideFeatureId` ayarlama API'si/test kancası. |
| Seviye sayısı ve formül | Karar yok; tablo uzunluğu serbest, test 3 adımla. |
| Ana özellik sınırları | Kullanıcı kararı: denge sınırı yok. Şanslı Cast ihtimali ve Av Saati cooldown'u yoğun durumda çok yükselebilir; değerler gelişimle ayarlanır. Teknik sınırlar (mayın sayısı, mermi ömrü) kalır. |
| Kayıt/yükleme yok | Yan özellik ve seviye şimdilik çalışma zamanı verisi; kayıt sistemi gelince `sideFeatureId`/`level` serileştirilir. |
| Offense tetiğinde ortak sınır | Yayılım (Artçı Darbe yan özelliği) ve Saldırı Ritmi aynı tetikte; birlikte takılırsa şişer. Karar açık. |
| Darbe Kuyruğu, Hızlı Yoğunlaştırıcı | Tanımsız/ertelenmiş; bu planın dışında. |

---

## 7. Sol/Luna devir protokolü

Common planı §7 aynen geçerlidir: Sol fazı sırayla başlatır, tek yazıcıyı garanti eder, her ajan çıktısını bağımsız build + diff + E2E ile doğrular, commit'i yalnız kullanıcı onayıyla kendisi atar, doğrulanmamış hiçbir şeyi kanıt saymaz.

**Her Luna görevinin metni şunları içerir:** hedef modüller ve modules.md kartları; ilgili §3 bölümü ve **izinli dosya listesi**; kirli, dokunulmayacak dosyalar; proje kuralları (kompakt C++17, tipli kural struct'ları, `weak_ptr`/benzersiz kimlik, RAII kayıt, tek yönlü gameplay→sunum, JSON/C++ sözleşme uyumu); kabul ölçütü ve komutlar; rapor biçimi (değişen dosyalar, build sonucu, senaryo sonuçları, sapmalar, açık sorular).

**Sol'un faz sonu inceleme listesi:** handle/abonelik sızıntısı, özyineleme koruması, modülsüz yolun değişmemesi, `(instanceId, ruleIndex)` anahtarı çakışması, yan özellik ad uzayı, JSON/C++ uyumu, yalnız izinli dosyaların değişmesi.
