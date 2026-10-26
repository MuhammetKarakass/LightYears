# Legendary denge önerisi — 2026-10-26

Durum: **son yanıtta sunulan denge düzeltmeleri kullanıcı tarafından "okey kaydet" ile onaylandı (2026-10-26); runtime uygulanmadı.** Boyutlar ve son yanıtta sunulmamış uygulama önerileri ayrıca açık kalır. Seçilmiş mekaniklerin sahibi [modules.md](modules.md), seçim geçmişi [kararlar](MODULE_DESIGN_DECISIONS.md). Bu belge onaylı değerlerle yeni önerileri ayırır; ikinci bir canlı içerik kataloğu değildir. Kullanıcının “tamam şimdi tüm legendarylerin dengelemelerini hesaplamalarını yapalım scaling değerlerini seviye değerlerini” isteği üzerine hazırlanmıştır.

Amaç: mevcut 11 yönün L1 temeli, etkin seviye artışı, stat katkısı, sabitleri ve doğrudan birlikte kullanım sınırlarını tek hesapta karşılaştırmak. Kapsam bu belge, katalog/karar bağlantıları ve kaynaklı hafızadır; oyun/JSON/engine değişmez. Kanıt aritmetik hesap, seviye tabloları ve kaynak karşılaştırmasıdır; oynanış dengesi kanıtı değildir.

## 1. Ortak seviye ve hesap sözleşmesi

- Doğal modül seviyesi 1–3. Etkin seviye `L = doğal seviye + bölge bonusu`; aile bölgesi +1, Gövde +2, diğer bölge +0. Böylece mevcut yerleşimde L1–L5 değerlendirilir. L4/L5 ayrı satın alınan kademeler değildir.
- `n = L − 1`; gemi/ability/weapon seviyesi bu n yerine geçirilmez. Ability ve weapon seviyeleri kendi sınırsız progression sözleşmelerini korur.
- `H = gerçek son MaxHealth`, `Hpre = can→kalkan dönüşümü uygulanmadan önceki MaxHealth`, `S = dönüşüm öncesi MaxShield`, `E = EP`, `A = AP`. Hasar anında ilgili kaynak değerler snapshot alınır.
- Bütün hasar rakamları aksi yazılmadıkça armor/kalkan/status/kritik öncesi ham değerdir. Energy mevcut kuralla kalkan kapasitesini ×1,50 tüketir; bu bonus gövde taşmasına uygulanmaz.
- Global CombatTick 0,25 sn: tick hasarı ×4 = kesintisiz DPS. Haste tick'i hızlandırmaz.
- Bekleme hesabında son kabul edilen yüzde yenilenme kullanılır: `C = max(1 sn, C_level / (1 + Haste/100) × kapsam çarpanları)`. Bu yeni tasarım formülüdür; runtime eski matematiği bu çalışmayla değiştirilmedi.
- Legendary yan almaz. Seviye, özellikle belirtilmedikçe hücre/şekil, tetik koşulu, mekanik hak sayısı veya status stack sayısını büyütmez.
- Modülden doğan hasar başka hasar üretici modülleri zincirleyerek kendini tekrar tetiklemez. Normal hasar alma, öldürme ve loot muhasebesi korunur. Her gerçek tetik tek kez işlenir.

## 2. Havuz ve hücre bütçesi

Aktif beğenilen/tasarlanan havuz **12**: Primary2 / Offense3 / Defense3 / Movement2 / Utility2. Hedef12 Parasite Hull ile doldu (2026-10-26); onaylı ayrıntıları [katalogda](modules.md). Heat Reactor, Segmented Shield ve Combo Sequencer dışarıda; Splinter Frame beklemede; zamanlama barı nadirliksiz yetenek modülüdür.

| Modül | Aile | Hücre bütçesi |
|---|---|---|
| Second Hardpoint | Primary | 3, L — onaylı |
| Meltdown Barrel | Primary | 3, L — öneri |
| Primed Arsenal | Offense | 3, L — öneri |
| Reaction Core | Offense | 3, L — öneri |
| Neural Hijacker | Offense | 3, L — öneri |
| Can→kalkan | Defense | 3, L — öneri |
| Defense şok dalgası | Defense | 3, L — öneri |
| Blink Drive | Movement | 2 — sayı onaylı, domino öneri |
| Electric hız alanı | Movement | 3, L — öneri |
| Twin Rack | Utility | 4, 2×2 kare — onaylı |
| Ters rarity stat çekirdeği | Utility | 3, L — öneri |

Hücre önerileri güç kıyasının parçasıdır; kullanıcının açık bıraktığı boyutları kesinleştirmez.

## 3. Second Hardpoint

**Onaylı:** ikinci Primary; tek aktif silah; fare tekerleği; yedekte otomatik reload; 3 hücre L; hedef başına diğer silaha 5 sn işaret; sadece hasar bonusu seviye alır.

**Öneri:** işaret bonusu `20 + 5n` yüzde puanı. **Onaylı (kullanıcı 2026-10-26):** silah geçişi sabit **1 sn** (önceki 0,35 sn önerisi geçersiz; aşağıdaki hesap 0,35 sn ile yapılmıştır ve 1 sn ile kazanç azalır). Reload her silahın doğal süresidir. Reload/geçiş/işaret süresi seviyeden etkilenmez. Yedekteki kısmi şarjör doğal reload süresinde dolar; her geçiş şarjörü anında sıfırlayıp doldurmaz. Ateş ederken reload ilerlemez. Şarjörsüz silaha bedelsiz yeni mühimmat/heat sıfırlama verilmez; kendi soğuma kontratı korunur.

| Etkin L | 1 | 2 | 3 | 4 | 5 |
|---|---:|---:|---:|---:|---:|
| Diğer silah işareti hasarı | +%20 | +%25 | +%30 | +%35 | +%40 |

Bir silahın doğrudan isabeti öteki silahın işaretini koyar; bonusu alan isabetler kendi karşı işaretini yeniler. İki silah aynı anda ateş etmez. Silah kaynak kimliği korunur; nova/modül hasarı işaret koymaz veya bonusu kullanmaz. Yerleştirilmiş DoT'nin her tick'i yeni işaret isabeti sayılmaz.

Hesap: aynı normal DPS'li iki silah, her biri 12 sn şarjör +2 sn reload; 4 sn ateş ve0,35 sn geçiş, işaret sürekli tutuluyor. Baseline ateş oranı12/14; modüllü oran `4/4,35 × (1+bonus)`. Baseline'a göre L1 yaklaşık **+%28,74**, L5 **+%50,19**; ilk işaretleme, ıskalama, farklı silah DPS'si ve empowered hariç. Bu sabit kazanç garantisi değildir.

## 4. Meltdown Barrel

**Onaylı:** şarjör yerine ısı; aşırı ısınma nova hazırlar. Son kullanıcı düzeltmesiyle anında patlama yerine yaklaşık **2 sn odaklanma**, ardından **500 yarıçaplı** Thermal nova. Oyuncu bu aralıkta konum alabilir; patlama odaklanma sonundaki gemi konumundan çıkar. Önceki350 yarıçap geçersiz.

Onaylı zamanlama: ısı dolunca Q/patlama gücü kilitlenir; odaklanma boyunca hareket serbest, ısınan silahın ateşi kapalı. Normal soğuma odaklanmayla paralel ilerler: ateş kilidi `max(2 sn, doğal soğuma/reload süresi)`. Böylece2 sn ayrıca reload'un üstüne eklenmez. Paralel soğuma son "okey kaydet" ile onaylandı; Second Hardpoint ile diğer silaha geçişin odaklanmayı etkileyip etkilemeyeceği açık kalır. Soğuma pending nova snapshot'ını azaltmaz.

**Onaylı hasar hesabı (son "okey kaydet"):** silahın kendi hasar hesabından ölçeklenme:

`NovaDamage = (0,30 + 0,04n) × Q`

`Q`: o ısı döngüsünde ateşlenen başarılı saldırıların, tek hedefe potansiyel normal ham hasar bütçesi. İsabet etme şartı aranmaz; ateş gerçekten üretilmiş olmalı. Tek saldırının çok hedefe/pierce zincirine vurması Q'yu çoğaltmaz. Normal weapon base + silah seviyesi + kendi AP/EP katsayısı dahil; empowered/crit, hedef status çarpanı, Second Hardpoint ve diğer modül bonusları hariç. Q nova öncesi snapshot alınır, sonra sıfırlanır. Son nova hasarına uygun outgoing çarpanları bir kez uygulanabilir; Q içinde önceden uygulanmış olamaz.

| Etkin L | 1 | 2 | 3 | 4 | 5 |
|---|---:|---:|---:|---:|---:|
| Döngü bütçesinden nova | %30 | %34 | %38 | %42 | %46 |

Sabit yarıçap **500** (kullanıcı seçimi); nova Thermal hasar verir ve **4 Ignite stack** uygular (kullanıcı 2026-10-26; önceki 2 stack önerisi geçersiz), seviye stack/yarıçap artırmaz.

**Erteleme (kullanıcı 2026-10-26):** ileride her Primary silaha şarjör değiştirme gelecek. Aşağıdaki şarjörsüz silah 6/2 (ve önerilen 6/1,5) modeli bu yüzden ertelendi; ısı eşlemesi o sistemle birlikte yeniden yapılacak. Q tek hedef bütçesi olsa da nova alanındaki her hedefe tam hesaplanan hasarı verir.

- Şarjörlü silahta100 ısı kapasitesi, tetik başına `100 / doğal şarjör atış sayısı` ısı; pellet/muzzle sayısı tekrar sayılmaz. Soğuma kilidi doğal reload süresi. Doğal son-şarjör empowered penceresi eşdeğer ısı aralığında korunur.
- Heat Laser kendi100 kapasite/saniyede38 ısı üretimi/saniyede25 ısı soğuması/2,5 sn aşırı ısınma kilidini ve ısı bantlarını korur; ikinci bağımsız ısı sistemi eklenmez. Q o doğal döngüdeki normal beam tick'lerini toplar; yüksek ısı normal hasar bandı silah davranışının parçasıdır.
- Isı/şarjörü olmayan diğer Primary'lere nominal6 sn doğal ateş ritmine eşdeğer100 ısı, ardından2 sn soğuma önerilir. Şarjörlü ve doğal ısılı silaha bu6/2 kuralı uygulanmaz.
- AS yükselirse şarjörlü silah daha çabuk ısınır; Q saldırı sayısından geldiği için hızlı silah hem fazla nova hem ayrıca yapay bir düz hasar katsayısıyla iki kez ödüllendirilmez.
- Normal soğumada Q aynı oranda azalır: `Q_new = Q_old × Heat_new / Heat_old`. Isı0 olunca Q0. Modül/silah değişimi ısıyı sıfırlıyorsa bütçeyi de sıfırlar; yedekte soğuyan silah eski atış hasarını sınırsız biriktiremez. Aşırı ısınmada nova snapshotından sonra Q0.

Rapid Laser L1 AP39: normal atış25,99, 48 atışlık Q1247,52. Nova L1 **374,256**, L5 **573,8592**. Önerilen paralel soğumayla12+2 sn yaklaşık döngüde tek hedef normal DPS89,11→L1 **115,84**, L5 **130,10**; yalnız nova menzilinde kalan hedef için, empowered/Ignite/crit hariç. Şarjörsüz6/2 modelinde L1 toplam teorik oran `(1+0,30)×6/8 = 0,975`; uzaktan düz atış zayıflar, yakın çok hedefli nova güçlenir. Gerçek hedef yoğunluğu zorunlu oyun testi konusudur.

## 5. Primed Arsenal

**Onaylı düzeltme:** yalnız tavan değil, saniyelik birikim de etkin seviye başına5 yüzde puanı artar: `%15/%20/%25/%30/%35`. Tavan bağımsız bir seviye tablosu yerine birikim hızından türetilecek. Önceki sabit%15/sn ve bağımsız2/2,25/2,5/2,75/3 tavanı geçersiz.

**Onaylı birikim süresi:** önceki L1×2 tavanını korumak için `T = 1/0,15 = 6⅔ sn` sabit birikim penceresi. Bu süre ve türetilen tavanlar son "okey kaydet" ile kabul edildi; süresiz sınırsız birikim yok.

`Rate = 0,15 +0,05n`

`Multiplier = 1 +Rate × min(ReadySeconds,T)`

`Cap = 1 +Rate × T`

| Etkin L | 1 | 2 | 3 | 4 | 5 |
|---|---:|---:|---:|---:|---:|
| Saniyelik birikim | +%15 | +%20 | +%25 | +%30 | +%35 |
| Türetilen güç tavanı | ×2 | ×2,3333 | ×2,6667 | ×3 | ×3,3333 |
| Tavana hazır bekleme | 6⅔ sn | 6⅔ sn | 6⅔ sn | 6⅔ sn | 6⅔ sn |

Birikim bekleme sürerken veya ilgili yetenek aktifken oluşmaz. Başarılı doğal kullanımda tek kez snapshot alınır ve sıfırlanır. Şarjlı yetenek için tam dolu/aktif olmayan durumda tek ortak rezerv; her charge ayrı paralel rezerv açmaz. Twin Rack'teki yetenekler bağımsız birikir. Bu ayrıntılar uygulama önerisidir.

Uygun ana sayısal çıktı (doğrudan hasar, tüm kanalın tick hasarı, summon saldırı hasarı, AP/AS buff miktarı) bir kez çarpılır; alan, süre, mermi sayısı, status stack'i ve cooldown çarpılmaz. Owner statı ile ondan türeyen çıktıya aynı bonus iki kez uygulanmaz. Kanal başında snapshot, her tick'te yeni birikim yok. Echo/replay yeni Primed rezervini tüketmez veya üretmez; kopyalanan orijinal çözülmüş güce yalnız replay'in kendi çarpanı uygulanır.

**Ana etki çarpanı yığılma kuralı (kullanıcı, 2026-10-26):** Primed Arsenal, Ultimate Protocol (×1,6), Cascade Reactor bonusu ve Monochrome Core aynı kullanımın ana etkisini çarpıyorsa birbirleriyle çarpılmaz; toplamsal tek grupta birleşir: `M_toplam = 1 + Σ(M_i − 1)`. Örnek Primed L5 tam dolu ×3,3333 + Ultimate ×1,6 = ×3,9333 (çarpımla ×5,3333 olurdu). Grubun dışındaki genel hasar çarpanları (AP/EP, status, kritik, hedef çarpanları) kendi yerinde kalır. Gruba girecek diğer modüller uygulama sırasında açıkça listelenmeli.

Sürekli kullanım verimi: `DPS_ratio = M×C/(C+T)`, aktif süre sıfır ve hasar sabit varsayımıyla. Onaylı hesapla tam dolu L1/L3/L5 Rocket(C6): **0,947/1,263/1,579**; SunBeam(C9): **1,149/1,532/1,915**. L1 eski burst kimliğini korurken üst seviyeler artık daha yüksek sürekli hasar da sağlayabilir. Haste C'yi düşürdükçe beklemenin fırsat maliyeti artar; bunlar gerçek oyun DPS ölçümü değildir.

## 6. Reaction Core

**Onaylı:** kullanıcı "reaction core okey" ile önceki yanıtta sunulan altı tepkimeyi, sayısal ilerlemelerini ve görünür sınırlarını kabul etti. Şarapnelin AP+EP katkısı ve aşağıdaki EP katsayısı da son "okey kaydet" ile onaylandı.

Uygulama ayrıntısı önerisi: yalnız Thermal(Ignite), Cryo, Electric ve Kinetic eşleşir; Photonic/Energy kendi stack'i olmadığından eşleşmez. Gerçek bir oyuncu saldırısı yeni stack eklediğinde oluşur, süre yenileme veya DoT tick'i tek başına tetiklemez. Yeni tür + en uzun kalan süreli uygun eski tür seçilir; eşitlikte sabit sıra Thermal/Cryo/Electric/Kinetic. İki türün bütün stack ve süreleri tüketilir; üçüncü status korunur. Aynı hedefte **ortak1 sn** yeniden tetik kilidi; kilitte status normal kalır, sonra yeni stack olayı gerekir. Tetikleyen saldırı kendi normal damage/status sırasını tamamlar, ardından tüketim yapılır; mevcut Frozen veya FreezeCooldown gibi ayrı kontrol etkileri kendiliğinden silinmez.

| Çift / Tepkime | L1 | Etkin seviye artışı | Sabit/sınır |
|---|---|---|---|
| Thermal+Cryo / Buhar | 2 sn hedef atış yönünde sapma | +0,2 sn/L | Sapma ±25 derece (kullanıcı 2026-10-26; önceki ±15); mevcut mermiyi geri dönüştürmez |
| Electric+Kinetic / Mıknatıs | 0,5 sn,300 birim/sn çekiş | +40 birim/sn/L | 250 yarıçap; CC bağışıklığı korunur |
| Thermal+Electric / Plazma | `60 +0,60E` Thermal patlama | Base+12 ve EP katsayısı+0,06/L | 250 yarıçap; yeni Ignite/status üretmez |
| Cryo+Kinetic / Kırılma | 2 sn Armor0 (kullanıcı 2026-10-26; önceki 1 sn) | +0,15 sn/L | Boss/miniboss Armor en fazla%50 azalır; diğer armor modifier'ları geri dönüşte korunur |
| Cryo+Electric / Süperiletken | 1 sn stun | +0,1 sn/L | CC bağışıklığı/direnci korunur |
| Thermal+Kinetic / Şarapnel | 6 parça; parça başına `10 +0,10A +0,10E` Kinetic | Parça base+2, AP ve EP katsayıları ayrı ayrı+0,01/L | Tek hedef en fazla2 parça; yeni Kinetic stack üretmez |

Örnek EP100: Plazma L1/2/3/4/5 **120/138/156/174/192**. AP100 ve EP100: şarapnel parça başına **30/34/38/42/46**, bir hedefe tavan **60/68/76/84/92**. Formül `10+2n +(0,10+0,01n)×(AP+EP)`; EP0 iken eski AP-only değerleri aynen korunur. Onaylı formül AP ve EP katkılarını toplar. Bu iki sonuç aynı rol değildir: şarapnel dağılır, Plazma alandaki her hedefe tam hasar verir. Süre/alan/CC faydaları tek bir DPS sayısıyla eşitlenemez. Status tüketim bedeli (ör. Electric gelen hasar artışı, Ignite kalan hasarı, Kinetic armor kırma) ayrıca kaybedilir. Reaction hasarı aynı veya diğer hedefte yeni Reaction doğurmaz.

## 7. Neural Hijacker

**Onaylı:** öldürücü vuruşla ele geçirme; eşzamanlı sayı sınırsız; sonra ölüm; `Duration = 3 +0,4n sn`.

| Etkin L | 1 | 2 | 3 | 4 | 5 |
|---|---:|---:|---:|---:|---:|
| Dost kalma | 3 | 3,4 | 3,8 | 4,2 | 4,6 sn |

Son "neural okey" ile kabul edilen temel paket: normal ve uygun elit; boss/miniboss/özel görev nesnesi hariç. Ele geçirilirken özgün maksimum canın%25'iyle hayata döner, doğal saldırı hasarının%50'siyle savaşır. Bu iki oran seviye almaz; süre tek gelişimdir. Normal saldırı ritmini korur, kendi gemisini veya oyuncuyu hedeflemez. Dostların öldürmeleri yeni ele geçirme başlatmaz; böylece sınırsız adet korunur, kendi kendine büyüyen zincir kurulmaz.

Orijinal öldürücü olayda Gold/XP/kill credit bir kez verilir; geçici dostun yok oluşunda ikinci ödül veya yeni on-kill olayı yoktur. Dostun gerçek düşman öldürmesi ayrı gerçek kill olarak ödüllendirilebilir fakat Hijacker tetikleyemez. Sahip yok olduğunda geçici dostlar kapanır; yeni koşuya taşınmaz.

Kaba üst hesap: oyuncu saniyedeλ uygun düşman öldürüyor ve dostlar erken ölmüyorsa ortalama adet `λ×Duration`. λ3 için9/10,2/11,4/12,6/13,8. Her düşmanın doğal tek hedef DPS'si20 varsayılırsa%50 hasarla toplam90→138 DPS; bu düşman DPS20 ölçümü değil örnek varsayımdır. Hedef bulma/yol/erken ölüm daha da düşürür. Asıl risk süre değil spawn ve öldürme hızıdır; buna gerçek oyun testi gerekir.

## 8. Can→kalkan dönüşümü

**Onaylı:** `MaxHealth=1`; `MaxShield = S +(0,50 +0,04n)×Hpre`; önceden mevcut kalkan korunur. HP ölçekli başka etkiler dönüşüm sonrası1 MaxHealth'i okur. Kullanıcı 2026-10-26 taban oranı %40'tan %50'ye çıkardı ve başka değişiklik istemedi (kalkan gecikmesi vb. korunur); modül diğer kalkan modülleriyle build kurulması için tasarlanır.

| Etkin L | 1 | 2 | 3 | 4 | 5 |
|---|---:|---:|---:|---:|---:|
| Hpre'den ek kalkana | %50 | %54 | %58 | %62 | %66 |

Önerilen sıra: kendi dönüşümü dışındaki normal MaxHealth katkıları çözülür; Hpre tek snapshot; dönüşüm en son uygulanır. Her yeniden hesaplamada orijinal stat kaynaklarından başlanır; 1 HP tekrar tekrar dönüştürülmez. Vital Alloy/HP tabanlı Armor gibi aşağı akış etkiler1'i okur, Hpre'ye geri besleme yapmaz. Takma/sökme bedelsiz iyileştirme değildir: önceki doluluk oranları yeni havuza taşınır; başka kaynaktan heal veya shield ödülü oluşmaz. Bu ayrıntı uygulama sözleşmesi olarak henüz onaylanmadı.

Modülsüz Fighter gemiL1 H250/S140/Armor2: yeni MaxShield L1/L3/L5 **265/285/305**, HP1. Nötr toplam etkili dayanıklılık eskiden `140+250×1,02=395`; dönüşümden sonra **266,02/286,02/306,02**. Aynı6 sn tam dolum korunursa kalkan regen23,33/sn→44,17/47,50/50,83/sn. Başlangıç tek-darbe dayanıklılığı azalırken tekrar dolabilen savunma büyür. Energy saldırısına karşı aynı EHP kullanılamaz.

GemiL20 Fighter H1010/S520/Armor40: eski nötr EHP1934; yeni L1 **1026,4**, L5 **1188**. Kalkan regen86,67→170,83/197,77/sn. %50 tabanla da ham toplam dayanıklılık düşer; kazanç diğer kalkan modülleriyle (Shield Battery, Polar vb.) birleşen kalkan build'inden beklenir. Bu yüzden “canın%40'ı kalkana eklendi, toplam tanklık artıyor” sonucu yanlıştır. Low-health koşulları HP1/MaxHP1=%100 olduğundan otomatik tetiklenmez; can≤%30 şartı için HP'nin gerçekten1'in altında olması gerekir. Eski büyük MaxHealth'e göre oran hesaplamak hem bu karara hem HP ölçek kaybına aykırıdır.

## 9. Defense şok dalgası

**Onaylı çekirdek:** başarılı Defense kullanımında500 menzilli Energy dalgası; base+HP ölçeklemesi. Son kullanıcı isteği: hasarı Meltdown Barrel'e çok yakın olsun. Önceki50+10n+(0,20+0,02n)HP bu yeni hedefe göre düşük kaldı ve öneri olarak bırakıldı.

**Onaylı yeni formül (son "okey kaydet"):**

`Damage = (50+7n) +(2,25+0,30n)×max(MaxHP−100,0)`

Bu formül, ilk100 MaxHP'yi HP katkısından çıkarır; taban hasar ayrıca korunur. 100 HP eşiği ve yeni katsayılar son "okey kaydet" ile kabul edildi. Meltdown'un silah seviyesi/AP/EP üzerinden büyüyen bütçesini, HP ölçekli bir formülle her olası build'de bire bir eşitlemek mümkün değildir. Kalibrasyon referansı: Fighter gemi ve Rapid Laser silahı eşit seviyede, empowered/crit/status/modül bonusları hariç.

| Etkin L | 1 | 2 | 3 | 4 | 5 |
|---|---:|---:|---:|---:|---:|
| Base | 50 | 57 | 64 | 71 | 78 |
| 100 üzeri HP katsayısı | %225 | %255 | %285 | %315 | %345 |
| H250 ham hasar | 387,5 | 439,5 | 491,5 | 543,5 | 595,5 |
| H300 ham hasar | 500 | 567 | 634 | 701 | 768 |
| H1 ham hasar | 50 | 57 | 64 | 71 | 78 |

| Fighter gemi ve silah seviyesi | Etkin modül L | Meltdown ham nova | Şok dalgası | Fark |
|---|---:|---:|---:|---:|
| 1 | 1 | 374,256 | 387,5 | +%3,54 |
| 1 | 5 | 573,8592 | 595,5 | +%3,77 |
| 10 | 1 | 1181,664 | 1197,5 | +%1,34 |
| 20 | 1 | 2078,784 | 2097,5 | +%0,90 |
| 20 | 5 | 3187,4688 | 3217,5 | +%0,94 |

Farklar referans koşullarında%4'ün altında; farklı silah/HP yatırımı bu oranı değiştirebilir. Yalnız ham tek patlama hasarı kıyaslandı, tetik sıklığı veya sürekli DPS eşitlenmedi. Energy kalkan tüketiminde×1,50 avantajına sahip; bu yüzden aynı ham hasarda kalkana karşı nova ile eşdeğer değildir. Thermal nova Ignite katkısı ayrı kalır.

Her gerçek başarılı aktivasyon tek dalga; cancel/kanal tick'i/telegraph/hak tüketmeyen replay ayrı dalga üretmez. Her hedef dalga başına en fazla bir kez hasar alır. Dalga kritik vurmaz, itme veya mermi temizleme eklenmez (bu kullanım sınırları öneridir).

**İç bekleme (kullanıcı, 2026-10-26): 6 sn.** Dalga modül genelinde en fazla 6 sn'de bir çıkar; bekleme sırasında yapılan Defense kullanımı dalga üretmez ve beklemeyi yenilemez. Defense yeteneğinin kendisini engellemez; Haste ve seviye bu beklemeyi kısaltmaz. Önceki 1 sn kilit önerisi geçersiz. Gerekçe: tek patlama hasarı Meltdown novasına eşitlendi ama tetik sıklığı eşitlenmemişti; Twin Rack + kısa beklemeli Defense + Haste ile birkaç saniyede bir dalga çıkabiliyordu. 6 sn ile H300 L5 tavanı hedef başına 768/6 = 128 ham DPS; Meltdown'un yaklaşık 14 sn döngüsüne göre hâlâ daha sık, fakat Energy ve kritik vurmama farkıyla kabul edilebilir.

## 10. Blink Drive

**Onaylı:**3 hak, sıralı dolum; hareket girdisi yönü; sabit mesafe;6−0,3n sn/hak; kullanımlar arasında1 sn;2 sn MS buff, +%20+3n yüzde puanı; buff süre yeniler, oran birikmez;2 hücre.

| Etkin L | 1 | 2 | 3 | 4 | 5 |
|---|---:|---:|---:|---:|---:|
| Hak dolum süresi | 6 | 5,7 | 5,4 | 5,1 | 4,8 sn |
| Hareket hızı | +%20 | +%23 | +%26 | +%29 | +%32 |
| Boştan3 hak dolumu | 18 | 17,1 | 16,2 | 15,3 | 14,4 sn |

Yeni sabit öneriler: mesafe **350** dünya birimi; girdisiz mevcut hız yönü, hız da sıfırsa burun yönü. Yol ara noktaları taranmaz, ancak varış engel içine olamaz; engel öncesinde güvenli varış. Ek dokunulmazlık veya status temizleme yok. Blinkin kendisi kat edilmiş seyir mesafesi/hız/AB çalışma süresi sayılmaz; Electric alanına sahte hız vermez. Sonraki2 sn buff gerçek uçuş hızını etkilerse o uçuş normal sayılır.

AB kapasitesinin ek hak üretmemesi, dolumun bağımsız Blink saati olması önerilir; kapasite yatırımı bu modülle değer kaybeder, UI açık göstermeli. Mevcut AB yenilenme eşyalarının Blink'e aktarılması ayrı açık eşleme kararıdır; bu rapor onları sessizce devre dışı bırakan runtime değişikliği yapmaz. L1–L5 aralığında en düşük doğal dolum4,8 sn olduğundan ek2 sn tabanına ihtiyaç yok; gelecekte dış dolum çarpanı eklenecekse ayrıca denge gerekir.

## 11. Electric hız alanı ve fren patlaması

**Son kullanıcı kararı:** hız hem hasarı hem alanı büyütür. Azami hız yüzdesi yerine mutlak gerçek hız kullanılır. Alan300 hızda300 yarıçapla açılır; bundan sonra her2 hız başına yarıçap1 artar, sürekli hesaplanır. Şimdilik yarıçap üst sınırı yok. Önceki%50 eşik, yüzdeye bağlı yarıçap ve400 tavanı geçersiz.

`v <300: alan kapalı`

`v ≥300: Radius =300 +(v−300)/2`

**Yüksek hız azalması (kullanıcı, 2026-10-26):** 700 hızın üstünde hız katkısı her 200 hızda bir yarıya iner. Yarıçap ve hasar çarpanı gerçek hız yerine etkin hızı `v_eff` kullanır:

`v ≤700: v_eff = v`

`v >700: k = floor((v−700)/200); v_eff = 700 + 200×(1 − 0,5^k) + (v−700−200k)×0,5^(k+1)`

700–900 arası artış ½, 900–1100 arası ¼, 1100–1300 arası ⅛ … Etkin hız 900'e asimptotik yaklaşır; böylece yarıçap 600'e, hasar çarpanı ×3'e yaklaşır ama geçmez. ZeroDrag gibi hız tavanını kaldıran etkiler sonsuz alan/hasar üretmez. Önceki "yarıçap üst sınırı yok" kararı bu kuralla değişti.

300'de300,301'de300,5,400'de350,500'de400,700'de500; 800'de525, 900'de550, 1100'de575, 1300'de587,5 yarıçap. Hız artışları100'lük basamağa yuvarlanmaz. Blink/teleport mesafesi gerçek uçuş hızı sayılmaz.

**Kabul edilen taban sayılar:** önceki EP ve etkin seviye katsayıları korunur. **Onaylı hız hasar katsayısı:** `SpeedMultiplier =v/300` (alan aktifken). Oran ve fren öncesi snapshot son "okey kaydet" ile kabul edildi.

`BaseTick =6 +1,5n +(0,08+0,01n)×EP`

`BaseBurst =65 +10n +(0,50+0,05n)×EP`

`TickDamage =BaseTick×v_eff/300`

`BurstDamage =BaseBurst×v_eff_before_brake/300`

Patlamada fren öncesi hız ve yarıçap snapshot'ı kullanılır; fren sonrası sıfıra yaklaşan hız patlama hasarını silmez. Seviye hız eşiği veya yarıçap formülünü değiştirmez.

| Etkin L; EP100, v300 | 1 | 2 | 3 | 4 | 5 |
|---|---:|---:|---:|---:|---:|
| Tick | 14 | 16,5 | 19 | 21,5 | 24 |
| Kesintisiz alan DPS | 56 | 66 | 76 | 86 | 96 |
| Fren patlaması | 115 | 130 | 145 | 160 | 175 |

| Gerçek hız | Yarıçap | Önerilen hasar çarpanı | L1,EP100 alan DPS | L1,EP100 patlama |
|---|---:|---:|---:|---:|
| 299 | Alan yok | — | 0 | — |
| 300 | 300 | ×1 | 56 | 115 |
| 400 | 350 | ×1,3333 | 74,67 | 153,33 |
| 500 | 400 | ×1,6667 | 93,33 | 191,67 |
| 600 | 450 | ×2 | 112 | 230 |
| 700 | 500 | ×2,3333 | 130,67 | 268,33 |
| 900 (v_eff 800) | 550 | ×2,6667 | 149,33 | 306,67 |
| 1100 (v_eff 850) | 575 | ×2,8333 | 158,67 | 325,83 |
| 1300 (v_eff 875) | 587,5 | ×2,9167 | 163,33 | 335,42 |
| Sonsuz (v_eff 900) | 600 | ×3 | 168 | 345 |

300 hız satırında patlama değeri, o hızdan kontrollü fren ve hazır charge varsayımıdır. Charge ilk300 hızda kendiliğinden dolmuş sayılmaz.

Charge100 ve yüksek hızda en kısa5 sn tam dolum korunur. Yüzde/azami hız referansını kaldırmak için yeni mutlak dolum önerisi: saniyede `20×clamp((v−300)/300,0,1)`; v300'de0, v450'de10, v≥600'de20 charge/sn. Düşük hızda dolum durur, mevcut charge korunur; hasar vurmak şart değil. Bu dolum uyarlaması ayrı öneridir; yarıçapta veya hasarda tavan getirmez.

Kontrollü fren taslağı: oyuncu fren/ters itki girdisiyle, önceki hızı en az300 iken0,30 sn içinde en az%40 hız kaybederse ve charge100 ise tek patlama. İtme/stun/çarpışma/teleport tek başına tetiklemez. Hazır patlama, hız300 altına düştüğü için alanı kapatmadan önce işlenir; charge sıfırlanır. Fren girdisi/erişilebilir yavaşlama koşulu henüz runtime/playtest ile doğrulanmadı.

**Status (kullanıcı 2026-10-26):** alan her tick'te (0,25 sn) hedefe **1 Electric stack**, fren patlaması **4 stack** uygular. Önceki "tick stack üretmez, ilk temas 1, patlama 2" önerisi geçersiz. Sonuç: alanda kalan hedef saniyede 4 stack alır; Electric stack tavanına hızla ulaşır ve Reaction Core tepkimelerini sık tetikler (Reaction'ın hedef başına 1 sn kilidi bunu sınırlar). Reaction ile tüketilmiş stack sırf alan devam ediyor diye hemen yeniden üretilmez. Bu uygulama ayrıntıları kullanıcı tarafından ayrıca seçilmedi.

Yüksek hız azalmasıyla mutlak tavan oluşur: v300'den sonsuz hıza yarıçap en fazla2 kat, alan4 kat, hedef başına DPS3 kat; aynı yoğunlukta kapsanan toplam hasar potansiyeli en fazla yaklaşık12 kat. Önceden bu oran 900 hızda ulaşılıyordu ve üstünde sınırsızdı. Kullanıcı genel hasarın gerekirse düşürülebileceğini söyledi (2026-10-26); taban tick/patlama değerleri şimdilik korunur, oyun testinde fazla çıkarsa önce taban düşürülür. Bu bir aritmetik ilişki; düşman yoğunluğu/gerçek hız/saha sınırı ölçümü değildir.

## 12. Twin Rack

**Onaylı:**8 yetenek,4 hücre2×2, bağımsız cooldown, sabit×1,30; başarılı gerçek kullanım aynı slot eşinin kalan beklemesinden `10+2n` yüzde puanı iade; iade iade tetiklemez, aile koşulu yok.

| Etkin L | 1 | 2 | 3 | 4 | 5 |
|---|---:|---:|---:|---:|---:|
| Eşin kalan beklemesinden iade | %10 | %12 | %14 | %16 | %18 |
| Eşin10 sn kalanında iade | 1 | 1,2 | 1,4 | 1,6 | 1,8 sn |

Kalan bekleme r ile çarpılır: `Remaining_after = Remaining_before×(1−r)`; toplamCD'ye her defasında r düşülmez. Hazır eşe iade depolanmaz; charge'lı yetenekte yalnız aktif yenilenen hak etkilenir. Sadece başarılı gerçek aktivasyon; kanal tick'i, iptal veya masrafsız replay eş iadesi değildir. Bağımsız satın alınan ability level/Scrap maliyeti önerilir; tek yükseltme iki yeteneği birden yükseltmez.

Eşit tabanCD C0, sıfır aktif süre, kusursuz dönüşümlü iki yetenek, tek eş kullanımından iade varsayımında:

`T_each =1,30×C0×(1−r)/(1−r/2)`

`PairThroughput / single_without_module =2×(1−r/2)/(1,30×(1−r))`

L1/L3/L5 oranları **1,624 /1,664 /1,707**; doğal dört slotu sekize çıkarmanın teorik kullanım kapasitesidir, gerçek DPS çarpanı değildir. Hedef/menzil/animasyon/aktif süre ve ability başına bağımsız güç yatırımı sonucu değiştirir. “8 yetenek=2 kat hasar” hesabı yapılmaz. Birden fazla eş kullanımı, farklı cooldown veya başka refund kaynağında bu kapalı formül geçmez; doğrudan olay çizelgesi gerekir.

## 13. Ters rarity stat çekirdeği

**Onaylı:** kullanıcı "ters rarity okey" ile5/4/3/2/1 ağırlıkları ve önceki yanıtta sunulan aile başına stat/seviye artışlarını kabul etti. Modül bazında özel sayım ayrıntıları ayrıca uygulama önerisidir.

`W_f = o ailedeki uygun ana modüllerin rarity ağırlıkları toplamı`

Common5 / Uncommon4 / Rare3 / Epic2 / Legendary1. Modül sayılır, hücre ve modül seviyesi tekrar ağırlık eklemez. Kendisi, sistem modülleri, ability slot kabukları, nadirliksiz yetenek modülleri, Trophy/parça sayılmaz. Ayrı Hunt Clock kayıtları gerçek aileleriyle bir ana modül sayılır; F1 birleşimi bir takılı modül sayılır. Confluence Core ile birlikte bulunabilir ama biri diğerinin verdiği statı yeniden modül sayısına dönüştürmez.

| Aile / stat | Her1 ağırlık için L1 | Seviye başına artış | L5 |
|---|---:|---:|---:|
| Offense / AP ve EP ayrı ayrı | 1 | +0,125 | 1,5 |
| Defense / MaxHealth | 4 | +0,5 | 6 |
| Primary / AS | +%0,375 | +0,05 yüzde puanı | +%0,575 |
| Movement / hareket rating | 0,05 | +0,0075 | 0,08 |
| Utility / yüzde Haste | +%0,25 | +0,025 yüzde puanı | +%0,35 |

Kullanıcı 2026-10-26 bütün ağırlık başına değerleri yarıya indirdi (önceki Offense 2+0,25n, Defense 8+n, Primary %0,75+0,10n, Movement 0,10+0,015n, Utility %0,50+0,05n geçersiz). Gerekçe: 4 Common Offense ile L1'de AP+40, Fighter L1 taban AP'sini (39) ikiye katlıyordu.

Her satırın sonucu kendi W_f'siyle çarpılır. Luck/Gold gibi ayrı yanlar eklenmez. Örnek4 Common Offense: W20; L1 **AP+20,EP+20**, L5 **AP+30,EP+30**. Aynı4 modül Legendary olursaW4; L1+4/+4, L5+6/+6. Örnek2 Rare Defense: W6; L1 **HP+24**, L5 **HP+36**. Bu buff gemi seviyesiyle ikinci defa otomatik çarpılmaz; mevcut stat yatırımlarına eklenir.

Confluence karşılaştırması:4 Common Offense L1'de Epic Confluence AP/EP+8; yeni çekirdek+20, karşılığında önerilen3 hücre ve düşük rarity'li ana modüllere bağlı build. L5'te Confluence+24, yeni çekirdek+30. Her ana modülü yüksek rarity'ye yükseltmek güç kaybına yol açabilir; bu modülün amaçlanan karar maliyetidir. Upgrade anındaki kayıp saklanmamalıdır.

## 14. Denge sonuçları ve doğrulama sınırı

- İkinci denge turu (kullanıcı, 2026-10-26): Electric alanında 700 üstü hız katkısı her 200 hızda yarıya iner (etkin hız 900'e, yarıçap 600'e, çarpan ×3'e asimptotik); şok dalgası 6 sn iç bekleme; ters rarity değerleri yarıya; can→kalkan %50+4n; Buhar ±25°, Kırılma L1 2 sn; Primed/Ultimate/Cascade/Monochrome ana etki çarpanları toplamsal grupta. Aşağıdaki eski maddelerle çelişen yerlerde bu madde geçerlidir. Python hesaplayıcısı bu turla güncellenmedi.

- Önceki Twin/Blink/Neural/can→kalkan ilerlemeleri korundu. Son turda Reaction (EP şarapnel katsayısı hariç), Neural temel paketi, Twin ve ters rarity sayıları kabul edildi. Son "okey kaydet" ile Primed6⅔ sn penceresi, AP+EP şarapnel katsayıları, şok formülü, hız/300 hasar katsayısı ve paralel Meltdown soğuması da onaylandı. Son yanıtta sunulmayan boyut/uygulama önerileri açık kalır.
- En yüksek çarpan riski: Twin+Primed (8 bağımsız hazır rezerv), Neural+sürü öldürme hızı, Reaction+çok hızlı status üretimi ve Electric alanın çok hedefli teması. Bunları aynı tek hedef DPS sütununda eşit ilan etmek doğru olmaz.
- Can→kalkan, HP ölçekli shockwave/Vital Alloy/Health Bridge'i zayıflatır. Düşük-can koşulları eski HP'ye göre değil, yeni1/1 oranına göre değerlendirilir.
- Reaction'da1 sn hedef kilidi ve Neural'da dost kaynaklı zincirin kapanması son açık onay kapsamında korunur. Şok dalgasında1 sn yeniden tetik kilidi ise hâlâ öneridir.
- Son kullanıcı kararıyla Electric alanın hem hasarı hem yarıçapı gerçek hızla büyür, yarıçap tavanı yoktur. Blink mesafesi, shockwave500 ve Meltdown500 sabit; Meltdown2 sn odaklanma alır.
- Loot dağılımı/son12. modül, kullanıcı tarafından açık bırakılmış boyutlar ve koşu ekonomisinin gerçek Scrap edinimi bu aritmetikle çözülmüş sayılmaz.

Hesaplayıcı: [Python](../outputs/legendary-balance-calculations-2026-10-26.py), [üretilen değerler](../outputs/legendary-balance-calculations-2026-10-26.json). Çalıştırma: `python outputs/legendary-balance-calculations-2026-10-26.py`. Aritmetik/monotonluk kontrolleri geçti; oyun, E2E veya playtest yapılmadı. Belge değişikliği olduğundan oyun build'i gerekmiyor.

Kaynaklar: [seçilmiş modüller ve bölge kuralları](modules.md), [ability formülleri/global tick](ABILITY_SCALING_CATALOG.md), [gemi/weapon/Energy/EHP](GDD.md), [gemiler](../LightYearsGame/assets/content/data/ships.json), [silahlar](../LightYearsGame/assets/content/data/weapons.json), [mevcut modül içeriği](../LightYearsGame/assets/content/data/modules.json), [ModuleLevelTable](../LightYearsGame/include/gameplay/module/ModuleLevelTable.h), [doğal seviye doğrulaması](../LightYearsGame/src/gameplay/module/ModuleLoadout.cpp).
