---
type: class
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
source_files:
  - VegaEngine/include/framework/PhysicsSystem.h
  - VegaEngine/src/framework/PhysicsSystem.cpp
symbols:
  - vega::PhysicsSystem
  - vega::PhysicsSystem::AddListener
  - vega::PhysicsSystem::RemoveListener
  - vega::PhysicsSystem::SetCollisionRadius
related:
  - "[[Physics System]]"
  - "[[Physics Step and Contact Flow]]"
---

# PhysicsSystem

`PhysicsSystem`, tek `b2WorldId`, pending body removal listesi ve physics rate (`0.01f`) taşır. `AddListener` actor sprite bounds'ından dynamic rectangle body üretir; `RemoveListener` body ID'yi deferred listede tutar. `ProcessPendingRemoveListeners` sonraki `Step` öncesinde geçerli ID'leri yok eder.

`SetCollisionRadius` mevcut shape'leri kaldırıp dairesel shape oluşturur. Singleton shutdown, `Cleanup` çağırıp Box2D world'ünü destroy eder.

