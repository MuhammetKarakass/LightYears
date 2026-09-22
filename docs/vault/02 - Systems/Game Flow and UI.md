---
type: system
verified_on: 2026-09-07
verification: source-review-only
verified_worktree_state: dirty
---

# Game Flow and UI

`GameApplication` doğrudan `ArenaTestLevel` açar. `GameLevel::BeginPlay`: initialize → GameHUD → HUD controller'ları → OnGameStart. `GameLevel::Tick`, warning ve ability controller'larını tick eder. `AbilityUIController::Tick` active ship/slot state'ini view model'e taşır, ship değişince widget'ları yeniden bağlar. `World::DispatchEvent` overlay'i önceler; Escape pause GameLevel'da işlenir.

ArenaTestLevel test hedefleri ve ArenaLevel respawn/camera/boundary yolunu kullanır. `MainMenuLevel` ve `LevelOne` ayrı oyun yolu sunar. LevelOne stage listesi bekleme, düşman dalgaları, chaos, boss ve infinite stage içerir. Bu sahnelerin tam oyun oturumu denenmedi; bütün AI state'leri incelenmedi. Vanguard örneğinde hareket + sürekli PrimaryFire input, Dummy'de sabit hareket + ateş doğrulandı.

## Doğrulanmış eksikler

- `LightYearsGame/src/level/GameLevel.cpp`, `OnRestartLevel`: PlayerManager reset + unpause var, world reload yok. Arena override'ı yok. Eski actor/delegate/HUD ile yeni player durumu ayrışabilir.
- `LightYearsGame/src/level/LevelOne.cpp`, `Tick`: World::Tick'e doğrudan giderek GameLevel HUD controller güncellemesini atlar.
- `Player::AwardScrap` / `TryPurchaseAbilityLevel` üretim çağrısı yok. Satın alma paneli tamamlanmış sayılmaz.

Bu bulguların önceliği ve tamamlanma ölçütleri [[2026-09-07 Project Status Review]] içinde. UI layout, font fallback, farklı çözünürlük ve input focus kaybı test edilmedi.
