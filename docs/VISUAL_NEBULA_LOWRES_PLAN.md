# Düşük Çözünürlüklü Nebula — Sol/Luna Uygulama Planı

**Tarih:** 2026-10-05 · **Durum:** uygulama ve otomatik doğrulama tamamlandı; manuel kabul/commit bekliyor · **İlgili:** [VISUAL_LIBRARY_PLAN.md](VISUAL_LIBRARY_PLAN.md), `LightYearsVisual/README.md`

Amaç: arka plan nebula'sını (`nebula.frag`) çıkış çözünürlüğünün bir kesrinde çizip büyüterek GPU dolum maliyetini düşürmek. Kesir `pipeline.json` içinden **değiştirilebilir** olacak; `1` bugünkü davranışın aynısıdır.

---

## 0. Neden (ölçülmüş)

Ölçüm makinesi GTX 950M + Intel HD 530, 1920×1080, `--visual-capture` ile 180 kare duvar saati (kare sınırı 60 fps; sayılar sınır üstü ek süreyi gösterir, kesin GPU süresini değil).

| Durum | Süre (180 kare + açılış) | Pipeline ek maliyeti |
|---|---|---|
| Pipeline kapalı (`--visual-off`) | 4,22 sn | — |
| Pipeline açık, bugünkü hâl (Release) | 7,29 sn | ~17 ms/kare |
| Pipeline açık, Debug | 8,3 sn | aynı (~3,1 sn) → CPU değil, **GPU dolum hızı** |
| Nebula yerine sabit renk (deneme) | 4,20 sn | ≈ 0 |
| Nebula 5→3 oktav (deneme) | 5,62 sn | ~7,8 ms/kare |
| Nebula domain-warp'sız (deneme) | 4,63 sn | ~2,3 ms/kare |
| Bloom kapalı | 6,56 sn | ~13 ms/kare (bloom ≈ 4 ms) |
| Vignette, gezegen, dither, `virtualHeight` | — | her biri ~0,5–1 ms |

Kaynak: `nebula.frag` her piksel için 3 `fbm` × 5 oktav hesaplar ve `ProceduralBackdrop::DrawBackground` bunu **iki katman** (`nebulaFar`, `nebulaNear`) olarak tam çözünürlükte, her karede çizer. Nebula "yumuşak, düşük frekanslı" olduğundan düşük çözünürlük görünümü az değiştirir.

Bu plan yalnız nebula'yı kapsar. Gerçek oynanış CPU ölçümü (Release'te profiler/log kapalı) ve diğer maliyetler §7'de.

---

## 1. Kapsam

| Dahil | Hariç (bu plan değil) |
|---|---|
| `nebula.resolutionDivisor` ayarı (`pipeline.json`, sıcak yükleme) | Kalite ön ayarları (Düşük/Orta/Yüksek) |
| Nebula için düşük çözünürlüklü ara doku + premultiplied birleştirme | Bloom seviye/çözünürlük ayarı |
| `1` = bugünkü yol (bayt-eş çıktı) | Nebula güncelleme hızını düşürme / önbellek |
| Oyun (`ProceduralBackdrop`) ve galeri çağıranları, README | Adaptif kalite (kare süresine göre otomatik) |
| Doğrulama araçları (galeri capture, süre ölçümü) | Shader hata olunca yalnız o özelliği kapatma |

---

## 2. Tasarım sözleşmesi

### 2.1 Ayar
- `PipelineSettings::nebulaResolutionDivisor` (`int`, **kod varsayılanı 1**). JSON: `"nebula": { "resolutionDivisor": N }`, `std::clamp(N, 1, 8)`. Anahtar yoksa 1 (eski dosyalar bozulmaz).
- Gönderilen `assets/pipeline.json` değeri P3'te ölçüme göre seçilir (beklenti 3 veya 4). P1–P2 sırasında `1` kalır.
- Çözünürlük `ceil(çıkış / N)` ve her eksende en az 1 piksel.

### 2.2 Pipeline (`VisualPipeline`)
- `Layer` enum'una `Nebula` eklenir. `Target(Layer::Nebula)`:
  - N > 1 ve ara doku hazırsa → ara doku;
  - aksi halde → `mBackground` (çağıran tarafı fark etmez; N = 1 bugünkü yoldur).
- Ara doku `mNebula` (`sf::RenderTexture`): `setSmooth(true)`; **view'ı tam çıkış boyutu uzayında** (`View(çıkış/2, çıkış)`), böylece `DrawNebula` bugünkü piksel koordinatlarıyla çizer, shader'a dokunulmaz ve gürültü alanı aynı kalır. `BeginFrame` içinde `Transparent` ile temizlenir ve view yeniden kurulur (yalnız N > 1 iken).
- `void VisualPipeline::CompositeNebula()`: N > 1 ise `mNebula.display()`, dokuyu `mBackground`'a tam çıkış boyutunda bir dörtgenle **premultiplied** karışımla çizer: `BlendMode(One, OneMinusSrcAlpha, Add, One, OneMinusSrcAlpha, Add)`. N = 1 ise hiçbir şey yapmaz.
  - Neden premultiplied: iki nebula şeffaf dokuya `BlendAlpha` ile çizilince renk `rgb·a` (premultiplied), alfa birikimlidir. `One, OneMinusSrcAlpha` ile birleştirmek, bugünkü ardışık alfa karışımıyla matematiksel olarak aynıdır. Düz `BlendAlpha` ile birleştirmek kenarları koyulaştırır.
- `EnsureTargets`: `outputChanged` **veya** N değişince `mNebula` yeniden kurulur (`virtualHeight` değişimindeki desenle). Kurulamazsa stderr'e bir kez uyarı yazılır ve N = 1 gibi davranılır; **pipeline kapatılmaz**.
- `Compose` sırası değişmez (arka plan, world, sprites, energy, bloom, flash, vignette).

### 2.3 `DrawNebula` (`Backdrop.cpp`)
`sf::Vector2f(target.getSize())` → `target.getView().getSize()`. `mBackground` için varsayılan view boyutu hedef boyutuna eşit olduğundan N = 1 çıktısı değişmez.

### 2.4 Çağıranlar
- `ProceduralBackdrop::DrawBackground`: `DrawGradient(background…)` → iki `DrawNebula` **`pipeline.Target(Layer::Nebula)`** üstüne → `pipeline.CompositeNebula()` → `mStars.Draw(background…)`. Sıra (gradyan, nebula, yıldız) bugünküyle aynı kalır.
- `LightYearsVisual/gallery/GalleryScene.cpp`: aynı değişiklik. Galeride mevcut canlı ayar mekanizması (`V` tuşu `virtualHeight`'ı nasıl değiştiriyorsa) ile bir tuş (öneri `N`) bölücüyü 1→2→4→8 döngüsünde değiştirir; mekanizma yoksa bu tuş eklenmez, ayar `pipeline.json` üstünden denenir.

---

## 3. İzinli dosyalar ve dokunulmayacaklar

**İzinli:** `LightYearsVisual/include/visual/PipelineSettings.h`, `LightYearsVisual/src/PipelineSettings.cpp`, `LightYearsVisual/include/visual/VisualPipeline.h`, `LightYearsVisual/src/VisualPipeline.cpp`, `LightYearsVisual/src/Backdrop.cpp`, `LightYearsVisual/include/visual/Backdrop.h` (yalnız yorum), `LightYearsGame/src/presentation/visual/ProceduralBackdrop.cpp`, `LightYearsVisual/gallery/GalleryScene.cpp` (+ `.h` gerekirse), `LightYearsVisual/gallery/main.cpp` (mevcut canlı tuş mekanizması burada), `LightYearsVisual/assets/pipeline.json`, `LightYearsVisual/README.md`, `docs/VISUAL_LIBRARY_PLAN.md` (kısa not), bu belge.

**Dokunulmaz (dirty / başka iş):** `docs/MODULE_COMMON_IMPLEMENTATION_PLAN.md`, `docs/GDD.md`. `LightYearsVisual/assets/shaders/planet.frag` bu işin ön koşuludur (§5 P0); Luna değiştirmez. `nebula.frag` değiştirilmez.

---

## 4. Proje kuralları (Luna görevlerine yazılır)
Kompakt C++17; mevcut `LightYearsVisual` stili (tab girinti, `vega::visual`, `m` önekli üyeler); RAII; yeni bağımlılık yok; SFML 3 API'si; GLSL `#version 120` kısıtı (yerleşik adlarla çakışma yok); gereksiz soyutlama ve gelecek için ayar eklenmez; yorumlar neden'i anlatır.

---

## 5. Fazlar (Sol yönetir, Luna uygular)

**Çalışma kuralları:** aynı anda tek yazıcı ajan; her faz sonunda Sol bağımsız derler ve diff'i okurken ajan raporuna güvenmez; commit'ler yalnız ilgili fazın dosyalarını stage eder ve **kullanıcı onayıyla** atılır (commit mesajına Claude atıf satırı eklenmez). Windows derleme komutu (VS'nin CMake'i gerekir; PATH'teki 3.31.5 projenin istediği 3.31.6'dan eskidir):

```
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul && set "VSLANG=1033" && "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build C:\LightYears\out\build\x64-debug --target LightYearsGame LightYearsVisualGallery --parallel 4'
```
Galeri için `--target LightYearsVisualGallery`. Release ölçümü `out\build\x64-release` (LTCG bağlama ~3 dk).

### P0 — Ön kontrol ve taban çizgisi (Sol)
- `planet.frag` düzeltmesi (`noise3` → `valueNoise3`) çalışma ağacında; kullanıcı onayıyla ayrı commit olarak kapatılır ya da bu işe dahil edilmeden önce commit'lenir.
- **Taban çizgisi:** değişiklikten önce galeri capture'ı iki kez alınır (`LightYearsVisualGallery --capture .tmp/visual/nebula_base_a.png --frames 120 --size 1280x720` ve `…_b.png`); iki dosyanın bayt-eş olup olmadığı kaydedilir (belirlenimcilik kontrolü). Eş değilse kıyas ölçütü yalnız arka plan bölgesi ortalama farkıdır. Aynısı `1920x1080` için de alınır.
- Süre taban çizgisi: §0'daki 180 kare yöntemi (Release, pipeline açık/kapalı) yeniden koşturulup kaydedilir.
- **Çıktı:** taban PNG'leri, süreler, belirlenimcilik sonucu. Araç notu: numpy/PIL yok; piksel farkı için PowerShell + `System.Drawing` (veya küçük bir C++/galeri yardımcısı) kullanılır.

### P1 — Ayar ve pipeline (Luna A)
- Dosyalar: `PipelineSettings.h/.cpp`, `VisualPipeline.h/.cpp`, `Backdrop.cpp`, `Backdrop.h`.
- Yap: §2.1–2.3. `Layer::Nebula`, `mNebula`, `CompositeNebula()`, `EnsureTargets` genişletmesi, `BeginFrame` temizliği, ayrıştırıcıda `nebula.resolutionDivisor`.
- Kabul: `LightYearsVisual` ve `LightYearsGame` derlenir (çağıranlar henüz eski API'yi kullanır, derleme bozulmaz); N = 1 ile oyun kodu yolu değişmemiştir.

### P2 — Çağıranlar, galeri, belge (Luna A, P1'den sonra)
- Dosyalar: `ProceduralBackdrop.cpp`, `GalleryScene.cpp/.h`, `pipeline.json` (anahtar eklenir, değer `1`), `README.md`, `VISUAL_LIBRARY_PLAN.md` notu.
- Yap: §2.4. README'de katman tablosuna "Nebula (arka planın alt katmanı, `resolutionDivisor` ile düşük çözünürlük)" ve ayar listesine `nebula.resolutionDivisor` eklenir.
- Kabul: oyun ve galeri derlenir; `pipeline.json`'da N = 1 iken görüntü P0 taban çizgisiyle **bayt-eş** (veya P0'da belirlenmeyen ise farksız).

### P3 — Doğrulama, ayar seçimi, kapanış (Sol, bağımsız inceleme Codex)
- Kabul ölçütleri §6. Sonuçlara göre `pipeline.json` varsayılan değeri seçilir, kanıt bu belgeye yazılır.
- Faz sonu inceleme listesi: `Target(Layer::Nebula)` N = 1'de `mBackground` döndürüyor mu; `BeginFrame` temizliği yalnız N > 1'de; `mNebula` view'ı her karede/yeniden boyutta doğru mu; premultiplied karışım; `EnsureTargets`'ta bölücü değişimi ve pencere boyutu 0; kurulamazsa geri düşüş; yalnız izinli dosyaların değişmesi; `nebula.frag` değişmedi.

---

## 6. Kabul ölçütleri

| # | Ölçüt | Yöntem |
|---|---|---|
| A1 | Debug derleme exit 0 (`LightYearsGame`, `LightYearsVisualGallery`) | §5 komutu |
| A2 | N = 1 çıktısı taban çizgisiyle bayt-eş | Galeri capture, 1280×720 ve 1920×1080 |
| A3 | N = 2, 4, 8: kenar halkası/koyulaşma, blok/bant artefaktı yok; ortalama mutlak kanal farkı raporlanır (öneri sınır: N = 4 için ≤ ~3/255) | Yan yana kare + fark; son karar gözle, kullanıcıya gösterilir |
| A4 | Hareket: iki farklı kamera konumunda capture; titreme/yüzme yok | Oyunda `--visual-capture` + el ile kısa oynanış |
| A5 | Sıcak yükleme: oyun açıkken `pipeline.json`'da bölücü değiştir + F9; yeniden başlatma gerekmez; 1'e dönüş çalışır | El ile + log |
| A6 | Pencere yeniden boyutlandırma ve çok küçük/simge durumu çökmez, `ERROR` yok | El ile + log |
| A7 | Performans: Release, 180 kare, pipeline açık; hedef N = 4 için ≤ ~4,8 sn (kapalı 4,22; eski 7,29). Hedef tutmazsa **raporlanır, hedef değiştirilmez** | §0 yöntemi, 2 koşu en küçüğü |
| A8 | `Visual pipeline ON`, shader derleme hatası yok | Oyun logu |
| A9 | `git diff` yalnız §3 izinli dosyaları | `git status` |

---

## 7. Riskler ve açık konular
- **Premultiplied karışım hatası** en olası görsel kusur (kenar koyulaşması). A2/A3 yakalar; çözüm karışım faktörleridir, shader değil.
- **Hareket:** düşük çözünürlük örneklemesi ekranda sabit, desen altından kayar; düşük frekans nedeniyle sorun beklenmez, A4 doğrular. N = 8 görünür yumuşama verebilir; üst sınır gerekirse 6'ya çekilir.
- **Ölçüm sınırı:** süreler 60 fps sınırına takılıdır; bütçe altındaki işler görünmez. Gerçek kazanç kare sınırı kaldırılarak da ölçülebilir (A7 raporu bunu not eder).
- **Bu planın dışında kalan, ölçülmemiş maliyetler:** Release'te CPU tarafı (profiler/log derleme dışı), yoğun savaştaki aktör/mermi/parçacık yükü, yeni içerik ilk kullanımda diskten yükleme (takılma), bloom (~4 ms). Nebula düzeltmesinden sonra gerçek oynanışta ölçülür; gerekirse ayrı plan.
- **Sonraki iş (ayrı karar):** kalite ön ayarları, bloom seviyesi ayarı, nebula güncelleme hızı/önbellek, adaptif kalite, shader hatasında yalnız ilgili özelliği kapatma (bugün tek shader hatası tüm pipeline'ı sessizce kapatıyor), hangi GPU'nun kullanıldığının doğrulanması.

---

## 8. Sol/Luna devir protokolü

**Sol (orkestratör):** fazı sırayla başlatır, tek yazıcıyı garanti eder, her ajan çıktısını bağımsız build + diff + capture ile doğrular, commit'i kullanıcı onayıyla kendisi atar, doğrulanmamış hiçbir şeyi kanıt saymaz.

**Her Luna görevinin metni şunları içerir:**
1. Hedef (§2'nin ilgili alt bölümleri) ve faz kabulü.
2. **İzinli dosya listesi** (§3); listede olmayan dosya değiştirilmez.
3. Dokunulmayacak dirty dosyalar (§3).
4. Proje kuralları (§4).
5. Yürütülecek komutlar (§5 derleme) ve kabul ölçütü.
6. Rapor biçimi: değişen dosyaların tam listesi, build sonucu, çalıştırılan capture'lar ve sonuçları, sapmalar, açık sorular.

## 9. Uygulama kanıtları — 2026-10-05

Kullanıcı uygulamayı başlattı; orkestratör Codex, uygulayıcılar Luna max. Üretim kodunda aynı anda tek yazıcı. Commit onayı ayrıca alınacak; planet.frag mevcut düzeltilmiş çalışma ağacı hâliyle ön koşul olarak korundu, commit atılmadı.

### P0
- Debug LightYearsGame ve LightYearsVisualGallery: §5 komutu iki hedefle, VSLANG=1033, exit 0. Sandbox içindeki ilk Ninja glob aşaması ilerlemedi; yalnız o derlemenin cmake/ninja süreçleri durduruldu ve yetkili araç erişimiyle tekrar derlendi.
- Galeri 120 kare: 1280x720 ve 1920x1080 çiftleri SHA256 bayt-eş. Çıktılar: .tmp/visual/nebula/base_720_a.png, base_720_b.png, base_1080_a.png, base_1080_b.png; hashler baseline-hashes.json.
- Release 180 kare + açılış: açık 9,20 / 7,48 sn; kapalı 4,26 / 4,35 sn. İki koşudan en küçük süre esas alındı; aynı açık shaderlar ve --visual-assets ile kaynak assets kullanıldı. Ham süreler .tmp/visual/nebula/baseline-times.json. Kare sınırı ve açılış dahil, kesin GPU süresi değil.
- P2 sınır düzeltmesi: V tuşu GalleryScene içinde değil gallery/main.cpp içinde; N tuşu aynı mekanizmaya eklenecek. Üretim dışı doğrulama araçları/çıktıları yalnız gitignored .tmp/visual/nebula altında.

### P1
- Luna max altı izinli dosyada ayar/target/view/composite uyguladı. Orkestratör diff'i bağımsız okudu; gereksiz üç generation sayacı yerine availability/frame-readiness booleans kullanıldı.
- Luna build exit 0; Codex aynı §5 komutuyla LightYearsGame + LightYearsVisualGallery bağımsız build exit 0. VSLANG=1033; mevcut başka oyun uyarıları vardı, nebula kaynaklarında hata yok.
- Gerçek SFML static kütüphanesine bağlı geçici runtime E2E: .tmp/visual/nebula/runtime_e2e.cpp, build_e2e.cmd, runtime_e2e.log. 91 kontrol, failed=0, exit 0. Default/clamp/invalid-type reload-keeps-previous; N1->4->1; 1279x719 ceil320x180; view reset; 3x5 ceil1x2; sıfır eksen guard ve restore; iki katman premultiplied fark toleransı <=2; boş karede eski nebula kalmaması; iki kamera noktasında shader capture ve N1 roundtrip max fark0.
- Bu test F9 tuşu ya da gerçek oynanış hareketinin kanıtı değildir; P3 canlı kontrolü ayrıca yapılacak. Opsiyonel doku resize başarısızlığı güvenli deterministik biçimde tetiklenemedi; fallback yalnız kaynak incelemesiyle doğrulandı.
- P1 sırasında nebula kapsamı dışındaki dokuz modül/game dosyası ayrıca dirty hâle geldi; nedeni doğrulanmadı, başka çalışma olarak korundu. A9 yalnız bu işin kendi değişiklik listesi üzerinden değerlendirilir; tüm çalışma ağacının temiz/yalnız nebula olduğu iddia edilmez.

### P2
- Luna max altı izinli dosyada game/gallery çağıranlarını, N tuşu ve başlık bilgisini, ayarı (değer1) ve belgeleri bağladı. Bağımsız Luna salt-okunur incelemesinde ve Codex diff incelemesinde somut kusur bulunmadı.
- İlk build, geçici response file bulunamadığı için LNK1104 ile durdu; ilk Codex tekrarında static lib açılamadı. Kaynak hatası tanısı yok, eşzamanlı dış derleme ihtimali var, nedeni kesin doğrulanmadı. Süreçler sakinleşince tek Codex tekrarında iki hedef exit0; galeri ve oyun kaynakları derlenip bağlandı. Kod bu hata için değiştirilmedi.
- MSVC ortam komutu set "VSLANG=1033" biçimine düzeltildi (cmd set'te sondaki boşluğu önlemek için). Yukarıdaki §5 komutu günceldir.
### P3 — tamamlanan otomatik/görsel kanıtlar
- A2: 1280x720 ve 1920x1080 N=1 capture hashleri P0 ile birebir aynı. 720p SHA256 F8D2C1A98D133EED7B665B7041E592C7829116F7374BBDB583504606FE972CAC; 1080p SHA256 1FFA8A70EDFDA847B729FCFD68ACC511958BCA27D722444C5ACD48A822335859.
- A3: N=2/4/8 sabit 120 kare galeri capture'ları .tmp/visual/nebula/gallery_{720,1080}_n{1,2,4,8}.png. Tam RGB ortalama mutlak fark (0–255):

| Çözünürlük | N2 | N4 | N8 |
|---|---:|---:|---:|
| 1280x720 | 0,173620 | 0,171742 | 0,194934 |
| 1920x1080 | 0,192314 | 0,189170 | 0,210363 |

N4 en büyük tek kanal farkı3, ortalama imzalı fark +0,006/255 (sistematik koyulaşma yok). Sabit arka plan boşluğu MAE 720p0,111; 1080p0,209. .tmp/visual/nebula/gallery-metrics.json tüm ham ölçümler. İncelenen karelerde belirgin kenar koyulaşması/halka/bloklaşma görülmedi; son sanatsal tercih kullanıcıya ait.
- İki kontrollü kamera merkezi (0,0) ve (1800,-600), game palette ve game nebula parallax/seed/scale değerleriyle actual shader capture: .tmp/visual/nebula/camera_{0,1}_n{1,2,4,8}.png. N4 MAE 0,201729 ve 0,193196; kesin değerler camera-metrics.json. Bu, canlı hareket/jitter testi yerine geçmez.
- A8: Debug gerçek oyun --visual-assets .../assets_n4 --visual-capture .../game_n4.png --capture-frames 180 exit0. game-n4-debug.log: Visual pipeline ON; capture180 ON; shader derleme ERROR yok. Bu capture derleme sürerken alındı, performans kanıtı değildir.
- Shader koruması: planet.frag ve nebula.frag başlangıç hashleri değişmedi (preserved-hashes.json).
- Canlı Computer Use testi kullanıcı tarafından fiziksel Esc ile durduruldu; bu tur başka UI girdisi gönderilmedi. F9 tuşu, gerçek hareket ve minimize/restore gözlemi tamamlanmış sayılmaz. Dosya reload (aynı VisualAssets::Reload(true) yolu), boyut/API geçişleri E2E'de geçti. Oyun/galeri pencereleri Titlebar|Close ile açılır; bu iş pencere stiline Resize eklemez. A4/A5/A6'nın manuel kısmı açık.
### P3 — performans, gönderilen ayar ve final durum
Aynı güncel Release çalıştırılabilir dosyası, --visual-capture ile 180 kare, --visual-assets ile ayrı N1/N4 kopyaları, aynı 1920x1080 pencere, başka oyun/galeri yok. İki koşudan en küçüğü kullanıldı. Açılış ve 60fps sınırı dahil; GPU süresi veya yoğun savaş ölçümü değildir. Ham veri: .tmp/visual/nebula/final-times.json.

| Durum | Koşu1 | Koşu2 | En küçük |
|---|---:|---:|---:|
| N1 | 7,605673 sn | 6,829206 sn | 6,829206 sn |
| N4 | 4,371222 sn | 4,480866 sn | 4,371222 sn |
| Pipeline kapalı | 4,179045 sn | 4,168996 sn | 4,168996 sn |

A7 hedefi 4,8sn: geçti. Aynı yeni binary ile toplam süre yaklaşık %36 azaldı; kapalı yolun üstündeki fark N1'de2,660sn, N4'te0,202sn. Bunlar kesin GPU maliyeti değildir. P0 ile yeni N1 farkı nedeniyle kazanç yeni binary'nin N1/N4 çiftinden raporlandı.

Gönderilen pipeline.json bölücüsü4 seçildi: görsel fark düşük ve performans hedefi tutuyor. C++ varsayılanı1 ve eski dosyada anahtar yoksa1 korunur. Final Debug Game+Gallery ve Release Game build exit0; source-assets shipped_720.png ile ölçülmüş gallery_720_n4.png SHA256 eş (7693A1C5C7E7CE2F281F53F685B4F879052D029DC4623A13B60A458B9C2E03A8).

Final Debug komutu §5'te iki hedefle aynen verilmiştir. Release komutu:

```powershell
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul && set "VSLANG=1033" && "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build C:\LightYears\out\build\x64-release --target LightYearsGame --parallel 4'
```

| Kabul | Son durum |
|---|---|
| A1 | Debug Game/Gallery ve Release Game exit0 |
| A2 | İki çözünürlükte bayt-eş, geçti |
| A3 | N2/4/8 ölçüldü; incelenen karelerde belirgin kusur yok; son kullanıcı görsel tercihi açık |
| A4 | İki kontrollü kamera capture'ı geçti; canlı hareket/jitter kontrolü tamamlanmadı |
| A5 | Gerçek loader + force reload geçişleri geçti; F9 tuşu canlı kontrolü tamamlanmadı |
| A6 | Gerçek pipeline API odd/tiny/zero/restore geçti; UI resize/minimize/restore tamamlanmadı |
| A7 | 4,371222sn <=4,8sn, geçti |
| A8 | Gerçek Debug oyun ON/capture180; shader ERROR yok |
| A9 | Bu işin 13 dosyası §3 sınırında (main.cpp genişletmesi dahil); mevcut/sonradan gelen başka dirty işler korunuyor, tüm git diff yalnız nebula değil |

Üretim dışı E2E/capture/JSON/script çıktıları gitignored .tmp/visual/nebula altında. Opsiyonel nebula allocation-failure tetiklenmedi; stale gating/fallback kaynak incelemesiyle doğrulandı. Commit/staging yapılmadı; kullanıcı onayı bekleniyor. Kullanıcı fiziksel Esc ile Computer Use'ı durdurdu, bu tur UI girdisi tekrar gönderilmedi.