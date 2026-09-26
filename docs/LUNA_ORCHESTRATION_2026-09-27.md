# Luna uygulama orkestrasyonu — 2026-09-27

Durum: devam ediyor; final kabul değildir.

Kullanıcının bu oturumdaki talebi: şef ajan işi sınırları belirli, birbirinin dosyalarına müdahale etmeyen 4–9 Luna max alt ajana ayırır; sonuçları commit eder. Altı görev rolü için toplam **7** alt ajan açıldı: ilk P3 ajanı model override aktarımını kesinleştirmek için durduruldu ve geçmişi miras almayan açık `gpt-6-luna / max` ayarıyla yeniden başlatıldı. Durdurulan ajan yeniden çalıştırılmaz. Ortam aynı anda üç alt ajanı desteklediğinden işler dalgalar halinde yürür. Eski planın tek-worker ve otomatik commit yasağı bu açık talep için geçerli değildir. Teknik sözleşmeler [plandan](LUNA_IMPLEMENTATION_PLAN_2026-09-26.md) korunur.

## Başlangıç

- Branch: `codex/luna-callback-lifetime-checkpoint`; HEAD: `5bffe04`.
- `git status --short` boş. `9bad22a` ve `47fa439` kapsamları incelendi; ikinci commit'in geniş önceki işleri bu oturuma atfedilmez.
- `ctest --test-dir build -N -R 'E2E$'`: altı kayıtlı E2E.
- `ctest --test-dir build -R 'E2E$' --output-on-failure --output-log build/e2e-artifacts/luna-orchestra-baseline.log`: exit 0, 6/6. Bu önceden derlenmiş binary başlangıcıdır; son kaynakların derleme kanıtı değildir.
- Depoda `docs/AGENTS.md` bulunmadı; kök talimatlar ve mevcut mühendislik skill'i okundu.

## Sahiplik ve devretme sözleşmesi

| Ajan | Paket ve izinli yazma alanı | Bağımlılık |
|---|---|---|
| `p3_luna` | Acceptance P3 runtime dosyaları; `GameAbility.h` ve `AbilitySystemRuntime.h` dahil | P1/P2 mevcut checkpoint |
| `p4_purchase` | `Player.h/.cpp`; level commit için P3'ten devralınan SAS/game ability owner dosyaları | P3 yazma hakkını bıraktıktan sonra |
| `p5_combat` | `CombatRuntime.h/.cpp`; zorunlu payload değişikliği önce şefe bildirilir | P3 teslimi |
| `p6_timer` | `TimerManager.cpp`, gerekirse header; `TimerManagerSceneE2E.cpp` | Bağımsız |
| `e2e` | `AuditFixesE2E.cpp`; zorunlu CMake bağlantısı şef onayıyla | Runtime API teslimleri; timer fixture'ına yazmaz |
| `review` | Salt okunur üretim kodu ve kabul incelemesi | Önce P1/P2, sonra son entegrasyon |

Şef dokümanlar, hafıza, build/test kilidi ve Git index/commit sahibidir. Alt ajanlar commit, toplu stage, hafıza yazımı veya birbirlerinin dosyalarını değiştirmez. Aynı dosyanın yeni sahibi ancak önceki sahibin tesliminden sonra atanır. Gerekli düzeltmeler kanıtla ilgili sahibine döner.

## Değişiklik sözleşmesi

Amaç P3–P6'nın callback, kaynak ömrü ve commit sınırlarını tamamlayıp E01–E19'u güncel gerçek-runtime akışında doğrulamaktır. Canonical owner'lar plandaki gibidir. Balance, slot, JSON, normal cooldown/charge, hasar sırası ve presentation korunur. Null Pulse, wave/encounter ve basit AI kapsam dışıdır. Kullanıcının mevcut E2E-only tercihi gereği unit test çalıştırılmaz; GasLite hedefi yalnız derlenir.

Kanıt: her paket diff incelemesi, güncel oyun/GasLite/E2E build, gerçek E2E assertion ve artefaktları, suite ve üçer tekrar. Eski loglar yeni kabul sayılmaz. Tamamlanan paketler açık dosya listesiyle commit edilir; push istenmedi.

