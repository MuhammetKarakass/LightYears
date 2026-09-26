# Luna callback/lifetime çalışması — bağlam devri

**Tarih:** 2026-09-26
**Durum:** Kullanıcının durdurduğu checkpoint; plan tamamlanmadı.
**Amaç:** Yeni bağlamda uygulamaya kaldığı yerden, kanıt sınırlarını koruyarak devam etmek.

Bu dosya kısa devir rehberidir. Kapsam ve normatif davranış için önce [uygulama planı](LUNA_IMPLEMENTATION_PLAN_2026-09-26.md), mevcut bulgu/kanıt dökümü için [acceptance belgesi](LUNA_IMPLEMENTATION_ACCEPTANCE_2026-09-26.md) esas alınmalıdır.

## Durdurulduğumuz yer

Aktif branch `codex/luna-callback-lifetime-checkpoint`:

1. `9bad22a Checkpoint Luna P1-P2; pause P3 work`
2. `47fa439 Commit remaining workspace changes`

Kullanıcının “hepsini al” talimatıyla ikinci commit'te kalan **165 Git-visible yol** birlikte commit edildi: 9.247 ekleme, 1.468 silme. Bu commit yalnız Luna paketinden oluşmuyor; önceki engine, game, SpaceAbilitySystem ve docs çalışmaları da var. Bu nedenle commit adı bir kabul/kalite hükmü değildir. Devirden önce çalışma ağacı temizdi. Bu devir belgesi eklendiğinde yeni bir doküman değişikliği oluşur.

## P0–P7 ilerleme tablosu

| Paket | Durum | Kanıt ve sınır |
|---|---|---|
| P0 — başlangıç/scope kapısı | Tamamlandı | Başlangıç revision'ı ve o sıradaki 172 dirty path kaydedildi; baseline E2E 6/6 geçti. Bu, güncel branch için kabul kanıtı değildir. |
| P1 — attribute mutation/ownership | Uygulandı | P1/P2 E2E fixture'ında E01–E02 dahil. |
| P2 — effect operation ömrü/cleanup | Uygulandı | E03–E07 dahil; `LightYearsAuditFixesE2E` son kayıtlı koşuda 1/1 geçti: `build/e2e-artifacts/luna-p1-p2-final.log`. |
| P3 — ability callback/action/weapon ömrü | **Kısmi, doğrulanmadı** | Yalnız `GameplayAbilityInstance.h` içinde başlamış callback-depth/deferred-end çalışması var. Executor, snapshot, weapon cleanup ve E08–E11 yok. Son P3 kaynak değişikliğinden sonra build veya E2E çalıştırılmadı. |
| P4 — Player purchase commit | Başlamadı | E12 bekliyor. |
| P5 — nested damage context | Başlamadı | E13–E15 bekliyor. |
| P6 — one-shot timer | Başlamadı | E16–E17 bekliyor. |
| P7 — son kabul/regresyon | Bekliyor | E18–E19, güncel build, E2E suite ve tekrar koşuları bekliyor. |

P1/P2 öncesindeki tam build de P3 düzenlemesinden önceydi; başarılı build veya önceki E2E sonucu son P3 kodunu doğrulamaz. P0'daki incremental build yalnız asset synchronization yaptı ve derleme kanıtı sayılmaz. P7 regresyon turu yapılmadı. Unit test ekleme/çalıştırma bu planın dışında; istenen kanıt gerçek-runtime E2E'dir.

## P3'teki gerçek kod durumu

Direct owner'ların graph araması ve kaynak incelemesi şu ayrımı gösteriyor:

- `SpaceAbilitySystem/include/abilities/GameplayAbilityInstance.h` içine callback derinliği, callback esnasında `Tick`/`TryActivate`/`SetLevel` korumaları, iptali callback sonuna erteleme ve `OwnerDestroyed` önceliği için bir başlangıç eklendi. `EndAbility` içindeki bazı lifecycle adımlarını ilk exception'ı saklayıp diğer adımları deneyecek biçimde yürütme de mevcut. Bunların tümü **denenmiş ama derlenmemiş/test edilmemiş** P3 kodudur.
- `AbilityExecutionLifecycle::Tick` hâlâ active action listesini koşulsuz dolaşıyor. İptal bilgisini her callback sonrasında sınayan continuation bağlantısı yapılmadığı için yalnız instance çevresindeki guard, action döngüsünü tek başına durdurmuyor.
- Action runtime'ları `mDefinition.actions` içindeki spec'lere pointer tutuyor. Scoped refresh sırasında tanımın yenilenmesiyle aynı aktivasyon için immutable spec snapshot henüz sağlanmamış. Begin/Tick/OnEnd'in tek bir aktivasyon snapshot'ı kullanması ve sonraki aktivasyonun güncel tanımı alması gerekiyor.
- `AbilityExecutionLifecycle::End`, weapon `EndFire` ve FireWeapon runtime resource temizliği P3 kontratına göre henüz tamamlanıp doğrulanmadı. Temizlik callback'lerinden biri throw ettiğinde diğer bağımsız borçların denendiğini ve sahiplik durumunun callback'ten önce kapatıldığını E08–E11 göstermeli.
- Oyunun `GameAbility`/`GameAbilityActionExecutor` katmanına iptal devam koşulu ve activation snapshot entegrasyonu eklenmemiştir. Yarım instance kodunu güvenli kabul edip üst katmanda doğrudan kullanma; önce current owner/caller sözleşmesini tekrar doğrula.

Tam P3 sözleşmesi (reason önceliği, yapılandırma ömrü, canlı sayısal değerler, temizlik ve E08–E11) planın **P3** bölümünde; doğrudan sahipler ve dosya sınırı acceptance belgesinin **P3 — Change contract** bölümündedir.

## Devam sırası

1. Branch ve `git status` kontrolü yap; iki checkpoint commit'ini, özellikle 165 yollu geniş ikinci commit'i kapsam açısından tanı. Commit kapsamını Luna'ya mal etme.
2. P3'e başlamadan `GameplayAbilityInstance`, `AbilityExecutionLifecycle`, `GameAbility`, `GameAbilityActionExecutor`, ability component, FireWeapon runtime ve PrimaryWeapon owner'larını/caller'larını yeniden oku. `GameplayAbilityInstance.h` kısmi düzenlemesini ayrıca denetle; eksik/yanlış sözleşme varsa küçük bir yama varsaymak yerine owner düzeyinde düzelt.
3. P3'ü tamamla: callback içi iptal sonraki action/behavior/duration işini kessin; nested aynı-instance çağrıları reddedilsin; aynı aktivasyon Begin/WhileActive/OnEnd boyunca immutable action spec snapshot kullansın; scoped sayısal/config değerler plandaki live davranışı korusun; tüm bağımsız action/weapon temizliği borçları denensin ve ilk hata korunsun.
4. Mevcut gerçek-runtime fixture'ına E08–E11 ekle ve P3 kaynak/diff incelemesini yap. Sonra sırayla P4/E12, P5/E13–E15, P6/E16–E17'e geç.
5. P7'de çapraz akış E18 ve regresyon E19'u, kaynak taramasını, build'i ve gerçek-runtime E2E suite/repeat koşularını çalıştır. Acceptance belgesini ancak güncel kanıt geldikten sonra güncelle.

Kapsam sınırı: balance, default slot mapping, normal cooldown/charge ve presentation davranışını sessizce değiştirme; Null Pulse/wave/encounter/basit AI plan kapsamı dışındadır. Yeni runtime/registry/CMake değişikliklerinde `AGENTS.md` gereksinimlerini uygula.

## Son kabulde çalıştırılacak komutlar

Plan şu PowerShell/MSVC biçimini kaydediyor; hedef/test listesi devam ederken güncel CMake ile tekrar doğrulanmalı:

```powershell
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul && set VSLANG=1033&& cmake --build build --target LightYearsGame LightYearsGasLiteTests LightYearsContinuousBeamWallE2ETests --parallel 8'
ctest --test-dir build -N -R 'E2E$'
ctest --test-dir build -R 'E2E$' --output-on-failure --output-log build/e2e-artifacts/luna-lifecycle-suite.log
ctest --test-dir build -R '^LightYearsAuditFixesE2E$' --repeat until-fail:3 --output-on-failure --output-log build/e2e-artifacts/luna-lifecycle-repeat.log
ctest --test-dir build -R '^LightYearsTimerManagerSceneE2E$' --repeat until-fail:3 --output-on-failure --output-log build/e2e-artifacts/luna-timer-repeat.log
```

Bu devir raporu için build/test çalıştırılmadı. Yeni bağlamda yukarıdaki komutları **P3–P6 değişikliklerinden sonra** çalıştır; eski logları yeni kaynak sonucu gibi sunma.
