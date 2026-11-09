# Mapy pozemních a podvodních operací

Mapy vytvořené mapeditorem ukládej jako `*.uaa-map.json` do:

- `pozemni/` – pozemní operace.
- `podvodni/` – podvodní operace.

Tato složka `native/maps` je součástí herní instalace i bez mapeditoru. Při spuštění volitelného mapeditoru z místní hry je jeho pracovní složkou právě tento adresář. Původní projektová složka `maps` se zachovává; její případné vlastní mapy lze sem zkopírovat.

Složky jsou připravené pro výběr map podle prostředí budoucího taktického boje. Samotné vytvoření složek neimplementuje taktický boj ani jeho výběrové rozhraní. Žádné vzorové či uživatelské mapy se automaticky nepřepisují.
