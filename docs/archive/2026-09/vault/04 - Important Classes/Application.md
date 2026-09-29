---
type: class
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
source_files:
  - LightYearsEngine/include/framework/Application.h
  - LightYearsEngine/src/framework/Application.cpp
  - LightYearsGame/src/gameFramework/GameApplication.cpp
symbols:
  - ly::Application
  - ly::Application::Run
  - ly::Application::TickInternal
  - ly::Application::LoadWorld
  - ly::Application::ShutdownApplication
related:
  - "[[Game Loop]]"
  - "[[Application Startup Flow]]"
  - "[[World]]"
---

# Application

`ly::Application`, SFML window’unu, ana loop’u, current/pending World sahipliğini ve process-level servis sırasını yöneten merkezi runtime sınıfıdır. Somut `ly::GameApplication`, başlangıç content’ini kaydeder ve ilk world’ü ister.

## Temel ilişkiler

- `main` Application’ı `unique_ptr` ile sahiplenir.
- Application, current ve pending World’leri `shared_ptr` ile sahiplenir.
- Timer, physics, audio, shader ve asset singleton lifecycle’ını koordine eder.
- Game-specific `Tick` ve `Render` sanal hook’larını sağlar.

## Kritik gerçek kod

Dosya: `LightYearsEngine/include/framework/Application.h`  
Sınıf veya namespace: `ly::Application`  
Fonksiyon: sınıf alanları  
Görevi: Window, quit state ve World sahipliğini göstermek.

```cpp
sf::RenderWindow mWindow;
float mTargetFrameRate;
bool mShouldQuit;
bool mQuitRequested;
sf::Clock mTickClock;

sf::Clock mCleanCycleClock;
float mCleanCycleTime;

shared_ptr<World> mCurrentWorld;
shared_ptr<World> mPendingWorld;
```

Dosya: `LightYearsEngine/src/framework/Application.cpp`  
Sınıf veya namespace: `ly::Application`  
Fonksiyon: `Run`  
Görevi: Update sonrası quit kontrolünü ve render geçişini göstermek; event döngüsü bu alıntının dışındadır.

```cpp
TickInternal(dt);
if (mQuitRequested)
{
	ShutdownApplication();
	break;
}

RenderInternal();
```

Dosya: `LightYearsEngine/include/framework/Application.h`  
Sınıf veya namespace: `ly::Application`  
Fonksiyon: `LoadWorld`  
Görevi: World değişimini hemen uygulamak yerine pending state’e almak.

```cpp
auto newWorld = std::make_shared<WorldType>(this);
mPendingWorld = newWorld;
return newWorld;
```

Dosya: `LightYearsEngine/src/framework/Application.cpp`  
Sınıf veya namespace: `ly::Application`  
Fonksiyon: `ShutdownApplication`  
Görevi: Runtime servislerini ve world’leri normal kapanışta bırakmak.

```cpp
AudioManager::ShutdownAudioManager();
TimerManager::ShutdownTimerManagers();
mPendingWorld.reset();
mCurrentWorld.reset();
PhysicsSystem::ShutdownPhysicsSystem();
ShaderManager::ShutdownShaderManager();
AssetManager::ShutdownAssetManager();
mWindow.close();
```

## Test ve doğrulama

### Mevcut testler

Production Application loop’unu doğrudan çalıştıran test bulunamadı. Test binary ayrı `main` kullanır ve World’ü doğrudan çağırır.

### Doğrudan test edilmeyen davranışlar

Event order, quit branches, world switch, cleanup sırası ve exception cleanup.

### Manuel doğrulama gereken noktalar

SFML window yaşam döngüsü, platform event’leri ve normal/exception kapanışı.

### Önerilen fakat henüz bulunmayan testler

- Headless/fake window ile Application frame-order testi.
- İki test World arasında geçiş ve timer/physics cleanup testi.
- Quit’in render öncesi ve event-loop içindeki davranışı.

## Kod Okuma Sırası

1. `LightYearsEngine/include/framework/Application.h`
2. `LightYearsEngine/src/framework/Application.cpp`
3. `LightYearsGame/include/gameFramework/GameApplication.h`
4. `LightYearsGame/src/gameFramework/GameApplication.cpp`
