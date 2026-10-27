# Unique modüller — uygulama sınırı ve doğrulama

2026-10-27, `D:/LightYears-unique`, `unique-modules`. Tasarım sahibi [modules.md / Unique](modules.md#unique-modüller--7); bu belge uygulama kararlarını ve destek sınırını kaydeder. Vega kaynaklarına veya Git durumuna müdahale edilmedi.

## Sahipler

- `ModuleDefinition::unique` ve `ContentIdSchema`: ikinci nadirliksiz sınıf; sayıların tek sahibi `modules.json` içindeki `unique` kuralıdır. Loader sınıfa yabancı alanları, çok hücreli şekli, geçersiz sayıları ve normal rule eklenmesini reddeder. İç yerleşim seviyesi 1 yalnız nötr depolama değeridir; UI seviye göstermez.
- `ModulePlacementRules::GetAdjacentPlacements`, `SharesCellEdge`, `GetCoveredCells`, `FacingOffset`: kenar, döndürme ve F1 hücrelerinin ortak sahibi.
- `ModuleTriggerService`: geminin ASC'sine ait tek tetik sınırı. Her bildirimin placement/rule kimliği ve natural bilgisi vardır. Zorlama, gerçek olayın doğal alıcıları bittikten sonra uygulanır; placement başına tekrar engellenir. Receiver bağlantıları RAII ve generation korumalıdır. Aktör kimliği/raw aktör pointer registry'si yoktur.
- `UniqueModuleRuntime`: komşu placement seçimi, JSON sayıları, zamanlayıcı ve aile alias politikası. Etki uygulamasını ilgili mevcut runtime API'sine bırakır; yetenek aktive etmez.
- `ModuleResolver`: bölge bonusundan ayrı ek seviye; `FindLevelBonus` toplamı, `FindZoneLevelBonus` yalnız bölgeyi verir. Standart kabuk için `ScopedAbilityRule::requiredSlot` mevcut geçici seviye yolunu kullanır.
- HUD: Unique mor tonu; Mirror oku kurulu hücrede, sürükleme önizlemesinde, envanter satırında ve detayda. Geliştirici envanterine her Unique'ten bir tane verilir.

## Tetik desteği

| Kural | Doğal bildirim | Zorlama ve yarı güç |
|---|---|---|
| Trigger buff; aile kurallarından üretilen buff dahil | Var | Düz/override büyüklük × güç; çarpanın 1'den farkı × güç. Hareket hızı da aynı şekilde. Son tetik mevcut buff büyüklüğünü değiştirip süresini yeniler/uzatır; üst üste eklenmez. |
| Event action | Var | `amount` × güç: kalkan/can/Afterburner, geçici kalkan, zamanlı iyileşme, cooldown iadeleri, Cryo, Gold. Tamsayı sonuçlar aşağı yuvarlanır. Koşul, eşik, her-N sayacı ve şans bypass edilir; doğal sayaç ilerlemesi korunur. İç cooldown korunur. |
| Kullanılan yeteneğe iade (`RefundUsedCooldown`) | Var | Bu placement'ın son doğal aksiyonundaki yetenek kimliğini kullanır. Henüz böyle bağlam yoksa etkisiz; başka yetenek seçilmez. |
| `ShieldOverflowToHealth` | Bağımsız tetik değildir | Aynı placement'ın RestoreShield aksiyonunun dönüşüm katsayısı olarak mevcut yolda uygulanır. |
| Trigger ile açılan damage window | Var | Window açılır; hasar, ek hasar ve azaltma büyüklüğü ölçeklenir. Sonraki vuruşun kendi damage koşulu korunur. |
| Owner burst: ability use, shield break, kill-charge | Var | Cooldown hazırsa şarj koşulunu bypass ederek patlar. Hasar/katsayılar ve itme/çekme gücü ölçeklenir. Doğal kill sayacı tüketilmez. |
| Reflection Shield | Window açılışında var | Window açılır; yansıtılan hasar kesri ölçeklenir. Gelecekteki gerçek kalkan hasarı gerekir. |
| Release Valve / Trail Mines / Speed Lance | Var | Patlama hasarı / mayın hasarı / örneklenen hızın hasar katsayısı ölçeklenir. Pencere, örnekleme süresi, mesafe ve geometri korunur. |
| Ricochet / Guided Salvo / Aftershock / kill explosion | Var | **Zorlama desteklenmiyor:** gerçek kaynak vuruş, hit-group tek-kullanım bütçesi, hedef/status snapshot'ı gerekir. Yeni bir kaynak vuruş uydurulmaz ve mevcut shot lockout aşılmaz. |
| Phase Slip | Faz başlangıcında var | Afterburner koşulunu bypass eder; faz kilidi ve cooldown korunur. Sonraki patlama hasarı ölçeklenir; fazın binary koruması tam sürede kalır. Zorlanmış fazın ertelenmiş patlaması module-trigger suppression taşır. |
| Legendary Bulwark Pulse | Var | Kendi cooldown'u korunur; taban hasar ve can katsayısı ölçeklenir. Mirror da placement'a özgü aile filtresinde çalışır. |

Süre, yarıçap, iç bekleme, hedef/mermi sayısı ve binary kontrol durumları güçten etkilenmez. Owner burst Ignite ve Reflection Electric gibi ayrı tamsayı status adetleri korunur; event action'ın ana `amount` alanındaki Cryo/Gold ise ölçeklenip aşağı yuvarlanır. Modül üretimi projectile/explosion yolları kendi suppression sözleşmelerini kullanır.

### Desteklenmeyen diğer olay türleri

Aşağıdakiler merkezi servise doğal bildirim ve force API'si eklenmeden bırakıldı. Dolayısıyla bu türlerin kendisi Relay/Echo/Profit Share kaynağı değildir; placement'ta ayrıca desteklenen bir yan kural varsa o kural çalışır. Amplifier'ın seviye çözümlemesi bunlardan bağımsızdır.

- Escape Velocity Sonic Boom, Spare Capacitor charge trickle ve Shield Battery reserve-fill olaylar?: mevcut ayr? hareket, periyodik ?arj da??t?m? ve kalkan kaynak sahibinde placement bazl? fired/lockout s?n?r? yoktur. Bunlar genel force dispatch i?ine ta??nmad?; mevcut do?al yollar? korunur.
- Status tepkimeleri/spread/freeze/consume ve TargetSlow; Shield Rupture: hedefe ait canlı status/kalkan kaybı, vuruş bağlamı ve hedef başına kilit gerektirir. Bazı çözümlenmiş aggregate'lerde placement kimliği de kaybolur. Genel Force bunları hedef uydurmadan yeniden oluşturamaz.
- Run-counter ilerlemesi (Tempered Hull, Trophy Core ve öldürme/aile kullanım sayımları), Last Stand revive: kalıcı loadout kredi/charge işlemi ve gerçek kullanım/öldürme/ölüm bağlamı gerekir. Genel forced etkiyi sahte ilerlemeye veya canlı gemide revive işlemine çevirmek güvenli kabul edilmedi.
- Epic Prism Core / Blood Contract / Storm Capacitor: mevcut birleşik combat state kaynak placement'ı korumaz. Cascade Reactor ve Field Mastery: aile sırası veya belirli yeteneğe kalıcı kullanım/seviye kredisi gerekir. Bunları doğru desteklemek ilgili runtime'ın kaynak kimliğini/transaction sınırını genişletmeyi gerektirir.
- Legendary Second Hardpoint, Meltdown Barrel, Primed Arsenal, Reaction Core, Neural Hijacker, Parasite Hull, Blink Drive, Ion Comet ve Twin Rack'in olay parçaları: sırasıyla gerçek silah değişimi/işaret, heat bütçesi, belirli yetenek tüketimi, hedef-status çifti, ölümcül hedef, ölüm/possession, oyuncu hareket girdisi, hız/charge geçmişi veya eşlenmiş slot kullanım bağlamı gerekir. Genel forced giriş bu durumları uydurmaz veya kaynak kilitlerini aşmaz.
- Sürekli attribute/damage/family-stat/mount/shield şekillendirme, Assault Vector, Emergency Protocol'un eşik görünümü, Hardlight Hull, Underdog Core ve sürekli hazır-zaman birikimleri bağımsız tetik sayılmaz. Bunlar için force API'si yoktur.

Bu sınırlamalar oyun tarafındadır; bu uygulama için engine request yoktur.

## Kararlaştırılan belirsizlikler

- Mirror rotation 0 yukarı (`0,-1`), sonra saat yönünde sağ/aşağı/sol. Alias yalnız ön hücrenin placement'ına ve ability-use aile koşuluna uygulanır; Primary alias almaz.
- Pulsar yalnız canlı, dünyada, combat içindeki gemide ilerler. Portal transit ve possession sırasında durur. Reapply/taşıma süreyi korur; kaldırıp yeniden takmak sıfırlar. Aynı türün savunma amaçlı duplicate içerikleri aynı pulse fazını paylaşır.
- Echo gecikmesi combat dışında ilerler; gemi oyun dışında veya ölü ise durur. Kaynak ya da hedef kaldırılınca/reboot'a girince veya kenar teması kaybolunca bekleyen Echo iptal edilir. Ateşleme anındaki aktif çözümlenmiş kural değerleri kullanılır.
- Her doğal olayda placement politikası bir kez işlenir; birden fazla rule aynı olayda doğal çalışsa da bir Echo planlanır. Ayrı doğal olayların Echo'ları ayrı olaydır. Force uygun rule'ların hepsini dener; bir rule'un cooldown'u diğer bağımsız rule'ları kapatmaz.
- Echo buff'ı ayrı stack olarak eklemez; aktif buff'ı yarı büyüklüğe getirip normal refresh/extend sözleşmesini uygular. Doğal sonraki tetik yeniden tam büyüklüğe döner.
- Amplifier normal modülün seviye kullanan ana ve yan değerlerini artırır; yanlar bölge seviyesi almaz ve yanın kullanılabilirlik şartı depolanmış modül seviyesidir. Unique ve Ultimate Protocol seviyesiz olduğu için Amplifier'dan etkilenmez. Standart Q/E/F/R kabuğu geçici slot-seviyesi sağlar.
- Profit Share placement başına oyun-zamanı cooldown kullanır; combat dışında da geçer. Forced bildirimler ödeme veya yeni Unique tetik zinciri üretmez.
- Yarı güç yorumu ve bağlam gerektiren istisnalar yukarıdaki destek tablosunda açıkça belirtilmiştir.

## Şema komşuluğu

Q/E/F/R sırasıyla unlocked, AbilityBay dışı kenar komşu hücre sayıları:

| Gemi | Q | E | F | R |
|---|---:|---:|---:|---:|
| Fighter | 2 | 3 | 3 | 2 |
| Breacher | 2 | 2 | 2 | 2 |
| Interceptor | 1 | 2 | 2 | 2 |
| Conductor | 2 | 2 | 2 | 2 |
| Aegis | 2 | 2 | 2 | 2 |
| Cryo Controller | 1 | 2 | 2 | 2 |

24 kabuğun hepsinde en az bir normal komşu vardır. Sayım hücrenin normal ve açık olmasını ölçer; hücredeki mevcut varsayılan sistemin önce taşınması gerekebilir.

## Doğrulama

Yeni `UniqueModulesE2E.cpp`, gerçek ship/ASC/loadout/applier yolunu kullanır. Kontrollü geometri için Fighter şemasının altına test içinde Hull hücreleri eklenir; shipped şema komşuluk denetimi ayrıca değiştirilmemiş altı içerikte yapılır. Duplicate-kind savunması yalnız test lookup'ına kopyalanmış Unique tanımlarıyla denenir; canlı içerikte aynı id'nin ikinci kurulu örneği yasaktır.

Mevcut odaklı senaryolar: UltimateProtocol (slot değiştirme ve geçici scoped kurallar), TemperedHull (Bastion ve event action cooldown'ları), ReleaseValve (generated activation sınırı), EpicUtilityMovement (Phase Slip), LegendarySimpleModules (Bulwark), ModuleScreen (presentation ve envanter). Tam CTest çalıştırılmadı.

Komutlar ve sonuçların kesin kaydı: `outputs/unique-validation.md`. Build/test logları yalnız bu worktree'nin `outputs/` klasöründedir.
