# P02 - drivers för ESP32-S3

* Driverbibliotek i C++17 för mikrocontrollern `ESP32-S3`, byggt på **ESP-IDF 6.0**
* Målkort: **Arduino Nano ESP32** (ESP32-S3, 16 MB flash)
* Genomförs i kursen Hårdvarunära programmering C/C++
* Pågår: portering till **Zephyr RTOS** (se [Zephyr-portering](#zephyr-portering))

---

## Innehåll

* [Arkitektur](#arkitektur)
* [Katalogstruktur](#katalogstruktur)
* [Drivrutiner](#drivrutiner)
* [Hårdvara och inkoppling](#hårdvara-och-inkoppling)
* [Kommandon via seriellt gränssnitt](#kommandon-via-seriellt-gränssnitt)
* [Utvecklingsmiljö (WSL + ESP-IDF)](#utvecklingsmiljö-wsl--esp-idf)
* [Bygga](#bygga)
* [Flasha och monitor](#flasha-och-monitor)
* [Felsökning](#felsökning)
* [Tester](#tester)
* [Kodstil och CI](#kodstil-och-ci)
* [Zephyr-portering](#zephyr-portering)

---

## Arkitektur

Varje drivrutin består av tre delar:

| Del | Fil | Syfte |
|---|---|---|
| Gränssnitt | `driver/<namn>/interface.h` | Abstrakt klass (`Interface`) som logiken programmerar mot |
| Implementation | `driver/<namn>/esp32s3.h/.cpp` | Riktig drivrutin mot ESP-IDF |
| Stub | `driver/<namn>/stub.h` | Simulerad drivrutin för test utan hårdvara |

Drivrutinerna skapas via en **fabrik** (`driver::factory::Interface`) med en implementation per plattform
(`factory::Esp32s3`, `factory::Stub`). Applikationslogiken (`logic::Logic`) känner bara till gränssnitten,
vilket gör att samma logik kan köras mot riktig hårdvara, stubbar eller en annan plattform (t.ex. Zephyr)
genom att byta fabrik.

```
main.cpp ──► logic::Logic ──► factory::Interface ──┬──► factory::Esp32s3 ──► driver::*::Esp32s3 ──► ESP-IDF
                                                   └──► factory::Stub    ──► driver::*::Stub
```

Pinnar hanteras av `driver::pin`:
* `driver/pin/esp32s3.h` mappar Arduino-namn (`A0`–`A7`, `D0`–`D13`) till ESP32-S3 GPIO-nummer.
* `driver/pin/manager.h` (`PhysicalPinManager`) håller reda på vilka fysiska pinnar som är upptagna så att
  två drivrutiner inte kan ta samma pinne.

---

## Katalogstruktur

```
.
├── CMakeLists.txt            # ESP-IDF-projektfil
├── dependencies.lock         # Låsta versioner (ESP-IDF 6.0, espressif/mqtt)
├── sdkconfig                 # ESP-IDF-konfiguration (idf.py menuconfig)
├── main/
│   ├── CMakeLists.txt        # Lista över källfiler + ESP-IDF-komponenter
│   ├── idf_component.yml     # Externa komponenter (espressif/mqtt)
│   ├── include/
│   │   ├── arch/env/         # Val av plattform (riktig hårdvara / testmiljö)
│   │   ├── communication/    # wifi, mqtt (publish/subscribe)
│   │   ├── driver/           # adc, factory, gpio, nvs, pin, serial, tempsensor, timer
│   │   ├── logic/            # Applikationslogik (kommandotolk)
│   │   └── ml/               # Linjär regression
│   └── source/               # Implementationer, samma struktur som include/
├── tests/                    # Testsvit som körs på PC (g++) med mockad hårdvara
├── tools/                    # Skript för clang-format
├── docs/                     # Övrig dokumentation (t.ex. git_commands.md)
└── .github/workflows/ci.yml  # CI: formatkontroll + bygge på Linux och Windows
```

---

## Drivrutiner

| Modul | Namespace | ESP-IDF-API | Kommentar |
|---|---|---|---|
| GPIO | `driver::gpio` | `driver/gpio.h` | Output, input med pull-up/pull-down, `toggle()` |
| ADC | `driver::adc` | `esp_adc/adc_oneshot.h` + kalibrering | Dämpning väljs automatiskt utifrån `referenceVoltage` |
| Serial | `driver::serial` | `driver/usb_serial_jtag.h` | Via kortets USB-port, `readLine()` är icke-blockerande |
| Timer | `driver::timer` | `esp_timer.h` | Mjukvarutimer, pollas med `timeout()` |
| NVS | `driver::nvs` | `nvs_flash.h` | Strängar + heltal (template), nycklar max 15 tecken |
| Tempsensor | `driver::tempsensor` | – | `Tmp36` använder en `adc::Interface`, plattformsoberoende |
| Wi-Fi | `comm::wifi` | `esp_wifi.h` | Skannar och ansluter till konfigurerat nätverk |
| MQTT | `comm::mqtt::publish/subscribe` | `espressif/mqtt` | Publicera och prenumerera på topics |
| ML | `ml::lin_reg` | – | Adaptiv linjär regression |

---

## Hårdvara och inkoppling

| Funktion | Arduino-pinne | ESP32-S3 GPIO | Anmärkning |
|---|---|---|---|
| Lysdiod | D6 | GPIO9 | `create_gpio(9U, ...)` i `logic.cpp` |
| TMP36 | A6 | GPIO13 | ADC2, **fungerar inte samtidigt som Wi-Fi** |

> **OBS:** ADC2 (A4–A7) kan inte användas när Wi-Fi är aktivt. Använd A0–A3 (ADC1) för analoga givare
> om Wi-Fi ska användas.

Fullständig pinmappning finns i [`main/include/driver/pin/esp32s3.h`](main/include/driver/pin/esp32s3.h).

---

## Kommandon via seriellt gränssnitt

Anslut med `idf.py -p /dev/ttyACM0 monitor` och skriv:

| Kommando | Funktion |
|---|---|
| `help` | Lista kommandon |
| `on` / `off` | Tänd / släck lysdioden |
| `blink on` / `blink off` | Starta / stoppa blinkning |
| `period <ms>` | Sätt blinkperiod, t.ex. `period 500` |
| `status` | Visa blinkläge, period och temperatur |
| `store` | Spara blinkläge och period i NVS (läses in vid uppstart) |

---

## Utvecklingsmiljö (WSL + ESP-IDF)

Rekommenderad miljö är **Ubuntu 24.04 i WSL2** med ESP-IDF 6.0. USB-kortet lånas ut från Windows till WSL
med **usbipd-win**.

### 1. WSL och usbipd (Windows, PowerShell som administratör)

```powershell
wsl --install -d Ubuntu-24.04
winget install --exact dorssel.usbipd-win
```

Starta om datorn och öppna Ubuntu från startmenyn.

### 2. Paket i Ubuntu

```bash
sudo apt update && sudo apt install -y --no-install-recommends git cmake ninja-build gperf ccache dfu-util device-tree-compiler wget flex bison python3-dev python3-pip python3-venv xz-utils file make gcc g++ gcc-multilib g++-multilib libffi-dev libssl-dev libusb-1.0-0 libsdl2-dev libmagic1 usbutils clang-format
```

Ge din användare rätt att använda serieportar (gäller först efter `wsl --shutdown` i PowerShell):

```bash
sudo usermod -aG dialout $USER
```

> Lägg alla projekt i Linux-hemkatalogen (`~/...`), **inte** under `/mnt/c/...` – byggen över
> Windows-filsystemet är många gånger långsammare.

### 3. ESP-IDF 6.0

ESP-IDF är ett ramverk som byggs från källkod tillsammans med projektet, därför måste det finnas lokalt.
En grund klon (utan historik) räcker:

```bash
mkdir -p ~/esp
git clone -b v6.0 --depth 1 --recursive --shallow-submodules https://github.com/espressif/esp-idf.git ~/esp/esp-idf
cd ~/esp/esp-idf && ./install.sh esp32s3
```

`install.sh` laddar ner kompilatorn och skapar Python-miljön i `~/.espressif/`. Lägg gärna till ett alias:

```bash
echo "alias get_idf='. ~/esp/esp-idf/export.sh'" >> ~/.bashrc
```

### 4. Hämta repot och Wi-Fi-konfiguration

```bash
cd ~
git clone --recursive git@github.com:guszahrma/yrgo_hwc-_p02.git
cd yrgo_hwc-_p02
cp main/include/communication/wifi/esp32s3_local_config_template.h main/include/communication/wifi/esp32s3_local_config.h
```

Fyll i SSID och lösenord i `esp32s3_local_config.h`. Filen är ignorerad av git och ska **aldrig** checkas in.

### Alternativ: ESP-IDF direkt i Windows

Installera ESP-IDF 6.0 med Espressifs installerare och öppna ESP-IDF PowerShell
(`C:\Espressif\tools\Microsoft.v6.0.PowerShell_profile.ps1`). Vid behov:
`Set-ExecutionPolicy RemoteSigned -Scope CurrentUser`. Kommandona nedan är desamma, men porten heter `COMx`
i stället för `/dev/ttyACM0`.

---

## Bygga

Kör alltid från **projektets rotkatalog** (inte inne i `~/esp/esp-idf`):

```bash
get_idf                       # en gång per terminal
cd ~/yrgo_hwc-_p02
idf.py set-target esp32s3     # bara första gången / efter fullclean
idf.py build
```

Konfiguration (t.ex. flashstorlek) ändras med `idf.py menuconfig`.

---

## Flasha och monitor

### 1. Koppla kortet till WSL (PowerShell som administratör)

```powershell
usbipd list                                    # hitta kortets BUSID, t.ex. 6-4
usbipd bind --busid 6-4                        # en gång per enhet och port
usbipd attach --wsl --busid 6-4 --auto-attach  # låt fönstret vara öppet
```

`--auto-attach` behövs eftersom ESP32-S3 kopplar ner och upp USB vid varje reset/flashning.

Kontrollera i WSL:

```bash
lsusb | grep -iE "303a|2341"; ls /dev/ttyACM*
```

| USB-ID | Betydelse |
|---|---|
| `2341:0070` | Arduinos fabriksfirmware – esptool fungerar **inte**, se nedan |
| `303a:1001` | ESP32-S3 USB-Serial/JTAG – redo att flasha |

### 2. Första gången: ta kortet ur Arduino-läget

Nano ESP32 levereras med Arduinos bootloader. Inför första flashningen:

1. Koppla pinnen **B1 till GND**.
2. Tryck på **reset** – kortet startar i ROM-bootloadern och syns som `303a:1001`.
3. Kör `usbipd list` igen; om enheten inte är delad, kör `usbipd bind` + `attach` på nytt.
4. Flasha, ta bort sladden och tryck reset.

Därefter kan esptool själv sätta kortet i flashläge.

> För att återställa Arduino-bootloadern: B1 till GND + reset, och kör sedan *Burn Bootloader* i Arduino IDE.

### 3. Flasha

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

Avsluta monitorn med `Ctrl+]`. Om monitorn inte hittar porten direkt efter flashning (kortet håller på att
kopplas in igen), vänta några sekunder och kör `idf.py -p /dev/ttyACM0 monitor`.

### Byta mellan ESP-IDF och Zephyr

Plattformarna har olika partitionslayout. Radera hela flashminnet när du byter:

```bash
idf.py -p /dev/ttyACM0 erase-flash
```

---

## Felsökning

| Fel | Orsak / lösning |
|---|---|
| `ESP-IDF Python virtual environment ... not found` | `install.sh` har inte körts klart. Kör `cd ~/esp/esp-idf && ./install.sh esp32s3`. |
| `libusb-1.0.so.0: cannot open shared object file` (vid install) | `sudo apt install -y libusb-1.0-0`, kör sedan `install.sh` igen. |
| `Current directory '.../esp-idf' is not buildable` | Du står i ESP-IDF-katalogen. `cd ~/yrgo_hwc-_p02`. Ta bort `~/esp/esp-idf/build`. |
| CMake använder `Unix Makefiles` i stället för Ninja | `ninja-build` saknas: `sudo apt install -y ninja-build`. |
| `Could not open COM3` | Windows-portnamn i WSL. Använd `/dev/ttyACM0`. |
| `Path '/dev/ttyACM0' is not readable` | Användaren saknar gruppen `dialout`. Se steg 2 ovan, kör sedan `wsl --shutdown` i PowerShell. |
| `usbipd: error: Device is not shared` | Kör `usbipd bind --busid <id>` som administratör först. |
| Ingen `/dev/ttyACM*` i WSL | Kontrollera att `usbipd attach` körs, och att kortet inte är i Arduino-läge (`2341:0070`). |

---

## Tester

Testsviten i `tests/` byggs med vanlig `g++` och kör drivrutinerna mot mockad hårdvara
(`TESTSUITE` definieras, vilket väljer `arch/env/test/hw_platform.h`). Kör **inte** i en terminal där
`export.sh` är aktiv:

```bash
cmake -S tests -B tests/build
cmake --build tests/build
```

Testerna körs automatiskt som en del av bygget (target `run`).

---

## Kodstil och CI

Koden formateras med `clang-format` enligt `.clang-format`.

```bash
# Linux / WSL
clang-format --dry-run --Werror --style=file $(git ls-files '*.c' '*.cpp' '*.h')   # kontrollera
clang-format -i --style=file $(git ls-files '*.c' '*.cpp' '*.h')                    # formatera
```

I Windows finns motsvarande skript: `tools/clang-format-check.ps1` och `tools/clang-format-fix.ps1`.

CI (`.github/workflows/ci.yml`) kör vid varje push och pull request:
* formatkontroll med clang-format
* bygge med ESP-IDF 6.0 på Ubuntu (Docker-image `espressif/idf:release-v6.0`) och Windows

Git-kommandon och SSH-uppsättning finns i [`docs/git_commands.md`](docs/git_commands.md).

---

## Zephyr-portering

Arbete pågår med att porta drivrutinerna till Zephyr RTOS (kort: `arduino_nano_esp32/esp32s3/procpu`).
Tack vare fabriksmönstret behöver `logic::Logic` inte ändras – i stället läggs Zephyr-implementationer
till bredvid ESP32-S3-implementationerna, plus en `factory::Zephyr`.

Planerad ordning:

1. Timer, GPIO och Serial på Zephyr – stubbar för ADC, NVS och tempsensor
2. ADC (devicetree-overlay) – TMP36 fungerar då direkt
3. NVS via Zephyrs Settings-subsystem
4. Wi-Fi och MQTT

Instruktioner för Zephyr-miljön läggs till här när porteringen kommit igång.
