---
type: walkthrough
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
source_files:
  - VegaEngine/include/framework/World.h
  - VegaEngine/src/framework/World.cpp
  - VegaEngine/src/framework/Actor.cpp
  - VegaEngine/src/framework/Object.cpp
  - VegaEngine/src/framework/PhysicsSystem.cpp
  - LightYearsGame/tests/GasLiteCoreTests.cpp
symbols:
  - vega::World::SpawnActor
  - vega::World::TickInternal
  - vega::Actor::BeginPlayInternal
  - vega::Actor::Destroy
  - vega::Object::Destroy
  - vega::World::CleanCycle
related:
  - "[[Object Lifecycle]]"
  - "[[Actor and World System]]"
  - "[[Actor]]"
  - "[[World]]"
---

# Actor Spawn and Destruction Flow

```mermaid
sequenceDiagram
    participant Caller
    participant World
    participant Actor
    participant Physics
    participant Observer

    Caller->>World: SpawnActor<ActorType>(args)
    World->>Actor: make_shared(World*, args)
    World->>World: mPendingActors.push_back
    World-->>Caller: weak_ptr<ActorType>
    Caller->>World: TickInternal(dt)
    World->>World: swap pending list
    World->>Actor: BeginPlayInternal()
    World->>Actor: TickInternal(dt)
    Caller->>Actor: Destroy()
    Actor->>Physics: RemoveListener(bodyId)
    Actor->>Observer: onActorDestroyed.Broadcast(this)
    Actor->>Actor: Object::Destroy()
    Actor->>Observer: onDestory.Broadcast(this)
    World->>World: CleanCycle / erase shared_ptr
    World-->>Actor: destructor if last owner
```

## Adım 1 — Spawn talebi ve bellek tahsisi

- Dosya: `VegaEngine/include/framework/World.h`
- Sembol: `vega::World::SpawnActor`
- Çağıran: Herhangi bir runtime/game system
- Çağrılan: `ActorType(World*, args...)`
- State değişikliği: Heap allocation ve `mPendingActors` güçlü sahipliği.
- Sonraki adım: Caller yalnız weak pointer alır.

Dosya: `VegaEngine/include/framework/World.h`  
Sınıf veya namespace: `vega::World`  
Fonksiyon: `SpawnActor`  
Görevi: Actor tahsisi ve pending kuyruğu.

```cpp
shared_ptr<ActorType> newActor = std::make_shared<ActorType>(this, args...);
mPendingActors.push_back(newActor);
vega::perf::IncActiveActors();
return newActor;
```

## Adım 2 — Pending listeden aktif listeye promotion

- Dosya: `VegaEngine/src/framework/World.cpp`
- Sembol: `vega::World::TickInternal`
- Çağıran: `Application::TickInternal` veya test
- Çağrılan: `Actor::BeginPlayInternal`, `World::OnActorSpawned`
- State değişikliği: `mPendingActors` boşalır, `mActors` büyür, began-play set edilir.
- Sonraki adım: Actor aynı tick içinde tick alabilir.

Dosya: `VegaEngine/src/framework/World.cpp`  
Sınıf veya namespace: `vega::World`  
Fonksiyon: `TickInternal`  
Görevi: Promotion sırasında yeni pending eklemelerinden izole olmak.

```cpp
List<shared_ptr<Actor>> actorsToSpawn;
actorsToSpawn.swap(mPendingActors);

for (const shared_ptr<Actor>& actor : actorsToSpawn)
{
	mActors.push_back(actor);
	actor->BeginPlayInternal();
	OnActorSpawned(actor.get());
}
```

## Adım 3 — BeginPlay ve Tick guard’ları

- Dosya: `VegaEngine/src/framework/Actor.cpp`
- Sembol: `vega::Actor::BeginPlayInternal`, `vega::Actor::TickInternal`
- Çağıran: `World::TickInternal`
- Çağrılan: Sanal `BeginPlay`, sanal `Tick`
- State değişikliği: `mBeganPlay = true`; actor state’i frame ilerler.
- Sonraki adım: Stage/world/HUD update ve clean cycle.

Dosya: `VegaEngine/src/framework/Actor.cpp`  
Sınıf veya namespace: `vega::Actor`  
Fonksiyon: `BeginPlayInternal`, `TickInternal`  
Görevi: Lifecycle hook’larını state guard’larıyla çağırmak.

```cpp
if (!mBeganPlay)  
{
	mBeganPlay = true;    
	BeginPlay();      
}
```

```cpp
if (mBeganPlay && !GetIsPendingDestroy())
{
	Tick(deltaTime);
}
```

## Adım 4 — Destruction talebi

- Dosya: `VegaEngine/src/framework/Actor.cpp`
- Sembol: `vega::Actor::Destroy`
- Çağıran: Actor veya başka runtime sistem
- Çağrılan: `UnInitializePhysics`, `onActorDestroyed.Broadcast`, `Object::Destroy`
- State değişikliği: Physics body removal kuyruğu; actor pending-destroy.
- Sonraki adım: Tick/render guard’ları actor’ı atlar.

Dosya: `VegaEngine/src/framework/Actor.cpp`  
Sınıf veya namespace: `vega::Actor`  
Fonksiyon: `Destroy`  
Görevi: Tekrarlı destroy çağrısını erken çıkışla idempotent yapmak.

```cpp
if (GetIsPendingDestroy())
{
	return;
}

UnInitializePhysics();
onActorDestroyed.Broadcast(this);
Object::Destroy();
```

## Adım 5 — Object pending state ve callback

- Dosya: `VegaEngine/src/framework/Object.cpp`
- Sembol: `vega::Object::Destroy`
- Çağıran: `Actor::Destroy`
- Çağrılan: `onDestory.Broadcast`
- State değişikliği: `mPendingDestroy = true`.
- Sonraki adım: World temizleme.

Dosya: `VegaEngine/src/framework/Object.cpp`  
Sınıf veya namespace: `vega::Object`  
Fonksiyon: `Destroy`  
Görevi: Bütün runtime object’ler için ortak deferred-destruction işareti.

```cpp
if (mPendingDestroy)
{
	return;
}

mPendingDestroy = true;
onDestory.Broadcast(this);
```

## Adım 6 — Aktif collection’dan çıkarma

- Dosya: `VegaEngine/src/framework/World.cpp`
- Sembol: `vega::World::CleanCycle`
- Çağıran: `World::TickInternal` ve periyodik Application clean cycle
- Çağrılan: `List::erase`
- State değişikliği: World’ün güçlü actor referansı bırakılır.
- Sonraki adım: Başka güçlü owner yoksa destructor.

Dosya: `VegaEngine/src/framework/World.cpp`  
Sınıf veya namespace: `vega::World`  
Fonksiyon: `CleanCycle`  
Görevi: Iterator-safe active actor removal.

```cpp
if (iter->get()->GetIsPendingDestroy())
{
	// Decrement global active actor count for monitoring
	vega::perf::DecActiveActors();
	iter = mActors.erase(iter);
}
else
{
	iter++;
}
```

## Adım 7 — Destructor ve fizik cleanup

- Dosya: `VegaEngine/src/framework/Actor.cpp`
- Sembol: `vega::Actor::~Actor`
- Çağıran: Son `shared_ptr` release
- Çağrılan: `UnInitializePhysics`
- State değişikliği: Varsa body removal tekrar talep edilir.
- Sonraki adım: Base `Object` destructor ve bellek bırakma.

Dosya: `VegaEngine/src/framework/Actor.cpp`  
Sınıf veya namespace: `vega::Actor`  
Fonksiyon: `~Actor`  
Görevi: Fizik unregister için son güvenlik noktası.

```cpp
Actor::~Actor()
{
	UnInitializePhysics();
}
```

## Zamanlama sonucu

Spawn allocation’ı anlık, aktivasyonu deferred’dır. Destroy state’i anlık, C++ nesne yok edilmesi sahiplik durumuna bağlı ve deferred’dır. World dışındaki bir `shared_ptr`, destructor’ı uzatabilir. Collision callback’inde destroy edilen actor pending olarak aynı frame render’dan filtrelenir; collection erase bir sonraki world tick’e kalabilir.

## Test ve doğrulama

### Mevcut testler

`GasLiteCoreTests.cpp` içindeki projectile/telegraph senaryoları actor spawn’dan sonra `World::TickInternal(0.f)` ile promotion yapar; destroy/expire sonrası ek tick ile `GetActorsByType(...).empty()` veya weak expiration denetler.

### Doğrudan test edilmeyen davranışlar

Base Actor’ın delegate yayın sırası, destructor’ın tam zamanı, dış `shared_ptr`ın destruction’ı uzatması ve physics removal kuyruğu izole edilmemiştir.

### Manuel doğrulama gereken noktalar

Collision event’i içinde actor destroy, world switch anında pending actor ve destructor sırasında singleton physics erişimi.

### Önerilen fakat henüz bulunmayan testler

- Lifecycle event log’lu minimal Actor testi.
- BeginPlay içinde spawn edilen ikinci actor’ın bir sonraki tick’te başladığı testi.
- Destroy edilen actor’a dış `shared_ptr` tutulduğunda world query’den çıkıp nesnenin pending kaldığı testi.

## Kod Okuma Sırası

1. `VegaEngine/include/framework/World.h`
2. `VegaEngine/src/framework/World.cpp`
3. `VegaEngine/include/framework/Actor.h`
4. `VegaEngine/src/framework/Actor.cpp`
5. `VegaEngine/src/framework/Object.cpp`
6. `VegaEngine/src/framework/PhysicsSystem.cpp`
7. `LightYearsGame/tests/GasLiteCoreTests.cpp`
