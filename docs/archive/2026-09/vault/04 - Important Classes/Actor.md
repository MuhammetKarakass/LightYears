---
type: class
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
source_files:
  - LightYearsEngine/include/framework/Actor.h
  - LightYearsEngine/src/framework/Actor.cpp
symbols:
  - ly::Actor
  - ly::Actor::BeginPlayInternal
  - ly::Actor::TickInternal
  - ly::Actor::Destroy
  - ly::Actor::InitializePhysics
  - ly::Actor::Render
related:
  - "[[Actor and World System]]"
  - "[[Object Lifecycle]]"
  - "[[World]]"
  - "[[Object]]"
---

# Actor

## 7 Eylül 2026 kaynak kontrolü

Actor::Destroy spatial kaydı ve fiziği kaldırır, onActorDestroyed yayar, sonra Object::Destroy ile pending olur. Callback içinde yeniden Destroy guard'ı aşabilir; bu risk test edilmedi. Sıralama kaynakta doğrulandı: LightYearsEngine/src/framework/Actor.cpp.

Bu ek kaynak incelemesidir; build/test çalıştırılmadı. Aşağıdaki eski örnekler tarihsel inceleme kapsamını taşır; bu güncelleme ile çelişen ayrıntılar güncel davranış kabul edilmemeli.


`ly::Actor`, `Object` tabanlı world entity’sidir. Transform, sprite/light, render layer, collision layer/mask, isteğe bağlı Box2D body ve lifecycle hook’ları sağlar.

Generic Actor, kendi belleğinin sahibi değildir; World onu `shared_ptr` ile tutar. Actor’ın `World*` alanı non-owning’dir. Generic owner/parent/component container içermez.

## Kritik gerçek kod

Dosya: `LightYearsEngine/src/framework/Actor.cpp`  
Sınıf veya namespace: `ly::Actor`  
Fonksiyon: constructor / destructor  
Görevi: World bağlantısını ve başlangıç runtime state’ini kurmak.

```cpp
Actor::Actor(World* OwningWorld, const std::string& TexturePath)
	: mOwningWorld{ OwningWorld },
	mBeganPlay{ false },
	mSprite{},
	mTexture{},
	mActorLocation{ 0.f, 0.f },
	mActorRotation{ 0.f },
	mVelocity{},
	mPhysicsBodyId{},
	mPhysicsEnabled{ false },
	mCollisionLayer{ CollisionLayer::None },
	mCollisionMask{ CollisionLayer::None },
	mCanCollide{ false },
	mTickWhenPaused{ false }
{
	SetTexture(TexturePath);
}

Actor::~Actor()
{
	UnInitializePhysics();
}
```

Dosya: `LightYearsEngine/src/framework/Actor.cpp`  
Sınıf veya namespace: `ly::Actor`  
Fonksiyon: `BeginPlayInternal`, `TickInternal`  
Görevi: Sanal hook’ları lifecycle state ile korumak.

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

Dosya: `LightYearsEngine/src/framework/Actor.cpp`  
Sınıf veya namespace: `ly::Actor`  
Fonksiyon: `Destroy`  
Görevi: Physics bağlantısını kesmek ve deferred object destruction’ı başlatmak.

```cpp
if (GetIsPendingDestroy())
{
	return;
}

UnInitializePhysics();
onActorDestroyed.Broadcast(this);
Object::Destroy();
```

Dosya: `LightYearsEngine/src/framework/Actor.cpp`  
Sınıf veya namespace: `ly::Actor`  
Fonksiyon: `UnInitializePhysics`  
Görevi: Body removal’ı PhysicsSystem kuyruğuna vermek.

```cpp
if(mPhysicsBodyId)
{
	PhysicsSystem::Get().RemoveListener(*mPhysicsBodyId);
	mPhysicsBodyId.reset();  
}
```

## Test ve doğrulama

### Mevcut testler

Testlerde base Actor ve çok sayıda Actor türevi World üzerinden spawn edilir; transform, collision layer ve pending-destroy kontrolleri kullanılır.

### Doğrudan test edilmeyen davranışlar

Base BeginPlay/Tick call-count, destructor sırası, render guard’ı ve idempotent physics unregister için odaklı test bulunamadı.

### Manuel doğrulama gereken noktalar

Texture/sprite bounds yokken physics davranışı, light render ve collision callbacks.

### Önerilen fakat henüz bulunmayan testler

- Sayaçlı Actor lifecycle testi.
- Double-destroy ve double-unregister testi.
- Pending actor render/tick filtreleme testi.

## Kod Okuma Sırası

1. `LightYearsEngine/include/framework/Actor.h`
2. `LightYearsEngine/src/framework/Actor.cpp`
3. `LightYearsEngine/include/framework/Object.h`
4. `LightYearsEngine/src/framework/World.cpp`
5. `LightYearsEngine/src/framework/PhysicsSystem.cpp`
