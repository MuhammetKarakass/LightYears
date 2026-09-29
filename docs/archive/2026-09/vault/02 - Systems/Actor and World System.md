---
type: system
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
uncommitted_areas:
  - gameplay-effect presentation actor and area integration
  - ability actors, movement and GasLiteCoreTests
source_files:
  - LightYearsEngine/include/framework/World.h
  - LightYearsEngine/src/framework/World.cpp
  - LightYearsEngine/include/framework/Actor.h
  - LightYearsEngine/src/framework/Actor.cpp
  - LightYearsEngine/src/framework/Application.cpp
  - LightYearsEngine/src/framework/PhysicsSystem.cpp
  - LightYearsEngine/include/gameplay/GameStage.h
symbols:
  - ly::World
  - ly::Actor
  - ly::Object
  - ly::World::SpawnActor
  - ly::World::TickInternal
  - ly::World::Render
  - ly::PhysicsSystem
related:
  - "[[Game Loop]]"
  - "[[Object Lifecycle]]"
  - "[[Ownership and Lifetime]]"
  - "[[World]]"
  - "[[Actor]]"
  - "[[CombatRuntime]]"
  - "[[Gameplay Effect System]]"
---

# Actor and World System

## 7 Eylül 2026 kaynak kontrolü

World::TickInternal artık actor dt için SimulationTimeDomain ölçeğini kullanır; pending promotion pause sırasında da olur. Body-less actor spatial grid ve fizik broadphase birlikte sorgulanır. World::SpawnActor hâlâ make_shared yapar; actor pooling yok. Actor::Destroy callback öncesi pending işaretlememesi yeniden giriş riskidir. Kaynak: LightYearsEngine/src/framework/{World,Actor}.cpp. [[Temporal Runtime]] ve [[2026-09-07 Project Status Review]].

Bu ek kaynak incelemesidir; build/test çalıştırılmadı. Aşağıdaki eski örnekler tarihsel inceleme kapsamını taşır; bu güncelleme ile çelişen ayrıntılar güncel davranış kabul edilmemeli.


> GAS-Lite bağlantısı: Actor/world katmanı effect policy veya attribute formülü sahiplenmez. Combat actor, value member [[CombatRuntime]] üzerinden [[Attribute System]] ve [[Gameplay Effect System]]'a bağlanır; effect visual ve area actor'ları normal Actor/World lifecycle kurallarına uyar.

## Sistem özeti

`World`, runtime container ve actor yaşam döngüsü sahibidir. `Actor`, world içindeki tick/render/physics katılımcısının tabanıdır. Actor kendi koleksiyonunu, frame loop’unu, timer manager’ı veya scene geçişini yönetmez.

| Sınıf | Dosya | Sorumluluk | Sahibi | Yaşam süresi |
|---|---|---|---|---|
| `Application` | `LightYearsEngine/include/framework/Application.h` | Window, frame loop, current/pending world | `main` içindeki `unique_ptr` | Process uygulama ömrü |
| `World` | `LightYearsEngine/include/framework/World.h` | Actor/HUD/stage koleksiyonları, spawn, tick, render, cleanup | Application `shared_ptr`ları | Level/scene ömrü |
| `Actor` | `LightYearsEngine/include/framework/Actor.h` | Transform, sprite/light, physics body bağlantısı, lifecycle hook’ları | World `shared_ptr` koleksiyonları | Spawn promotion’dan destroy cleanup’a; dış güçlü referans uzatabilir |
| `Object` | `LightYearsEngine/include/framework/Object.h` | Unique ID, weak self-reference, pending-destroy state | Türetilmiş nesnenin sahibi | Türetilmiş nesne ömrü |
| `GameStage` | `LightYearsEngine/include/gameplay/GameStage.h` | World içi aşama akışı | World `shared_ptr` listesi | World veya stage tamamlanmasına kadar |
| `PhysicsSystem` | `LightYearsEngine/include/framework/PhysicsSystem.h` | Box2D world, body ve contact event’leri | Static singleton `unique_ptr` | Application runtime; world geçişinde iç Box2D world yenilenir |

## World’ün sorumlulukları

- Actor’ları pending ve active `shared_ptr` listelerinde sahiplenmek.
- Spawn’ları tick başlangıcında promote edip BeginPlay çağırmak.
- Pause durumuna göre actor tick’lerini seçmek.
- Stage, HUD, perf ve camera güncellemelerini sıraya koymak.
- Pending-destroy actor’ları koleksiyondan çıkarmak.
- Render layer sırasına göre actor’ları ve ardından HUD’u çizmek.
- Event’i overlay HUD’a, yoksa ana HUD’a yönlendirmek.

## Actor’ın sorumlulukları

- Bir kez `BeginPlay`, canlıyken `Tick`, render, transform ve light davranışı.
- İsteğe bağlı Box2D body kaydı ve collision callback yüzeyi.
- `Destroy` isteğinde physics kaydını bırakmak, event yayınlamak ve pending state’e geçmek.

## Actor’ın sorumlu olmadığı işler

Actor aktif koleksiyona kendini eklemez veya çıkarmaz; frame loop’unu, world switch’i, global/game timer güncellemesini, render layer traversal’ını ya da Box2D step sırasını yönetmez. Generic Actor’da generic `owner`, `parent` veya component listesi yoktur.

## Kritik gerçek kod

Dosya: `LightYearsEngine/include/framework/World.h`  
Sınıf veya namespace: `ly::World`  
Fonksiyon: sınıf alanları  
Görevi: Aktif/pending actor koleksiyonları ile stage/HUD sahipliğini göstermek.

```cpp
List<shared_ptr<Actor>> mActors;
List<shared_ptr<Actor>> mPendingActors;
List<shared_ptr<GameStage>> mGameStages;
List<shared_ptr<GameStage>>::iterator mCurrentStage;
shared_ptr<HUD> mHUD;
shared_ptr<HUD> mOverlayHUD;
```

Dosya: `LightYearsEngine/src/framework/World.cpp`  
Sınıf veya namespace: `ly::World`  
Fonksiyon: `TickInternal`  
Görevi: Normal frame’de spawn, actor, stage ve world tick sırasını yürütmek.

```cpp
if (!mIsPaused) {
	promotePendingActors();

	for (auto iter = mActors.begin(); iter != mActors.end();)
	{
		iter->get()->TickInternal(deltaTime);
		iter++;
	}

	if (mCurrentStage != mGameStages.end())
	{
		mCurrentStage->get()->TickStage(deltaTime);
	}

	if (mBeganPlay)
	{
		Tick(deltaTime);
	}
}
```

Dosya: `LightYearsEngine/src/framework/World.cpp`  
Sınıf veya namespace: `ly::World`  
Fonksiyon: `TickInternal`  
Görevi: HUD, cleanup, perf ve camera’nın actor/world update sonrasındaki yerini göstermek.

```cpp
if(mHUD)
{
	if(!mHUD->HasInit())
		mHUD->NativeInit(mOwningApp->GetRenderWindow());
	mHUD->Tick(deltaTime);
}
if(mOverlayHUD)
{
	if(!mOverlayHUD->HasInit())
		mOverlayHUD->NativeInit(mOwningApp->GetRenderWindow());
	mOverlayHUD->Tick(deltaTime);
}

// Ensure destroyed actors are removed promptly to avoid large accumulation
CleanCycle();

// Perf monitoring
ly::perf::TickAndReport(deltaTime);

// Camera updates last so render uses the latest actor/stage/world state without a frame-order snap.
UpdateCamera(deltaTime);
```

Dosya: `LightYearsEngine/src/framework/World.cpp`  
Sınıf veya namespace: `ly::World`  
Fonksiyon: `Render`  
Görevi: Actor’ları pending state ve render layer ile filtreleyip HUD’dan önce çizmek.

```cpp
for (std::uint8_t layerIndex = 0;
	layerIndex < static_cast<std::uint8_t>(RenderLayer::Count);
	++layerIndex)
{
	const RenderLayer layer = static_cast<RenderLayer>(layerIndex);
	for (const std::shared_ptr<Actor>& actor : mActors)
	{
		if (!actor->GetIsPendingDestroy() && actor->GetRenderLayer() == layer)
		{
			actor->Render(window);
		}
	}
}

window.setView(window.getDefaultView());
RenderHUD(window);
```

Dosya: `LightYearsEngine/src/framework/Actor.cpp`  
Sınıf veya namespace: `ly::Actor`  
Fonksiyon: `InitializePhysics`, `UnInitializePhysics`  
Görevi: Actor’ın Box2D body lifecycle bağlantısını göstermek.

```cpp
if (!mPhysicsBodyId)  
{
	const b2BodyId bodyId = PhysicsSystem::Get().AddListener(this);
	// AddListener returns the zero-initialized id when an actor has no
	// physical shape. Do not pass that sentinel back into Box2D: its
	// debug validation intentionally asserts on an invalid world id.
	if (bodyId.index1 != 0)
	{
		mPhysicsBodyId = bodyId;
	}
}
```

```cpp
if(mPhysicsBodyId)
{
	PhysicsSystem::Get().RemoveListener(*mPhysicsBodyId);
	mPhysicsBodyId.reset();  
}
```

Dosya: `LightYearsEngine/src/framework/Application.cpp`  
Sınıf veya namespace: `ly::Application`  
Fonksiyon: `TickInternal`  
Görevi: World değişiminde timer, physics ve BeginPlay sırasını göstermek.

```cpp
if(mPendingWorld && mPendingWorld!=mCurrentWorld)
{
	mCurrentWorld = nullptr;

	TimerManager::GetGameTimerManager().ClearAllTimers();
	TimerManager::GetGlobalTimerManager().ClearAllTimers();

	PhysicsSystem::Get().Cleanup();
	PhysicsSystem::Get().InitializeWorld({ 0.f,0.f });

	mCurrentWorld = mPendingWorld;
	mCurrentWorld->BeginPlayInternal();
}
```

## Collision, rendering ve timer bağlantısı

- Actor physics’i açtığında Box2D `userData` içine raw Actor pointer yazılır.
- `Application::TickInternal`, önce `World::TickInternal`, sonra game timer ve `PhysicsSystem::Step` çağırır. Contact callback’leri bu step içindedir.
- Pending-destroy kontrolü contact başlangıcında ve render traversal’ında yapılır.
- Timer’lar World üyesi değildir. Static `TimerManager` singleton’ları Application tarafından güncellenir ve world geçişinde game/global timer’lar topluca temizlenir.

## Level/scene ilişkisi

Somut level’lar `World` türevleridir. `World::BeginPlayInternal`, sanal `BeginPlay`, `InitGameStages` ve `BeginStages` sırasını kurar. Application `LoadWorld` ile pending world oluşturur; gerçek geçiş frame tick sonunda yapılır.

## Bu sistemi kullanan ana alanlar

Player/ship, environment, enemy, ability projectile ve presentation actor’ları `World::SpawnActor` ve `Actor` lifecycle’ını kullanır. Bu not, bu gameplay alanlarının iç davranışını kapsamaz.

## Test ve doğrulama

### Mevcut testler

`GasLiteCoreTests.cpp` içinde gerçek `World` nesneleriyle spawn/promotion, type query ve gameplay actor cleanup kontrolleri bulunur. Camera tests weak actor hedefini; projectile tests pending-destroy sonrası collection cleanup’ı kullanır.

### Doğrudan test edilmeyen davranışlar

World pause branch’inin tüm sırası, render layer traversal’ı, HUD event önceliği, stage/world tick sırası ve Application world switch temizliği izole edilmemiştir.

### Manuel doğrulama gereken noktalar

Gerçek SFML render view değişimleri, Box2D contact event sırası ve level değişiminde dış referansların durumu.

### Önerilen fakat henüz bulunmayan testler

- Instrumented actor/stage/world ile tick sırası testi.
- Pending actor’ların pause sırasında promote olup yalnız `GetTickWhenPaused` actor’ların tick aldığını doğrulayan test.
- Render layer ve pending-destroy filtreleme testi.

## Kod Okuma Sırası

1. `LightYearsEngine/include/framework/World.h` — container ve public API.
2. `LightYearsEngine/src/framework/World.cpp` — spawn promotion, tick, cleanup ve render.
3. `LightYearsEngine/include/framework/Actor.h` — actor yetenekleri ve sınırları.
4. `LightYearsEngine/src/framework/Actor.cpp` — lifecycle ve physics bağlantısı.
5. `LightYearsEngine/src/framework/Application.cpp` — world’ün frame ve scene geçişine bağlandığı yer.
6. `LightYearsEngine/src/framework/PhysicsSystem.cpp` — collision callback’leri.
