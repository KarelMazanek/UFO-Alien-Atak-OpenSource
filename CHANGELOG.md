# Protokol změn

Záznamy jsou od nejnovějšího k nejstaršímu. Datum dokumentace není datem změny hry. Tento veřejný protokol začíná vydáním dev.57; původní pracovní reporty a soukromá historie nejsou jeho součástí.

## 10. 11. 2026 — veřejná dokumentace

- Přidán tento protokol a odkaz z README. Herní kód, data, verze ani zveřejněné instalátory se touto úpravou nemění.

## 9. 11. 2026 — zabezpečení veřejného repozitáře

- Aktivována ochrana výchozí větve `main`: změny přes pull request, zákaz force push a smazání větve, povinné vyřešení diskusí před sloučením.
- Pravidla nemají výjimky pro správce. Počet povinných schválení je nula, aby vlastník mohl slučovat vlastní pull requesty bez druhého správce.
- Jde o nastavení GitHubu, nikoli novou herní verzi.

## 9. 11. 2026 — veřejné vydání 0.4.0-dev.57-local

- Zveřejněny C++17/SDL2 zdroje a herní data v samostatné veřejné historii.
- Vlastní kód je pod MIT. Hudba, grafika a loga si ponechávají dosavadní licenční podmínky; zveřejnění je nepřevádí pod MIT. Viz [LICENSING.md](LICENSING.md).
- [Vývojové vydání dev.57](https://github.com/KarelMazanek/UFO-Alien-Atak-OpenSource/releases/tag/v0.4.0-dev.57) obsahuje Linux32 hru, samostatný volitelný Linux32 mapeditor a Windows32 instalátor s volitelným, zpočátku nezaškrtnutým mapeditorem. Přiloženy SHA256SUMS.txt a CTI-ME.md.
- Veřejné balíčky používají stejné ověřené programové binárky jako sestavení dev.57. Z balíčků jsou vynechány soukromé pracovní reporty a podklady generování obrázků; kontrolní součty patří k tomuto veřejnému vydání.
- Ověřena shoda SHA-256 všech pěti příloh a jejich stažení bez přihlášení.

### Sestavení a ověření dev.57

- Sestavena hra a UAA Mapeditor 0.1.4 pro Linux i386 a Windows PE32. Mapeditor zůstává volitelný; GAMESET je součástí hry.
- Opraveno paměťově náročné dekódování podrobného glóbu na 32bitových systémech použitím připravené renderovací textury. Původní podrobná mapa zůstává zachována; kontrolní snímek linuxového glóbu byl totožný.
- Windows hra používá grafický subsystém a přibaluje potřebnou SDL2 knihovnu. Instalační balíčky obsahují návody a licence.
- Vydané programy prošly 61 kontrolami Linux32 a 61 Windows32 ve Wine. Windows nebyl ověřen na fyzickém počítači a instalátor není digitálně podepsán.

## 8.–9. 11. 2026 — změny hry a editorů v dev.57

Následuje konečný stav těchto úprav; přechodné varianty rozhraní jsou nahrazené popisem níže.

- Vestavěný editor sad je přístupný přímo z hlavního menu jako **GAMESET**. Vlastní sadu lze vybrat při založení kampaně; kampaň uchovává vlastní snapshot sady.
- Editor má samostatnou plochu bez levé herní nabídky a horní lišty času a peněz. Globální akce jsou nahoře s ikonami a zarovnanými tlačítky. Tlačítko Nová hra a původní řádek přímých akcí položek byly z editoru odstraněny.
- Referenční **DUMMY**, včetně jeho objektů, je pouze ke čtení a nelze jej přejmenovat ani smazat.
- Vlastní kopie sad lze přejmenovat a mazat s potvrzením. Opraveno trvalé mazání kopií a chování při chybě zápisu.
- Objekty vlastních sad mají samostatné přejmenování a potvrzené smazání. Přejmenování zachovává ID; změny podporují vrácení kroku a uložení do knihovny.
- Editor poskytuje úpravy vlastností, receptů a křivek přesnosti, import/export JSON a práci s vlastní grafikou. Podrobný rozsah: [návod Gamesetu](native/GAMESET-EDITOR.md).
- Obnoveno spuštění volitelného mapeditoru ze hry; výsledek spuštění nebo chyba se zobrazí v nabídce. Jeho datový balík obsahuje 1 668 prostředků.
- Hudba hraje pouze na herních obrazovkách. V hlavním menu, dalších neherních obrazovkách a obou editorech je pozastavena; návrat do hry respektuje hlasitost.
- Opravena lokalizace nadpisu **Autoři**, který se chybně měnil na „Automatři“.

## Známá omezení vydání dev.57

- Taktický boj a úplné vyhodnocování misí nejsou dokončené. Editovatelná data misí nenahrazují chybějící mechaniky.
- Herní nabídka **Grafy** zůstává prázdná.
- Spuštění kampaně vyžaduje úplný základní katalog; neúplné autorské sady lze upravovat a exportovat.
- Linuxové balíčky vyžadují podporu i386, libc6 >= 2.35 a SDL2 >= 2.0.18.
