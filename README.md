# UFO Alien Atak — open-source C++ hra

Veřejné zdroje vývojové verze **0.4.0-dev.57-local**. Hra používá C++17 a SDL2. Vestavěný editor **GAMESET** je součástí hry; samostatný UAA Mapeditor 0.1.4 zůstává volitelný.

## Stažení a testování

[Instalátory pro Linux32 a Windows32](https://github.com/KarelMazanek/UFO-Alien-Atak-OpenSource/releases). U vydání jsou kontrolní součty SHA-256 a návod k instalaci. Linuxová hra a mapeditor jsou samostatné DEB; ve Windows je mapeditor ve výchozím stavu nezaškrtnutý. Chyby prosím hlaste přes [Issues](https://github.com/KarelMazanek/UFO-Alien-Atak-OpenSource/issues), včetně systému, verze, postupu a případného screenshotu. Nepřikládejte osobní údaje ani přístupové klíče.

## Licence

Vlastní zdrojový kód je pod licencí **MIT** — viz [LICENSE](LICENSE). **Hudba, grafika a loga tím nezískávají licenci MIT.** Hudba: © Karel Keslly Mazánek, všechna práva vyhrazena. Podrobnosti a licence závislostí: [LICENSING.md](LICENSING.md), [MUSIC-LICENSE.txt](MUSIC-LICENSE.txt), `native/LICENSES/` a licence mapeditoru. Natural Earth je public domain.

## Sestavení na Linuxu

Potřeba: CMake >=3.20, překladač C++17, SDL2 >=2.0.18, Make nebo Ninja. Na Debianu/Ubuntu lze nástroje nainstalovat pomocí `sudo apt install build-essential cmake libsdl2-dev`.

```sh
cmake -S native -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
./build/uaa-native --data "$PWD/native/data"
```

Uživatelská data se ukládají do uživatelského adresáře SDL, nikoliv do zdrojů. Vlastní sady a kampaně nejsou součástí tohoto repozitáře. [Návod Gamesetu](native/GAMESET-EDITOR.md).

## Volitelný mapeditor

Pro sestavení pouze hry použij `-DUAA_BUILD_MAPEDITOR=OFF`. Pro úplné sestavení editoru a původního balíku dat (Python 3):

```sh
python3 native/tools/mapeditor_data.py build \
  --graphics native/external/mapeditor-0.1.4/graphics \
  --config native/external/mapeditor-0.1.4/runtime/config.json \
  --font native/external/mapeditor-0.1.4/runtime/font.ttf \
  --output "$PWD/build/mapeditor/uaa-data.pak"
cmake -S native -B build -DUAA_MAPEDITOR_DATA="$PWD/build/mapeditor/uaa-data.pak"
cmake --build build -j2
./build/mapeditor/uaa-mapeditor --data "$PWD/build/mapeditor/uaa-data.pak"
```

Pro otevření editoru přímo ze hry potřebuje spustitelná hra sourozenecké složky `data`, `maps` a `mapeditor` jako v instalátoru. Pro vývoj lze spouštět hru a editor samostatně výše uvedenými příkazy. Pro vytvoření instalátorů je potřeba 32bitový překladač, NSIS pro Windows a `UAA_INSTALLERS=ON`; modul editoru s daty musí být před konfigurací připraven v `UAA_MAPEDITOR_ROOT`. Windows používá SDL2 pro MinGW/i686, nikoliv 64bitovou DLL.

## Stav vývoje

Pro vydané binárky prošlo 61 kontrol Linux32 a 61 Windows32 ve Wine. Windows instalátor byl ověřen ve Wine, nikoliv na fyzickém Windows. Port má nedokončený taktický boj a úplné vyhodnocení misí; nabídka Grafy zůstává prázdná. Hratelný gameset vyžaduje úplný základní katalog. Windows instalátor není digitálně podepsán.

Herní média jsou zde zveřejněna se souhlasem autora; tím se nemění jejich licenční podmínky. Tento repozitář začíná samostatnou veřejnou historií. Obsahuje kód, potřebná herní data a licence; soukromá vývojová historie, pracovní reporty a zálohy sem nepatří.
