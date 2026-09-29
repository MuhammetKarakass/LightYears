---
type: class
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
source_files:
  - LightYearsEngine/include/framework/camera/CameraManager.h
  - LightYearsEngine/src/framework/camera/CameraManager.cpp
  - LightYearsEngine/src/framework/World.cpp
symbols:
  - ly::CameraManager
  - ly::CameraManager::Update
  - ly::CameraManager::GetView
  - ly::CameraManager::GetDesiredCenter
related:
  - "[[Engine Runtime Services]]"
  - "[[Timer, Delegate and Camera Frame Flow]]"
  - "[[Balance Atlas]]"
---

# CameraManager

`ly::CameraManager`, World'ün value-member kamera runtime'ıdır. Weak follow target, cursor/movement look-ahead, speed/additional/relative zoom, world bounds ve sine tabanlı envelope shake state'ini tutar.

`Update`, follow target geçerliyse desired center ve zoom'u hesaplar; position exponential smoothing, zoom critically damped smoothing kullanır. `GetView`, güncel center/zoom'a yalnız render için shake offset ekler. Follow target temizlendiğinde velocity, zoom katmanları, offset preservation ve cursor look-ahead state'i sıfırlanır.

Arena game-side entegrasyonu World wrapper'ları ve `ArenaLevel::UpdateArenaCameraInputs` üzerinden gelir. Camera settings'in değeri ve playtest dengesi için [[Balance Atlas]] yalnız kaynak haritasıdır.

