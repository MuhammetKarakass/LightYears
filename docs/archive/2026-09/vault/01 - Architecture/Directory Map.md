---
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
---

# Directory Map

## 7 Eylül 2026 kaynak kontrolü

Tam güncel dosya listesi [[Source File Index]]. Yeni alanlar gameplay/movement, gameplay/temporal, gameplay/resource, gameplay/weapon/runtime ve feature-local presentation aileleridir. Listeleme bütün kaynakların okunması değildir.

Bu ek kaynak incelemesidir; build/test çalıştırılmadı. Aşağıdaki eski örnekler tarihsel inceleme kapsamını taşır; bu güncelleme ile çelişen ayrıntılar güncel davranış kabul edilmemeli.


| Yol | Amaç | Önemli içerik | Durum |
|---|---|---|---|
| `LightYearsEngine/include` | Engine genel arayüzleri | framework, gameplay stage, widget, VFX | Implemented |
| `LightYearsEngine/src` | Engine uygulaması | Application, World, Actor, physics, asset/audio/timer | Implemented |
| `SpaceAbilitySystem/include` | SAS public arayüzleri | Attribute, Ability ve Effect contract/runtime/collection/registry/binding/lifecycle arayüzleri; `AbilitySystemComponent` ve `GameplayAbilityInstance` | Implemented; son ability sahiplik düzeltmesi için toplu doğrulama bekliyor |
| `SpaceAbilitySystem/src` | SAS uygulaması | AttributeSystem; Ability ve Effect validation/state/scheduler/binding/lifecycle implementasyonları | Implemented; güncel CMake'de 8 CTest kaydı var; bu docs-only denetiminde build/test çalıştırılmadı |
| `LightYearsGame/include` | Oyun arayüzleri ve veri tanımları | gameplay, gameConfigs, player, level, presentation; ability content ve concrete `bindings/` | Implemented; generic ability instance/event/registry dosyaları kaldırıldı |
| `LightYearsGame/src` | Oyun uygulaması | GameApplication, ability, weapon, combat, level, UI | Implemented |
| `LightYearsGame/assets` | Oyun varlıkları | `SpaceShooterRedux` | Implemented |
| `LightYearsGame/tests` | Oyun testleri | `GasLiteCoreTests.cpp` | Implemented |
| `docs` | Mevcut proje dokümanları | catalog, roadmap, design, project documentation | Implemented |
| `docs/vault` | Obsidian navigasyon ve envanter | Bu notlar, `.obsidian` | Implemented |
| `build`, `build-asan`, `build-gravity-*` | Üretilmiş build çıktıları ve indirilen bağımlılıklar | CMake/CTest, `_deps` | Generated; kaynak değildir |
| `.codebase-memory`, `.codex`, `.claude`, `.vs` | Araç/IDE çalışma verisi | indeks ve yerel ayarlar | Tooling; kaynak değildir |

Build klasörlerindeki SFML, Box2D ve ses kütüphanesi kaynakları proje kaynağı olarak sayılmamıştır.
