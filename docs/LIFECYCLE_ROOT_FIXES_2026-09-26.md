# Lifecycle kök sözleşme düzeltmesi — 2026-09-26

Kullanıcı, ardışık tekil yamalar yerine kök tasarım düzeltmesi istedi. Kapsam
[kabul incelemesindeki](LIFECYCLE_ACCEPTANCE_REVIEW_2026-09-26.md) üç başlık ve
doğrudan sahip/çağıran bağlantılarıdır. Null Pulse, wave/encounter ve basit AI
inceleme/değişiklik kapsamı değildir. Ortak hedefler derlenirken bu kaynakların
da derlenmesi onları inceleme kapsamına almak anlamına gelmez.

## Değişiklik kontratı

- Sahipler: SAS GameplayAbilityInstance/AbilitySystemComponent; game GameAbility,
  ReturnProtocol cleanup, AbilityLoadoutManager ve NanoPlagueControllerActor.
- Kanıt: kaynak/diff kontrolü, oyun ve GasLite build, mevcut AuditFixesE2E içinde
  gerçek World/SpaceShip/shipped ability fault injection, E2E suite ve tekrar.
- Invariant: normal slot/balance/JSON değişmez; cleanup bütün owner aşamalarını
  dener; ilk hata korunur; callback canlı kayıt/instance'ı erken silemez.
- Sözleşme değişiklikleri aşağıda açıktır; ilgisiz dirty iş korunur. Yeni compiled
  source/registry/JSON eklenmez. Unit test eklenmez/çalıştırılmaz; GasLite build-only.

## Tasarım

### Aktivasyonun kaynak sahipliği

Hazırlık sırasında ActivateContent'e girildiği kaydedilir. Ret, exception veya
Cancel/Clear, AbortActivationContent yolunu çalıştırır. GameAbility behavior'a
gerçekten girilip girilmediğini ayrıca bilir; activation guard ret verirse başlamayan
behavior'ı bitirmez. Başlayan behavior'ın kısmi kaynakları End(Interrupted) ile
bırakılır. Guard/behavior içinden tekrar activate/level/tick başlangıcı reddedilir;
Cancel başlangıç stack'i döndüğünde uygulanır.

Charge/cooldown/duration commit'inden sonra lifecycle Activated yayınlanır.
Ardından action execution başlar; pending Clear/Cancel varsa yeni execution
başlatılmaz. Committed aktivasyon hatası End(Interrupted) ve normal cooldown ile
kapanır; precommit hata charge tüketmez. Execution başlamamışsa OnEnd action
çalıştırılmaz. İlk hata cleanup hatasına rağmen yeniden fırlatılır.

Return Protocol End, receiver/state/visual sahipliğini önce ayırır; visual,
tag ve Ended bildirim aşamalarının hepsini hataya rağmen dener. Böylece ortak
abort yolunun çağrılması somut kaynak cleanup'ına da ulaşır. Başka custom
behavior'ın End gövdesi kendi kısmi kaynaklarını bırakma kontratına uymalıdır;
generic sistem keyfi bir behavior'ın iç yan etkilerini otomatik geri alamaz.

### Tek doğruluk kaynaklı loadout

Manager'daki ikinci binding tablosu kaldırıldı. GetLoadout, component'e bağlı
salt-okunur AbilityLoadoutView döndürüyor. Önceden alınan view bile sorgu anında
runtime'ı okur: direct grant/remove, nested event, exception ve deferred Clear
için ayrı cache senkronizasyon yolu yok. Clear pending ise view boş görünür;
Grant/Rebind de pending Clear varken mevcut ama silinecek handle'ı başarı olarak
döndürmez. Inventory içerik sahipliğini saklamaya devam eder.

Bu kasıtlı API dönüş tipi değişikliğidir (`AbilityLoadout` → `AbilityLoadoutView`).
Mevcut FindAbility/FindSlot çağıranları korunur; pointer runtime definition ömrüne
bağlıdır. Harici mutasyonlar arasında ID kopyalanmalıdır. Yeni registry/observer
aboneliği veya destructor sıralamasına bağımlı cache eklenmez.

### Nano veri yapısı

Vector ve id başına find_if yerine tek `std::map<infectionId, Infection>` deposu:
id arama/silme O(log N), normal snapshot Tick O(N log N). Monotonik id sırası
eski eklenme/tick sırasını korur; ayrı indeksin senkronizasyon riski yoktur.
Callback sonrası id/revision tekrar kontrol edilir; yeni kayıt sonraki Tick'te
yaşlanır. Pulse expiry toplu erase-remove ile O(P); ardışık vector kaydırma yok.
Apply başarısızlığında kayıt/abonelik geri alınır. Destroy bütün kayıtların
abonelik/tag cleanup'ını dener; destructor exception yaymaz. Spread hedef araması
ve hedefe göre refresh taraması bu pakette aynı kalır; bütün Nano operasyonları
O(log N) veya ölçülmüş FPS kazancı iddia edilmez.

## Doğrulama sonucu

İlk build yeni E2E World sorgusunun weak_ptr erişiminde hata verdi; lock() ile
düzeltildi. Sonraki tam build (450 adım) ve header'ın son yorumu nedeniyle gereken
18 adımlık artımlı build exit 0; son tekrar yalnız asset sync yaptı, exit 0.

```powershell
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul && set VSLANG=1033&& cmake --build build --target LightYearsGame LightYearsGasLiteTests LightYearsContinuousBeamWallE2ETests --parallel 8'
ctest --test-dir build -R '^LightYearsAuditFixesE2E$' --output-on-failure --output-log build/e2e-artifacts/root-fixes-first.log
ctest --test-dir build -R 'E2E$' --output-on-failure --output-log build/e2e-artifacts/root-fixes-suite.log
ctest --test-dir build -R '^LightYearsAuditFixesE2E$' --repeat until-fail:3 --output-on-failure --output-log build/e2e-artifacts/root-fixes-repeat.log
```

- Oyun, GasLite ve E2E hedefleri derlendi. GasLite çalıştırılmadı; unit eklenmedi.
- İlk lifecycle E2E 1/1; tüm mevcut E2E suite **6/6**; lifecycle tekrar **3/3**.
- [Artefakt](../build/e2e-artifacts/audit-fixes.json): **59 assertion true**;
  ayrıca precommit/committed × Clear var/yok dört case'de cleanup ve gözlenen
  runtime state doğru. Clear olmayan case'lerde cleanup ayrıca hata veriyor;
  aktivasyonun ilk hatası korunuyor, sonrasında tekrar activation mümkün.
- Echo precommit hatası ve başlangıç sırasında Cancel kaynakları bırakıyor.
  Nested grant/rebind→Clear başarı döndürmüyor, saklanan live view boş görünüyor;
  aynı view direct runtime grant'i de görüyor.
- Nano gerçek damage callback'inde kayıt sayısı **1→33**; kayıt adresi kararlı,
  refresh sayacı/new-infection timing korunuyor; toplu expiry deposu boşaltıyor.
- SHA-256: `9E96D5C259CD0258C1148D42FF9780AE19CA4A28685F827E4AFAAC6BCB76C0ED`.
- Ninja audit obj dependency kaydı **431, VALID**; son build güncel.
- Scoped diff whitespace kontrolü exit 0. Runtime diff ve eski isimler incelendi;
  eski SynchronizeLoadout/vector-index infection yolu kalmadı. Commit yok.

Kabul: tanımlı üç bulgunun kökleri bu pakette düzeltildi. Bu, tüm repo için yeni
tam tarama veya bütün özel behavior'ların her olası exception'ını güvenli kılma
iddiası değildir. Sanitizer/FPS ölçümü yapılmadı. Önceki raporun açıkları tarihsel
tespittir; yukarıdaki kaynak ve runtime kanıtı güncel kabul kapsamıdır.
