---
type: guide
verified_on: 2026-09-20
source: "CURRENT_IMPLEMENTATION_CATALOG.md"
---

# Yetenekler — güncel yönlendirme

Bu not eski uzun yetenek kataloğunun yerine geçer. Güncel JSON content
envanteri, kayıt sayıları, tüm ability ID'leri ve doğrulama sınırı
[Current Implementation Catalog](../../CURRENT_IMPLEMENTATION_CATALOG.md)
içindedir: 55 ability, 19 effect, 10 weapon, 4 ship (1 player + 3 enemy) ve
1 JSON attachment. C++ fallback attachment tanımları ayrıca `AttachmentConfig.h`
içinde 7 kayıttır.

Runtime default slot binding'inin canonical sahibi
`LightYearsGame/src/gameplay/ability/loadout/DefaultAbilityLoadout.cpp`'dir;
bu guide complete loadout tablosunu kopyalamaz. Katalog kaydı veya input gönderimi
tek başına uçtan uca oynanış, acquisition UI ya da evolve seçimi kanıtı değildir.

Teknik identity, `AbilityDefinition`/`GameplayEffectDefinition` ve actor
presentation kuralları [Identity and Contract Rules](../../IDENTITY_AND_CONTRACT_RULES.md)
ve [Project Documentation](../../PROJECT_DOCUMENTATION.md) içindedir. Tarihli
ability tuning, deney ve ertelenmiş içerik kararları
[Balance & Roadmap Notebook](../../BALANCE_AND_ROADMAP_NOTEBOOK.md)'ta tutulur.

`Abilites/control`, `defensive`, `functional`, `movement`, `offensive` ve
`utility` altındaki notlar aile bazlı tasarım/kaynak notlarıdır. Her notun
frontmatter `source` alanı bu guide'ı veya owner teknik belgeyi gösterir;
ayrıntılı eski stat tabloları güncel JSON değeri yerine geçmez.
