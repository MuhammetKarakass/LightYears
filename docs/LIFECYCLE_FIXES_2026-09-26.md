# Yaşam döngüsü düzeltme paketi — 2026-09-26

Kapsam: [son kaynak denetimindeki](IMPLEMENTATION_FIXES_REVIEW_2026-09-26.md) dört açık. Kullanıcı doğrudan uygulama istedi; worker kullanılmadı. İlgisiz kirli değişiklikler korunur; wave/encounter, basit AI ve Null Pulse bu paketin inceleme/değişiklik kapsamı değildir.

## Değişiklik kontratı ve bağlantı incelemesi

Amaç: callback bitmeden çalışan instance'ı silmemek; component teardown'ını tek güvenli sınırda tamamlamak; hata halinde kalan temizlik adımlarını atlamamak; mutable vector referansını callback boyunca taşımamak; spatial shape değişimini indeksle eşlemek.

Sahipler/dosya sınırı: SAS `AbilityRuntimeSystem`, `AbilitySystemRuntime`, `GameplayAbilityInstance`, `AbilitySystemComponent`, effect runtime; game `GameAbility`, `LightYearsAbilitySystemComponent`, `AbilityInvocationRuntime`, `CombatRuntime`; Nano Plague controller; engine `Actor::SetActorRotation`; mevcut `AuditFixesE2E`. Bu yeni bir framework, yeni ability veya loadout değişikliği değildir. JSON, balance, presentation profilleri ve slot mapping değiştirilmedi. Yeni compiled source eklenmedi; mevcut kaynaklar ortak hedeflerde derleniyor.

İncelenen bağlantılar: runtime Tick/input/level/cooldown/event dispatch → instance lifecycle → activation/ended/changed delegates → component Clear; game component → Echo, lifecycle/history, weapon overrides; component effect cleanup → combat presentation/damage kayıtları; Nano damage → gameplay event/onDamageResolved → infection apply/refresh; rotation → manual-grid hücreleri → static sweep.

## Tasarım kararları

### İşlem sınırı ve çalışan instance

- Component `RunOperation` iç içe girişleri sayar. `Clear` işlem içindeyse yalnız istek kaydeder; effect/tag/attribute ve game temizliği de dış işlem bitimine kadar bekler. Exception durumunda da bu sınır kapanır; ilk hata korunur.
- Instance lifecycle metotları ve game'e özgü callback yayımlayan girişler aynı sınıra bağlanır. Çıkışta instance silinebileceği için execution callable önce kopyalanır; çıkıştan sonra instance üyesi okunmaz. Echo'nun normal bildirimleri kapalı olsa da execution sınırı korunur.
- SAS runtime `VisitAbility` canlı handle/slot/id'yi callback süresince rezerve eder; Tick, input, level/cooldown ve event/trigger yolları bunu kullanır. Callback'in aynı instance'ı remove/replace etmesi engellenir; Clear güvenli dönüşe ertelenir. Nested Tick tekrar yürütülmez.
- Component katmanında bir instance çağrısı sürerken yeniden girişle grant/rebind/remove **reddedilir**, kuyruğa alınmaz. Bu konservatif politika çağrı süresine aittir, sonraki normal işlem serbesttir. Grant/rebind post-commit exception için rollback vaat edilmez; loadout mevcut canonical sync davranışını korur. Component'te ertelenen Clear sonrası grant/rebind dönüşü tekrar doğrulanır.

### Teardown sırası ve hata politikası

Ability cancel/collection → game ek state (Echo cancel, history, observers, overrides) → effect cleanup → owned tags → attributes → varsa owner completion hook. Her aşama denendikten sonra ilk exception yeniden fırlatılır. Cooldown tag cache bayrakları da sıfırlanır.

Instance end akışı EndContent/EndExecution/state transition/bildirim aşamalarını bir önceki callback hata verse de dener. Yeniden girişli End engellenir. Effect removal kendi snapshot'ı ile callback çalıştırır, reservation'ı RAII ile bırakır; Clear bir effect callback'i hata verse de kalan effect'leri kaldırır. Echo Clear de bütün invocation'ları iptal etmeyi dener.

Clear pending/active iken component API'sinden yeni effect/tag, ability grant/rebind/remove, Echo invocation, weapon override veya observer/activation-guard kaydı kabul edilmez. Ham mutable storage erişimleri bu API garantisini bypass edebilir; bütün keyfi dış mutasyonlar/thread güvenliği bu paketin iddiası değildir.

`CombatRuntime::Clear` ayrıca component'in gerçek temizliği tamamlanana kadar presentation/pending-event/damage-protection/runtime-modifier temizliğini bekler. Component'in owner-completion hook'u exception olsa da çağrılır. Callback component'in sahibi tarafından kurulur, bağımsız global kayıt veya unmanaged dış receiver listesi eklenmez. Yalnız component Clear isteği, ayrı bir Combat Clear isteği yoksa combat state'i ayrıca sıfırlamaz.

### Nano Plague

Enfeksiyon kaydı monotonik controller-local id ve refresh revision taşır. Tick girişte kimlik listesi alır; damage callback'i sonrası id ile tekrar bulur. Tick sayacı damage öncesi ilerler; callback aynı enfeksiyonu refresh ederse yeni revision/duration/sayaçlar eski tick tarafından ezilmez. Nested Tick reddedilir. Damage ayarları callback öncesi değer olarak kopyalanır.

Zamanlama kararı: callback/spread sırasında **yeni oluşan enfeksiyonun süre ve tick hesabı sonraki Tick'te başlar**; mevcut delta geriye dönük uygulanmaz. Spread'in tag/pulse uygulaması o çağrıda devam eder. Bu açık bir sınır davranışıdır; hasar/duration/spread balance değerleri değiştirilmedi.

### Spatial rotasyon

`SetActorRotation`, fizik transformundan sonra World manual spatial kaydını da yeniler. Başlangıçta döndürülmüş bir kutuyu kaydetmekle sınırlı değildir: kayıt sonrası yer değiştirmeden rotasyonla hücre aşma ve eski hücrelerden çıkma ele alınır.

## Doğrulama

Kaynak/diff incelemesine ek olarak mevcut runtime E2E genişletildi: shipped Return Protocol aktivasyonundan Clear ve mutation denemeleri, doğrudan instance aktivasyonu, Echo lifecycle callback Clear, ended ve effect-removal hataları birlikte, Combat owner cleanup, yeniden kullanım; gerçek Nano damage callback'inde 32 ek hedefle kapasite artışı/refresh/nested Tick; gövdesiz duvarda 0→90→0 rotasyon.

Unit test eklenmedi/çalıştırılmadı; kullanıcı tercihi E2E. `LightYearsGasLiteTests` yalnız derlendi.

### Çalıştırılan komutlar ve sonuç

PowerShell'den MSVC x64 ortamı:

```powershell
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul && set VSLANG=1033&& cmake --build build --target LightYearsGame LightYearsGasLiteTests LightYearsContinuousBeamWallE2ETests --parallel 8'
ctest --test-dir build -R 'E2E$' --output-on-failure --output-log build/e2e-artifacts/lifecycle-fixes-suite-final.log
ctest --test-dir build -R '^LightYearsAuditFixesE2E$' --repeat until-fail:3 --output-on-failure --output-log build/e2e-artifacts/lifecycle-fixes-repeat.log
```

- Build: üç hedef başarıyla derlendi/bağlandı, exit 0. Bir ara 451 adımlık build'in oyun link aşaması `GameAbilityActionExecutor.cpp.obj : LNK1163 invalid selection for COMDAT section 0x3145` verdi. Yalnız tam yolu doğrulanan `build/LightYearsGame/CMakeFiles/LightYearsGame.dir/src/gameplay/ability/GameAbilityActionExecutor.cpp.obj` silinip yeniden derlendi; ardından oyun link'i exit 0. Kaynak silinmedi, üretilmiş nesne yeniden oluşturuldu. Bu olayın daha derin MSVC kök nedeni teşhis edilmiş sayılmıyor.
- Son suite: **6/6**, exit 0; [log](../build/e2e-artifacts/lifecycle-fixes-suite-final.log).
- Son lifecycle senaryosu: **51 assertion**, hepsi true; üç ardışık tekrar **3/3**, exit 0; [tekrar logu](../build/e2e-artifacts/lifecycle-fixes-repeat.log).
- [Artifact](../build/e2e-artifacts/audit-fixes.json): gerçek callback sırasında Nano kapasitesi **1→42**, refresh sayacı korunuyor, sonraki Tick normal çalışıyor. İlk cleanup hatası ve activation+Clear hatası ayrı kaydediliyor. SHA-256: `4E8EE5CD2B9015A1A84DB3019B9AA4A398F6D8ADAA4569EEF3D371749AAB48AF`.
- Ninja audit obj header dependency kaydı: **429 deps, VALID**. Güncel kaynakların build'e katılması kontrol edildi.
- Scoped diff/whitespace kontrolü temiz. Commit yapılmadı; ilgisiz dirty değişiklikler korundu.

Bu kanıt dört bulgunun tanımlı tetikleme koşulları ve mevcut E2E regresyonları içindir. Sanitizer, çok iş parçacığı, bellek tükenmesi altında güçlü rollback veya tüm repo hatasızlığı iddiası değildir. Keyfi custom behavior gövdesi içindeki bütün yan etkileri callback exception'ı sonrası geri almak garanti edilmez; ortak cleanup aşamaları tamamlanır ve ilk hata yayılır.
