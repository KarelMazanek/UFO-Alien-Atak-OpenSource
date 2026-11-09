# UFO Alien Atak — open-source C++

Veřejné zdroje vývojové hry **dev.57**, C++17 a SDL2. Vlastní kód je pod [MIT](LICENSE). Samostatný mapeditor je volitelný; editor sad GAMESET je součástí hry. [Návod Gamesetu](native/GAMESET-EDITOR.md).

## Sestavení

Potřeba: CMake >=3.20, C++17, SDL2 >=2.0.18 a Make/Ninja.

```sh
cmake -S native -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
```

`-DUAA_BUILD_MAPEDITOR=OFF` sestaví jen hru. Zdrojové kódy editoru a testů jsou přiložené.

## Herní média a spuštění

Tento veřejný zdrojový snímek zatím neobsahuje původní hudbu, grafiku ani fontové soubory. **Hudba a původní grafika nejsou pod MIT**; práva a licence jsou v [LICENSING.md](LICENSING.md) a [MUSIC-LICENSE.txt](MUSIC-LICENSE.txt). Pro běh a úplné testy jsou potřeba herní média z oprávněně získané instalace. Spuštění: `./build/uaa-native --data /cesta/k/uplnym/datum`. Samostatný mapeditor vyžaduje původní `uaa-data.pak` předaný pomocí `--data`. Veřejné instalátory budou připojené k Releases po vypořádání zveřejnění médií.

## Hlášení chyb

Použijte Issues; uveďte verzi, systém, přesný postup a případný screenshot. Nepřikládejte osobní údaje, přístupové klíče ani pracovní zálohy.

## Stav

Port má nedokončený taktický boj a úplné vyhodnocení misí; nabídka Grafy zůstává prázdná. Původní pracovní sestavení prošlo 61 kontrolami Linux32 a 61 Windows32 ve Wine. Tento repozitář má samostatnou veřejnou historii, bez soukromých reportů, záloh a uživatelských kampaní.
