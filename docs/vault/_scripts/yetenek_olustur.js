// Obsidian Templater user script.
// Templater ayarlarinda User scripts folder olarak _scripts klasorunu secin.

module.exports = async function (tp) {
  const categories = ["passive", "offensive", "defensive", "functional"];
  const categoryLabels = {
    passive: "Passive",
    offensive: "Offensive",
    defensive: "Defensive",
    functional: "Functional",
  };
  const statuses = ["planned", "design", "implemented", "tested", "cancelled"];

  const category = await tp.system.suggester(
    categories.map((value) => categoryLabels[value]),
    categories,
    false,
    "Yetenek kategorisi"
  );
  const name = await tp.system.prompt("Yetenek adi");
  if (!category || !name) {
    return "<!-- Yetenek olusturma iptal edildi. -->";
  }

  const slotInput = await tp.system.prompt("Kategori icindeki sira (1-5)", "1");
  const parsedSlot = Number.parseInt(slotInput ?? "1", 10);
  const slot = Number.isFinite(parsedSlot) ? Math.min(5, Math.max(1, parsedSlot)) : 1;

  const status = await tp.system.suggester(
    statuses,
    statuses,
    false,
    "Yetenek durumu"
  );
  const description = await tp.system.prompt("Kisa aciklama", "");

  const slug = name
    .toLowerCase()
    .normalize("NFD")
    .replace(/[\u0300-\u036f]/g, "")
    .replace(/[^a-z0-9]+/g, "-")
    .replace(/^-+|-+$/g, "") || `yetenek-${slot}`;

  const folder = `Abilites/${category}`;
  const fileBaseName = `${String(slot).padStart(2, "0")} - ${slug}`;
  if (!app.vault.getAbstractFileByPath(folder)) {
    await app.vault.createFolder(folder);
  }
  await tp.file.rename(fileBaseName);
  await tp.file.move(`${folder}/${fileBaseName}`);

  const yaml = (value) => JSON.stringify(value);
  const evolveRows = Array.from(
    { length: 5 },
    (_, index) => `| ${index + 1} | | planned | |`
  );

  return [
    "---",
    "type: ability",
    `id: ${yaml(`${category}.${slug}`)}`,
    `name: ${yaml(name)}`,
    `category: ${category}`,
    `slot: ${slot}`,
    `status: ${status || "planned"}`,
    "priority: medium",
    "max_level: 5",
    "evolve_count: 5",
    `created: ${tp.date.now("YYYY-MM-DD")}`,
    "features: []",
    "scaling: []",
    "tags:",
    "  - ability",
    `  - ability/${category}`,
    "---",
    "",
    `# ${name}`,
    "",
    `> Kategori: **${categoryLabels[category]}** | Durum: **${status || "planned"}**`,
    "",
    "## Ozellikler",
    "",
    description ? description : "Bu bolume yetenegin oyuncuya sagladigi ozellikleri yazin.",
    "",
    "| Ozellik | Deger | Aciklama |",
    "|---|---:|---|",
    "| Ornek stat | 0 | Degeri ve etkisini yazin |",
    "",
    "## Algoritma",
    "",
    "1. Tetiklenme kosulunu kontrol et.",
    "2. Hedefi veya etki alanini belirle.",
    "3. Temel etkiyi uygula.",
    "4. Scaling ve evolve degerlerini hesapla.",
    "5. Cooldown, duration ve temizleme islemlerini uygula.",
    "",
    "## Seviye Gelisimi ve Olcekleme",
    "",
    "- Seviye basi artis (L2 - L5):",
    "  - Hasar:",
    "  - Yaricap:",
    "  - Bekleme suresi:",
    "- Nitelik olcekleme:",
    "  - Owner statlari:",
    "  - Formul:",
    "",
    "## Evolve'ler",
    "",
    "| Seviye | Evolve adi | Durum | Degisim |",
    "|---:|---|---|---|",
    ...evolveRows,
    "",
    "## Notlar",
    "",
    "- Bagimliliklar:",
    "- Test senaryolari:",
    "- Dengeleme notlari:",
  ].join("\n");
};
