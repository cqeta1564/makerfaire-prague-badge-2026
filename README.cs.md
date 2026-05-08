# Maker Badge 2019 Game - PlatformIO

Česká verze README. English version: [README.md](README.md).

PlatformIO firmware pro běh hry ve stylu Maker Faire Prague 2019 badge na
ESP32-S2 Czech Maker Badge.

Firmware umí vyměňovat ID mezi badgi přes IR, ukládat potkané badge do NVS,
zobrazit spárovaná ID na e-paper displeji a na RGB LED.
Po dokončení akce se badge uspává do deep sleepu kvůli šetření baterie.
V běžném úsporném režimu se probouzí pouze resetem nebo odpojením a připojením
napájení.

## Co firmware umí

- Párování s jinou badge přes původní IR protokol z roku 2019
- Ukládání potkaných badge ID do ESP32-S2 Preferences/NVS
- Zobrazení uložených spárovaných/potkaných ID na e-paper displeji
- Použití čtyř onboard NeoPixel LED jako viditelného ID/tým kódu
- Odpovědi na dump požadavky pro gateway/debug nástroje
- Serial Monitor příkazy pro nastavení, testování, výpisy a uspání
- Napájení přidaného IR receiveru z existující spínané LED/NeoPixel větve
- Deep sleep po akcích a po timeoutu na idle obrazovce

## Struktura projektu

| Cesta | Účel |
| --- | --- |
| `platformio.ini` | PlatformIO prostředí a build flagy |
| `include/BadgeConfig.h` | Piny, konstanty úložiště a nastavení uspávání |
| `src/main.cpp` | Arduino `setup()` / `loop()` vstup |
| `src/GameLogic.*` | Párování, zobrazování, dump flow a idle timeout |
| `src/IrProtocol.*` | IR kódování, dekódování, vysílání a příjem |
| `src/BadgeDisplay.*` | E-paper a NeoPixel výstup |
| `src/BadgePower.*` | Finální low-power deep sleep sekvence |
| `src/BadgeStorage.*` | Trvalé uložení ID, týmu a potkaných badge |
| `src/SerialCommands.*` | Rozhraní pro Serial Monitor příkazy |

## Hardware

Základní Maker Badge deska nemá původní IR hardware z roku 2019. Je potřeba
doplnit krátkodosahovou IR LED a 38 kHz IR receiver.

Výchozí piny jsou v `include/BadgeConfig.h`:

| Funkce | Výchozí hodnota |
| --- | --- |
| IR vysílací LED | GPIO 13 (`PIN_IR_TX`) |
| IR receiver OUT | GPIO 7 (`PIN_IR_RX`) |
| IR receiver VCC | spínaná LED/NeoPixel `V+` větev |
| Spínání LED/NeoPixel napájení | GPIO 21 (`PIN_NEOPIXEL_PWR`) |
| NeoPixel data | GPIO 18 (`PIN_RGB_LED`) |
| Spínání e-paper napájení | GPIO 16 (`PIN_EPD_POWER`) |
| BOOT tlačítko | GPIO 0 |
| Touch 1 pro párování | GPIO 1 (`PIN_TOUCH_PAIR_1`) |
| Touch 2 pro párování | GPIO 2 (`PIN_TOUCH_PAIR_2`) |
| Touch 3 bez akce | GPIO 3 (`PIN_TOUCH_UNUSED_3`) |
| Touch 4 pro zobrazení spárovaných ID | GPIO 4 (`PIN_TOUCH_SHOW_1`) |
| Touch 5 pro zobrazení spárovaných ID | GPIO 5 (`PIN_TOUCH_SHOW_2`) |

## Zapojení IR

Přidané IR součástky zapoj takto:

| IR součástka | Zapojení |
| --- | --- |
| Anoda IR LED | GPIO 13 přes proudový odpor |
| Katoda IR LED | GND |
| 38 kHz receiver OUT | GPIO 7 |
| 38 kHz receiver VCC | spínaná LED/NeoPixel `V+` větev |
| 38 kHz receiver GND | GND |

Pokud záleží na nízké spotřebě ve sleepu, nepřipojuj VCC receiveru na trvalé
`3V3`. Firmware počítá s tím, že receiver sdílí existující spínanou LED/NeoPixel
větev. Tato větev je aktivní v logické nule: `LOW` znamená zapnuto, `HIGH`
znamená vypnuto.

## Revize desky

| Revize desky | LED / NeoPixel napájecí větev | Spínání e-paperu | IR receiver VCC |
| --- | --- | --- | --- |
| rev. A | GPIO 21 (`PIN_NEOPIXEL_PWR`) | bez samostatného GPIO spínače | LED/NeoPixel `V+` |
| rev. B | GPIO 21 (`PIN_NEOPIXEL_PWR`) | bez samostatného GPIO spínače | LED/NeoPixel `V+` |
| rev. C | GPIO 21 (`PIN_NEOPIXEL_PWR`) | bez samostatného GPIO spínače | LED/NeoPixel `V+` |
| rev. D | GPIO 21 (`PIN_NEOPIXEL_PWR`) | GPIO 16 (`PIN_EPD_POWER`) | LED/NeoPixel `V+` |

Pro rev. A badge se starším UC8151D / GDEW0213T5D displejem zapni
`MAKER_BADGE_REV_A_DISPLAY`.

## Chování napájení

Release firmware je nastavený agresivně na šetření baterie:

- po párování, zobrazení, dumpu, timeoutu nebo serial příkazu `S` badge přejde
  do finálního deep sleepu
- po `IDLE_SLEEP_TIMEOUT_MS` na ready obrazovce se badge také uspí
- probuzení je záměrně pouze resetem; není zapnutý žádný button/touch wake source
- před uspáním firmware vypne NeoPixely, LED/IR napájecí větev, e-paper
  napájení, IR interrupt handling a tam, kde to ESP32-S2 podporuje, i RTC sleep
  domény

Důležitá nastavení v `include/BadgeConfig.h`:

| Nastavení | Výchozí hodnota | Význam |
| --- | --- | --- |
| `SLEEP_AFTER_ACTION` | `1` | Uspat hned po dokončení akce |
| `IDLE_SLEEP_TIMEOUT_MS` | `60000UL` | Timeout ready obrazovky před uspáním |
| `PIN_IR_RX_POWER` | `PIN_NEOPIXEL_PWR` | Napájecí větev IR receiveru |
| `IR_RX_POWER_ACTIVE_LOW` | `1` | `LOW` zapíná IR receiver větev |

Při ladění nastav `SLEEP_AFTER_ACTION=0`, pokud nechceš, aby se badge uspala po
každé běžné akci. Nastavením `IDLE_SLEEP_TIMEOUT_MS=0` vypneš idle sleep.

## PlatformIO prostředí

| Prostředí | Použití |
| --- | --- |
| `maker_badge_esp32s2` | Normální release build |
| `maker_badge_debug` | Debug build s `SLEEP_AFTER_ACTION=0` a 15 s idle timeoutem |

Debug prostředí je praktické pro práci přes Serial Monitor, protože se badge po
běžných akcích vrátí na ready obrazovku místo okamžitého uspání.

## Build

Z této složky:

```powershell
python -m platformio run -e maker_badge_esp32s2
```

Debug build:

```powershell
python -m platformio run -e maker_badge_debug
```

Pokud je `pio` dostupné v `PATH`, jde použít i krátká varianta:

```powershell
pio run -e maker_badge_esp32s2
```

## Nahrání z prohlížeče

Release firmware jde nahrát přes GitHub Pages web flasher:

[Nahrát z prohlížeče](https://cqeta1564.github.io/makerfaire-prague-badge-2026/)

Použij desktopový Chrome nebo Microsoft Edge, připoj badge přes USB-C a zapni
ji. Pokud se sériový port neukáže, uveď ESP32-S2 do bootloader režimu a zkus to
znovu. Nejdřív zadej číslo badge; platné hodnoty jsou `1111` až `7777` a každá
číslice musí být `1` až `7`. Web flasher číslo zapíše při instalaci a používá
release build `maker_badge_esp32s2`.

## Upload

Připoj badge přes USB-C, zapni ji a spusť:

```powershell
python -m platformio run -e maker_badge_esp32s2 -t upload
```

Upload debug firmwaru:

```powershell
python -m platformio run -e maker_badge_debug -t upload
```

Pokud PlatformIO samo nenajde sériový port, přidej `upload_port` do
`platformio.ini` nebo ho předej v příkazu.

## Serial Monitor

Monitor otevřeš na 115200 baud:

```powershell
python -m platformio device monitor -b 115200
```

Příkazy:

| Příkaz | Význam |
| --- | --- |
| `?` | Informace o firmwaru |
| `H` | Výpis nápovědy |
| `I` | Přečíst aktuální badge ID |
| `Ixxxx` | Nastavit badge ID, čtyři hex číslice |
| `T` | Přečíst aktuální tým |
| `Tn` | Nastavit tým: `0` žádný, `1` červený, `2` zelený, `3` modrý |
| `C` | Přečíst počet potkaných badge |
| `D` | Dump raw bitmapy potkaných badge |
| `E` | Vypsat potkaná ID |
| `F!` | Formát/vymazání úložiště potkaných badge |
| `P` | Spustit párování |
| `V` | Zobrazit uložená spárovaná ID |
| `S` | Návrat do idle; release build se potom uspí |
| `Z` nebo `Z!` | Okamžitě přejít do deep sleepu |

## Ovládání na badge

| Vstup | Akce |
| --- | --- |
| Touch 1 nebo touch 2 na ready obrazovce | Spustit párování |
| Touch 3 na ready obrazovce | Bez akce |
| Touch 4 nebo touch 5 na ready obrazovce | Zobrazit uložená spárovaná ID |
| Touch 1 nebo touch 2 během zobrazení | Přepnout do párování |
| RESET tlačítko | Probudit z finálního deep sleepu |

## Typický testovací postup

1. Sestav a nahraj `maker_badge_debug`.
2. Otevři Serial Monitor na 115200 baud.
3. Pošli `?` nebo `H` a ověř, že firmware odpovídá.
4. Pošli `I` a `T` pro kontrolu ID a týmu.
5. Použij `P` nebo touch 1/2 pro spuštění párování.
6. Použij `V` nebo touch 4/5 pro zobrazení uložených spárovaných ID.
7. Pošli `Z` a ověř finální sleep cestu.
8. Stiskni RESET a badge znovu probuď.

## Troubleshooting

| Problém | Zkontroluj |
| --- | --- |
| IR receiver nikdy nevidí pakety | Receiver OUT musí být na GPIO 7 a napájený ze spínaného `V+` |
| IR receiver funguje jen když svítí LED | To je očekávané, receiver sdílí LED napájecí větev |
| Badge se uspává při ladění | Použij `maker_badge_debug` nebo nastav `SLEEP_AFTER_ACTION=0` |
| Badge se na ready obrazovce nikdy neuspí | Zkontroluj `IDLE_SLEEP_TIMEOUT_MS`; `0` timeout vypíná |
| Rev. A displej se zobrazuje špatně | Zapni `MAKER_BADGE_REV_A_DISPLAY=1` |
| Upload selhává | Zkontroluj USB-C kabel, power switch badge a vybraný sériový port |
| Serial Monitor se po akci odpojí | Release firmware přešel do deep sleepu; stiskni RESET nebo použij debug build |

## Licence a původ

Projekt je založený na Maker Faire Prague 2019 badge firmwaru a je publikovaný
pod MIT licencí. Při dalším šíření zachovej licenční a copyright oznámení.

## Poznámky

- IR receiver záměrně sdílí LED/NeoPixel napájecí větev. Samostatné spínání by
  vyžadovalo další hardware.
- Reálná spotřeba v deep sleepu závisí na konkrétní desce. U hotové badge ji
  změř, protože regulátor, pull-upy, moduly a zapojení mohou výsledek výrazně
  ovlivnit.
- Arduino IDE verze je držená samostatně v
  `SW/Arduino/examples/Maker_badge_2019_game`, ale tento PlatformIO projekt je
  hlavní vývojová verze.
