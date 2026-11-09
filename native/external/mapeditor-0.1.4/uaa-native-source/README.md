# UAA — nativní editor map, C++ / SDL2, 0.1.4

První zkušební nativní implementace. Nepoužívá HTML, JavaScript, Chromium ani
Electron. Obsahuje samostatné C++ datové jádro, SDL2 renderer a vlastní rozhraní.
Nejde o ověřenou náhradu všech funkcí a všech grafických kombinací HTML editoru.

## Co obsahuje

- Původní ukázkovou mapu, katalog a grafiku ze zálohy.
- Izometrické vykreslení, posun pravým tlačítkem nebo mezerníkem, přiblížení.
- Paletu terénu/objektů, hledání, kreslení, výběr, přesun, otočení a mazání.
- Historii změn, velikosti 20/50/80/120, minimapu, mřížku a průhledné zdi.
- Čtení starších JSON map a samostatných map UAA-TacticalMap 1.0.
- Ukládání samostatné JSON mapy včetně PNG grafiky, geometrie a pořadí objektů.
- Kontrolu kolizí a hranic. Výsledky pro složité průchody a některé modulové
  kombinace se mohou lišit od původního editoru.

Grafika ukázkové mapy používá původní přenosné sprity. Dosud nepoužité objekty
se vykreslují podle katalogu nebo nativní procedurální náhrady. Některé složité
registrace základnového vybavení a speciální tvary zdí nemají zatím plnou
vizuální shodu s HTML verzí. Export samostatného HTML prohlížeče není součástí
nativní verze. Nové nebo upravované mapy proto ukládejte jako nové soubory.

## Windows 32 bit

Rozbalte celý Windows ZIP do jedné složky a spusťte `UAA-mapeditor-win32-0.1.4.exe`.
`SDL2.dll` musí zůstat vedle EXE. Grafika je vložena přímo do EXE. Zápis map
probíhá do umístění vybraného v dialogu, nikoliv vedle programu automaticky.
Windows binárka byla ověřena přes Wine; na fyzickém Windows nebyla ověřena.

## Linux 32 bit

Balíček má architekturu i386. Vyžaduje x86 procesor s SSE2, glibc >= 2.34 a
SDL2 >= 2.0.18. Nainstalujte DEB správcem balíků. Na amd64 systému musí být
povolena i386 architektura a dostupné i386 závislosti. Program se objeví v
nabídce aplikací, příkaz je `uaa-mapeditor`.
Zenity je doporučené pro souborové dialogy; bez něj lze cestu napsat do editoru.

## Ovládání

Pravé tlačítko / mezerník + tažení: posun. Levé tlačítko: zvolený nástroj.
V režimu Vybrat lze objekt přetáhnout za jeho půdorys. Kolečko nad mapou:
přiblížení; nad paletou: posun nabídky. R: otočit, Delete: smazat, G: mřížka.
Ctrl+S/O/N: uložit / otevřít / nová mapa. Ctrl+Z/Y: zpět / vpřed. F1: nápověda.

## Sestavení

C++17, SDL2 >= 2.0.18, CMake >= 3.16. JSON/PNG/font knihovny jsou ve vendor/.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Datový balík získáte z dodaného EXE bez jeho spuštění:

```
python3 tools/extract_data.py UAA-mapeditor-win32.exe build/uaa-data.pak
./build/uaa-mapeditor --data build/uaa-data.pak
```

32bitový cíl musí být zvolen v toolchainu. Pouhé přejmenování 64bitové
binárky nebo změna položky Architecture v DEB nestačí.

Diagnostické režimy bez okna:

```
uaa-mapeditor --headless --data uaa-data.pak --frames 1
uaa-mapeditor --data uaa-data.pak --self-test work-test-directory
uaa-mapeditor --data uaa-data.pak --snapshot preview.bmp
```

Poslední dva režimy používají dummy video driver a neotevírají okno.


## Změny 0.1.4

Velikost 20 × 20 v menu, ukládání i načítání. Při zmenšení obsazené mapy se
po potvrzení odstraní přesahující objekty; Zpět obnoví původní stav.

Ovládání zachovává horní lištu, logo, pořadí karet, pět sloupců palety terénu
a pravý informační panel podle dodaného snímku. Při menším okně se celý návrh
rovnoměrně zmenší, nabídka se nepřeskupuje. Mapové měřítko je pevné 100 %.

Domy obsahují stěny omítané a cihlové, okna a dveře, rohy a konce zdí, kamennou
zeď a ploty. UFO obsahuje rovné i zakřivené stěny, okna, široké dveře, technické
a poškozené varianty. Doplněné moduly mají nativní náhledy a geometrii, nikoliv
prázdné karty. Speciální moduly dosud bez původního přenosného spritu používají
nativní procedurální kresbu; nemusí se ve všech detailech shodovat s HTML.

## Licence a autorství

Vlastní kód mapeditoru: **MIT** (LICENSE). Hudba projektu UAA: **© Karel Keslly Mazánek**, všechna práva vyhrazena, mimo MIT (MUSIC-LICENSE.txt). Oznámení najdeš také pod tlačítkem **?**. Licenční doplnění nemění formát map ani původní grafiku.
