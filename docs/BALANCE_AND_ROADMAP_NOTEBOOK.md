# Light Years — Balance & Roadmap Notebook

Bu belge yalnız tasarım kararlarını, geçici denge varsayımlarını, playtest sonuçlarını ve gelecek işleri tutar.

- Güncel runtime mimarisi: [PROJECT_DOCUMENTATION.md](PROJECT_DOCUMENTATION.md)
- Güncel içerik ve sayısal envanter: [CURRENT_IMPLEMENTATION_CATALOG.md](CURRENT_IMPLEMENTATION_CATALOG.md)

Buradaki bir değer açıkça **onaylı** veya **uygulandı** olarak işaretlenmedikçe production balance değildir. JSON ve kod ile çelişen tarihsel değerler kaynak kabul edilmez.

## 1. Sabit tasarım kararları

### 1.1 Attribute sahipliği

- Player ve enemy combat statları runtime `AttributeSystem` üzerinde Owner Attribute olarak kalır.
- `EnergyMax`, `EnergyPower` olarak değiştirilmiştir. `EnergyPower` tüketilen kaynak değil, reaktör gücüdür.
- `EnergyRegen` kaldırılmıştır.
- EnergyPower referans değeri `100` kabul edilir; bu bütün gemilerin değerini 100'e sabitlemez.
- Ship natural growth generic `{ attributeId, perLevel }` girdileriyle tanımlanır.
- Owner Attribute ownership progression, ship profile veya enemy sistemi tarafından değiştirilmez; bu sistemler yalnız sahip oldukları modifier kaynaklarını yönetir.
- Yüzde benzeri rating'lerde canonical ölçek `100 = %100` biçimindedir.

### 1.2 Armor

Armor hard cap olmadan diminishing return uygular:

```text
ArmorDamageReduction = Armor / (Armor + 100)
FinalDamageMultiplier = 100 / (Armor + 100)
```

| Armor | Damage reduction |
| ---: | ---: |
| 50 | yaklaşık %33.3 |
| 100 | %50 |
| 200 | yaklaşık %66.7 |
| 300 | %75 |
| 500 | yaklaşık %83.3 |

Crit sırası, Shield/Hull yerleşimi ve final rounding politikası Armor refactor'ından bağımsızdır.

### 1.3 Damage type döngüsü

- Photonic varsayılan damage type'tır ve eşit-sayı hedefinin dışındadır.
- Energy kalkan kırma, Kinetic armor'a karşı bitirici/penetration, Cryo alan tutma/slow, Thermal süreli hasar ve Electric chain/proc kimliğine yakındır.
- Damage type sayıları mümkün olduğunca dengeli tutulur; rol çeşitliliği uğruna birebir eşitlik zorlanmaz.
- Crystal Barricade ve Relay Prism gelen vuruşun damage type'ını taşır.
- Wing Sentinels ve Combat Sentry varsayılan Photonic kullanır.

### 1.4 Ability ve weapon mimarisi

- Ability/weapon level artışı generic attribute modifier ve scaling-rule hattını kullanır.
- Aynı `attributeId + coefficient/perLevel` kavramı paralel özel sistemlerle tekrar oluşturulmaz.
- Feature-local davranışlar generic definition'a optional alan yığını olarak eklenmez.
- Presentation tek `presentationProfileId` üzerinden typed feature-local registry ile çözülür.
- Global base crit multiplier Owner Attribute üzerinden başlar ve başlangıç değeri `1.5x`tir.
- Empowered cadence weapon kimliğidir; silaha ve upgrade'e göre değişebilir.
- AttackSpeed yüzde cadence modelinde `1 + max(0, AttackSpeedRating) / 100` çarpanını kullanır ve yalnız bir kez uygulanır.

## 2. Fighter vertical-slice referansı

### 2.1 Ship L1 ve natural growth

| Alan | L1 / level başına |
| --- | ---: |
| MaxHealth | 250 / +75 |
| AttackPower | 35 / +3 |
| EnergyPower | 35 / +2 |
| Armor | mevcut base / +2 |
| Luck | mevcut base / +1 |

AttackSpeed, CriticalChance, AbilityHaste ve movement için Fighter natural growth yoktur. Diğer player ship profilleri daha sonra belirlenecektir.

### 2.2 BasicRapidLaser

| Parametre | Değer |
| --- | ---: |
| Magazine | 48 |
| Fire rate | 4.0 atış/sn |
| Base reload | 2.0 sn |
| Base damage | 12 |
| AP ratio | 0.40 |
| Empowered bonus base | 2 |
| Empowered EP ratio | 0.10 |
| Empowered cadence | Her başarılı 6. atış ve son 6 round |
| Empowered crit | Guaranteed; Owner CriticalDamage kullanır |

```text
L1 normal = ceil(12 + 35 × 0.40) = 26
L1 empowered = ceil((26 raw + 2 + 35 × 0.10) × 1.5) = 48
```

Magazine dağılımı `35 normal + 13 empowered`, toplam `1534` hasardır. Bu değer enemy production TTK kararı değildir.

### 2.3 Weapon L1–L15 progression

```text
BaseDamage(L) = 12 + 10 × (L - 1)
APRatio(L) = 0.40 + 0.05 × (L - 1)
EmpoweredBase(L) = 2 + 1 × (L - 1)
EmpoweredEPRatio(L) = 0.10 + 0.03 × (L - 1)
```

Fire rate, magazine, reload, empowered cadence ve base crit weapon level ile değişmez. Scrap maliyetleri geçici ekonomi placeholder'ıdır.

## 3. Enemy foundation kararları

### 3.1 Data-driven enemy modeli

- Yeni düşmanlar generic `EnemyActor` üzerinden oluşturulur.
- Şasi `ShipDefinition`, loadout `EnemyCombatProfile`, karar parametreleri `EnemyBehaviorProfile` içinde tutulur.
- `EnemyDefinition` bu içerikleri bağlar; `EnemyFactory` oluşturmayı gerçekleştirir.
- Enemy türü başına C++ child class zorunlu değildir. Yalnız farklı lifecycle veya motor entegrasyonu gerekiyorsa özel actor düşünülür.
- `DummyEnemy` ability/weapon test hedefidir ve enemy progression/AI kapsamına dahil edilmez.

### 3.2 Enemy hasar ve stat yaklaşımı

- Enemy saldırıları okunabilir authored base damage taşır.
- Varsayılan enemy AttackPower ve EnergyPower değeri `0`dır.
- AP/EP scaling yalnız açıkça özel bir enemy mekaniği gerektirirse etkinleştirilir.
- Genel güç artışı enemy-owned stat ve outgoing-damage modifier'larıyla uygulanır.
- Enemy level aynı türün Health, Shield, Armor ve outgoing damage büyümesini kendi profiline göre çözer.
- Aynı tür içinde profile tarafından sınırlanmış, seed tabanlı deterministik spawn varyasyonu kullanılabilir.

```text
Base enemy profile
+ enemy-specific level growth
+ seed tabanlı sınırlı spawn varyasyonu
= resolved enemy stats
```

Enemy progression mevcut generic `AttributeGrowthEntry` tipini tekrar kullanacaktır. Yeni struct yalnız yeni ownership, lifecycle veya validation sınırı temsil ediyorsa eklenir.

### 3.3 Enemy behavior

Mevcut foundation; Approach, Strafe, HoldRange, target acquisition/validity, aim/rotation, movement intent, Hold/Pulse slot komutları, priority ve retry sağlar.

Threat, behavior tree, coordinated formations ve boss kararları vertical slice sonrasına ertelenmiştir.

## 4. Arena wave vertical slice

```text
Idle → Spawning → WaitingForClear → InterWaveDelay → Spawning / Completed
                                              ↘ Failed
```

- Runtime yalnız kendi spawn ettiği enemy referanslarını takip eder.
- Dummy veya harici enemy wave completion'ı engellemez.
- Spawn `EnemyFactory` callback'i üzerinden yapılır.
- Spawn interval büyük `deltaTime` içinde kalan süreyi kaybetmez.
- Ara wave'lerde delay uygulanır; son wave temizlenince doğrudan `Completed` olur.
- Restart yalnız encounter-owned düşmanları temizleyip runtime'ı yeniden başlatır.
- Mevcut üç-wave kompozisyonu vertical-slice test içeriğidir, production encounter balance değildir.

### 4.1 Mevcut wave progression kararı

- ✅ Arena `EncounterWaveRuntime` endless test akışında üç authored composition template'i döngüsel kullanır. İlk wave hedefi `3` enemy'dir; sonraki wave'lerde temel hedef her wave `+1` artar.
- ✅ Enemy level her `3` wave'de bir artar ve level-up dalgasında hedef enemy sayısından bilinçli `2` enemy düşülür. Bu monoton enemy-count artışı değildir; level artışının ek zorluğunu düşman sayısında kısmen telafi eden kasıtlı vertical-slice test kararıdır. Örnek: `3, 4, 5, 4, 5, 6, 5, ...`.
- ✅ Arena enemy level cap'i `15`, aktif encounter-owned enemy cap'i `24` ve progression seed'i `1337`dir. Bu değerler test level'a aittir; production encounter balance kararı değildir.
- Endless test akışında son wave hedeflenmez; teorik olarak sınırsız devam edebilir. Spawn offset ve uzun süreli telemetry optimizasyonları şu an öncelikli değildir; testte pratik oynanabilirlik sınırları yeterlidir.
- ✅ Wave runtime yalnız level ve deterministik variation seed üretir; stat matematiğini bilmez. Seed, encounter seed + wave + wave-local spawn + enemy ContentId üzerinden deterministik türetilir.
- ✅ Enemy profile Health, Shield, Armor ve outgoing damage büyümesini kendi kimliğine göre çözer. Growth yalnız enemy-owned modifier ve `ShipRuntime` source-owned shield contribution üzerinden uygulanır.
- AP/EP varsayılan olarak `0` kalacak.
- ✅ Varyasyon Health, Shield, Armor ve outgoing damage ile sınırlıdır; fire rate, cooldown, behavior mesafesi, loadout ve projectile sayısı rastgele değişmez.
- Fire rate, cooldown, behavior mesafesi, loadout ve projectile sayısı rastgele değiştirilmeyecek.

### 4.2 Arena lifecycle ve gözlemlenebilirlik

- ✅ Arena encounter lifecycle açık `Idle / Running / Completed / Failed` durumlarıyla yönetilir. Oyuncu yoksa başlangıç `Failed` olur; oyuncu ölümü yalnız encounter-owned enemy'leri temizler ve harici actor'ları korur.
- ✅ Level restart eski player ship'i yok eder, bekleyen respawn callback/timer durumunu temizler, `PlayerManager` run state'ini resetler ve yeni player + encounter yaşam döngüsünü kurar.
- ✅ `PlayerRespawnSystem`, player ship destroy delegate handle'ını sahiplenir ve `Clear()`/destructor sırasında unbind eder. Böylece level teardown sonrasında stale callback kalmaz.
- ✅ `EncounterWaveRuntime`, UI veya debug tüketicileri için yan etkisiz `EncounterWaveSnapshot` üretir. Snapshot wave numarası, toplam wave, planlanan/spawn edilmiş/canlı/kalan enemy, inter-wave süre ve enemy level içerir; runtime state'in sahibi olmaya devam eder.
- Snapshot temizleme yapmaz; expired/pending-destroy enemy referanslarının canonical temizliği runtime `Tick()` akışındadır.

### 4.3 Tamamlanan parça: snapshot-driven encounter HUD

- ✅ `EncounterHUDController`, `EncounterWaveSnapshot` üzerinden `WAVE current / total`, kalan threat, enemy level, inter-wave countdown ve completion metnini gösterir.
- ✅ HUD encounter matematiğini yeniden hesaplamaz; snapshot'ı presentation view-model'e dönüştürür.
- Arena lifecycle state machine ile wave runtime state machine birleştirilmeyecek.
- Ödül, victory/game-over redesign, wave JSON migration ve büyük UI polish bu parçanın dışındadır.

## 5. Playtest hedefleri

- Wave başına gerçek mücadele süresi
- Aynı anda yaşayan enemy sayısı
- Player'ın aldığı toplam hasar
- Shield/Hull hasar dağılımı
- Enemy türü başına time-to-kill
- Silah ve ability kullanım oranları
- Reload sırasında oluşan risk penceresi
- Ölüm nedeni ve arena dışına çıkma sıklığı

| Tarih / build | Wave | Loadout | Süre | Alınan hasar | Ölüm nedeni | Gözlem / takip |
| --- | ---: | --- | ---: | ---: | --- | --- |
|  |  |  |  |  |  |  |

## 6. Ertelenmiş içerik kararları

- Evolve sistemi vertical slice sonrasına ertelendi.
- Başlangıç paketi sistemi planlanmıyor.
- Yaklaşık 40–45 ability ilk içerik hedefidir; araştırma sırasında konuşulan 60 kesin production sınırı değildir.
- Napalm Beacon, Vector Loom, Shield Molt, Parallax Twin, Time Slip, Zero Drag ve Foldspace Arena fikirleri ayrı ability kartlarında değerlendirilir; burada uygulanmış kabul edilmezler.
- Boss, elite, affix, procedural director ve gelişmiş AI mevcut wave progression kapsamına dahil değildir.
- Diğer beş player ship'in gerçek base/growth balance değerleri belirlenmemiştir; Fighter üzerinden ilerlenir.

## 7. Teknik refactor backlog'u

Bu maddeler tek toplu refactor olarak uygulanmayacaktır.

1. ✅ Snapshot tüketen minimal encounter HUD tamamlandı.
2. Arena vertical slice'ını playtest ile ölç: wave süresi, alınan hasar, shield/hull dağılımı, reload riski ve death rate.
3. Monolitik GasLite test runner'ını aynı executable düzenini koruyarak sistem bazlı kaynaklara böl.
4. `ShipDefinition` içindeki legacy `health`, `speed` ve `primaryWeaponId` sahipliklerini canonical kaynaklara migrate ederek temizle.
5. `LevelOne` akışını yeni encounter sistemine geçir.
6. Yeni encounter geçişi doğrulandıktan sonra kalan eski stage/wave sınıflarını kaldır.
7. Domain loader'larını birleştirmeden yalnız tekrar eden küçük JSON yardımcılarını ortaklaştır.
8. Ability progression validation'ını ASC runtime sorumluluğundan validator katmanına taşı.
9. Gerçek include veya ownership baskısı oluşursa `WeaponStructs` dosyasını davranış değiştirmeden fiziksel sorumluluklara ayır.
10. İşlevsel sınırlar sabitlendikten sonra content klasörlerinin fiziksel taşınmasını değerlendir.

Öncelik oynanabilir vertical slice ve eski/yeni paralel sistem sahipliklerinin temizlenmesidir. Büyük klasör taşıma veya toplu yeniden yazım yapılmaz.

## 8. Açık tasarım soruları

- Enemy level başına Health, Shield, Armor ve outgoing damage büyüme aralıkları playtest sonucunda nasıl ayarlanmalı?
- Wave sayısı ve enemy level artış cadence'i nasıl olmalı?
- Enemy spawn varyasyon yüzdeleri tür bazında hangi sınırları aşmamalı?
- Damage type kombinasyonları birden fazla tag taşıdığında nasıl birleşmeli?
- Barrier, permanent Shield ve Armor arasında hedeflenen savunma kimliği nedir?
- Vertical slice için kabul edilebilir wave süresi ve player ölüm oranı nedir?
- Save/load ve run özeti hangi progression/seed verilerini taşımalıdır?
