# Tarihsel katalog — temizlik öncesi, 2026-10-13

Bu dosya tarihsel anlık görüntüdür; güncel karar sahibi [modules.md](../../modules.md). İçindeki eski seçenekler güncel kabul sayılmaz.

> **2026-10-23 açıklaması:** Bu arşivdeki Blink Drive/Epic satırı geçersiz eski sınıflandırmadır. Güncel karar **Legendary: tek sabit boyut, yan yok, hiçbir bölge bonusu yok**. [Güncel Blink Drive kaydı](../../modules.md#legendary-modüller--3).

# Modules

**Tarih:** 2026-10-08 · **Amaç:** Oyunda veri olarak tanımlı modülleri ve tasarım aşamasındaki modül fikirlerini tek yerde izlemek. · **Durum:** Canlı oyun verisi ile planlar ayrılmıştır; Uncommon ana özellikler ve P8 sonrası yedi eksik yan doğrulandı. Rare: 16 modül ve 35 yan özellik canlı içerikte (dalga 1–7, 2026-10-12); F1 Parçalama çekirdeği uygulandı; Sonic Boom yerine Slipstream Control (+%25 sonik dönüş) geldi (2026-10-12); Rare sunumu ve Overflow Transfer açık.

Modül kurallarının sahibi [Modül Sistemi Tasarımı](../../MODULE_SYSTEM_DESIGN.md); 11 standart Common modülün tam değer ve davranış sözleşmesinin sahibi [Common uygulama planı §2](../../MODULE_COMMON_IMPLEMENTATION_PLAN.md#2-modül-şartnamesi), çalıştırılan içerik kaynağı [modules.json](../../../LightYearsGame/assets/content/data/modules.json)'dır. Bu katalog fikir/status izleme ve hızlı bakış içindir.

## Durum etiketleri

| Durum | Anlamı |
|---|---|
| **Implemented · Common** | Standart Common içeriğinde ve uygulama planında yer alıyor. |
| **Implemented · Uncommon** | Ana özellik ve satırda belirtilen yan özellikler canlı içerikte; P8 sonrası yedi eksik yan dahil oyun build ve hedefli gerçek E2E ile doğrulandı; loot/ekonomi, nihai denge ve kullanıcı görsel kabulü anlamına gelmez. |
| **Implemented · Rare** | Ana özellik ve yan özellikler `modules.json` / `module_side_features.json` içinde; ilgili Rare E2E'si (`Rare*E2E`) geçiyor. Denge, loot, sunum (R8) ve kullanıcı görsel kabulü anlamına gelmez. |
| **System · default** | JSON'da geminin takılı sistem modülü olarak tanımlı. |
| **System · variant** | JSON'da sistem modülü varyantı olarak tanımlı. Kod/veri kaydı bulunması, ödül havuzunda edinilebilir olduğunu tek başına göstermez. |
| **Defined · review** | JSON tanımı var; güncel Common setinin dışında. Edinilebilirlik ve güncel denge ayrıca doğrulanmadı. |
| **Planned** | Fikir/modül yönü listeye alındı; değer, şekil, nadirlik veya runtime uygulaması hâlâ açık olabilir. Oyun içi değildir. |
| **Reworking** | Beğenilen bir fikir var; kullanıcı geri bildirimiyle tetik ya da davranış yeniden tarif ediliyor. Oyun içi değildir. |
| **Review** | Önceki öneri; kullanıcıdan seçim veya düzeltme gelmedi. Oyun içi değildir. |
| **Rejected** | Kullanıcı öneriyi reddetti; kapsam dışı tutulur. |

`■■` iki hücreli yatay domino, `■■■` üç hücreli yatay I, `■` tek hücredir. L şekli iki satırda gösterilir. Mevcut veri şekilleri 90° döndürülebilir. Planlarda şekil belirtilmemişse `Açık` yazar. Önerilerde belirtilen sayılar kullanıcının önerisi veya ilk fikir taslağıdır; durum hücresi uygulama ya da nihai denge onayı anlamına gelmez.

## Sayısal envanter — 2026-10-13

**Ana havuz: 49 eklenmiş + 39 planlı/yeniden tasarım = 88 modül kaydı.** Bir modül bir kez sayılır; boyut varyantı, seviye, parça ve yan özellik ayrı modül değildir. Dört Hunt Clock aile sürümü JSON'da ayrı kimlik olduğundan dört kayıt sayılır. Eklenmiş sayımı güncel `modules.json` ile katalogdaki 11 standart Common, 22 Uncommon ve 16 Rare üzerinden doğrulandı; bu, loot/edinilebilirlik veya nihai denge kabulü değildir.

Planlı sayımı kullanıcı tarafından tutulan adayları içerir; öneri nadirlikler ilgili aday basamağında gösterilir, kesin nadirlik onayı sayılmaz. Combo Sequencer yeniden tasarım olarak dahil; Pathfinder Array kullanıcının Movement sınıflandırmasıyla dahil, ayrı modül biçimi/ayrıntıları açık. Survey Relay ve Combo Sequencer iki nadirlikte birden sayılmaz; ayrı belirsiz satırda bir kez sayılır. Monochrome Core (Review), Circle Spirit (açık aday), diğer seçilmemiş/kararsız fikirler, ertelenenler, reddedilenler ve yan özellikler bu 88'e dahil değildir.

### Eklenmiş ana modüller · 49

| Nadirlik | Primary | Offense | Defense | Movement | Utility | Yetenek modülü | Ailesiz | Toplam |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Common | 2 | 4 | 2 | 2 | 1 | 0 | 0 | 11 |
| Uncommon | 2 | 6 | 6 | 4 | 4 | 0 | 0 | 22 |
| Rare | 0 | 5 | 3 | 3 | 5 | 0 | 0 | 16 |
| **Toplam** | **4** | **15** | **11** | **9** | **10** | **0** | **0** | **49** |

### Planlı / yeniden tasarım modülleri · 39

| Nadirlik | Primary | Offense | Defense | Movement | Utility | Yetenek modülü | Ailesiz | Toplam |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Common | 0 | 0 | 2 | 0 | 0 | 0 | 0 | 2 |
| Uncommon | 1 | 2 | 3 | 0 | 0 | 0 | 0 | 6 |
| Rare | 0 | 0 | 3 | 2 | 4 | 0 | 0 | 9 |
| Epic | 2 | 3 | 3 | 2 | 5 | 0 | 0 | 15 |
| Legendary | 1 | 0 | 0 | 0 | 1 | 0 | 0 | 2 |
| Uncommon / Rare açık | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 1 |
| Epic / Legendary açık | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 1 |
| Nadirlik açık | 0 | 0 | 0 | 1 | 0 | 1 | 1 | 3 |
| **Toplam** | **4** | **5** | **11** | **5** | **11** | **2** | **1** | **39** |

### Eklenmiş + planlı · 88

| Nadirlik | Primary | Offense | Defense | Movement | Utility | Yetenek modülü | Ailesiz | Toplam |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Common | 2 | 4 | 4 | 2 | 1 | 0 | 0 | 13 |
| Uncommon | 3 | 8 | 9 | 4 | 4 | 0 | 0 | 28 |
| Rare | 0 | 5 | 6 | 5 | 9 | 0 | 0 | 25 |
| Epic | 2 | 3 | 3 | 2 | 5 | 0 | 0 | 15 |
| Legendary | 1 | 0 | 0 | 0 | 1 | 0 | 0 | 2 |
| Uncommon / Rare açık | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 1 |
| Epic / Legendary açık | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 1 |
| Nadirlik açık | 0 | 0 | 0 | 1 | 0 | 1 | 1 | 3 |
| **Toplam** | **8** | **20** | **22** | **14** | **21** | **2** | **1** | **88** |

### Ana havuz dışındaki JSON kayıtları · 23

Bu kayıtlar JSON'dadır; ana havuzdaki 49'un üzerine eklendiğinde JSON toplamı **72** olur. Sistem modüllerine nadirlik atanmadığından Common/Rare gibi bir basamağa zorlanmazlar. Eski dokuz tanımın mevcut JSON'da bulunması standart Common seçimine dahil oldukları anlamına gelmez.

| Nadirlik | Primary | Offense | Defense | Movement | Utility | Yetenek modülü | Ailesiz | Toplam |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Sistem · varsayılan | 1 | 0 | 1 | 2 | 7 | 0 | 0 | 11 |
| Sistem · varyant | 0 | 0 | 2 | 1 | 0 | 0 | 0 | 3 |
| Eski tanım · review | 2 | 2 | 1 | 1 | 3 | 0 | 0 | 9 |
| **Toplam** | **3** | **2** | **4** | **4** | **10** | **0** | **0** | **23** |

**Geniş kayıt toplamı:** 72 JSON kaydı + 39 planlı = **111**. Bunların 88’i ana modül havuzu, 23’ü sistem/eski tanım kaydıdır. Beğenilip ertelenen Scavenger Net ve Retreat Beacon ayrıca **2** kayıttır; aktif plan toplamına dahil değildir.

### Havuz sadeleştirme taslağı — 2026-10-13

**Kullanıcı yönü:** Eklenmiş ve planlı modüller birlikte değerlendirilecek; kalabalık gruplarda bazı etkiler yan özelliğe dönüştürülebilir, zayıf adaylar şimdilik ertelenebilir. Uncommon Defense için örnek hedef 7; Rare Utility, Rare Defense ve Epic de incelenecek. İlk sekizli öneri kullanıcı geri bildirimiyle revize edildi: Gold Plating ve Armor Conversion bağımsız modül kalır; Tempered Hull yerine Shatter Shock eleme/dönüşüm adayıdır. Diğer beş seçim **ajan önerisidir, kullanıcı tarafından henüz seçilmedi**; yukarıdaki mevcut 88 kayıt sayımı geçerlidir. Grupların eşit sayıda olması zorunluluğu yoktur; ayırt edici build kararı ve bir modülün hücre maliyetine değmesi esas alınır. Bu taslak runtime/içerik kaldırmaz.

| Grup | Şimdi | Önerilen | Bağımsız modülden çıkarılacak adaylar |
|---|---:|---:|---|
| Uncommon Defense | 9 | 9 | Gold Plating ve Armor Conversion bağımsız kalır; yeni eleme seçilmedi |
| Rare Utility | 9 | 6 | Monoculture, Four Corners, Interest Ledger → yan özellik adayları |
| Rare Defense | 6 | 5 | Shatter Shock → kaldırma veya yan özellik; biçim açık |
| Epic Defense | 3 | 2 | Field Medic → yan özellik adayı |
| Epic Utility | 5 | 4 | Danger Pay → yan özellik adayı |
| **Ana havuz** | **88** | **82** | **6 bağımsız modül azalır; yalnız Shatter Shock eleme yönü kullanıcıdan, kalan 5 öneri** |

| Aday | Önerilen yön | Gerekçe ve sınır |
|---|---|---|
| Monoculture | Utility ortak yan özelliği | Üç aynı aile koşulu korunabilir; ayrı bir cooldown modülü yerine build seçimini destekleyen yan olur. ×0,85 yeniden ölçülür; tüm yeteneklere çalışan yanın kapsamı açıkça tanımlanmalıdır. |
| Four Corners | Utility ortak yan özelliği | Dört farklı aile koşulu korunabilir; Monoculture'ın alternatif build yönü olur. ×1,15 yeniden ölçülür; Defense/Utility'de etki kapsamı zaten açık olduğundan hâlâ çözülmelidir. İkisi tek modülde birlikte rastgele atanmaz. |
| Interest Ledger | Rare ve üstü ekonomi ortak yan özelliği | Nokta sonu Gold faizi başka koşu modüllerine eşlik edebilir. Zayıf olduğu iddia edilmez: sınırsız bileşik kazanç güçlüdür. Kullanıcının üst sınır istememesi korunur; %5 yan özellik için yeniden ölçülür. |
| Shatter Shock | Bağımsız modülü kaldırma veya Defense yan özelliğine dönüştürme | Kullanıcı kullanımı zor buldu ve Tempered Hull yerine bunu seçti (2026-10-13). Yan özellik biçimi, gücü ve taşıyıcı modül açık; eski Stagger/Aftershock Wave yanlarının akıbeti ayrıca çözülür. Canlı JSON kaydı bu tur kaldırılmadı. |
| Field Medic | Defense ortak yan özelliği | Defense kullanımından süreli iyileştirme, mevcut haliyle Epic bağımsız hücre maliyetine göre ince bir etki. İyileşme fikri korunur; oran ve toplam/hız semantiği netleştirilir. Nadirlik ve hangi modüllerde bulunacağı açık. |
| Danger Pay | Rare ve üstü ekonomi ortak yan özelliği | Yüksek Threat'te ödül artışı, koşu modüllerini tamamlayan pasif olabilir. Threat 3/+%25 taslağı yeniden ölçülür; yüksek Threat'i seçme yönü ve ekonomi bağımlılığı korunur. |

**Kalanlar (revize taslak):** Uncommon Defense: Reflection Shield, Health Bridge, Loot Shield, Cold Retort, Grit Your Teeth, Hunt Clock: Defense, Overshield Layer, Gold Plating, Armor Conversion (9). Rare Utility: Zone Overclocker, Misfit Wiring, Triple Core, Stripped Hull, Spare Capacitor, Release Valve (6); Silent Running artık Movement. Rare Defense: Continuous Field Generator, Impact Redirector, Bastion Slam, Tempered Hull, Polar Shield (5). Epic Defense: Last Stand, Shield Battery (2). Epic Utility: Cascade Reactor, Confluence Core, Storm Capacitor, Field Mastery (4).

**Shatter Shock geçişi:** Kullanıcı eleme/dönüşüm yönünü seçti; canlı içerik hâlâ 49 ana modül, tasarımda Shatter Shock çıkarılınca 48 tutulan eklenmiş + 39 planlı = **87** olur. Rare Defense hedefi 6→5. Yan özellik veya tam kaldırma biçimi seçilmeden canlı modül/yanları silinmedi.

**Yan özellik entegrasyonu açık:** Dönüşüm seçilirse izin listesi ve nadirlik gücü ayrıca hazırlanır; mevcut 2 özel + 3 ortak = 5 seçenek sözleşmesine altıncı seçenek kendiliğinden eklenmez. Sabit boyutlu modüllere yan özellik eklenmez. Gemi modülü yan özellikleri ile Yetenek modülü sınıfı/kabuk yanları ayrı tutulur. Listeyi başka sınıfa taşımak toplamı azaltmadığından eleme yerine sayılmaz. Cascade Reactor/Combo Sequencer yakınlığı ayrı bir birleşme değerlendirmesidir; bu altılı sayımda birleşmiş kabul edilmez.

### Kill Ward — 2026-10-13

Kullanıcı Kill Ward önerisini açıkça onayladı. Loot Shield’ın özel yanları Kill Ward ve Overflow Recovery; ortak izinleri D1/D2/D4. Kill Ward yalnız sahibin geçerli öldürmesinde çalışır; maksimum boyut altında kilitli, çıkarılınca süreli hasar azaltma temizlenir. Mevcut Fleet Guard `incoming` + `KillConfirmed` süre penceresi sözleşmesiyle veri olarak tanımlıdır; yeni runtime davranışı gerekmedi. Game build exit0; yalnız Kill Ward E2E **108 kontrol** geçti. [Kanıt](../../../build/e2e-artifacts/loot-shield-kill-ward-2026-10-13.json).

### Boyut ve denge revizyonu — 2026-10-13

Kullanıcı kararları: Arc Extender sabit **1 hücre**, Double Feed Core sabit **2 hücre**; mevcut güç/seviye değerleri korunur. Thermal Shock Coil / Death Burst **400 yarıçap ve hedef MaxHealth'inin %15'i**; iki statü koşulu ve zincirleme yasağı korunur. Trail Mines L1 **30 + AP'nin %30'u Kinetic**; L2 **35 + %35**, L3 **40 + %40** ajan gelişim seçimi. AP mayın bırakılırken örneklenir; sonraki AP değişimi bırakılmış mayını değiştirmez. Normal outgoing hasar çarpanı mevcut vuruş yolunda korunur; kritik/diğer modül tetikleri kapalıdır. Hunt Fuel kaldırıldı; kalkan yenilenmesi +%30/3 sn alternatifi reddedildi. **Kill Ward kullanıcı tarafından onaylandı ve eklendi:** öldürünce 2 sn alınan hasar ×0,90, tekrar öldürmede süre yenilenir; 3 hücrede açılır, üç modül seviyesinde aynı güçte, birikmez. Silent Running **Movement**, Omni Thrusters mevcut **Uncommon** kaydını korur. Gold Plating ve Armor Conversion yan özelliğe çevrilmez. Tempered Hull'ın bağımsız tasarımı korunur; yerine Shatter Shock eleme/dönüşüm yönü seçildi. Game build exit0; yalnız boyut/mayın/patlama E2E 379 kontrol ve Loot Shield kill E2E 192 kontrol geçti. [Boyut/mayın/patlama kanıtı](../../../build/e2e-artifacts/module-sizing-mines-burst-2026-10-13.json), [Loot Shield kanıtı](../../../build/e2e-artifacts/loot-shield-no-hunt-fuel-2026-10-13.json). Tam suite ve kullanıcı oynanış kabulü değildir.

## Oyunda tanımlı modüller

`modules.json` içinde **72 kayıt** vardır: 11 varsayılan sistem, 3 sistem varyantı, 11 standart Common, 9 eski Common dışı tanım, 22 Uncommon ve 16 Rare (2026-10-13). Uncommon ana/yan özellik sözleşmeleri aşağıdaki kendi tablolarındadır; güncel doğrulama [P8 sonrası sonuç §9](../../MODULE_UNCOMMON_IMPLEMENTATION_PLAN.md#9-p8-sonrası-eksik-yanlar--2026-10-08) içindedir. Aşağıdaki “tanımlı” statüsü JSON'daki varlığı gösterir; dağıtım/loot havuzunda çıkma kanıtı değildir.

### Varsayılan sistemler · 11

| Modül | Şekil | Aile / sistem rolü | Durum |
|---|---:|---|---|
| Standard Shield Generator | `■■` | Defense · Shield | System · default |
| Standard Afterburner | `■■` | Movement · Afterburner | System · default |
| Health Readout | `■` | Utility · HealthReadout | System · default |
| Damage Readout | `■` | Utility · DamageReadout | System · default |
| Life Support Unit | `■` | Utility · LifeSupport | System · default |
| Flight Control Unit | `■■` | Movement · FlightControl | System · default |
| Primary Weapon | `■■` | Primary · PrimaryWeapon | System · default |
| Q Ability Slot | `■` | Utility · AbilitySlot1 · Ability Bay | System · default |
| E Ability Slot | `■` | Utility · AbilitySlot2 · Ability Bay | System · default |
| F Ability Slot | `■` | Utility · AbilitySlot3 · Ability Bay | System · default |
| R Ability Slot | `■` | Utility · AbilitySlot4 · Ability Bay | System · default |

**Ability Bay (2026-10-10, kullanıcı kararı):** Q/E/F/R kabukları 1 hücredir ve geminin şemasında yatay tek sırada, **Ability Bay** adlı 4 hücrelik özel bölgede durur (sırayla Q, E, F, R). Bölge yalnız bu dört kabuğa açıktır: başka modül giremez, kabuklar da bölgeden çıkamaz. Bir kabuğu başka bir kabuğun hücresine bırakmak ikisinin yerini değiştirir. Bölge bonusu yoktur. Bölgeyi çevreleyen hücrelerdeki modüller (Health Bridge, Trophy Core gibi) komşuluğu yalnız bu sıradaki kabuklarla kurar. Bölge şemada 4 hücre kaplar; varsayılan modüller sonrası boş hücre sayısı **25**, toplam hücre **40**dır (kullanıcı kararı, 2026-10-10). Bölge x3–x6 hücrelerinde yatay tek sıradır (satırı gemiye göre y2 veya y3 civarıdır). **Gemiler uçak gibidir (2026-10-10, kullanıcı kararı):** Varsayılan modüller 12 hücre tutar (Primary Weapon, Life Support, Flight Control ve Afterburner 1 hücre; Shield 2, readout 2, Q/E/F/R kabukları 4). **Fighter** 16x10, kullanıcının çizdiği şema; **kilitli alanlar** (çizimde büyük harf) gemi silüetinin ve bölgenin parçasıdır ama şimdilik yalnız görüntüsel: üzerlerine modül yerleştirilemez ("covers a locked area"), ekranda soluk ve asma kilitli çizilir. Kilidin nasıl açılacağı henüz tasarlanmadı. Fighter'da açık hücreler: Primary 4, kanat 8, Hull 6, Core 8, Movement 4, Utility 4, Ability Bay 4; kilitli 16 (Primary 2, Utility 2, kanat 4, Hull 4, Core 2, Movement 2). Serbest hücre (açık, varsayılan dışı): Primary 2, kanat 8, Hull 6, Core 6, Movement 2, Utility 2 = 26. Şema JSON'unda `lockedLegend` kilitli karakterleri tanımlar. Q/E/F/R modülleri tek renklidir (Ability Bay sarısı). Toplam/serbest hücre sayısı artık sabit bir kural değil. Diğer beş gemi tek sayılı genişlikte ve eski çizimlerinde.

### Sistem varyantları · 3

| Modül | Şekil | Etki | Durum |
|---|---:|---|---|
| Fast-Charge Generator | `■■` | Shield capacity ×0.80; shield regeneration ×1.50 | System · variant |
| Heavy Generator | `■■■` | Shield capacity +60; regeneration +30%; recharge delay −2 sec | System · variant |
| Extended Afterburner | `L` · 3 hücre | Afterburner capacity +50 | System · variant |

### Standart Common · 11

| Modül | Aile | Şekil | Etki | Durum |
|---|---|---:|---|---|
| Heavy Armor Plate | Defense | `■■■` | Armor +30 | Implemented · Common |
| Assault Core | Offense | `■■` | Attack Power +15 | Implemented · Common |
| Focused Barrel | Primary | `■■` | Primary damage +30% | Implemented · Common |
| Thrust Booster | Movement | `■■` | Horizontal and vertical movement rating +2.2 | Implemented · Common |
| Attack Rhythm | Offense | `■■` | On Offense use, Attack Speed +15% for 3 sec; does not stack, duration refreshes | Implemented · Common |
| Shatter Current | Defense | `■■` | When the shield breaks, Primary hits deal additional Energy damage equal to 30% of hit damage for 5 sec | Implemented · Common |
| Energy Feedback | Offense | `■` | On dealing Energy damage, Energy Power +16 for 4 sec; does not stack, duration refreshes | Implemented · Common |
| Escape Reflex | Movement | `■■` | On taking damage, movement speed ×1.25 for 2 sec; does not stack, duration refreshes | Implemented · Common |
| Finishing Blow | Offense | `■■` | Damage you deal ×1.20 while the target is at 25% health or below | Implemented · Common |
| Zone Cooler | Utility | `■■■` | In a single-family zone, that family's ability cooldown ×0.79; in Hull, active non-Primary abilities ×0.90; no bonus in Nose | Implemented · Common |
| Full Magazine | Primary | `■■` | The first 3 successful Primary shots in each magazine deal ×1.30 damage | Implemented · Common |

Standart Common listesinde Energy Coil yerine Assault Core bulunur. **Energy Coil JSON tanımında korunmuştur**; aşağıdaki Common dışı tanımlar bölümündedir.

### Common dışı JSON tanımları · 9

Bu tanımlar içerik dosyasında duruyor; 11 standart Common listesine dahil değiller. Şeklin satır dizilimi özgün hücre koordinatlarına göre gösterilir.

| Modül | Aile | Şekil | JSON etkisi | Durum |
|---|---|---:|---|---|
| Piercing Tip | Primary | `■■` | Primary projectile Piercing Count +1 | Defined · review |
| Shot Accelerator | Primary | Domino (2) / L (3) | 2 cells: fire rate ×1.10; 3-cell L: ×1.14 | Defined · review |
| Attack Matrix | Offense | Domino (2) / I (3) | 2 cells: ability damage ×1.12; 3-cell I: ×1.16 | Defined · review |
| Overload Core | Offense | `■■`<br>`■■` | Ability damage ×1.25; cooldown ×1.10 | Defined · review |
| Hull Reinforcement | Defense | Domino (2) / L (3) | 2 cells: max health +40; 3-cell L: +55 | Defined · review |
| Maneuver Thruster | Movement | `■` | Horizontal movement rating +1.5 | Defined · review |
| Cooling Circuit | Utility | Domino (2) / L (3) | 2 cells: Ability Haste +8; 3-cell L: +11 | Defined · review |
| Energy Coil | Utility | `■■` | Energy Power +24 | Defined · review |
| Targeting Computer | Utility | Single (1) / domino (2) | 1 cell: Critical Chance +3; 2 cells: +5 | Defined · review |

## Tasarım kuyruğu

Aşağıdaki kayıtlar kullanıcı geri bildirimini, uygulanan Uncommon içeriği ve kalan tasarım kuyruğunu birlikte tutar. 18 Uncommon ana modül P2–P6 ile runtime'a eklendi; P7 sunumu ve P8 kapanışı doğrulandı. Rare/Epic fikirleri ve belirtilen eksik yan özellikler tasarım aşamasındadır.

### İçerik ve tasarım havuzu

Kullanıcının beğendiği fikirler, ilettiği taslaklar ve 2026-10-05/06 toparlama turlarının sonucu aynı havuzda tutulur. Uygulanan ana/yan özellikler ve kalan eksikler satırlarda belirtilir. P8 mevcut runtime diliminin kapanışıdır; tüm tasarım önerilerinin uygulandığı anlamına gelmez. Uygulama planı: [MODULE_UNCOMMON_IMPLEMENTATION_PLAN.md](../../MODULE_UNCOMMON_IMPLEMENTATION_PLAN.md). Bu ilk sürümdür; değerler tablo biçimli ve genişletilebilir, seviye sayısı, yan özellik ağırlıkları ve yükseltme harcaması henüz karar değildir. Uncommon set **18 modül**: 7 sabit boyutlu, 11 değişken boyutlu. Rare uygulama planı: [MODULE_RARE_IMPLEMENTATION_PLAN.md](../../MODULE_RARE_IMPLEMENTATION_PLAN.md). Rare 18 modül (Armor Conversion, Status Resonator, Gold Plating, Status Primer ve Arc Discharge Uncommon adayına taşındı, 2026-10-10); Epic/Legendary ana aday tablosunda 14 kayıt; Broadside Mount açık tasarım bölümünde.

**Kurallar (kullanıcı, 2026-10-05/06):**
- Uncommon yeni varlık (mermi, mayın vb.) üretebilir; ama az üretir ve az vurur.
- Düz stat modülü olabilir; değeri nadirliğine uygun olmalı. Sayılar Common ×1,3 ölçütüyle yazılmış **öneridir**, onaylı denge değildir.
- Modüller gelişecek. Gelişim sütunu, seviye başına artan parametreyi ve örnek 3 adımlık değerleri gösterir. Seviye sayısı ve formül karara bağlanmadı.
- Üretilen mermi, patlama ve sekme başka modülü tetiklemez. Hasar sırası: modül çarpanları, sonra kritik.
- Birimler: kritik şansı yüzde puanıdır (Targeting Computer +3/+5). Kritik hasar çarpanı `Owner.CriticalDamage`, tabanı 1,5. Attack Speed rating'i yaklaşık %1 atış hızıdır (`GDD.md`). Hız tavanı gemiye göre 480–590 birim/sn.
- Ana özelliklere denge amaçlı üst sınır konmaz (kullanıcı, 2026-10-06); başlangıç değerleri küçük tutulur, güç gelişimle artar. Mekanik/teknik sınırlar (Electric stack üst sınırı, kalkanın maksimumu, performans için mayın sayısı) bundan ayrıdır.
- **Sabit boyutlu modül** tek bir hücre boyutunda gelir; 1, 2 veya 3 hücre kaplayabilir ve yan özellik taşımaz. **Değişken boyutlu (boyut merdivenli) modül** birden fazla hücre boyutunda sunulur (ör. 1-2-3, 1-3 veya 2-3); yan özellik en büyük boyutta açılır.

#### Boyut merdiveni ve yan özellik sistemi (tasarım yönü, kurallar öneri)

- **Merdivenler:** 1-2-3, 1-2, 1-3, 2-3. Boyut hem yükseltmeyle hem loot'ta rastgele değişir.
- **Ana özellik (A) sabittir.** Boyutla sayısal olarak büyüyebilir; belirtilmeyen boyutlarda aynı kalır.
- **Yan özellik, modülün en büyük boyutunda açılır.** Gücü, o boyuta eklenen hücre sayısıyla ölçeklenir.
- **Yan özellik rastgeledir, hep seçilen bir şey değildir:** bir loot'ta A modülü B yan özellikle, başka loot'ta C yan özellikle düşebilir. Tasarım hedefi her değişken boyutlu modül için 5 olası yan özelliktir: 2 kendine özel + ortak havuzlardan 3. İzin listelerinde her değişken boyutlu Uncommon için 2 özel + 3 ortak olmak üzere 5 seçenek vardır; Loot Shield’da kaldırılan Hunt Fuel yerine Kill Ward eklendi (kullanıcı onaylı, 2026-10-13).
- **Düşerken** yan özellik atanır ve sabittir; küçük boyutta düşse bile kart "kilitli" gösterir, büyütünce açılır. **Yükseltmede** özel bir harcamayla havuzdan 3 rastgele teklif görülüp biri seçilebilir (öneri; harcama biçimi açık).
- **Parçalama (kullanıcı, 2026-10-08):** Rare ve üstü değişken boyutlu modüllerde ortak yan özellik seçeneği F1. Her zaman tek hücrelere bölünür (kısmi 2+1 seçimi yok); yan yana konan parçalar otomatik bağlanır. Bölge bonusu orantılıdır. Komşuluk modüllerinde parça başına bitişiklik sayılır ve sınır konmaz; oyuncu serbest, oyunu kırabilmesi kabul edilir. Parça yerleşimi için veri/ızgara/kayıt değişikliği maliyeti kabul edildi.
- Common'da yan özellik yoktur. **Rare, Uncommon tasarımını sürdürür (kullanıcı, 2026-10-08):** sabit boyutlu Rare'de yan özellik yok; değişken boyutlu Rare'de en büyük boyutta 1 yan özellik, 2 özel + 3 ortak seçenek; şekiller Rare'e uygun seçilir. Önceki "Rare'de iki yan özellik" önerisi geçersiz. Yan özelliklerin güç düzeyi eşit tutulur.
- Kayıt/yükleme tasarım hedefi: modül kimliği, boyut seviyesi ve yan özellik kimliği saklanacak; yeniden yüklemede değişmeyecek. Şimdilik bu alanlar yalnız runtime verisi; kalıcılık sistemi uygulanmadı.
- **Yan özelliklerin gelişimi (öneri, karar bekliyor):** modül seviyesiyle yan özellik de büyür, ana özelliğin yaklaşık yarı hızında. Ortak havuz özelliklerinin 3 adımlık değerleri bir kez tanımlanır; özel yan özelliklerin değerleri modül başına yazılır.

#### Ortak yan özellik havuzları (öneri değerler)

| Havuz | Kimlik | Özellik | Runtime durumu |
|---|---|---|---|
| Offense (O) | O1 | Attack Power +8 | Implemented |
| | O2 | Critical Chance +8% | Implemented |
| | O3 | Attack Speed +4% (sabit, tetiksiz) | **Kullanıcı kararı (2026-10-10):** süreli "öldürünce AS" kaldırılıyor çünkü zamanlı AS veren modül/yan özellik zaten var (Attack Rhythm, Critical Rhythm). Kodda uygulandı: `Shared.O3` artık sabit Owner.AttackSpeed +4 (3 seviyede de aynı), adı "Offensive O3". Sharp Scale ve Scatter Core O3 kullanır |
| | O4 | Your applied damage status durations +1 sec; includes Frozen, excludes freeze cooldown | Implemented |
| | O5 | Your first successful hit against each target ×1.15 | Implemented · blocked/zero damage does not consume it |
| Defense (D) | D1 | MaxHealth +25 | Implemented |
| | D2 | On taking damage, Armor +12 for 2 sec (does not stack; duration refreshes) | Implemented |
| | D3 | On kill, restore health equal to 1% of MaxHealth | Implemented |
| | D4 | Shield regeneration rate ×1.15 | Implemented |
| | D5 | Max Shield +8% | Implemented |
| | D6 | *Desperate Patch:* when a Defense ability is used while health is at or below 40%, restore health equal to 12% of MaxHealth | Planned · kullanıcı, 2026-10-13 · modül değil yan özellik (Defense havuzu) · %40 eşik ve %12 ajan önerisi |
| General (G) | G1 | On owner kill, +1 Gold for this run | Implemented · resets on new run |
| | G2 | Ability Haste +4 | Implemented |
| | G3 | Afterburner capacity +10% | Implemented |
| | G4 | Horizontal and vertical movement rating +0.8 | Implemented |
| | G5 | Luck +1 | Implemented |
| | G6 | *Homing Beacon:* returning to a completed node adds no Threat | Planned · kullanıcı, 2026-10-13 · yolculuk yan özelliği; yeniden ziyaret kuralı GDD'de açık karar olduğundan onunla birlikte kesinleşir |
| | G7 | *Wayside Repairs:* on arriving at a new node, restore health equal to 6% of MaxHealth (same for every node type) | Planned · kullanıcı beğendi, 2026-10-13 · modül değil yan özellik (ajan önerisi, bir hücreye değmeyecek kadar küçük) · %6 ajan önerisi; nokta türüne göre ikiye katlama istenmedi |
| | G8 | *Quartermaster:* shop prices −15%; one free reroll per shop | Planned · kullanıcı beğendi, 2026-10-13 · modül değil yan özellik (ajan önerisi) · −%15 ve yenileme ajan önerisi |
| | G9 | *Salvage Processor:* dismantling unwanted modules yields more Scrap | Planned · kullanıcı fikri, 2026-10-13 · modül değil yan özellik (ajan önerisi) · parçalama mekaniğinin varlığı, çarpan ve Scrap kaynağı açık |
| Zone (Z) · öneri | Z1 | *Overclocked Seat:* while fully inside its family's zone, this module's zone bonus gets +10 points (+25% → +35%; with Zone Overclocker 50% → 60%). Only this module | Planned · kullanıcı, 2026-10-08 · ana modülden ayrışsın diye zayıflatıldı |
| | Z2 | *Misfit Seat:* while outside its family's zone, this module gets +10% (with Misfit Wiring 25% → 35%). Only this module | Planned · kullanıcı, 2026-10-08 · ana modülden ayrışsın diye zayıflatıldı |
| Fragment (F) · yalnız Rare ve üstü | F1 | *Fragmentable:* at max size the module splits into single cells (3 → 1+1+1, 2 → 1+1) that can be placed anywhere on the grid; fragments placed side by side auto-connect. Zone bonus is proportional to fragments in the matching zone (2 of 3 in zone → 2/3 of the bonus). Adjacency modules count adjacency from every fragment with no extra cap | Planned · kullanıcı fikri ve kararları (2026-10-08) |


#### Uncommon · sabit boyutlu (8, yan özellik yok)

| Modül | Aile · boyut | Kart metni ve kural | Gelişim (öneri) | Durum |
|---|---|---|---|---|
| Photonic Lens | Primary · `■` | *Primary hits deal more damage to targets without a status effect.* Primary damage to a target with no status effects ×1.50. Status is checked before each hit. | ×1.50 → ×1.53 → ×1.56 | Implemented · Uncommon · P4; yalnız Primary, vuruş öncesi status kontrolü |
| Reflection Shield | Defense · `■■` | *Your shield reflects attacks after a Defense ability.* When a Defense ability is used, for 5 sec, 35% of the damage absorbed by your shield is reflected as Energy damage to the nearest enemy; cannot critically hit; inactive while no shield is available; duration refreshes on reuse. Reflected damage does not trigger other modules, including Energy Feedback (user, 2026-10-07). | 35% → 40% → 45% | Implemented · Uncommon · P4; 5 sn Defense penceresi, Energy Feedback dahil zincirleme yok |
| Health Bridge | Defense · `■■` · komşuluk | *Your health becomes strength for adjacent abilities.* Each direct damage hit from up to 2 adjacent Q/E/F/R slot modules deals additional damage equal to 3% of MaxHealth (at 250 health, +7.5 per hit). No effect without an adjacent module. | 3% → 3.5% → 4% | Implemented · Uncommon · P4; yeteneğin kendi hasar türü, DoT tick hariç |
| Lucky Cast | Utility · `■■` | *Your abilities sometimes refresh instantly.* Luck +1. When a Q/E/F/R ability is used, Luck × 5% chance to reduce that ability's cooldown by 50%; the chance naturally stops at 100% (no separate cap). Excluding Primary, each use rolls separately. | 5% → 6% → 7% / Luck | Implemented · Uncommon · P3; %50 tavanı kaldırıldı, doğal ihtimal üst sınırı %100 |
| Frost Lock | Utility · `■■` | *Targets freeze at 4 Cryo stacks.* A normal target reaching 4 Cryo stacks cannot move or attack for 1 sec and loses its stacks (slow is removed); 8 sec cooldown per target. Elites and bosses are immune to the freeze. | Freeze 1.0 → 1.3 → 1.6 sec; cooldown 8 → 7 → 6 sec | Implemented · Uncommon · P5; hareket+saldırı kesilir, elit/miniboss/boss muaf |
| Arc Extender | Offense · `■` | *Electric can reach 5 stacks.* Your applied Electric stack limit increases from 4 to 5. Incoming damage bonus +3%/6%/9%/**13%/19%** (previously, the 4th stack gave +16%). Duration and refresh rules are unchanged. | 5th-stack bonus +19% → +21% → +23% | Implemented · Uncommon · P5; sahip profili ve mevcut ortak Electric stack modeli |
| Double Feed Core | Utility · `■■` | *Grants Attack Power and Energy Power.* Attack Power +15, Energy Power +23. | +15/+23 → +18/+28 → +21/+33 | Implemented · Uncommon · P2; seviye tablosu JSON'da, nihai denge ayrı |

| Omni Thrusters | Movement · `■■` | *Same speed in every direction.* Forward, reverse, and strafe thrust all become equal: thrust and top speed −30%, and reverse and strafe are set to that same value (e.g. Fighter 650/190/270 → 455/455/455). | İleri cezası seviye başına 1–2 puan azalır (öneri 1,5): −30% → −28.5% → −27% (Fighter 455 → 465 → 475) | **Implemented · Uncommon (2026-10-13)** · kullanıcı tanımı (2026-10-08): üç yön eşit (~450/450/450); seviye artışı %5 değil 1–2 puan |

#### Uncommon · değişken boyutlu (boyut merdivenli, 14; yan özellikli)

Sütunlar: A boyuta göre; özel yan özellikler (2); ortak havuzdan uygun olanlar (3). Yan özellik her satırda en büyük boyutta açılır. Her değişken boyutlu modül için 5 olası yan özellik: 2 özel + 3 ortak. Loot Shield’ın ikinci özel yanı **Kill Ward** (kullanıcı onaylı, 2026-10-13); Hunt Fuel ve kalkan yenilenmesi +%30/3 sn alternatifi kaldırıldı/reddedildi.

| Modül | Aile · merdiven | A: ana özellik | Özel yan özellikler | Ortak havuzdan | Gelişim (A, öneri) | Durum |
|---|---|---|---|---|---|---|
| Glass Cannon Core | Offense · 1-3 | *Attack Power rises, hull health falls.* 1c: AP +12, MaxHealth −6% · 3c: AP +34, MaxHealth −16% (after other bonuses) | Critical Chance +10% while health ≤50% · half the MaxHealth penalty returns as Max Shield +6% | O2, O4, O5 | AP +17% / level; penalty is fixed | Implemented · Uncommon · P2 A + Shield Return + Desperate Aim; O4/O5 dahil. P3 can sayısını korur, gerekirse son azami değere keser |
| Loot Shield | Defense · 1-2-3 | *Kills replenish your shield.* On each kill, gain shield equal to 1c: 2% of Max Shield; 2c/3c: 3%; cannot exceed maximum | On kill, damage taken ×0.90 for 2 sec; refreshes, does not stack *(Kill Ward)* · convert 50% of overflow shield into health | D1, D2, D4 | 1c 2% → 2.5% → 3%; 2c/3c 3% → 4% → 5% | Implemented · Uncommon · P3; Overflow Recovery + Kill Ward mevcut; Hunt Fuel yerine Kill Ward kullanıcı onayıyla eklendi (2026-10-13) |
| Sharp Scale | Offense · 2-3 | *Critical hits hit harder and happen more often.* Critical Damage +0.20 (1.5 → 1.7), Critical Chance +15% (user requested +15–20%) | On critical hit, AS +15% for 2 sec *(Critical Rhythm)* · Critical Chance +2% per point of total Luck *(Luck Lens)* | O1, O3, O5 | Critical Damage +0.20 → +0.25 → +0.30 | Implemented · Uncommon · P2 A + P4 Critical Rhythm/Luck Lens; O5 dahil |
| Scatter Core | Offense · 2-3 | *The first hit fires small guided missiles.* First successful hit of each Offense use fires 3 missiles, preferring distinct nearest enemies; each deals 15% of that hit. With fewer targets, remaining missiles reuse available targets; speed 600 units/s, lifetime 3 sec. No status or critical hit | Once per use, 0.8 sec after first hit, a 100-radius explosion deals 25% of that hit *(Aftershock)* · 4th missile (may reuse a target) | O1, O3, O4 | 15% → 18% → 21% per missile | Implemented · Uncommon · P6; Aftershock/Fourth Missile ve O4 dahil; generated hasar başka modülü tetiklemez |
| Thermal Shock Coil | Offense · 2-3 | *Deal more damage to burning and frozen targets.* While a target has at least 1 Thermal and 1 Cryo stack, all damage you deal to it ×1.30. Status is checked before the hit | Thermal and Cryo duration +2 sec · on their death, 400-radius Thermal explosion equal to 15% of target MaxHealth | O1, O2, O5 | ×1.30 → ×1.35 → ×1.40 | Implemented · Uncommon · P4 A + P5 Lingering Shock + P6 Death Burst; O5 dahil |
| Ricochet Tip | Primary · 2-3 | *A Primary hit bounces once.* First successful Primary hit bounces to the nearest other enemy within 450 units for 25% of applied damage; if none is nearby, emits a random straight projectile travelling up to 450 units; same texture, visual scale and actual speed as the source Primary projectile; damage lands on contact; one base bounce per shot, shared by all pellets/targets (continuous beam: one damage step) | Transfer one damage status stack · one additional bounce deals 15% | O2, O4, O5 | 25% → 30% → 35% | Implemented · Uncommon · P6; Status Transfer/Second Bounce ve O4/O5 dahil |
| Cold Retort | Defense · 2-3 | *Slow enemies that hit you.* When you take damage, apply 1 Cryo stack to the 2 nearest enemies within 300 units; 2 sec cooldown | Enemies with a status effect deal ×0.82 damage to you *(Jarring Effect)* · enemies you freeze by Cryo spread 1 Cryo stack to enemies within 400 units | D2, D3, D5 | Target count 2 → 3 → 4 | Implemented · Uncommon · P4 A/Jarring Effect + P5 Frozen Spread (400 birim) |
| Grit Your Teeth | Defense · 2-3 | *Take less damage when you're in danger.* While hull health ≤35%, damage you take ×0.80; cannot revive from death | At the same threshold, +35 to whichever is higher: Attack Power or Energy Power (AP on a tie; use values before this bonus) *(Critical Circuit)* · shield regeneration ×2 at the same threshold | D1, D3, D4 | ×0.80 → ×0.77 → ×0.74 | Implemented · Uncommon · P3 A + Critical Circuit + Emergency Recharge |
| Hunt Clock: Offense | Offense · 2-3 | Each kill refunds 5% of remaining Offense ability cooldowns; Primary excluded. | Every fifth kill refunds another 20% from the longest remaining cooldown in the same family · Hunt Bounty +1 Gold | G2, G3, G5 | 5% → 6% → 7% | Implemented · Uncommon; family filter and percentage refund |
| Hunt Clock: Defense | Defense · 2-3 | Each kill refunds 5% of remaining Defense ability cooldowns; Primary excluded. | Every fifth kill refunds another 20% from the longest remaining cooldown in the same family · Hunt Bounty +1 Gold | G2, G3, G5 | 5% → 6% → 7% | Implemented · Uncommon; family filter and percentage refund |
| Hunt Clock: Utility | Utility · 2-3 | Each kill refunds 5% of remaining Utility ability cooldowns; Primary excluded. | Every fifth kill refunds another 20% from the longest remaining cooldown in the same family · Hunt Bounty +1 Gold | G2, G3, G5 | 5% → 6% → 7% | Implemented · Uncommon; family filter and percentage refund |
| Hunt Clock: Movement | Movement · 2-3 | Each kill refunds 5% of remaining Movement ability cooldowns; Primary excluded. | Every fifth kill refunds another 20% from the longest remaining cooldown in the same family · Hunt Bounty +1 Gold | G2, G3, G5 | 5% → 6% → 7% | Implemented · Uncommon; family filter and percentage refund |
| Movement Rhythm | Movement · 2-3 | *The faster you move, the faster you fire.* Gain +3.5% Attack Speed at 200 actual speed, +7% at 300, +10.5% at 400; intermediate speeds keep the lower step. Zero below 200 | Horizontal and vertical rating +2.9 *(Advanced Thrust)* · when speed drops, the AS bonus fades over 1.5 sec instead of disappearing immediately | G2, G3, G5 | 3.5 → 4.0 → 4.5 per complete 100-speed step, starting at 200 | Implemented · Uncommon · P3 A + Advanced Thrust + Lingering Rhythm; Afterburner dahil gerçek hız sayılır |
| Trail Mines | Movement · 2-3 | *Leave mines after a movement ability.* For 5 sec after a successful Movement use, drop a mine every 50 units actually traveled; no stationary or teleport mines. Fuse 1 sec, radius 100, Kinetic damage 30 + 30% Attack Power at L1 (AP snapshotted when dropped) | +1 Cryo stack · active limit +4 and fuse 0.7 sec | G1, G3, G4 | Damage 30 + 30% AP → 35 + 35% AP → 40 + 40% AP (L1 kullanıcı; L2/L3 ajan gelişim seçimi); active limit 12 → 14 → 16 | Implemented · Uncommon · P6; Cryo Mines/Quick Fuse ve koşu içi G1 dahil |

#### P8 sonrası eksik yanlar — 2026-10-08

Canlı içerik **18 ana Uncommon modül** (7 sabit boyutlu, 11 değişken boyutlu) ve **37 yan özellik** taşır: 15 ortak, 22 özel. O4/O5/G1 ve Glass/Grit/Hunt/Movement'in bekleyen dört özel yanı tamamlandı. Her değişken boyutlu Uncommon modül 2 özel + 3 ortak seçeneğe açıktır; yan özellik yalnız en büyük boyutta çalışır. Sayılar ve ana modüller mevcut JSON sahiplerindedir. Kaynak: [modules.json](../../../LightYearsGame/assets/content/data/modules.json), [module_side_features.json](../../../LightYearsGame/assets/content/data/module_side_features.json).

O5, modüllü kaynağın ilk başarılı hasarında hedef başına bir kez çalışır; engellenen/sıfır hasar ve generated/suppressed vuruş tüketmez. Aynı hedefe sök/tak veya ilgisiz rebuild bonusu yeniden açmaz. O4 taban süreye +1 sn ekler; tekrar uygulamada birikmez. Ignite/Cryo/Electric/Kinetic ve Frozen dahildir; freeze cooldown'u hariçtir.

Kullanıcı Gold için koşu içi kapsamı onayladı: başlangıç 0; G1 ve Hunt Bounty takılıyken sahibin normal düşman öldürmesi başına ayrı ayrı +1; gemi yenilenmesinde korunur, yeni koşuda sıfırlanır. Mağaza, temel Gold düşüşü, harcama ve kalıcılık sonraya kalır. Movement'in Lingering Rhythm yan özelliği hız bonusunu 1,5 sn söndürür; ayrı AS eklemez. Glass Shield Return mevcut sabit +%6 Max Shield profilidir; cezanın yarısını her boyut/seviyede dinamik hesaplayan ayrı bir davranış yoktur.

Yeni doğrulama sonucu [uygulama planının güncel sonuç bölümünde](../../MODULE_UNCOMMON_IMPLEMENTATION_PLAN.md#9-p8-sonrası-eksik-yanlar--2026-10-08) tutulur. Loot dağılımı, yükseltme harcaması/arayüzü, kalıcılık ve nihai denge ayrı işlerdir. Test hibesi edinilebilirlik kanıtı değildir. Kullanıcının görsel kabulü ve simetrik gemi şemalarının bonus-bölgesine sığma sorunu ayrı açık kalır; bu çalışma görsel/şema kaynaklarını değiştirmez.

#### Yan özellik olarak birleşenler (artık ayrı modül değil)

| Eski modül | Nereye girdi |
|---|---|
| Hunt Fuel | **Kaldırıldı (kullanıcı, 2026-10-13)**; yerine Loot Shield özel yanı Kill Ward eklendi |
| Hunt Bounty Processor | Hunt Clock · implemented private side feature; koşu içi +1 Gold |
| Advanced Thrust Block | Movement Rhythm · private side feature |
| Critical Circuit | Grit Your Teeth · private side feature |
| Critical Rhythm | Sharp Scale · private side feature |
| Luck Lens | Sharp Scale · private side feature |
| Aftershock | Scatter Core · private side feature |
| Jarring Effect | Cold Retort · private side feature |

#### Rare · sabit boyutlu (9 tasarım · 8 oyunda; yan özellik yok)

Her modülün tek bir boyutu vardır; şekil 1, 2 veya 3 hücre kaplayabilir. Sabit boyutlu Rare modüller yan özellik taşımaz.

| Modül | Aile · boyut | Kart metni ve kural | Gelişim | Durum |
|---|---|---|---|---|
| Zone Overclocker | Utility · `■` | *Supercharge one zone.* Other modules in the zone where this module is placed get a +50% zone bonus instead of +25%. No effect of its own. | 50% → 55% → 60% | **Implemented · Rare · dalga 2 (2026-10-10)** · kullanıcı beğendi; tek hücre (2026-10-08; esin Last Epoch Idol Altar) · Z1 yan özelliğinden ayrışsın diye +45'ten +50'ye güçlendirildi (kullanıcı: ana buff + yan nerf) · Gövde'deki +%10'a etkisi: uygulanan varsayım ölçektir (Hull %10 → %20), kullanıcı onayı bekler |
| Stripped Hull | Utility · `■` | *Fly light, hit hard.* Damage +2.5% for each empty cell on the ship. | 2.5% → 3% → 3.5% per cell (öneri) | **Implemented · Rare · dalga 3 (2026-10-10; reboot sayımı dalga 4)** · kullanıcı: Utility, tek hücre, güçlü olmalı (2026-10-08; esin Cobalt Core) · koşu başındaki bedava erken güç kullanıcı tarafından kabul edildi; boş hücre sınırı yok · boş hücre sayısı 28 (kullanıcı, 2026-10-10): L3'te +%98 |
| Odometer | Movement · `■` | *The farther you travel, the stronger you get.* Every 5,000 units traveled during the run permanently grants +1% damage and +1% movement speed for the rest of the run; no cap. Stacks are stored in the module. | Stack başına değer +1% → +1.2% → +1.4% (mesafe sabit) | **Implemented · Rare · dalga 3 (2026-10-10)** (mesafe sayacı oyunda çalışır; rota/yolculuk sistemi gerekmez) · kullanıcı beğendi; seviye ile birim başına % artar (2026-10-08) · oyunun yolculuk hissine bağlı · kullanıcı iyi buldu, Rare'de kalır (2026-10-10); yolculuk/rota sistemi yok: bağımlılık borcu |
| Silent Running | Movement · `■` | *Travel quietly between fights.* Threat gained while traveling between route nodes ×0.75. | Threat ×0,75 → 0,70 → 0,65 (kullanıcı onaylı, 2026-10-10) | Planned · **kapsam dışı** (Rare uygulama planı §1) · kullanıcı beğendi (2026-10-08) · rota/Threat sistemine bağlı; harita henüz yok · kullanıcı iyi buldu, Rare'de kalır (2026-10-10): bağımlılık borcu |
| Misfit Wiring | Utility · `■` | *Modules outside their own zone work better.* Modules placed in a zone that does not match their family get +25% to their main value (normally no bonus, no penalty) — as good as their own zone. | 25% → 30% → 35% | **Implemented · Rare · dalga 2 (2026-10-10)** · kullanıcı beğendi (2026-10-08) · Z2'den ayrışsın diye +20'den +25'e güçlendirildi · Zone Overclocker ile bilinçli zıtlık · kullanıcı (2026-10-10): 1 hücre, sabit boyutlu · gemi genelinde, yalnız ana paket; Hull ve Ability Bay aile bölgesi sayılmadığı için Misfit'e tabi değildir |
| Momentum Shield | Movement · `■■` | *Your shield charges from speed, not time.* The shield no longer regenerates normally. Instead it recharges in proportion to your current movement speed: at max speed ×0.5 the normal regeneration rate at level 1; zero below 200 units/sec. Works during Afterburner. No recharge delay after damage. | Max hızdaki çarpan ×0.5 → ×0.7 → ×0.9; hız <200 ise yenilenme yok (kullanıcı, 2026-10-12) | **Implemented · Rare · dalga 2 (2026-10-10)** · kullanıcı fikri (2026-10-08); Rare/Epic açık, Claude önerisi Rare (Continuous Field Generator ile aynı ağırlıkta kural değişimi) · Continuous Field Generator ile aynı anda takılamaz; modül ekranı uyuşmazlık uyarısıyla iki modülü işaretler (kullanıcı, 2026-10-08) |
| Triple Core | Utility · `■■` | *Leave one slot empty to empower the others.* If at least one of Q/E/F/R is empty, direct damage of every remaining ability ×1.25; active with one, two or three empty slots. | Çarpan ×1,25 → 1,30 → 1,35 (×1,18'den yükseltildi; kullanıcı onaylı, 2026-10-10) | **Implemented · Rare · dalga 3 (2026-10-10; reboot sayımı dalga 4, `RareStaticDamageRulesE2E`)** · kullanıcının 2026-10-08 listesi · kullanıcı Rare'de iyi buldu (2026-10-10) · Claude notu: boşaltılan yetenek hasar veriyorsa 3×1.25 < 4×1.0, yani değer boş slota ne koyulduğuna bağlı; gerçek hasar dağılımıyla ölçülmeli |
| Continuous Field Generator | Defense · `■■■` | *Your shield never stops recharging.* Removes the shield recharge delay; the shield recharges continuously, even while taking damage, at 20% of the normal recharge rate. | Yenilenme normalin %20 → 35 → 45'i (üç seviye kullanıcı düzeltmesi, 2026-10-13) | **Implemented · Rare · dalga 2 (2026-10-10)** · kalkan-kırılma tetikleri (Shatter Current, Shatter Shock…) kalkan %25 dolduktan sonra yeniden kurulur, 0↔az döngüsü tekrar tetiklemez (kullanıcı, 2026-10-08: eşik Max Shield'in %25'i) · Grit/D4/Fast-Charge çarpanlarıyla test |
| Impact Redirector | Defense · `■■■` | *Armor also protects your shield.* While shield > 0, damage to the shield also benefits from 50% of Armor's damage reduction. | Armor'un kalkana etkisi %50 → 60 → 70 (kullanıcı onaylı, 2026-10-10) | **Implemented · Rare · dalga 2 (2026-10-10)** · kullanıcının 2026-10-08 listesi · `armorShare`: kalkan harcaması f = 1 − pay × (1 − Armor çarpanı) ile ölçeklenir; yan etki: kalkan yansıması (Reflection Shield) aynı oranda azalır, onay bekler |

#### Rare · değişken boyutlu (boyut merdivenli, 8 tasarım · 8 oyunda; yan özellikli)

Bu modüller birden fazla hücre boyutunda sunulur. Yan özellik en büyük boyutta açılır; boyut seçenekleri satırda gösterilir.

| Modül | Aile · boyut merdiveni | Kart metni ve kural | Gelişim | Durum |
|---|---|---|---|---|
| Spare Capacitor | Utility · `■` / `■■` / `■■■` (1-2-3) · komşuluk | *Adjacent abilities gain a charge.* 1 cell: 1 adjacent Q/E/F/R ability gains +1 charge; 2 cells: 2 adjacent abilities; 3 cells: 3 adjacent abilities at once. Cooldowns are unchanged. | Komşu yetenek sayısı boyutla 1 → 2 → 3; verilen charge sayısı seviye ile artar, boyutla artmaz; boyut yalnız kaç yeteneğe verileceğini artırır (kullanıcı, 2026-10-10; her seviyede +1 charge: L1 +1, L2 +2, L3 +3; parçalar yığılmaz, bir yetenek bu modülden en çok seviyesinin charge'ını alır) | **Implemented · Rare · R-Ön (charge geri yükleme), dalga 5-7 (çok charge, GrantCharge, komşu charge; `RareSpareCapacitorMechanicsE2E`)** · **KARAR VERİLDİ (kullanıcı, 2026-10-12):** parçalı olsun olmasın L1'de komşu her yetenek +1 charge alır (varsayılan yetenek 1 charge → 2; Q/E/R komşuysa 2/2/2); seviye yükseldikçe +L (L2 +2, L3 +3); parçalar yığılmaz (uygulanan A yorumu doğrulandı). Satır 245'teki '+1' ifadesi L1 anlamındadır; çelişki kapandı ([Rare uygulama planı §7.12](../../MODULE_RARE_IMPLEMENTATION_PLAN.md)). Overflow Transfer uygulanmadı · kullanıcı ceza istemedi: bekleme ×1,25 değil ×1,0 (2026-10-08) · kullanıcı (2026-10-10): ayrı yan özellik gerekmez, 2. boyutta 2 yeteneğe geçiş zaten ana özelliğin büyümesi; 3. boyutta 3 yetenek ve ek özellik (yan özellik) gelir · Ability Bay'de Q/E/F/R tek satırda: 3'lü hat 3 kabuğa değer |
| Shatter Shock | Defense · `■` / `■■■` (1-3) | *Your shield bursts when it breaks.* When the shield breaks, enemies within 250 units take damage equal to 10% of your MaxHealth with 1 cell or 25% with 3 cells (level 1; JSON 10/16/22 and 25/31/37, 2026-10-12) and are pushed back; 20 sec cooldown. Cryo stacks and shield restoration were removed from the Rare main effect (2026-10-08). | Hasar oyuncunun MaxHealth'inin yüzdesidir (kullanıcı, 2026-10-10) ve seviye başına artar; bekleme 20 sn sabit, seviyeyle azalmaz (kullanıcı, 2026-10-10). Artış +6 puan/seviye (eski +4'e ek +2; kullanıcı onayladı, 2026-10-10): 1 hücre %10 → 16 → 22, 3 hücre %25 → 31 → 37 (1 ve 3 hücre başlangıç oranları öneri) | **Implemented · Rare · dalga 4 (çekirdek, `RareOwnerBurstE2E`), dalga 5-7 (Stagger, Aftershock Wave)** · **KARAR (kullanıcı, 2026-10-12):** `pushImpulse` 150 kalır; kuramsal toplam ~420 birim, etkin ~350 birim (~5 sn; hız sönümü 0,70/sn; 3 sn ~276, 5 sn ~349); kart 'birim' ifadesi güncel değil · Aftershock kararı: satır 239 |
| Ember Accumulation | Offense · `■■` (2-3) | When Thermal reaches 4 stacks, the target explodes once instead of taking Ignite damage over time: 130% of that stack's total 5 sec DoT; stacks are cleared. | Patlama %130 → 140 → 150 (+10 puan/seviye, kullanıcı onaylı 2026-10-10; tüm boyutlarda aynı) | **Implemented · Rare · dalga 4 (çekirdek, `RareStatusRulesE2E`), dalga 5-7 (ladder, `RareLadderModulesE2E`)** · kullanıcı Rare için uygun buldu · stack sıfırlama Thermal Shock Coil koşulunu bozar; kullanıcı bu çatışmayı kabul etti (2026-10-08) · ana patlama yalnız hedefe vurur (katalog metni hedefin patladığını söyler, alan belirtmez); alan hasarı yan özellik olarak 200 birim önerildi (kullanıcı, 2026-10-10) |
| Kinetic Breaker | Offense · `■■` (2-3) | *Spend Kinetic stacks to pierce Armor.* While the target has 4 Kinetic stacks, your next direct hit consumes all 4 stacks and ignores the target's Armor completely. | Zırh yok sayan vuruşa hasar çarpanı ×1,10 → 1,15 → 1,20 (+5 puan/seviye, tüm boyutlarda aynı; kullanıcı onaylı 2026-10-10; çarpan özgün kart metnine eklenmiş yeni parçadır) | **Implemented · Rare · dalga 4 (çekirdek, `RareStatusRulesE2E`), dalga 5-7 (Hair Trigger, Armor Rend)** · kullanıcı beğendi (2026-10-08) · değeri düşman Armor'una bağlı; düşman Armor değerleri ayarlanacak |
| Giant Slayer | Offense · `■■` (2-3) | *The bigger they are, the harder they fall.* Against a target whose hull MaxHealth is at least 1.5× your hull MaxHealth, damage you deal is increased; the larger the gap, the larger the bonus. Lower threshold 1.5×, no upper cap. Öneri formül: ×(1.10 + 0.05 × (oran − 1.5)) → 1.5× = ×1.10, 3× = ×1.175, 10× = ×1.525. | Her seviyede +0,05 (kullanıcı, 2026-10-10); Claude yorumu: taban çarpan ×1,10 → 1,15 → 1,20, eğim 0,05/oran sabit; ana özellik tüm boyutlarda aynı | **Implemented · Rare · dalga 4 (oran tabanlı hasar, `RareRatioRulesE2E`), dalga 5-7 (Toppler, Underdog)** · kullanıcı yeniden tanımladı (2026-10-08): elit/boss %8 kalkan/Armor atlama yerine hull MaxHealth oranına bağlı hasar; alt sınır 1.5×, üst sınır yok · formül eğimi açık |
| Trophy Core | Offense · `■■` (2-3) · komşuluk | *Kills make the adjacent ability stronger for the rest of the run.* Each enemy killed by the adjacent Q/E/F/R ability permanently adds +0.4% damage for the run (elite +3%, boss +10%). Stacks are stored in the module: move the module next to another ability and the bonus moves with it. | Normal öldürme başına %0,40 → 0,45 → 0,50 (+0,05/seviye, kullanıcı onaylı 2026-10-10); elit ve boss oranı korunur (öneri: elit %3 → 3,4 → 3,75, boss %10 → 11,25 → 12,5); tüm boyutlarda aynı | **Implemented · Rare · dalga 5-7 (kill sayacı, `RareTrophyCoreMechanicsE2E`; Bounty Mark, Trophy Wall)** · kullanıcı beğendi (2026-10-08; esin Shape of Dreams Divine Faith); aile Utility değil Offense (kullanıcı) · komşuluk kullanıcı kararıyla korunur · modül sökülünce stack'ler korunur mu: öneri olarak korunur, yeniden takılınca etkin; kod bunu yapar ve `RareTrophyCoreMechanicsE2E` sınar (kullanıcı onayı ayrıca açık) |
| Dying Star | Offense · `■■■` (2-3) | *Kills feed a star that goes supernova.* Each enemy that dies within 300 units of you charges the star; at 10 charges it explodes in a large Thermal blast around you and applies 4 Ignite stacks to every enemy hit; charges reset. | Patlama hasarı her seviyede +%15 (kullanıcı onaylı, 2026-10-10); ölüm sayısı (10) ve yarıçap tüm boyutlarda ve seviyelerde sabit (yarıçap öneri 350, mutlak hasar denge testiyle) | **Implemented · Rare · dalga 5-7 (KillConfirmed şarj, patlama, Core Collapse, Red Giant; `RareDyingStarMechanicsE2E`)** · kullanıcı Süpernova yönünü seçti (2026-10-08; esin Nova Drift Dying Star; can harcayan aura sürümü bırakıldı) · **AÇIK / PLACEHOLDER:** taban hasar JSON 60/69/78 (2. seviye +%15, 3. seviye +%13; kart +%15 diyor); yarıçap 350 JSON; **KARAR (kullanıcı, 2026-10-12):** 60/69/78 placeholder şimdilik kalır, denge koşusunda ayarlanır (güncel ölçek, kullanıcı 2026-10-12: taban 60/69/78 + 0,40 × Energy Power; patlama anındaki canlı EP) |
| Escape Velocity | Movement · `■■` (2-3) | *Reach top speed and your shots go sonic.* After 2 sec of uninterrupted flight above 90% of max speed, the ship goes sonic: Primary projectiles travel twice as fast and pierce one target. Ends when you slow down. | Sonik durumda Primary hasarı +10% → +15% → +20% | **Implemented · Rare · dalga 4 (sustainedSpeedRules, `RareSustainedSpeedE2E`), Afterglow dalga 5-7** · kullanıcı beğendi (2026-10-08) · **Slipstream Control (kullanıcı, 2026-10-12):** Sonic Boom yerine sonik durumda +%25 dönüş kabiliyeti |

#### Rare boyut yaklaşımı

**Kullanıcı kuralı (2026-10-10):** Sabit boyutlu Rare modüller yan özellik taşımaz ve setin en güçlü, net davranışlı seçimleridir. Değişken boyutlu Rare modüllerde yan özellik en büyük boyutta açılır. Sabit/değişken dağılımı bir tasarım önerisidir ve kullanıcı onayı bekler; modül ve hücre bilgileri yukarıdaki tablolarda tek yerde tutulur.

**Boyutla ana özellik (kullanıcı, 2026-10-10):** Değişken boyutlu Rare modüllerin ana özelliği her boyutta aynı kalır; büyüyen boyut yan özelliği açar. İstisnalar: Spare Capacitor (boyut komşu yetenek sayısını 1/2/3 yapar; charge sayısı boyutla değil seviyeyle artar) ve Shatter Shock (boyutla hasar: 1 hücre küçük, 3 hücre büyük). Uncommon'da da yalnız Glass Cannon Core ve Loot Shield ana özelliği boyutla değişir; diğer 9 değişken boyutlu modülün kural verisi tüm boyutlarda aynıdır.

#### Rare değişken boyutlu modüllerin yan özellikleri: özel + ortak (2026-10-10)

Değişken boyutlu Rare'de en büyük boyutta 1 yan özellik açılır: 2 özel + 3 ortak seçenek. Kullanıcı (2026-10-10): yanıtlanmayan taslak kabul sayılır. Ortak 3'lü kullanıcı tarafından onaylandı (2026-10-10): her modülde F1 Parçalama + O/D/G/R havuzlarından iki seçenek. Modül başına havuz 5 seçenektir (2 özel + 3 ortak); düşerken biri rastgele atanır (ağırlıklar eşit, her biri %20), en büyük boyutta açılır. F1 kararları (kullanıcı, 2026-10-10): Trophy Core parçalanmaz (F1 listesinde yok); Spare Capacitor parçalanınca her parça değdiği yeteneğe +1 charge verir, bir yetenek bu modülden en çok +1 alır (yığma yok). Havuz kimlikleri için yukarıdaki "Ortak yan özellik havuzları" tablosuna bakın.

| Modül | Özel yan özellikler | Ortak (onaylı) | Durum |
|---|---|---|---|
| Ember Accumulation | *Area Blast:* patlama 200 birim çevreye de vurur, yan hedefe ana patlamanın %60'ı (oran onaylı, 2026-10-10) · *Smoldering Remains:* patlama yerinde 3 sn kor alanı, saniyede 1 Thermal stack | O4, O5, F1 | **Implemented · dalga 5-7** (Area Blast, Smoldering Remains; `RareWave5ContentE2E`) · Area Blast kullanıcı fikri, Smoldering Remains kabul; Chain Ember kaldırıldı (kullanıcı, 2026-10-10). O4 Ember DoT'unu 5 → 6 sn yapar, patlama ~%20 büyür |
| Shatter Shock | *Stagger:* itilen düşmanlar 1 sn stun; elit, miniboss ve boss muaf (kullanıcı, 2026-10-10) · *Aftershock Wave:* 1,5 sn sonra %50 ikinci itme dalgası | R2, D4, F1 | **Implemented · dalga 5-7** (Stagger, Aftershock Wave) · Kabul · **Aftershock kararı (kullanıcı, 2026-10-12):** ikinci dalga ilk patlamanın vurduğu aynı düşmana değil, sonradan gelen/yarıçap içindeki diğer düşmanlara vurur; aynı düşmana iki kez isabet gerekmez (kod uyumu açık: yankı hasarsız, yalnız 0,5× itme; ilk dalga hedefi dışlanmıyor) · D4 kalkan yeniden kurulumunu (%25 eşiği) hızlandırır |
| Kinetic Breaker | *Hair Trigger:* normal düşmanda 3 stack'te tetiklenir (elit/boss 4) · *Armor Rend:* zırh yok sayan vuruştan sonra hedef 4 sn %25 daha az Armor'a sahip | R5, O2, F1 | **Implemented · dalga 5-7** (Hair Trigger, Armor Rend) · Shrapnel Burst ve Quick Load reddedildi; yenileri kabul · değeri düşman Armor'una bağlı |
| Giant Slayer | *Toppler:* hull MaxHealth'i seninkinin ≥2,5 katı hedef ilk vuruştan sonra 2 sn %30 yavaşlar; boss'ta %15 (kullanıcı, 2026-10-10) · *Underdog:* seninkinden büyük MaxHealth'li düşmanlar sana ×0,92 hasar verir | R3, O5, F1 | **Implemented · dalga 5-7** (Toppler, Underdog) · Kabul |
| Trophy Core | *Bounty Mark:* her 25 stack'te komşu yeteneğin bekleme süresi kalıcı kısalır; kazanç adım başına azalır (kullanıcı, 2026-10-10; sayılar onaylı: %5 → 4 → 3 → 2 → 1, sonra %1 sabit, toplamsal; mutlak 1 sn tabanı geçerli) · *Trophy Wall:* stack'ler gemideki tüm Offense yeteneklere yarı değerde yansır | O3, G1, O1 | **Implemented · dalga 5-7** (Bounty Mark, Trophy Wall) · Kabul · Trophy Core parçalanmaz (kullanıcı, 2026-10-10): F1 çıkarıldı, üçüncü ortak O1 (kullanıcı, 2026-10-10) |
| Dying Star | *Core Collapse:* patlamadan önceki 1 sn düşmanlar merkeze çekilir; boss muaf, elit çekilir (kullanıcı, 2026-10-10) · *Red Giant:* şarj için ölüm yarıçapı 300 → 450 | O3, D3, F1 | **Implemented · dalga 5-7** (Core Collapse, Red Giant) · Kabul (kullanıcı, 2026-10-10) · Binary Star ve Stellar Wind reddedildi |
| Escape Velocity | *Afterglow:* yavaşlayınca sonik durum 1,5 sn sürer · *Slipstream Control:* sonik durumda dönüş kabiliyeti +%25 | G4, R6, F1 | **Implemented · dalga 5-7** (Afterglow); **Implemented · Slipstream Control (2026-10-12):** Sonic Boom kaldırıldı; sonik durumda dönüş kabiliyeti ×1,25; yavaşlama/çıkarma/ölümde modifier temizlenir |
| Spare Capacitor | 3. boyutta açılır: *Overflow Transfer:* dolu bir komşu yeteneğe gelecek charge diğer komşuya aktarılır · *Trickle Charge:* her 20 sn en az charge'lı komşuya +1 charge | G2, R1, F1 | **Implemented · dalga 5-7** (Trickle Charge); **Overflow Transfer uygulanmadı (AÇIK: kullanıcı teyidi bekleniyor, 2026-10-12; '1'deki gibi olacak' yorumu belirsiz)**; **KARAR VERİLDİ (2026-10-12):** charge sayısı çelişkisi kapandı, bkz. satır 204 · Kabul (kullanıcı, 2026-10-10); Reserve ve Overcharged Cast reddedildi · F1 ile her parça değdiği yeteneğe +1 charge verir; bir yetenek bu modülden en çok +1 alır, +3 yığma yok (kullanıcı, 2026-10-10) |

Reddedilen yanlar: Gold Plating (Bounty Armor, Tithe; modül Uncommon'a taşındı), Kinetic Breaker (Shrapnel Burst, Quick Load), Dying Star (Binary Star, Stellar Wind), Spare Capacitor (Reserve, Overcharged Cast), Ember Accumulation (Chain Ember, kaldırıldı).

#### Rare ortak yan özellik ölçeği ve Rare'e özel ortak havuz (öneri, 2026-10-10)

**Kullanıcı yönü (2026-10-10):** Ortak yan özelliklerin değeri Rare'de artmalı (örnek: AP +8 → Rare'de 12–15). Rare'e özel birkaç ortak yan özellik olabilir ve Rare'ler arasında paylaşılabilir olmalı.

**Ölçek kuralı (Claude önerisi):** Rare değeri = Uncommon değeri × 1,75, okunur sayıya yuvarlanır (AP +8 → +14). Süre ve Gold gibi ayrık değerlerde ayrı not düşülür. Uncommon değerleri L1, her seviyede aynı.

| Kimlik | Uncommon (L1) | Rare (öneri) | Not |
|---|---|---|---|
| O1 | AP +8 | AP +14 | |
| O2 | Crit +8% | Crit +14% | Kritik güçlü: Targeting Computer +3/+5 ile birlikte düşünülmeli |
| O3 | AS +4% | AS +7% | Yeni sabit tanım |
| O4 | Statü süresi +1 sn | +1,5 sn | ×1,75 = 1,75; Status Resonator ile çakışmasın diye aşağı yuvarlandı |
| O5 | İlk vuruş ×1,15 | ×1,25 | |
| D1 | MaxHealth +25 | +45 | |
| D2 | Armor +12 (2 sn) | Armor +20 (2 sn) | |
| D3 | Öldürünce %1 can | %1,75 | |
| D4 | Kalkan yenilenme ×1,15 | ×1,25 | |
| D5 | Max Shield +8% | +14% | |
| G1 | +1 Gold | +2 Gold | Kullanıcı kararı (2026-10-10); Gold ekonomisi yok |
| G2 | Ability Haste +4 | +7 | |
| G3 | Afterburner +10% | +18% | |
| G4 | Hareket derecesi +0,8 | +1,4 | |
| G5 | Luck +1 | Luck +2 | Luck ayrık; Lucky Cast ve Luck Lens'e çarpan olur, dikkat |

Ölçek kararı: kullanıcı ×1,75'i onayladı; tam 2 kat olmaması bilinçli (2026-10-10).

Rare'e özel ortak havuz (Rare'ler arası paylaşılır, F1 ile birlikte Rare havuzunu oluşturur):

| Kimlik | Özellik | Durum |
|---|---|---|
| R1 | *Guarded Strike:* Defense yeteneği kullanınca 4 sn Primary hasarı ×1,15 | **Implemented · dalga 2** (`RareSideFeaturesE2E`; canlı mermi dalga 4) · Kabul (kullanıcı, 2026-10-10); sayı öneri |
| R2 | *Reactive Plating:* kalkan kırılınca 4 sn Armor +30 (yeniden kurulum eşiği %25) | **Implemented · dalga 2** · Kabul (kullanıcı, 2026-10-10); sayılar onaylı (4 sn, Armor +30) |
| R3 | *Boss Hunter:* miniboss ve boss'a verdiğin hasar ×1,10 | **Implemented · dalga 1–2, fiilen işlemez** (oyunda boss sınıflı düşman yok) · Kabul (kullanıcı, 2026-10-10); sayı onaylı |
| R4 | *Full Charge:* kalkan tamamen doluyken verdiğin hasar ×1,08 | **Implemented · dalga 1–2** · Kabul (kullanıcı, 2026-10-10); sayı onaylı · Momentum Shield ve Continuous Field Generator ile etkileşir |
| R5 | *Status Weaver:* hedefte 2 veya daha fazla farklı statü varsa verdiğin hasar ×1,20 | **Implemented · dalga 1–2** · Kabul; kullanıcı ×1,20 veya ×1,25 dedi, ×1,20 seçildi (Claude): Thermal Shock Coil'in Thermal+Cryo ×1,30 ana özelliğini geçmesin |
| R6 | *Fleet Guard:* Movement yeteneği kullanınca 3 sn aldığın hasar ×0,85 | **Implemented · dalga 1–2** (gelen hasar penceresi, `RareDamageConditionsE2E`; `RareSideFeaturesE2E`) · Kabul (kullanıcı, 2026-10-10); sayılar onaylı |
| ~~Hunter's Rush, Steady Aim, Second Wind~~ | Elit/boss öldürünce Ability Haste +20; hasar almadan sonraki Primary ×1,2; canın %50 altında kalkanın %40'ı | Reddedildi |

Rare ortak havuz F1 ile birlikte R1–R6'dan oluşur (toplam 7 Rare'e özel ortak seçenek). Z1/Z2 hâlâ tüm nadirliklerde kullanılabilir.

Veri modeli notu: ortak yan özellikler bugün seviye başına değer dizisi taşıyor (`levelCount`, hepsi aynı); nadirlik boyutu yok. Rare değeri için ya nadirlik başına değer tablosu ya da ayrı Rare kimlikleri gerekir. Modül başına davranış kodu yazılmaz kuralı gereği ikisi de yalnız JSON olmalı. Karar (2026-10-10): ayrı Rare kimlikleri (örn. `Shared.O1.Rare`), kullanıcı seçimi Claude'a bıraktı ("uygun şekilde"). R3 (hedef boss mu), R4 (kalkan dolu mu) ve R5 (≥2 farklı statü) üç yeni, yeniden kullanılabilir hasar koşulu ister; R2 mevcut ShieldBroken tetiğiyle yazılabilir görünüyor; R1/R6 için ability-kullanıldı tetiği var, süreli çarpan penceresi doğrulanmadı. *(R0 anı, 2026-10-10.)* *Uygulama (2026-10-12):* Rare kimlikleri JSON'da ayrı kimliklerle ve Uncommon'dan ayrı havuzlarda (`Module.SideFeature.Shared.O1.Rare` … `G5.Rare`, 15 kopya; R1–R6 için RareSpecial ve diğer Rare havuzları) bulunuyor; R3–R5 koşulları ve R6 pencere dalga 1–2'de eklendi.

#### Epic / Legendary adayları (16 kayıt: 15 planlı/yeniden tasarım, 1 Review)

İlk Epic tasarımlarıdır; Epic nadirlik kuralları (yan özellik sayısı, gelişim) henüz karar değildir.

| Modül | Aile · şekil | Kart metni ve kural | Gelişim | Durum |
|---|---|---|---|---|
| Phase Slip | Movement · `■■` | *Slip through fire and answer with an explosive shot.* For the first 0.35 sec after Afterburner starts, enemy projectiles pass through the ship, but you cannot use abilities or fire the Primary weapon. Your first Primary shot after the phase becomes an explosive round that deals area damage around the impact. 6 sec cooldown. | Patlama hasarı/yarıçapı büyür; faz süresi ve bekleme sabit | Planned · kullanıcı faz sırasında ateş/yetenek kilidi ve sonraki merminin patlayıcıya dönüşmesini istedi, Epic önerdi; Claude katıldı (2026-10-08) · patlama hasarı, yarıçap: açık (öneri ×2, 120 birim) · geçilen mermi başına +%25 hasar ve +15 birim yarıçap, sınır yok (kullanıcı, 2026-10-08: %10 riskli yetenek için azdı; sorun çıkarsa sınır sonra eklenir) · temel ×2 hasar/120 birim öneri |
| Last Stand | Defense · `■■` | *Cheat death once.* On lethal damage, revive once with 50% health; the module then burns out (shown broken, no effect). | Dirilme canı 50% → 60% → 70% | Planned · kullanıcı Epic dedi (2026-10-08; esin Nova Drift Last Stand) · kullanıcı (2026-10-13): Max Shield cezası kaldırıldı; modülün işlevini kaybetmesi yeterli bedel · Grit Your Teeth ölümden dirilmez kuralıyla ayrı mekanik |
| Combo Sequencer | Yetenek modülü · Açık | İlk Rare taslağı: four different Q/E/F/R abilities within 10 sec → cooldowns recover ×1.5 for 5 sec. Kullanıcı fikri tuttu ama Rare için değil; Epic/Legendary olarak farklı tasarlanacak (2026-10-08). | Açık | Reworking · yeni tasarım bekliyor |
| Turret Mount | Primary · `■■■` · Epic | *Your primary weapon aims itself.* The Primary weapon automatically aims at the nearest enemy in range, independent of the ship's facing; damage ×0.80. | Açık | Planned · kullanıcı beğendi (2026-10-08) · Omni Thrusters ile sinerji |
| Blink Drive | Movement · `■■` · Epic **(eski sınıflandırma; güncel karar Legendary)** | *Afterburner becomes short-range teleports.* Afterburner is replaced by 3 short teleport charges; recharge is derived from Afterburner capacity. | Açık | Planned · kullanıcı beğendi (2026-10-08) · menzil/dolum açık |
| Shield Battery | Defense · şekil açık · Epic adayı | Kalkan doluyken yenilenme ek rezerv biriktirir; gelen hasar önce rezervi tüketir. Rezerv tavanı önerisi Max Shield'in %40'ı. | Açık | Planned · kullanıcı fikri ve Defense ailesini beğendi (2026-10-13); %40 öneri, sayı/boyut/gelişim ve kalkan-kırılma etkileşimleri açık |
| Cascade Reactor | Utility · şekil açık · Epic adayı | Yetenek kullandıktan sonraki 3 sn içinde farklı aileden kullanılan sonraki yetenek güçlenir; zincir sürerse bonus yükselir, pencere kaçarsa sıfırlanır. Öneri %20 → %40 → %60. | Açık | Planned · kullanıcı fikri ve Utility ailesini beğendi (2026-10-13); süre/bonuslar öneri, etkilenen güç değerleri, boyut/gelişim açık; Combo Sequencer ile ayrışma veya birleştirme kararı bekler |
| Twin Reactor | Offense · şekil açık · Epic adayı | AP ve EP'den düşük olanı yüksek olana eşitler; birbirine eklemez. Tek güç yatırımını hibrit saldırılara taşır. | Açık | Planned · kullanıcı beğendi (2026-10-13); boyut/gelişim açık. Eşitlemenin kendi bonusundan önceki AP/EP tabanına göre çözülmesi öneri; kaynak sırası ve diğer dönüşümlerle etkileşim henüz kesinleşmedi |
| Confluence Core | Utility · şekil açık · Epic adayı | Gemide bulunan her farklı modül ailesi tüm statlardan bonus verir: Armor, Attack Speed, hareket hızı, MaxHealth ve Critical Chance dahil geniş bir stat paketi. Aynı aileden çok modül yalnız bir aile sayılır. Beş aile tamamlanınca ayrıca bir bonus veya yeni etki açılmaz; her aile aynı stat paketine katkı yapar. | Açık | Planned · kullanıcı düzeltmesi (2026-10-13): yalnız AP/EP veren paket yerine her şeyden stat veren Epic; eski aile başına AP/EP +8 ve beş ailede Armor +25/hız +%15 taslağı geçersiz. Stat başına miktarlar, tüm stat listesinin teknik kapsamı, boyut/gelişim ve varsayılan sistem/slot kabuklarının aile sayımına katılması açık; sistemleri dışarıda tutmak ajan önerisi, kullanıcı kararı değil |
| Monochrome Core | Offense · şekil açık · Epic adayı | En az üç aktif Q/E/F/R yeteneği aynı hasar türündeyse o türde verilen bütün hasar artar. İlk çarpan önerisi ×1,50. | Açık | Review · kullanıcı "olabilir" dedi (2026-10-13); kesin kabul/uygulama kararı değil. Sayı/boyut/gelişim ve çok tür hasarlı yeteneklerin sayım kuralı açık. Ability ailesi değil damage type sayılır |
| Prism Core | Offense · `■■■` · Epic adayı | *Every Offense cast shifts the spectrum.* Her Offense isabeti kendi statüsüne ek olarak sıradaki statüyü uygular (Thermal → Cryo → Electric → Kinetic → başa); her cast sıradaki statüye geçer. | Açık | Planned · kullanıcı beğendi (2026-10-13) · Thermal Shock Coil, Status Weaver (R5) ve Kinetic Breaker ile sinerji · eklenen stack sayısı (öneri 1), Frost Lock/Arc Discharge zincirlenmesi ve boyut/gelişim açık |
| Blood Contract | Offense · `■■` · Epic adayı | *Pay hull for power, and fight harder the weaker you are.* Her Offense yeteneği mevcut canın %6'sını harcar (kalkanı atlar, ölümcül olamaz, en az 1 can bırakır). Yeteneğinin hasar verdiği bir düşman, o hasardan sonraki 4 sn içinde ölürse (öldüren kaynak fark etmez) MaxHealth'inin %1'i kadar can kazanırsın. Canın azaldıkça Offense hasarın artar: eksik can oranına doğrusal, tam canda ×1,20 → can sıfıra yakınken ×1,70. Bekleme süresi değişmez. | Açık | Planned · kullanıcı kararları (2026-10-13): bedel %6; iade MaxHealth'in %1'i, mevcut can değil; 4 sn penceresinde yeteneğin düşmana hasar vermesi yeter, ölümü kimin yaptığı önemsiz; hasar eğrisi doğrusal (tam canda ×1,20 → ×1,70 öneri değerleri); bekleme ×0,85 kaldırıldı · **açık:** boyut/gelişim ve eğri sayıları (kullanıcı eğriyi onayladı, sayılar ajan önerisi) · Grit Your Teeth'in %35 eşiğiyle çakışmaması için eğri yumuşak olmalı · Glass Cannon Core ile birlikte MaxHealth düşük kalır, %6 bedel daha sert vurur (playtest notu) |
| Field Medic | Defense · `■■` · Epic adayı | *Defense abilities mend you.* Defense yeteneği kullanınca 3 sn boyunca MaxHealth'inin %4'ü kadar can yenilenir; tekrar kullanımda süre yenilenir, birikmez. | Açık | Planned · kullanıcı beğendi (2026-10-13) · yenileme/can çalma yönü (oyunda iyileşme az) · Defense yeteneği yoksa etkisiz kalır (bilinçli bağımlılık) · %4 / 3 sn ajan önerisi; boyut/gelişim açık · Hull Siphon (kalkan boşken hasarın %4'ü can) alternatif olarak önerildi, seçilmedi |
| Storm Capacitor | Utility · `■■` · Epic adayı | *Abilities charge a storm.* Her Q/E/F/R yeteneği kullanımı 1 yük verir (en fazla 4). 4 yükte bir sonraki Primary isabeti yıldırım düşürür: menzildeki 3 rastgele düşmana vurur, her birine 2 Electric stack uygular; yükler sıfırlanır. | Açık | Planned · kullanıcı beğendi (2026-10-13) · Electric üretimi ve yeni varlık türü (yıldırım) · Arc Extender ve Arc Discharge ile sinerji · tetik "Primary isabeti" olduğundan silah tipinden bağımsız (beam tick'i, cone, wave dahil) · yıldırım hasarı, menzil, hedef seçimi (rastgele mi en yakın mı), boyut/gelişim ve yük sayısı ajan önerisi/açık |
| Second Hardpoint | Primary · `■■■` · Legendary | *Mount a second primary weapon.* Breaks the one-per-category rule: a second Primary Weapon module can be installed (e.g. Fighter + Breacher shotgun). Only one weapon fires at a time; a key switches between them. | Açık | Planned · kullanıcı kararı (2026-10-10): aynı anda değil geçişli · geçiş tuşu açık (öneri fare tekerleği; Ctrl Twin Rack'te) · geçiş beklemesi ve şarjör durumu açık |
| Twin Rack | Utility · `■■■` · Legendary | *Ability sets, Hwei-style.* Each Q/E/F/R slot holds 2 abilities (8 total). Q/E/F/R casts the first set; holding Ctrl casts the slot's second ability (Ctrl+Q). Each ability keeps its own cooldown. If both abilities in a slot share a family, both get a small set bonus. | Açık | Planned · kullanıcı kararları (2026-10-10): slot başına 2 yetenek, Ctrl ikinci set (Shift=Afterburner ile çakışmasın), aynı aile set bonusu olsun · bonus değeri açık (öneri +10% hasar) |

#### Koşu / yolculuk modülleri

Savaş için değil, koşunun geneli (rota, Threat, Gold/Scrap, mağaza, iyileşme) için özellik veren modüller. Survey Relay ve Silent Running'in ilk örnek olduğu bu yön kullanıcı tarafından uygun bulundu (2026-10-13). Bunlar da haritaya, Threat'e ve ekonomiye bağlı olduğundan **sistem bağımlılığı borcu** taşır (aşağıdaki not). Boyut ve nadirlik ajan önerisidir; sayılar kesin değildir.

| Modül | Aile · şekil | Kart metni ve kural | Gelişim | Durum |
|---|---|---|---|---|
| Danger Pay | Utility · `■■` · Epic adayı | *Danger pays.* Threat 3 ve üzerindeyken kazandığın Scrap ve Gold +%25. | Açık | Planned · kullanıcı beğendi (2026-10-13) · %25 ve eşik 3 ajan önerisi · Silent Running ile zıt yön |
| Interest Ledger | Utility · `■` · Rare adayı | *Gold earns Gold.* Her nokta sonunda elindeki Gold'un %5'i eklenir. **Üst sınır yok** (kullanıcı kararı). | Açık | Planned · kullanıcı beğendi (2026-10-13) · harcamayan oyuncu üstel kazanır; bu bilinçli risk/ödül takası (harcamamak riskli) · %5 ajan önerisi · Gold Plating ile döngü kurar |
| Pathfinder Array | Movement · şekil açık | *Open a shortcut.* Bölüm başına bir kez haritada birbirine yakın iki nokta arasında yeni bir rota bağlantısı açarsın: ulaşamadığın ödüle geçmek veya tehlikeli karşılaşmayı dolaşmak. | Açık | Planned · kullanıcı fikri (2026-10-13) · modül (ajan önerisi, onay bekliyor: bölümde bir kez yapılan anlamlı harita hamlesi) · "yakın" tanımı, Threat maliyeti ve rota haritasına bağımlılık açık |
| Courier Hold | **Ailesiz** · hücre sayısı açık | *Carry sealed cargo to the next chapter.* Birkaç gemi hücresini kaplayan mühürlü kargo taşırsın; sonraki bölüme ulaştırınca güçlü bir ödüle dönüşür. Şimdiki modül alanından vazgeçip geleceğe yatırım yapmaktır. | **Geliştirilemez** | Planned · kullanıcı fikri ve kararı (2026-10-13): ailesiz, geliştirilemez, **mağazada satılır** · ödülün ne olduğu, hücre sayısı ve kargo kaybedilme/ölüm kuralı açık |

**Yan özellikler (kullanıcı, 2026-10-13):** Homing Beacon, Wayside Repairs, Quartermaster ve Salvage Processor ayrı modül değil, General (G) havuzunda G6–G9 yan özelliği olarak kayıtlıdır ([Ortak yan özellik havuzları](#ortak-yan-özellik-havuzları-öneri-değerler)). Gerekçe: bir hücre yuvasına değmeyecek kadar küçük pasif etkiler; yan özellik yalnız boyut merdivenli modülün en büyük boyutunda ve rastgele açılır.

**Biçim kararı (kullanıcı, 2026-10-13):** Bu yönün her fikri ayrı modül olmak zorunda değil. Modül kalanlar: Danger Pay, Interest Ledger, Courier Hold, Pathfinder Array. Scavenger Net ve Retreat Beacon ertelendi (aşağıdaki Açık / ertelenen tablosunda).

**Yetenek modülü sınıfı (kullanıcı, 2026-10-13):** Combo Sequencer ve Ultimate Protocol, Utility ailesinde değildir; beş gemi ailesinden ayrı **Yetenek modülü** sınıfında izlenir. Bu sınıf, Offense/Defense/Movement/Utility yetenek ailelerinden ve mevcut Q/E/F/R sistem kabuklarından ayrıdır. Yerleşim, bölge bonusu, edinim ve yetenek kabuğuna bağlanma sözleşmesi henüz açık; bu belge kararı runtime sınıfı veya enum uygulaması değildir. Başka yetenek odaklı modüller kendiliğinden bu sınıfa taşınmaz.

#### Yetenek ailesi ilişkileri ve yetenek yan özellikleri

Kullanıcı (2026-10-13), modüllerde yetenek ailelerine bağlı ilişkileri az buldu. Asıl istek: **Utility, Defense ve Movement yetenekleri kullanılınca tetiklenen modüller** (Offense için Attack Rhythm ve Scatter Core gibi). Yetenek yükselticisi (süre, alan, çarpan) önerileri yanlış anlama sayıldı ve reddedildi.

| Modül | Aile · şekil | Kart metni ve kural | Gelişim | Durum |
|---|---|---|---|---|
| Monoculture | Utility · `■` · Rare adayı | *Specialize your loadout.* Q/E/F/R'dan en az 3'ü aynı aileden ise hepsinin bekleme süresi ×0,85. | Açık | Planned · kullanıcı beğendi (2026-10-13) · Triple Core ile uyumlu, Four Corners'ın zıddı · çarpan ajan önerisi |
| Four Corners | Utility · `■` · Rare adayı | *Diversify your loadout.* Dört yetenek slotu da farklı aileden ise hepsinin hasarı ve etkisi ×1,15. | Açık | Planned · kullanıcı beğendi (2026-10-13) · Triple Core ile birlikte çalışmaz (biri boş slot ister) · çarpan ajan önerisi · "etki ×1,15" Defense/Utility yeteneklerinde neyi büyüttüğü açık |
| Ultimate Protocol | Yetenek modülü · `■■` | *Your R ability becomes an ultimate.* R slotundaki yeteneğin bekleme süresi ×2, etkisi ×1,6. | Açık | Planned · kullanıcı beğendi, yalnız **tek slot (R)** olsun dedi (2026-10-13) · **kapsam uyarısı (kullanıcı):** bazı yetenekler farklı çalıştığı için "etki ×1,6" yeteneğe göre ayrı çözüm isteyebilir, iş büyüyebilir · çarpanlar ajan önerisi |
| Release Valve | Utility · `■■` · Rare adayı | *Fire a ring of damage far away.* Utility yeteneği kullanınca etrafında iki çember oluşur: iç yarıçap **600**, dış yarıçap **900**. İç çemberin içi boştur; iki çember arasındaki halka bölgesi hasar verir. İtme yoktur, yalnız doğrudan hasar. | Açık | Planned · kullanıcı beğendi ve menzili/şekli değiştirdi (2026-10-13): ilk öneri yarıçap 250 + itme idi, yarıçap 600–900 halka ve itmesiz oldu · **açık:** hasar değeri (öneri EP'nin %40'ı), halka bir kez mi vurur yoksa kısa süre kalır mı (öneri: tek anlık vuruş), halkanın gelişimde büyümesi, ekran sınırı dışında kalma durumu |
| Barrier Recharge | Defense · `■■` · Common adayı | *Defense abilities refill your shield.* Defense yeteneği kullanınca Max Shield'in %10'u yenilenir. | Açık | Planned · kullanıcı beğendi: "basit ve etkili" (2026-10-13) · %10 ajan önerisi |
| Brace Reflex | Defense · `■■` · Common adayı | *Defense abilities harden your hull.* Defense yeteneği kullanınca 4 sn Armor +25; süre yenilenir, birikmez. | Açık | Planned · kullanıcı beğendi (2026-10-13) · +25 ve 4 sn ajan önerisi |
| Bastion Slam | Defense · `■■` · Rare adayı | *Your Armor becomes a shockwave.* Defense yeteneği kullanınca 300 birim içindeki düşmanlara Armor'unun %150'si kadar Energy hasarı verilir. | Açık | Planned · kullanıcı beğendi (2026-10-13) · %150 ve 300 birim ajan önerisi; Armor'dan hasar ölçeklemesi tank gemiler (Aegis) için · Armor Conversion ve Impact Redirector ile Armor yatırımını çok yönlü yapar |
| Tempered Hull | Defense · `■■` · Rare adayı | *Every defensive cast hardens you for the rest of the run.* Defense yeteneği her kullanıldığında bu koşu boyunca kalıcı Armor +1; toplam bir üst sınırı vardır. | Üst sınır modül seviyesiyle artar: +30 → +40 → +50 (öneri) | Planned · kullanıcı beğendi (2026-10-13); üst sınırın seviye ile artması kullanıcıdan: "kontrol yine bizde olur" · +1 Armor/kullanım ve sınır sayıları ajan önerisi · stack'ler modülde saklanır (Odometer gibi); sökülüp takılınca davranışı açık |
| Overshield Layer | Defense · `■■` · Uncommon adayı | *Your shield swells past its limit.* Defense yeteneği kullanınca Max Shield'in %20'si kadar geçici kalkan eklenir; kalkan maksimumu aşabilir ve 6 sn içinde azalarak kaybolur. | Açık | Planned · kullanıcı beğendi (2026-10-13) · %20 ve 6 sn ajan önerisi · Barrier Recharge yalnız eksik kalkanı doldurur, bu ise tavanın üstüne çıkar · tavan üstü kalkanın Shield Battery/Continuous Field Generator ile etkileşimi açık |
| Field Mastery | Utility · `■■` **sabit** · Epic adayı | *Practice is progression.* Her Utility yeteneği kendi kullanım sayacını ayrı tutar. Bir Utility yeteneğini her N kez kullandığında o yetenek **kalıcı** olarak (koşu boyunca) +1 seviye kazanır; **üst sınır yoktur**. Bu modül takılıyken Utility yetenekleri **Scrap ile geliştirilemez**; seviye yalnız kullanımla gelir. | Eşik N modül seviyesiyle düşer: 15 → 14 → 13 → 12 | Planned · kullanıcı beğendi (2026-10-13) · kullanıcı kararları: sayaçlar yetenek başına ayrı; modül 2 hücre ve sabit boyutlu (merdiven ve yan özellik yok); **+2 sınırı ve "geçici" kaldırıldı:** seviye kalıcı ve sınırsız; eşik 15 ile başlar, 14 → 13 → 12'ye düşer; modül takılıyken Utility yetenekleri Scrap ile geliştirilemez · **yorum (ajan, doğrulama bekliyor):** kazanılan seviye yalnız sayacı dolan yeteneğe gider · **kullanıcı kararları (2026-10-13, ikinci tur):** yeteneğin kendi azami seviyesi geçerli kalır; modül takılmadan önce Scrap ile alınmış seviyeler korunur; modül sökülünce kazanılmış seviyeler **kalır** ve bunun için bir koruma eklenecek (kullanıcı örneği: her seviye alındığında gereken kullanım sayısı 1 azalır) · **açık:** koruma kuralının tam biçimi (bu azalma modül seviyesine bağlı 15 → 14 → 13 → 12 eşiğinin yerine mi geçer, yoksa onunla birlikte mi çalışır; ajan okuması: yetenek başına her kazanılan seviyede eşik 1 azalır) ve takıp sökmeyi sömürü yapmayacak biçimi · 4 seviyeli gelişim (diğer modüller 3 seviyeli) |
| Speed Lance | Movement · `■■` · Rare adayı | *Your speed becomes a spear.* Movement yeteneği kullanınca, **kullanımdan sonraki 1 sn içinde** ileri doğru delen bir Energy mızrağı fırlar. Hasarı hızınla ölçeklenir. | Açık | Planned · kullanıcı beğendi (2026-10-13); 1 sn kuralı kullanıcıdan: hız yeteneğin ardından toparlandığı için mızrak yetenek anında değil 1 sn içinde fırlamalı · **yorum (ajan, kullanıcı doğrulaması bekliyor):** pencere boyunca ulaşılan en yüksek hız örneklenir ve mızrak pencere sonunda fırlar · hız çarpanı (öneri 0,4×), mızrak hızı/menzili/delme sayısı açık |

**Yetenek yan özellikleri (kullanıcı, 2026-10-13):** Q/E/F/R slot modüllerine (yetenek kabuğuna) bağlı yan özellik fikri. Kullanıcı daha önce bazı modüllerin yeteneğin yerine konulup davranışı değiştirebileceğini (örnek: yetenek Thermal hasar verir) konuşmuştu; bu kavramın ayrıntısı bu dokümanda yazılı değil, çerçeve açık.

| Yan özellik | Kural | Durum |
|---|---|---|
| Cross-Wired (aile takma adı) | Taşıyan yetenek, modül tetikleri için ek olarak Offense yeteneği sayılır. "Offense yeteneği kullanınca" diyen modüller (Attack Rhythm, Scatter Core, Prism Core, Blood Contract) bu yeteneğin kullanımında da çalışır; yalnız tetikler geçer, hasar çarpanları geçmez. Defense ve Movement yeteneklerinin yanında **Utility yetenekleri de** tetikler (kullanıcı). Tersi (Offense'in Defense sayılması, Guarded Wiring) ajan önerisi. | Planned · kullanıcı beğendi, yetenek yan özelliği olarak düşünülsün dedi (2026-10-13); önceki "modül" önerisi buna dönüştü · Prism Core/Blood Contract'ta güç sınırı gerekebilir |
| Timing Bar (zamanlama çubuğu) | Q/E/F/R'ye basınca altta bir çubuk çıkar; tatlı noktaya denk gelince bastığında yetenek güçlü çıkar. Tatlı nokta dışı ×0,9; büyük tatlı nokta ×1,2; küçük tatlı nokta ×1,5. Overcast (basılı tutma) önerisinin yerine geçti. | Planned · kullanıcı fikri (2026-10-13), sayılar kullanıcıdan · **açık:** çubuk hızı, tatlı nokta boyutu/yeri, basmayınca ne olur, anlık yetenekler (Dash gibi) için uygulanabilirlik, bekleme süresi ve diğer yetenek yan özellikleriyle etkileşim |

#### Açık / ertelenen

| Modül | Durum |
|---|---|
| Broadside Mount | **Planned · kullanıcı fikri beğendi (2026-10-08, öneri 2; Epic görüşmesinde yeniden beğendi, 2026-10-13).** Primary: *Your primary weapon fires from both sides of the ship instead of forward.* Öneri: sabit 3 hücre, her yanda normal hasarın %70'i, Epic adayı; sayı/boyut/nadirlik kesinleşmedi. Silah tiplerine uyarlama ve gelişim açık; sabit boyutlu seçilirse yan özellik yok. |
| Polar Shield | **Planned · kullanıcı fikri beğendi (2026-10-08, öneri 4).** Defense: *Your shield splits into independent front and rear pools. Hits drain the pool facing the attack; an empty pool exposes the hull.* Öneri: mevcut toplam kapasite eşit bölünür, toplam regen bütçesi paylaşılır; 2–3 hücre, Rare adayı. Sayı/boyut/nadirlik, kırılma tetikleri ve diğer kalkan modülleriyle etkileşim açık. |
| Survey Relay | **Planned · kullanıcı fikri beğendi (2026-10-08, öneri 6).** Utility: *Reveal the module families of upcoming rewards on the route map.* Öneri: sonraki 2 ödül görünür; gelişim görülen düğüm sayısı; 1–2 hücre, Uncommon/Rare açık. Sayı/boyut/nadirlik kesinleşmedi; gelecek rota haritasına bağlı, mevcut arena özelliği değil. |
| Scavenger Net | **Ertelendi · Utility `■` Uncommon önerisi (kullanıcı tuttu, 2026-10-13).** *Even empty fights pay.* Ödülsüz savaşlar normal savaşın Scrap'inin %40'ını verir. **Uygulanmayacak;** bölüm/ödül tasarımı sırasında yeniden düşünülecek (kullanıcı kararı). %40 ajan önerisi. Kullanıcı (2026-10-13): şimdilik eklenmeyecek, ilgili özellik (ödülsüz savaşın Scrap vermesi) oyunda yok. |
| Retreat Beacon | **Ertelendi · Utility `■■` Epic önerisi (2026-10-13).** *Leave a fight you can't win.* Bölüm başına bir kez savaşı terk edip önceki noktaya dönersin; ödül yok, Threat artmaz. Kullanıcı: şimdilik eklenmeyecek, savaştan kaçış özelliği oyunda yok. |
| Heavy Hull Reinforcement | Defense · `■■■` Armor +50, movement −1/−1. Düz stat takası olduğundan Armor Conversion (Rare) ile birleştirilmesi öneri; karar yok. |
| Layered Defense | Defense · `■■` proposal: while shield > 0, Armor +35; bonus is removed when the shield breaks. Karar yok; savunma sayısı yeterli olduğundan çıkarılabilir. |
| Circle Spirit | Offense · `■■` proposal: damage you deal increases by 2% for each enemy within 300 units, capped at 20% (progression 2% → 2.5% → 3%; cap 20% → 25% → 30%). Nadirlik: **Uncommon adayı** (Claude, kullanıcı rarity seçimini bıraktı, 2026-10-08); devam eden Uncommon fazlarını bozmamak için 18'lik sete henüz alınmadı. |
| Gold Plating | Defense · `■■■` sabit · **Uncommon adayı (kullanıcı, 2026-10-10; önce Rare'di, yan özellikler Bounty Armor/Tithe reddedildi).** +1 Armor per 20 Gold, up to ~+30 (draft +15). When Gold is spent, the bonus is recalculated from current Gold. 3 hücre, yan özelliksiz. Uncommon'a göre sayı yeniden ölçülecek (Armor çarpanı düşük kalırsa artırılır); Gold sistemi yok: bağımlılık borcu. 18'lik sete henüz alınmadı. |
| Status Primer | Primary · `■■` sabit · **Uncommon adayı (kullanıcı, 2026-10-10; önce Rare'di).** *Primary hits feed your strongest status.* A Primary hit adds +1 stack to the status with the most stacks on the target; at most once per target per 1 sec. Does nothing if the target has no status. Yan özelliksiz. 18'lik sete henüz alınmadı. |
| Arc Discharge | Offense · `■■` sabit · **Uncommon adayı (kullanıcı, 2026-10-10; önce Rare'di).** *Electric jumps to nearby enemies.* When a target reaches 4 Electric stacks, 2 Electric stacks jump to the 2 nearest enemies within 350 units; stacks on the source are kept; 4 sec cooldown per target. Yan özelliksiz; sıçrama hedef sayısı gelişimde artabilir (2 → 3 → 4). 18'lik sete henüz alınmadı. |
| Armor Conversion | Defense · `■■` · **Uncommon adayı (kullanıcı, 2026-10-10; önce Rare'di).** *Your Armor becomes Attack or Energy Power.* Add 40% of your Armor to whichever is higher: Attack Power or Energy Power (using values before this bonus). At 80 Armor, about +32. Uncommon'a göre değerleri yeniden ölçülecek; 18'lik sete henüz alınmadı. Gelişim 40% → 45% → 50% (Rare önerisi). |
| Status Resonator | Offense · `■■` · **Uncommon adayı (aile Offense: kullanıcı, 2026-10-13; kullanıcı, 2026-10-10; 10-06'da Uncommon'dan Rare'e taşınmıştı, şimdi geri).** *Your applied statuses last longer.* Status effects you apply last ×1.4 (Kinetic/Electric/Cryo 6 → 8.4 sec; Thermal 5 → 7 sec); toplam Ignite hasarı ~%40 artar. ×1.4 Rare ölçeğinde, Uncommon için düşürülmesi gerekebilir; Ember Accumulation'ın 5 sn referansıyla etkileşimi açık. 18'lik sete henüz alınmadı. |
| Chain Link | Utility · `■■` adjacency proposal: when one of two adjacent Q/E/F/R slots is used, reduce the other's cooldown by 0.5 sec (progression 0.5 → 0.7 → 0.9 sec). Karar yok. |
| Level Refresh | Defense · `■■` proposal: on level up, restore health equal to 25% of MaxHealth and 50% of Max Shield. Altı öldürmede seviye sistemi geçici test amaçlı olduğundan riskli; karar yok. |
| Blood Bond, Drift Armor | Defense yedek önerileri; karar yok. |
| Momentum Strike | Movement · `■■` Rare proposal: within 3 sec after a Movement ability, the first Offense ability deals ×1.30 damage. Kullanıcı emin değil; karar yok. **Kullanıcı reddetti (2026-10-13, aile ilişkisi turunda yeniden önerildi, hiçbirini beğenmedi).** |
| Impact Tail | Effect undefined; awaiting a one-sentence description from the user. |
| Rapid Condenser | Ertelendi: charge'lı yetenek sayısı sayılacak; azsa ikinci bir etki gerekir ya da elenir. |

**Sistem bağımlılığı borcu (kullanıcı, 2026-10-10):** Gold Plating (Gold harcaması/ekonomisi; artık Uncommon adayı), Silent Running (rota haritası, Threat) ve Odometer (yolculuk/rota) henüz var olmayan sistemlere bağlıdır. Kullanıcı üçünü Rare'de tutmayı seçti; ilgili sistem gelene kadar borç olarak izlenir, o zamana dek sayı ve etkisi doğrulanamaz.

**Modül uyuşmazlığı (kullanıcı, 2026-10-08):** Aynı kuralı farklı biçimde yeniden yazan modüller aynı anda takılamaz. İkincisi yerleştirilmek istendiğinde modül ekranı uyuşmazlık uyarısı verir ve çatışan iki modülü işaretler. İlk çift: Momentum Shield ↔ Continuous Field Generator. Yalnız ters sinerjisi olan modüller (ör. Ember Accumulation ↔ Thermal Shock Coil) bu kurala girmez; birlikte takılabilirler.

**Komşuluk az kullanılır (kullanıcı, 2026-10-08):** modül sistemi gemi geneli ve bölge temelli çalışır; Shape of Dreams'teki gibi yeteneğe takılan öz mantığı esas değildir. Yeni tasarımlarda komşuluk istisna olmalıdır.

**Komşuluk kuralı:** Health Bridge, Spare Capacitor ve Trophy Core şu kuralı kullanır: hücre kenarı paylaşan Q/E/F/R slot modülleri komşudur; bir domino en çok 2 slota değer. Primary slotu komşuluk dışıdır.

**Ortak sınır notu:** Scatter Core (Aftershock yan özelliğiyle) ve Attack Rhythm aynı "Offense kullanınca" tetiğindedir; Sharp Scale'in Critical Rhythm yan özelliği ile Attack Rhythm ikisi de zamanlı Attack Speed verir. Birlikte takılırsa tek yetenek şişer. Ortak sınır/bütçe kararı açık.

### Geri bildirimi bekleyen önceki fikirler

| Modül | Önceki taslak | Durum |
|---|---|---|
| Void Tip | After Offense, the first Primary shot deals area damage around its target | Review |
| Critical Handoff | A Primary critical hit reduces pending Offense cooldown | Review |
| Retaliation Sequence | On taking damage, fire guided missiles at the attacker | Review |
| Emergency Egress | On shield break, clear slows and grant brief immunity | Review |
| Momentum Battery | Charge accumulated based on Afterburner duration | Review |
| Hunter's Mark | Mark a target with consecutive Primary hits; consume the mark with Offense | Review |
| Orbital Marker | Offense `■■■` Epic önerisi: Offense yeteneği hedefin konumunu işaretler; 1,5 sn sonra telegraf'lı yörünge saldırısı iner (yetenek vuruşunun %60'ı, alan hasarı, Thermal) | Review · ajan önerisi (2026-10-13), kullanıcı seçmedi |
| Signal Jammer | Utility `■■` Epic önerisi: Utility yeteneklerinin vurduğu düşmanların atış hızı 4 sn −%25; düşman tarafında atış hızını değiştirecek özellik doğrulanmadı | Review · ajan önerisi (2026-10-13), kullanıcı seçmedi |
| Cold Front | Movement `■■` öneri (Rare düzeyi daha uygun olabilir): Movement yeteneğinde ayrıldığın noktada 2,5 sn, yarıçap 200 Cryo alanı, saniyede 1 Cryo stack | Review · ajan önerisi (2026-10-13), kullanıcı seçmedi |
| Hull Siphon | Defense `■■` Epic önerisi: kalkan boşken verdiğin hasarın %4'ü can olur, saniyede en çok MaxHealth'in %3'ü | Review · Field Medic alternatifi olarak önerildi (2026-10-13), seçilmedi |
| Unspent Guard | Defense `■■` öneri: hasar emmeden biten Defense yeteneğinin bekleme süresinin %60'ı geri verilir | Review · ajan önerisi (2026-10-13), kullanıcı yorum yapmadı |
| Dragnet | Utility `■` öneri: Utility yeteneğinin etkilediği her düşman için o yeteneğin kalan bekleme süresi 1 sn kısalır (en fazla 5 sn) | Review · ajan önerisi (2026-10-13), kullanıcı yorum yapmadı |
| Harvest Field | Utility `■■` öneri: Utility yeteneğinin etkisindeyken ölen düşmanlar +%50 Scrap ve +1 Gold verir | Review · ajan önerisi (2026-10-13), kullanıcı yorum yapmadı; Scrap/Gold sistemi tam değil |
| Gold Recast | Utility `■■` öneri: bekleme süresindeki Utility yeteneği 30 Gold ile hemen tekrar kullanılır (savaşta her ek kullanım bedeli ×2) | Review · ajan önerisi (2026-10-13), kullanıcı yorum yapmadı; Shield Loan'a yakın |
| Stoic Plating | Defense `■■` öneri: Defense yeteneği kullanınca 3 sn tek bir vuruş en fazla MaxHealth'inin %15'ini alabilir | Review · ajan önerisi (2026-10-13), kullanıcı yorum yapmadı |
| Purge Protocol | Defense `■` öneri: Defense yeteneği kullanınca yavaşlatma ve statüler temizlenir, 3 sn aynı tür statüye bağışık | Review · ajan önerisi (2026-10-13), kullanıcı yorum yapmadı; Emergency Egress ile yakın |
| Haste Slam | Utility `■■` öneri: Utility yeteneği kullanınca 400 birim içine Ability Haste'in 5 katı Energy hasarı | Review · ajan önerisi (2026-10-13), kullanıcı yorum yapmadı |
| Shield Cannon | Defense `■■` öneri: Defense yeteneği kullanınca mevcut kalkanın %15'ini harca, harcananın 3 katı Energy hasarı en yakın düşmana | Review · ajan önerisi (2026-10-13); kullanıcı Defense için hayatta kalma yönünü istedi, hasar odaklı olduğundan geride kaldı |
| Dread Field | Utility `■■` öneri: Utility yeteneği kullanınca 600 birim içindeki düşmanlar 4 sn sana ×0,85 hasar verir | Review · ajan önerisi (2026-10-13), kullanıcı yorum yapmadı |
| Slipstream Recharge | Movement `■` öneri: Movement yeteneği kullanınca 4 sn kalkan yenilenmesi ×2,5 ve gecikme yok | Review · ajan önerisi (2026-10-13), kullanıcı yorum yapmadı; Momentum Shield ile çakışır |
| Wide Net | Utility `■■` Epic önerisi: her ödül ekranında bir seçenek fazla gelir | Review · ajan önerisi (2026-10-13), kullanıcı yorum yapmadı; ödül dağılımı belli olmadan gücü ölçülemez |

### Reddedilen fikirler

| Modül | Durum |
|---|---|
| Magazine Residue | Rejected |
| Crossfeed | Rejected |
| Condenser Barrel | Rejected |
| Staggered Loading | Rejected |
| Recoil Engine | Rejected |
| Emergency Power Transfer | Rejected |
| Overflow Charge | Rejected |
| Fixed Position | Rejected |
| Impact Buffer | Rejected |
| Luck Core | Rejected (2026-10-05, "Luck +2" çok sıradan bulundu; yerine Lucky Cast) |
| Recovery Line | Rejected (2026-10-05, kullanıcı beğenmedi; yerine başka defansif modüller aranıyor) |
| Counterpush | Rejected (2026-10-05, kullanıcı beğenmedi) |
| Impact Shell | Rejected (2026-10-05, kullanıcı beğenmedi) |
| Last Breath Cell | Rejected (2026-10-05, kullanıcı beğenmedi) |
| Leaky Armor | Rejected (2026-10-05, Afterburner ile sürekli kullanılabildiği için fazla güçlü bulundu) |
| Tension Matrix | Rejected (2026-10-08, Rare aday: Electric 4 → 8 stack; kullanıcı yapılmamasını istedi) |
| Energy Coupler | Rejected (2026-10-08, Rare aday: komşu slot hasarının bir kısmını Energy'ye çevirme; kullanıcı kaldırdı) |
| Precision Processor | Rejected (2026-10-08, Rare aday: kritik RNG yerine yük sayacı; kullanıcı beğenmedi) |
| Overkill Relay | Rejected (2026-10-08, Rare aday: artan öldürme hasarını yakın hedefe aktarma; kullanıcı beğenmedi) |
| Graze Coil, Ram Plating, Shatterpoint, Charged Chamber | Rejected (2026-10-08, Rare adayları: sıyırma→Afterburner, yüksek hızda çarpma hasarı, donmuş hedefte buz kıymığı, bekleyince şarjlı Primary; kullanıcı hiçbirini beğenmedi) |
| Status Reaction, Death Transfer, Afterburner Overflow, Echo Slot | Rejected (2026-10-08, kullanıcı yaratıcı bulmadı; Echo Slot EchoProtocol yeteneğinin modül hali) |
| Gilded Trigger, Afterglow, Autopilot Slot, Cold Preservation, Amplifier Frame | Rejected (2026-10-08, Shape of Dreams/Hades/RoR2 esinli tur; komşuluğa fazla dayandı) |
| Salvo Loader | Rejected (2026-10-08, Primary yeniden dolmaz/Afterburner şarjörü doldurur; kullanıcı kötü buldu) |
| Slipstream Tether, Decoy Beacon, Full Thrusters | Rejected (2026-10-08, Movement Rare adayları: Afterburner'la düşman sürükleme, hologram yem, Motor bölgesi doluysa +1 charge; kullanıcı beğenmedi) |
| Comet Core, Dynamo, Wake Charger | Rejected (2026-10-08, Movement Rare adayları; kullanıcı Escape Velocity'yi seçti, Dynamo yerine kendi hız→kalkan fikrini verdi) |
| Hot Swap Bay, Trade Uplink, Belt Feed, Modular Frame, Shield Drones | Seçilmedi (2026-10-08, sistem değiştiren tur; kullanıcı Silent Running, Turret Mount, Blink Drive, Second Hardpoint, Twin Rack'i seçti) |
| Shield Loan, Grace Period, Recall Anchor, Solo Act | Rejected (2026-10-13, Epic öneri turu: bekleme için kalkan ödeme, gecikmeli gövde hasarı, Afterburner çapasına geri sıçrama, tek yetenekli build; kullanıcı Prism Core, Blood Contract ve Siege Core mantığı dışındakileri beğenmedi) |
| Siege Core | Rejected (2026-10-13, kullanıcı "boşverelim" dedi: düşük hız tetiği oynanışta zor bulunmuştu, sonraki "her 5. saldırı" mantığı Cone/Wave/Beam silahlarına uyarlanamadığı için bırakıldı; Beam kararı hiç alınmadı) |
| Tactician's Lens, Overlap Core, Zone Capacitor, Time Dilator, Crowd Plating, Guard Break | Rejected (2026-10-13, yetenek aileleri arası ilişki turu: Utility'nin hasar işaretlemesi, eşzamanlı aktif aile bonusu, bölge başına +1 şarj, süre uzatma, Offense vuruşundan Armor, Defense bitişinden Offense hasarı; kullanıcı hiçbirini beğenmedi) |
| Static Charge, Fortune Window, Cold Snap, Kinetic Wake, Sharpened Edge | Rejected (2026-10-13, ability-tetikli modül turu: Utility sonrası 3 Primary isabetinde Electric, Utility sonrası Luck +2, Defense sonrası yakın düşmana Cryo, Movement sonrası Primary isabetlerinde Kinetic, Movement sonrası Critical Chance; kullanıcı hiçbirini beğenmedi. Not: Primary'ye statü yükleyen ve Luck/Critical veren tetikler çoğunlukla beğenilmedi) |
| Tempo Coil, Triage Kit, Slip Guard, Thrust Surge, Departure Blast | Rejected (2026-10-13, ability-tetikli modül turu: Utility sonrası Ability Haste, Utility sonrası %3 can, Movement sonrası tek seferlik hasar emme, Movement sonrası Attack Power, Movement başlangıç noktasında patlama; kullanıcı Bastion Slam dışındakileri beğenmedi) |
| Field Study, Axis Strike, Entropy Pulse, Crowd Shell | Rejected (2026-10-13, Utility tetikli modül turu: Utility kullanımıyla koşu boyu Energy Power, dört eksen şeridi hasarı, düşman canının %4'ü, yakın düşman sayısına bağlı Armor; kullanıcı hiçbirini beğenmedi) |
| Reflect Pulse, Clearing Pulse, Primed Element, Slow Time | Rejected (2026-10-13, ikinci Utility tetikli modül turu: Utility sonrası düşman mermilerini yansıtma, yok etme, sıradaki Offense yeteneğinin elementini değiştirme, düşman mermilerini yavaşlatma; kullanıcı hiçbirini beğenmedi) |

## Kaynaklar ve sınırlar

- Çalıştırılan modül kayıtları: [modules.json](../../../LightYearsGame/assets/content/data/modules.json).
- Common etki değerleri ve doğrulama geçmişi: [Common uygulama planı](../../MODULE_COMMON_IMPLEMENTATION_PLAN.md).
- Genel modül yerleşimi/nadirlik ilkeleri: [Modül Sistemi Tasarımı](../../MODULE_SYSTEM_DESIGN.md).
- Planned/Reworking fikirleri, kullanıcının 2026-10-05 ilettiği taslaklar ve geri bildirimlerdir. Bunları oyun davranışı, loot, son değer veya kesin rarity olarak sunma.

2026-10-13 metin ve aile açıklığı: Attack Speed sayıları yüzde olarak gösterilir (AS 15 = +%15; runtime değeri değişmez). Zone Cooler Utility modülüdür: Wings/Core/Engine/Sensor sırasıyla Offense/Defense/Movement/Utility cooldown ×0.79, Hull tüm Q/E/F/R ×0.90, Nose etkisiz. Glass Cannon Core Offense, Double Feed Core Utility kalır. Control olarak anılan dört yetenek kullanıcı kararıyla Utility içinde kalır; ayrı Hunt Clock Control yoktur. Zone Overclocker, stat/ability modifier yanında ana impact-damage, owner-attribute ve event-action gücüne de ek bölge artışını aktarır; mevcut baz değerler Overclocker yokken korunur. Modül ekranı kurulu modüllerde gerçek amplified zone bonusunu gösterir.
