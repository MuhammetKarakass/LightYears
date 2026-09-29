---
type: status-report
verified_on: 2026-09-07
verification: source-review-only
verified_worktree_state: dirty
---

# Proje durum raporu — 7 Eylül 2026

## Bugünkü nokta

Light Years, savaş mekaniklerinin denendiği kapsamlı bir C++17 prototipi. Kaynakta input → ability/silah → hedef sorgusu → hasar → shield/health → ölüm/XP → respawn bağlantıları var. Güncel giriş `GameApplication::GameApplication` içinden `ArenaTestLevel`; ana menü veya dalgalı bölüm doğrudan açılmıyor. Bu, oynanabilirlik için kod yolunun var olduğunu gösterir; bu incelemede oyun açılmadı ve başarılı bir oyun oturumu doğrulanmadı.

`ArenaTestLevel::OnGameStart` sekiz sabit dummy üretir; dördünün health değeri 99999, dördünün 50. `DummyEnemy::Tick` ateş eder; bunlar hasarsız dekor değildir. Dummy shield değeri 50'ye sabitlenir. Bunların tamamı test düzenidir, nihai düşman dengesi değildir. Bu 7 Eylül snapshot'ında görülen Q/E/F/R, Frozen Throng / Relay Prism / Glacial Pressure / Ironclad Protocol idi; güncel eşleme için `DefaultAbilityLoadout.cpp` esas alınır.

## İnceleme yöntemi ve sınırı

- AGENTS.md ve `docs/PROJECT_DOCUMENTATION.md` talimatları okundu. Git başlangıçtan beri dirty; kullanıcıya ait kaynak, JSON, CMake, log ve üç ana dokümantasyon dosyası değişikliği vardı. Commit/push ve kaynak düzenlemesi yapılmadı; `.obsidian/workspace.json` korundu.
- HEAD `16a27553cd67e7a6f554e672b1482b14bd1b6f7d`; vault altında Git'in gösterdiği son commit 2026-08-25, `Ability Pack 2`; bu Obsidian ayar geçmişidir. `.gitignore` içindeki `*.md` kuralı vault notlarını dışladığı için notların son güncelleme commit'i belirlenemedi. Notlarda 14 Ağustos ve daha eski commit bilgileri kalmıştı. HEAD'den çalışma ağacına kaynak farkları başlangıç noktası oldu; değişmeyen runtime, input, progression ve level akışları ayrıca okundu.
- Grafik önce kullanıldı. Grafik yeni SimulationTime/MovementPolicy dosyalarını bulamadı; güncel çalışma ağacı için yetersiz kaldığından dosya envanteri ve doğrudan kaynak okumalarıyla tamamlandı. Grafik sonucu tek başına yokluk kanıtı sayılmadı.
- [[Source File Index]] tüm 946 `.h/.cpp` dosyasının envanterini verir; bu sayı ayrıntılı okunmuş dosya sayısı değildir. [[System Index]] sistem bazında mevcut davranışı ve doğrulama sınırını gösterir.
- Ayrıntılı/odaklı okuma: Application/World/Actor, asset/timer, fizik step ve filtre, render sırası; GameApplication/GameLevel/ArenaTestLevel/LevelOne; Player/PlayerManager, input, MovementComponent; ship kaynakları, damage girişi, progression; bootstrap/catalog/registration; Time Slip ve Ironclad aktivasyon/cleanup. Projectile, effect, Temporal Recall/Foldspace ve HUD yolları seçilmiş fonksiyon düzeyinde incelendi.
- Her ability actor'ının tüm fazları, tüm weapon handler'ları, boss AI durumları, tüm shader/asset hata yolları, bütün test assertion'ları ve bütün balance formülleri okunmadı. `Abilites/` tasarım/evolve tabloları uygulanmış içerik sayılmadı.
- Build, CTest ve oyun testi çalıştırılmadı. Eski binary veya eski 2/2 test sonucu bu ağacın doğrulaması değildir. Markdown bağlantıları, envanter kaynak linkleri ve takip edilen dokümantasyon diff'i ayrıca kontrol edildi. Vault notları Git tarafından ignore edildiği için normal diff'te görünmez; yerel dosyalar güncellendi, ignore kuralına dokunulmadı.

## Ana döngü

| Parça | Kaynakta doğrulanan durum | Eksik / sınır |
|---|---|---|
| Başlangıç | JSON bootstrap başarısızsa quit; başarılıysa test arena yüklenir | Kullanıcıya sunulan normal başlangıç seçimi yok |
| Savaş | Input, primary fire, ability registry, damage ve kaynak tüketimi bağlı | Kombinasyonların gerçek zamanlı doğruluğu ölçülmedi |
| Hedef ve ödül | Dummy ateşi; EnemyActor::Blew → score/XP; GameLevel::OnActorSpawned ödül delegate'lerini Player'a bağlar | Test arena sürekli düşman/dalga üretmiyor |
| İlerleme | Ship XP → level modifier; Player yeni gemiye modifier ve satın alınmış level'ları uygular | Scrap award/purchase için üretim çağrı noktası bulunmadı |
| Ölüm ve devam | Arena respawn sistemi; yaşam tükenince game over yolu | Arena restart dünyayı yeniden yüklemiyor |
| Alternatif bölüm | LevelOne::InitGameStages dalga, boss, infinite stage kurar; MainMenu yolu mevcut | LevelOne::Tick, GameLevel::Tick'i atlıyor; HUD controller tick entegrasyonu eksik |
| Kalıcılık | Player ömrü boyunca bellekte progression | Disk save/load akışı bulunmadı; respawn restore save değildir |

## Öncelikli bulgular

### Kritik doğruluk riskleri

1. **Arena restart eksik.** `LightYearsGame/src/level/GameLevel.cpp`, `GameLevel::OnRestartLevel`, PlayerManager'ı sıfırlayıp yalnız pause durumunu kaldırıyor. `LevelOne::OnRestartLevel` ayrıca LoadWorld çağırıyor; ArenaLevel/ArenaTestLevel böyle bir override sağlamıyor. Eski world/actor/HUD kalırken player kaydı siliniyor. Raw Player delegate bağlantıları da `GameLevel::OnActorSpawned` ve `Player::SpawnSpaceShip` içinde mevcut: ömrü bitmiş alıcıya erişim riski var; crash burada çalıştırılarak kanıtlanmadı.
2. **LevelOne HUD controller güncellemesi atlanıyor.** `LightYearsGame/src/level/LevelOne.cpp`, `LevelOne::Tick` doğrudan `World::Tick` çağırıyor. Controller tick döngüsü `GameLevel::Tick` içinde. Bu nedenle alternatif bölümde ability view model yenilemesi çalışacak diye kabul edilemez.
3. **Destroy callback yeniden girişi.** `LightYearsEngine/src/framework/Actor.cpp`, `Actor::Destroy`, pending kontrolünden sonra `onActorDestroyed.Broadcast(this)` yapıp ancak ardından `Object::Destroy` çağırıyor. Callback aynı actor'da Destroy çağırırsa guard henüz işaretlenmemiş olur. Bu somut sıralama riski; mevcut oynanışta tetiklendiği doğrulanmadı.

### Geliştirme engelleri

- **Scrap ekonomisi erişilebilir değil:** `Player::AwardScrap` ve `TryPurchaseAbilityLevel` implementasyonları var; üretim `.cpp` taramasında tanımları dışında kullanım yok. Grafik inbound sonucu da boş. UI üzerinden kazan–satın al döngüsünün tamamlandığı söylenemez.
- **Güncel build/test kanıtı yok:** Bu 7 Eylül tarihsel snapshot'ında CMake dokuz test kaydı olarak raporlanmıştı. Güncel çalışma ağacında CMake sekiz CTest kaydı içeriyor; yeni temporal, movement, weapon override ve overcap değişiklikleri birlikte doğrulanmalı.
- **Normal run seçimi çözülmemiş:** Test arena ile LevelOne farklı başlangıç/respawn yaklaşımları kullanıyor. İlk oynanabilir hedefin arena dalgası mı LevelOne mı olduğu tasarım kararı; burada önerilen en küçük iş mevcut yolları düzgün erişilebilir kılmak.

### Ertelenebilir teknik borç

- `Timer::TickTimer` repeat olduğunda birikeni sıfırlıyor, büyük dt'de kalan süreyi taşımıyor. Hassas cadence için drift riski; bütün timer'ları yeniden yazmak yerine gereken çağrıda ölçülmeli.
- Application değişken dt ile `PhysicsSystem::Step(dt)` çağırıyor; Box2D dört substep kullanımı sabit zaman accumulator'ı değildir. Uzun frame/collision/temporal senaryoları ölçülmeli.
- Generic actor pooling bulunmadı: `World::SpawnActor` her çağrıda `make_shared` yapıyor. Render bucket/scratch bellek tekrar kullanımı pooling değildir. Ölçüm olmadan havuzlaştırma öncelikli değil.
- Asset yükleyiciler başarısızlıkta null döndürebiliyor; tüm UI/sprite tüketicilerinin fallback güvenliği incelenmedi.
- Katalogdaki eski default-slot yorumları ve belgelerdeki eski loadout'lar drift yaratmış. Gerçek kaynak `GetDefaultAbilityLoadout`; sayı tabloları ölçülmüş denge yerine tuning verisi olarak okunmalı.

## Öneri sırası ve iş büyüklüğü

Boyutlar tek geliştirici için yaklaşık aktif çalışma; mevcut build sağlığına göre değişir.

| Sıra | İş | Gerekçe ve beklenen fayda | Bağımlılık | Boyut |
|---|---|---|---|---|
| 1 | Arena restart + Player yaşam süresi doğrulaması | Bozuk yeniden başlatmayı ve olası geçersiz delegate erişimini kapatır | Güncel build kurulması | S–M, 1–2 gün |
| 2 | LevelOne HUD tick düzeltmesi | Mevcut ability HUD'sunu alternatif bölümde kullanılabilir yapar | Bağımsız; 1 ile aynı smoke senaryosu | S, yarım gün |
| 3 | Actor Destroy yeniden giriş testi ve dar düzeltme | Çift callback/rekürsiyon riskini kapatır | Engine lifetime hedefi | S, yarım–1 gün |
| 4 | Scrap ödülü ve tek satın alma UI akışı | Mevcut progression API'sini oynanışa bağlar | 1–2; ödül kaynağı seçimi | M, 2–3 gün |
| 5 | Mevcut arena/LevelOne giriş seçimi ve kısa run kabul testi | Test sahası dışındaki akışı erişilebilir ve tekrar test edilebilir yapar | 1–2 | S–M, 1–2 gün |
| 6 | Temporal/overcap/override kombinasyon doğrulaması | Yeni sistemlerin ölüm, iptal, pause ve world geçişinde cleanup güvenini artırır | Güncel test build'i | M, 2–3 gün |
| 7 | Save/load veya yeni evolve içerikleri | Ancak hedef run ve kalıcılık kapsamı kararlaştırılınca değer üretir | 4–5; tasarım kararı | M–L, kapsam belirlenecek |

## Sonraki beş küçük Codex görevi

1. **Arena restart sözleşmesini tamamla.** ArenaTestLevel restart'ında yeni world/player/ship oluştur; eski timer ve delegate'ler yeni koşuya taşınmasın. Tamamlanma: pause ve game-over ekranından restart ayrı ayrı test edilir; tek oyuncu/gemi, başlangıç canları ve güncel loadout görülür; eski düşman ödülü yeni oyuncuyu iki kez etkilemez; ilgili test hedefi geçer.
2. **LevelOne HUD tick zincirini bağla.** LevelOne'ın GameLevel controller tick'ini tam bir kez çalıştırmasını sağla. Tamamlanma: ability activation/cooldown görünümü ve respawn sonrası bağlanma test edilir; pause davranışı değişmez; gereksiz UI yeniden kurma yok.
3. **Actor Destroy yeniden girişini güvenceye al.** Destroy callback'inden yeniden Destroy çağıran test ekle ve küçük lifecycle düzeltmesini yap. Tamamlanma: callback bir kez, fizik/spatial cleanup bir kez; engine lifetime testleri geçer.
4. **Scrap için en küçük kazan–harca döngüsünü bağla.** Mevcut Player API'sine tek ödül kaynağı ve tek slot upgrade kontrolü bağla; yeni ekonomi framework'ü kurma. Tamamlanma: ödül bir kez gelir, yetersiz bakiye reddedilir, level/max-level doğru, bakiye HUD'da değişir, respawn level'ı korur ve yeni run sıfırlar.
5. **Başlangıçta mevcut iki oyun yolunu erişilebilir yap.** Mevcut menüye ArenaTestLevel/LevelOne seçimi ekle veya mevcut giriş akışını buna bağla. Tamamlanma: iki yola giriş, pause, menüye dönüş, tekrar giriş ve restart smoke testi kaydedilir; test dummy değerleri normal düşman tasarımı diye sunulmaz.

## Belge uyuşmazlıkları

Dashboard Crescent Reaver/Scorch Drive/Rail Burst, Snapshot Void Gate/Ion Storm/Glacial Pressure, ana proje belgesi Time Slip/Foldspace/Temporal Recall default'larını gösteriyordu; güncel kod bunlardan farklı. Dashboard ve Snapshot düzeltildi. Üç ana belge kullanıcı tarafından zaten değiştirildiğinden içerikleri yeniden yazılmadı; güncel denetim bağlantısı eklendi. Eski 2/2 test iddiaları tarihsel, güncel build kanıtı değil. Sistem ve sınıf notlarındaki yer değiştirmiş ability/effect kaynak yolları güncel SAS karşılıklarına taşındı. Evolve mimari kontratı, evolve seçim ekranının veya kalıcı unlock sisteminin uygulanması anlamına gelmiyor.
