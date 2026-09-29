---
type: ability-dashboard
aliases:
  - yetenekler
target_per_category: 5
evolves_per_ability: 5
last_verified: 2026-09-20
---

# Yetenekler Panosu

> 7 Eylül 2026: Bu pano tasarım/not kapsamını sayar. Runtime kayıtları
> [[Ability Content Inventory]] içindedir; gerçek default slot kaynağı
> `LightYearsGame/src/gameplay/ability/loadout/DefaultAbilityLoadout.cpp`'dir.
> [[00 - Runtime Snapshot]] yalnızca tarihsel snapshot'tır; evolve tablo
> satırları uygulanmış özellik kanıtı değildir.

Bu sayfa, `Abilites/` klasöründeki yetenek notlarını otomatik tarar. Yeni
yetenek eklemek için **Yetenek Notu** şablonunu kullanın. Güncel runtime
kaynağı `DefaultAbilityLoadout.cpp` ve `assets/content/data/abilities.json`'dır.

## Calisma algoritmasi

1. Templater, yetenek notu icin kategori, slot, durum ve adi ister.
2. Not `Yetenekler/<kategori>/` altina kaydedilir ve frontmatter ozellikleri yazilir.
3. Dataview, `type: ability` olan notlari tarar.
4. Kategori, durum ve slot bilgilerine gore toplamlar hesaplanir.
5. Bos slotlar “Planlananlar” tablosunda gorunur.
6. Her yetenek notu icindeki evolve tablosu, 5 evolve seviyesinin durumunu tutar.

## Genel durum

```dataviewjs
const pages = dv.pages('"Abilites"').where(p => p.type === "ability");
const categories = ["passive", "offensive", "defensive", "movement", "control", "utility"];
const labels = {
  passive: "Passive",
  offensive: "Offensive",
  defensive: "Defensive",
  control: "Control",
  utility: "Utility",
  movement: "Movement"
};
const target = 5;

dv.table(
  ["Kategori", "Mevcut", "Planlanan hedef", "Eksik slot"],
  categories.map(category => {
    const count = pages.where(p => p.category === category).length;
    return [labels[category], count, target, Math.max(target - count, 0)];
  }).concat([["TOPLAM", pages.length, target * categories.length, Math.max(target * categories.length - pages.length, 0)]])
);
```

## Durum dagilimi

```dataviewjs
const pages = dv.pages('"Abilites"').where(p => p.type === "ability");
const statuses = ["planned", "design", "implemented", "tested", "cancelled"];
const labels = {
  planned: "Planlandi",
  design: "Tasarimda",
  implemented: "Uygulandi",
  tested: "Test edildi",
  cancelled: "Iptal"
};

dv.table(
  ["Durum", "Yetenek sayisi"],
  statuses.map(status => [labels[status], pages.where(p => p.status === status).length])
);
```

## Planlananlar ve 5'li slotlar

```dataviewjs
const pages = dv.pages('"Abilites"').where(p => p.type === "ability");
const categories = ["passive", "offensive", "defensive", "movement", "control", "utility"];
const labels = {
  passive: "Passive",
  offensive: "Offensive",
  defensive: "Defensive",
  control: "Control",
  utility: "Utility",
  movement: "Movement"
};
const statusLabels = {
  planned: "Planlandi",
  design: "Tasarimda",
  implemented: "Uygulandi",
  tested: "Test edildi",
  cancelled: "Iptal"
};

const rows = [];
for (const category of categories) {
  for (let slot = 1; slot <= 5; slot += 1) {
    const matches = pages.where(p => p.category === category && Number(p.slot) === slot);
    const page = matches.length > 0 ? matches[0] : null;
    rows.push([
      labels[category],
      slot,
      page ? dv.fileLink(page.file.path, false, page.name ?? page.file.name) : "Bos plan slotu",
      page ? (statusLabels[page.status] ?? page.status) : "Planlandi",
      page ? (page.evolve_count ?? 5) : 5
    ]);
  }
}

dv.table(["Kategori", "Slot", "Yetenek", "Durum", "Evolve"], rows);
```

## Tum yetenekler

```dataviewjs
const pages = dv.pages('"Abilites"')
  .where(p => p.type === "ability")
  .sort(p => [p.category, Number(p.slot), p.name]);
const labels = {
  passive: "Passive",
  offensive: "Offensive",
  defensive: "Defensive",
  control: "Control",
  utility: "Utility",
  movement: "Movement"
};
const statusLabels = {
  planned: "Planlandi",
  design: "Tasarimda",
  implemented: "Uygulandi",
  tested: "Test edildi",
  cancelled: "Iptal"
};

dv.table(
  ["Yetenek", "Kategori", "Durum", "Slot", "Evolve", "Ozellikler"],
  pages.map(p => [
    dv.fileLink(p.file.path, false, p.name ?? p.file.name),
    labels[p.category] ?? p.category,
    statusLabels[p.status] ?? p.status,
    `${p.slot ?? "-"}/5`,
    p.evolve_count ?? 5,
    Array.isArray(p.features) && p.features.length > 0 ? p.features.join(", ") : "-"
  ])
);
```

## Durum gecisleri

| Durum | Anlami |
|---|---|
| `planned` | Fikir veya backlog kaydi var |
| `design` | Algoritma, statlar ve evolve'ler tasarlaniyor |
| `implemented` | Kod veya oyun entegrasyonu yapildi |
| `tested` | Test senaryolari tamamlandi |
| `cancelled` | Artik planlanmiyor |

## Yetenek notunda doldurulacak temel alanlar


ame`, `category`, `slot`, `status`, `priority`, `features`, `scaling` ve 5 satirlik evolve tablosu. Sayaçlar elle girilmez; notlarin frontmatter alanlarindan otomatik hesaplanir.
