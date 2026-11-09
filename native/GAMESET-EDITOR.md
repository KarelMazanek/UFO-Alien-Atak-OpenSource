# Editor gamesetu přímo v C++ hře

Editor má samostatnou plochu bez herního menu a bez lišty času a peněz. Akční tlačítka s ikonami jsou nahoře; dole zůstává stavový text. Řádek přímého přidávání/mazání položek a importu obrázku byl odstraněn na přání uživatele.

Platí pro 0.4.0-dev.57-local. Původní HTML a DUMMY jsou zachovány.

## Postup

1. V hlavním menu otevři **GAMESET**.
2. DUMMY je chráněný. **KOPÍROVAT** vytvoří upravitelnou vlastní kopii. Šipky vedle názvu přepínají mezi sadami. Kliknutí na název vlastní sady otevře přejmenování s potvrzením nebo zrušením. DUMMY nelze přejmenovat ani smazat. SMAZAT vedle KOPÍROVAT otevře potvrzení s názvem kopie a tlačítky SMAZAT/ZRUŠIT. Technické ID se nezadává. DUMMY smazat nelze. Potvrzené přejmenování a smazání se ukládá ihned.
3. Vyber kategorii a položku. Pravý panel zobrazuje všechna její pole. Kliknutí na skupinu otevře její vnitřek, na hodnotu otevře její úpravu. **ZPĚT** vrací o úroveň výš.
4. Například zbraň → **Výroba a ceny** → **Výrobní cena**, nebo **Bojové údaje** → **Časové jednotky** → **Od boku**. Křivky přesnosti obsahují body `[vzdálenost v metrech, přesnost v procentech]`; body měň v rostoucím pořadí vzdáleností. Při otevření konkrétní křivky je vlevo graf.
5. **ULOŽIT** uloží knihovnu vlastních sad. Přes **MENU** se vrať do hlavního menu a zvol **NOVÁ HRA**. V jejím dialogu vyber uloženou sadu kliknutím na název sady a potom spusť novou hru.

Text se zadává přímo, včetně dlouhých popisů. Ctrl+A vymaže aktuální hodnotu, Ctrl+V vloží text ze schránky. Enter potvrdí, Esc zruší. Boolean se přepíná kliknutím. Číslo musí být konečné, nezáporné a nejvýše 10 000 000; celočíselná pole nepřijímají desetinné hodnoty. Chyby a neuložené změny jsou vyznačené dole.

## Kategorie a práce s daty

Sada a překlady; budovy; jednotky; zbraně; vybavení; výzkum; UFO a vetřelci; mise a události; frakce; finance a státy. Dostupná jsou i vnořená pole, technické údaje, výrobní materiály, vlastnosti UFO, výzkumné podmínky a výsledky. Většina běžných polí má české i anglické popisky; méně běžná autorská pole používají původní klíč.

Záznamy a jejich vnořené hodnoty lze upravovat kliknutím na příslušné pole. Nad detailem objektu jsou PŘEJMENOVAT a SMAZAT OBJEKT. Obě akce vyžadují potvrzení a ukládají změnu ihned; přejmenování zachová technické ID. Objekty DUMMY nelze měnit ani mazat. Přidávání záznamů, polí, bodů a samostatný import obrázků nejsou v aktuálním rozhraní dostupné. Nové struktury a vložené obrázky lze přinést importem JSON. **ZPĚT KROK / ZNOVU** uchovává až 40 změn právě otevřené sady; při změně sady se historie kroků resetuje.

**NOVÁ SADA** vytvoří prázdnou autorskou sadu. Pro hraní použij úplnou kopii DUMMY: současný engine vyžaduje základní katalogové záznamy. **OVĚŘIT** zobrazí, co případně chybí. Neplatná data nelze uložit do knihovny; neúplnou, ale strukturálně platnou autorskou sadu lze uložit a exportovat, avšak nelze s ní spustit kampaň.

## Uložení, import a obrázky

Knihovna `gamesets.json` je v uživatelské složce hry, kterou určuje spouštěč (`~/.local/share/uaa-native-dev55`, nebo odpovídající XDG_DATA_HOME). Starší `gameset-custom.json` se při prvním načtení bez nové knihovny převezme, pokud je platný.

**IMPORT** vyžaduje cestu k souboru JSON. Podporuje samostatnou sadu, balíček `uaa-gameset-package-v1` i databázi se seznamem `gamesets`. Importované sady dostanou vlastní ID; DUMMY se nepřepisuje. **EXPORT** vytvoří `UAA-gameset-N.json` v uživatelské složce a ukáže cestu dole. Vlastní obrázky se v balíčku ukládají vložené, aby byly přenosné. Původní referenční obrázky používají assety přibalené ke hře.

Obrázky lze přinést importem JSON balíčku. Import dekóduje vložené PNG/JPG/BMP; WebP vyžaduje dostupnou knihovnu libwebp (libwebp.so.7 na Linuxu nebo libwebp.dll na Windows). Chybějící dekodér se ohlásí jako chyba importu. Maximální rozměry jsou 8192 × 8192. Samostatné tlačítko výběru obrázku bylo odstraněno.

## Použití v kampani a hranice

Nová kampaň používá vybraný katalog pro výrobní recepty, obchodní ceny, výzkum, popisy a bojové údaje. Změny počátečních zásob a počtů základních profesí se uplatní, pokud se liší od DUMMY. Nezměněná kopie zachovává místní počáteční zásoby dev.56. Změněná cena a doba budovy ovlivní stavbu a vrácení části ceny při demolici; schválené rozměry staveb zůstávají pravidly enginu. Finance vlastní sady určují seznam podporujících států, ekonomické třídy, pevný příspěvek a spokojenost; `null` znamená dynamický výpočet.

Sada se vloží do uložené kampaně jako `gamesetSnapshot`. Pozdější změny knihovny nemění rozehranou hru. Staré kampaně bez snapshotu používají referenční sadu. Poškozený snapshot nebo neúspěšné načtení nesmí změnit aktuální kampaň ani aktivní sadu.

Editor umožňuje vytvářet a uchovávat data misí, událostí a frakcí; jejich plné herní vyhodnocování není tímto krokem doplněno. Editor nenahrazuje samostatný mapeditor a nedokončuje dosud nepřevedené části taktického boje. Nové autorské typy budov nebo vozidel mohou vyžadovat doplnění enginu. Změna libovolného popisného pole sama o sobě nepřidává novou herní mechaniku.
