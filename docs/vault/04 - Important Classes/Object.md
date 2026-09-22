---
type: class
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
source_files:
  - LightYearsEngine/include/framework/Object.h
  - LightYearsEngine/src/framework/Object.cpp
  - LightYearsEngine/include/framework/Delegate.h
symbols:
  - ly::Object
  - ly::Object::Destroy
  - ly::Object::GetWeakPtr
  - ly::Delegate::Broadcast
related:
  - "[[Object Lifecycle]]"
  - "[[Ownership and Lifetime]]"
  - "[[Actor]]"
---

# Object

`ly::Object`, runtime nesneleri için pending-destroy state’i, unique ID, weak self-reference ve destroy delegate’i sağlayan tabandır. `std::enable_shared_from_this<Object>` kullanır; `GetWeakPtr`, nesne gerçekten `shared_ptr` tarafından yönetiliyorsa anlamlı weak reference üretir.

## Kritik gerçek kod

Dosya: `LightYearsEngine/include/framework/Object.h`  
Sınıf veya namespace: `ly::Object`  
Fonksiyon: sınıf tanımı  
Görevi: Shared-from-this tabanı ve lifecycle API’si.

```cpp
class Object : public std::enable_shared_from_this<Object>
{
public:
	Object();
	virtual ~Object();

	virtual void Destroy();
	bool GetIsPendingDestroy() const { return mPendingDestroy; };

	Delegate<Object*> onDestory;
	weak_ptr<Object> GetWeakPtr();
	weak_ptr<const Object> GetWeakPtr() const;
	unsigned int GetUniqueID() { return mUniqueID; };
```

Dosya: `LightYearsEngine/src/framework/Object.cpp`  
Sınıf veya namespace: `ly::Object`  
Fonksiyon: `Destroy`  
Görevi: Idempotent pending-destroy geçişi ve listener bildirimi.

```cpp
if (mPendingDestroy)
{
	return;
}

mPendingDestroy = true;
onDestory.Broadcast(this);
```

Dosya: `LightYearsEngine/src/framework/Object.cpp`  
Sınıf veya namespace: `ly::Object`  
Fonksiyon: `GetWeakPtr`  
Görevi: Object yaşamını uzatmayan self-reference üretmek.

```cpp
weak_ptr<Object> Object::GetWeakPtr()
{
	return weak_from_this();
}
weak_ptr<const Object> Object::GetWeakPtr() const
{
	return weak_from_this();
}
```

## Test ve doğrulama

### Mevcut testler

Timer ve delegate kullanan gameplay testleri `GetWeakPtr` yolunu dolaylı kullanır. Pending-destroy state’i projectile/visual cleanup kontrollerinde okunur.

### Doğrudan test edilmeyen davranışlar

Unique ID artışı, double-destroy broadcast count ve shared ownership dışında oluşturulan Object için empty weak pointer davranışı.

### Manuel doğrulama gereken noktalar

Stack üzerinde oluşturulmuş Object türevlerinde weak-bound callback’lerin sessizce bağlanıp bağlanmadığı.

### Önerilen fakat henüz bulunmayan testler

- Destroy’un yalnız bir kez broadcast yaptığı test.
- Heap/shared ve stack Object `GetWeakPtr` farkı.
- Unique ID uniqueness testi.

## Kod Okuma Sırası

1. `LightYearsEngine/include/framework/Object.h`
2. `LightYearsEngine/src/framework/Object.cpp`
3. `LightYearsEngine/include/framework/Delegate.h`
4. `LightYearsEngine/include/framework/Actor.h`

