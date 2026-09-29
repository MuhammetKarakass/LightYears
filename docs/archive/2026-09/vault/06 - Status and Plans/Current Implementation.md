---
type: status
verified_on: 2026-09-07
verification: source-review-only
verified_worktree_state: dirty
---

# Current Implementation

7 Eylül 2026 kaynak denetimi: [[2026-09-07 Project Status Review]]. Sistemlerin bugünkü kapsamı [[System Index]], input/loadout [[00 - Runtime Snapshot]], kayıtlı içerik [[Ability Content Inventory]] içinde tek yerde tutulur.

Uygulanmış kod ile oynanış entegrasyonu farklıdır: combat/runtime/ship XP/respawn çağrı yolları mevcut; arena restart, LevelOne HUD tick ve scrap kazan–harca bağlantısı eksik. Save/load yolu bulunmadı. Evolve mimari kontratı uygulanmış seçim sistemi değildir. Test/oyun çalıştırılmadı.

## Kurtarılan kişisel yetenek ve tasarım notları

7 Eylül 2026'da Obsidian yerel indeksinden kurtarıldı. Aşağıdakiler kaynak
kodla doğrulanmış durum iddiası değil; uygulanması değerlendirilen kişisel
fikir/notlardır. Metin özgün nottan anlam değiştirmeden korunmuştur.

> **Echo Protocol geçici uygulama notu — ileride revize edilecek:** Ortak
> ability kullanım hafızası son 10 kaydı saklamaya devam eder; Echo ise kendi
> son tükettiği sequence sonrasındaki yeni normal yeteneği kullanır. Bu nedenle
> `A → B → C → Echo` sonrasında yeni yetenek gelmeden Echo tekrar kullanıldığında
> `B/A` kayıtlarına geri dönmez. Tüketilen kayıtlar ortak hafızadan silinmez;
> gelecekteki checkpoint, replay ve analytics sistemleri için tutulur.
> Cursor/branching davranışı gelecekte ortak hafıza sistemi tasarımı netleşince
> yeniden değerlendirilecek.

```cpp
void CombatRuntime::InitializeOwnerAttributes(float maxHealth)
{
	mAttributeSystem.RegisterAttribute(OwnerAttributeIds::MaxHealth, maxHealth);
	mAttributeSystem.RegisterAttribute(OwnerAttributeIds::HealthRegen, 0.f);
	mAttributeSystem.RegisterAttribute(OwnerAttributeIds::EnergyPower, 0.f);
	mAttributeSystem.RegisterAttribute(OwnerAttributeIds::AfterburnerRegen, 0.f);
	mAttributeSystem.RegisterAttribute(OwnerAttributeIds::AttackPower, 0.f);
	mAttributeSystem.RegisterAttribute(OwnerAttributeIds::AttackSpeed, 0.f);
	mAttributeSystem.RegisterAttribute(OwnerAttributeIds::AbilityHaste, 0.f);
	mAttributeSystem.RegisterAttribute(OwnerAttributeIds::MoveSpeedHorizontal, 0.f);
	mAttributeSystem.RegisterAttribute(OwnerAttributeIds::MoveSpeedVertical, 0.f);
	mAttributeSystem.RegisterAttribute(OwnerAttributeIds::Armor, 0.f);
	mAttributeSystem.RegisterAttribute(OwnerAttributeIds::Luck, 0.f);
	mAttributeSystem.RegisterAttribute(OwnerAttributeIds::CriticalChance, 0.f);
}
```

- her gemi farklı statlar ile başlayacak, başlangıç değeri 0 olmayacak
- infernospray seviye atlıma yapısı lazım
- parry veya hasar biriktirme yeteneği
- DirectionalBarrier
- Rush yeteneği hızlanarak çarpma hasarı vurma
- json runtime'da değiştirilmiyormuş

### Arc Relay

Bir düşmana enerji işareti koyarsın. O hedefe vurdukça yakın düşmanlara
elektrik arkları sıçrar. `AttackSpeed` daha sık tetiklenmesini, `Luck` ekstra
zincir ihtimalini etkileyebilir. Tek başına damage ability değil, **diğer
saldırılarını dönüştüren aktif buff**.

### Time Fracture

Büyük bir alan oluşturur. İçindeki düşmanların hareketi, projectile'ları ve
attack cycle'ları yavaşlar; oyuncu normal hızda kalır. Gravity Anomaly gibi
çekmez. Saf **tempo kontrolü**.

### Ricochet Protocol

Aktive edildikten sonra birkaç saniye boyunca primary weapon projectile'ların
ilk düşmana çarptıktan sonra başka bir düşmana seker. `Luck → ekstra sekme
ihtimali`, `AttackPower → sekme hasarı`. Çok karmaşık olmayan ama build
dönüştüren bir buff.

### Kinetic Ram

Oyuncu kısa mesafe ileri fırlar. Bir düşmana çarparsa onu da beraberinde
sürükler; başka düşmana veya duvara çarptırırsa iki tarafa da hasar verir.
`MaxHealth → çarpma hasarı`, `Mobility → taşıma/atılma mesafesi`.

- sweetspotlu bir yetenek grubu
- sejuani r si
- oversiheld yeteneklerde bulunan bir davranış değil bir gemi davranışı olmalı
- hasar scale tipleri
- bir kaç kere
- hareket hızına göre dalga oluşturacak ses duvarının aşılması gibi o anki
  hareket hızına göre hasar verecek alan artacak

### Solar Moths

Dört küçük termal drone farklı hedeflere dağılır.

- Öncelikle Ignite olmayan hedefleri seçerler.
- Hedefe çarpmaz, hedefin çevresinde kısa süre dönerler.
- Her turda küçük Thermal hasar ve Ignite buildup uygularlar.
- Hedef Ignite olduğunda başka bir yanmamış hedefe geçerler.
- Uygun hedef kalmazsa mevcut yanan hedefin çevresinde birleşerek kısa bir
  patlama yaparlar.

**Rol:** Offensive / Status spread  
**Hasar:** Thermal  
**Scale:** AttackPower; Luck hedef değiştirme sayısını artırabilir.  
**Farkı:** Doğrudan patlayan homing füze değil, Ignite kuran gezici sürü.

### Ember Leash

Bir termal zıpkın hedefe homing yaparak yapışır ve atıldığı noktaya yanan bir
bağ oluşturur.

- Zıpkın hedefe ulaştığında ilk hasarı düşüktür.
- Hedef hareket ettikçe sabit başlangıç noktasıyla arasında uzayan ateş hattı
  oluşur.
- Hattı geçen düşmanlar Thermal buildup alır.
- Bağ maksimum uzunluğu aşınca koparak küçük bir patlama yapar.
- Oyuncu hedefin hareketini kullanarak arena içinde ateş duvarı çizdirebilir.

**Rol:** Control / Thermal setup  
**Hasar:** Thermal  
**Scale:** AttackPower → tick hasarı; EnergyPower düşünülebilir → bağ uzunluğu  
**Farkı:** Scorch Drive izi oyuncudan, Ember Leash izi düşmandan üretir.

- ciddi hareket hızı kazanır fakat rotation speed azalır geminin önünde mızrak
  gibi bir obje belirir düşmanlara hasar verir ve sağa sola savurur
- sivir w
- starfall düşmana havadan yıldırım düşer rastgele 4 düşman luck ile düşman
  sayısı artar ufak alan hasarı verir yani 2 düşman çok yakınsa hasar verilir
  birbirşne
- kalkan kazanır overshield ise overshield bittiğinde alan patlaması oluşur
- tesla tower

### Orbit Arsenal

Birkaç projectile oyuncunun çevresinde bekler.

- Oyuncu bir düşmana primary fire ile vurduğunda bir projectile o hedefe homing
  yapar.
- Her isabette yalnızca bir projectile gönderilir.
- Projectile’lar kendi hasar tiplerini taşır.
- Süre sonunda kullanılmayanlar kaybolur.

Aktivasyondan sonra karmaşık kontrol istemez; normal saldırı akışına homing ekler.

### Shield Graft

Mevcut gemi kalkanını hull onarımına dönüştürür.

- Aktif kalkanın belirli miktarını tüketir.
- Tüketilen kalkanın bir bölümü hull olarak geri gelir.
- Geçici barrier’ları tüketmez; yalnız geminin ana shield kaynağını kullanır.
- Kalkan yoksa kullanılamaz.

**Rol:** Defensive/Resource conversion  
**Nadirlik:** Rare  
**Scaling:** EnergyPower veya Armor

Güçlüdür ancak oyuncu kendisini Energy saldırılarına karşı açık bırakır.

### Reclaimer Protocol

Kısa bir süre boyunca öldürülen düşmanlar hull iyileştirir.

- Örneğin 6 saniye sürer.
- Her öldürme sabit veya küçük yüzdesel iyileştirme verir.
- Toplam iyileştirmenin üst sınırı vardır.
- Yetenek aktivasyon anında iyileştirme yapmaz.

**Rol:** Offensive/Defensive  
**Nadirlik:** Rare  
**Scaling:** MaxHealth  
**Örnek:** Kill başına %4, toplam en fazla %20 MaxHealth

Execution Drive saldırı gücü toplarken bu yetenek hayatta kalma sağlar.
