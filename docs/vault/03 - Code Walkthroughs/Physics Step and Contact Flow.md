---
type: walkthrough
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
source_files:
  - LightYearsEngine/src/framework/Application.cpp
  - LightYearsEngine/src/framework/PhysicsSystem.cpp
  - LightYearsEngine/src/framework/Actor.cpp
symbols:
  - ly::Application::TickInternal
  - ly::PhysicsSystem::Step
  - ly::PhysicsSystem::ProcessContactEvents
related:
  - "[[Physics System]]"
  - "[[PhysicsSystem]]"
  - "[[Actor and World System]]"
---

# Physics Step and Contact Flow

```mermaid
sequenceDiagram
    participant App as Application
    participant Physics as PhysicsSystem
    participant Box2D as Box2D World
    participant ActorA as Actor A
    participant ActorB as Actor B

    App->>Physics: Step(dt), only if World not paused
    Physics->>Physics: destroy pending body IDs
    Physics->>Box2D: b2World_Step(dt, 4)
    Box2D-->>Physics: begin/end contact events
    Physics->>Physics: validate shapes, bodies, user data, pending-destroy
    Physics->>ActorA: OnActorBegin/EndOverlap(ActorB)
    Physics->>ActorB: OnActorBegin/EndOverlap(ActorA)
```

Actor `Destroy` veya destructor yolunda physics body removal istenir. Body, `Step` başlangıcındaki deferred listede geçerliyse Box2D'den silinir. World switch ise application physics world'ünü tamamen cleanup edip tekrar initialize eder; eski body'lerin yeni World'e taşınması yoktur.

Overlap dispatch, begin ve end eventlerini ayrı döngülerde işler. Bir taraf pending destroy ise o tarafın callback'i atlanır; diğer taraf yine o anda geçerliyse kendi callback'ini alabilir.

