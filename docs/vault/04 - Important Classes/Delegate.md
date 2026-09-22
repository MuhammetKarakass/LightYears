---
type: class
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
source_files:
  - LightYearsEngine/include/framework/Delegate.h
symbols:
  - ly::Delegate
  - ly::Delegate::BindAction
  - ly::Delegate::Broadcast
related:
  - "[[Engine Runtime Services]]"
  - "[[Timer, Delegate and Camera Frame Flow]]"
  - "[[Ownership and Lifetime]]"
---

# Delegate

`ly::Delegate<Args...>` publisher'ın sahip olduğu callback listesiyle event yayınlar. Callback `bool` döner: `true` listener'ın aktif kaldığını, `false` callback'in Broadcast sırasında silineceğini belirtir.

| Binding | Lifetime davranışı |
|---|---|
| `BindAction(weak_ptr<Object>, member)` | Owner expired ise callback çalışmaz ve sonraki Broadcast'te listeden çıkar |
| `BindAction(ClassName*, member)` | Yalnız null kontrolü; object lifetime garantisi caller'a ait |

`Broadcast` empty list için erken döner ve başarısız callback'i iterator-safe `erase` ile temizler. Explicit unsubscribe, subscription token veya listener sıralama önceliği API'si yoktur. Callback'in yayın sırasında aynı delegate'i yeniden değiştirmesi için güvenli reentrancy garantisi doğrulanmadı.

