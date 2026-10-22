# Zephyr port

Plan, division of work and notes for porting P02 from ESP-IDF to Zephyr RTOS (v4.4.2).
Setup, build and flash instructions are in the [readme](../readme.md#zephyr-port).

---

## Goal

Run the existing application (`logic::Logic`) on Zephyr without changing it, by adding Zephyr
implementations of the driver interfaces and a `factory::Zephyr` – the same way `factory::Esp32s3` and
`factory::Stub` exist today.

---

## How Zephyr gets into the project (west, not submodules)

Zephyr is not a single repository: it is Zephyr plus ~80 modules (HALs such as `hal_espressif`,
libraries such as `mbedtls`). **west** fetches them at pinned versions from a manifest (`west.yml`) and
tells the build where they are. This is why Zephyr is not added as a git submodule.

| Topology | Manifest repository | Example |
|---|---|---|
| T1 | Zephyr itself | `~/zephyrproject` (what we use today) |
| T2 ("star") | The application | The course repository `~/zephyr` |
| T3 | A separate manifest repository | Large organisations with many applications |

**Today:** we build against the T1 workspace in `~/zephyrproject`.

**Planned:** make this repository a T2 workspace, like the course repository:

```
yrgo_hwc-_p02/
├── west.yml       ← Zephyr v4.4.2 + name-allowlist: [hal_espressif]
├── .west/         ← created locally by `west init -l .`, not committed
├── deps/          ← created by `west update`, in .gitignore
└── zephyr/        ← our Zephyr code
```

Then anyone (or CI) can get the exact same Zephyr with `west init -l . && west update`.

---

## Planned directory structure

```
zephyr/
├── boards/
│   └── nano_esp32/            ← adapts esp32s3_devkitc to the Nano ESP32, shared by all apps
│       ├── usb_console.overlay   (done)
│       └── leds.overlay          (exercise)
├── samples/                   ← our own learning exercises
│   └── blinky/
│       ├── CMakeLists.txt
│       ├── prj.conf
│       └── src/main.c
└── app/                       ← the P02 port itself
```

Later, `zephyr/boards/nano_esp32/` becomes a real out-of-tree board definition `arduino_nano_esp32`
(based on the schematic), and the overlays go away.

---

## Order of work

Each step maps to a lecture in the Zephyr course (`~/zephyr`): do the exercise on `native_sim` there
first, then apply it to P02 on the real board.

| # | Step | Course topic | Status |
|---|---|---|---|
| 0 | Environment, hello_world on the board | – | ✅ |
| 1 | Blinky on the yellow LED with our own overlay | Devicetree, GPIO | ⬜ |
| 2 | Own Zephyr app skeleton: `Logic` builds and runs with **only stubs** | West, build, Kconfig | ⬜ |
| 3 | `timer::Zephyr` | Time, timers, work queues | ⬜ |
| 4 | `gpio::Zephyr`, then pins from devicetree (`gpio_dt_spec`) instead of `9U` | Devicetree, GPIO | ⬜ |
| 5 | `serial::Zephyr` – or replace our command parser with Zephyr's shell (design decision) | Shell | ⬜ |
| 6 | `adc::Zephyr` + overlay – TMP36 then works as-is | Devicetree, driver model | ⬜ |
| 7 | `nvs` via the Settings subsystem | Settings | ⬜ |
| 8 | Own board definition `arduino_nano_esp32` | Devicetree, bindings | ⬜ |
| 9 | Own `west.yml` (T2 workspace) + Zephyr build in CI | West manifest | ⬜ |
| 10 | Wi-Fi and MQTT | Networking | ⬜ |

---

## How we work

* **One driver per branch and PR. The other person always reviews.** Reviewing each other's Zephyr code
  is half the learning.
* **Pair programming** for steps that set the structure (2, 8, 9, 10): one types (driver), one thinks and
  reads docs (navigator); switch every 25 minutes.
* **Parallel work** for drivers once the skeleton exists, e.g.:

  | | Person A | Person B |
  |---|---|---|
  | 1 | `timer::Zephyr` | `gpio::Zephyr` |
  | 2 | `serial::Zephyr` | `adc::Zephyr` + overlay |
  | 3 | `nvs` via Settings | Pins from devicetree |

* **Small PRs.** Three small ones are better than one big one.
* **Write down what you learn** in the notes below – every "aha", every error and how it was solved.

---

## Notes

### Board / hardware

* The Arduino Nano ESP32 is not in Zephyr. We use `esp32s3_devkitc/esp32s3/procpu` + overlays.
* The console must go over `usb_serial` (USB-Serial/JTAG) – the devkitc uses `uart0`.
* USB re-enumerates on every reset; with usbipd the board is gone for ~8 s. Use `CONFIG_BOOT_DELAY`.
* The devkitc definition says 8 MB flash, the Nano has 16 MB.

### Devicetree

* `DT_ALIAS(led0)` looks in the **final** devicetree (`build/zephyr/zephyr.dts`), which is the board's
  `.dts` + automatically found overlays (`boards/<board>.overlay`, `app.overlay`) + `EXTRA_DTC_OVERLAY_FILE`.
* A sample's `boards/<board_name_with_underscores>.overlay` is only used when building for exactly that
  board.

### Kconfig

* `-DCONFIG_X=y` on the command line works, but belongs in `prj.conf` so everyone builds the same way.

### Open questions

* Keep our own command parser or use Zephyr's shell?
* Why does the USB connection sometimes drop right after boot?
