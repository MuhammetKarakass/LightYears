# Ability catalog ikinci paket: 10–18

## Değişiklik sözleşmesi

Amaç: `ABILITY_SCALING_CATALOG.md` içindeki RailBurst, AstralSurge,
CrescentReaver, AegisReaver, ScorchDrive, IonStorm, SolarBombardment,
ChainLightning ve StormMark davranışlarını ve dengelerini uygulamak.
19. yeteneğe geçilmeyecek.

Sahiplik: Üç Luna max ajanı sırasıyla 10–12, 13–15 ve 16–18 ailelerinin
doğrudan C++ sahiplerini düzenler. Orkestratör ortak `abilities.json`,
entegrasyon incelemesi, doğrulama, dokümantasyon ve commit sahibidir.
İzinli dosyalar bu ailelerin gameplay/contract/config/presentation dosyaları,
gerekli derleme bağlantıları, ortak ability verisinin yalnız bu dokuz kaydı
ve bu paket raporudur.

Aegis siphon incelemesinde `absorbedDamage` alanının shield yanında barrier
ve cryostasis tarafından da artırıldığı görüldü. Bu nedenle izinli ortak
sınır yalnız `DamageContext.h` ve `SpaceShip.cpp` içindeki gerçek shield
hasarı sonucudur: `shieldDamage`, uygulanan shield kaybını kendi biriminde
taşır. Diğer emilim türleri siphon üretmez; mevcut damage hesabı değişmez.

İnvariantlar: Runtime loadout, diğer yetenekler ve ortak damage/status
sözleşmeleri korunur. Belirtilmeyen değerler ve seviye sınırları korunur.
Energy hasarı mevcut canonical shield/hull yolunu, Thermal/Electric
birikimleri mevcut status yolunu kullanır. Presentation aileye ait typed
registry üzerinde kalır; yeni evrensel görsel yapı eklenmez.

Kanıt: Önce kaynak/call-path ve seçili JSON formül incelemesi; ardından tek
birleştirilmiş Game ve GasLite derlemesi. Kullanıcının test maliyeti
düzeltmesi gereği unit test koşulmaz. E2E yalnız somut yeni davranış riski
için mevcut uygun fixture ile dar seçilir; otomatik yeni harness açılmaz,
başarılı kontrol yeni değişiklik/bulgu olmadan tekrarlanmaz.

## Sonuç

Üç Luna max ajanının değişiklikleri orkestratör kaynak incelemesiyle
birleştirildi. Dokuz aile uygulandı; 19. yeteneğe geçilmedi. Son Scorch
attribute düzeltmesinden sonraki build/yükleme doğrulaması kullanıcıya bırakıldı.

| Yetenek | Davranış değişikliği |
| --- | --- |
| RailBurst | EP-only hasar; sıralı delme hasarı, %10 kayıp ve %60 taban. |
| AstralSurge | EP ölçeği; mevcut sınırsız dalgada delme temasları sıralı, %5 kayıp ve %40 taban. |
| CrescentReaver | Luck bonusu floor; sekme başına sınırsız lineer +%25; eski isabet cooldown indirimi kaldırıldı. |
| AegisReaver | Aktivasyonda tam shield fedası ve uçuş regen kilidi; yalnız gerçek shield hasarından siphon, yalnız doğrulanmış dönüşte ödül. |
| ScorchDrive | Özel burn modu ve HP lifetime uzatması kaldırıldı; her field hit canonical Ignite, configured lifetime 5 saniye. |
| IonStorm | AP kaldırıldı; EP hasarı ve projectile→field hattında her tick için 1 Electric stack. |
| SolarBombardment | Dış/İç çarpan .75/1.50, inner radius 130, mesafeye göre 1–3.4 saniye varış ve self-hit. |
| ChainLightning | Her .22 saniyede canlı hedef araması; önce yeni, sonra önceki hedef; Luck floor, tek timer, ölen hedefin son konumundan devam. |
| StormMark | Kesin floor(Luck/20) hedef bonusu; mevcut odaklanma ve sıralı tek-vuruş akışı korundu. |

Aegis siphon oranı katalogda ayrı dengelenecek denildiği için .10 taban ve
seviye başına +.01 korundu. Overshield hold/decay zamanlamasına yeni sayı
uydurulmadı; mevcut kaynak-tüketim davranışı korundu. Crescent'ın artık
kullanılmayan cooldown attribute sabiti eski C++ tüketicilerinin derlenmesi
için kaldı; shipped veride ve runtime etkisinde kullanılmıyor.

## Veri incelemesi

Seçili dokuz JSON kaydı dışında içerik değişmedi. Dokuz yeteneğin mevcut
14 yükseltmesi ve scrap maliyetleri korundu. Katalog katsayıları ve global
azalan cooldown adımları işlendi; L15 cooldown değerleri sırasıyla RailBurst
5.85, AstralSurge 8.45, CrescentReaver 4.75, AegisReaver 8.45, ScorchDrive
7.8, IonStorm 5.85, SolarBombardment 9.1, ChainLightning 6.5 ve StormMark
6.5 saniye. Hiçbiri başlangıç cooldown'ının %20 altına inmiyor.

Ortak `damage_status_balance.json` zaten Energy shield 1.50, Electric
%3/%6/%9/%16 (4 saniye) ve Ignite 1/2/3/5 DPS (en çok 4 stack)
tanımlıyor; bu ortak veri değiştirilmedi. Solar'ın 3.4 saniyelik azami
varışından önce kaybolmaması için actor lifetime 3'ten 4 saniyeye çıkarıldı.

## Doğrulama kapsamı

Yeni test dosyası/harness eklenmedi; unit test koşulmadı. Kaynak incelemesi
hasar ölçekleri, status payload aktarımı, Aegis dönüş/kayıp ve abonelik
temizliği, Chain canlı hedef seçimi/tek timer ve JSON kapsamını kapsadı.
Seçilen mevcut E2E `LightYearsAbilityLoaderPublicLoadE2E`; dokuz yeteneğin
yeni oynanışını uçtan uca ispatladığı iddia edilmez.

İlk derleme tanıları Chain'in TimerHandle API kullanımını ve Ion'un eksik
damage config include'unu gösterdi; yerel olarak düzeltildi. İlk E2E eski
Aegis action'ının hâlâ çalıştığını gösterdi. Kök neden: Ninja'nın
`msvc_deps_prefix = Note: including file:` ayarına karşı derleyicinin Türkçe
`Not: eklenen dosya:` üretmesi; header bağımlılıkları izlenmemişti.
`VSLANG=1033` ile bir temiz derleme gerekli oldu. Sonraki incremental
derlemelerde de aynı dil korunmalıdır; bu yerel build ortamı ayarıdır.

Son komut ve sonuçlar:

- `set VSLANG=1033` ardından `cmake --build D:\LightYears\build --target LightYearsGame LightYearsGasLiteTests LightYearsContinuousBeamWallE2ETests --parallel 6 --clean-first`: exit 0, 1104/1104 adım tamamlandı. Log: `build/balance-batch2-build-clean.log`.
- `ctest --test-dir build -R '^LightYearsAbilityLoaderPublicLoadE2E$' --output-on-failure`: son koşu 1/1 başarısız. Aegis eski action hatası temiz build ile kapandı; bu koşu ability-scoped `Damage.Ignite.Stacks` alanını reddetti.
- Scorch stack ayarı mevcut Chain/Storm desenindeki gibi aile-local `Ability.Offense.ScorchDrive.IgniteStacks` contract'ına taşındı. Canonical Ignite payload ve status sistemi korunuyor; ortak validator genişletilmedi.
- Kullanıcı rebuild'i kendisinin yapabileceğini sordu; tekrar build/test başlatılmayacağı bildirildi. Bu son düzeltme kaynak/veri düzeyinde uygulandı, yeniden derlenmedi veya E2E'de doğrulanmadı. Başarılı E2E kabulü henüz yok.
- `git diff --check`: exit 0. Yeni test/harness ve unit test koşusu yok.
