---
type: walkthrough
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
uncommitted_areas:
  - gameplay-effect, ability, movement and presentation frame integrations
  - GasLiteCoreTests
source_files:
  - LightYearsEngine/src/framework/Application.cpp
  - LightYearsEngine/src/framework/World.cpp
  - LightYearsEngine/src/framework/Actor.cpp
  - LightYearsEngine/src/framework/TimerManager.cpp
  - LightYearsEngine/src/framework/PhysicsSystem.cpp
symbols:
  - ly::Application::Run
  - ly::Application::TickInternal
  - ly::World::TickInternal
  - ly::Actor::TickInternal
  - ly::TimerManager::UpdateTimer
  - ly::PhysicsSystem::Step
  - ly::Application::RenderInternal
related:
  - "[[Game Loop]]"
  - "[[Actor and World System]]"
  - "[[Actor Spawn and Destruction Flow]]"
  - "[[CombatRuntime]]"
  - "[[Gameplay Effect Duration and Removal Flow]]"
---

# Frame Update Flow

## 7 Eylül 2026 kaynak kontrolü

Actor tick dt artık dt * World::GetSimulationTimeScale(actor.GetSimulationTimeDomain()). HUD/camera ve Application game/global timer orijinal dt kullanır. Diyagramdaki actor dt bu ölçeklenmiş değerdir. [[Temporal Runtime]].

Bu ek kaynak incelemesidir; build/test çalıştırılmadı. Aşağıdaki eski örnekler tarihsel inceleme kapsamını taşır; bu güncelleme ile çelişen ayrıntılar güncel davranış kabul edilmemeli.


> GAS-Lite bağlantısı: `SpaceShip::Tick` hareketten sonra `CombatRuntime::Tick` çağırır; `CombatRuntime` aynı frame içinde `mAbilitySystemComponent::Tick` üzerinden typed effect runtime'ı ve ability runtime'ını çalıştırır. Duration decrement ve expiry cleanup için [[Gameplay Effect Duration and Removal Flow]] notuna bakın.

```mermaid
sequenceDiagram
    participant Run as Application::Run
    participant App as Application::TickInternal
    participant World as World::TickInternal
    participant Actor as Actor::TickInternal
    participant Timer as TimerManager
    participant Physics as PhysicsSystem
    participant Render as Application::RenderInternal

    Run->>Run: pollEvent / DispatchEvent
    Run->>App: TickInternal(dt)
    App->>App: Tick(dt)
    App->>World: TickInternal(dt)
    World->>World: promotePendingActors()
    World->>Actor: TickInternal(dt * actor domain scale)
    World->>World: stage/world/HUD/cleanup/camera
    App->>Timer: global UpdateTimer(dt)
    alt world not paused
        App->>Timer: game UpdateTimer(dt)
        App->>Physics: Step(dt)
        Physics->>Actor: overlap callbacks
    end
    App->>App: audio / periodic clean / world switch
    Run->>Render: clear → render → display
```

## Adım 1 — Delta time ve event dispatch

- Dosya: `LightYearsEngine/src/framework/Application.cpp`
- Sembol: `ly::Application::Run`
- Çağıran: `main`
- Çağrılan: `DispatchEvent`, `TickInternal`
- State değişikliği: Clock restart ile frame `dt`; input event’leri current world/HUD’a gider.
- Sonraki adım: Application update.

Dosya: `LightYearsEngine/src/framework/Application.cpp`  
Sınıf veya namespace: `ly::Application`  
Fonksiyon: `Run`  
Görevi: Frame başında delta time ve input event sırasını oluşturmak. Quit branch’leri bu parçada çıkarılmıştır.

```cpp
sf::Time deltaTime = mTickClock.restart();
float dt = deltaTime.asSeconds();

while (const std::optional event = mWindow.pollEvent())
{
	if (event->is<sf::Event::Closed>())
	{
		QuitApplication();
	}
	else 
	{
		DispatchEvent(event);
	}

	if (mQuitRequested)
	{
		break;
	}
}
```

## Adım 2 — Application ve World update

- Dosya: `LightYearsEngine/src/framework/Application.cpp`
- Sembol: `ly::Application::TickInternal`
- Çağıran: `Application::Run`
- Çağrılan: `Application::Tick`, `World::TickInternal`
- State değişikliği: Game-specific application hook ve current world state’i güncellenir.
- Sonraki adım: Timer ve physics.

Dosya: `LightYearsEngine/src/framework/Application.cpp`  
Sınıf veya namespace: `ly::Application`  
Fonksiyon: `TickInternal`  
Görevi: Application hook’unu World’den önce çalıştırmak.

```cpp
Tick(deltaTime);

if(mCurrentWorld)
{
	mCurrentWorld->TickInternal(deltaTime);
}
```

## Adım 3 — Pending spawn ve actor update

- Dosya: `LightYearsEngine/src/framework/World.cpp`
- Sembol: `ly::World::TickInternal`
- Çağıran: `Application::TickInternal`
- Çağrılan: `Actor::BeginPlayInternal`, `Actor::TickInternal`
- State değişikliği: Pending actor’lar active listeye; canlı actor state’i bir frame ilerler.
- Sonraki adım: Stage, World, HUD ve cleanup.

Dosya: `LightYearsEngine/src/framework/World.cpp`  
Sınıf veya namespace: `ly::World`  
Fonksiyon: `TickInternal`  
Görevi: Collection değişimini tick iterasyonundan önce tamamlamak.

```cpp
promotePendingActors();

for (auto iter = mActors.begin(); iter != mActors.end();)
{
	iter->get()->TickInternal(deltaTime);
	iter++;
}
```

## Adım 4 — Timer ve collision

- Dosya: `LightYearsEngine/src/framework/Application.cpp`
- Sembol: `ly::Application::TickInternal`
- Çağıran: `Application::Run`
- Çağrılan: `TimerManager::UpdateTimer`, `PhysicsSystem::Step`
- State değişikliği: Global timer her frame; game timer ve physics yalnız pause değilken ilerler.
- Sonraki adım: Audio ve bakım.

Dosya: `LightYearsEngine/src/framework/Application.cpp`  
Sınıf veya namespace: `ly::Application`  
Fonksiyon: `TickInternal`  
Görevi: Pause-aware timer/physics sırası.

```cpp
TimerManager::GetGlobalTimerManager().UpdateTimer(deltaTime);

bool isPaused = mCurrentWorld && mCurrentWorld->IsPaused();

if (!isPaused)
{
	TimerManager::GetGameTimerManager().UpdateTimer(deltaTime);
	PhysicsSystem::Get().Step(deltaTime);
}
```

## Adım 5 — Deferred timer ve physics temizliği

- Dosya: `LightYearsEngine/src/framework/TimerManager.cpp`, `LightYearsEngine/src/framework/PhysicsSystem.cpp`
- Sembol: `ly::TimerManager::UpdateTimer`, `ly::PhysicsSystem::Step`
- Çağıran: `Application::TickInternal`
- Çağrılan: timer callback’leri, `ProcessPendingRemoveListeners`, contact callbacks
- State değişikliği: Expired timer’lar silinir; pending Box2D body removal uygulanır.
- Sonraki adım: Application maintenance ve render.

Dosya: `LightYearsEngine/src/framework/TimerManager.cpp`  
Sınıf veya namespace: `ly::TimerManager`  
Fonksiyon: `UpdateTimer`  
Görevi: Expired timer’ları iterator-safe biçimde silmek, diğerlerini ilerletmek.

```cpp
for(auto iter=mTimers.begin(); iter!=mTimers.end();)
{
	if (iter->second.IsExpired())
	{
		iter = mTimers.erase(iter);
	}
	else
	{
		iter->second.TickTimer(deltaTime);
		iter++;
	}
}
```

Dosya: `LightYearsEngine/src/framework/PhysicsSystem.cpp`  
Sınıf veya namespace: `ly::PhysicsSystem`  
Fonksiyon: `Step`  
Görevi: Önce deferred body removal, sonra simulation, sonra overlap dispatch.

```cpp
ProcessPendingRemoveListeners();
b2World_Step(mPhysicsWorld, deltaTime, 4);

ProcessContactEvents();
```

## Adım 6 — Render ve frame sunumu

- Dosya: `LightYearsEngine/src/framework/Application.cpp`
- Sembol: `ly::Application::RenderInternal`
- Çağıran: `Application::Run`
- Çağrılan: `Application::Render`, `World::Render`
- State değişikliği: Back buffer temizlenir ve sunulur.
- Sonraki adım: Yeni frame delta time.

Dosya: `LightYearsEngine/src/framework/Application.cpp`  
Sınıf veya namespace: `ly::Application`  
Fonksiyon: `RenderInternal`  
Görevi: Update/collision sonrası render frame’i.

```cpp
mWindow.clear();  
Render();         
mWindow.display();
```

## Erken çıkış ve deferred noktalar

- Event veya tick sırasında `mQuitRequested` set edilirse render atlanabilir.
- World tick sırasında spawn edilen actor, promotion swap’ından sonra geldiği için sonraki world tick’e kalır.
- Actor destroy world tick içindeyse aynı tick sonundaki `CleanCycle` kaldırabilir; collision sırasında destroy ise removal sonraki world tick’e kalır.
- Physics body destroy, `mPendingRemoveListeners` üzerinden sonraki physics step başına ertelenir.

## Test ve doğrulama

### Mevcut testler

`GasLiteCoreTests.cpp`, `World::TickInternal` üzerinden spawn ve cleanup davranışını yoğun kullanır. Physics callback’leri bazı projectile testlerinde doğrudan callback çağrısıyla da simüle edilir; production `Application::TickInternal` sırası tam olarak sürülmez.

### Doğrudan test edilmeyen davranışlar

Event → actor tick → timer → physics → render tam sırası, pause branch’i, periodic asset/audio clean cycle ve pending world geçişinin frame içindeki yeri.

### Manuel doğrulama gereken noktalar

SFML input gecikmesi, real-time input kullanan actor’ların frame yeri, Box2D contact dispatch ve render presentation.

### Önerilen fakat henüz bulunmayan testler

- Çağrı loglayan fake servislerle tam frame order testi.
- Collision callback’inde destroy ve aynı frame render filtreleme testi.
- Clean-cycle clock eşiği ile asset/audio/world cleanup çağrı testi.

## Kod Okuma Sırası

1. `LightYearsEngine/src/framework/Application.cpp`
2. `LightYearsEngine/src/framework/World.cpp`
3. `LightYearsEngine/src/framework/Actor.cpp`
4. `LightYearsEngine/src/framework/TimerManager.cpp`
5. `LightYearsEngine/src/framework/PhysicsSystem.cpp`
