---
type: architecture
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
source_files:
  - VegaEngine/include/framework/World.h
  - VegaEngine/src/framework/World.cpp
  - VegaEngine/src/framework/Actor.cpp
  - VegaEngine/src/framework/Object.cpp
  - VegaEngine/src/framework/PhysicsSystem.cpp
symbols:
  - vega::World::SpawnActor
  - vega::World::TickInternal
  - vega::Actor::BeginPlayInternal
  - vega::Actor::TickInternal
  - vega::Actor::Destroy
  - vega::World::CleanCycle
related:
  - "[[Actor Spawn and Destruction Flow]]"
  - "[[Ownership and Lifetime]]"
  - "[[Actor]]"
  - "[[World]]"
---

# Object Lifecycle

## Actor yaşam döngüsü

Gerçek akışta allocation anında, world’e aktif katılım deferred’dır. Destruction da iki parçalıdır: fizik/event durdurma ve pending işareti hemen; `shared_ptr` koleksiyonundan çıkarma world temizleme noktasında gerçekleşir.

```mermaid
sequenceDiagram
    participant Caller
    participant World
    participant Pending as mPendingActors
    participant Active as mActors
    participant Actor
    participant Physics

    Caller->>World: SpawnActor<ActorType>(args)
    World->>Actor: make_shared(this, args)
    World->>Pending: push_back(shared_ptr)
    World-->>Caller: weak_ptr
    Caller->>World: TickInternal(dt)
    World->>Pending: swap(actorsToSpawn)
    World->>Active: push_back(actor)
    World->>Actor: BeginPlayInternal()
    World->>Actor: TickInternal(dt)
    Actor->>Actor: Destroy()
    Actor->>Physics: RemoveListener(bodyId)
    Actor->>Actor: pendingDestroy = true
    World->>Active: CleanCycle / erase
    Active-->>Actor: son shared_ptr ise destructor
    Actor->>Physics: UnInitializePhysics()
```

## Aşamalar

1. **Oluşturma talebi:** `World::SpawnActor`.
2. **Bellek tahsisi:** `std::make_shared<ActorType>(this, args...)`.
3. **World’e kayıt:** önce `mPendingActors`; aktif koleksiyona henüz girmez.
4. **BeginPlay:** sonraki `World::TickInternal` başında pending liste swap edilir, actor `mActors`a eklenir ve `BeginPlayInternal` bir kez çağrılır.
5. **Tick:** aynı world tick’inde `Actor::TickInternal`; began-play ve pending-destroy guard’ları geçerse sanal `Tick`.
6. **Collision/rendering:** fizik açık ise body `PhysicsSystem`e kaydolur; physics world tick’inden sonra step edilir. Render yalnızca `mActors` içindeki, pending olmayan actor’ları katman sırasıyla çizer.
7. **Destruction talebi:** `Actor::Destroy`, physics unregistration talebini verir ve destroy delegate’lerini yayınlar.
8. **Koleksiyondan çıkarılma:** `World::CleanCycle`, pending actor’ı `mActors`tan siler.
9. **Destructor/cleanup:** son güçlü referans bırakıldığında `Actor::~Actor`; fizik unregistration idempotent biçimde tekrar denenir.

## Kritik gerçek kod

Dosya: `VegaEngine/include/framework/World.h`  
Sınıf veya namespace: `vega::World`  
Fonksiyon: `SpawnActor`  
Görevi: Actor’ı hemen tahsis edip deferred spawn kuyruğuna koymak.

```cpp
shared_ptr<ActorType> newActor = std::make_shared<ActorType>(this, args...);
mPendingActors.push_back(newActor);
vega::perf::IncActiveActors();
return newActor;
```

Dosya: `VegaEngine/src/framework/World.cpp`  
Sınıf veya namespace: `vega::World`  
Fonksiyon: `TickInternal` / yerel `promotePendingActors`  
Görevi: Kuyruğu ayrı listeye swap ederek spawn sırasında yeni spawn’ların iterasyonu bozmamasını sağlamak.

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

Dosya: `VegaEngine/src/framework/Actor.cpp`  
Sınıf veya namespace: `vega::Actor`  
Fonksiyon: `BeginPlayInternal`, `TickInternal`  
Görevi: BeginPlay’in tek seferlik, Tick’in yalnızca canlı actor için olmasını sağlamak.

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

Dosya: `VegaEngine/src/framework/Actor.cpp`  
Sınıf veya namespace: `vega::Actor`  
Fonksiyon: `Destroy`  
Görevi: Fizik katılımını kesmek, actor event’ini yayınlamak ve Object’i pending-destroy yapmak.

```cpp
if (GetIsPendingDestroy())
{
	return;
}

UnInitializePhysics();
onActorDestroyed.Broadcast(this);
Object::Destroy();
```

Dosya: `VegaEngine/src/framework/World.cpp`  
Sınıf veya namespace: `vega::World`  
Fonksiyon: `CleanCycle`  
Görevi: Pending-destroy actor’ları güçlü sahiplik koleksiyonundan güvenli iterator ile çıkarmak.

```cpp
for (auto iter = mActors.begin(); iter != mActors.end();)
{
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
}
```

Dosya: `VegaEngine/src/framework/PhysicsSystem.cpp`  
Sınıf veya namespace: `vega::PhysicsSystem`  
Fonksiyon: `ProcessPendingRemoveListeners`  
Görevi: Actor destruction sırasında kuyruğa alınan Box2D body silmelerini bir sonraki physics step başında uygulamak.

```cpp
for (auto bodyId : mPendingRemoveListeners)
{
	if (b2Body_IsValid(bodyId))
	{
		b2DestroyBody(bodyId);
	}
}
mPendingRemoveListeners.clear();
```

## Collection güvenliği

- Spawn kuyruğu `swap` ile boşaltılır; `BeginPlay`, actor tick veya stage tick içindeki yeni spawn’lar mevcut promotion döngüsüne katılmaz ve sonraki `World::TickInternal`a kalır.
- Aktif actor listesi tick sırasında silinmez. `Destroy` yalnızca state işaretler; `CleanCycle` tick döngülerinden sonra erase eder.
- Collision sırasında destruction, world clean cycle’dan sonra oluştuğu için fiziksel/aktif koleksiyon silme bir sonraki frame’e kalabilir; pending guard’ları tick ve render katılımını engeller.

## Test ve doğrulama

### Mevcut testler

`GasLiteCoreTests.cpp` birçok yerde `SpawnActor(...).lock()` ardından `World::TickInternal(0.f)` kullanarak pending actor’ları promote eder. Rocket/telegraph testleri destruction sonrası ek tick ile koleksiyonun boşaldığını ve `weak_ptr` expiration’ı doğrular.

### Doğrudan test edilmeyen davranışlar

Temel `Actor` için BeginPlay’in tam bir kez çağrılması, BeginPlay içinden spawn’ın sonraki tick’e kalması, collision callback’inde destroy sonrası erase zamanı ve destructor çağrı sırası için izole runtime testi bulunamadı.

### Manuel doğrulama gereken noktalar

Box2D body removal kuyruğunun collision callback’i sırasında güvenliği ve world switch sırasında actor destructor/physics cleanup sırası.

### Önerilen fakat henüz bulunmayan testler

- Sayaçlı test actor ile constructor → BeginPlay → Tick → Destroy → destructor sırası.
- Actor tick içinden spawn ve destroy yapıldığında collection iterator güvenliği.
- Collision callback’inde destroy edilen actor’ın aynı frame render edilmediği testi.

## Kod Okuma Sırası

1. `VegaEngine/include/framework/World.h` — spawn template’i ve sahiplik koleksiyonları.
2. `VegaEngine/src/framework/World.cpp` — promotion, tick ve temizleme.
3. `VegaEngine/include/framework/Actor.h` — lifecycle yüzeyi.
4. `VegaEngine/src/framework/Actor.cpp` — guard’lar, destroy ve destructor.
5. `VegaEngine/src/framework/Object.cpp` — pending-destroy temel state’i.
6. `VegaEngine/src/framework/PhysicsSystem.cpp` — deferred body removal.
