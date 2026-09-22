# Obsidian Yetenek Sistemi Kurulumu

Bu yapı için topluluk eklentilerinden **Templater** ve **Dataview** gereklidir.

## Kurulum

1. Obsidian ayarlarında Templater'ın **Template folder location** değerini `_templates` yapın.
2. Templater'ın **User scripts folder location** değerini `_scripts` yapın.
3. `Yetenek Notu` şablonunu çalıştırın.
4. Kategori olarak `passive`, `offensive`, `defensive`, `movement`, `control` veya `utility` seçin.
5. Slotu 1-5 arasında seçin ve durumu kaydedin.
6. `yetenekler-dashboard.md` dosyasını açın; sayaçlar ve planlanan slotlar otomatik güncellenir.

## Klasor yapisi

```text
_scripts/
  yetenek_olustur.js
_templates/
  Yetenek Notu.md
Abilites/
  passive/
  offensive/
  defensive/
  movement/
  control/
  utility/
yetenekler-dashboard.md
```

Eski uzun `yetenekler.md` kopyası kaldırıldı. Bu klasördeki source metadata ve
okuma yolu artık kısa güncel [[yetenekler]] rehberine, oradan da
`docs/CURRENT_IMPLEMENTATION_CATALOG.md` owner belgesine bağlanır.
