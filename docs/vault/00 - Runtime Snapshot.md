---
type: snapshot
verified_on: 2026-09-07
verification: source-review-only
verified_worktree_state: dirty
---

# Runtime Snapshot — 7 Eylül 2026

Kaynak incelemesi, dirty çalışma ağacı; build/test/oyun oturumu doğrulanmadı.
Bu belge 7 Eylül 2026 tarihli tarihsel snapshot'tır; güncel runtime slot
eşlemesinin canonical sahibi
`LightYearsGame/src/gameplay/ability/loadout/DefaultAbilityLoadout.cpp`'dir.
Ayrıntı ve sınırlar: [[2026-09-07 Project Status Review]].

## 7 Eylül snapshot'ındaki başlangıç ve input

`LightYearsGame/src/gameFramework/GameApplication.cpp`, `GameApplication::GameApplication`: asset kökü → `GameContentBootstrap::Register` → `LoadWorld<ArenaTestLevel>`. Bootstrap başarısızsa dünya açılmadan quit istenir.

| Girdi | Kaynakta bağlı davranış |
|---|---|
| Space | PrimaryFire |
| Q | Frozen Throng |
| E | Relay Prism |
| F | Glacial Pressure |
| R | Ironclad Protocol |
| WASD / oklar | PlayerMovementComponent hareket girdisi |
| Shift | Afterburner isteği |
| Escape | GameLevel pause/overlay akışı |

Ability slot eşlemesi `src/gameplay/ability/loadout/DefaultAbilityLoadout.cpp`, tuşlar `src/gameplay/input/AbilityInputSchema.cpp` içindedir (iki yol da LightYearsGame altında). Default loadout test seçimi, nihai tasarım değildir. [[Ability Content Inventory]] kayıtlı içerik listesidir; kayıt, her ability'nin uçtan uca test edildiği anlamına gelmez.

## Mevcut yapı

- Engine: World/Actor, SFML render, Box2D, timer, asset/audio/camera.
- SAS: generic attribute, ability lifecycle/runtime ve effect koleksiyonları.
- LightYearsGameplayCore: oyun sözleşmeleri ve yeniden kullanılan gameplay servisleri için ek statik hedef; oyun ve engine/SAS sınırlarını ortadan kaldırmaz.
- Game: concrete behavior/actor, damage/ship, weapon, level ve presentation.
- Presentation kayıtları `RegisterGameAbilityPresentationContent()` ile behavior/actor/effect kayıtlarından ve bootstrap validasyonundan önce hazırlanır. Feature-local typed profiller geçerlidir; global universal visual model yoktur.

## Son değişikliklerin etkisi

[[Movement]] impulse, acceleration source, forced translation ve movement policy ayrımını; [[Temporal Runtime]] actor time domain'lerini anlatır. Health/shield geçici fazlalıkları source-owned ledger kullanır. Primary projectile Box2D body açmadan swept collision kullanır; World query artık fizik broadphase ile body-less spatial grid'i birleştirir. Render bucket'larına görünürlük elemesi eklenmiştir.

## Tamamlanmamış oyun bağlantıları

Arena restart ve LevelOne HUD tick sorunları; scrap award/purchase üretim çağrısı yok; disk save/load bulunmadı. Ship XP ve respawn restore kaynakta bağlıdır. Save, evolve seçimi ve nihai balance bu bağlantılardan çıkarılamaz. Öncelikler: [[Technical Debt and Roadmap]].
