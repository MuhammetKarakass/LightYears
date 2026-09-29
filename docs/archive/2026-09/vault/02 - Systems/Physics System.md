---
type: system
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
source_files:
  - LightYearsEngine/include/framework/PhysicsSystem.h
  - LightYearsEngine/src/framework/PhysicsSystem.cpp
  - LightYearsEngine/src/framework/Actor.cpp
  - LightYearsEngine/src/framework/Application.cpp
symbols:
  - ly::PhysicsSystem
  - ly::PhysicsSystem::Step
  - ly::PhysicsSystem::ProcessContactEvents
  - ly::PhysicsSystem::QueryActorsInBounds
  - ly::Actor::InitializePhysics
  - ly::Actor::UnInitializePhysics
related:
  - "[[Actor and World System]]"
  - "[[PhysicsSystem]]"
  - "[[Physics Step and Contact Flow]]"
  - "[[Ownership and Lifetime]]"
---

# Physics System

## 7 Eylül 2026 kaynak kontrolü

PhysicsSystem::Step, b2World_Step(dt, 4) çağırır; sabit accumulator değildir. AddListener collision layer/mask filtresini Box2D shape filter olarak kurar. World body-less spatial grid kullanır. PrimaryWeaponProjectileActor constructor SetAbilityPhysicsEnabled(false) çağırır; primary hit sahipliği sweep yolundadır. Tüm fizik/portal/interception kombinasyonları çalıştırılmadı.

Bu ek kaynak incelemesidir; build/test çalıştırılmadı. Aşağıdaki eski örnekler tarihsel inceleme kapsamını taşır; bu güncelleme ile çelişen ayrıntılar güncel davranış kabul edilmemeli.


`ly::PhysicsSystem`, Box2D 3.x world'ünü yöneten lazy singleton'dır. `Application::TickInternal`, World pause değilken `PhysicsSystem::Step(dt)` çağırır; World değişiminde eski physics world cleanup edilir ve sıfır gravity ile yenisi oluşturulur.

## Actor body lifecycle

`Actor::InitializePhysics`, enabled actor için `PhysicsSystem::AddListener(this)` çağırır. Listener, actor global sprite bounds'ından dynamic Box2D body + box shape üretir; body user data'sı raw `Actor*` olur. Görsel bounds sıfırsa body oluşturulmaz. `Actor::Destroy` **ve destructor** `UnInitializePhysics` çağırır; bu işlem body ID'sini deferred removal listesine verir.

## Step ve overlap

`Step`, önce pending body removals'ı işler, sonra `b2World_Step(world, dt, 4)` çağırır ve begin/end contact event'lerini actor callback'lerine yönlendirir. Her contact tarafı için shape/body validity, user data ve `pending destroy` kontrolü yapılır; geçerliyse `OnActorBeginOverlap` veya `OnActorEndOverlap` çağrılır.

## Broadphase bölge sorgusu

`PhysicsSystem::QueryActorsInBounds`, Box2D `b2World_OverlapAABB` broadphase
sorgusunu gameplay koordinatlarına açar ve aynı actor'ı birden fazla shape
üzerinden yalnız bir kez döndürür. `World::GetActorsInBounds` sonucu weak actor
referanslarına çevirir, başka world/pending actor'ları eler ve fizik body'si
olmayan koordinatör/test actor'ları için bounds/konum tabanlı fallback uygular.

Auto targeting, radius damage, projectile sweep, beam/wave, gravity field,
drone contact, portal ve directional barrier discovery artık bütün world actor
listesini her tick dolaşmak yerine bu dar bölge kontratını kullanır.

Collision layer/mask bilgisi Actor'da tutulur; bu katmanın Box2D filter ayarına nasıl çevrildiği bu sistemde doğrulanmadı. `SetCollisionRadius`, geçerli body'nin mevcut shape'lerini silip circle shape ile değiştirir.

## Sınırlar

- Physics step iteration, contact callback içinde actor/body mutation ve deferred removal etkileşimi için izole test doğrulanmadı.
- Broadphase sorgusu yalnız mevcut Box2D world'ündeki body'leri indeksler;
  fizik body'si olmayan actor'lar `World` fallback'i üzerinden çözülür.
- `Actor.h` içindeki destructor TODO'su kaynak gerçekle çelişir: implementation zaten cleanup yapar; yorum eski kabul edilmelidir.
