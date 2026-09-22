---
type: walkthrough
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
source_files:
  - LightYearsEngine/src/framework/Application.cpp
  - LightYearsEngine/src/framework/TimerManager.cpp
  - LightYearsEngine/src/framework/World.cpp
  - LightYearsEngine/src/framework/camera/CameraManager.cpp
symbols:
  - ly::Application::TickInternal
  - ly::TimerManager::UpdateTimer
  - ly::Delegate::Broadcast
  - ly::CameraManager::Update
related:
  - "[[Engine Runtime Services]]"
  - "[[Game Loop]]"
  - "[[TimerManager]]"
  - "[[Delegate]]"
  - "[[CameraManager]]"
---

# Timer, Delegate and Camera Frame Flow

```mermaid
sequenceDiagram
    participant App as Application
    participant World as World
    participant Global as Global TimerManager
    participant Game as Game TimerManager
    participant Camera as CameraManager
    participant Delegate as Delegate

    App->>World: TickInternal(dt)
    World->>Camera: Update(dt, defaultView)
    App->>Global: UpdateTimer(dt)
    alt World not paused
        App->>Game: UpdateTimer(dt)
        App->>App: Physics step(dt)
    end
    Global->>Delegate: timer callback may Broadcast
    App->>World: Render() / GetWorldView()
    World->>Camera: GetView(defaultView)
```

## Timer execution

`TimerManager::UpdateTimer` önce mevcut kaydın expired olup olmadığını kontrol eder; expired ise erase eder, değilse `TickTimer(dt)` çağırır. `TickTimer`, süre dolunca callback'i çalıştırır; tek seferlik timer'ı expired işaretler, repeat timer'ın sayacını sıfırlar. Bu nedenle tek seferlik timer fiziksel olarak aynı pass içinde değil, sonraki update'in erase adımında listeden çıkar.

## Weak callback cleanup

Timer, `weak_ptr<Object>` ile callback taşır. Owner expired veya pending-destroy ise `IsExpired` true olur ve callback çağrılmaz. Delegate weak binding de benzer şekilde expired owner için `false` döner; fakat delegate listener'ı timer gibi frame tick'te değil, sonraki `Broadcast` sırasında listeden kaldırılır.

## Camera update ve render

Follow target yoksa veya pending destroy ise CameraManager mevcut state'i varsayılan view ile başlatır/korur ve target-follow hesaplamasını atlar. Hedef varsa desired center/zoom hesaplanır, center exponential smoothing ile, zoom ise critically damped scalar smoothing ile güncellenir. Render aşamasında `World` bu state'ten `sf::View` oluşturur, shake offset'i yalnız view'a eklenir.

## Pause ve world geçişi

Pause, game timer ve physics update'ini durdurur; global timer update devam eder. Pending World switch tespit edildiğinde game/global timerlar temizlenir, physics cleanup + world initialize edilir, sonra yeni World `BeginPlayInternal` çağrısını alır. Bu, timerın eski World callback'ini taşımasını normal geçiş yolunda engeller.

