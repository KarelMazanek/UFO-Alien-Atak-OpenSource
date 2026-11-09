# Grafika kosmických lokací

Model ISS (`iss.json`) je polygonová geometrie funkcí `build`, `beam`, `cylinder` a `panel` z kreslení `iss3dScene` v původním `game/UFO-ALIEN-ATAK-v0.3.0.html`. Zachovány souřadnice i barvy.

Model Ariane 64 (`kourou.json`) je geometrie funkcí `pad`, `gantry`, `mast` a `rocket` původního `kourouModelCanvas`. Nativní vykreslení používá stejnou paletu, řazení polygonů a původní projekci podle velikosti panelu.

Florida SVG je původní `rocketScene kennedyRocket`; PNG je jeho průhledný rastr. Bajkonur používá původní nákladní variantu stejné kresby (viditelný náklad, skryté okno a rameno posádky).

Mapový podklad používá existující původní geografická data. Graf výšek čte aktuální `satelliteStatus`. Dráha ISS používá referenčních 92,68 min a sklon 51,6°. Satelitní dráhy jsou označené jako schematické: nativní uložená hra dosud neobsahuje úplné orbitální elementy ani historii skenování původní HTML simulace. Nejde o mapu skutečně proskenovaného území. Od dev.38 má ISS provozní simulaci popsanou níže.

Původní HTML zůstává beze změn.

Od dev.37 jsou kamera a gesta převzaty z původních skriptů: ISS yaw .68, pitch .34, distance 27 (16–48); Kourou yaw .34, pitch .36, zoom 1.5 (.6–1.7). Ovládání: tažení, kolečko, −/RESET/+, dvojklik; ISS také dvouprstový pinch. Implementace `src/space_view.inc`, obrazové a vstupní kontroly `verifySpaceGestures`.

## Dev.38 – střediska, zásobování a provoz ISS

- `houston-layout.json` přebírá `houstonGridLayoutV2`: ubytování [0,0], [2,0], datové centrum [0,2], elektrárna [3,2], velitelství [0,4], výcvik posádek [3,4]. Dvě buňky po čtyřech místech jsou dokončené a účtují se jednou dle HTML. Migrace zachová starší již zaplacené buňky.
- `control-layouts.json`: ESA má dva obytné objekty, šest skladů, dílnu, laboratoř, elektrárnu a datové centrum; Koroljov dva obytné objekty, čtyři sklady, velitelství, datové centrum, elektrárnu, dílnu a laboratoř. Bajkonur zůstává samostatnou startovní rampou.
- ISS/Ariane otevírá vlastní SDL okno se samostatným rendererem, texturami a vstupními oblastmi. Zavření modelu neukončuje hru. Ovládání kamer odpovídá HTML.
- Mapa přeletů používá existující schválený reliéf `globe/surface.png`, původní geografická data a stopy navazujících oběhů. Satelitní dráhy zůstávají schematické, ISS používá referenční sklon a periodu. Meteorologická vrstva je herní simulace, nikoli skutečná předpověď pro rok 2042. Mapu a pravidla startů pohání stejná funkce.
- Výslovná uživatelská odchylka: sedm stálých obyvatel namísto původních tří. Jde o fiktivní herní osoby, nikoli o seznam skutečné aktuální expedice. Běžná velikost sedm je doložena NASA: https://www.nasa.gov/international-space-station/space-station-facts-and-figures/ . Dva vědci a tři technici mají oddělenou evidenci práce; velitel a lékař zajišťují provoz.
- `iss-operations.json` uchovává pravidla simulace. Výchozí životní zásoby a dodávky jsou přepočítané poměrem 7/3, aby sedmičlenná posádka měla stejnou počáteční zásobu na osobu jako v HTML. Původní sazby spotřeby, nouzový násobek 0,35, korekce dráhy a intervaly/pravděpodobnosti zásahů pocházejí z HTML. Individuální stárnutí systémů, následky úlomků, opravy a meteorologické rezervování letů jsou doplněné dle nového zadání.
- Hodinové zpracování ukládá stav systémů, rezervy, opravy, lety, deník i samostatný náhodný stav zásahů. Poškození podpory života, nedostatek zásob či napájení zastaví vědeckou práci, zatímco zásadní opravy pokračují. Režim není jen popisek.
- Pravidelný cyklus 14 dnů lze směrovat přes Floridu či Bajkonur. Špatné počasí odkládá start, kontrola probíhá po 6 h. Při blížícím se vyčerpání zásob se zohlední let a konzervativní rezerva; kritický let odstartuje i při nevhodném počasí. Peníze a pronájmy zůstávají skutečnými podmínkami a jejich nedostatek je zobrazený. Zásilky se platí jen jednou, doplní zásoby až při příletu a nedoručují se opakovaně.
- Datová struktura ISS obsahuje moduly, rodiče a připojovací porty pro budoucí rozšíření. Nové moduly zatím nemají nákupní ani připojovací rozhraní.

## Dev.39: prostorové detaily a popisky

`surface-details.json` je samostatná vrstva doplňkových polygonů. Nemění původní `iss.json` ani `kourou.json`. Kamera řadí plochy podle promítnuté hloubky; osvětlení a tlumené obrysy zvýrazňují členění. Číslované značky používají souřadnice modelu; názvy a účel jsou dostupné česky i anglicky.

`florida-detailed.svg/png` a `baikonur-detailed.svg/png` jsou místní odvozeniny originálů s věrně vykreslenými přechody a doplněnými spárami. Původní soubory zůstávají zachovány. Statická vyobrazení ramp nemají interaktivní 3D kameru. Ariane má výchozí celé zobrazení, při ručním přiblížení lze zkoumat detail.

## Dev.40: satelitní snímky a pozorování UFO

`state.satelliteScans` ukládá samostatné EARTH/WATER/AIR mapy buněk `row:col`, 36 řad a 72 sloupců. Každá má čas `at` v herních hodinách, číslo satelitu a kontakty pozorované při snímkování. Prázdná buňka není důkazem nepřítomnosti UFO. Čas snímku není časem otevření obrazovky.

Průzkum používá kruhové herní dráhy s náklonem 85°, dvouminutový interval, poloměr pokrytí z původního HTML a platnost pronájmu ESA v čase snímku. Jde o herní simulaci, nikoli skutečné družicové snímky. Přeletové čáry satelitů se nekreslí. ISS si ponechává vlastní mapu přeletů. Filtry oddělují typ senzoru; stejně jako původní HTML mohou ukládat pokrytí nad celým glóbem, kontakty se však ověřují podle prostředí AIR/LAND/SEA.

`ufoSites` obsahuje skutečná místa dopadu po úspěšném sestřelení (72 h), klasifikovaná stávajícími zeměpisnými daty. Taktické vyzvednutí nebylo přidáno. Kontakty na snímcích jsou historické; po 72 h mizí jejich mapové značky. Novější snímek bez záchytu nahradí předchozí záznam v dané vrstvě. Starší uložená hra zahajuje průzkum až od aktuálního času, bez zpětného vymýšlení snímků.

## Dev.41: aktivní dosedání a radary

U části přilétajících UFO se vytvoří plán `landingAt` / `takeoffAt` a `surfaceEnvironment`. Prostředí v čase snímku určuje `ufoEnvironmentAt`, nikoli aktuální koncový stav velkého časového kroku. Po dosednutí jde stále o nepoškozené UFO, které může odletět; `ufoSites` je vyhrazeno troskám.

Dokončené povrchové radary zachycují vzdušná i dosedlá UFO. `radar-ranges.json` určuje dosah 2 500 / 5 000 km dle nového zadání (odchylka od původního HTML). `radarContacts` uchovává pozorování s časem a zdrojem. Satelitní a radarová pozorování se slučují podle identity a času; filtr prostředí je zachován. Dosah je herní pravidlo, nikoli model fyziky radarového šíření pod vodou.
