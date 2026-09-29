---
type: class
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
source_files:
  - LightYearsEngine/include/framework/TimerManager.h
  - LightYearsEngine/src/framework/TimerManager.cpp
  - LightYearsEngine/src/framework/Application.cpp
symbols:
  - ly::TimerManager
  - ly::Timer
  - ly::TimerHandle
related:
  - "[[Engine Runtime Services]]"
  - "[[Timer, Delegate and Camera Frame Flow]]"
  - "[[Game Loop]]"
---

# TimerManager

## 7 Eylül 2026 kaynak kontrolü

Timer::TickTimer repeat olduğunda mTimeCounter=0 yapar; kalan dt korunmaz. Application global/game manager'ları günceller, actor temporal domain'leri timer'a otomatik taşınmaz. Büyük frame cadence ölçümü yapılmadı.

Bu ek kaynak incelemesidir; build/test çalıştırılmadı. Aşağıdaki eski örnekler tarihsel inceleme kapsamını taşır; bu güncelleme ile çelişen ayrıntılar güncel davranış kabul edilmemeli.


`TimerManager`, `TimerHandle` anahtarlı timer sözlüğünü yönetir. `SetTimer` weak owner + member callback veya `std::function` kabul eder; `ClearTimer` kaydı expired işaretler, `ClearAllTimers` sözlüğü anında boşaltır.

Application üç singletonı kullanır: global timer her frame, game timer yalnız pause dışındayken güncellenir. `ShutdownTimerManagers` hepsini clear edip resetler. World switch'te global ve game listeleri ayrıca temizlenir.

Timer callback'i owner expired/pending-destroy ise çağrılmaz. Aynı timerın callback'i içinde timer mutation davranışı için izole test doğrulanmadı.

