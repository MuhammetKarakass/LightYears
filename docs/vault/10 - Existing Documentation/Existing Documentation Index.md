---
type: index
status: active
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
uncommitted_areas:
  - GAS-Lite gameplay-effect core and content registration
  - ability, damage, movement and presentation integrations
  - GasLiteCoreTests and LightYearsGame CMake target
---

# Existing Documentation Index

> 20 Eylül 2026 tutarlılık kontrolünde kök belgeler ve vault, [[00 - Runtime Snapshot]] ve
> [[2026-09-07 Project Status Review]] kapsamıyla değerlendirilmiştir. Bu indeks
> belge sahipliğini gösterir; kayıt sayısı veya eski test sonucu e2e kanıt değildir.

Bu dosyalar taşınmamış veya silinmemiştir; vault içinden yalnızca indekslenir.

| Belge | Amaç | İçerik özeti | Güncellik | Yeni yapıdaki karşılığı | Tutarsızlık |
|---|---|---|---|---|---|
| `README.md` | Kök proje tanıtımı | Repo başlangıç bilgisi, canonical source'a bağlı kontrol/loadout ve doküman girişleri | 20 Eylül 2026 consistency update | [[Project Overview]], [[00 - Runtime Snapshot]] | Ayrıntılı balance için JSON/vault kullanılır |
| `docs/PROJECT_DOCUMENTATION.md` | Teknik referans | Runtime sahipliği, identity, JSON sınırı, damage matematiği ve presentation §3.0.1 | 20 Eylül consistency update; build/test çalıştırılmadı | [[Project Overview]], [[Attribute System]], [[Gameplay Effect System]], [[Derived Attributes]] | Default binding source owner'a bırakılır |
| `docs/CURRENT_IMPLEMENTATION_CATALOG.md` | Tek güncel content envanteri | JSON 55 ability, 19 effect, 10 weapon, 4 ship, 1 attachment; ID ve numeric kapsam | 20 Eylül consistency update | [[Current Implementation]], [[System Index]], [[00 - Runtime Snapshot]] | Kayıt sayısı e2e oynanış kanıtı değildir; C++ fallback ve JSON shipped içerik ayrıdır |
| `docs/BALANCE_AND_ROADMAP_NOTEBOOK.md` | Tarihli karar/denge/roadmap | Deney kartları, tarihsel kararlar ve güncel öncelik sırası | Tarihsel satırlar korunur; 7 Eylül roadmap arena restart/HUD/lifecycle/economy doğruluğuna öncelik verir | [[Balance Atlas]], [[Balance Data and Runtime Resolution Flow]], [[Derived Attributes]], [[Modifier Operations]], [[Current Implementation]], [[Technical Debt and Roadmap]] | Boş deney satırları plan veya test sonucu değildir |
| `docs/Designs.md` | Uzun vadeli ürün tasarımı | Chapter, biome, threat, hangar/constellation/mastery, currencies ve milestone hedefleri | 7 Eylül implementation checkpoint eklendi; özgün plan korunur | [[Project Overview]] | Tasarım hedefi runtime kanıtı değildir |
| `docs/IDENTITY_AND_CONTRACT_RULES.md` | Identity sözleşmesi | String-backed IDs, typed ContentId, AttributeId, GameplayTags aggregator ve leaf sahipliği | 7 Eylül source review | [[Gameplay Tag System]], [[Ability System]] | Contract alanı gameplay tag veya numeric key değildir |
| `docs/vault/Abilites/yetenekler.md` | Kısa yetenek yönlendirmesi | Current Catalog'a bağlanan source metadata rehberi ve default binding | 20 Eylül consistency update | [[Ability Content Inventory]] | Eski root `docs/yetenekler.md` tekrar olduğu için silindi |
| `docs/vault/00 - Light Years Dashboard.md` | Güncel giriş | Yerel kaynak denetimi | Mevcut | [[00 - Light Years Dashboard]] | 7 Eylül raporuna bağlı; eski Hoş geldiniz notu envanterde yok |

Build klasörlerindeki üçüncü taraf Markdown dosyaları proje dokümanları değildir; bu indekse bilinçli olarak alınmamıştır.

## GAS-Lite karşılaştırma sonucu

- Attribute çözüm sırası ve equal-priority Override uyarısı `PROJECT_DOCUMENTATION.md §2.3` ile kodda aynıdır.
- Rating ve ship-derived formüller `AttributeMath.h` / `ShipRuntime.cpp` ile uyumludur.
- Effect yaşam modeli ve cleanup sırası dokümanla uyumludur, fakat güncel spec/content/validation mimarisi dirty worktree'dedir.
- Uygulama durumu için her durumda kod ve test sonucu esas alınmıştır.

## 7 Eylül 2026 kapsam düzeltmesi

Yukarıdaki GAS-Lite/formül karşılaştırmaları eski incelemenin sonucudur; bugün
tamamı yeniden doğrulanmadı. Güncel kaynak kapsamı [[2026-09-07 Project Status Review]],
content envanteri [Current Implementation Catalog](../../CURRENT_IMPLEMENTATION_CATALOG.md),
tüm notlar [[Vault Note Inventory]].
Vault Markdown dosyaları Git ignore kapsamındadır; notlar için son commit tarihi
belirlenemedi.
