# Uncommon Modüller — Sol/Luna Uygulama Planı

**Güncel tasarım geçişi (2026-10-13):** Bu belgedeki dalga sayıları tarihli kanıttır; güncel sahip [modules.md](modules.md). İlk yeni paket ZoneCooler2,Arc merdiven/Discharge/Resonator,Lucky yan dönüşümü,Cold/Shatter kaldırma,Spare yansız,Barrier/Brace; Game build ve [212 kontrol](../build/e2e-artifacts/module-plan-transition-2026-10-13.json) geçti. Yeni ana/yan paketleri ayrı; eski ana-modül senaryoları yeni tasarımın kanıtı sayılmaz.

**Tarih:** 2026-10-06 · **P8 kapanışı:** 2026-10-08 · **Durum:** P0–P8 mevcut runtime dilimi doğrulandı; 18 ana modül + 30 yan özellik ve P7 sunumu mevcut. Oyun build exit 0; hedefli gerçek E2E 10/10. Eksik/onaysız yan özellikler §8'de açık; tam tasarım/ürün kabulü değildir. P2/P3/P4 `da12985` ile commit edildi; P5–P8 için stage/commit/push yapılmadı. · **Tasarım kaynağı:** [modules.md](modules.md), [Modül Sistemi Tasarımı](MODULE_SYSTEM_DESIGN.md), [Common uygulama planı](MODULE_COMMON_IMPLEMENTATION_PLAN.md).

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
| 18 Uncommon modül (7 tek boyutlu, 11 merdivenli) | Rare ve üstü modüller ([Rare uygulama planı](MODULE_RARE_IMPLEMENTATION_PLAN.md); Kor Birikimi = Ember Accumulation, Kırılma Şoku = Shatter Shock). Altın Kaplama, Zırh Dönüşümü, Kalıcı Etki Rezonatörü, Status Primer ve Arc Discharge 2026-10-10'dan beri Uncommon adayıdır ([modules.md](modules.md)), bu planın 18'lik setinde değildir. Enerji Kuplörü reddedildi |
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
- **Seviye tabloları:** bir kuralın sayısal alanı `[v1, v2, v3…]` listesi olabilir; çözümleme `level` ile seçer. P2 sözleşmesi: liste bildirilen seviye sayısını kapsamalıdır, kısa tablo yüklemede reddedilir; skaler sabittir. Kural şablonu ile değer tablosu ayrıdır.

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

### P5 — Status kuralı geçersiz kılma (doğrudan Codex)
- **2026-10-08 kullanıcı kapsamı:** P5 Codex tarafından, Luna/alt ajan kullanılmadan yapılır. Frost Lock hareket ve saldırıyı durdurur; Elite/MiniBoss/Boss muaf, normal hedefler etkilenir. Cold Retort donma yayılımı kullanıcı kararıyla **400 birim**; yalnız büyük varyantta, status-only ve yeni donma/yayılım zinciri üretmez. Thermal Shock Coil'in +2 saniye süre yan özelliği bu status dilimine dahildir; ölüm patlaması P6'da açık kalır.
- **Sahiplik:** `ModuleRulePackage::statusRules` dört ayrı tipli veri listesi taşır: Electric stack tablosu, Cryo freeze, Thermal+Cryo süre bonusu ve freeze spread. Ana/yan özellik aynı loader ve level resolver yolundan geçer. `ShipModuleApplier` çözülmüş değerleri kendi `CombatRuntime`'ına aktarır, sökme/teardown temizler; global registry yoktur. Global `damage_status_balance.json` değişmez.
- **Mevcut hedef status'u:** Electric target üzerindeki mevcut tek effect/ortak stack modelini korur. Son başarılı Electric stack uygulaması kendi sahibinin profilini taşır; daha düşük limite dönüşte fazla stack kesilir. Stack eklemeyen vuruş yalnız mevcut sahibin aynı tür vuruşunda süreyi/profili yeniler. Başka tür hasar uygulanmış Electric bonusunu okur; modül sökme mevcut effect'i anında silmez, sonraki kendi uygulaması normal profile döner.
- **Süre ve bekleme:** Thermal/Cryo süre bonusu vuruş öncesinde ikisi de varsa kendi yeni status uygulamasına/olağan aynı tür yenilemesine eklenir; tekrar +2 birikmez. Freeze stack'leri ve slow modifier'ını temizler; hedef effect'i freeze beklemesini taşır. Muaf veya control-immunity nedeniyle reddedilen donma stack tüketmez. Frozen status'un hareket sahibi `MovementComponent` mevcut stasis yolu üzerinden sürüklenme, burst ve zorunlu hareketi durdurur; saldırı sahibi mevcut ASC stun yoludur. Effect'e yalnız izin verilen `Status.*` ve `State.Effect.*` etiketleri verilir. Yeni davranış kaynakları/fallback effect kayıtları oyun ve E2E bağlantısındadır.
- Sahip başına max stack ve donma eşiği için `DamageTypeSystem` kancası (global denge dosyası korunur, sahip değeri üstüne biner).
- **Modüller:** Ark Genişletici (3), Kırağı Kilidi (2).
- **Kabul:** modülsüz hedefte stack sınırı değişmez; modüllü sahipte 5. Electric stack ve 4 Cryo stack'te donma + bekleme çalışır; modül sökülünce davranış geri döner.

- **Kanıt (2026-10-08):** vcvars64 sonrası `VSLANG=1033` ile oyun ve ilgili hedefler build exit 0. Mixed/Common/TriggerLifetime/P2/P3/P4/P5 gerçek runtime E2E **7/7**; P5 **373/373** kontrol. Beş Electric stack ve kaynak değişimi, seviye/reboot/sökme, donmada hareket/zorunlu hareket/saldırı kesme ve geri dönüş, hedef muafiyeti/control immunity, süre bonusu ve 400 birim zincirsiz yayılım, hatalı içerik reddi ve 10 tur temizleme kanıtlı. Global status denge dosyası hash'i aynı. Başlangıçtaki 18 ilgisiz path'in 17'si byte-identical; ModuleScreenView.cpp paralel ekran işiyle değişti, P5 bu dosyaya yazmadı. Stage/commit/push ve manuel görsel kabul yok. [Kesin komutlar ve sonuç kaydı](../beyin/notes/uncommon-p5-2026-10-08.md), [P5 artefaktı](../out/build/x64-debug/e2e-artifacts/uncommon-module-p5.json).

### P6 — Üretilen varlıklar (doğrudan Codex)
- Güdümlü mermi, sekme, gecikmeli patlama, mayın; zincirleme koruması ve teknik sınırlar (§3.4).
- **Modüller:** Sekme Ucu, Yayılım Çekirdeği (+ Artçı Darbe yan özelliği), İz Mayınları. Alt fazlar halinde, her biri ayrı commit.
- **2026-10-08 kullanıcı kararı:** Ricochet 300 birim ve atış başına tek ana sekme. Scatter az hedefte her farklı hedefe tek mermi, 600 birim/sn ve 3 sn ömür; Aftershock 100 yarıçap. Trail Mines, Movement sonrası 5 sn bırakma penceresi, 8/10/12 hasar ve 100 yarıçap. Thermal ölüm patlaması 100 yarıçap. Kullanıcı toplu taslağa “Bu değerlerle tamamla” yanıtını verdi. [Kanonik değerler](ABILITY_SCALING_CATALOG.md#uncommon-p6-üretilen-hasar).
- **Uygulama:** Üç ana modül, altı özel yan özellik ve Thermal Death Burst; tipli generatedRules + ortak loader/level resolver. Runtime lease, weak aktörler, RAII observer, tek atış/kullanım first-hit bütçesi; gerçek guided/mine/delayed actors. O4/O5/G1 ortak yan özellikleri önceki eksik kapsam olarak açık; JSON yalnız mevcut desteklenen ortak özellikleri açar. Default ability loadout ve global status dengesi değiştirilmez. P7 kart sunumu ayrı.
- **Kabul:** üretilen hasar ve aktarılan Ignite tick'leri başka modülü tetiklemez; config actor/lifetime sınırları aşılmaz; runtime aktif aktör sayısı E2E'de ölçülür. vcvars64 sonrası `VSLANG=1033` oyun/ilgili hedefler build exit 0; gerçek modül E2E 7/7 + GasLiteMixedLoadout 1/1 = **8/8**, P6 **972/972** kontrol. Üç seviye, gerçek Primary/beam, per-shot/use bütçesi, status transfer, actor cap/lifetime, mine yol/fuse/reboot, ölüm snapshot ve nested/10 tur temizlik kanıtlı. [Kesin komutlar ve sınırlar](../beyin/notes/uncommon-p6-2026-10-08.md), [P6 artefaktı](../out/build/x64-debug/e2e-artifacts/uncommon-module-p6.json). Alt ajan, stage/commit/push ve manuel görsel kabul yok.

### P7 — Sunum (doğrudan Codex)
- Kart metni seviye ve yan özelliği çözer; kilitli yan özellik gösterimi; merdiven boyutları modül ekranında. Türkçe glyph/font ve satır genişliği geçerli formatter/fontla ölçülür (Common P7 yöntemi). **Görsel kabul kullanıcıda.**
- Uygulama: gerçek modül seviyesi ve toplam seviye, mevcut hücre sayısı/merdiven, seçili yan özellik adı ve açıklaması kartta. Küçük boyutta kilit, en büyük boyutta envanterde hazır/takılıyken etkin, combat reboot sırasında bekleme gösterilir. Envanter satırı seviye ve yan özellik durumunu özetler; üzerine gelmek tam kartı açar.
- Sayılar `ModuleLevelTable::ValueAtLevel` ile okunur. `ModuleDescriptionValues` tipli kuralların numeric anahtarlarını hem JSON doğrulayıcısına hem formatter'a verir; ana ve yan açıklamalar aynı seviye tablosunu kullanır. İçerikte 18 ana ve 30 yan açıklama İngilizcedir; davranış/sayılar değişmez. Satırlar gerçek SFML font genişliğiyle sarılır; kısa envanter özeti kesilirse üç nokta alır, tam kartın ana/yan açıklaması E2E'de kesilmeden sığmalıdır.
- **Tamamlandı (2026-10-08):** Doğrudan Codex, alt ajansız. `LightYearsGame` ve ilgili hedefler `vcvars64` sonrası `VSLANG=1033` ile build exit 0. Gerçek P7 E2E 3212/3212: 18 modülün 363 seviye/boyut/yan özellik kombinasyonu, gerçek font ölçümü, Noto Sans Türkçe glyph'leri, loader reddi/geriye uyumluluk, kurma/taşıma/sökme/reboot, seviye ve yan seçim revizyonu, idle yenileme ve 10 tur cleanup. P7 + Common/TriggerLifetime/P2–P6/ModuleScreen/MixedLoadout toplam 10/10 E2E geçti. Yedi gerçek ekran görüntüsü incelendi; kart çerçevesinin alt tuş şeridine değmesi giderildi. [Komutlar ve sınırlar](../beyin/notes/uncommon-p7-2026-10-08.md), [artefakt](../out/build/x64-debug/e2e-artifacts/uncommon-module-p7.json). Stage/commit/push yok; kullanıcı görsel kabulü ve P8 ayrı.

### P8 — Kapanış (doğrudan Codex, tamamlandı)
- E2E dosyaları modül grubuna göre; hedefli E2E ve oyun build bağımsız doğrulanır; modules.md statüleri (Planned → Implemented) ve bu belgenin sonuç bölümü güncellenir.
- **2026-10-08:** Mevcut ayrılmış E2E dosyaları/CMake bağlantıları incelendi; yeni runtime/test davranışı eklenmedi. Game ve ilgili hedefler vcvars64 sonrası VSLANG=1033 ile build exit 0; Common/TriggerLifetime/P2–P7/ModuleScreen/MixedLoadout seri **10/10** geçti. JSON sayımları ve katalog uzlaştırıldı; 18 ana modül Implemented, eksik yan özellikler ayrı açık. [Kapsam ve kesin komutlar](../beyin/notes/uncommon-p8-2026-10-08.md), [sonuç §8](#8-p8-sonucu--2026-10-08).

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
| Kirli çalışma ağacı | P0 başlangıcı tarihsel olarak kapandı. P8 başlangıcı HEAD da12985; P5–P7 ve paralel görsel/şema işleri hâlâ dirty. P8 yalnız kendi belge/hafıza değişikliklerini yazdı; stage/commit/push yok. |
| Gold | Koşu içi kapsam kullanıcı tarafından onaylandı ve G1/Hunt Bounty eklendi; mağaza, temel düşüş, harcama ve kalıcılık ayrı. Güncel sonuç §9. |
| P8 sonrası yanlar | O4/O5/G1 ve dört özel yan tamamlandı; güncel kanıt §9, P8 başlangıcı §8 tarihsel. |
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


## 8. P8 sonucu — 2026-10-08

> Tarihsel checkpoint: aşağıdaki eksikler P8 bitimindeki durumu gösterir. Kullanıcının kalanları yapma isteği ve koşu içi Gold onayıyla sonraki kapsam §9’da tutulur.

Kullanıcının “p8 ile devam edelim” isteğiyle Codex doğrudan, alt ajansız doğruladı. P8, **mevcut Uncommon runtime diliminin ve sunumunun teknik kapanışıdır**; tüm tasarım yan özelliklerinin, ekonominin veya kullanıcı görsel kabulünün bittiği anlamına gelmez. Katalogdaki 18 ana modül Implemented olarak güncellendi; Rare/Epic tasarım satırları korundu.

### Doğrulanmış kapsam

- JSON'da 52 modül: 34 önceki kayıt + 18 Uncommon. Uncommon 7 tek boyutlu + 11 merdivenli; 30 yan özellik = 12 ortak + 18 özel. Shipped modüller üç seviyeli; altyapı N seviyeli tabloları destekler. İzinli kombinasyonlar ve kaynak kimlikleri mevcut loader/resolver sözleşmesinden gelir.
- P2 içerik/zar/runtime, P3 sahip durumu/öldürme, P4 hasar/komşuluk/tepki, P5 status/içerik, P6 sekme/aktör/mayın/içerik ayrı E2E dosyalarında; P7 tek 229 satırlık sunum E2E'sidir. En uzun Uncommon E2E .cpp 366 satır; gereksiz dosya bölme veya yeni test ekleme yapılmadı. Mevcut CMake test hedefleri ve game kaynak listesi bağlıdır.
- `LightYearsGame` ve ilgili 10 test hedefi build **exit 0**. `vcvars64` sonrasında `VSLANG=1033`; seri bağlama. İlk sandbox denemesi CMake glob kontrolünde ilerlemedi ve durduruldu; aynı yerel komut normal ortamda geçti.
- Common + TriggerLifetime + P2–P7 + ModuleScreen + GasLiteMixedLoadout gerçek E2E **10/10**, **41,03 sn**, exit 0. Unit test eklenmedi/çalıştırılmadı; tam suite/manuel gameplay iddiası yok. Artifact girdileri/çıktıları bağımsız okundu; kaynaklar ve kesin komutlar [P8 kaydında](../beyin/notes/uncommon-p8-2026-10-08.md).

| E2E artefaktı | Geçen kontrol |
|---|---:|
| [Common](../out/build/x64-debug/e2e-artifacts/common-modules.json) | 358 |
| [Trigger/lifetime](../out/build/x64-debug/e2e-artifacts/common-module-trigger-lifetime.json) | 41 |
| [P2](../out/build/x64-debug/e2e-artifacts/uncommon-modules.json) | 185 |
| [P3](../out/build/x64-debug/e2e-artifacts/uncommon-module-p3.json) | 385 |
| [P4](../out/build/x64-debug/e2e-artifacts/uncommon-module-p4.json) | 670 |
| [P5](../out/build/x64-debug/e2e-artifacts/uncommon-module-p5.json) | 373 |
| [P6](../out/build/x64-debug/e2e-artifacts/uncommon-module-p6.json) | 972 |
| [P7](../out/build/x64-debug/e2e-artifacts/uncommon-module-p7.json) | 3212; 363 seviye/boyut/yan kombinasyonu |

ModuleScreen kendi gerçek Player/Presenter/View artefaktını üretir; MixedLoadout ayrıca JSON kontrol sayısı yazdırmaz. Toplam test sayısına ikisi de dahildir.

### Açık kalanlar ve kabul sınırı

| Konu | Güncel sınır / sonraki iş |
|---|---|
| Ortak O4/O5 | Status süresi +1 sn ve hedefe ilk vuruş davranışı içerikte/izinlerde yok; ayrı uygulama işi. |
| Gold / G1 / Hunt Bounty | Gold ekonomisi tasarlanıp uygulanmadan açılamaz; JSON'da yok. |
| Glass Cannon düşük-canda kritik | Özel yan özellik içerikte yok. Mevcut Shield Return sabit +%6 Max Shield; cezanın yarısının dinamik hesabı değildir. |
| Grit koşullu shield regen | Ana hasar azaltma + Critical Circuit mevcut; koşullu ×2 regen yan özelliği yok. |
| Movement Rhythm sönme | 1,5 sn sönme önerisi onaysız; mevcut hız bonusu anlık, Advanced Thrust mevcut. |
| Loot / upgrade / kalıcılık | Drop dağılımı, harcama/upgrade UI ve serileştirme kapsam dışı; test hibesi loot kanıtı değil. |
| Son denge / görsel kabul | Tablolar uygulanmıştır, nihai denge kabulü değildir. P7'nin önceki görüntü incelemesi kullanıcı kabulü değildir; P8 yeni estetik kabul iddia etmez. |
| Simetrik şema ve hologram işi | Ayrı kullanıcı konusu: bonus-bölgesine sığma sorunu açık. P8 şekil/hologram kaynaklarını değiştirmedi. |
| Git | HEAD da12985; P5–P8 commit edilmedi. Commit için ayrıca açık kullanıcı onayı gerekir. |

Tam tasarım kapsamının bir sonraki teknik işi O4/O5 ve Glass/Grit eksik özel yanlarının ayrı sözleşmeyle uygulanmasıdır; Gold ve sönme tasarım kararlarına bağlı kalır. Runtime, denge, loadout, görsel ve diğer dirty işler P8'de değiştirilmedi. [Dar diff/hash kanıtı](../.tmp/p8/audit.json), [değişiklik sözleşmesi](../.tmp/p8/change-contract.md).


## 9. P8 sonrası eksik yanlar — 2026-10-08

Kullanıcı “yarım kalan işleri de yapalım” dedi; Gold için açık seçimi “Bu koşu içi kapsamla uygula.” Codex doğrudan, alt ajansız çalıştı. Kapsam yedi yandır: O4/O5/G1, Glass Desperate Aim, Grit Emergency Recharge, Hunt Bounty ve Movement Lingering Rhythm (1,5 sn). Ana değerler ve mevcut 30 yan korunur. İçerik 37 yan = 15 ortak + 22 özel; 11 merdivenli modülün her biri 2 özel + 3 ortak seçeneğe sahiptir.

Tipli kurallar, loader/level resolver, numeric kart anahtarları, runtime temizliği ve test/CMake bağlantıları birlikte uygulanır. [Sahiplik ve davranış sözleşmeleri](MODULE_SYSTEM_ARCHITECTURE.md), [sınırlandırılmış değişiklik sözleşmesi](../.tmp/uncommon-completion/change-contract.md).

**Teknik doğrulama tamamlandı:** `LightYearsGame` ve ilgili hedefler build exit 0; `vcvars64` sonrasında `VSLANG=1033`. Seri gerçek E2E **11/11**, **46,55 sn**, exit 0; 10 JSON artefaktında **7854** geçen kontrol. Yeni grup **959**, P7 **3860** kontrol ve **435** seviye/boyut/yan kombinasyonu. [Kesin build/test komutları ve kaynaklı sonuç](../beyin/notes/uncommon-completion-2026-10-08.md), [sabitlenmiş artefakt özeti](../.tmp/uncommon-completion/evidence.json), [başlangıca göre dar kapsam denetimi](../.tmp/uncommon-completion/audit.json). [Gruplanmış E2E girişi](../LightYearsGame/tests/UncommonModuleCompletionE2E.cpp) gerçek loadout/ship/damage/Player akışını kullanır. Mağaza, temel Gold düşüşü, loot/upgrade arayüzü, kalıcılık, nihai denge ve kullanıcı görsel kabulü açık; stage/commit/push yok.


## 10. Boyut ve hasar takip revizyonu — 2026-10-13

Arc Extender/Double Feed boyutları, Trail Mines AP formülü, Death Burst ve Hunt Fuel kaldırma [katalogdaki güncel karar](modules.md#boyut-ve-denge-revizyonu--2026-10-13) üzerinden uygulandı. Game build exit0; yalnız revision E2E379 ve Loot Shield kills E2E192 kontrol geçti. İkinci Loot Shield özel yanı kullanıcı onayıyla Kill Ward oldu (2026-10-13); eski P3/P8 sonuçları tarihli uygulama geçmişidir. Tam suite/oynanış kabulü değildir.
